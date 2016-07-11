%%module %(name)s
%%exception { 
	try{ $action } 
	catch(const char *e){ PyErr_SetString(PyExc_RuntimeError, e); return NULL; } 
	catch(...){ return NULL; } 
 }

%%typemap(out) bool&   %%{ $result = PyBool_FromLong    ( *$1 ); %%}
%%typemap(out) char&   %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) short&  %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) int&    %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) long&   %%{ $result = PyInt_FromLong     ( *$1 ); %%}
%%typemap(out) float&  %%{ $result = PyFloat_FromDouble ( *$1 ); %%}
%%typemap(out) double& %%{ $result = PyFloat_FromDouble ( *$1 ); %%}

%%feature("autodoc", "1");
%%{
	namespace aiw{}
	using namespace aiw;
#include "../include/aiwlib/mesh"
%%}
%%include "../include/aiwlib/mesh"

# serialization ???

%%template(%(name)s) aiw::Mesh<%(type)s, %(dim)s>;
%%pythoncode %%{from vec import *%%}
