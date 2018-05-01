## Install Open MPI
* Download a version from: https://www.open-mpi.org/software/ompi/
  * Note: In this example the Open MPI V3.0.1 will be installed
* Extract the archive file
  * tar -xvf openmpi-3.0.1.tar.gz
* cd openmpi-3.0.1/
* Create a directory where MPI will be installed
  * sudo mkdir /opt/openmpi3
* ./configure  --prefix=/opt/openmpi3 --enable-mpi-thread-multiple
* make
* sudo make install
