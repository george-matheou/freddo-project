##################################################### The FREDDO's Makefile #####################################################

############ Flags and Variables ############
SHELL 		= /bin/bash
RM			= rm -f --preserve-root
MV 			= mv -f
MAKE		= make
SED 		= sed
CP			= cp
MKDIR		= mkdir -p

### Extra flags for optimizations
#OPT_FLAGS   = -finline-functions -funswitch-loops -fgcse-after-reload -ftree-vectorize -ffast-math
#OPT_FLAGS   := $(OPT_FLAGS) -fthread-jumps -falign-functions  -falign-jumps -falign-loops  -falign-labels -fcaller-saves -fcrossjumping -fcse-follow-jumps  -fcse-skip-blocks -fdelete-null-pointer-checks -fexpensive-optimizations -fgcse  -fgcse-lm -foptimize-sibling-calls -fpeephole2 -fregmove -freorder-blocks  -freorder-functions -frerun-cse-after-loop -fsched-interblock  -fsched-spec  -fschedule-insns2 -fstrict-aliasing -fstrict-overflow -ftree-pre -ftree-vrp
#OPT_FLAGS   := $(OPT_FLAGS) -fcprop-registers -fdefer-pop -fguess-branch-probability -fif-conversion2 -fif-conversion -fipa-pure-const -fipa-reference -fmerge-constants -ftree-ccp -ftree-ch -ftree-copyrename -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-fre -ftree-sra -ftree-ter -funit-at-a-time

# Extra flags for debugging
################# NET_DEBUG 	= Print debug messages about the network operations	#################
################# KERNEL_DEBUG 	= Print debug messages about the Kernels			#################
################# TSU_DEBUG 	= Print debug messages about the TSU				#################
################# -pg			= For the gprof tool								#################
DEB_FLAGS = #-g
#DEB_FLAGS += -DTSU_COLLECT_STATISTICS
#DEB_FLAGS := $(DEB_FLAGS) -DNET_DEBUG
#DEB_FLAGS := $(DEB_FLAGS) -DKERNEL_DEBUG
#DEB_FLAGS := $(DEB_FLAGS) -DTSU_DEBUG
#DEB_FLAGS := $(DEB_FLAGS) -DNETWORK_STATISTICS

# Extra flags to give to the C++ compiler
LIBS_PATHS		=
LIBS			= 
INCLUDES		=
EXTRA_OPTIONS 	= -static
CXXFLAGS 		= -Wall -O3 -std=c++11 -pthread $(EXTRA_OPTIONS) $(DEB_FLAGS) $(INCLUDES) $(LIBS_PATHS) $(LIBS)
# Extra flags to give to the C compiler
CFLAGS     		= 
# The compilers that will be used
CXX_CNI			= g++
CC_CNI			= gcc
CXX_MPI			= $(MPI_BIN_PATH)mpic++
CC_MPI			= $(MPI_BIN_PATH)mpicc

# Flags for creating libraries
AR 				= ar
ARFLAGS			= crs
# The folder that contains the FREDDO libraries
FREDDO_LIB		= ./libfreddo.a

# Collect information from each module in these variables
programs		:=
sources			:=
libraries 		:=
include_dirs 	:=
objects			:= 
dependencies	:= 
target_dirs		:=

# Calculated Variables
#objects			= $(subst .cpp,.o,$(sources))
#dependencies 	= $(subst .cpp,.d,$(sources))

# Include the header files
INCLUDES		+= $(addprefix -I ,$(include_dirs))
vpath %.h $(include_dirs) # Specifies a list of directories that make should search

# Disable Makefile's built-in rules
.SUFFIXES:
.SECONDARY:

# Set the default goal of this makefile
.DEFAULT_GOAL := all

############ Include the Makefiles ############
ifneq ($(MAKECMDGOALS),clean)
	ifndef net
		$(error net is not set. Use the following options: net=cni, net=mpi)
	endif

	ifeq ($(net),cni)
		CXX=$(CXX_CNI)
		CC=$(CC_CNI)
		__net_imple:=custom
		export __net_imple
	else ifeq ($(net),mpi)
		CXX=$(CXX_MPI)
		CC=$(CC_MPI)
		__net_imple:=mpi
		CXXFLAGS+=-DUSE_MPI_FREDDO
		export __net_imple
	else
		$(error net value is wrong. Use the following options: net=cni, net=mpi)
	endif
endif

include ./freddo.mk
include ./Distributed/dist.mk
include ./Timer/timer.mk
include ./TSU/tsu.mk
include ./Collections/collections.mk

############ The Rules ############
.PHONY: dist
dist: all

.PHONY: all
all: libs $(programs)

.PHONY: libs
libs: $(libraries) $(FREDDO_LIB)	
	
############ Create the FREDDO library
$(FREDDO_LIB): $(objects)
	@echo "Creating FREDDO library: " $@
	@$(AR) $(ARFLAGS) $@ $^
	
############ Cleanup the exported files
.PHONY: clean
clean: 
	$(RM) $(objects) $(programs) $(libraries) $(dependencies)
	$(RM) $(FREDDO_LIB)
	@echo -e "FREDDO Project Cleaned"

############ Find the dependencies and Include them 
-include $(dependencies)

# Arg1: Object Directory, Arg2: Source Directory
define compile_rule
$(1)%.o: $(2)%.cpp
	@echo -e "Compililing" $(2)$$*.cpp
	$(CXX) $(CXXFLAGS) -c $(2)$$*.cpp -o $$@
endef	

# Create the dependency files for cpp sources. # Arg1: Depedency Directory, Arg2: Source Directory
define depend_rule
$(1)%.d: $(2)%.cpp
	
	@# The target of the rule
	@#echo -e '@: ' $$@
	@#echo -e 'dir of @: ' $$(dir $$@)
	
	@$(CXX) $(CXXFLAGS) $(TARGET_ARCH) -MM $$< | $(SED) 's,\($$(notdir $$*)\.o\) *:,$$(dir $$@)\1 $$@: ,' > $$@.tmp
	@$(MV) $$@.tmp $$@
endef

# For the Root Directory
$(eval $(call compile_rule,, ../))
$(eval $(call depend_rule,, ../))

# For all the other directories
$(foreach dir, $(target_dirs), $(eval $(call compile_rule, $(dir), ../$(dir))))
$(foreach dir, $(target_dirs), $(eval $(call depend_rule, $(dir), ../$(dir))))
