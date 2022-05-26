# Copyright (C) 2016-2021 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
#
# Usage:
#    make [CXX|MPICXX|NVCC|MINGW=<custom compiler>] 
# or edit file 'include/aiwlib/config.mk' for select custom compilers permanently;
#
#    make [zlib=off] [swig=off] [png=off] [pil=off] [bin=off] 
# or edit file 'include/aiwlib/config.mk' for change main target 'all' and list of aiwlib C++-core modules.  
#
#    make ... debug=on   for make debug version
#
# See include/aiwlib/config.mk and doc/aiwlib.pdf for details.

include include/aiwlib/config.mk
#-------------------------------------------------------------------------------
#   main targets
#-------------------------------------------------------------------------------
#.PHONY: clean all install-links

libaiw_n=debug,sphere,configfile,segy,isolines,checkpoint,mixt,racs,farfield,typeinfo
qplt_n=imaging,accessor,base,mesh,mesh_cu,vtexture
ifeq (on,$(bin))
bin_n=arr2segY,segY2arr,isolines,sph2dat,arrconv,aiw-diff
#dat2mesh,fv-slice,uplt-remote
endif
#-------------------------------------------------------------------------------
ifeq (on,$(swig)) 
all: iostream swig MeshF1-float-1 MeshF2-float-2 MeshF3-float-3 SphereF-float $(shell if [ -f $(dst)TARGETS ]; then cat $(dst)TARGETS; fi)
#all:  MeshUS2-uint16_t-2 MeshUS3-uint16_t-3 SphereD-double SphereUS-uint16_t 
endif

all: $(shell echo $(dst)bin/{$(bin_n)})

ifeq (on,$(ezz))
all: mplt fplt splt
endif

ifeq (3,$(python))
all: qplt
endif

#ifeq (on,$(mpi)) 
#ifeq ($(shell if $(MPICXX) -v &> /dev/null; then echo OK; fi),OK)
#all: mpi4py
#endif
#endif
#-------------------------------------------------------------------------------
dst?=
so?=.so
py_dst?=$(dst)python$(python)

