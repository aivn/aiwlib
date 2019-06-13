#    user modules Makefile
# Copyright (C) 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
#-------------------------------------------------------------------------------
define reset
name=    # main target and module name
headers= # user's headers for SWIG (with extentions)
modules= # user's modules for compile and link (with extentions)
objects= # user's object files for linking (need make rules by users self)
sources= # additional user's sources files
aiwmake= # aiwlib modules for make and python import (aiwlib make notation, micro architecture) ???
aiwinst= # aiwlib classes for compile and link (aiwlib make notation, mono architecture)
iheader= # user\'s code to include at start  SWIG file "$(name).i"
ifinish= # user\'s code to include at finish SWIG file "$(name).i"
CXXOPT=  # compile options
LINKOPT= # linker options
SWIGOPT= # SWIG options
debug=   # on or any
cxxmain= # .cpp files with main() functions for make
endef
#-------------------------------------------------------------------------------
aiwlib_include:=$(shell dirname $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
aiwlib:=$(dir $(aiwlib_include))
ifeq ($(wildcard $(aiwlib)libaiw.a),)
libaiw_a:=-laiw
else
libaiw_a:=$(aiwlib)libaiw.a
libaiw_a_file:=$(aiwlib)libaiw.a
endif
#ifeq ($(wildcard $(aiwlib)python/,)
#aiwlib:=
#endif
include $(aiwlib_include)/aiwlib/config.mk
all_sources:=$(sources) $(sort $(filter-out /usr/%,$(MAKEFILE_LIST)) $(foreach m,$(headers) $(modules),$(filter-out $(notdir $(basename $m)).o: /usr/%,$(subst \,,$(shell $(CXX) -I$(aiwlib_include) $(CXXOPT) -M $m)))))			
sources:=$(sort $(filter-out $(aiwlib_include)/%,$(all_sources)))
all_objects:=$(addsuffix .o,$(basename $(modules))) $(objects)
#-------------------------------------------------------------------------------
.PRECIOUS : %.py _%.so %.o %_wrap.cxx %.i
#-------------------------------------------------------------------------------
all: $(name) $(notdir $(basename $(cxxmain)));
$(name): _$(name).so $(name).py
	@echo -ne "\033[7mCHECK IMPORT: python -c 'import $(name)' ... \033[0m"
	@if (python -c 'import $(name)'); then echo -e "\033[7mOK\033[0m"; fi
#	@if [ "$(aiwlibs)" ]; then make-aivlib $(foreach m,$(aivlibs),'$m') ; fi
#-------------------------------------------------------------------------------
aiwmake:=$(sort $(aiwmake))
aiwinst:=$(sort $(aiwinst))
#-------------------------------------------------------------------------------
$(name)-sets :
	@echo headers=\"$(headers)\" --- user\'s headers for SWIG \(with extentions\)
	@echo modules=\"$(modules)\" --- user\'s modules for compile and link \(with extentions\)
	@echo objects=\"$(objects)\" --- user\'s object files for linking \(with extentions, need make rules by users self\)
	@echo sources=\"$(sources)\" --- additional user\'s sources files
	@echo aiwmake=\"$(aiwmake)\" --- aivlib modules for make and python import \(aivlib make notation, micro architecture\)
	@echo aiwinst=\"$(aiwinst)\" --- aivlib modules for compile and link \(aivlib make notation, mono architecture\)
	@echo iheader=\"$(iheader)\" --- user\'s code to include at start SWIG file \"$(name).i\"
	@echo ifinish=\"$(ifinish)\" --- user\'s code to include at finish SWIG file \"$(name).i\"
	@echo CXXOPT=\"$(CXXOPT)\" --- compile options
	@echo LINKOPT=\"$(LINKOPT)\" --- linker options
	@echo SWIGOPT=\"$(SWIGOPT)\" --- SWIG options
	@echo debug=\"$(debug)\" --- debug mode
	@echo cxxmain=\"$(cxxmain)\" --- .cpp files with main() functions for make
#-------------------------------------------------------------------------------
#   start swig
#-------------------------------------------------------------------------------
#$(name).py $(name)_wrap.cxx : $(name).i $(headers) $($(name)_headers)
$(name).py $(name)_wrap.cxx: $(name).i $(headers)
	$(show_target)
	swig $(SWIGOPT) -I$(aiwlib_include) $(name).i
	@mv $(name).py /tmp/$$$$.py; echo 'import sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' > $(name).py; \
	cat /tmp/$$$$.py >> $(name).py; rm /tmp/$$$$.py
	@echo -e "\033[7mFile \"$(name).py\" patched for load shared library with RTLD_GLOBAL=0x00100 flag\033[0m"
#-------------------------------------------------------------------------------
#   link shared library
#-------------------------------------------------------------------------------
_$(name).so: $(name)_wrap.o $(all_objects)
	$(show_target)
	$(CXX) -shared -o $@ $^ $(libaiw_a) $(LINKOPT)
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
ifneq ($(headers),)
$(name)_wrap.o: $(name)_wrap.cxx $(filter-out %:, $(subst \,,$(shell $(CXX) -I$(aiwlib_include) $(CXXOPT) -M $(headers))))
endif
%.o:; $(RUN_CXX) -I$(aiwlib_include) -o $@ -c $<
#-------------------------------------------------------------------------------
#   .i file
#-------------------------------------------------------------------------------
$(name).i: $(MAKEFILE_LIST)
	$(show_target)
	$(imodule)
#	@echo '%include "std_string.i"' >> $@
	@echo $(iheader) >> $@
#	@H=$(aiwmake); $(iinclude)
	@echo "%{ #include <aiwlib/vec> %}" >> $@
	@echo "%include \"aiwlib/vec\"" >> $@
#ifneq ($(filter Mesh%,$(aiwinst)),)
	@echo "%{ #include <aiwlib/mesh> %}" >> $@
	@echo "%include \"aiwlib/mesh\"" >> $@
#endif
#ifneq ($(filter Sphere%,$(aiwinst)),)
	@echo "%{ #include <aiwlib/sphere> %}" >> $@
	@echo "%include \"aiwlib/sphere\"" >> $@
#endif
	@echo "%{" >> $@
	@for i in $(headers); do echo "#include \"$$i\"" >> $@; done
	@echo "%}" >> $@	
	@for i in $(headers); do echo "%include \"$$i\"" >> $@; done
	@$(foreach i,$(filter Mesh%,$(aiwinst)), \
	echo '%template($(word 1,$(subst -, ,$i))) aiw::Mesh<$(word 2,$(subst -, ,$i)),$(word 3,$(subst -, ,$i))>;' >> $@;)
	@$(foreach i,$(filter Sphere%,$(aiwinst)), \
	echo '%template($(word 1,$(subst -, ,$i))) aiw::Sphere<$(word 2,$(subst -, ,$i))>;' >> $@;)
	@$(foreach i,$(aiwinst), echo '%pythoncode %{$(word 1,$(subst -, ,$i)).__setstate__ = _setstate %}' >> $@;)
ifneq ($(wildcard $(aiwlib)python/aiwlib/),)
	@echo "%pythoncode %{import sys; sys.path.insert(1, '$(aiwlib)python/')%}" >> $@
endif
ifneq ($(filter Sphere%,$(aiwinst))$(filter Mesh%,$(aiwinst)),)
	@echo "%pythoncode %{from aiwlib.vec import *%}" >> $@
endif
	@echo $(ifinish) >> $@
#-------------------------------------------------------------------------------
#   other targets
#-------------------------------------------------------------------------------
clean:; rm -f $(name)_wrap.o $(all_objects) _$(name).so $(notdir $(basename $(cxxmain))) $(addsuffix .o,$(basename $(cxxmain))) 
cleanall: clean; rm -f $(name).i $(name)_wrap.cxx $(name).py
#-------------------------------------------------------------------------------
ifeq ($(words $(wildcard $(aiwlib)python/aiwlib/ $(aiwlib)swig/ $(aiwlib)Makefile)),3)
aiwlib_local_check:; 
else 
aiwlib_local_check:; @echo Warning: local aiwlib FAILED! 
endif

ifeq ($(strip $(findstring swig,$(with))),swig)
all_sources:=$(all_sources) $(name).i $(name).py $(name)_wrap.cxx
endif

ifeq ($(strip $(findstring aiwlib,$(with))),aiwlib) 
all_sources:=$(all_sources) $(wildcard $(aiwlib)Makefile $(aiwlib)python/aiwlib/*.py) 
all_sources:=$(all_sources) $(wildcard $(shell echo $(aiwlib)swig/{iostream,swig,mesh,swig}.i))
ifeq ($(strip $(findstring swig,$(with))),swig)
all_sources:=$(sort $(all_sources) $(wildcard $(aiwlib)swig/*.i $(aiwlib)swig/*_wrap.cxx))
endif
endif

sources:; @echo $(sources)
all_sources:; @echo $(all_sources)

$(name).tgz $(name).md5: $(sources) 
	tar -cf $(name).tar $(sources)
	md5sum $(name).tar > $(name).md5
	gzip -c $(name).tar > $(name).tgz
	rm $(name).tar
tar: aiwlib_local_check $(all_sources); tar -zcf $(name)-all.tgz $(all_sources)
#-------------------------------------------------------------------------------
ehost:=$(word 1,$(subst :, ,$(to)))
epath:=$(word 2,$(subst :, ,$(to))) 

export: aiwlib_local_check $(all_sources)
ifeq (,$(strip $(epath)))
	@mkdir -p "$(strip $(to))"
	@tar -cf "$(strip $(to))"/$(name)-$$$$.tar $(all_sources); cd "$(strip $(to))"; tar -xf $(name)-$$$$.tar; rm $(name)-$$$$.tar
else
	@ssh $(ehost) mkdir -p "$(strip $(epath))"
	@tar -zcf /tmp/$(name)-$$$$.tgz $(all_sources); scp /tmp/$(name)-$$$$.tgz $(to); rm /tmp/$(name)-$$$$.tgz; \
	ssh $(ehost) "cd '$(strip $(epath))'; tar -zxf $(name)-$$$$.tgz; rm $(name)-$$$$.tgz;"
endif
	@echo export to \"$(to)\" completed
#-------------------------------------------------------------------------------
#   extras to makefile
#-------------------------------------------------------------------------------
mkextras:=$(firstword $(MAKEFILE_LIST)).extras
$(shell echo '# This file is generated automatically, do not edit it!' > $(mkextras))
$(shell echo '# The file contains additional dependencies and rules for building your project.' >> $(mkextras))
$(shell for i in $(cxxmain); do echo `basename $${i%.*}`:$${i%.*}.o '$(all_objects) $(libaiw_a_file); $$(RUN_CXX) -o $$@ $$^ $(libaiw_a) $(LINKOPT)';done >> $(mkextras))
$(shell for m in $(modules) $(cxxmain); do echo -n `dirname $$m`/; $(CXX) -I$(aiwlib_include) $(CXXOPT) -M $$m; done >> $(mkextras))
include $(mkextras)
#-------------------------------------------------------------------------------
