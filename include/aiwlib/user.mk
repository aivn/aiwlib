#   user modules Makefile
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
debug=   # yes or any
endef
#-------------------------------------------------------------------------------
aiwlib_include:=$(shell dirname $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))
aiwlib:=$(dir $(aiwlib_include))
include $(aiwlib_include)/aiwlib/config.mk
all_sources:=$(sources) $(sort $(filter-out /usr/%,$(MAKEFILE_LIST)) $(foreach m,$(headers) $(modules),$(filter-out $(basename $m).o: /usr/%,$(subst \,,$(shell $(GCC) -I$(aiwlib_include) $(CXXOPT) -M $m)))))			
sources:=$(sort $(filter-out $(aiwlib_include)/%,$(all_sources)))
#-------------------------------------------------------------------------------
.PRECIOUS : %.py _%.so %.o %_wrap.cxx %.i
#-------------------------------------------------------------------------------
$(name) : _$(name).so $(name).py; 
#	@if [ "$(aiwlibs)" ]; then make-aivlib $(foreach m,$(aivlibs),'$m') ; fi
#-------------------------------------------------------------------------------
aiwmake:=$(sort $(aiwmake))
aiwinst:=$(sort $(aiwinst))
LINKOPT:=$(LINKOPT) -lpng -lz 
#AIVLIB=aivlib.
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
#-------------------------------------------------------------------------------
#   start swig
#-------------------------------------------------------------------------------
#$(name).py $(name)_wrap.cxx : $(name).i $(headers) $($(name)_headers)
$(name).py $(name)_wrap.cxx : $(name).i $(headers)
	$(show_target)
	swig $(SWIGOPT) -I$(aiwlib_include) $(name).i
	@mv $(name).py /tmp/$$$$.py; echo 'import sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' > $(name).py; \
	cat /tmp/$$$$.py >> $(name).py; rm /tmp/$$$$.py
	@echo ==== $(name).py patched for load shared library with RTLD_GLOBAL=0x00100 flag ====
#-------------------------------------------------------------------------------
#   link shared library
#-------------------------------------------------------------------------------
_$(name).so : $(name)_wrap.o $(addsuffix .o,$(basename $(modules))) $(objects)
	$(show_target)
	$(GCC) -shared -o $@ $^ $(LINKOPT) #-laiw
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
ifndef MODULE
$(name)_wrap.o: $(name)_wrap.cxx
#$(addsuffix .o,$(basename $(modules) $($(name)_modules))) : $(filter-out %:, $(subst \,,$(shell $(GCC) -DPYTHON -I$(PY) $(CXXOPT) -M $(modules) $($(name)_modules))))
%.o:; @$(MAKE) --no-print-directory -f $(word 1, $(MAKEFILE_LIST)) \
	MODULE:=$(strip $(foreach m,$(modules) $(name)_wrap.cxx,$(shell if [ $(basename $m) == $* ]; then echo $m; fi ))) $@
else
#ifeq ($(MODULE),)
#endif
$(strip $(dir $(MODULE))$(subst \,,$(shell $(GCC) $(CXXOPT) -M $(MODULE))))
	$(CXX) -I$(aiwlib_include) -o $(basename $(MODULE)).o -c $(MODULE)
endif
#-------------------------------------------------------------------------------
#   .i file
#-------------------------------------------------------------------------------
$(name).i: $(MAKEFILE_LIST)
	@echo $(MAKEFILE_LIST) 
	$(show_target)
	$(imodule)
#	@echo '%include "std_string.i"' >> $@
	@echo $(iheader) >> $@
#	@H=$(aiwmake); $(iinclude)
	@echo "%{" >> $@
	@for i in $(headers); do echo "#include \"$$i\"" >> $@; done
	@echo "%}" >> $@	
	@for i in $(headers); do echo "%include \"$$i\"" >> $@; done
	@echo $(ifinish) >> $@
#-------------------------------------------------------------------------------
clean:; rm -f $(name)_wrap.o $(addsuffix .o, $(basename $(modules))) _$(name).so 
cleanall: clean; rm -f $(name).i $(name)_wrap.cxx $(name).py
#-------------------------------------------------------------------------------
ifeq ($(strip $(findstring swig,$(with))),swig)
all_sources:=$(all_sources) $(name).i $(name).py $(name)_wrap.cxx
endif

ifeq ($(strip $(findstring aiwlib,$(with))),aiwlib)
all_sources:=$(all_sources) $(aiwlib)Makefile $(wildcard $(aiwlib)python/aiwlib/*.py) 
all_sources:=$(all_sources) $(shell echo $(aiwlib)swig/{iostream,swig,mesh,swig}.i)
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
tar: $(all_sources); tar -zcf $(name)-all.tgz $(all_sources)
#-------------------------------------------------------------------------------
ehost:=$(word 1,$(subst :, ,$(to)))
epath:=$(word 2,$(subst :, ,$(to))) 

export: $(all_sources)
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