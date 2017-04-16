#include "../include/aiwlib/mesh"
using namespace aiw;

const char *help = "usage: arrconv options src.arr dst.arr|dst.dat\noptions:\n"
	"    -h --- show this help and exit\n"
	"    -t ij --- transpose axes i and j (for example -s 01)\n"
	"    -f i --- flip axe i (for example -f 0)\n"
	"    -c min max step --- crop mesh (all sizes in cells, for example -c 0,1 15,10 2,1)\n"
	"    -s pos --- slice mesh (for example -1,2,-1)\n";

struct BaseArr{
	virtual ~BaseArr(){}
	virtual BaseArr* run(int argc, const char **argv, int &pos) = 0;
	virtual void out(const char *path) = 0;
};

#define SLICE(DD) if(DD==D2){ TDArr<T, DD> *res = new TDArr<T, DD>; res->arr = arr.template slice<T, DD>(s, 0); return res; }

template <typename T, int D> struct TDArr: public BaseArr{
	Mesh<T, D> arr;
	Ind<D> atoI(const char *arg){
		Ind<D> I; int a, b = -1; 
		for(int k=0; k<D; k++){
			a = ++b;
			while(arg[b] and arg[b]!=',') b++;
			I[k] = atoi(std::string(arg+a, b-a).c_str());
		}
		return I;
	}
	BaseArr* run(int argc, const char **argv, int &pos){
		std::string opt = argv[pos++];
		if(opt=="-t" and pos<argc){ arr = arr.transpose(argv[pos][0]-48, argv[pos][1]-48); pos++; return this; }
		if(opt=="-f" and pos<argc){ arr = arr.flip(argv[pos++][0]-48); return this; }
		if(opt=="-c" and pos<argc-2){
			Ind<D> a=atoI(argv[pos]), b=atoI(argv[pos+1]), d=atoI(argv[pos+2]);
			arr = arr.crop(a, b, d); pos += 3;
			return this;
		}
		if(opt=="-s" and pos<argc){
			Ind<D> s = atoI(argv[pos++]); int D2 = 0;
			for(int i=0; i<D; i++){ if(s[i]==-1) D2++; }
			SLICE(1); SLICE(2);	SLICE(3); SLICE(4);	SLICE(5); SLICE(6); SLICE(7);
			std::cerr<<"incorrect slice parametr "<<argv[pos-1]<<std::endl;
			abort();
		}
		return nullptr;
	}
	void out(const char *path){
		File fout(path, "wb");		
		if(fout.name.substr(fout.name.size()-4, 4)==".dat") arr.out2dat(fout);
		else arr.dump(fout);
	}
};

#define LOADT(T, D)														\
	try{ TDArr<T, D> *res = new TDArr<T, D>; now = res; res->arr.load(fin); } \
	catch(...){ delete now; now = nullptr; }

#define LOAD(D) LOADT(float, D); LOADT(double, D);

int main(int argc, const char **argv){
	if(argc<3){ std::cout<<help; return 0; }
	for(int i=1; i<argc; i++) if(std::string(argv[i])=="-h"){ std::cout<<help; return 0; }

	File fin(argv[argc-2], "rb");
	BaseArr *old, *now = nullptr;
	LOAD(1); LOAD(2); LOAD(3); LOAD(4); LOAD(5); LOAD(6); LOAD(7); LOAD(8);
	if(!now){ std::cerr<<"uncknow mesh type\n"; return -1; }

	for(int pos=1; pos<argc-2; pos++){
		old = now; now = now->run(argc-2, argv, pos);
		if(now!=old and old) delete old;
	}
	now->out(argv[argc-1]);
	return 0;
}
