include ../Makefile.in

EXECS=bin/cholesky_scalapack_with_coll bin/cholesky_scalapack
LIB_DIR_LAPACK= -L /home/geomat/scalapack/lib/ 
LIBS_LAPACK=  -lscalapack -lreflapack -lrefblas -lgfortran

all: $(EXECS)

bin/cholesky_scalapack: cholesky_scalapack.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack $(LIB_DIR_LAPACK) $(LIBS_LAPACK)

bin/cholesky_scalapack_with_coll: cholesky_scalapack_with_coll.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack_with_coll $(LIB_DIR_LAPACK) $(LIBS_LAPACK)

clean:
	rm -f $(EXECS) 
	