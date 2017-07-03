$(SPHERE_NAME): python/aiwlib/$(SPHERE_NAME).py python/aiwlib/_$(SPHERE_NAME).so iostream swig; 
#python/aiwlib/_$(SPHERE_NAME).so: src/sphere.o
swig/$(SPHERE_NAME).py swig/$(SPHERE_NAME)_wrap.cxx: include/aiwlib/sphere
swig/$(SPHERE_NAME).i: include/aiwlib/sphere.mk
	$(imodule)
	@echo '%{#include "../include/aiwlib/sphere"%}' >> $@
	@echo '%include "../include/aiwlib/vec"' >> $@
	@echo '%include "../include/aiwlib/sphere"' >> $@
	@echo '%template($(SPHERE_NAME)) aiw::Sphere<$(SPHERE_TYPE) >;' >> $@
	@echo '%pythoncode %{$(SPHERE_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from vec import *%}' >> $@	
