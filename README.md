# freddo-project
FREDDO is a software runtime, implemented in C++11, that provides data-driven concurrency on single-node and distributed multi-core systems. It is based on Data-Driven Multithreading [1-2], a hybrid control-flow/data-flow
model that schedules threads based on data availability on sequential processors.

## Preface
In case you have any publications resulting from FREDDO, please cite the following publications:
  - G. Matheou and P. Evripidou, "FREDDO: an efficient framework for runtime execution of data-driven objects," Proceedings of the International Conference on Parallel and Distributed Processing Techniques and Applications (PDPTA), 2016. ISBN: 1-60132-444-8. [Online]. Available: http://worldcomp-proceedings.com/proc/p2016/PDP3458.pdf

  - G. Matheou and P. Evripidou, "Data-Driven Concurrency for High Performance Computing," ACM Transactions on Architecture and Code Optimization (TACO) 14.4 (2017): 53. DOI: 10.1145/3162014

## Install/uninstall FREDDO

**A) Network Interfaces** <br />
FREDDO utilizes two different network interfaces for inter-node communication: *Custom Network Interface (CNI)* and *MPI*. CNI is an optimized implementation with TCP sockets and implements a fully connected mesh of inter-node connections (i.e., each node maintains a connection to all other nodes). Currently, CNI supports only Ethernet-based interconnects. The major benefits of providing MPI support are portability and flexibility. We have tested FREDDO with OpenMPI (Versions: 1.8.4, 2.0.1 and 2.1.1). However, we expect that FREDDO will work with any MPI version that supports multithreading. FREDDO with CNI support is called *FREDDO+CNI* and FREDDO with MPI support is called *FREDDO+MPI*.

**B) Install Prerequisites**
1. GCC compiler with C++11 support
2. Install an MPI Library with multi-threading support in the case you want to use FREDDO+MPI

**C) Install FREDDO**
- Go to *./freddo* directory
- Install FREDDO+CNI
  - ```make cni```
  - ```make install```
  - FREDDO library will be installed in *../install_freddo*
- Install FREDDO+MPI
  - make mpi MPI_BIN_PATH=<Path to the MPI's bin directory>
    - e.g., ```make mpi MPI_BIN_PATH=/opt/openmpi3/bin/```
  - ```make install```
  - FREDDO library will be installed in *../install_freddo*

**D) Uninstall FREDDO**
- Go to *./freddo* directory
- ```make unistall``` *(will remove the binaries/libraries and header files from ../install_freddo)*
- ```make clean``` *(will clean the freddo project in the current directory)*

## Install and execute benchmarks using FREDDO+CNI
- Go to *./benchmarks/cni* directory
- ```make clean```
- ```make``` *(all benchmarks will be compiled and installed in bin directory)*
- make &lt;X&lt; *(will compile and install the X benchmark, e.g. ```make fibonacci```)*
- Execute a benchmark
  - Example with LU
    - Usage: ./bin/lu/luMain &lt;port&lt; &lt;matrix size&gt; &lt;block size&gt; &lt;run serial&gt; &lt;peer file&gt;
    - *e.g.*: ```./bin/lu/luMain 1234 8192 32 0 peers.txt```
    - For executing the above command in multi-core clusters, the user should use ssh to execute the command on each node/peer. A user can execute benchmarks easier using FREDDO+MPI (see below).

**Notice:** *For the proper installation of the benchmarks, the ScaLAPACK, LAPACK and PLASMA libraries should be installed. Such libraries are needed for the Cholesky and QR benchmarks.*

## Install and execute benchmarks using FREDDO+MPI
- Go to *./benchmarks/mpi* directory
- make clean net=\<X\> where X=(openmpi or mvapich or mpich or mpidefault) <br />
  &nbsp;&nbsp;&nbsp; *e.g.*: make clean net=openmpi
- make net=openmpi OPENMPI_PATH=\<OPENMPI PATH\> (in the case OpenMPI will be used) **or** <br />
  make net=mvapich MVAPICH_PATH=\<MVAPICH PATH\> (in the case MVAPICH will be used) **or**  <br />
  make net=mpich MPICH_PATH=\<MPICH PATH\> (in the case MPICH will be used) **or**  <br />
  make net=mpidefault (in the case the system's default MPI implementation will be used) <br />
  &nbsp;&nbsp;&nbsp; *e.g.*: make net=openmpi OPENMPI_PATH=/opt/openmpi2.1.1
- Execute a benchmark as a regular MPI application. Here we show how we run Cholesky using OpenMPI (V2.1.1)
  - Run Cholesky on the same machine using 2 MPI processes (not recommended, only for testing)
    - /opt/openmpi2.1.1/bin/mpirun -np 2 --npernode 2 ./bin/cholesky/cholesky_scalapack_with_coll 2 1024 32 1
  - Run Chokesky on a 4-node cluster (one MPI process per node)
    - /opt/openmpi2.1.1/bin/mpirun -np 4 --machinefile peers.txt  --npernode 1 ./bin/cholesky/cholesky_scalapack_with_coll 2 1024 32 1

**Notice:** *For the proper installation of the benchmarks, the ScaLAPACK, LAPACK and PLASMA libraries should be installed. Such libraries are needed for the Cholesky and QR benchmarks.*

## References
[1] Kyriacou, Costas, Paraskevas Evripidou, and Pedro Trancoso. "Data-driven multithreading using conventional microprocessors." IEEE Transactions on Parallel and Distributed Systems 17.10 (2006): 1176-1188.

[2] Matheou, George, and Paraskevas Evripidou. "Architectural support for data-driven execution." ACM Transactions on Architecture and Code Optimization (TACO) 11.4 (2015): 52.
