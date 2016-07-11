%module iostream
%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%include "std_string.i"
%{
#include "../include/aiwlib/iostream"
%}
%include "../include/aiwlib/iostream"

