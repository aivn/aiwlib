/**
 * Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "../../include/aiwlib/binhead"


int main(int argc, const char **argv){
	if(argc<2){ std::cerr<<"usage: vtk2msh [-mv] vtk files... ==> msh files... \n"; return 1; }
	bool mv_mode = false;
	for(int ipath=1; ipath<argc; ipath++){
		if(std::string(argv[ipath])=="-mv"){ mv_mode = true; continue; }
		std::string src = argv[ipath]; int slash = src.rfind('/'), dot = src.rfind('.');
		std::string dst = dot>slash? src.substr(0, dot)+".msh": src+".msh"; 

		std::ifstream fin(src);  if(!fin.good()){ std::cout<<"can't open "<<src<<" --- skipped\n"; continue; }
		std::ofstream fout(dst); if(!fout.good()){ std::cout<<"can't open "<<dst<<" --- skipped\n"; continue; }
		
		aiw::BinaryHead bh; bh.dim = 3; bh.szT = 4;  bh.type = aiw::BinaryHead::mesh;
		std::vector<float> data; std::string line, word; float step[3];
		bool read_head_mode = true, cell_mode = false; size_t counter = 0, frames = 0;

		do{
			if(read_head_mode){
				fin>>word;
				if(word.size() && word[0]=='#'){ std::getline(fin, line); continue; }
				if(word=="DIMENSIONS") for(int i=0; i<3; i++) fin>>bh.bbox[i];
				if(word=="SPACING") for(int i=0; i<3; i++) fin>>step[i];
				if(word=="ORIGIN") for(int i=0; i<3; i++) fin>>bh.bmin[i];
				if(word=="POINT_DATA"){ size_t sz;  fin>>sz; data.resize(sz, 0.f); }
				if(word=="CELL_DATA"){  cell_mode = true; size_t sz;  fin>>sz; data.resize(sz, 0.f); }
				if(word=="SCALARS") fin>>bh.head;
				if(word=="default"){
					counter = 0; for(int i=0; i<3; i++) bh.bbox[i] -= cell_mode;
					for(int i=0; i<3; i++) if(bh.bbox[i]==1){
							bh.dim = 2;
							for(int j=i+1; j<3; j++){ bh.bbox[j-1] = bh.bbox[j]; bh.bmin[j-1] = bh.bmin[j]; step[j-1] = step[j]; }
							break;
						}
					for(int i=0; i<3; i++) bh.bmax[i] = bh.bmin[i] + step[i]*bh.bbox[i];
					read_head_mode = cell_mode = false;
				}
				continue;
			}
			fin>>data[counter++];
			if(counter==data.size()){
				bh.dump(fout); fout.write((const char*)data.data(), data.size()*4);
				counter = 0; read_head_mode = true; frames++;
			}
		} while(!fin.eof());
		
		if(mv_mode) std::remove(argv[ipath]);
		std::cout<<src<<" ==> "<<dst<<' '<<frames<<" frames\n";		
	}
	return 0;
}
