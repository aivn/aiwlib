# main options for aiwlib make
# Copyright (C) 2017-2018,2021 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
#
# Edit this part of the file manually to configure the make
# Отредактируйте эту часть файла вручную для настройки сборки
#-------------------------------------------------------------------------------
python:=2
# paths and utlities for install/links-install targets
# пути и утилиты для установки
#ifeq ($(python),2)
#PYTHONDIR=/usr/lib/python2.7
#else
#PYTHONDIR=/usr/lib/python3.6
#endif
PYTHONDIR=$(shell python$(python) -c 'import os; print(os.path.dirname(os.__file__))')

LIBDIR=/usr/lib
INCLUDEDIR=/usr/include
BINDIR=/usr/bin
BIN_LIST=racs approx isolines gplt uplt splt mplt fplt uplt-remote sph2dat arr2seg-Y
#-------------------------------------------------------------------------------
# comment out lines for refusing to use the unwanted modules
# закомментируйте строки для отказа от использования лишних модулей 
zlib=off
swig=on
bin=on
ezz=on
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

#ifeq ($(python),2)
#PYTHON_H_PATH:=/usr/include/python2.7
#else
#PYTHON_H_PATH:=/usr/include/python3.6
#endif
PYTHON_H_PATH:=$(shell python$(python) -c 'import os, sysconfig; print(os.path.dirname(sysconfig.get_config_h_filename()))')
#override CXXOPT:=$(CXXOPT) -std=c++11 -Wall -fopenmp -fPIC -g -O3 -DAIW_TYPEINFO
override CXXOPT:=$(CXXOPT) -std=c++11 -Wall -fopenmp -fPIC -g -O3 
#override CXXOPT:=$(CXXOPT) -std=c++11 -Wall -fopenmp -fPIC -g 
override MPICXXOPT:=$(MPICXXOPT) $(CXXOPT)
#  -I/usr/lib/openmpi/include/
override LINKOPT:=$(LINKOPT) -lgomp  -ldl
override SWIGOPT:=$(SWIGOPT) -Wall -python -c++

# устанавливать 64x битный дистрибутив питона под wine как
# wine64 ~/.wine/drive_c/windows/explorer.exe python-2.7.18.amd64.msi
# запускать вьювер как
# wine ~/.wine/drive_c/Python27/python.exe uplt.py ИМЕНА-ФАЙЛОВ
MINGW:=x86_64-w64-mingw32-g++
#MINGW:=i686-w64-mingw32-g++
MINGW_LINKOPT:=-L ~/.wine/drive_c/Python27/libs/ -lpython27 -lgomp
MINGW_OPT:=-Wall -O3  -std=c++11 -I ~/.wine/drive_c/Python27/include/ -fopenmp -DAIW_WIN32 -DAIW_NO_ZLIB -DM_PI=3.14159265358979323846 -DM_2_PI='(2/M_PI)' -DMS_WIN64
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
