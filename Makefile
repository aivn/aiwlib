ifndef PYTHONDIR
PYTHONDIR=/usr/lib/python2.7
endif

#LIBDIR=/usr/lib
ifndef INCLUDEDIR
INCLUDEDIR=/usr/include
endif

ifndef BINDIR
BINDIR=/usr/bin
endif
BIN_LIST=racs

include include/aiwlib/config.mk

#-------------------------------------------------------------------------------
all: iostream swig $(shell if [ -f TARGETS ]; then cat TARGETS; fi);
iostream swig: %:python/aiwlib/%.py python/aiwlib/_%.so;
#-------------------------------------------------------------------------------
#   run SWIG
#-------------------------------------------------------------------------------
.PRECIOUS : swig/%.py swig/%.o src/%.o
python/aiwlib/%.py: swig/%.py; @cp $< $@
swig/%.py swig/%_wrap.cxx: swig/%.i 
	$(show_target)
	$(SWIG) $(SWIGOPT) $<
python/aiwlib/_%.so: swig/%_wrap.o
	$(show_target)
	$(GCC) -shared -o $@ $^
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
ifndef MODULE
src/%.o: src/%.cpp include/aiwlib/*;   @$(MAKE) --no-print-directory MODULE:=$(basename $@).cpp $@
swig/%.o: swig/%.cxx include/aiwlib/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cxx $@
else
$(strip $(dir $(MODULE))$(subst \,,$(shell $(GCC) $(CXXOPT) -MM $(MODULE))))
	$(CXX) -o $(basename $(MODULE)).o -c $(MODULE)
endif
#-------------------------------------------------------------------------------
#   Mesh
#-------------------------------------------------------------------------------
ifndef MESH_NAME
Mesh%: 
	@$(MAKE) --no-print-directory $(word 1,$(subst -, ,$@)) MESH_NAME:=$(word 1,$(subst -, ,$@)) \
	MESH_TYPE:="$(word 2,$(subst -, ,$@))" MESH_DIM:="$(word 3,$(subst -, ,$@))" 
	@echo $(foreach t,$(sort $(shell cat TARGETS) $@),"$t") > TARGETS
else
$(MESH_NAME): python/aiwlib/$(MESH_NAME).py python/aiwlib/_$(MESH_NAME).so iostream swig; 
swig/$(MESH_NAME).py swig/$(MESH_NAME)_wrap.cxx: include/aiwlib/mesh
swig/$(MESH_NAME).i: #swig/mesh.i
	$(imodule)
	@echo '%{#include "../include/aiwlib/mesh"%}' >> $@
	@echo '%include "../include/aiwlib/mesh"' >> $@
	@echo '%template($(MESH_NAME)) aiw::Mesh<$(MESH_TYPE), $(MESH_DIM)>;' >> $@
	@echo '%pythoncode %{$(MESH_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from vec import *%}' >> $@	
#	python -c "for l in open('swig/mesh.i'): print l[:-1]%{'name':'$(MESH_NAME)', 'type':'$(MESH_TYPE)', 'dim':'$(MESH_DIM)'}" > swig/$(MESH_NAME).i
endif
#-------------------------------------------------------------------------------
#   Sphere
#-------------------------------------------------------------------------------
ifndef SPHERE_NAME
Sphere%: 
	@$(MAKE) --no-print-directory $(word 1,$(subst -, ,$@)) SPHERE_NAME:=$(word 1,$(subst -, ,$@)) \
	SPHERE_TYPE:="$(word 2,$(subst -, ,$@))" 
	@echo $(foreach t,$(sort $(shell cat TARGETS) $@),"$t") > TARGETS
else
$(SPHERE_NAME): python/aiwlib/$(SPHERE_NAME).py python/aiwlib/_$(SPHERE_NAME).so iostream swig; 
python/aiwlib/_$(SPHERE_NAME).so: src/sphere.o
swig/$(SPHERE_NAME).py swig/$(SPHERE_NAME)_wrap.cxx: include/aiwlib/sphere
swig/$(SPHERE_NAME).i:
	$(imodule)
	@echo '%{#include "../include/aiwlib/sphere"%}' >> $@
	@echo '%include "../include/aiwlib/sphere"' >> $@
	@echo '%template($(SPHERE_NAME)) aiw::Sphere<$(SPHERE_TYPE) >;' >> $@
	@echo '%pythoncode %{$(SPHERE_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from vec import *%}' >> $@	
endif
#-------------------------------------------------------------------------------
#   other targets
#-------------------------------------------------------------------------------
clean:; rm -rf swig/*.o src/*.o python/aiwlib/_*.so 
cleanall: clean; rm -rf swig/*.py swig/*_wrap.cxx python/aiwlib/{swig,iostream}.py
#clean-%: ; rm -f $(foreach n, $(filter-out $(word 1,$(subst -, ,$@)),$(subst -, ,$@)), \
#				src/$(n)_wrap.cxx src/$(n)_wrap.o src/_$(n).so src/$(n).py src/$(n).i python/aivlib/$(n).py* python/aivlib/_$(n).so #lib/$(n).o )
#-------------------------------------------------------------------------------
uninstall:; rm -rf `cat install-links`; rm -f install-links
install: all uninstall
	-cp -r include/aiwlib $(INCLUDEDIR)
	-cp -r python/aiwlib  $(PYTHONDIR)
	-for i in $(BIN_LIST); do cp -f bin/$i $(BINDIR); done
links-install install-links: all uninstall
	-ln -s include/aiwlib $(INCLUDEDIR)
	-ln -s python/aiwlib  $(PYTHONDIR)
	-for i in $(BIN_LIST); do ln -s bin/$i $(BINDIR); done
#-------------------------------------------------------------------------------
