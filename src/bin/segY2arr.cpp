/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/segy"
using namespace aiw;

const char *help = "usage: segY2arr options and input segYfiles...\noptions:\n"
	"    -h --- show this help and exit\n"
	"    -2d|-3d --- set arrays dimention (default 2)\n"
	"    -PV --- set coord by PV (default PP)\n"
	// "    -s dx,[dy,]dz --- steps (default from arrayfiles)\n"
	"    -x coeff --- scale signal (mule data on coeff)\n"
	"    -Z pow --- mule data on z^pow (default pow=0)\n"
	"    -ibm --- use IBM format for segY (default not using)\n"
	"    -p dstpath --- destination DIRECTORY, default convertation file[.arr] ===> file.sgy\n";


int main(int argc, const char ** argv){
	int dim = 2;
	bool use_step = false, PV = false, use_scale = false;
	float z_pow = 0., scale = 1.;
	std::string dstpath;
	if(argc==1){ std::cout<<help; return 0; }
	for(int i=1; i<argc; i++){
		std::string opt = argv[i];
		if(opt=="-h"){ std::cout<<help; return 0; }
		if(opt=="-2d"){ dim = 2; continue; }
		if(opt=="-3d"){ dim = 3; continue; }
		if(opt=="-PV"){ PV = true; continue; }
		// if(opt=="-s" and i<argc-1){ step = atoV(argv[++i]); use_step = true; continue; }
		if(opt=="-Z" and i<argc-1){ z_pow = atof(argv[++i]); continue; }
		if(opt=="-x" and i<argc-1){ scale = atof(argv[++i]); use_scale = true; continue; }
		if(opt=="-ibm"){ segy_ibm_format = true; continue; }
		if(opt=="-p" and i<argc-1){ dstpath = argv[++i]; continue; }

		std::string src = argv[i]; int slash = src.rfind('/'), dot = src.rfind('.');
		std::string dst = dot>slash? src.substr(0, dot)+".msh": src+".msh"; 

		/*
		std::list<std::vector<float> > data;
		std::vector<Vecf<8> > heads;
		WMSG(argv[i]);
		
		int sz = segy_raw_read(File(argv[i], "r"), data, heads, -1, false);
		std::cout<<sz<<' '<<data.size()<<'\n';
		*/
		Mesh<float, 2> data; segy_read(File(argv[i], "r"), data, true);
		if(use_scale) for(float &x: data) x *= scale;
		data.dump(File(dst.c_str(), "w")); std::cout<<src<<" ==> "<<dst<<'\n';
		
		/* 
		Mesh<float, 2> arr2D; Mesh<float, 3> arr3D;
		if(arr2D.load(File(src.c_str(), "rb"), !use_scale, false)) dim = 2; 
		else if(arr3D.load(File(src.c_str(), "rb"), !use_scale, false)) dim = 3; 
		else { printf("'%s' incorrect format\n", src.c_str()); return 1; } 
		
		if(dim==2){
			mk_swaps(arr2D, swaps, sw_count);
			if(use_step) for(int i=0; i<dim; i++) arr2D.step[i] = step[i]; // else arr2D.step[0] *= -1e3; // <=== to mks ???
			WOUT(arr2D.bbox(), arr2D.bmin, arr2D.bmax, arr2D.step);
			if(use_scale) for(Ind<2> pos; pos^=arr2D.bbox(); ++pos) arr2D[pos] *= scale;
			segy_write(File(dst.c_str(), "wb"), arr2D, z_pow, PV(0,1), (use_PP0?PP0:arr2D.bmin|0.));
		} else {
			mk_swaps(arr3D, swaps, sw_count);
			if(use_step) for(int i=0; i<dim; i++) arr3D.step[i] = step[i]; // else arr3D.step[0] *= -1e3; // <=== to mks ???
			WOUT(arr3D.bbox(), arr3D.bmin, arr3D.bmax, arr3D.step);
			if(use_scale) for(Ind<3> pos; pos^=arr3D.bbox(); ++pos) arr3D[pos] *= scale;
			segy_write(File(dst.c_str(), "wb"), arr3D, z_pow, PV(0,1), (use_PP0?PP0:arr3D.bmin));			
		}
		std::cout<<dim<<"D: "<<src<<" ===> "<<dst<<" [OK]\n";
		*/
	}
}
