dir_name		:= TSU
local_dir 		:= ../$(dir_name)
local_src 		:= $(wildcard $(local_dir)/*.cpp)
local_objs		:= $(subst .cpp,.o,$(local_src))
local_dep 		:= $(subst .cpp,.d,$(local_src))

objects			+= $(subst $(local_dir),$(dir_name),$(local_objs)) 
dependencies	+= $(subst $(local_dir),$(dir_name),$(local_dep))
target_dirs		+= $(dir_name)/

include_dirs  	+= $(local_dir)/SM

# Include the Makefiles
include ./TSU/SM/sm.mk 	