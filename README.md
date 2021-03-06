# freddo-project
FREDDO is a software runtime, implemented in C++11, that provides data-driven concurrency on single-node and distributed multi-core systems. It is based on Data-Driven Multithreading [1-2], a hybrid control-flow/data-flow model that schedules threads based on data availability on sequential processors.

## Preface
In case you have any publications resulting from FREDDO, please cite the following publications:
  - G. Matheou and P. Evripidou, "FREDDO: an efficient framework for runtime execution of data-driven objects," Proceedings of the International Conference on Parallel and Distributed Processing Techniques and Applications (PDPTA), 2016. ISBN: 1-60132-444-8. [Online]. Available: http://worldcomp-proceedings.com/proc/p2016/PDP3458.pdf

  - G. Matheou and P. Evripidou, "Data-Driven Concurrency for High Performance Computing," ACM Transactions on Architecture and Code Optimization (TACO) 14.4 (2017): 53. DOI: 10.1145/3162014

## Install/uninstall FREDDO

### A) Install Prerequisites
1. GCC compiler with C++11 support
2. MPI library with multi-threading support. FREDDO has been tested with OpenMPI (Versions: 1.8.4, 2.0.1 and 2.1.1). However, we expect that FREDDO will work with any MPI version that supports multithreading.

### B) Install FREDDO
- Go to *freddo* directory
- ```make clean``` *(will clean the freddo project in the current directory)*
- make MPI_BIN_PATH=<Path to the MPI's bin directory>
    - e.g., ```make MPI_BIN_PATH=/opt/openmpi3/bin/```
- ```make install```
  - FREDDO library will be installed in **/usr/local** by default. If you want to change the installation directory you should set the *FREDDO_INSTALL_DIR* variable. For example:
    - ``make install FREDDO_INSTALL_DIR=../install_freddo``

### C) Uninstall FREDDO
- Go to *freddo* directory
- ```make uninstall``` *(will remove the binaries/libraries and header files from the installation directory)*
  - If during installation you set a different installation directory than the default (i.e., **/usr/local**), you should set the *FREDDO_INSTALL_DIR* variable. For example:
    - ``make uninstall FREDDO_INSTALL_DIR=../install_freddo``

## Compile and execute benchmarks

### A) Install prerequisite libraries
Install ScaLAPACK, LAPACK and PLASMA libraries following the instructions found in **docs** directory. The **SCALAPACK_LIB_DIR**, **LAPACK_DIR** and **PLASMA_DIR** should be specified, either by modifying the Makefile files or by using them directly along with the `make` commands, or by exporting them to the shell.  

### B) Compile benchmarks
- Go to *./benchmarks* directory
- ```make clean```
- make MPI_BIN_PATH=<Path to the MPI's bin directory> (all benchmarks will be compiled and installed in bin directory)
  - *e.g.*: ```make MPI_BIN_PATH=/opt/openmpi3/bin/```
- make &lt;X&gt; MPI_BIN_PATH=&lt;MPI bin directory&gt; (will compile and install the X benchmark)
  - *e.g.*: ```make fibonacci MPI_BIN_PATH=/opt/openmpi3/bin/```
- **Notice:** if during FREDDO installation you set a different installation directory than the default (i.e., **/usr/local**), you should set the *FREDDO_INSTALL_DIR* variable, using absolute paths. For example:
    - *e.g.*: `make MPI_BIN_PATH=/opt/openmpi3/bin/ FREDDO_INSTALL_DIR=~/Desktop/Workspace/freddo-project/install_freddo/`
    - *e.g.*: `make fibonacci MPI_BIN_PATH=/opt/openmpi3/bin/ FREDDO_INSTALL_DIR=~/Desktop/Workspace/freddo-project/install_freddo/`

### C) Execute benchmarks
- Execute a benchmark as an MPI application. Here we show how we run Cholesky using OpenMPI (V3).
  - Run Cholesky on the same machine using 2 MPI processes (not recommended, only for testing)
    - ```/opt/openmpi3/bin/mpirun -np 2 --npernode 2 ./bin/cholesky/cholesky_scalapack_with_coll 2 1024 32 1```
  - Run Chokesky on a 4-node cluster (one MPI process per node)
    - ```/opt/openmpi3/bin/mpirun -np 4 --machinefile peers.txt  --npernode 1 ./bin/cholesky/cholesky_scalapack_with_coll 2 1024 32 1```

## References
[1] Kyriacou, Costas, Paraskevas Evripidou, and Pedro Trancoso. "Data-driven multithreading using conventional microprocessors." IEEE Transactions on Parallel and Distributed Systems 17.10 (2006): 1176-1188.

[2] Matheou, George, and Paraskevas Evripidou. "Architectural support for data-driven execution." ACM Transactions on Architecture and Code Optimization (TACO) 11.4 (2015): 52.
