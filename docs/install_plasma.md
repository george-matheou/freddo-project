## Install Plasma
* Download the installer from: http://icl.cs.utk.edu/plasma/software/index.html
* Extract the installer
* ```mkdir ~/plasma```
* ```export lapack_dir=/home/geomat/scalapack/lapack-3.6.1```
* ```./setup.py --prefix=/home/geomat/plasma --blaslib=$lapack_dir/lib/libblas.a --cblaslib=$lapack_dir/libcblas.a --lapacklib=$lapack_dir/lib/liblapack.a  --downlapc --notesting```
