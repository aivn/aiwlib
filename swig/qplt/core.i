%module core
%begin%{
#define SWIG_PYTHON_STRICT_BYTE_CHAR
%}

%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%include "std_string.i"
%include "std_vector.i"

%typemap(in) int[ANY] (int temp[$1_dim0]) {
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expected a sequence");
    SWIG_fail;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError, "Size mismatch. Expected $1_dim0 elements");
    SWIG_fail;
  }
  for (int i = 0; i < $1_dim0; i++) {
    PyObject *o = PySequence_GetItem($input, i);
    if (PyNumber_Check(o)) {
      temp[i] = (int) PyLong_AsLong(o);
    } else {
      PyErr_SetString(PyExc_ValueError, "Sequence elements must be numbers");      
      SWIG_fail;
    }
  }
  $1 = temp;
}

%typemap(in) float[ANY] (float temp[$1_dim0]) {
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expected a sequence");
    SWIG_fail;
  }
  if (PySequence_Length($input) != $1_dim0) {
    PyErr_SetString(PyExc_ValueError, "Size mismatch. Expected $1_dim0 elements");
    SWIG_fail;
  }
  for (int i = 0; i < $1_dim0; i++) {
    PyObject *o = PySequence_GetItem($input, i);
    if (PyNumber_Check(o)) {
      temp[i] = (float) PyFloat_AsDouble(o);
    } else {
      PyErr_SetString(PyExc_ValueError, "Sequence elements must be numbers");      
      SWIG_fail;
    }
  }
  $1 = temp;
}

%typemap(out) float [ANY] {
  $result = PyList_New($1_dim0);
  for(int i = 0; i < $1_dim0; i++) {
    PyObject *o = PyFloat_FromDouble((double) $1[i]);
    PyList_SetItem($result, i, o);
  }
}
%typemap(out) int [ANY] {
  $result = PyList_New($1_dim0);
  for(int i = 0; i < $1_dim0; i++) {
    PyObject *o = PyLong_FromLong((double) $1[i]);
    PyList_SetItem($result, i, o);
  }
}

%{
#include "../../include/aiwlib/qplt/base"
%}
		 
%include "../../include/aiwlib/qplt/base"

%template(std_vector_QpltContainer) std::vector<aiw::QpltContainer*>;

