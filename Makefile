PYTHONDIR=/usr/lib/python2.7
LIBDIR=/usr/lib
INCLUDEDIR=/usr/include
BINDIR=/usr/bin
BIN_LIST=racs approx isolines

include include/aiwlib/config.mk

#-------------------------------------------------------------------------------
all: iostream swig MeshF1-float-1 MeshF2-float-2 MeshF3-float-3 $(shell echo bin/{arr2seg-Y,arrconv,isolines,dat2mesh}) $(shell if [ -f TARGETS ]; then cat TARGETS; fi) libaiw.a;
iostream swig mpi4py: %: python/aiwlib/%.py python/aiwlib/_%.so;
.PRECIOUS: swig/%.py swig/%.o src/%.o
#-------------------------------------------------------------------------------
#libaiw.a: $(shell echo src/{sphere,configfile,segy,isolines,checkpoint,geometry,mixt,magnets/{data,lattice}}.o); rm -f libaiw.a; ar -csr libaiw.a   $^
libaiw.a: $(shell echo src/{debug,sphere,configfile,segy,isolines,checkpoint,mixt}.o); rm -f libaiw.a; ar -csr libaiw.a   $^
#-------------------------------------------------------------------------------
#   run SWIG
#-------------------------------------------------------------------------------
swig/swig.py swig/swig_wrap.cxx: include/aiwlib/swig
swig/iostream.py swig/iostream_wrap.cxx: include/aiwlib/iostream
swig/mpi4py.py swig/mpi4py_wrap.cxx: include/aiwlib/mpi4py

python/aiwlib/%.py: swig/%.py
	@echo 'import sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' > $@
	@cat $< >> $@; echo -e "\033[7mFile \"$@\" patched for load shared library with RTLD_GLOBAL=0x00100 flag\033[0m"
swig/%.py swig/%_wrap.cxx: swig/%.i 
	$(show_target)
	$(SWIG) $(SWIGOPT) $<
#-------------------------------------------------------------------------------
#   make shared library
#-------------------------------------------------------------------------------
python/aiwlib/_%.so: swig/%_wrap.o libaiw.a
	$(show_target)
	$(GCC) -shared -o $@ $^ $(LINKOPT)
#-------------------------------------------------------------------------------
#   mpiCC
#-------------------------------------------------------------------------------
python/aiwlib/_mpi4py.so: swig/mpi4py_wrap.cxx include/aiwlib/mpi4py
	$(show_target)
	$(MPICC) $(MPIOPT) -shared -o $@ $< $(LINKOPT)
src/$(subst \,,$(shell $(MPICC) $(MPIOPT) -M src/racs.cpp))  
	$(show_target)
	$(MPICC) $(MPIOPT) -o $@ -c $< 
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
ifndef MODULE
src/%.o:  src/%.cpp  include/aiwlib/* include/aiwlib/magnets/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cpp $@
swig/%.o: swig/%.cxx include/aiwlib/* include/aiwlib/magnets/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cxx $@
else
$(strip $(dir $(MODULE))$(subst \,,$(shell $(GCC) $(CXXOPT) -M $(MODULE))))
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
#   utils
#-------------------------------------------------------------------------------
#bin/arr2seg-Y: src/bin/arr2seg-Y.o src/segy.o; $(CXX) -DEBUG -o bin/arr2seg-Y src/bin/arr2seg-Y.o src/segy.o -lz
#bin/arr2seg-Y: src/segy.o
#bin/isolines: src/isolines.o
bin/arr2seg-Y bin/arrconv bin/isolines bin/dat2mesh: bin/%: src/bin/%.o libaiw.a; $(CXX) -o $@ $^ -lz
#-------------------------------------------------------------------------------
#   other targets
#-------------------------------------------------------------------------------
clean:; rm -rf swig/*.o src/*.o src/bin/*.o python/aiwlib/_*.so 
cleanall: clean 
	for i in swig/*.py; do rm -f $$i python/aiwlib/$$(basename $$i){,c}; done 
	rm -f swig/*_wrap.cxx 
	-for i in $$(cat TARGETS); do rm -f swig/$${i%%-*}.i; done
clean-%:; -n=$@; rm swig/$${n:6}_wrap.o python/aiwlib/_$${n:6}.so
cleanall-%: clean-%; -n=$@; rm swig/$${n:9}.py swig/$${n:9}_wrap.cxx swig/$${n:9}.i python/aiwlib/$${n:9}.py{,c}
#-------------------------------------------------------------------------------
uninstall:; 
	rm -rf $(INCLUDEDIR)/aiwlib $(PYTHONDIR)/aiwlib $(LIBDIR)/libaiw.a 
	for i in $(BIN_LIST); do rm -rf $(BINDIR)/$$i; done
install: all uninstall
	-cp -r include/aiwlib $(INCLUDEDIR)
	-cp -r python/aiwlib  $(PYTHONDIR)
	-cp libaiw.a $(LIBDIR)/
	-for i in $(BIN_LIST); do cp -f bin/$$i $(BINDIR); done
links-install install-links: all uninstall
	-ln -s $$(pwd)/include/aiwlib $(INCLUDEDIR)
	-ln -s $$(pwd)/python/aiwlib  $(PYTHONDIR)
	-ln -s $$(pwd)/libaiw.a  $(LIBDIR)
	-for i in $(BIN_LIST); do ln -s $$(pwd)/bin/$$i $(BINDIR); done
#-------------------------------------------------------------------------------
