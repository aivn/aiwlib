# main options for aiwlib make
# Copyright (C) 2017-2018 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
#
# Edit this part of the file manually to configure the make
# Отредактируйте эту часть файла вручную для настройки сборки
#-------------------------------------------------------------------------------
# paths and utlities for install/links-install targets
# пути и утилиты для установки
PYTHONDIR=/usr/lib/python2.7
LIBDIR=/usr/lib
INCLUDEDIR=/usr/include
BINDIR=/usr/bin
BIN_LIST=racs approx isolines gplt uplt splt mplt fplt uplt-remote
#-------------------------------------------------------------------------------
# comment out lines for refusing to use the unwanted modules
# закомментируйте строки для отказа от использования лишних модулей 
zlib=on
swig=on
png=on
pil=on
bin=on
ezz=off
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

PYTHON_H_PATH:=/usr/include/python2.7
override CXXOPT:=$(CXXOPT) -std=c++11 -Wall -fopenmp -fPIC -g -O3
override MPICXXOPT:=$(MPICXXOPT) $(CXXOPT)
#  -I/usr/lib/openmpi/include/
override LINKOPT:=$(LINKOPT) -lgomp  
override SWIGOPT:=$(SWIGOPT) -Wall -python -c++ 
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

ifeq (on,$(zlib)) 
override LINKOPT:=$(LINKOPT) -lz
else
override CXXOPT:=$(CXXOPT) -DAIW_NO_ZLIB
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_ZLIB
endif

ifeq (on,$(swig))
override CXXOPT:=$(CXXOPT) -I$(PYTHON_H_PATH)
override MPICXXOPT:=$(MPICXXOPT) -I$(PYTHON_H_PATH)
ifneq (on,$(pil))
override CXXOPT:=$(CXXOPT) -DAIW_NO_PIL
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_PIL
endif
else
override CXXOPT:=$(CXXOPT) -DAIW_NO_PIL
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_PIL
endif

ifeq (on,$(png))
override LINKOPT:=$(LINKOPT) -lpng
else
override CXXOPT:=$(CXXOPT) -DAIW_NO_PNG
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_PNG
endif

ifeq (on,$(debug)) 
override CXXOPT:=$(CXXOPT) -DEBUG
override MPICXXOPT:=$(MPICXXOPT) -DEBUG
endif
#-------------------------------------------------------------------------------
define show_target
@echo
@echo TARGET: "$@"
@if [ -f "$@" ]; then if [ "$(filter-out /usr/%,$?)" ]; then echo NEW: $(filter-out /usr/%,$?); else echo NEW: $?; fi; fi
@echo ALL: $(filter-out /usr/%,$^)
endef
#-------------------------------------------------------------------------------
ifeq (on,$(mpi))
define RUN_CXX

$(show_target)
$(MPICXX) $(MPICXXOPT)
endef
else
define RUN_CXX

$(show_target)
$(CXX) $(CXXOPT)
endef
endif
#-------------------------------------------------------------------------------
ifeq (off,$(mpi))
override CXXOPT:=$(CXXOPT) -DAIW_NO_MPI
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_MPI
define RUN_MPICXX

$(show_target)
$(CXX) $(CXXOPT)
endef
endif
#-------------------------------------------------------------------------------
ifeq '$(mpi)' ''
ifeq ($(shell if $(MPICXX) -v &> /dev/null; then echo OK; fi),OK)
define RUN_MPICXX

$(show_target)
$(MPICXX) $(MPICXXOPT)
endef
else
override CXXOPT:=$(CXXOPT) -DAIW_NO_MPI
override SWIGOPT:=$(SWIGOPT) -DAIW_NO_MPI
define RUN_MPICXX

$(show_target)
@echo -e "\033[31;1;5mCompiler \"$(MPICXX)\" is not available, MPI DISABLED! Using $(CXX) -DAIW_NO_MPI...\033[0m"
$(CXX) $(CXXOPT)
endef
endif
endif
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
endef
#-------------------------------------------------------------------------------
