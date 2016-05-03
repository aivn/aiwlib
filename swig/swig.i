%module swig
%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%{
#include "../include/swig"
%}
%include "../include/swig"

