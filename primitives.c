#include <mpich/mpi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/**
 * @name API_STATUS_INTERNAL
 * @brief Convenience macro to check predicate, log error on console
 * and return
 */
#define API_STATUS_INTERNAL(expr, code_block, ...)                             \
  do {                                                                         \
    if (expr) {                                                                \
      printf(__VA_ARGS__);                                                     \
      code_block;                                                              \
    }                                                                          \
  } while (0)

#define API_STATUS(rv, code_block, ...)                                        \
  API_STATUS_INTERNAL(((rv) < 0), code_block, __VA_ARGS__)

#define EXT_API_STATUS(exp, code_block, ...)                                   \
  API_STATUS_INTERNAL(exp, code_block, __VA_ARGS__)

#define API_NULL(obj, code_block, ...)                                         \
  API_STATUS_INTERNAL(((obj) == NULL), code_block, __VA_ARGS__)

static inline void print_buf(void *buf, size_t nbytes, int rank) {
  size_t i = 0;
  printf("\n============[%zu bytes RANK %d START]===========\n", nbytes, rank);
  for (i = 0; i < nbytes; i++) {
    if (i % 8 == 0) {
      printf("\n");
      printf("[Rank(%d) 0x%lx]: ", rank, i);
    }

    printf("%02x \t", *(uint8_t *)(buf + i));
  }

  printf("\n============[RX %zu bytes RANK %d END]=============\n", nbytes,
         rank);
}

static inline int randomize_buf(void **buf, size_t nbytes) {
  size_t i = 0;
  srand(time(NULL));
  for (i = 0; i < nbytes; i++) {
    *(uint8_t *)(*buf + i) = (rand() % 256);
  }

  return (0);
}

static inline int fill_buf(void **buf, size_t nbytes, int pattern) {
  size_t i = 0;
  for (i = 0; i < nbytes; i++) {
    *(uint8_t *)(*buf + i) = (pattern % 256);
  }

  return (0);
}

static inline int add_buf(void **src, void **dst, size_t size) {
  // Can be optimized for cache-line and 8B add using uint64_t
  for (int i = 0; i < size; i++) {
    *(uint8_t *)(*dst + i) = *(uint8_t *)(*src + i) + *(uint8_t *)(*dst + i);
  }

  return (0);
}

static int ring_allreduce(MPI_Comm new_comm, void *send_buf, size_t send_size,
                          void *recv_buf, size_t recv_size) {
  MPI_Request recv_req;
  MPI_Status status;
  int rank = -1, recv_rank = -1, send_rank = -1;
  int size = 0;

  // TODO: Add error handling support
  MPI_Comm_rank(new_comm, &rank);
  MPI_Comm_size(new_comm, &size);
  printf("Rank [%d], World Size [%d]\n", rank, size);
  fill_buf(&send_buf, send_size, 1);
  randomize_buf(&recv_buf, recv_size);

  // Ring: LEFT => RANK => RIGHT
  // Recv from (rank - 1 + size) % size
  // Send to (rank + 1) % size
  recv_rank = (rank - 1 + size) % size;
  send_rank = (rank + 1) % size;
  size_t chunk_size = send_size / size;
  void *recv_chunk = NULL;
  void *send_chunk = NULL;
  void *org_chunk = NULL;

  // Now start ring. At every step, for every rank, we iterate through
  // segments with wraparound and send and recv from our neighbors and reduce
  // locally. At the i'th iteration, sends segment (rank - i) and receives
  // segment (rank - i - 1).

  /** The scatter-reduce is done in N-1 steps. In the ith step, node j will send
   *  the (j - i)th chunk and receive the (j - i - 2)th chunk, adding it in to
   *  its existing data for that chunk.
   **/
  for (int i = 0; i < size - 1; i++) {
    recv_chunk = (void *)((char *)(recv_buf) +
                          ((rank - i - 1 + size) % size) * chunk_size);
    send_chunk =
        (void *)((char *)(send_buf) + ((rank - i + size) % size) * chunk_size);
    org_chunk = (void *)((char *)(send_buf) +
                         ((rank - i - 1 + size) % size) * chunk_size);

    MPI_Irecv(recv_chunk, chunk_size, MPI_UINT8_T, recv_rank, 0, new_comm,
              &recv_req);
    MPI_Send(send_chunk, chunk_size, MPI_UINT8_T, send_rank, 0, new_comm);
    MPI_Wait(&recv_req, &status);

    add_buf(&recv_chunk, &org_chunk, chunk_size);
  }

  for (int i = 0; i < size - 1; i++) {
    recv_chunk =
        (void *)((char *)(send_buf) + ((rank - i + size) % size) * chunk_size);
    send_chunk = (void *)((char *)(send_buf) +
                          ((rank + 1 - i + size) % size) * chunk_size);

    MPI_Irecv(recv_chunk, chunk_size, MPI_UINT8_T, recv_rank, 0, new_comm,
              &recv_req);
    MPI_Send(send_chunk, chunk_size, MPI_UINT8_T, send_rank, 0, new_comm);
    MPI_Wait(&recv_req, &status);
  }

  print_buf(send_buf, send_size, rank);
  return 0;
}

