EXECS=bin/cholesky_scalapack_with_coll bin/cholesky_scalapack
LIBS_LAPACK=-lscalapack -lreflapack -lrefblas -lgfortran

all: $(EXECS)

bin/cholesky_scalapack: cholesky_scalapack.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack -L $(SCALAPACK_LIB_DIR) $(LIBS_LAPACK)

bin/cholesky_scalapack_with_coll: cholesky_scalapack_with_coll.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack_with_coll -L $(SCALAPACK_LIB_DIR) $(LIBS_LAPACK)

clean:
	rm -f $(EXECS) 
	