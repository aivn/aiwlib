PYTHONDIR=/usr/lib/python2.7
#LIBDIR=/usr/lib
INCLUDEDIR=/usr/include
BINDIR=/usr/bin
BIN_LIST=racs

include include/aiwlib/config.mk

#-------------------------------------------------------------------------------
all: iostream swig MeshF1-float-1 MeshF2-float-2 MeshF3-float-3 $(shell if [ -f TARGETS ]; then cat TARGETS; fi);
iostream swig: %:python/aiwlib/%.py python/aiwlib/_%.so;
.PRECIOUS : swig/%.py swig/%.o src/%.o
#-------------------------------------------------------------------------------
#   run SWIG
#-------------------------------------------------------------------------------
swig/swig.py swig/swig_wrap.cxx: include/aiwlib/swig
swig/iostream.py swig/iostream_wrap.cxx: include/aiwlib/iostream

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
include include/aiwlib/mesh.mk
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
include include/aiwlib/sphere.mk
endif
#-------------------------------------------------------------------------------
#   other targets
#-------------------------------------------------------------------------------
clean:; rm -rf swig/*.o src/*.o python/aiwlib/_*.so 
cleanall: clean; rm -rf swig/*.py swig/*_wrap.cxx python/aiwlib/{swig,iostream}.py
#clean-%: ; rm -f $(foreach n, $(filter-out $(word 1,$(subst -, ,$@)),$(subst -, ,$@)), \
#				src/$(n)_wrap.cxx src/$(n)_wrap.o src/_$(n).so src/$(n).py src/$(n).i python/aivlib/$(n).py* python/aivlib/_$(n).so #lib/$(n).o )
#-------------------------------------------------------------------------------
uninstall:; 
	rm -rf $(INCLUDEDIR)/aiwlib $(PYTHONDIR)/aiwlib 
	for i in $(BIN_LIST); do rm -rf $(BINDIR)/$$i; done
install: all uninstall
	-cp -r include/aiwlib $(INCLUDEDIR)
	-cp -r python/aiwlib  $(PYTHONDIR)
	-for i in $(BIN_LIST); do cp -f bin/$$i $(BINDIR); done
links-install install-links: all uninstall
	-ln -s $$(pwd)/include/aiwlib $(INCLUDEDIR)
	-ln -s $$(pwd)/python/aiwlib  $(PYTHONDIR)
	-for i in $(BIN_LIST); do ln -s $$(pwd)/bin/$$i $(BINDIR); done
#-------------------------------------------------------------------------------
