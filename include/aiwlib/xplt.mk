GL_LINKOPT=-lglut -lGL -lGLU -lGLEW
$(VIEWERS):%: AV  $(shell echo python/aiwlib/{_%.so,%.py}) $(shell echo AbstractViewer/{_viewer.so,viewer.py}) iostream swig;
AbstractViewer/%.o: AV
AV:
	$(MAKE) -C AbstractViewer
define swig_template=
swig/$(1).py swig$(1)_wrap.cxx: $$(headers_$(1)) include/aiwlib/$(1).hpp
endef
$(foreach VIEW,$(VIEWERS),$(eval $(call swig_template,$(VIEW))))
$(foreach VIEW,$(VIEWERS),swig/$(VIEW).i):swig/%.i : include/aiwlib/xplt.mk
	@echo $@: $^
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
	@for i in $(headers_$(shell basename $@ .i)) "include/aiwlib/$(shell basename $@ .i).hpp"; do echo "#include \"../$$i\"" >> $@; done
	@echo "%}" >> $@	
	@for i in $(headers_$(shell basename $@ .i)) "include/aiwlib/$(shell basename $@ .i).hpp"; do echo "%include \"../$$i\"" >> $@; done
	@echo "%pythoncode %{from aiwlib.vec import *%}" >> $@
	@echo '%apply const std::string& {std::string* appends_names};'>>$@
	@echo '%array_class(float, float_array);'>>$@
	@echo '%array_class(int, int_array);'>>$@
define so_template=
python/aiwlib/_$(1).so:  $$(objects_$(1)) swig/$(1)_wrap.o $(shell echo AbstractViewer/{plottable,shaderprog,viewer_template}.o) libaiw.a
	$$(show_target)
	$(GCC) -shared -o $$@ $$^ $(LINKOPT) $(GL_LINKOPT)
endef
$(foreach VIEW,$(VIEWERS),$(eval $(call so_template,$(VIEW))))
