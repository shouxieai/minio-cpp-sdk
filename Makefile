
cpp_srcs := $(shell find src -name "*.cpp")
cpp_objs := $(patsubst %.cpp,%.o,$(cpp_srcs))
cpp_objs := $(subst src/,objs/,$(cpp_objs))

openssl_path := /data/datav/wuhan/lean/openssl1.1.1j
curl_path    := /data/datav/wuhan/lean/curl7.77.0-DEV

include_paths := src \
			$(openssl_path)/include \
			$(curl_path)/include

library_paths := \
			$(openssl_path)/lib \
			$(curl_path)/lib

link_librarys := ssl crypto curl stdc++ dl

run_paths     := $(foreach item,$(library_paths),-Wl,-rpath=$(item))
include_paths := $(foreach item,$(include_paths),-I$(item))
library_paths := $(foreach item,$(library_paths),-L$(item))
link_librarys := $(foreach item,$(link_librarys),-l$(item))

cpp_compile_flags := -std=c++11 -fPIC -m64 -g -fopenmp -w -O0
link_flags        := -pthread -fopenmp

cpp_compile_flags += $(include_paths)
link_flags 		  += $(library_paths) $(link_librarys) $(run_paths)

pro : workspace/pro

workspace/pro : $(cpp_objs)
	@echo Link $@
	@mkdir -p $(dir $@)
	@g++ $^ -o $@ $(link_flags)

objs/%.o : src/%.cpp
	@echo Compile CXX $<
	@mkdir -p $(dir $@)
	@g++ -c $< -o $@ $(cpp_compile_flags)

run : workspace/pro
	@cd workspace && ./pro

clean :
	@rm -rf objs workspace/pro

.PHONY : clean pro run