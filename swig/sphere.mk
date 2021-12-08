# Copyright (C) 2017, 2021 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0

$(SPHERE_NAME): $(dst)python$(python)/aiwlib/$(SPHERE_NAME).py $(dst)python$(python)/aiwlib/_$(dbg_)$(SPHERE_NAME)$(so) iostream swig; 
$(dst)build/swig/$(SPHERE_NAME).py $(dst)build/swig/$(SPHERE_NAME)_wrap.cxx: $(dst)build/swig/$(SPHERE_NAME).i; $(run_swig)
#$(dst)build/swig/$(SPHERE_NAME).py $(dst)build/swig/$(python)_$(dbg_)$(SPHERE_NAME)_wrap.o: $(dst)build/swig/$(python)_$(SPHERE_NAME).d
#$(dst)python/aiwlib$(python)/_$(dbg_)$(SPHERE_NAME)$(so): $(dst)build/dbg/sphere.o

$(dst)build/swig/$(SPHERE_NAME).i: swig/sphere.mk
	@mkdir -p $(dst)build/swig
	$(imodule)
	@echo '%{#include "include/aiwlib/sphere"%}' >> $@
	@echo '%include "include/aiwlib/vec"' >> $@
	@echo '%include "include/aiwlib/sphere"' >> $@
	@echo '%template($(SPHERE_NAME)) aiw::Sphere<$(SPHERE_TYPE) >;' >> $@
	@echo '%pythoncode %{$(SPHERE_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from vec import *%}' >> $@	
	@echo '%pythoncode %{$(SPHERE_NAME).__iter__ = lambda self: ((self.center(i), self[i], self.area(i)) for i in xrange(self.size()))%}' >> $@	
