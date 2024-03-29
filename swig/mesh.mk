# Copyright (C) 2017,2021 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0

$(MESH_NAME): $(dst)python$(python)/aiwlib/$(MESH_NAME).py $(dst)python$(python)/aiwlib/_$(dbg_)$(MESH_NAME)$(so) iostream swig; 
$(dst)build/swig/$(MESH_NAME).py $(dst)build/swig/$(MESH_NAME)_wrap.cxx: $(dst)build/swig/$(MESH_NAME).i; $(run_swig)
#$(dst)build/swig/$(MESH_NAME).py $(dst)build/swig/$(python)_$(dbg_)$(MESH_NAME)_wrap.o: $(dst)build/swig/$(python)_$(MESH_NAME).d

$(dst)build/swig/$(MESH_NAME).i: swig/mesh.mk
	@mkdir -p $(dst)build/swig
	$(imodule)
	@echo '%{#include "include/aiwlib/mesh"%}' >> $@
	@echo '%include "include/aiwlib/vec"' >> $@
	@echo '%include "include/aiwlib/base_mesh"' >> $@
	@echo '%include "include/aiwlib/mesh"' >> $@
	@echo '%template(Base$(MESH_NAME)) aiw::BaseMesh<$(MESH_DIM)>;' >> $@
	@echo '%template($(MESH_NAME)) aiw::Mesh<$(MESH_TYPE), $(MESH_DIM)>;' >> $@
	@for D in `seq 1 $$(($(MESH_DIM)-1))`; do echo "%inline %{inline aiw::Mesh<$(MESH_TYPE), $$D> "\
	"$(MESH_NAME)_slice$$D(const aiw::Mesh<$(MESH_TYPE), $(MESH_DIM)> &M, aiw::Ind<$(MESH_DIM)> pos, size_t offset){"\
	"return M.slice<$(MESH_TYPE), $$D>(pos, offset); }%}"; done >> $@
	@echo '%pythoncode %{$(MESH_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from .vec import *%}' >> $@	
	@echo '%pythoncode %{$(MESH_NAME).__C_getitem__, $(MESH_NAME).__C_setitem__ =  $(MESH_NAME).__getitem__, $(MESH_NAME).__setitem__%}' >> $@	
	@echo '%pythoncode %{$(MESH_NAME).__getitem__ = lambda self, k: self.__C_getitem__(Ind(k) if type(k) in (tuple, list) else k)%}' >> $@	
	@echo '%pythoncode %{$(MESH_NAME).__setitem__ = lambda self, k, v: self.__C_setitem__(Ind(k) if type(k) in (tuple, list) else k, v)%}' >> $@
	@echo '%pythoncode %{$(MESH_NAME).__iter__ = lambda self: ((pos, self[pos]) for pos in iterbox(self.bbox()))%}' >> $@
	@for D in `seq 1 $$(($(MESH_DIM)-1))`;\
	do echo "%pythoncode %{$(MESH_NAME).slice$$D = "\
	"lambda self, pos, offset=0:_$(MESH_NAME).$(MESH_NAME)_slice$$D(self, Ind(pos, D=$(MESH_DIM)), offset) %}"; done >> $@
#	python -c "for l in open('swig/mesh.i'): print l[:-1]%{'name':'$(MESH_NAME)', 'type':'$(MESH_TYPE)', 'dim':'$(MESH_DIM)'}" > swig/$(MESH_NAME).i
