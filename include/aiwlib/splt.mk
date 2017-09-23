headers=AbstractViewer/plottable.hpp $(shell echo include/aiwlib/{fv_interface.hpp,splt.hpp})
GL_LINKOPT=-lglut -lGL -lGLU -lGLEW
splt: $(shell echo python/aiwlib/{_splt.so,splt.py} AbstractViewer/{_viewer.so,viewer.py}) iostream swig;
AbstractViewer/%:
	$(MAKE) -C AbstractViewer
splt.py swig/splt_wrap.cxx: $(shell echo include/aiwlib/{fv_interface.hpp,splt.hpp}) AbstractViewer/plottable.hpp
swig/splt.i: include/aiwlib/splt.mk
	$(imodule)
	@echo '%include "std_string.i";'>>$@
	@echo '%include "carrays.i";'>>$@
	@echo '%{ #include "../include/aiwlib/vec" %}' >> $@
	@echo '%include "../include/aiwlib/vec"' >> $@
	@echo '%{ #include "../include/aiwlib/mesh" %}' >> $@
	@echo '%include "../include/aiwlib/mesh"' >> $@
	@echo '%{ #include "../include/aiwlib/sphere" %}' >> $@
	@echo '%include "../include/aiwlib/sphere"' >> $@
	@echo "%{" >> $@
	@for i in $(headers); do echo "#include \"../$$i\"" >> $@; done
	@echo "%}" >> $@	
	@for i in $(headers); do echo "%include \"../$$i\"" >> $@; done
	@echo "%pythoncode %{from aiwlib.vec import *%}" >> $@
	@echo '%apply const std::string& {std::string* appends_names};'>>$@
	@echo '%array_class(float, float_array);'>>$@
	@echo '%array_class(int, int_array);'>>$@
python/aiwlib/_splt.so: swig/splt_wrap.o AbstractViewer/plottable.o AbstractViewer/shaderprog.o AbstractViewer/viewer_template.o libaiw.a
	$(show_target)
	$(GCC) -shared -o $@ $^ $(LINKOPT) $(GL_LINKOPT)
