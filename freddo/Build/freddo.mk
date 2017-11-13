local_dir 		:= ..
local_src 		:= $(wildcard $(local_dir)/*.cpp)
local_objs		:= $(subst .cpp,.o,$(local_src))
local_dep 		:= $(subst .cpp,.d,$(local_src))

objects			+= $(subst $(local_dir)/,,$(local_objs)) 
dependencies	+= $(subst $(local_dir)/,,$(local_dep))

include_dirs  	+= $(local_dir)/Algorithm $(local_dir)/Distributed $(local_dir)/Timer $(local_dir)/TSU $(local_dir)/Collections
