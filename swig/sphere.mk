# Copyright (C) 2017, 2021 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0

$(SPHERE_NAME): python$(python)/aiwlib/$(SPHERE_NAME).py python$(python)/aiwlib/_$(SPHERE_NAME).so iostream swig; 
#python/aiwlib/_$(SPHERE_NAME).so: src/sphere.o
build/swig/$(SPHERE_NAME).py build/swig/$(SPHERE_NAME)_wrap.cxx: build/src/sphere.d
build/swig/$(SPHERE_NAME).i: swig/sphere.mk
	@mkdir -p build/swig
	$(imodule)
	@echo '%{#include "../include/aiwlib/sphere"%}' >> $@
	@echo '%include "../include/aiwlib/vec"' >> $@
	@echo '%include "../include/aiwlib/sphere"' >> $@
	@echo '%template($(SPHERE_NAME)) aiw::Sphere<$(SPHERE_TYPE) >;' >> $@
	@echo '%pythoncode %{$(SPHERE_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from vec import *%}' >> $@	
	@echo '%pythoncode %{$(SPHERE_NAME).__iter__ = lambda self: ((self.center(i), self[i], self.area(i)) for i in xrange(self.size()))%}' >> $@	
