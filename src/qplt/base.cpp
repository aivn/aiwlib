/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <algorithm>
// #include <pair>
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
void aiw::QpltContainer::prepare3D(QpltScene &S, int Nx, int Ny) const {
	// if(S.theta<0) S.theta += M_PI;
	// if(S.theta>M_PI) S.theta -= M_PI;
						  
	float c_ph, s_ph, c_th, s_th;  sincosf(S.phi*M_PI/180, &s_ph, &c_ph);  sincosf(S.theta*M_PI/180, &s_th, &c_th);
	S.nS = -vecf(c_ph*s_th, s_ph*s_th, c_th);  S.nX = -vecf(-s_ph, c_ph, 0.f);  S.nY = -vecf(-c_th*c_ph, -c_th*s_ph, s_th);
	Vecf<3> d(S.dx, S.dy, S.dz);
		
	Vecf<2> pp[8], A, B;  int i_min; float Zmin;  // координаты вершин и вмещающая оболочка на сцене, номер ближайшей к сцене вершины и ее глубина
	for(int i=0; i<8; i++){  // цикл по углам
		Vecf<3> r = -bbox&d/2;  for(int k=0; k<3; k++) if(i&1<<k) r[k] = -r[k];
		pp[i] = vecf(r*S.nX, r*S.nY);  A <<= pp[i]; B >>= pp[i];
		if(i==0 || r*S.nS>Zmin){ i_min = i; Zmin = r*S.nS; }
	}  // конец цикла по углам
	Vecf<2> scale(1.f);
	if(S.D3scale_mode==0) scale[0] = scale[1] = float(std::min(Nx, Ny))/(bbox&d).abs();
	if(S.D3scale_mode==1) scale[0] = scale[1] = std::min(Nx/(B[0]-A[0]), Ny/(B[1]-A[1]));
	if(S.D3scale_mode==2){ scale[0] = Nx/(B[0]-A[0]); scale[1] = Ny/(B[1]-A[1]); }
    S.nX *= scale[0];  S.nY *= scale[1];  S.flats = 0;
	
	for(int i=0; i<3; i++){  // цикл по осям --- ищем отображаемые грани, достаточно проверить глубину одной вершины
		Vecf<3> r = bbox&d/2; float z1 = S.nS*r;
		r[i] = -r[i]; float z2 = S.nS*r;
		if(z1>z2) S.flats |= 1<<4*i;  // точность ???
		if(z1<z2) S.flats |= 2<<4*i;  // точность ???
	}
		
	std::pair<float, int> stable[6];  // таблица для сортировки вершин
	for(int i=0, j=0; i<8; i++) if(i!=i_min && i!=7-i_min){ Vecf<2> dp = pp[i]-pp[i_min];  stable[j++] = std::make_pair(atan2f(dp[1], dp[0]), i); }
	std::sort(stable, stable+6);

	Ind<3> nb_min;  for(int i=0; i<3; i++) nb_min[i] = i_min&1<<i ? i_min-(1<<i): i_min+(1<<i);   // вершины, соседние центральной	
	bool cshift = !nb_min.contains(stable[0].second);  // необходимость сдвига, что бы с центральной точкой были ребра от вершин 0,2,4	
	Vecf<2> C(Nx/2.+.5, Ny/2.+.5);	S.flpoints[6] = C+(pp[i_min]&scale);  
	for(int i=0; i<6; i++) S.flpoints[(i+cshift)%6] = C+(pp[stable[i].second]&scale); 

	for(int i=0; i<3; i++){  // полуцикл по парам вершин, вторая половина цикла отрабатывает тут же
		int a[2], b[2]; for(int j=0; j<2; j++){ a[j] = stable[(i+3*j)%6].second;  b[j] = stable[(i+3*j+1)%6].second; }
		int ab = abs(a[0]-b[0]),  axe = ab==1? 0: (ab==2? 1: 2);  // ось которой отвечает пара вершин
		bool ver = std::min((pp[a[0]]-pp[i_min]).abs(), (pp[b[0]]-pp[i_min]).abs()) < std::min((pp[a[1]]-pp[i_min]).abs(), (pp[b[1]]-pp[i_min]).abs());
		bool inv = a[ver]>b[ver];
		S.flpoints2axes[2*axe+inv] = (i+3*ver)%6; S.flpoints2axes[2*i+axe-inv] = (i+3*ver+1)%6; 
	}
	for(int i=0; i<7; i++) if(S.flpoints[i]>=ind(0,0)) S.flpoints[i][1] = Ny-1-S.flpoints[i][1];  // разворачиваем ось Y
}
int aiw::QpltScene::get_flpoint_x(int i) const { return flpoints[i%6][0]; }
int aiw::QpltScene::get_flpoint_y(int i) const { return flpoints[i%6][1]; }
int aiw::QpltScene::get_flcenter_x() const { return flpoints[6][0]; }
int aiw::QpltScene::get_flcenter_y() const { return flpoints[6][1]; }
int aiw::QpltScene::get_flpoint_a(int i) const { return flpoints2axes[2*i]; }
int aiw::QpltScene::get_flpoint_b(int i) const { return flpoints2axes[2*i+1]; }
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
