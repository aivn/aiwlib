/**
 * Copyright (C) 2016 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/segy"
using namespace aiw;

const char *help = "usage: arr2seg-Y options and input arrfiles...\noptions:\n"
	"    -h --- show this help and exit\n"
	//	"    -2d|-3d --- set arrays dimention (default 2)\n"
	"    -s dx,[dy,]dz --- steps (default from arrayfiles)\n"
	"    -PV x,y --- PV coordinates (default 0,0)\n"
	"    -PP0 x,[y,]z --- first PP coordinates (default from arrayfile)\n"
	"    -t ij --- transpose axes i and j (for example -s 01), AS A RESULT OF THE TIME AXIS MUST BE ALONG THE X-AXIS!\n"
	"    -f i --- flip axe i (for example -f 0)\n"
	"    -Z pow --- mule data on z^pow (default pow=0)\n"
	"    -ibm --- use IBM format for SEG-Y (default not using)\n"
	"    -p dstpath --- destination DIRECTORY, default convertation file[.arr] ===> file.sgy\n";

Vec<3> atoV(const char *s){
	Vec<3> V; int a, b = -1; 
	for(int k=0; k<3; k++){
		a = ++b;
		while(s[b] and s[b]!=',') b++;
		V[k] = atof(std::string(s+a, b-a).c_str());
	}
	return V;
}

template <int D> void mk_swaps(Mesh<float, D> &arr, Ind<2> *swaps, int sw_count){
	for(int sw=0; sw<sw_count; sw++){
		if(swaps[sw][0]==-1) arr = arr.flip(swaps[sw][0]);
		else arr = arr.transpose(swaps[sw][0], swaps[sw][1]);
	}
}

int main(int argc, const char ** argv){
	int dim = 2;
	Vec<3> step, PV, PP0; bool use_step = false, use_PP0 = false;
	Ind<2> swaps[1024]; int sw_count = 0;
	double z_pow = 0.;
	std::string dstpath;
	if(argc==1){ std::cout<<help; return 0; }
	for(int i=1; i<argc; i++){
		std::string opt = argv[i];
		if(opt=="-h"){ std::cout<<help; return 0; }
		// if(opt=="-2d"){ dim = 2; continue; }
		// if(opt=="-3d"){ dim = 3; continue; }
		if(opt=="-s" and i<argc-1){ step = atoV(argv[++i]); use_step = true; continue; }
		if(opt=="-PV" and i<argc-1){ PV = atoV(argv[++i]); continue; }
		if(opt=="-PP0" and i<argc-1){ PP0 = atoV(argv[++i]); use_PP0 = true; continue; }
		if(opt=="-t" and i<argc-1){ i++; swaps[sw_count++] = ind(argv[i][0]-48, argv[i][1]-48); continue; }
		if(opt=="-f" and i<argc-1){ i++; swaps[sw_count++] = ind(-1, argv[i][0]-48); continue; }
		if(opt=="-Z" and i<argc-1){ z_pow = atof(argv[++i]); continue; }
		if(opt=="-ibm"){ segy_ibm_format = true; continue; }
		if(opt=="-p" and i<argc-1){ dstpath = argv[++i]; continue; }

		std::string src = argv[i]; int slash = src.rfind('/'), dot = src.rfind('.');
		std::string dst = dot>slash? src.substr(0, dot)+".sgy": src+".sgy"; 

		Mesh<float, 2> arr2D; Mesh<float, 3> arr3D;
		if(arr2D.load(File(src.c_str(), "rb"), 1, false)) dim = 2; 
		else if(arr3D.load(File(src.c_str(), "rb"), 1, false)) dim = 3; 
		else { printf("'%s' incorrect format\n", src.c_str()); return 1; } 

		if(dim==2){
			mk_swaps(arr2D, swaps, sw_count);
			if(use_step) for(int i=0; i<dim; i++) arr2D.step[i] = step[i]; else arr2D.step[0] *= 1e-3; // <=== to mks ???
			WOUT(arr2D.bbox(), arr2D.bmin, arr2D.bmax, arr2D.step);
			segy_write(File(dst.c_str(), "wb"), arr2D, z_pow, PV(0,1), (use_PP0?PP0:arr2D.bmin|0.));
		} else {
			mk_swaps(arr3D, swaps, sw_count);
			if(use_step) for(int i=0; i<dim; i++) arr3D.step[i] = step[i]; else arr3D.step[0] *= 1e-3; // <=== to mks ???
			WOUT(arr3D.bbox(), arr3D.bmin, arr3D.bmax, arr3D.step);
			segy_write(File(dst.c_str(), "wb"), arr3D, z_pow, PV(0,1), (use_PP0?PP0:arr3D.bmin));			
		}
		std::cout<<src<<" ===> "<<dst<<" [OK]\n";
	}
}
