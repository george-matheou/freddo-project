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

# Arg 1: The benchmark directory
define build_app
    $(MAKE) -C $1
    
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

bench_dirs=bmmult cholesky lu qr blackscholes fibonacci powerset swaptions

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

# Build the Blackscholes Benchmark
.PHONY: blackscholes
blackscholes:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);			

# Build the Fibonacci Benchmark
.PHONY: fibonacci
fibonacci:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);	

# Build the Recursive Power Set Benchmark
.PHONY: powerset
powerset:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);

# Build the PARSEC's Swaptions Benchmark
.PHONY: swaptions
swaptions:
	@echo -e "\nCreating -> " $@ 
	$(call build_app,$@);

.PHONY: clean
clean:
	$(foreach bench,$(bench_dirs), $(call clean_app,$(bench)); )
	$(RM) $(BENCH_DIR)/results/*.*
	@echo -e "Benchmarks' Executables removed..."
