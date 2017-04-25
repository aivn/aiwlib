#include <fstream>
#include "../../include/aiwlib/dat2mesh"
using namespace aiw;

const char *help = "usage: dat2mesh [-ln [x][y]] [-eps ex,ey] [-def default_value]  xcol,ycol,zcol src.dat|- dst.msh|-\n"
	"    convert dat file (CSV ofrmat) to mesh\n"
	"    - as src/dst - read/write to stdin/stdout\n"
	"    xcol,ycol - columns number selection (from 0)\n"
	"    -ln  --- logscale\n"
	"    -h --- show this help and exit\n";

int main(int argc, const char **argv){
	if(argc<4){ std::cout<<help; return 0; }
	for(int i=1; i<argc; i++) if(std::string(argv[i])=="-h"){ std::cout<<help; return 0; }

	int logscale = 0; Vec<2> eps(1e-6); float def_v=0.;
	for(int i=1; i<argc-3; i++){
		if(std::string(argv[i])=="-ln"){
			std::string opt = argv[++i];
			for(size_t k=0; k<2; ++k) if(opt.find("xy"[k])!=std::string::npos) logscale |= 1<<k;
		} else if(std::string(argv[i])=="-eps") std::stringstream(argv[++i])>>eps;
		else if(std::string(argv[i])=="-def") def_v = atof(argv[++i]);
		else{ std::cout<<help; return 1; }
	}
	Ind<3> dat; std::stringstream(argv[argc-3])>>dat;

	WOUT(dat, logscale, argv[argc-3]);
	
	Mesh<float, 2> dst;
	if(std::string(argv[argc-2])=="-") dst = dat2Mesh(std::cin, def_v, dat[2], dat(0,1), logscale, eps);
	else dst = dat2Mesh(std::ifstream(argv[argc-2]), def_v, dat[2], dat(0,1), logscale&3, eps);
	// WOUT(dst.bbox(), dst.bmin, dst.bmax, dst.step, dst.logscale);
	dst.dump(std::string(argv[argc-1])=="-"?File(::stdout):File(argv[argc-1], "wb"));
	
	return 0;
}
