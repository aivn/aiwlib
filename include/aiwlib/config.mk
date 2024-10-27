# main options for aiwlib make
# Copyright (C) 2017-2018,2021 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
#
# Edit this part of the file manually to configure the make
# Отредактируйте эту часть файла вручную для настройки сборки
#-------------------------------------------------------------------------------
ifeq ($(python),)
python=2
endif
# paths and utlities for install/links-install targets
# пути и утилиты для установки
PYTHONDIR=$(shell python$(python) -c 'import os; print(os.path.dirname(os.__file__))')
LIBDIR=/usr/lib
INCLUDEDIR=/usr/include
BINDIR=/usr/bin

#BIN_LIST=racs approx isolines gplt uplt splt mplt fplt uplt-remote sph2dat arr2segY segY2arr
BIN_LIST=racs approx isolines gplt2 qplt qplt-remote sph2dat arr2segY segY2arr vtk2msh gplt3
#-------------------------------------------------------------------------------
# comment out lines for refusing to use the unwanted modules
# закомментируйте строки для отказа от использования лишних модулей 
zlib=on
swig=on
bin=on
cuda=off
ezz=off
#view=amr,umesh,zcube

# uncomment one of these lines for PERMANENTLY use (or discarding) MPI
# раскомментируйте одну из этих строк для ПОСТОЯННОГО использования (или отказа от) MPI
# mpi=on
# mpi=off
#-------------------------------------------------------------------------------
# main settings
# основные параметры
CXX:=g++
MPICXX:=mpiCC
SWIG:=swig
NVCC:=nvcc

CU_LD:=g++
ifeq (on,$(cuda))
CU_LD:=$(NVCC)
endif

PYTHON_H_PATH:=$(shell python$(python) -c 'import os, sysconfig; print(os.path.dirname(sysconfig.get_config_h_filename()))')
ifeq (on,$(debug))
#override CXXOPT:=-std=c++11 -g -fopenmp -fPIC -Wall -Wextra -pedantic  -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op  -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2 -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector -DEBUG $(CXXOPT) 
#LINKOPT += -lasan -lubsan
override CXXOPT:=-std=c++11 -g -fopenmp -fPIC -Wall -DEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2 $(CXXOPT) 
#LINKOPT += -lasan -lubsan
else
override CXXOPT:=-std=c++11 -g -fopenmp -fPIC -Wall -O3 $(CXXOPT) 
endif

override MPICXXOPT:=$(MPICXXOPT) $(CXXOPT)
AR:=ar

#  -I/usr/lib/openmpi/include/
ifneq (on,$(mingw))
override LINKOPT:=$(LINKOPT) -lgomp -lz -ldl
endif
override SWIGOPT:=$(SWIGOPT) -Wall -python -c++ -I./
override NVCCOPT:=$(NVCCOPT) --compiler-options -fPIC -O3 -x cu

# устанавливать 64x битный дистрибутив питона под wine как
# wine64 ~/.wine/drive_c/windows/explorer.exe python-2.7.18.amd64.msi
# запускать вьювер как
# wine ~/.wine/drive_c/Python27/python.exe uplt.py ИМЕНА-ФАЙЛОВ

ifeq (on,$(mingw))
CXX:=x86_64-w64-mingw32-g++ -DAIW_WIN32 -DAIW_NO_ZLIB -DM_PI=3.14159265358979323846 -DM_2_PI='(2/M_PI)' -DMS_WIN64
#CXX:=i686-w64-mingw32-g++
LINKOPT:=-L ~/.wine/drive_c/Python38/libs/ -lpython38 -lgomp
#CXXOPT:=-Wall -O3  -std=c++11  -fopenmp 
PYTHON_H_PATH:=~/.wine/drive_c/Python38/include/
AR:=x86_64-w64-mingw32-ar
CU_LD:=$(CXX)
so:=.pyd
endif
#-------------------------------------------------------------------------------
#CXX:=i586-mingw32msvc-g++
#CXXOPT:= -O3 -DMINGW -g 
#SWIGOPT:=-DMINGW
#LINKOPT:=-L ~/.wine/drive_c/Python26/libs/ -lpython26
#PYTHON_H_PATH=~/.wine/drive_c/Python26/include/
#-------------------------------------------------------------------------------
# End of the part for editing, please DO NOT CHANGE the rest of the file!
# Конец части для редактирования, пожалуйста НЕ МЕНЯЙТЕ остальную часть файла!
#-------------------------------------------------------------------------------
SHELL=/bin/bash

ifeq (on,$(debug))
_dbg=-dbg
dbg_=dbg_
endif

ifeq (on,$(zlib)) 
override LINKOPT:=$(LINKOPT) -lz
else
override CXXOPT:=$(CXXOPT) -DAIW_NO_ZLIB
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_ZLIB
endif

