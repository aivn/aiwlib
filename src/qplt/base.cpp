/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <algorithm>
//#include <cmath>
#include "../../include/aiwlib/debug_ext"
#include "../../include/aiwlib/qplt/base"
#include "../../include/aiwlib/qplt/mesh"
#include "../../include/aiwlib/qplt/balls"
#include "../../include/aiwlib/iostream"
#include "../../include/aiwlib/gauss"
using namespace aiw;

# ifdef min
# undef min
# endif

# ifdef max
# undef max
# endif


//------------------------------------------------------------------------------
void aiw::qplt_global_init(){
	init_segfault_hook();	
}
//------------------------------------------------------------------------------
//   class QpltContainer
//------------------------------------------------------------------------------
double aiw::QpltContainer::mem_limit = 4.; // лимит на размер памяти, в GB
std::list<QpltContainer*> aiw::QpltContainer::mem_queue;
const std::string aiw::QpltContainer::default_anames[6] = {"x", "y", "z", "u", "v", "w"};

void aiw::QpltContainer::data_load(){  // освобождает память перед загрузкой данных, данные лежат в очереди в порядке последнего обращения
	// WERR(mem_limit, mem_queue.size(), mem_sz); 
	double queue_sz = mem_sz; auto Q = mem_queue.end();  // общий размер данных в очереди и итератор на this (если есть) 
	for(auto I=mem_queue.begin(); I!=mem_queue.end();){  // считаем размер памяти занятый очередью и при необходимости чистим ее хвост 
		if(*I==this){  Q = I; break; }  // данные уже загружены, все ОК		
		if(queue_sz+(*I)->mem_sz>mem_limit){ (*I)->data_free_impl(); mem_queue.erase(I++); }
		else{ queue_sz += (*I)->mem_sz; I++; }
	}
	// WERR(*Q, *mem_queue.end(), queue_sz, mem_limit);
	if(Q!=mem_queue.end()) mem_queue.erase(Q); // данные уже загружены, просто нужно переставить элемент вперед
	else data_load_impl();
	mem_queue.push_front(this);
	// WERR("OK");
}
void aiw::QpltContainer::data_free(){  // освобождает память перед загрузкой данных
	WERR("begin");
	for(auto I=mem_queue.begin(); I!=mem_queue.end(); ++I) if(*I==this){ WERR("free"); mem_queue.erase(I); break; }
	WERR("middle");
	data_free_impl();
	WERR("OK");
}
//------------------------------------------------------------------------------
const char* aiw::QpltContainer::fname() const { return fin->name.c_str(); }
void aiw::QpltContainer::calc_step(){
	for(int axe=0; axe<dim; axe++)
		if(logscale&1<<axe){ step[axe] = exp(log(bmax[axe]/bmin[axe])/bbox[axe]); rstep[axe] = 1./log(step[axe]); }
		else{ step[axe] = (bmax[axe]-bmin[axe])/bbox[axe]; rstep[axe] = 1.f/step[axe]; }
} 
float aiw::QpltContainer::fpos2coord(float fpos, int axe) const { return logscale&1<<axe ? bmin[axe]*pow(step[axe], fpos) : bmin[axe]+step[axe]*fpos; }
float aiw::QpltContainer::pos2coord(int pos, int axe) const { return logscale&1<<axe ? bmin[axe]*pow(step[axe], pos+.5) : bmin[axe]+step[axe]*(pos+.5); }
int aiw::QpltContainer::coord2pos(float coord, int axe) const { return floorf(logscale&1<<axe ? log(coord/bmin[axe])*rstep[axe] : (coord-bmin[axe])*rstep[axe]); }
//------------------------------------------------------------------------------
aiw::QpltPlotter* aiw::QpltContainer::plotter( int mode,
											   // f_opt --- 2 бита autoscale, 1 бит logscale, 1 бит модуль
											   int f_opt, float f_lim[2], const char* paletter, int arr_lw[2], float arr_spacing, int nan_color,
											   int ctype, int Din, int mask, int offsets[3], int diff, int vconv, bool minus,  // accessor
											   // описание сцены в исходных осях - ползунки, пределы, интерполяция и пр., сохраняет настройки при переключениях. Пока так
											   int axisID[3], float sposf[6], float bmin_[6], float bmax_[6], int fai, // 6 бит флипы, по 12 бит autoscale и  интерполяция
											   float th_phi[2], float cell_aspect[3], int D3deep
											   ){ // const {
	QpltPlotter* plt = mk_plotter(mode); plt->container = this; plt->mode = mode;
	// 1. настраиваем базовые вещи в плоттере
	plt->accessor.ctype = ctype; plt->accessor.Din = Din; plt->accessor.mask = mask; plt->accessor.diff = diff; plt->accessor.vconv = vconv;
	plt->accessor.minus = minus; for(int i=0; i<3; i++){ plt->accessor.offsets[i] = offsets[i];  plt->accessor.rsteps[i] = rstep[axisID[i]]; }
	plt->color.arr_length = arr_lw[0]; plt->color.arr_width = arr_lw[1]; plt->color.arr_spacing = arr_spacing;
	plt->color.init(paletter, f_lim[0], f_lim[1], f_opt&4); plt->color.modulus = f_opt&8; plt->color.nan_color = nan_color;

	for(int i=0; i<dim; i++){
		int pos = fai&(1<<(6+2*i))? 0 : coord2pos(sposf[i], i);
		// WERR(i, (fai&(1<<(6+2*i))), sposf[i], pos, coord2pos(sposf[i], i));
		if(pos<0){ pos = 0; } if(pos>=bbox[i]){ pos = bbox[i]-1; }
		plt->spos[i] = pos;
	}

	plt->axisID = ptr2vec<3>(axisID); plt->theta = th_phi[0]; plt->phi = th_phi[1]; plt->dim = 2+bool(mode);
	plt->cell_aspect = ptr2vec<3>(cell_aspect); plt->flips = plt->interp = plt->logscale = 0;
	plt->D3density = D3deep&0x7F; plt->D3opacity = (D3deep>>7)&0x7F;  plt->D3mingrad = (D3deep>>14)&0x7F; plt->D3tmask = D3deep>>21; // 11 осталось
	
	for(int i=0; i<2+bool(mode); i++){  // цикл по осям, настраиваем пределы отрисовки плоттера
		int a = axisID[i]; if(a>=dim) WRAISE("incorret axe", mode, dim, i, axisID[i]);
		if(fai&(1<<a)) plt->flips |= 1<<i;
		plt->interp |= (fai>>(18+2*a)&3)<<2*i; 
		bool lgs = logscale&(1<<a); plt->logscale |= int(lgs)<<i;
		if(fai&(1<<(7+2*a))){ plt->bmin[i] = bmin[a]; plt->bmax[i] = bmax[a]; plt->bbeg[i] = 0; plt->bbox[i] = bbox[a]; } // автошкалирование по оси
		else { // bbox, bbeg ???
			double A = bmin_[a], B = bmax_[a]; if((A<B)^(bmin[a]<bmax[a])) std::swap(A, B);  // ставим пределы сцены в том же порядке что и в контейнере		   
			if((A<=B && A<bmin[a])||(A>=B && A>bmin[a])){ plt->bbeg[i] = 0; plt->bmin[i] = bmin[a]; } // выход за левую границу
			else {
				plt->bbeg[i] = lgs? log(A/bmin[a])/step[a]: (A-bmin[a])/step[a];
				plt->bmin[i] = lgs ? bmin[a]*pow(step[a], plt->bbeg[i]) : bmin[a] + plt->bbeg[i]*step[a];
			}
			if((A<=B && B>bmax[a])||(A>=B && B<bmax[a])){ plt->bbox[i] = bbox[a]; plt->bmax[i] = bmax[a]; } // выход за правую границу
			else {
				plt->bbox[i] = (lgs? log(B/plt->bmin[i])/step[a]: (B-plt->bmin[i])/step[a])+.5;
				if(plt->bbox[i]<1) plt->bbox[i] = 1;
				if(plt->bbeg[i]+plt->bbox[i]>bbox[a]){
					if(bbox[a]-plt->bbeg[i]>0) plt->bbox[i] = bbox[a]-plt->bbeg[i];
					else plt->bbeg[i] = bbox[a]-plt->bbox[i];
				}
				plt->bmax[i] = lgs ? plt->bmin[i]*pow(step[a], plt->bbox[i]) : plt->bmin[i] + plt->bbox[i]*step[a];
			}
			// WERR(i, A, B, plt->bmin[i], plt->bmax[i]);
		}
		plt->spos[a] = plt->bbeg[i];
		if(plt->flips&(1<<i)) std::swap(plt->bmin[i], plt->bmax[i]); 
	}  // конец цикла по осям, настроены пределы отрисовки плоттера

	// WERR(plt->axisID, plt->spos, plt->dim, dim);
	
	if(!mode){ // 2D режим
		plt->phi = 0; plt->theta = 90;
		plt->flats.emplace_back();  auto &f = plt->flats.back();
		// f.ppf[0] = vecf(-.5f, -.5f);
		// f.ppf[1] = vecf( .5f, -.5f);
		// f.ppf[2] = vecf( .5f,  .5f);
		// f.ppf[3] = vecf(-.5f,  .5f);
		f.ppf[0] = vecf(-.5f,  .5f);
		f.ppf[1] = vecf( .5f,  .5f);
		f.ppf[2] = vecf( .5f, -.5f);
		f.ppf[3] = vecf(-.5f, -.5f);
		f.bounds = 255;
		f.spos = plt->spos; 
		f.axis[0] = 0; 	f.axis[1] = 1;
		plt->bmin(Ind<2>(f.axis)).to(f.bmin);
		plt->bmax(Ind<2>(f.axis)).to(f.bmax);
		f.spos[axisID[0]] = plt->bbeg[0];
		f.spos[axisID[1]] = plt->bbeg[1];
	} else { // 3D режим, настройки флэтов в плоттере
		if(plt->theta<0) plt->theta  = 0;
		if(plt->theta>180) plt->theta = 180;
		float c_ph, s_ph, c_th, s_th;  sincosf(-plt->phi*M_PI/180, &s_ph, &c_ph); sincosf(-plt->theta*M_PI/180, &s_th, &c_th);
		plt->nS = vecf(c_ph*s_th, s_ph*s_th, c_th); plt->nX = vecf(s_ph, -c_ph, 0.f); plt->nY = vecf(-c_th*c_ph, -c_th*s_ph, s_th);  // вектор ИЗ начала координат В экран
		Vecf<2> pp[8];  plt->Ahull = plt->Bhull = Vecf<2>();  float Zmin = 0;  // координаты вершин 
		for(int i=0; i<8; i++){  // цикл по углам куба
			Vecf<3> r = -plt->bbox&plt->cell_aspect/2;  for(int k=0; k<3; k++) if(i&1<<k) r[k] = -r[k];
			pp[i] = vecf(r*plt->nX, r*plt->nY);  plt->Ahull <<= pp[i]; plt->Bhull >>= pp[i];
			if(i==0 || r*plt->nS>Zmin){ plt->icenter = i;  Zmin = r*plt->nS; }
		}  // конец цикла по углам куба
		WERR(plt->icenter);
		for(int i=0; i<3; i++){
			int j = plt->icenter;  j = j&(1<<i) ? j-(1<<i): j+(1<<i);
			plt->orts[i] = (pp[j]-pp[plt->icenter])/plt->bbox[i];
		}
		// WERR(plt->spos, plt->bmin, plt->bmax, plt->bbeg, plt->bbox);
		for(int axe=0; axe<3; axe++){  // цикл по осям --- ищем отображаемые грани, достаточно проверить глубину одной вершины
			Vecf<3> r = plt->bbox&plt->cell_aspect/2; float z_plus = plt->nS*r;
			r[axe] = -r[axe]; float z_minus = plt->nS*r;
			if(z_plus>z_minus+1e-4f || z_plus<z_minus-1e-4f){ // точность ???
				plt->flats.emplace_back(); QpltFlat &f = plt->flats.back(); int m = 1<<axe, fix = (z_plus>z_minus)<<axe;
				for(int i=0, j=0; i<3; i++) if(i!=axe){ f.axis[j] = i; f.bmin[j] = plt->bmin[i]; f.bmax[j] = plt->bmax[i]; j++; }
				f.spos = plt->spos; f.spos[axisID[axe]] = plt->bbeg[axe]+(plt->bbox[axe]-1)*((z_plus>z_minus)^bool(plt->flips&(1<<axe))); // z_plus>z_minus ???
				for(int i=0, j=0; i<8; i++) if((i&m)==fix){
						if(j==0) for(int k=0; k<2; k++) f.cflips[k] = (plt->icenter&(1<<f.axis[k]))^(i&(1<<f.axis[k]));
						f.ppf[j++] = pp[i];
					}
				std::swap(f.ppf[2], f.ppf[3]);
				WERR(plt->flats.size()-1, f.axis[0], f.axis[1], f.cflips[0], f.cflips[1], f.spos, Vecf<2>(f.bmin), Vecf<2>(f.bmax));
			}
		}
		for(auto &f: plt->flats){
			f.bounds = 0;
			for(int i=0; i<4; i++){
				Vecf<2> a = f.ppf[i], b = f.ppf[(i+1)%4];
				int m = (fabs(a[0]-plt->Ahull[0])<1e-4) | (fabs(a[0]-plt->Bhull[0])<1e-4)<<1 | (fabs(a[1]-plt->Ahull[1])<1e-4)<<2 | (fabs(a[1]-plt->Bhull[1])<1e-4)<<3
					| (fabs(b[0]-plt->Ahull[0])<1e-4)<<4 | (fabs(b[0]-plt->Bhull[0])<1e-4)<<5 | (fabs(b[1]-plt->Ahull[1])<1e-4)<<6 | (fabs(b[1]-plt->Bhull[1])<1e-4)<<7;
				// первый бит --- оба конца принадлежат к оболочке AB,  второй бит --- оба конца слева или (не оба конца справа и ни один из концов сверху)
				if((m&0xF) && (m&0xF0)) f.bounds |= (1<<i*2) + (((m&17)==17 || ((m&34)!=34 && !(m&4) && !(m&64)))<<(i*2+1));
			}
		}
	} // 3D режим, флэты в плоттере настроены

	plt->init(f_opt&3);
	return plt;
}
//------------------------------------------------------------------------------
void aiw::QpltPlotter::set_image_size(int xy1[2], int xy2[2]){  // настраивает флэты согласно размеру изображения
	im_start = ptr2vec<2>(xy1);	im_size = ptr2vec<2>(xy2) - im_start; (im_start+im_size/2).to(center);
	scale = vecf(1.f, 1.f);
	if(dim==2){
		auto &f = flats[0]; im_start.to(f.d); (im_start+im_size).to(f.b);
		f.a[0] = f.d[0]; f.a[1] = f.b[1]; f.c[0] = f.b[0]; f.c[1] = f.d[1];
	} else { 
		// if(D3scale_mode==0) scale[0] = scale[1] = float(std::min(im_size[0], im_size[1]))/(bbox&cell_aspect).abs();
		// if(D3scale_mode==1)
		scale[0] = scale[1] = std::min(im_size[0]/(Bhull[0]-Ahull[0]), im_size[1]/(Bhull[1]-Ahull[1]));
		// WMSG(scale, im_size, Ahull, Bhull);
		// if(D3scale_mode==2){ scale[0] = im_size[0]/(Bhull[0]-Ahull[0]); scale[1] = im_size[1]/(Bhull[1]-Ahull[1]); }
		// nX *= scale[0];  nY *= scale[1];
		for(auto &f: flats) for(int i=0; i<4; i++) f.abcd(i) = Ind<2>(center)+(f.ppf[i]&scale); // +vecf(.5f); ???
	}

	for(int i=0; i<2; i++) ibmin[i] = ibmax[i] = center[i];
	for(auto &f: flats){
		for(int i=0; i<2; i++) f.bbox[i] = bbox[f.axis[i]];
		Vecf<2> abd[2] = {Ind<2>(f.b)-Ind<2>(f.a), Ind<2>(f.d)-Ind<2>(f.a)}, ac = Ind<2>(f.c)-Ind<2>(f.a), n_abd[2], n_perp[2];
		for(int i=0; i<2; i++){
			n_abd[i] = abd[i]/abd[i].abs(); n_perp[i] = vecf(-n_abd[i][1], n_abd[i][0]);
			if(n_perp[i]*abd[1-i]<0) n_perp[i] = -n_perp[i];
			f.rn[i] = abd[i]/f.bbox[i];
		}
		Vecf<2> n2X = f.bbox[0]*n_perp[1]/(n_perp[1]*ac);
		Vecf<2> n2Y = f.bbox[1]*n_perp[0]/(n_perp[0]*ac);
		f.nX[0] = n2X[0]; f.nX[1] = n2Y[0];
		f.nY[0] = n2X[1]; f.nY[1] = n2Y[1];
		for(int i=0; i<2; i++) for(int j=0; j<4; j++){ ibmin[i] = std::min(ibmin[i], f.abcd(j)[i]); ibmax[i] = std::max(ibmax[i], f.abcd(j)[i]); }

		/*
		WOUT(f.axis[0], f.axis[1]);
		aiw::Vecf<2> r;
		f.image2flat(f.a[0], f.a[1], r);
		f.image2flat(f.b[0], f.b[1], r);
		f.image2flat(f.c[0], f.c[1], r);
		f.image2flat(f.d[0], f.d[1], r);
		f.image2flat((f.a[0]+f.c[0])/2, (f.a[1]+f.c[1])/2, r);
		f.image2flat((f.b[0]+f.d[0])/2, (f.b[1]+f.d[1])/2, r);
		*/
	}
	// exit(1);
	// if(S.flcenter.y>=0) S.flcenter.y = Ny-1-S.flcenter.y;  // разворачиваем ось Y
	// for(int i=0; i<6; i++) if(S.flpoints[i].y>=0) S.flpoints[i].y = Ny-1-S.flpoints[i].y;  // разворачиваем ось Y
} 
//------------------------------------------------------------------------------
//   factory
//------------------------------------------------------------------------------
namespace aiw {
	void load_frames_impl(IOstream &fin, int frame,	std::vector<QpltContainer*> &res){  
		try{
			while(1){
				{   QpltMesh *msh = new QpltMesh;
					if(msh->load(fin)){ msh->frame_ = frame++; msh->end_offset = fin.tell(); res.push_back(msh);  continue; }
					else delete msh; 
				}
				{   QpltBalls *msh = new QpltBalls;
					if(msh->load(fin)){ msh->frame_ = frame++; msh->end_offset = fin.tell(); res.push_back(msh); continue; }
					else delete msh;
				}
				break;
			}
		} catch(...) {}
	}
} 
//------------------------------------------------------------------------------
std::vector<QpltContainer*> aiw::factory(const char *fname){  // поддержка GZ файлов?
	std::vector<QpltContainer*> res; 
	try{ File fin(fname, "rb"); load_frames_impl(fin, 0, res); } catch(...) {}
	return res;
}
//------------------------------------------------------------------------------
std::vector<QpltContainer*> aiw::QpltContainer::load_next_frames(){
	std::vector<QpltContainer*> res; 
	try{ fin->seek(0); fin->seek(end_offset); load_frames_impl(*fin, frame_+1, res); } catch(...) {}
	// std::cerr<<"load_nex_frames--> "<<res.size()<<'\n';
	return res;
}
//------------------------------------------------------------------------------
std::vector<QpltContainer*> aiw::QpltContainer::reload_all_frames(){
	std::vector<QpltContainer*> res; 
	try{ fin->seek(0); load_frames_impl(*fin, 0, res); } catch(...) {}
	return res;
}
//------------------------------------------------------------------------------
