# MPICH Collective/One-Sided `CommExamples`
## Supported primitives
- Rank0->Rank1, Rank1->Rank2, RankN-1->Rank0 `Send/Recv`
- Rank0->Rank1, RankN-1 `Bcast`
- RankN-1->Rank0->Rank1 (send 0th, recvN-1th, ADD old + recv), Rank0->Rank1->Rank2 (send 1st, recv 0th, ADD old + recv) `RingAllReduce(ADD)`
- RankN-1->Rank0->Rank1 (send 0th, recvN-1th), Rank0->Rank1->Rank2 (send 1st, recv 0th) `RingAllgather`
## Tutorial
To install `mpich`
```
apt install mpich
# MPICH header should be found at /usr/include/<platform>/mpich/mpi.h
# Library dependency should be libmpich
```

To compiler examples using `libmpich`,
```
mkdir build && cd build
cmake ..
```

To run examples using `mpirun`,
```
$ mpirun -np 4 ./CommExamples
Rank [0], World Size [4]
Rank [2], World Size [4]
Rank [3], World Size [4]

============[64 bytes RANK 3 START]===========

[Rank(3) 0x0]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x8]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x10]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x18]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x20]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x28]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x30]: 03 	03 	03 	03 	03 	03 	03 	03 	
[Rank(3) 0x38]: 03 	03 	03 	03 	03 	03 	03 	03 	
============[RX 64 bytes RANK 3 END]=============
Rank [1], World Size [4]

============[64 bytes RANK 1 START]===========

[Rank(1) 0x0]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x8]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x10]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x18]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x20]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x28]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x30]: 01 	01 	01 	01 	01 	01 	01 	01 	
[Rank(1) 0x38]: 01 	01 	01 	01 	01 	01 	01 	01 	
============[RX 64 bytes RANK 1 END]=============

============[64 bytes RANK 2 START]===========

[Rank(2) 0x0]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x8]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x10]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x18]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x20]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x28]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x30]: 02 	02 	02 	02 	02 	02 	02 	02 	
[Rank(2) 0x38]: 02 	02 	02 	02 	02 	02 	02 	02 	
============[RX 64 bytes RANK 2 END]=============

============[64 bytes RANK 0 START]===========

[Rank(0) 0x0]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x8]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x10]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x18]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x20]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x28]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x30]: 04 	04 	04 	04 	04 	04 	04 	04 	
[Rank(0) 0x38]: 04 	04 	04 	04 	04 	04 	04 	04 	
============[RX 64 bytes RANK 0 END]=============
```
