include ../Makefile.in

all: bin/powerset

ifeq (openmpi,$(MAKECMDGOALS))
      CXX=$(CXX_openmpi)
else ifeq (mvapich,$(MAKECMDGOALS))
	  CXX=$(CXX_mvapich)   
else ifeq (mpich,$(MAKECMDGOALS))
	  CXX=$(CXX_mpich)     
else 
      CXX=$(CXX_mpidefault)
endif

EXECS=bin/powerset

.PHONY: openmpi
openmpi:$(EXECS)

.PHONY: mvapich	
mvapich:$(EXECS)

.PHONY: mpich	
mpich:$(EXECS)

.PHONY: mpidefault	
mpidefault:$(EXECS)

bin/powerset: powerset.cpp
	$(CXX) $< $(CXXFLAGS) -o bin/powerset
	
clean:
	rm -f bin/powerset
	
	