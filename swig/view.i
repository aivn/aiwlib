%module view
%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%include "std_string.i"
%include "std_vector.i"
%{
#include "../include/aiwlib/typeinfo"
#include "../include/aiwlib/view/color"
#include "../include/aiwlib/view/images"
#include "../include/aiwlib/view/base"
#include "../include/aiwlib/view/mesh"
#include "../include/aiwlib/view/sphere"
#include "../include/aiwlib/view/amr"
#include "../include/aiwlib/view/zcube"
#include "../include/aiwlib/view/umesh3D"
%}
%include "../include/aiwlib/vec"
%include "../include/aiwlib/typeinfo"
%include "../include/aiwlib/view/color"
%include "../include/aiwlib/view/images"
%include "../include/aiwlib/view/base"
%include "../include/aiwlib/view/mesh"
%include "../include/aiwlib/view/sphere"
%include "../include/aiwlib/view/amr"
%include "../include/aiwlib/view/zcube"
%include "../include/aiwlib/view/umesh3D"

%template(access_vector) std::vector<aiw::CellFieldAccess>;

#ifndef AIW_NO_PNG
%template(plot_paletter) aiw::plot_paletter<aiw::ImagePNG>;
%template(preview) aiw::MeshView::preview<aiw::ImagePNG>;
%template(plot) aiw::MeshView::plot<aiw::ImagePNG>;
%template(preview) aiw::SphereView::preview<aiw::ImagePNG>;
%template(plot) aiw::SphereView::plot<aiw::ImagePNG>;
%template(preview) aiw::AdaptiveMeshView::preview<aiw::ImagePNG>;
%template(plot) aiw::AdaptiveMeshView::plot<aiw::ImagePNG>;
%template(preview) aiw::ZCubeView::preview<aiw::ImagePNG>;
%template(plot) aiw::ZCubeView::plot<aiw::ImagePNG>;
%template(preview) aiw::UnorderedMesh3DView::preview<aiw::ImagePNG>;
%template(plot) aiw::UnorderedMesh3DView::plot<aiw::ImagePNG>;
#endif

#ifndef AIW_NO_PIL 
%template(plot_paletter) aiw::plot_paletter<aiw::ImagePIL>;
%template(preview) aiw::MeshView::preview<aiw::ImagePIL>;
%template(plot) aiw::MeshView::plot<aiw::ImagePIL>;
%template(preview) aiw::SphereView::preview<aiw::ImagePIL>;
%template(plot) aiw::SphereView::plot<aiw::ImagePIL>;
%template(preview) aiw::AdaptiveMeshView::preview<aiw::ImagePIL>;
%template(plot) aiw::AdaptiveMeshView::plot<aiw::ImagePIL>;
%template(preview) aiw::ZCubeView::preview<aiw::ImagePIL>;
%template(plot) aiw::ZCubeView::plot<aiw::ImagePIL>;
%template(preview) aiw::UnorderedMesh3DView::preview<aiw::ImagePIL>;
%template(plot) aiw::UnorderedMesh3DView::plot<aiw::ImagePIL>;
#endif