static int ring_allgather(MPI_Comm new_comm, void *send_buf, size_t send_size,
                          void *recv_buf, size_t recv_size) {
  MPI_Request recv_req;
  MPI_Status status;
  int rank = -1, recv_rank = -1, send_rank = -1;
  int size = 0;

  // TODO: Add error handling support
  MPI_Comm_rank(new_comm, &rank);
  MPI_Comm_size(new_comm, &size);
  printf("Rank [%d], World Size [%d]\n", rank, size);
  fill_buf(&send_buf, send_size, rank + 1);
  MPI_Barrier(new_comm);
  print_buf(&send_buf, send_size, rank);
  // Ring: LEFT => RANK => RIGHT
  // Recv from (rank - 1 + size) % size
  // Send to (rank + 1) % size
  recv_rank = (rank - 1 + size) % size;
  send_rank = (rank + 1) % size;
  size_t chunk_size = send_size / size;
  void *recv_chunk = NULL;
  void *send_chunk = NULL;

  for (int i = 0; i < size - 1; i++) {
    recv_chunk = (void *)((char *)(send_buf) +
                          ((rank - i - 1 + size) % size) * chunk_size);
    send_chunk =
        (void *)((char *)(send_buf) + ((rank - i + size) % size) * chunk_size);

    MPI_Irecv(recv_chunk, chunk_size, MPI_UINT8_T, recv_rank, 0, new_comm,
              &recv_req);
    MPI_Send(send_chunk, chunk_size, MPI_UINT8_T, send_rank, 0, new_comm);
    MPI_Wait(&recv_req, &status);
  }

  print_buf(send_buf, send_size, rank);
  return 0;
}

static int send_recv(MPI_Comm new_comm, void *send_buf, size_t send_size,
                     void *recv_buf, size_t recv_size) {
  MPI_Request recv_req;
  MPI_Status status;
  int rank = -1, recv_rank = -1, send_rank = -1;
  int size = 0;

  // TODO: Add error handling support
  MPI_Comm_rank(new_comm, &rank);
  MPI_Comm_size(new_comm, &size);
  printf("Rank [%d], World Size [%d]\n", rank, size);
  fill_buf(&send_buf, send_size, rank + 1);
  randomize_buf(&recv_buf, recv_size);

  // Ring: LEFT => RANK => RIGHT
  // Recv from (rank - 1 + size) % size
  // Send to (rank + 1) % size
  recv_rank = (rank - 1 + size) % size;
  send_rank = (rank + 1) % size;

  MPI_Irecv(recv_buf, recv_size, MPI_UINT8_T, recv_rank, 0, new_comm,
            &recv_req);
  MPI_Send(send_buf, send_size, MPI_UINT8_T, send_rank, 0, new_comm);
  MPI_Wait(&recv_req, &status);

  print_buf(recv_buf, recv_size, rank);

  return 0;
}

