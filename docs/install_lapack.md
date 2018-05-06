## Install LAPACK (example with V3.6.1)
* Download lapack-3.6.1 from the official website
* Extract the downloaded file in the directory you want to install lapack-3.6.1
  * e.g, ```tar -xvf lapack-3.6.1.tgz```  
* Install fortran
  * ```sudo apt-get install fort77```
  * ```sudo apt-get install gfortran```
* ```mv make.inc.example make.inc```
* If cmake is not installed: ```sudo apt-get install cmake```
* ```cmake CMakeLists.txt```
* ```make```
* ```sudo make install```
* ```make -C CBLAS/```
* ```make -C LAPACKE/```
* ```sudo make install lapacke```
* ```sudo make install blas```
