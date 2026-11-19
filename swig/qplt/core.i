%module core
%begin%{
#define SWIG_PYTHON_STRICT_BYTE_CHAR
%}

// every C++ exception becomes a Python RuntimeError with its message (returning NULL without an error set
// gave "SystemError: error return without exception set"); WRAISE throws const char* (Windows) or std::stringstream
%exception { try{ $action }
	catch(const char *e){ PyErr_SetString(PyExc_RuntimeError, e); return NULL; }
	catch(const std::stringstream &e){ PyErr_SetString(PyExc_RuntimeError, e.str().c_str()); return NULL; }
	catch(const std::string &e){ PyErr_SetString(PyExc_RuntimeError, e.c_str()); return NULL; }
	catch(const std::exception &e){ PyErr_SetString(PyExc_RuntimeError, e.what()); return NULL; }
	catch(...){ PyErr_SetString(PyExc_RuntimeError, "unknown C++ exception"); return NULL; } }
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
    PyObject *o = PySequence_GetItem($input, i);  // new reference
    if (!o || !PyNumber_Check(o)) {
      Py_XDECREF(o); PyErr_SetString(PyExc_ValueError, "Sequence elements must be numbers");
      SWIG_fail;
    }
    temp[i] = (int) PyLong_AsLong(o); Py_DECREF(o);
    if (PyErr_Occurred()) SWIG_fail;  // conversion error (e.g. overflow)
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
    PyObject *o = PySequence_GetItem($input, i);  // new reference
    if (!o || !PyNumber_Check(o)) {
      Py_XDECREF(o); PyErr_SetString(PyExc_ValueError, "Sequence elements must be numbers");
      SWIG_fail;
    }
    temp[i] = (float) PyFloat_AsDouble(o); Py_DECREF(o);
    if (PyErr_Occurred()) SWIG_fail;  // conversion error (e.g. overflow)
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

%define CU_HOST %enddef
%define CU_DEVICE %enddef
%define CU_GLOBAL %enddef
%define CU_HD %enddef

%{
#include "include/aiwlib/qplt/base"
%}

// aiw::Ind<N>/Vecf<N> are not wrapped: these returned opaque objects that leaked ("swig/python detected a memory leak
// of type 'aiw::Ind< 6 > *', no destructor found"); Python uses get_bbox(axe) and the int/float array members instead
%ignore aiw::QpltContainer::get_bbox() const;
%ignore aiw::QpltFlat::spos;
%ignore aiw::QpltFlat::ppf;
%ignore aiw::QpltFlat::rn;
%ignore aiw::QpltFlat::bbox;
		 
%include "include/aiwlib/qplt/base"

%template(std_vector_QpltContainer) std::vector<aiw::QpltContainer*>;

