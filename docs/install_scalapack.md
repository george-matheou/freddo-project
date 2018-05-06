## Install ScaLAPACK
* Download ScaLAPACK Installer from http://www.netlib.org/scalapack/#_scalapack_installer_for_linux
* Extract the downloaded file
* Install fortran
  * ```sudo apt-get install fort77```
  * ```sudo apt-get install gfortran```
* Install MPI library
* ```mkdir ~/scalapack/```
* Run the python script. For this step we need to specify the output directory and the MPI's bin and include directories
  * e.g., ```python setup.py --prefix=/home/geomat/scalapack/ --mpibindir=/opt/openmpi3/bin/  --mpiincdir=/opt/openmpi3/include/ --downall```
* The output should be like this:
```
Your BLAS library is                     : -L/home/geomat/scalapack/lib -lrefblas
Your LAPACK library is                   : -L/home/geomat/scalapack/lib -ltmg -lreflapack
Your BLACS/ScaLAPACK library is          : -L/home/geomat/scalapack/lib -lscalapack
```
