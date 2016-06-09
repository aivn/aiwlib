all: iostream swig;
#-------------------------------------------------------------------------------
python/%.py: swig/%.py; @cp $< $@
swig/%.py swig/%_wrap.cxx: swig/%.i; swig -Wall -python -c++ $<
python/_%.so:  swig/%_wrap.cxx
	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o $@ $<

#-------------------------------------------------------------------------------
iostream: python/iostream.py python/_iostream.so;
swig/iostream.py swig/iostream_wrap.cxx: include/iostream
#python/iostream.py: swig/iostream.py; @cp swig/iostream.py python/iostream.py
#swig/iostream.py swig/iostream_wrap.cxx: swig/iostream.i include/iostream; swig -Wall -python -c++ swig/iostream.i
#python/_iostream.so: include/debug include/iostream include/alloc swig/iostream_wrap.cxx
#	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o python/_iostream.so swig/iostream_wrap.cxx
python/_iostream.so: include/debug include/iostream include/alloc
#-------------------------------------------------------------------------------
swig: python/swig.py python/_swig.so;
swig/swig.py swig/swig_wrap.cxx: include/swig
#python/swig.py: swig/swig.py; @cp swig/swig.py python/swig.py
#swig/swig.py swig/swig_wrap.cxx: swig/swig.i include/swig; swig -Wall -python -c++ swig/swig.i
#python/_swig.so: include/swig swig/swig_wrap.cxx
#	g++-4.8 -Wall -fPIC -shared -O3 -g -std=c++11 -I/usr/include/python2.7 -o python/_swig.so swig/swig_wrap.cxx
python/_swig.so: include/swig
#-------------------------------------------------------------------------------
ifndef MESH_NAME
Mesh%: 
	$(MAKE) --no-print-directory $(word 1,$(subst -, ,$@)) MESH_NAME:=$(word 1,$(subst -, ,$@)) \
	MESH_TYPE:="$(word 2,$(subst -, ,$@))" MESH_DIM:="$(word 3,$(subst -, ,$@))" 
	@echo $(foreach t,$(sort $(shell cat .Targets) $@),"$t") > .Targets
else
$(MESH_NAME): python/$(MESH_NAME).py python/_$(MESH_NAME).so iostream swig; 
python/_$(MESH_NAME).so: include/debug include/iostream include/binaryio include/alloc include/vec include/mesh
swig/$(MESH_NAME).py swig/$(MESH_NAME)_wrap.cxx: include/mesh
swig/$(MESH_NAME).i: swig/mesh.i
	python -c "for l in open('swig/mesh.i'): print l[:-1]%{'name':'$(MESH_NAME)', 'type':'$(MESH_TYPE)', 'dim':'$(MESH_DIM)'}" > swig/$(MESH_NAME).i
endif
#-------------------------------------------------------------------------------
clean:; rm -rf swig/*.o swig/*.py swig/*_wrap.cxx python/_*.so python/{swig,iostream}.py