all: $(dst)libaiw$(_dbg).a
ifneq (,$(dst))
	cp -up python$(python)/aiwlib/*.py $(py_dst)/aiwlib/
else
	@echo target \'all\' OK
endif

.PRECIOUS: $(dst)build/swig/%.py $(dst)build/swig/%_wrap.cxx $(dst)build/swig/%/core_wrap.cxx
.PRECIOUS: $(dst)build/src/%.d $(dst)build/src/bin/%.d $(dst)build/src/qplt/%.d $(dst)build/swig/%.d $(dst)build/swig/%/$(python)_core.d
.PRECIOUS: $(dst)build/src/%.o $(dst)build/src/bin/%.o $(dst)build/src/qplt/%.o $(dst)build/swig/$(python)_$(dbg_)%_wrap.o $(dst)build/swig/%/$(python)_$(dbg_)core_wrap.o
#-------------------------------------------------------------------------------
#   C++ core
#-------------------------------------------------------------------------------
src_o=$(shell echo $(dst)build/src/$(dbg_){$(libaiw_n)}.o)

$(dst)libaiw$(_dbg).a: $(src_o)
	$(show_target)
	rm -f $@; $(AR) -csr $@ $^
#-------------------------------------------------------------------------------
#   compile object files
#-------------------------------------------------------------------------------
$(dst)build/src/%.d: src/%.cpp
	@mkdir -p $(dst)build/src
	@p="$@"; echo -n "$@ $${p%.*}.o $(dst)build/dbg_" > $@
	@$(CXX) -M $< >> $@
$(dst)build/src/bin/%.d: src/bin/%.cpp
	@mkdir -p $(dst)build/src/bin
	@p="$@"; echo -n "$@ $${p%.*}.o $(dst)build/bin/dbg_" > $@
	@$(CXX) -M $< >> $@
$(dst)build/src/qplt/%.d: src/qplt/%.cpp
	@mkdir -p $(dst)build/src/qplt
	@p="$@"; echo -n "$@ $${p%.*}.o $(dst)build/qplt/dbg/qplt_" > $@
	@$(CXX) -M $< >> $@

-include $(shell echo $(dst)build/src/{$(libaiw_n)}.d $(dst)build/src/bin/{$(bin_n)}.d $(dst)build/src/qplt/{$(qplt_n)}.d)

$(dst)build/src/$(dbg_)%.o: src/%.cpp $(dst)build/src/%.d; $(run_cxx) 
$(dst)build/src/bin/$(dbg_)%.o: src/bin/%.cpp $(dst)build/src/bin/%.d; $(run_cxx)
$(dst)build/src/qplt/$(dbg_)%.o: src/qplt/%.cpp $(dst)build/src/qplt/%.d; $(run_cxx)
#-------------------------------------------------------------------------------
#   run swig and make shared libs
#-------------------------------------------------------------------------------
$(dst)build/swig/%.py: $(dst)build/swig/%_wrap.cxx;
$(dst)build/swig/%/core.py: $(dst)build/swig/%/core_wrap.cxx;
$(dst)build/swig/%_wrap.cxx: swig/%.i; $(run_swig)
$(dst)build/swig/%/core_wrap.cxx: swig/%/core.i; $(run_swig)
$(py_dst)/aiwlib/%.py: $(dst)build/swig/%.py; ./patch_swig.py $< $@
$(py_dst)/aiwlib/%/core.py: $(dst)build/swig/%/core.py; ./patch_swig.py $< $@

$(dst)build/swig/%_wrap.d: swig/%.i
	@mkdir -p $(dst)build/swig
	$(SWIG) $(SWIGOPT) -M $< >> $@
$(dst)build/swig/%/core_wrap.d: swig/%/core.i
	@mkdir -p $$(dirname $@)
	$(SWIG) $(SWIGOPT) -M $< >> $@
$(dst)build/swig/$(python)_%.d: $(dst)build/swig/%_wrap.cxx
	@mkdir -p $(dst)build/swig
	@p=$$(basename $@); echo -n "$@ " $(dst)build/swig/$(python)_dbg_$${p%.*}_wrap.o $(dst)build/swig/$(python)_ > $@
	@$(CXX) -I $(PYTHON_H_PATH) -I./ -M $< >> $@
$(dst)build/swig/%/$(python)_core.d: $(dst)build/swig/%/core_wrap.cxx
	@mkdir -p $$(dirname $@)
	@p=$$(dirname $@); echo -n "$@ " $$p/$(python)_dbg_core_wrap.o $$p/$(python)_ > $@
	@$(CXX) -I $(PYTHON_H_PATH) -I./ -M $< >> $@


ifeq (on,$(swig))
include $(shell echo $(dst)build/swig/$(python)_{iostream,swig}.d)
#, mpi4py
ifeq (3,$(python))
include $(dst)build/swig/qplt/$(python)_core.d
endif
endif

iostream swig mpi4py: %: $(py_dst)/aiwlib/%.py $(py_dst)/aiwlib/_$(dbg_)%$(so);

$(dst)build/swig/$(python)_$(dbg_)%_wrap.o: $(dst)build/swig/%_wrap.cxx $(dst)build/swig/$(python)_%.d; $(run_cxx) 
$(dst)build/swig/%/$(python)_$(dbg_)core_wrap.o: $(dst)build/swig/%/core_wrap.cxx $(dst)build/swig/%/$(python)_core.d; $(run_cxx) 

$(py_dst)/aiwlib/_$(dbg_)%$(so): $(dst)build/swig/$(python)_$(dbg_)%_wrap.o $(dst)libaiw$(_dbg).a
	$(show_target)
	$(CXX) -shared -o $@ $^ $(LINKOPT)  $(dst)libaiw$(_dbg).a
$(py_dst)/aiwlib/%/_$(dbg_)core$(so): $(dst)build/swig/%/$(python)_$(dbg_)core_wrap.o $(dst)libaiw$(_dbg).a
	$(show_target)
	$(CU_LD) -shared -o $@ $^ $(LINKOPT)  $(dst)libaiw$(_dbg).a
#---  qplt  --------------------------------------------------------------------
$(py_dst)/aiwlib/qplt/_$(dbg_)core$(so): $(shell echo $(dst)build/src/qplt/$(dbg_){$(qplt_n)}.o)
qplt: $(py_dst)/aiwlib/qplt/core.py $(py_dst)/aiwlib/qplt/_$(dbg_)core$(so)
ifneq (,$(dst))
	cp -upr python$(python)/aiwlib/qplt/*.py python$(python)/aiwlib/qplt/{qplt.ui,pals} $(py_dst)/aiwlib/qplt/
	mkdir -p $(dst)build/bin && cp bin/qplt $(dst)build/bin/qplt.py && cd $(dst)build/bin/ && wine pyinstaller --target-architecture x86_64 qplt.py
	cp -r python3/aiwlib/qplt/{qplt.ui,pals} $(dst)build/bin/dist/qplt/aiwlib/qplt/ && cp dat/llbe/Q4T.msh $(dst)build/bin/dist/qplt/
	cd $(dst)build/bin/dist/ && rm -f qplt.zip && zip -r qplt.zip qplt/
else
	@echo target \'qplt\' OK
endif
#cp /usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll qplt/

ifneq (on,$(mingw))
qplt: $(dst)bin/qplt-remote
endif

ifeq (on,$(cuda))
$(dst)build/src/qplt/%_cu.o: src/qplt/%_cu.cpp $(dst)build/src/qplt/%_cu.d
	$(show_target)
	$(NVCC) $(NVCCOPT) -o $@ -c $<
endif
#---  Mesh  --------------------------------------------------------------------
ifndef MESH_NAME
Mesh%: 
	@$(MAKE) --no-print-directory $(word 1,$(subst -, ,$@)) MESH_NAME:=$(word 1,$(subst -, ,$@)) \
	MESH_TYPE:="$(word 2,$(subst -, ,$@))" MESH_DIM:="$(word 3,$(subst -, ,$@))" 
	@echo $(foreach t,$(sort $(shell cat TARGETS) $@),"$t") > TARGETS
else
include swig/mesh.mk $(dst)build/swig/$(python)_$(MESH_NAME).d
endif
#---  Sphere  ------------------------------------------------------------------
ifndef SPHERE_NAME
Sphere%: 
	@$(MAKE) --no-print-directory $(word 1,$(subst -, ,$@)) SPHERE_NAME:=$(word 1,$(subst -, ,$@)) \
	SPHERE_TYPE:="$(word 2,$(subst -, ,$@))" 
	@echo $(foreach t,$(sort $(shell cat TARGETS) $@),"$t") > TARGETS
else
include swig/sphere.mk $(dst)build/swig/$(python)_$(SPHERE_NAME).d
endif
#-------------------------------------------------------------------------------
#   utils
#-------------------------------------------------------------------------------
$(dst)bin/%: $(dst)build/src/bin/%.o
	mkdir -p $$(dirname $@) && $(CXX) -o $@ $^ $(dst)libaiw.a $(LINKOPT)
$(dst)bin/qplt-remote: $(dst)build/src/bin/qplt-remote.o $(shell echo $(dst)build/src/qplt/{$(qplt_n)}.o)
	mkdir -p $$(dirname $@) && $(CU_LD) -o $@ $^ $(dst)libaiw.a $(LINKOPT)  
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
cleanall:
	rm -rf $(dst)build
	$(MAKE) python=2 dst=$(dst) clean
	$(MAKE) python=3 dst=$(dst) clean
clean:
	rm -f $(py_dst)/aiwlib/{,qplt/}_*$(so) $(py_dst)/aiwlib/{iosteam,swig,mpi4py}.py{,c} $(py_dst)/aiwlib/qplt/core.py{,c}
	rm -f $(py_dst)/aiwlib/{Mesh,Sphere}*.py{,c}
clean-bin:;	rm -f bin/{$(bin_n)} $(dst)build/src/bin/*.o
clean-qplt:; rm -rf bin/qplt-remote $(py_dst)/aiwlib/{core.py,_core$(so),_dbg_core$(so)} $(dst)build/src/qplt $(dst)build/src/bin/{dbg_,}qplt-remote.o
clean-qplt-cu:; rm -f bin/qplt-remote $(py_dst)/aiwlib/{_core$(so),_dbg_core$(so)} $(dst)build/src/qplt/*_cu.o
#clean-%:; -n=$@; rm swig/$${n:6}_wrap?.o python$(python)/aiwlib/_$${n:6}.so
#clean-mingw clean-windows:; rm -f mingw/*.o mingw/obj/*.o mingw/view/*.o windows/aiwlib/* windows/uplt 
#-------------------------------------------------------------------------------
uninstall: 
	rm -rf $(INCLUDEDIR)/aiwlib $(PYTHONDIR)/aiwlib $(LIBDIR)/libaiw{,-dbg}.a 
	for i in $(BIN_LIST); do rm -rf $(BINDIR)/$$i; done
install: all uninstall
	-cp -r include/aiwlib $(INCLUDEDIR)
	-cp -r python$(python)/aiwlib  $(PYTHONDIR)
	-cp libaiw.a $(LIBDIR)/
	-cp libaiw-dbg.a $(LIBDIR)/
	-for i in $(BIN_LIST); do cp -f bin/$$i $(BINDIR); done
links-install install-links: all uninstall
	-ln -s "$$(pwd)/include/aiwlib" $(INCLUDEDIR)
	-ln -s "$$(pwd)/python$(python)/aiwlib"  $(PYTHONDIR)
	-ln -s "$$(pwd)/libaiw.a"  $(LIBDIR)
	-ln -s "$$(pwd)/libaiw-dbg.a"  $(LIBDIR)
	-for i in $(BIN_LIST); do ln -s "$$(pwd)/bin/$$i" $(BINDIR); done
#-------------------------------------------------------------------------------
#   windows
#-------------------------------------------------------------------------------
windows: $(shell echo windows/aiwlib/{{__init__,vec,swig,tkPlot2D,tkConfView,tkWidgets,iostream,view}.py,_{iostream,swig,view}.pyd}) windows/uplt.py;
windows/aiwlib/%.py: python$(python)/aiwlib/%.py; cp $< $@
windows/uplt.py: bin/uplt; cp $< $@
mingw/%_wrap$(python).o: swig/%_wrap.cxx; $(MINGW) $(MINGW_OPT) -o $@ -c $<
mingw/obj/%.o: src/%.cpp; $(MINGW) $(MINGW_OPT) -o $@ -c $<
mingw/view/%.o: src/view/%.cpp; $(MINGW) $(MINGW_OPT) -o $@ -c $<
windows/aiwlib/_%.pyd: mingw/%_wrap$(python).o; $(MINGW) -shared -o $@ $^ $(MINGW_LINKOPT) 
windows/aiwlib/_iostream.pyd: mingw/obj/debug.o
windows/aiwlib/_view.pyd: $(shell echo mingw/obj/{debug,sphere,binary_format,segy}.o  mingw/view/{color,mesh}.o) 
#-------------------------------------------------------------------------------
