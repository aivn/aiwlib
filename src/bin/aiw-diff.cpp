#include "../../include/aiwlib/mesh"
using namespace aiw;

int main(int argc, const char **argv){
	if(argc!=4){ printf("usage: aiw-diff a.msh b.msh c.msh ==> c.msh = a.msh-b.msh\n"); return 0; }
	Mesh<float, 3> a, b, c;
	a.load(File(argv[1], "r"));
	b.load(File(argv[2], "r"));
	if(a.bbox()!=b.bbox()){ WRAISE("oops... ", a.bbox(), b.bbox()); }
	c.init(a.bbox(), a.bmin, a.bmax, a.logscale);
	for(Ind<3> pos; pos^=a.bbox(); ++pos) c[pos] = a[pos] - b[pos];
	c.dump(File(argv[3], "w"));	
	return 0;
}
