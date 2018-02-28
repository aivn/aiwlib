%module plot2D
%exception { try{ $action }catch( const char *e ){ PyErr_SetString( PyExc_RuntimeError, e ); return NULL; }catch(...){ return NULL; } }
%feature("autodoc", "1");
%include "std_string.i"
%{
#include "../include/aiwlib/plot2D"
%}
%include "../include/aiwlib/vec"
%include "../include/aiwlib/mesh"
%include "../include/aiwlib/sphere"
%include "../include/aiwlib/plot2D"

%template(plot2D) aiw::plot2D<float, aiw::ImagePIL>;
%template(plot2D) aiw::plot2D<float, aiw::ImagePNG>;
%template(plot2D) aiw::plot2D<double, aiw::ImagePIL>;
%template(plot2D) aiw::plot2D<double, aiw::ImagePNG>;
%template(plot2D) aiw::plot2D<uint16_t, aiw::ImagePIL>;
%template(plot2D) aiw::plot2D<uint16_t, aiw::ImagePNG>;
%template(plot_paletter) aiw::plot_paletter<aiw::ImagePIL>;
%template(plot_paletter) aiw::plot_paletter<aiw::ImagePNG>;


