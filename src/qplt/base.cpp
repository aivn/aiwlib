/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <algorithm>
#include "../../include/aiwlib/qplt/base"
#include "../../include/aiwlib/qplt/mesh"
using namespace aiw;

//------------------------------------------------------------------------------
//   class QpltContainer
//------------------------------------------------------------------------------
double aiw::QpltContainer::mem_limit = 4.; // лимит на размер памяти, в GB
std::list<QpltContainer*> aiw::QpltContainer::mem_queue;
const std::string aiw::QpltContainer::default_axes[8] = {"x", "y", "z", "a", "b", "c", "d", "e"};

void aiw::QpltContainer::data_load(){  // освобождает память перед загрузкой данных
	double queue_sz = mem_sz; auto Q = mem_queue.end();
	for(auto I=mem_queue.begin(); I!=mem_queue.end(); ++I){
		if(*I==this){ queue_sz = 0; Q = I; break; }
		queue_sz += (*I)->mem_sz; if(queue_sz>mem_limit) Q = I;
	}
	if(queue_sz>mem_limit) while(Q!=mem_queue.end()) mem_queue.erase(Q++); // нужна память для загрузки данных, чистим хвост списка
	else if(Q!=mem_queue.end()) mem_queue.erase(Q); // данные уже загружены, просто нужно переставить элемент вперед
	else data_load_impl();
	mem_queue.push_front(this);
}
void aiw::QpltContainer::data_free(){  // освобождает память перед загрузкой данных
	printf("free 0\n");
	for(auto I=mem_queue.begin(); I!=mem_queue.end(); ++I) if(*I==this){ printf("free =\n"); mem_queue.erase(I); break; }
	printf("free 2\n");
	data_free_impl();
	printf("free 3\n");
}
//------------------------------------------------------------------------------
void aiw::QpltContainer::prepare_scene(QpltScene &S){ // устанавливает bbox, bmin, bmax
	for(int i=0; i<3; i++) aI[i] = S.get_axe(i);
	flips = S.get_flip(0)|S.get_flip(1)<<1|S.get_flip(2)<<2;
	for(int a=0; a<3; a++){
		int i = aI[a]; if(i>=dim) continue; // break?
		if(S.get_autoscale(a)){	bbeg[a] = 0; bbox[a] = bbox0[i]; bmin[a] = bmin0[i]; bmax[a] = bmax0[i]; S.set_min_max(a, bmin[a], bmax[a], true); }
		else {
			bmin[a] = S.get_min(a); bmax[a] = S.get_max(a);  if(S.get_flip(a)) std::swap(bmin[a], bmax[a]); 
			bool lgs = logscale&1<<i; double step = lgs? exp(log(bmax0[i]/bmin0[i])/bbox0[i]) : (bmax0[i]-bmin0[i])/bbox0[i];
			// WOUT(a, S.get_min(a), S.get_max(a), bmin[a], bmax[a], lgs, step, bbeg[a], bbox[a], bbox0[a]);			
			if(bmin[a]<bmin0[i]){ bbeg[a] = 0; bmin[a] = bmin0[i]; }
			else {
				bbeg[a] = lgs? log(bmin[a]/bmin0[i])/step: (bmin[a]-bmin0[i])/step;
				bmin[a] = lgs ? bmin0[i]*pow(step, bbeg[a]) : bmin0[i] + bbeg[a]*step;
			}
			if(bmax[a]>bmax0[i]){ bbox[a] = bbox0[i]-bbeg[a]; bmax[a] = bmax0[i]; }
			else {
				bbox[a] = (lgs? log(bmax[a]/bmin[a])/step: (bmax[a]-bmin[a])/step)+.5;
				if(bbox[a]<1){ bbox[a] = 1; } if(bbox[a]+bbeg[a]>bbox0[i]) bbox[a] = bbox0[i]-bbeg[a];
				bmax[a] = lgs ? bmin[a]*pow(step, bbox[a]) : bmin[a] + bbox[a]*step;
			}
			// WOUT(a, bmin[a], bmax[a], bbeg[a], bbox[a]);
		}
		// S.set_min(a, bmin[a]); S.set_max(a, bmax[a]);
		// if(S.get_flip(a)) std::swap(bmin[a], bmax[a]);
	}
	for(int i=0; i<dim; i++){
		bool lgs = logscale&1<<i; double step = lgs? exp(log(bmax0[i]/bmin0[i])/bbox0[i]) : (bmax0[i]-bmin0[i])/bbox0[i];
		spos[i] = lgs ? log(S.get_pos(i)/bmin0[i])/step : (S.get_pos(i)-bmin0[i])/step;
		if(spos[i]<0){ spos[i] = 0; } if(spos[i]>=bbox0[i]){ spos[i] = bbox0[i]-1; }
	}
}
//------------------------------------------------------------------------------
//   class QpltFactory
//------------------------------------------------------------------------------
bool aiw::QpltFactory::open_file(const char* fname){ // поддержка GZ файлов?
	last_fin = File(fname, "r"); last_frame = 0;
	if(last_fin){ table.emplace_back(); return true; }
	return false;
}
void aiw::QpltFactory::close_file(){
	last_fin.close();
	if(table.back().empty()) table.pop_back();
}
//------------------------------------------------------------------------------
bool aiw::QpltFactory::load_frame(){
	printf("1234\n");
	QpltMesh *msh = new QpltMesh;  if(msh->load(last_fin)){ msh->frame_ = last_frame++; table.back().push_back(msh); printf("678\n"); return true; }
	else { printf("890\n"); delete msh; }
	
	return false;
}
//------------------------------------------------------------------------------		
bool aiw::QpltFactory::skip_frame(){
	if(!load_frame()) return false;
	free_frame(table.size()-1, table.back().size()-1); last_frame++;
	return true;
}
void aiw::QpltFactory::free_frame(int fileID, int frameID){
	delete table.at(fileID).at(frameID);
	table[fileID].erase(table[fileID].begin()+frameID);
	if(table[fileID].empty()) table.erase(table.begin()+fileID);
}
void aiw::QpltFactory::free_file(int fileID){
	for(QpltContainer* bc: table.at(fileID)) delete bc;
	table.erase(table.begin()+fileID);
}
//------------------------------------------------------------------------------
