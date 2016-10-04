%%module %(name)s
%%exception { 
	try{ $action } 
	catch(const char *e){ PyErr_SetString(PyExc_RuntimeError, e); return NULL; } 
	catch(...){ return NULL; } 
 }

%%pythoncode %%{
def _setstate(self, state):
    if not hasattr(self, 'this'): self.__init__()
    self.__C_setstate__(state)
def _swig_setattr(self, class_type, name, value):
    if name in class_type.__swig_setmethods__: value = getattr(self, name).__class__(value)
    return _swig_setattr_nondynamic(self, class_type, name, value, 0)
%%}

%%typemap(out) bool&   %%{ $result = PyBool_FromLong    ( *$1 ); %%}
%%typemap(out) char&   %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) short&  %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) int&    %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) long&   %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) float&  %%{ $result = PyFloat_FromDouble ( *$1 ); %%}
%%typemap(out) double& %%{ $result = PyFloat_FromDouble ( *$1 ); %%}

%%feature("autodoc", "1");
%%include "std_string.i"
%%{
	namespace aiw{}
	using namespace aiw;
#include "../include/aiwlib/mesh"
%%}
%%include "../include/aiwlib/mesh"

%%template(%(name)s) aiw::Mesh<%(type)s, %(dim)s>;
%%pythoncode %%{%(name)s.__setstate__ = _setstate %%}
%%pythoncode %%{from vec import *%%}
