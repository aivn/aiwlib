%module view
%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%include "std_string.i"
%include "std_vector.i"
%{
#include "../include/aiwlib/typeinfo"
#include "../include/aiwlib/view/color"
#include "../include/aiwlib/view/base"
#include "../include/aiwlib/view/mesh"
#include "../include/aiwlib/view/sphere"
#ifdef AIW_VIEW_AMR
#include "../include/aiwlib/view/amr"
#endif
#ifdef AIW_VIEW_ZCUBE	
#include "../include/aiwlib/view/zcube"
#endif
#ifdef AIW_VIEW_UMESH3D
#include "../include/aiwlib/view/umesh3D"
#endif
%}

%include "../include/aiwlib/vec"
%include "../include/aiwlib/typeinfo"
%include "../include/aiwlib/view/color"
%include "../include/aiwlib/view/base"
%include "../include/aiwlib/view/mesh"
%include "../include/aiwlib/view/sphere"

#ifdef AIW_VIEW_AMR
%include "../include/aiwlib/view/amr"
#else
%pythoncode %{
class AdaptiveMeshView: load = lambda self, fin: False
%}
#endif

#ifdef AIW_VIEW_ZCUBE	
%include "../include/aiwlib/view/zcube"
#else
%pythoncode %{
class ZCubeView:  load = lambda self, fin: False
%}
#endif

#ifdef AIW_VIEW_UMESH3D
%include "../include/aiwlib/view/umesh3D"
#else
%pythoncode %{
class UnorderedMesh3DHead:  load = lambda self, fin: False
%}
#endif

%template(access_vector) std::vector<aiw::CellFieldAccess>;
