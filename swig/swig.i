%module swig
%begin%{
#define SWIG_PYTHON_STRICT_BYTE_CHAR
%}
%exception { 
	try{ $action } 
	catch(const char *e){ PyErr_SetString(PyExc_RuntimeError, e); return NULL; } 
	catch(...){ return NULL; } 
 }
%feature("autodoc", "1");
%{
#include "include/aiwlib/swig"
%}
%include "include/aiwlib/swig"

