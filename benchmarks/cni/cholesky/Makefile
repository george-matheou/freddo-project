include ../Makefile.in

EXECS=bin/cholesky_scalapack_with_coll bin/cholesky_scalapack
LIBS_LAPACK=-lscalapack -lreflapack -lrefblas -lgfortran

all: $(EXECS)

bin/cholesky_scalapack: cholesky_scalapack.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack -L $(LIB_DIR_SCALAPACK) $(LIBS_LAPACK)

bin/cholesky_scalapack_with_coll: cholesky_scalapack_with_coll.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack_with_coll -L $(LIB_DIR_SCALAPACK) $(LIBS_LAPACK)

clean:
	rm -f $(EXECS) 
	