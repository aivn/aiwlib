#include "../../include/aiwlib/sphere"
using namespace aiw;

int main(int argc, const char**argv){
	if(argc!=2){ printf("usage: ./sph2dat file.sph ==> #:ID nx ny nz dS f\n"); return 1; }
	Sphere<float> sph; if(!sph.load(File(argv[1], "r"))){ printf("incorrect input file\n"); return 1; }
	sph_init_table(sph.rank());
	printf("# rank=%i, mode=%i, size=%i\n", sph.rank(), sph.get_mode(), int(sph.size()));
	printf("#:ID nx ny nz dS f\n");
	for(int i=0; i<int(sph.size()); i++) printf("%i %g %g %g %g %g\n", i, sph.center(i)[0], sph.center(i)[1], sph.center(i)[2], sph.area(i), sph[i]);
	return 0;
}
