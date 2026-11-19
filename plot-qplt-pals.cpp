#include <sstream>
#include <aiwlib/qplt/imaging>
using namespace aiw;

int main(){
	const char *path = "python3/aiwlib/qplt/pals/";
	for(auto pal: QpltColor::get_pals()){
		std::stringstream str; str<<path<<pal<<".ppm";
		WMSG(pal, str.str());
		QpltColor::plot_pal(pal.c_str(), 1, 1024, true).dump2ppm(str.str().c_str());
	}
}
