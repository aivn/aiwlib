$(MESH_NAME): python/aiwlib/$(MESH_NAME).py python/aiwlib/_$(MESH_NAME).so iostream swig; 
swig/$(MESH_NAME).py swig/$(MESH_NAME)_wrap.cxx: include/aiwlib/mesh
swig/$(MESH_NAME).i: include/aiwlib/mesh.mk
	$(imodule)
	@echo '%{#include "../include/aiwlib/mesh"%}' >> $@
	@echo '%include "../include/aiwlib/mesh"' >> $@
	@echo '%template($(MESH_NAME)) aiw::Mesh<$(MESH_TYPE), $(MESH_DIM)>;' >> $@
	@for D in `seq 1 $$(($(MESH_DIM)-1))`; do echo "%inline %{inline aiw::Mesh<$(MESH_TYPE), $$D> "\
	"$(MESH_NAME)_slice$$D(const aiw::Mesh<$(MESH_TYPE), $(MESH_DIM)> &M, aiw::Ind<$(MESH_DIM)> pos, size_t offset){"\
	"return M.slice<$(MESH_TYPE), $$D>(pos, offset); }%}"; done >> $@
	@echo '%pythoncode %{$(MESH_NAME).__setstate__ = _setstate %}' >> $@
	@echo '%pythoncode %{from vec import *%}' >> $@	
	@for D in `seq 1 $$(($(MESH_DIM)-1))`;\
	do echo "%pythoncode %{$(MESH_NAME).slice$$D = "\
	"lambda self, pos, offset=0:_$(MESH_NAME).$(MESH_NAME)_slice$$D(self, Ind(pos, D=$(MESH_DIM)), offset) %}"; done >> $@
#	python -c "for l in open('swig/mesh.i'): print l[:-1]%{'name':'$(MESH_NAME)', 'type':'$(MESH_TYPE)', 'dim':'$(MESH_DIM)'}" > swig/$(MESH_NAME).i
