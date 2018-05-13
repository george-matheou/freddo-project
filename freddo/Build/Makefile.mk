# Collect information from each module in these variables
programs		:=
sources			:=
libraries 		:=
include_dirs 	:=
objects			:= 
dependencies	:= 
target_dirs		:=

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
	@echo "FREDDO Project Cleaned"

############ Find the dependencies and Include them 
-include $(dependencies)

# Arg1: Object Directory, Arg2: Source Directory
define compile_rule
$(1)%.o: $(2)%.cpp
	@echo "Compililing" $(2)$$*.cpp
	$(CXX) $(CXXFLAGS) -c $(2)$$*.cpp -o $$@
endef	

# Create the dependency files for cpp sources. # Arg1: Depedency Directory, Arg2: Source Directory
define depend_rule
$(1)%.d: $(2)%.cpp
	
	@# The target of the rule
	@#echo '@: ' $$@
	@#echo 'dir of @: ' $$(dir $$@)
	
	@$(CXX) $(CXXFLAGS) $(TARGET_ARCH) -MM $$< | $(SED) 's,\($$(notdir $$*)\.o\) *:,$$(dir $$@)\1 $$@: ,' > $$@.tmp
	@$(MV) $$@.tmp $$@
endef

# For the Root Directory
$(eval $(call compile_rule,, ../))
$(eval $(call depend_rule,, ../))

# For all the other directories
$(foreach dir, $(target_dirs), $(eval $(call compile_rule, $(dir), ../$(dir))))
$(foreach dir, $(target_dirs), $(eval $(call depend_rule, $(dir), ../$(dir))))
