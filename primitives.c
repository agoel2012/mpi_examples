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

static inline void print_buf(void *buf, size_t nbytes) {
  size_t i = 0;
  printf("\n============[%zu bytes START]===========\n", nbytes);
  for (i = 0; i < nbytes; i++) {
    if (i % 8 == 0) {
      printf("\n");
    }

    printf("%02x \t", *(uint8_t *)(buf + i));
  }

  printf("\n============[RX %zu bytes END]=============\n", nbytes);
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


static int send_recv(MPI_Comm new_comm, void *send_buf, size_t send_size,
                     void *recv_buf, size_t recv_size) {
  MPI_Request send_req;
  MPI_Request recv_req;
  MPI_Status status;
  int rank = -1;
  int size = 0;

  // TODO: Add error handling support
  MPI_Comm_rank(new_comm, &rank);
  MPI_Comm_size(new_comm, &size);
  printf("Rank [%d], World Size [%d]\n", rank, size);
  fill_buf(&send_buf, send_size, rank + 1);
  randomize_buf(&recv_buf, recv_size);

  // Ring: LEFT => RANK => RIGHT
  // Recv from ((size - rank - 1) % size, (rank - 1) % size)
  // Send to (rank + 1) % size

  int recv_rank =
      ((rank - 1) < 0) ? ((size - rank - 1) % size) : ((rank - 1) % size);
  int send_rank = (rank + 1) % size;

  MPI_Irecv(recv_buf, recv_size, MPI_UINT8_T, recv_rank, 0, new_comm,
            &recv_req);
  MPI_Isend(send_buf, send_size, MPI_UINT8_T, send_rank, 0, new_comm,
            &send_req);
  MPI_Wait(&recv_req, &status);
  MPI_Wait(&send_req, &status);
  print_buf(recv_buf, recv_size);

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
#if 0
  // Primtive-2: rank0 bcast to rank1 & rank2
  API_STATUS(bcast(new_comm, send_buf, send_size, recv_buf, recv_size),
      {
      	goto cleanup_buf_comm;
	rc = -1;
      },
      "Unable to perform bcast primitive\n");

  // Primtive-3: rank0 top half scatter -> rank1; rank0 bottom half scatter -> rank2
  API_STATUS(scatter(new_comm, send_buf, send_size, recv_buf, recv_size),
      {
      	goto cleanup_buf_comm;
	rc = -1;
      },
      "Unable to perform scatter primitive\n");
#endif

cleanup_buf_comm:
  free(send_buf);
  free(recv_buf);
  MPI_Comm_free(&new_comm);
  MPI_Finalize();
  return (rc);
}