static int bcast(MPI_Comm new_comm, void *send_buf, size_t send_size,
                 void *recv_buf, size_t recv_size) {
  MPI_Request recv_req;
  MPI_Request *send_req;
  MPI_Status status;
  int rank = -1;
  int size = 0;

  // TODO: Add error handling support
  MPI_Comm_rank(new_comm, &rank);
  MPI_Comm_size(new_comm, &size);
  send_req = malloc(sizeof(MPI_Request) * size);
  printf("Rank [%d], World Size [%d]\n", rank, size);
  fill_buf(&send_buf, send_size, rank + 1);
  randomize_buf(&recv_buf, recv_size);

  // Radix N-ary tree: ROOT => LEAF1, LEAF2 ... LEAFN-1
  // Rank0 is root, replicating the data to its leaves
  // Rank1 to N-1 are leaves receiving the data from the root
  if (rank == 0) {
    for (int r = 1; r < size; r++) {
      MPI_Isend(send_buf, send_size, MPI_UINT8_T, r /* send rank */, 0,
                new_comm, &send_req[r]);
    }

    for (int r = 1; r < size; r++) {
      MPI_Wait(&send_req[r], &status);
    }

  } else {
    MPI_Irecv(recv_buf, recv_size, MPI_UINT8_T, 0 /* recv rank */, 0, new_comm,
              &recv_req);
    MPI_Wait(&recv_req, &status);
    print_buf(recv_buf, recv_size, rank);
  }

  return 0;
}

int main(int argc, char *argv[]) {
  MPI_Comm new_comm;
  size_t send_size = 64;
  size_t recv_size = 64;
  void *send_buf = malloc(sizeof(char) * send_size);
  void *recv_buf = malloc(sizeof(char) * recv_size);
  int rc = 0;

  EXT_API_STATUS(
      MPI_Init(&argc, &argv) != MPI_SUCCESS, { return (-1); },
      "Unable to initialize MPI\n");

  EXT_API_STATUS(
      MPI_Comm_dup(MPI_COMM_WORLD, &new_comm) != MPI_SUCCESS,
      {
        MPI_Finalize();
        return (-1);
      },
      "Unable to duplicate COMM_WORLD communicator\n");

  // Primitive-1: rank0 send -> recv rank1 send -> recv rank2 ring
  API_STATUS(
      send_recv(new_comm, send_buf, send_size, recv_buf, recv_size),
      {
        goto cleanup_buf_comm;
        rc = -1;
      },
      "Unable to perform send-recv ring primitive\n");

  // Primtive-2: rank0 send -> recv rank1 && recv rank2
  API_STATUS(
      bcast(new_comm, send_buf, send_size, recv_buf, recv_size),
      {
        goto cleanup_buf_comm;
        rc = -1;
      },
      "Unable to perform bcast primitive\n");

  // Primtive-3: rank0 send 0th (size/nranks) chunk to rank1, rank1 sends 1st
  // (size/nrank) chunk to rank2, .... Simultaneously, rank0 receives N-1th
  // (size/nrank) chunk from rankN-1, adds to existing chunk in-place
  API_STATUS(
      ring_allreduce(new_comm, send_buf, send_size, recv_buf, recv_size),
      {
        goto cleanup_buf_comm;
        rc = -1;
      },
      "Unable to perform scatter_reduce primitive\n");

  // Primtive-4: rank0 send 0th (size/nranks) chunk to rank1, rank1 sends 1st
  // (size/nrank) chunk to rank2, .... Simultaneously, rank0 receives N-1th
  // (size/nrank) chunk from rankN-1
  API_STATUS(
      ring_allgather(new_comm, send_buf, send_size, recv_buf, recv_size),
      {
        goto cleanup_buf_comm;
        rc = -1;
      },
      "Unable to perform allgather primitive\n");

cleanup_buf_comm:
  free(send_buf);
  free(recv_buf);
  MPI_Comm_free(&new_comm);
  MPI_Finalize();
  return (rc);
}
