# Copyright (C) 2016-2018 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
#
# Usage:
#    make [CXX=<custom compiler>] [MPICXX=<custom MPI compiler>] 
# or edit file 'include/aiwlib/config.mk' for select custom compilers permanently;
#
#    make [zlib=off] [swig=off] [png=off] [pil=off] [bin=off] 
# or edit file 'include/aiwlib/config.mk' for change main target 'all' and list of aiwlib C++-core modules.  
#
# See include/aiwlib/config.mk and doc/aiwlib.pdf for details.

include include/aiwlib/config.mk
#-------------------------------------------------------------------------------
ifeq (on,$(swig)) 
all: iostream swig plot2D MeshF1-float-1 MeshF2-float-2 MeshF3-float-3 MeshUS2-uint16_t-2 MeshUS3-uint16_t-3 SphereD-double SphereF-float SphereUS-uint16_t $(shell if [ -f TARGETS ]; then cat TARGETS; fi)
endif

ifneq (off,$(mpi)) 
ifeq ($(shell if $(MPICXX) -v &> /dev/null; then echo OK; fi),OK)
all: mpi4py
endif
endif

ifeq (on,$(bin)) 
all: $(shell echo bin/{arr2seg-Y,isolines,dat2mesh,fv-slice})
endif

ifeq (on,$(ezz))
all: mplt fplt splt
endif

all: libaiw.a;

iostream swig mpi4py plot2D: %: python/aiwlib/%.py python/aiwlib/_%.so;
.PRECIOUS: swig/%.py swig/%.o src/%.o
#-------------------------------------------------------------------------------
libaiw.a: $(shell echo src/{debug,sphere,configfile,segy,isolines,checkpoint,mixt,racs,plot2D,farfield}.o); rm -f libaiw.a; ar -csr libaiw.a $^
#-------------------------------------------------------------------------------
#   run SWIG
#-------------------------------------------------------------------------------
swig/swig.py swig/swig_wrap.cxx: include/aiwlib/swig
swig/iostream.py swig/iostream_wrap.cxx: include/aiwlib/iostream include/aiwlib/gzstream 
swig/mpi4py.py swig/mpi4py_wrap.cxx: include/aiwlib/mpi4py
swig/plot2D.py swig/plot2D_wrap.cxx: include/aiwlib/plot2D

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
	$(CXX) -shared -o $@ $^ $(LINKOPT)
#-------------------------------------------------------------------------------
#   mpiCC
#-------------------------------------------------------------------------------
python/aiwlib/_mpi4py.so: swig/mpi4py_wrap.cxx include/aiwlib/mpi4py
	$(show_target)
	$(MPICXX) $(MPICXXOPT) -shared -o $@ $< $(LINKOPT)

src/racs.cpp: include/aiwlib/mpi4py
src/$(subst \,,$(shell $(CXX) $(CXXOPT) -M -DAIW_NO_MPI src/racs.cpp))  
	$(RUN_MPICXX) -o $@ -c $< 
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
ifndef MODULE
#src/%.o:  src/%.cpp  include/aiwlib/* include/aiwlib/magnets/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cpp $@
#swig/%.o: swig/%.cxx include/aiwlib/* include/aiwlib/magnets/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cxx $@
src/%.o:  src/%.cpp  include/aiwlib/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cpp $@
swig/%.o: swig/%.cxx include/aiwlib/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cxx $@
else
$(strip $(dir $(MODULE))$(subst \,,$(shell $(CXX) $(CXXOPT) -M $(MODULE))))
	$(RUN_CXX) -o $(basename $(MODULE)).o -c $(MODULE)
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
bin/arr2seg-Y bin/arrconv bin/isolines bin/dat2mesh bin/fv-slice: bin/%: src/bin/%.o libaiw.a; $(RUN_CXX) -o $@ $^ $(LINKOPT)
#-------------------------------------------------------------------------------
#   viewers
#-------------------------------------------------------------------------------
VIEWERS:=splt mplt fplt
headers_splt = AbstractViewer/plottable.hpp include/aiwlib/fv_interface.hpp
objects_splt =
headers_mplt = AbstractViewer/plottable.hpp include/aiwlib/mview_format.hpp
objects_mplt = src/mview_format.o
headers_fplt = AbstractViewer/plottable.hpp
objects_fplt =
aiwinst_splt =
aiwinst_mplt =
aiwinst_fplt = MeshF3-float-3
include include/aiwlib/xplt.mk
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
	-ln -s "$$(pwd)/include/aiwlib" $(INCLUDEDIR)
	-ln -s "$$(pwd)/python/aiwlib"  $(PYTHONDIR)
	-ln -s "$$(pwd)/libaiw.a"  $(LIBDIR)
	-for i in $(BIN_LIST); do ln -s "$$(pwd)/bin/$$i" $(BINDIR); done
#-------------------------------------------------------------------------------
