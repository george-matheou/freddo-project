##################################################### The FREDDO's Makefile #####################################################

############ Flags and Variables ############
SHELL 		= /bin/bash
RM			= rm -f --preserve-root
MV 			= mv -f
MAKE		= make
SED 		= sed
CP			= cp
MKDIR		= mkdir -p

############ Installation paths of freddo
FREDDO_LIB_NAME=libfreddo.a
BUILD_PATH=Build

FREDDO_INSTALL_DIR=/usr/local
#FREDDO_INSTALL_DIR=../install_freddo
FREDDO_LIB_DIR=$(FREDDO_INSTALL_DIR)/lib
FREDDO_INCLUDE_DIR=$(FREDDO_INSTALL_DIR)/include/freddo

############ Extra flags for debugging
## NET_DEBUG 	= Print debug messages about the network operations
## KERNEL_DEBUG = Print debug messages about the Kernels
## TSU_DEBUG 	= Print debug messages about the TSU
## -pg			= For the gprof tool
DEB_FLAGS = #-g
#DEB_FLAGS += -DTSU_COLLECT_STATISTICS
#DEB_FLAGS := $(DEB_FLAGS) -DNET_DEBUG
#DEB_FLAGS := $(DEB_FLAGS) -DKERNEL_DEBUG
#DEB_FLAGS := $(DEB_FLAGS) -DTSU_DEBUG
#DEB_FLAGS := $(DEB_FLAGS) -DNETWORK_STATISTICS

############ Extra flags to give to the C++ compiler
LIBS_PATHS		=
LIBS			=
INCLUDES		=
EXTRA_OPTIONS 	= #-static
CXXFLAGS 		= -Wall -O3 -std=c++11 -pthread $(EXTRA_OPTIONS) $(DEB_FLAGS) $(INCLUDES) $(LIBS_PATHS) $(LIBS)
############ Extra flags to give to the C compiler
CFLAGS     		=
############ The compilers that will be used
CXX				= $(MPI_BIN_PATH)mpic++
CC				= $(MPI_BIN_PATH)mpicc
############ Flags for creating libraries
AR 				= ar
ARFLAGS			= crs
############ The folder that contains the FREDDO libraries
FREDDO_LIB		= ./$(FREDDO_LIB_NAME)

include_dirs=Algorithm TSU TSU/SM TSU/SM/google Distributed Collections Collections/Matrix Collections/TileMatrix Collections/PTileMatrix

# Export variables to other makefiles
export

.DEFAULT_GOAL := freddo

.PHONY: freddo
freddo:
	@echo "Creating FREDDO"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk

.PHONY: clean
clean:
	@echo "Cleaning FREDDO"
	$(MAKE) -C $(BUILD_PATH) -f Makefile.mk clean

.PHONY: install
install:
	$(MKDIR) $(FREDDO_LIB_DIR)
	$(CP) $(BUILD_PATH)/$(FREDDO_LIB_NAME) $(FREDDO_LIB_DIR)
	$(MKDIR) $(FREDDO_INCLUDE_DIR)
	$(CP) *.h $(FREDDO_INCLUDE_DIR)
	$(foreach dir,$(include_dirs),$(MKDIR) $(FREDDO_INCLUDE_DIR)/$(dir); $(CP) $(dir)/*.h $(FREDDO_INCLUDE_DIR)/$(dir) 2>/dev/null; )

.PHONY: uninstall
uninstall:
	$(RM) $(FREDDO_LIB_DIR)/$(FREDDO_LIB_NAME)
	$(RM) -r $(FREDDO_INCLUDE_DIR)
