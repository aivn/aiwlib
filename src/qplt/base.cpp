/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <algorithm>
#include "../../include/aiwlib/qplt/base"
#include "../../include/aiwlib/qplt/mesh"
#include "../../include/aiwlib/iostream"
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
	if(S.theta<0) S.theta  = 0;
	if(S.theta>180) S.theta = 180;
						  
	float c_ph, s_ph, c_th, s_th;  sincosf(S.phi*M_PI/180, &s_ph, &c_ph);  sincosf(S.theta*M_PI/180, &s_th, &c_th);
	S.nS = vecf(c_ph*s_th, s_ph*s_th, c_th);  S.nX = -vecf(-s_ph, c_ph, 0.f);  S.nY = vecf(-c_th*c_ph, -c_th*s_ph, s_th);  // вектор ИЗ начала координат В экран
	Vecf<3> d(S.dx, S.dy, S.dz);
		
	Vecf<2> pp[8], A, B;  int i_min; float Zmin;  // координаты вершин и вмещающая оболочка на сцене, номер ближайшей к сцене вершины и ее глубина
	for(int i=0; i<8; i++){  // цикл по углам
		Vecf<3> r = -bbox&d/2;  for(int k=0; k<3; k++) if(i&1<<k) r[k] = -r[k];
		pp[i] = vecf(r*S.nX, r*S.nY);  A <<= pp[i]; B >>= pp[i];
		if(i==0 || r*S.nS>Zmin){ i_min = i; Zmin = r*S.nS; }
		// WOUT(i, r, pp[i], i_min, Zmin, r*S.nS);
	}  // конец цикла по углам
	Vecf<2> scale(1.f);
	if(S.D3scale_mode==0) scale[0] = scale[1] = float(std::min(Nx, Ny))/(bbox&d).abs();
	if(S.D3scale_mode==1) scale[0] = scale[1] = std::min(Nx/(B[0]-A[0]), Ny/(B[1]-A[1]));
	if(S.D3scale_mode==2){ scale[0] = Nx/(B[0]-A[0]); scale[1] = Ny/(B[1]-A[1]); }
    S.nX *= scale[0];  S.nY *= scale[1];  S.flats = 0;
	
	for(int i=0; i<3; i++){  // цикл по осям --- ищем отображаемые грани, достаточно проверить глубину одной вершины
		Vecf<3> r = bbox&d/2; float z_plus = S.nS*r;
		r[i] = -r[i]; float z_minus = S.nS*r;
		if(z_plus>z_minus) S.flats |= 2<<4*i;  // точность ???
		if(z_plus<z_minus) S.flats |= 1<<4*i;  // точность ???
	}

	std::pair<float, int> stable[6]; int rtable[6];  // таблица для сортировки вершин и обратная таблица переходов
	for(int i=0, j=0; i<8; i++) if(i!=i_min && i!=7-i_min){ Vecf<2> dp = pp[i]-pp[i_min]; stable[j++] = std::make_pair(atan2f(dp[1], dp[0]), i); }
	std::sort(stable, stable+6);

	Ind<3> nb_min;  for(int i=0; i<3; i++) nb_min[i] = i_min&1<<i ? i_min-(1<<i): i_min+(1<<i);  // вершины соседние центральной	
	bool cshift = !nb_min.contains(stable[0].second);  // необходимость сдвига, что бы с центральной точкой были соединены ребра от вершин 0,2,4	
	Vecf<2> C(Nx/2.+.5, Ny/2.+.5);	S.flcenter = C+(pp[i_min]&scale);  // центр изображения и центральная точка  
	for(int i=0; i<6; i++){ rtable[i] = stable[(i+cshift)%6].second; S.flpoints[i] = C+(pp[rtable[i]]&scale);  }
	S.im_a = C+(A&scale);  S.im_b = C+(B&scale);

	for(int i=0; i<3; i++){  // полуцикл по парам вершин, вторая половина цикла отрабатывает тут же
		int a[2], b[2]; for(int j=0; j<2; j++){ a[j] = rtable[(i+3*j)%6];  b[j] = rtable[(i+3*j+1)%6]; }
		int ab = abs(a[0]-b[0]),  axe = ab==1? 0: (ab==2? 1: 2);  // ось которой отвечает пара вершин
		bool ver = std::min((pp[a[0]]-pp[i_min]).abs(), (pp[b[0]]-pp[i_min]).abs()) < std::min((pp[a[1]]-pp[i_min]).abs(), (pp[b[1]]-pp[i_min]).abs());
		bool inv = a[ver]>b[ver];
		// WOUT(i, a[0], a[1], b[0], b[1], ab, axe, ver, inv, 2*axe+inv, 2*axe+1-inv);
		S.flaxes[2*axe+inv] = (i+3*ver)%6;  S.flaxes[2*axe+1-inv] = (i+3*ver+1)%6;  // mode==auto
		ver = (pp[a[0]]+pp[b[0]])[axe<2] > (pp[a[1]]+pp[b[1]])[axe<2];  inv = a[ver]>b[ver];  // интерпретация индексов a, b (лево/право или верх/низ)
		S.flaxes[6+2*axe+inv] = (i+3*ver)%6;  S.flaxes[6+2*axe+1-inv] = (i+3*ver+1)%6;  // mode==1 left/bottom
		ver = !ver;  inv = a[ver]>b[ver];  // вторая пара осей
		S.flaxes[12+2*axe+inv] = (i+3*ver)%6;  S.flaxes[12+2*axe+1-inv] = (i+3*ver+1)%6;  // mode==2 right/top
	}
	//--------------------------------------------------------------------------
	// File fouts[3]; for(int i=0; i<3; i++) fouts[i] = File("fl%.dat", "w", i);

	for(int fl=0; fl<3; fl++) if(S.flats&(0xF<<fl*4)){  // цикл по флэтам
			// Ind<2> flchk;
			int a[2] = {(fl+1)%3, (fl+2)%3}; Vecf<2> nn[2], cf[2];  // оси флэта 
			for(int i=0; i<2; i++){   // цикл по осям флэта, нужно спроецировать точку изображения p на оси a[0,1] получая в итоге число в диапазоне 0...bbox[a[i]]
				int i0 = i_min, i1 = i_min;  // индексты точек в начале и конце оси a[i]
				// if(pm==2){ i0 &= ~(1<<a[i]); i1 |= 1<<a[i]; } else { i1 &= ~(1<<a[i]); i0 |= 1<<a[i]; }
				if(i_min&(1<<a[i])) i0 &= ~(1<<a[i]); else i1 |= 1<<a[i];
				
				// Vecf<2> n = (pp[i1]-pp[i0])&scale;  S.nflats[2*fl+i] = n/(n*n)*bbox[a[i]];
				// S.cflats[2*fl+i] = C+(pp[i0]&scale); // (C+(pp[i0]&scale))*-bbox[a[i]];
				cf[i] = C-ind(S.im_a.x, S.im_a.y)+(pp[i0]&scale);
				nn[i] = (pp[i1]-pp[i0])&scale;
				// WOUT(fl, i, pm, i_min, i0, i1);
				
				//WOUT(fl, i, n, S.nflats[2*fl+i], S.cflats[2*fl+i]);

				// flchk += C-ind(S.im_a.x, S.im_a.y)+(pp[i0==i_min? i1: i0 ]&scale); // )*(1+i);
				
				// fouts[fl]("%\n%\n\n\n", C-ind(S.im_a.x, S.im_a.y)+(pp[i0]&scale), C-ind(S.im_a.x, S.im_a.y)+(pp[i1]&scale));
			}
			for(int i=0; i<2; i++){
				Vecf<2> n_perp(nn[1-i][1], -nn[1-i][0]); n_perp /= n_perp.abs();
				if(n_perp*nn[i]<0) n_perp = -n_perp;
				S.nflats[2*fl+i] = n_perp/(n_perp*nn[i])*bbox[a[i]];
				S.cflats[2*fl+i] = -cf[i]*S.nflats[2*fl+i];
			}
			/*
			for(int x=0; x<S.im_b.x-S.im_a.x; x+=10)
				for(int y=0; y<S.im_b.y-S.im_a.y; y+=10){
					Vecf<2> r(S.nflats[2*fl]*ind(x, y)+S.cflats[2*fl], S.nflats[2*fl+1]*ind(x, y)+S.cflats[2*fl+1]);
					if(0<r[0] && 0<r[1] && r[0]<bbox[a[0]] && r[1]<bbox[a[1]]) fouts[fl].printf("%i %i\n", x, y);
				}
			*/		

			// for(int i=0; i<8; i++) if((pm==1 && !(i&1<<fl)) || (pm==2 && (i&1<<fl))){ flchk += C+(pp[i]&scale); WOUT(fl, i); } flchk /= 4;
			// flchk /= 2;
			// fouts[fl]("%\n", flchk);
			// WOUT(flchk, C, Nx, Ny, S.nflats[2*fl]*(flchk - S.cflats[2*fl]), S.nflats[2*fl+1]*(flchk - S.cflats[2*fl+1]));
			// WOUT(Nx, Ny, S.nflats[2*fl]*flchk+S.cflats[2*fl], S.nflats[2*fl+1]*flchk+S.cflats[2*fl+1]);
			// WOUT(flchk, S.nflats[2*fl], S.nflats[2*fl+1], S.cflats[2*fl], S.cflats[2*fl+1]);
			// WOUT(S.nflats[2*fl]*flchk, S.nflats[2*fl]*S.cflats[2*fl], S.nflats[2*fl+1]*flchk,  S.nflats[2*fl+1]*S.cflats[2*fl+1]);
		}

	/*
	File fout("flats.dat", "w");
	for(int i=0; i<7; i++) fout.printf("%i %i\n", S.flpoints[i%6].x, S.flpoints[i%6].y);
	fout.printf("\n\n");
	for(int i=0; i<3; i++) fout.printf("%i %i\n%i %i\n\n\n", S.flcenter.x, S.flcenter.y, S.flpoints[2*i].x, S.flpoints[2*i].y);
	*/ 
	//--------------------------------------------------------------------------
	if(S.flcenter.y>=0) S.flcenter.y = Ny-1-S.flcenter.y;  // разворачиваем ось Y
	for(int i=0; i<6; i++) if(S.flpoints[i].y>=0) S.flpoints[i].y = Ny-1-S.flpoints[i].y;  // разворачиваем ось Y
	// S.im_a.y = Ny - S.im_a.y; S.im_b.y = Ny - S.im_b.y;
}
int aiw::Ind2::x0 = 0;
int aiw::Ind2::y0 = 0;
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
