dir_name		:= Distributed
local_dir 		:= ../$(dir_name)
local_src 		:= $(wildcard $(local_dir)/*.cpp)
local_objs		:= $(subst .cpp,.o,$(local_src))
local_dep 		:= $(subst .cpp,.d,$(local_src))

objects			+= $(subst $(local_dir),$(dir_name),$(local_objs)) 
dependencies	+= $(subst $(local_dir),$(dir_name),$(local_dep))
target_dirs		+= $(dir_name)/

# Include the Makefiles

ifneq ($(MAKECMDGOALS),clean)
	ifeq ($(__net_imple),mpi)
		include_dirs  	+= $(local_dir)/mpi_net
		include ./Distributed/mpi_net/mpi_net.mk
	else
		include_dirs  	+= $(local_dir)/custom_net
		include ./Distributed/custom_net/custom_net.mk 	 
	endif
else
	include_dirs  	+= $(local_dir)/mpi_net
	include ./Distributed/mpi_net/mpi_net.mk
	include_dirs  	+= $(local_dir)/custom_net
	include ./Distributed/custom_net/custom_net.mk 	 
endif