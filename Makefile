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
all: iostream swig view MeshF1-float-1 MeshF2-float-2 MeshF3-float-3 MeshUS2-uint16_t-2 MeshUS3-uint16_t-3 SphereD-double SphereF-float SphereUS-uint16_t $(shell if [ -f TARGETS ]; then cat TARGETS; fi)
endif

ifneq (off,$(mpi)) 
ifeq ($(shell if $(MPICXX) -v &> /dev/null; then echo OK; fi),OK)
all: mpi4py
endif
endif

ifeq (on,$(bin)) 
all: $(shell echo bin/{arr2seg-Y,isolines,dat2mesh,fv-slice,uplt-remote})
endif

ifeq (on,$(ezz))
all: mplt fplt splt
endif

all: libaiw.a;

iostream swig mpi4py view: %: python/aiwlib/%.py python/aiwlib/_%.so;
.PRECIOUS: swig/%.py swig/%.o src/%.o
#-------------------------------------------------------------------------------
libaiw.a: $(shell echo src/{debug,sphere,configfile,segy,isolines,checkpoint,mixt,racs,farfield,typeinfo,binary_format,view/{color,mesh}}.o)
	rm -f libaiw.a; ar -csr libaiw.a $^

#,amr,zcube,umesh3D,vtexture

#libaiw.a: $(shell echo src/{debug,sphere,configfile,segy,isolines,checkpoint,mixt,racs,farfield,amrview,view/{images,color,mesh}}.o); rm -f libaiw.a; ar -csr libaiw.a $^
#libaiw.a: $(shell echo src/{debug,sphere,configfile,segy,isolines,checkpoint,mixt,racs,plot2D,farfield}.o); rm -f libaiw.a; ar -csr libaiw.a $^
#-------------------------------------------------------------------------------
#   run SWIG
#-------------------------------------------------------------------------------
swig/swig.py swig/swig_wrap.cxx: include/aiwlib/swig
swig/iostream.py swig/iostream_wrap.cxx: include/aiwlib/iostream include/aiwlib/gzstream 
swig/mpi4py.py swig/mpi4py_wrap.cxx: include/aiwlib/mpi4py
#swig/plot2D.py swig/plot2D_wrap.cxx: $(shell echo include/aiwlib/{vec,mesh,sphere,amrview,plot2D,view/{images,color}})
swig/view.py swig/view_wrap.cxx: $(shell echo include/aiwlib/{vec,typeinfo,view/{color,base,mesh,sphere}})
# +amr,zcube,umesh3D,vtexture

python/aiwlib/%.py: swig/%.py
	@echo 'try: import sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())' > $@
	@echo 'except: pass' >> $@
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
src/view/%.o:  src/view/%.cpp  include/aiwlib/* include/aiwlib/view/*; @$(MAKE) --no-print-directory MODULE:=$(basename $@).cpp $@
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
bin/arr2seg-Y bin/arrconv bin/isolines bin/dat2mesh bin/fv-slice bin/aiw-diff bin/uplt-remote: bin/%: src/bin/%.o libaiw.a; $(RUN_CXX) -o $@ $^ $(LINKOPT)
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
clean:; rm -rf swig/*.o src/*.o src/view/*.o src/bin/*.o python/aiwlib/_*.so   
cleanall: clean 
	@for i in $$(ls swig/*.py 2> /dev/null); do echo rm -f $$i python/aiwlib/$$(basename $$i){,c}; rm -f $$i python/aiwlib/$$(basename $$i){,c}; done
	rm -f swig/*_wrap.cxx 
	-@for i in $$(cat TARGETS); do echo rm -f swig/$${i%%-*}.i; rm -f swig/$${i%%-*}.i; done
clean-%:; -n=$@; rm swig/$${n:6}_wrap.o python/aiwlib/_$${n:6}.so
cleanall-%: clean-%; -n=$@; rm swig/$${n:9}.py swig/$${n:9}_wrap.cxx swig/$${n:9}.i python/aiwlib/$${n:9}.py{,c}
clean-mingw clean-windows:; rm -f mingw/*.o mingw/obj/*.o mingw/view/*.o windows/aiwlib/* windows/uplt 
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
#   windows
#-------------------------------------------------------------------------------
windows: $(shell echo windows/aiwlib/{{__init__,vec,swig,tkPlot2D,tkConfView,tkWidgets,iostream,view}.py,_{iostream,swig,view}.pyd}) windows/uplt.py;
windows/aiwlib/%.py: python/aiwlib/%.py; cp $< $@
windows/uplt.py: bin/uplt; cp $< $@
mingw/%_wrap.o: swig/%_wrap.cxx; $(MINGW) $(MINGW_OPT) -o $@ -c $<
mingw/obj/%.o: src/%.cpp; $(MINGW) $(MINGW_OPT) -o $@ -c $<
mingw/view/%.o: src/view/%.cpp; $(MINGW) $(MINGW_OPT) -o $@ -c $<
windows/aiwlib/_%.pyd: mingw/%_wrap.o; $(MINGW) -shared -o $@ $^ $(MINGW_LINKOPT) 
windows/aiwlib/_iostream.pyd: mingw/obj/debug.o
windows/aiwlib/_view.pyd: $(shell echo mingw/obj/{debug,sphere,binary_format,segy}.o  mingw/view/{color,mesh}.o) 
#-------------------------------------------------------------------------------
