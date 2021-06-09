%module qplt
%begin%{
#define SWIG_PYTHON_STRICT_BYTE_CHAR
%}

%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%include "std_string.i"
%include "std_vector.i"

%template(std_vectori) std::vector<int>;
%template(std_vectorf) std::vector<float>;
%template(std_vectors) std::vector<std::string>;

%{
#include "../include/aiwlib/qplt/imaging"
#include "../include/aiwlib/qplt/accessor"
#include "../include/aiwlib/qplt/scene"
#include "../include/aiwlib/qplt/base"
%}
		 
%include "../include/aiwlib/qplt/imaging"
%include "../include/aiwlib/qplt/accessor"
%include "../include/aiwlib/qplt/scene"
%include "../include/aiwlib/qplt/base"

