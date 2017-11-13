include ../Makefile.in

SOURCES=$(wildcard *.cpp)
EXECS=$(SOURCES:.cpp=)
BIN_DIR=./bin
Binaries := $(addprefix $(BIN_DIR)/,$(EXECS))

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

%:%.cpp
	$(CXX) $< $(CXXFLAGS) -o $(BIN_DIR)/$@

clean:
	rm -f $(Binaries)
	