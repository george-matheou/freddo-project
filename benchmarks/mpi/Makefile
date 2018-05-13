##################################################### Produce the Benchmarks #####################################################

BENCH_DIR	 		= .
BENCH_BIN			= $(BENCH_DIR)/bin

############ Flags and Variables
SHELL 		= /bin/bash
RM			= rm -f --preserve-root
MV 			= mv -f
CP			= cp
MAKE		= make
SED 		= sed

FREDDO_INSTALL_DIR=/usr/local
FREDDO_LIB_DIR=$(FREDDO_INSTALL_DIR)/lib
FREDDO_INC_DIR=$(FREDDO_INSTALL_DIR)/include

## SCALAPACK paths and libraries
SCALAPACK_LIB_DIR	=/home/geomat/scalapack/lib

## LAPACK paths and libraries
LAPACK_DIR			=/home/geomat/scalapack/lapack-3.6.1
LAPACK_INCLUDES		=-I$(LAPACK_DIR)/CBLAS/include -I$(LAPACK_DIR)/LAPACKE/include -I$(FREDDO_INC_DIR)
LAPACK_LIBS_PATHS	=-L$(LAPACK_DIR) -L$(LAPACK_DIR)/lib -L$(FREDDO_LIB_DIR)
LAPACK_LIBS			=-llapacke  -llapack -lcblas -lblas -lgfortran -lm

## Plasma paths and libraries
PLASMA_DIR 			=/home/geomat/plasma
PLASMA_INC_DIR 		=-I$(PLASMA_DIR)/include
PLASMA_LIB_DIR 		=-L$(PLASMA_DIR)/lib
PLASMA_LIBS 		=-lplasma -lcoreblas -lquark

# Extra flags to give to the C++ compiler
FREDDO_LIBS			=-lfreddo -pthread

# The compiler that will be used
CXX_MPI			= $(MPI_BIN_PATH)mpic++
CC_MPI			= $(MPI_BIN_PATH)mpicc

# Debug Flags
DEB_FLAGS	 	= #-g
#DEB_FLAGS 		+= -DTSU_COLLECT_STATISTICS

# Test Flags
TEST_FLAGS 		=  #-DTEST_DYNAMIC_SM

CXXFLAGS 		= -Wall -DUSE_MPI_FREDDO -O3 -std=c++11 $(TEST_FLAGS) $(DEB_FLAGS) -I$(FREDDO_INC_DIR) -L$(FREDDO_LIB_DIR) $(FREDDO_LIBS)

# Export all variables
export

# Arg 1: The benchmark directory
define build_app
    $(MAKE) -C $1 $(net)
    
	@mkdir -p $(BENCH_BIN)/$1
	
	@$(CP) $1/bin/*  $(BENCH_BIN)/$1
	@echo -e "==================="
endef

# Arg 1: The benchmark directory
define clean_app
	$(MAKE) -C $1 clean
	$(RM) -r $(BENCH_BIN)/$1
endef

# Set the default goal of this makefile
.DEFAULT_GOAL := all

bench_dirs= lu cholesky qr bmmult powerset fibonacci swaptions blackscholes

.PHONY: all
all: $(bench_dirs)

# Build the Blocked Matrix Multiplication Benchmark
.PHONY: bmmult
bmmult:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);

# Build the Cholesky Benchmark
.PHONY: cholesky
cholesky:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);

# Build the LU Benchmark
.PHONY: lu
lu:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);

# Build the QR Benchmark
.PHONY: qr
qr:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);	
	
# Build the Powerset Benchmark
.PHONY: powerset
powerset:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);	

# Build the Fibonacci Benchmark
.PHONY: fibonacci
fibonacci:	
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);		

# Build the Swaptions Benchmark
.PHONY: swaptions
swaptions: 	
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);	

# Build the Blackscholes Benchmark
.PHONY: blackscholes
blackscholes: 	
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);	

.PHONY: clean
clean:
	$(foreach bench,$(bench_dirs), $(call clean_app,$(bench)); )	
	
	$(RM) $(BENCH_DIR)/results/*.*
	@echo -e "Benchmarks' Executables removed..."
