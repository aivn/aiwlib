/**
 * Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include "../../include/aiwlib/binhead"


int main(int argc, const char **argv){
	if(argc<2){ std::cerr<<"usage: vtk2msh [-mv] [-dDx,Dy,Dz] [-swap] vtk files... ==> msh files... \n"; return 1; }
	bool mv_mode = false, swap_mode = false;  float step[3] = {1.f, 1.f, 1.f};
	for(int ipath=1; ipath<argc; ipath++){
		if(std::string(argv[ipath])=="-mv"){ mv_mode = true; continue; }
		if(std::string(argv[ipath])=="-swap"){ swap_mode = true; continue; }
		if(std::string(argv[ipath]).substr(0, 2)=="-d"){
			std::stringstream buf(argv[ipath]+3); char c; for(int i=0; i<3; i++) buf>>step[i]>>c;
			std::cout<<"set step "<<step[0]<<','<<step[1]<<','<<step[2]<<'\n';
			continue;
		}
		
		std::string src = argv[ipath]; int slash = src.rfind('/'), dot = src.rfind('.');
		std::string dst = dot>slash? src.substr(0, dot)+".msh": src+".msh"; 

		std::ifstream fin(src);  if(!fin.good()){ std::cout<<"can't open "<<src<<" --- skipped\n"; continue; }
		std::ofstream fout(dst); if(!fout.good()){ std::cout<<"can't open "<<dst<<" --- skipped\n"; continue; }
		
		aiw::BinaryHead bh; bh.dim = 3; bh.szT = 4;  bh.type = aiw::BinaryHead::mesh;
		std::vector<float> data; std::string line, word; 
		bool read_head_mode = true, cell_mode = false; size_t counter = 0, frames = 0, lines = 0, bad_counter = 0;

		std::cout<<src<<" ==> "<<dst<<" ... \n";
		do{
			if(read_head_mode){
				fin>>word;  lines++;
				if(word.size() && word[0]=='#'){ std::getline(fin, line); continue; }
				if(word=="DIMENSIONS"){
					for(int i=0; i<3; i++) fin>>bh.bbox[i];
					// if(swap_mode) std::swap(bh.bbox[0], bh.bbox[1]);
					std::cout<<"  frame "<<frames<<": size "<<bh.bbox[0]<<'*'<<bh.bbox[1]<<'*'<<bh.bbox[2]<<'='<<bh.bbox[0]*bh.bbox[1]*bh.bbox[2]<<std::endl;
				}
				if(word=="SPACING"){
					for(int i=0; i<3; i++) fin>>step[i];
					// if(swap_mode) std::swap(step[0], step[1]);
					std::cout<<"  frame "<<frames<<": step "<<step[0]<<' '<<step[1]<<' '<<step[2]<<std::endl;
				}
				if(word=="ORIGIN"){
					for(int i=0; i<3; i++) fin>>bh.bmin[i];
					// if(swap_mode) std::swap(bh.bmin[0], bh.bmin[1]);
					std::cout<<"  frame "<<frames<<": origin "<<bh.bmin[0]<<' '<<bh.bmin[1]<<' '<<bh.bmin[2]<<std::endl;
				}
				if(word=="POINT_DATA"){ size_t sz;  fin>>sz; data.resize(sz, 0.f); std::cout<<"  frame "<<frames<<": points "<<sz<<std::endl; }
				if(word=="CELL_DATA"){  cell_mode = true; size_t sz;  fin>>sz; data.resize(sz, 0.f);  std::cout<<"  frame "<<frames<<": cells "<<sz<<std::endl;  }
				if(word=="SCALARS") fin>>bh.head;
				if(word=="default"){
					counter = 0; for(int i=0; i<3; i++) bh.bbox[i] -= cell_mode;
					for(int i=0; i<3; i++) if(bh.bbox[i]==1){
							bh.dim = 2;
							for(int j=i+1; j<3; j++){ bh.bbox[j-1] = bh.bbox[j]; bh.bmin[j-1] = bh.bmin[j]; step[j-1] = step[j]; }
							break;
						}
					for(int i=0; i<3; i++) bh.bmax[i] = bh.bmin[i] + step[i]*bh.bbox[i];
					read_head_mode = cell_mode = false;  std::cout<<"  frame "<<frames<<": end of header, "<<lines<<" lines"<<std::endl;
				}
				continue;
			}
			/*
			fin>>data[counter++];  lines++;
			if(!fin.good()){
				fin.unget(); fin.clear(); std::string x; fin>>x;
				if(x=="nan") data[counter-1] = std::numeric_limits<float>::quiet_NaN();
				else if(x=="-nan") data[counter-1] = -std::numeric_limits<float>::quiet_NaN();
				else if(x=="inf") data[counter-1] = std::numeric_limits<float>::infinity();
				else if(x=="-inf") data[counter-1] = -std::numeric_limits<float>::infinity();
				bad_counter++;
			}
			*/
			fin>>word;    lines++;
			if(swap_mode){ int x = counter/bh.bbox[1], y = counter%bh.bbox[1];  data[x+y*bh.bbox[0]] = atof(word.c_str()); counter++; }
			else data[counter++] = atof(word.c_str());
			if(word=="nan" || word=="-nan" || word=="inf" || word=="-inf") bad_counter++;
			
			if(counter==data.size()){
				bh.dump(fout); fout.write((const char*)data.data(), data.size()*4);
				std::cout<<"  frame "<<frames<<": nan+inf "<<bad_counter<<std::endl;
				counter = 0; read_head_mode = true; bad_counter = 0; frames++;				
			}
		} while(!fin.eof());
		std::cout<<"  end of file, "<<lines<<" lines"<<std::endl;

		if(counter){
			bh.dump(fout); fout.write((const char*)data.data(), data.size()*4);
			std::cout<<"  frame "<<frames<<": nan+inf "<<bad_counter<<" dump brocken file: points "<<counter<<", "<<data.size()<<" expected"<<std::endl;
			counter = 0; read_head_mode = true; bad_counter = 0; frames++;				
		}
		
		if(mv_mode) std::remove(argv[ipath]);
		// std::cout<<src<<" ==> "<<dst<<' '<<frames<<" frames\n";		
	}
	return 0;
}
