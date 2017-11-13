include ../Makefile.in

EXECS=bin/cholesky_scalapack bin/cholesky_scalapack_with_coll
LIB_DIR_LAPACK= -L /home/geomat/scalapack/lib/ 
LIBS_LAPACK=  -lscalapack -lreflapack -lrefblas -lgfortran -lm

ifeq (openmpi,$(MAKECMDGOALS))
      CXX=$(CXX_openmpi)
else ifeq (mvapich,$(MAKECMDGOALS))
	  CXX=$(CXX_mvapich)   
else ifeq (mpich,$(MAKECMDGOALS))
	  CXX=$(CXX_mpich)     
else 
      CXX=$(CXX_mpidefault)
endif

.PHONY: openmpi
openmpi:$(EXECS)

.PHONY: mvapich	
mvapich:$(EXECS)

.PHONY: mpich	
mpich:$(EXECS)

.PHONY: mpidefault	
mpidefault:$(EXECS)

bin/cholesky_scalapack: cholesky_scalapack.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack $(LIB_DIR_LAPACK) $(LIBS_LAPACK)

bin/cholesky_scalapack_with_coll: cholesky_scalapack_with_coll.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/cholesky_scalapack_with_coll $(LIB_DIR_LAPACK) $(LIBS_LAPACK)

clean:
	rm -f $(EXECS)
	
