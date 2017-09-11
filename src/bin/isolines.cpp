/**
 * Copyright (C) 2016 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <fstream>
#include "../../include/aiwlib/dat2mesh"
#include "../../include/aiwlib/isolines"
using namespace aiw;

const char *help = "usage: isolines [-ln [x][y][z]] [-dat xcol,ycol,zcol] z0 dz src.msh|src.dat|- (read from stdin)\n"
	"    print isolines to stdout\n"
	"    -dat ... for .dat files, columns number selection (from 0)\n"
	"    -ln xy work for dat format only\n"
	"    -h --- show this help and exit\n";

int main(int argc, const char **argv){
	if(argc<4){ std::cout<<help; return 0; }
	for(int i=1; i<argc; i++) if(std::string(argv[i])=="-h"){ std::cout<<help; return 0; }

	int logscale = 0; Ind<3> dat(-1);
	for(int i=1; i<argc-3; i++){
		if(std::string(argv[i])=="-ln"){
			std::string opt = argv[++i];
			for(size_t k=0; k<3; ++k) if(opt.find("xyz"[k])!=std::string::npos) logscale |= 1<<k;
		} else if(std::string(argv[i])=="-dat") std::stringstream(argv[++i])>>dat;
		else{ std::cout<<help; return 1; }
	}
								  
	double z0 = atof(argv[argc-3]), dz = atof(argv[argc-2]);

	Mesh<float, 2> src;
	//std::ifstream fsrc(argv[argc-1]);
	if(dat==ind(-1)) src.load(std::string(argv[argc-1])=="-"?File(::stdin):File(argv[argc-1], "rb"));
	else if(std::string(argv[argc-1])=="-") src = dat2Mesh(std::cin, 0.f, dat[2], dat(0,1), logscale&3);
	else src = dat2Mesh(std::ifstream(argv[argc-1]), 0.f, dat[2], dat(0,1), logscale&3);
	// if(dat==ind(-1)) src.load(fsrc);
	// else
	// src = dat2Mesh(fsrc, 0.f, dat[2], dat(0,1), logscale&3);

	IsoLines iso; iso.init(src, z0, dz, logscale&4);
	iso.out2dat(std::cout);

	// WOUT(src.bmin, src.bmax, src.step, src.bbox(), src.logscale, logscale&4);
	
	return 0;
}
