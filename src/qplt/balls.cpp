/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <omp.h>
#include <sstream>
#include <algorithm>
#include "../../include/aiwlib/interpolations"
#include "../../include/aiwlib/binhead"
#include "../../include/aiwlib/qplt/balls"
using namespace aiw;

//------------------------------------------------------------------------------
//   load data
//------------------------------------------------------------------------------
bool aiw::QpltBalls::load(IOstream &S){
	// printf("load0: %p\n", mem_ptr);
	BinaryHead bh; size_t s = S.tell();
	if(!bh.load(S) || bh.type!=BinaryHead::balls || bh.dim!=3 || bh.szT<20){ S.seek(s);  return false; } // ???
	head = bh.head; dim = bh.dim; szT = bh.szT;
	for(int i=0; i<dim; i++){ anames[i] = bh.axis[i]; bmin[i] = bh.bmin[i]; bmax[i] = bh.bmax[i]; }
	
	
	bool calc_bb = false; for(int i=0; i<dim; i++){ if(anames[i].empty()){ anames[i] = default_anames[i]; } if(bmin[i]>=bmax[i]) calc_bb = true; }
	
	count = bh.count;  data_sz = szT*count; mem_sz = data_sz/1e9;
	fin = S.copy();  mem_offset = S.tell(); if(!S.seek(data_sz, SEEK_CUR)){  return false; }   // файл битый, записан не до конца
	WMSG(mem_offset, S.tell());
	if(calc_bb){
		S.seek(mem_offset); float rR[4]; // ???
		for(size_t i=0; i<count; i++){
			S.read(rR, 16); S.seek(szT-16, SEEK_CUR);
			if(!i) for(int k=0; k<3; k++){ bmin[k] = rR[k]-rR[3]; bmax[k] = rR[k]+rR[3]; }
			else for(int k=0; k<3; k++){ bmin[k] = std::min(bmin[k], rR[k]-rR[3]); bmax[k] = std::max(bmax[k], rR[k]+rR[3]); }
		}
	}

	// это костыль - пока нет возможность рисовать с сохранением размеров по bmin/bmax
	int i_min = 0; 	for(int i=0; i<dim; i++) if(bmax[i]-bmin[i]<bmax[i_min]-bmin[i_min]) i_min = i;
	bbox[i_min] = 1000; for(int i=0; i<dim; i++) if(i!=i_min) bbox[i] = (bmax[i]-bmin[i])/(bmax[i_min]-bmin[i_min])*bbox[i_min];

	calc_step();
	return true;
}
//------------------------------------------------------------------------------
// void aiw::QpltMesh::data_load_cuda(){ fin->seek(mem_offset); fin->read(mem_ptr, data_sz); }
void aiw::QpltBalls::data_free_impl(){  // выгружает данные из памяти
	WERR(this); mem.reset(); mem_ptr = nullptr;
}  
void aiw::QpltBalls::data_load_impl(){  // загружает данные в память
	fin->seek(mem_offset);
	mem = fin->mmap(data_sz, 0);
	mem_ptr = (char*)(mem->get_addr());
}
//------------------------------------------------------------------------------
QpltPlotter* aiw::QpltBalls::mk_plotter(int mode) { this->data_load(); return new QpltBallsPlotter; }
//------------------------------------------------------------------------------
//   implemetation of plotter virtual functions
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltBallsPlotter::init_impl(int autoscale){
	Vecf<2> f_lim(color.get_min(), color.get_max());
	if(autoscale&1){		
#ifdef PROFILE
		double t0 = omp_get_wtime();
#endif //PROFILE
		constexpr int dout = QpltAccessor::DOUT<AID>();
		QpltBalls* cnt = (QpltBalls*)container; Vecf<3> rmin = cnt->bmin(0,1,2), rmax = cnt->bmax(0,1,2); // ???
		if(autoscale==1) for(int i=0; i<3; i++){  // расчет пределов по выделенному кубу
				rmin[i] = cnt->pos2coord(spos[axisID[i]], axisID[i])-.5*cnt->step[axisID[i]]; // smax[axisID[i]] += bbox[i]-1;
				rmax[i] = cnt->pos2coord(bbox[axisID[i]], axisID[i])-.5*cnt->step[axisID[i]]; // smax[axisID[i]] += bbox[i]-1;
			}		
		const char* ptr0 = cnt->mem_ptr, *nb[6] = {nullptr}; float f_min = HUGE_VALF,  f_max = -HUGE_VALF; size_t count = cnt->count;
#ifndef AIW_WIN32
#pragma omp parallel for reduction(min:f_min) reduction(max:f_max)  // MSVC does not support OpenMP 3
#else //AIW_WIN32
		std::vector<float> f_mina(omp_get_max_threads(), HUGE_VALF), f_maxa(omp_get_max_threads(), -HUGE_VALF);
		int act_numthreads = -1; // to be determined in the following
#pragma omp parallel for //shared(f_mina, f_maxa)
#endif //AIW_WIN32
		for(size_t bID=0; bID<count; bID++){
			const char *ptr = ptr0+bID*cnt->szT;
			const Vecf<3> r = ptr2vec<3>((const float*)ptr);
			float R = *(const float*)(ptr+12);
			if(rmin<=r+vecf(R) && r-vecf(R)<=rmax){ // могут быть ложные срабатывания на ребрах и вершинах
				Vecf<3> ff; accessor.conv<AID>(ptr, (const char**)nb, &(ff[0]));
				float fres = dout==1? ff[0]: ff.abs();	      				
#ifndef AIW_WIN32
				f_min = std::min(f_min, fres);  f_max = std::max(f_max, fres);
#else //AIW_WIN32
				if (act_numthreads < 0){
# pragma omp critical
					act_numthreads = omp_get_num_threads();
				}
				int me = omp_get_thread_num();
				f_mina[me] = std::min(f_mina[me], fres);  f_maxa[me] = std::max(f_maxa[me], fres);
#endif //AIW_WIN32
			}
		}  // end of balls loop
#ifdef AIW_WIN32
		f_min = *std::min(f_mina.begin(), f_mina.begin()+ act_numthreads);
		f_max = *std::max(f_maxa.begin(), f_maxa.begin() + act_numthreads);
#endif //AIW_WIN32
		f_lim[0] = f_min;  f_lim[1] = f_max;  // cnt->flimits[LID] = f_lim;
#ifdef PROFILE
		fprintf(stderr, "limits %g sec\n", omp_get_wtime()-t0);
#endif //PROFILE
	} // конец расчета пределов
//	if(accessor.minus) f_lim = -f_lim(1,0);
	color.reinit(f_lim[0], f_lim[1]);
	// WMSG(f_lim, bmin, bmax, bbox);
}
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltBallsPlotter::plot_impl(std::string &res) const {
#ifdef PROFILE
	double t0 = omp_get_wtime();
#endif //PROFILE

	constexpr int dout = QpltAccessor::DOUT<AID>();
	// constexpr int PAID = dout==1? AID: (AID&0x7F)|(3<<6); // если итоговое поле векторное - рисуем его модуль
	QpltBalls* cnt = (QpltBalls*)container;
	const char* ptr0 = cnt->mem_ptr, *nb[6] = {nullptr};  int Nx = ibmax[0]-ibmin[0], Ny = ibmax[1]-ibmin[1];  	
	std::vector<ZBuffer> zbufs(omp_get_max_threads()); for(auto &zb: zbufs) zb.init(Nx, Ny);
	// Vecf<2> S0((bmin+bmax)*nX/2, (bmin+bmax)*nY/2);
	size_t count = cnt->count;
	
	float flats_coord[3], zbc = nS*(bmin+bmax)/2;
	for(int i=0; i<int(flats.size()); i++){
		int a = 3-(flats[i].axis[0]+flats[i].axis[1]), a0 = axisID[a];  // 0+1=1-->2 || 1+2=3-->0 || 0+2-->1
		flats_coord[i] = -(cnt->bmin[a0]+flats[i].spos[a0]*cnt->step[a0] - (bmin[a0]+bmax[a0])/2);
	}

	
	// WMSG(bmin, bmax);
#pragma omp parallel for
	for(size_t bID=0; bID<count; bID++){
		const char *ptr = ptr0+bID*cnt->szT;
		const Vecf<3> r0 = ptr2vec<3>((const float*)ptr); Vecf<3> r;
		for(int i=0; i<3; i++){ r[i] = r0[axisID[i]]; if(flips&(1<<i)) r[i] = bmax[i]-r[i]; }  // flips???
		float R = *(const float*)(ptr+12);  

		// if(bmin<=r+vecf(R) && r-vecf(R)<=bmax){ 
		if(bmin<=r && r<=bmax){ 
			Vecf<3> br = (bmin+bmax)/2-r, ff; accessor.conv<AID>(ptr, (const char**)nb, &(ff[0]));
			float fres = dout==1? ff[0]: ff.abs();	int cf = color(fres);      				
			auto &zb = zbufs[omp_get_thread_num()];

			Ind<2> ri = -vecf(nX*br, nY*br)*scale[0]/cnt->step[0] + ind(Nx/2, Ny/2);
			float z = nS*br;  // проекция центра и радиус шара
			int Ri = R*scale[0]/cnt->step[0]-.5; float _Ri2 = 1.f/(Ri*Ri);
			const bool in_mode = true; // bmin<=r-vecf(R) && r+vecf(R)<=bmax;  // могут быть ложные срабатывания на ребрах и вершинах			
			
			for(int iy=std::max(0, ri[1]-Ri), iy_max=std::min(ri[1]+Ri+1, Ny); iy<iy_max; iy++){
				int yi2 = (iy-ri[1])*(iy-ri[1]), xsz = sqrtf(Ri*Ri-yi2), y0 = ibmin[1]+iy;
				for(int ix=std::max(0, ri[0]-xsz), ix_max=std::min(ri[0]+xsz+1, Nx); ix<ix_max; ix++){
					int ixc = ix-ri[0];  float dz = (yi2+ixc*ixc)*_Ri2, z1 = z-R*sqrtf(1-dz);
					if(in_mode) zb.set_pixel(ix, iy, color.move2diag(cf, .5*dz, 255), z1); 
					else {  // шарик частично выходит за область отрисовки
						int x0 = ibmin[0]+ix, fli = 0; Vecf<2> rf2D; 
						for(const auto &fl: flats){
							if(fl.image2flat(x0, y0, rf2D)){
								Vecf<3> rf; for(int k=0; k<2; k++) rf[fl.axis[k]] = -(fl.bmin[k] + rf2D[k]/fl.bbox[k]*(fl.bmax[k]-fl.bmin[k]) - (fl.bmin[k]+fl.bmax[k])/2);
								rf[3-fl.axis[0]-fl.axis[1]] = flats_coord[fli];  // 0+1=1-->2 || 1+2=3-->0 || 0+2-->1
								float z1 = z-dz, z2 = zbc-nS*rf;
								if(z1>z2) zb.set_pixel(ix, iy, color.move2diag(cf, .5*dz, 255), z1); else zb.set_pixel(ix, iy, cf, z2);
								break;
							}
							fli++;
						}
					}
				}
			}				
		}
	}  // end of balls loop
	join_zbufs(zbufs, res);
#ifdef PROFILE
	fprintf(stderr, "plot %g sec\n", omp_get_wtime()-t0);
#endif //PROFILE
}
//------------------------------------------------------------------------------

