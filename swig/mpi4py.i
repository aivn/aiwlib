%module mpi4py
%exception { 
	try{ $action } 
	catch(const char *e){ PyErr_SetString(PyExc_RuntimeError, e); return NULL; } 
	catch(...){ return NULL; } 
 }
%feature("autodoc", "1");
%include "std_string.i"
%{
#include "../include/aiwlib/mpi4py"
%}
%include "../include/aiwlib/mpi4py"

%pythoncode {
import cPickle
def mpi_send(data, proc): 
	"mpi_send(data, proc=-1) -> bool"
	return _mpi4py.mpi_send(cPickle.dumps(data), proc)
def mpi_recv(proc): 
	"mpi_recv(proc=-1) -> (data, proc)"
	msg = aiw_mpi_msg_t() 
	if not _mpi4py.mpi_recv(msg, proc): raise Exception("mpi_recv(proc=%i) failed"%proc)
	return cPickle.loads(msg.data), msg.source
}

