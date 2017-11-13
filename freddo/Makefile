############ Flags and Variables ############
SHELL 		= /bin/bash
RM			= rm -f
MV 			= mv -f
MAKE		= make
SED 		= sed
CP			= cp
MKDIR		= mkdir -p

FREDDO_LIB_NAME=libfreddo.a
BUILD_PATH = Build

#FREDDO_INSTALL_DIR=/usr/local
FREDDO_INSTALL_DIR=../install_freddo
FREDDO_LIB_DIR=$(FREDDO_INSTALL_DIR)/lib
FREDDO_INCLUDE_DIR=$(FREDDO_INSTALL_DIR)/include/freddo

include_dirs=Algorithm TSU TSU/SM TSU/SM/google Timer Distributed Distributed/custom_net Distributed/mpi_net Collections Collections/Matrix Collections/TileMatrix Collections/PTileMatrix

.DEFAULT_GOAL :=

# For the Custom network
.PHONY:  cni
cni:
	@echo "Creating the FREDDO framework with custom network"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk net=cni  

# For the network using OpenMPI
.PHONY: openmpi
openmpi:
	@echo "Creating the FREDDO framework with Open MPI"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk net=openmpi 

# For the network using MVAPICH
.PHONY: mvapich
mvapich:
	@echo "Creating the FREDDO framework with MVAPICH"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk net=mvapich 

# For the network using MPICH
.PHONY: mpich
mpich:
	@echo "Creating the FREDDO framework with MPICH"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk net=mpich

# For the network using default mpi
.PHONY: mpidefault
mpidefault:
	@echo "Creating the FREDDO framework with the default MPI (the one that is in the path)"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk net=mpidefault

# For the Custom network
.PHONY: clean
clean:
	@echo "Cleaning the FREDDO framework"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk clean

.PHONY: install
install:
	$(MKDIR) $(FREDDO_LIB_DIR)
	$(CP) $(BUILD_PATH)/$(FREDDO_LIB_NAME) $(FREDDO_LIB_DIR)
	$(MKDIR) $(FREDDO_INCLUDE_DIR)
	$(CP) *.h $(FREDDO_INCLUDE_DIR)
	$(foreach dir,$(include_dirs),$(MKDIR) $(FREDDO_INCLUDE_DIR)/$(dir); $(CP) $(dir)/*.h $(FREDDO_INCLUDE_DIR)/$(dir) 2>/dev/null; )

.PHONY: unistall
unistall:
	$(RM) --preserve-root $(FREDDO_LIB_DIR)/$(FREDDO_LIB_NAME)
	$(RM) -r --preserve-root $(FREDDO_INCLUDE_DIR)