#ifeq (on,$(swig))
#override CXXOPT:=$(CXXOPT) -I$(PYTHON_H_PATH)
#override MPICXXOPT:=$(MPICXXOPT) -I$(PYTHON_H_PATH)
#endif
#-------------------------------------------------------------------------------
#   macros
#-------------------------------------------------------------------------------
define show_target
@echo
@echo -e '\033[44mTARGET: "$@"\033[0m'
@if [ -f "$@" ]; then if [ "$(filter-out /usr/%,$?)" ]; then echo NEW: $(filter-out /usr/%,$?); else echo NEW: $?; fi; fi
@echo ALL: $(filter-out /usr/%,$^)
@echo
endef
#-------------------------------------------------------------------------------
define run_swig
@mkdir -p $$(dirname $@)
$(show_target)
$(SWIG) $(SWIGOPT) -I$(PYTHON_H_PATH) -outdir $$(dirname $@) -o "$@" $<
./patch_swig.py $@
endef
#-------------------------------------------------------------------------------
define patch_py
@mkdir -p $$(dirname $@)
@echo '#--- begin of aiwlib patch ---' > $@ 
@echo 'try: import os, sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' >> $@
@echo 'except: pass' >> $@
@echo "if os.environ.get('debug')=='on':" >> $@
@p=$$(basename $@); echo "  if __package__ or '.' in __name__: from . import _dbg_$${p%.*}" >> $@
@p=$$(basename $@); echo "  else: import _dbg_$${p%.*}" >> $@
@p=$$(basename $@); echo "  sys.modules['_$${p%.*}'] = _dbg_$${p%.*}" >> $@
@echo '#--- end of aiwlib patch ---' >> $@ 
@echo >> $@ 
@cat $< >> $@; echo -e "\033[7m  File \"$@\" patched for load shared library with RTLD_GLOBAL=0x00100 flag and support debug mode  \033[0m"
endef
#-------------------------------------------------------------------------------
define run_cxx
$(show_target)
$(CXX) $(CXXOPT) -I $(PYTHON_H_PATH) -I./  -o $@ -c $<
endef
#-------------------------------------------------------------------------------
.SUFFIXES :
#-------------------------------------------------------------------------------
define imodule
@echo %module $(notdir $(basename $@)) > $@
@echo "%exception {" >> $@
@echo "    try{ \$$action }" >> $@
@echo "    catch(const char *e){ PyErr_SetString(PyExc_RuntimeError, e); return NULL; }" >> $@
@echo "    catch(...){ return NULL; }" >> $@
@echo "}" >> $@
@echo "%pythoncode %{def _setstate(self, state):" >> $@
@echo "    if not hasattr(self, 'this'): self.__init__()" >> $@
@echo "    self.__C_setstate__(state)" >> $@
@echo "def _swig_setattr(self, class_type, name, value):" >> $@
@echo "    if name in class_type.__swig_setmethods__:" >> $@
@echo "        try: value = getattr(self, name).__class__(value)" >> $@
@echo "        except: pass" >> $@
@echo "    return _swig_setattr_nondynamic(self, class_type, name, value, 0)" >> $@
@echo "__makefile__ = '$(word 1, $(MAKEFILE_LIST))'" >> $@
@echo "_$(notdir $(basename $@)).__makefile__ = '$(word 1, $(MAKEFILE_LIST))'" >> $@
@echo "%}" >> $@
@echo "%typemap(out) bool&   %{ \$$result = PyBool_FromLong    ( *\$$1 ); %}" >> $@
@echo "%typemap(out) char&   %{ \$$result = PyInt_FromLong     ( *\$$1 ); %}" >> $@
@echo "%typemap(out) short&  %{ \$$result = PyInt_FromLong     ( *\$$1 ); %}" >> $@
@echo "%typemap(out) int&    %{ \$$result = PyInt_FromLong     ( *\$$1 ); %}" >> $@
@echo "%typemap(out) long&   %{ \$$result = PyInt_FromLong     ( *\$$1 ); %}" >> $@
@echo "%typemap(out) float&  %{ \$$result = PyFloat_FromDouble ( *\$$1 ); %}" >> $@
@echo "%typemap(out) double& %{ \$$result = PyFloat_FromDouble ( *\$$1 ); %}" >> $@
@echo '%feature("autodoc","1");' >> $@
@echo '%include "std_string.i"' >> $@
@echo "%inline %{ namespace aiw{}; %}" >> $@
@echo "%define CONFIGURATE(ARGS...) %enddef" >> $@
@echo "%define CU_HOST %enddef" >> $@
@echo "%define CU_DEVICE %enddef" >> $@
@echo "%define CU_GLOBAL %enddef" >> $@
@echo "%define CU_HD %enddef" >> $@
endef
#-------------------------------------------------------------------------------
