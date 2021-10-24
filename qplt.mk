SHELL=/bin/bash

CXX:=g++
SWIG:=swig

PYTHON_H_PATH:=/usr/include/python3.6
PYTHONDIR=/usr/lib/python3/dist-packages/


override CXXOPT:=$(CXXOPT) -std=c++11 -Wall -fopenmp -fPIC -g 
override LINKOPT:=$(LINKOPT) -lgomp  -ldl
override SWIGOPT:=$(SWIGOPT) -Wall -python -c++

# устанавливать 64x битный дистрибутив питона под wine как
# wine64 ~/.wine/drive_c/windows/explorer.exe python-2.7.18.amd64.msi
# запускать вьювер как
# wine ~/.wine/drive_c/Python27/python.exe uplt.py ИМЕНА-ФАЙЛОВ
MINGW:=x86_64-w64-mingw32-g++
MINGW_LINKOPT:=-L ~/.wine/drive_c/Python36/libs/ -lpython36 -lgomp
MINGW_OPT:=-Wall -O3  -std=c++11 -I ~/.wine/drive_c/Python36/include/ -fopenmp -DAIW_WIN32 -DAIW_NO_ZLIB -DM_PI=3.14159265358979323846 -DM_2_PI='(2/M_PI)' -DMS_WIN64
#-------------------------------------------------------------------------------
define show_target
@echo
@echo TARGET: "$@"
@if [ -f "$@" ]; then if [ "$(filter-out /usr/%,$?)" ]; then echo NEW: $(filter-out /usr/%,$?); else echo NEW: $?; fi; fi
@echo ALL: $(filter-out /usr/%,$^)
endef
#-------------------------------------------------------------------------------
define RUN_CXX

$(show_target)
$(CXX) $(CXXOPT)
endef
#-------------------------------------------------------------------------------
.PRECIOUS: swig/qplt/%.py src/qplt/%.o

qplt: python3/aiwlib/qplt/core.py python3/aiwlib/qplt/_core.so

swig/qplt/core.py swig/qplt/core_wrap.cxx: $(shell echo include/aiwlib/qplt/{imaging,accessor,scene,base})

#python3/aiwlib/%.py: swig/%.py
#	@echo 'try: import sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' > $@
#	@echo 'except: pass' >> $@
#	@cat $< >> $@; echo -e "\033[7mFile \"$@\" patched for load shared library with RTLD_GLOBAL=0x00100 flag\033[0m"
python3/aiwlib/qplt/%.py: swig/qplt/%.py
	@echo 'try: import sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' > $@
	@echo 'except: pass' >> $@
	@cat $< >> $@; echo -e "\033[7mFile \"$@\" patched for load shared library with RTLD_GLOBAL=0x00100 flag\033[0m"
swig/qplt/%.py swig/qplt/%_wrap.cxx: swig/qplt/%.i 
	$(show_target)
	$(SWIG) $(SWIGOPT) $<
#-------------------------------------------------------------------------------
#   make shared library
#-------------------------------------------------------------------------------
python3/aiwlib/qplt/_core.so: swig/qplt/core_wrap.o $(shell echo src/qplt/{imaging,accessor,base,mesh}.o) libaiw.a
	$(show_target)
	$(CXX) -shared -o $@ $^ $(LINKOPT)
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
ifndef MODULE
#src/%.o:  src/%.cpp  include/aiwlib/* include/aiwlib/magnets/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cpp $@
#swig/%.o: swig/%.cxx include/aiwlib/* include/aiwlib/magnets/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cxx $@
src/qplt/%.o:  src/qplt/%.cpp  include/aiwlib/* include/aiwlib/qplt/*; @$(MAKE) -f qplt.mk --no-print-directory MODULE:=$(basename $@).cpp $@
swig/%.o: swig/%.cxx include/aiwlib/*; @$(MAKE) -f qplt.mk CXXOPT:='$(CXXOPT) -I$(PYTHON_H_PATH)' --no-print-directory MODULE:=$(basename $@).cxx $@
else
$(strip $(dir $(MODULE))$(subst \,,$(shell $(CXX) $(CXXOPT) -M $(MODULE))))
	$(RUN_CXX) -o $(basename $(MODULE)).o -c $(MODULE)
endif
#-------------------------------------------------------------------------------
clean:; rm -rf swig/qplt/core_wrap.* src/qplt/*.o  python3/aiwlib/qplt/_core.so python3/aiwlib/qplt/core.py
links-install install-links: qplt
	-ln -s "$$(pwd)/python3/aiwlib"  $(PYTHONDIR)
#-------------------------------------------------------------------------------
