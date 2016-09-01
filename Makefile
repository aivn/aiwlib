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

all: iostream swig;
#-------------------------------------------------------------------------------
python/aiwlib/%.py: swig/%.py; @cp $< $@
swig/%.py swig/%_wrap.cxx: swig/%.i 
	$(show_target)
	$(SWIG) $(SWIGOPT) $<
python/aiwlib/_%.so:  swig/%_wrap.cxx
	$(show_target)
	$(GCC) -shared -I$(PYTHON_H_PATH) -o $@ $<
#-------------------------------------------------------------------------------
iostream: python/aiwlib/iostream.py python/aiwlib/_iostream.so;
swig/iostream.py swig/iostream_wrap.cxx: include/aiwlib/iostream
#python/iostream.py: swig/iostream.py; @cp swig/iostream.py python/iostream.py
#swig/iostream.py swig/iostream_wrap.cxx: swig/iostream.i include/iostream; swig -Wall -python -c++ swig/iostream.i
#python/_iostream.so: include/debug include/iostream include/alloc swig/iostream_wrap.cxx
#	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o python/_iostream.so swig/iostream_wrap.cxx
python/aiwlib/_iostream.so: include/aiwlib/debug include/aiwlib/iostream include/aiwlib/alloc
#-------------------------------------------------------------------------------
swig: python/aiwlib/swig.py python/aiwlib/_swig.so;
swig/swig.py swig/swig_wrap.cxx: include/aiwlib/swig
#python/swig.py: swig/swig.py; @cp swig/swig.py python/swig.py
#swig/swig.py swig/swig_wrap.cxx: swig/swig.i include/swig; swig -Wall -python -c++ swig/swig.i
#python/_swig.so: include/swig swig/swig_wrap.cxx
#	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o python/_swig.so swig/swig_wrap.cxx
python/aiwlib/_swig.so: include/aiwlib/swig
#-------------------------------------------------------------------------------
ifndef MESH_NAME
Mesh%: 
	$(MAKE) --no-print-directory $(word 1,$(subst -, ,$@)) MESH_NAME:=$(word 1,$(subst -, ,$@)) \
	MESH_TYPE:="$(word 2,$(subst -, ,$@))" MESH_DIM:="$(word 3,$(subst -, ,$@))" 
	@echo $(foreach t,$(sort $(shell cat .Targets) $@),"$t") > .Targets
else
$(MESH_NAME): python/aiwlib/$(MESH_NAME).py python/aiwlib/_$(MESH_NAME).so iostream swig; 
python/aiwlib/_$(MESH_NAME).so: include/aiwlib/debug include/aiwlib/iostream include/aiwlib/binaryio include/aiwlib/alloc include/aiwlib/vec include/aiwlib/mesh
swig/$(MESH_NAME).py swig/$(MESH_NAME)_wrap.cxx: include/aiwlib/mesh
swig/$(MESH_NAME).i: swig/mesh.i
	python -c "for l in open('swig/mesh.i'): print l[:-1]%{'name':'$(MESH_NAME)', 'type':'$(MESH_TYPE)', 'dim':'$(MESH_DIM)'}" > swig/$(MESH_NAME).i
endif
#-------------------------------------------------------------------------------
clean:; rm -rf swig/*.o python/aiwlib/_*.so 
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
