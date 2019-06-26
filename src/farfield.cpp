/**
 * Copyright (C) 2019 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <omp.h>
#include "../include/aiwlib/farfield"
#include "../include/aiwlib/mesh"
using namespace aiw;
//------------------------------------------------------------------------------
void aiw::FarField::init(FarFieldBuf *buf, int sph_rank, int time_max_, const char *path){
	p = buf; time_max = time_max_; data.init(sph_rank, 0, 1); 
	for(size_t i=0; i<data.size(); i++) data[i].resize(time_max, Vecf<3>());
	if(path){
		fdiagn = true;
		for(int i=0; i<3; i++) for(int j=0; j<3; j++) farrs[i][j] = File("%/%E%.msh", "w", path, ("xyz"[i]), ("xyz"[j]));
	}
	dtaus.resize(data.size());
}
//------------------------------------------------------------------------------
void aiw::FarField::step(int ti0){
	for(size_t i=0; i<dtaus.size(); i++) for(int k=0; k<3; k++) dtaus[i][k] = data.center(i)*Ecoord[k];
	double _dt = 1/dt; 
	int sph_sz = data.size();
	for(tile_t& T: *this){
		double mul = h*h/(4*M_PI); // h[T.ai]*h[T.aj]/(4*M_PI);
		for(cell_t &C: T){
#pragma omp parallel for	
			for(int ni=0; ni<sph_sz; ni++){
				const Vec<3> &n = data.center(ni); // направление
				double ngn = T.n*n, nr = C.center*n, tau = (ti0+C.ti+.5)*dt-nr; 
				Vecf<3> u = -(C.dEdn-C.dEdt*ngn)*mul;
				for(int k=0; k<3; k++){
					int dst_it = (tau-dtaus[ni][k])*_dt; // это можно как то оптимизировать? Поднять точность - аппроксимация первого порядка по времени?
					if(0<=dst_it && dst_it<time_max) data[ni][dst_it][k] += u[k];
				}
			}
		}
	}
	if(fdiagn) for(int axe=0; axe<3; axe++){
			int ai = (axe+1)%3, aj = (axe+2)%3;
			Mesh<float, 3> arrs[3]; for(int i=0; i<3; i++) arrs[i].init(ind(p->surf_sz[ai], p->surf_sz[aj], 4));
			for(int ti=0; ti<p->time_sz; ti++){
				for(tile_t& T: *this){
					if(T.axe!=axe) continue;
					T.ti = ti;
					for(cell_t &C: T){
						for(int l=0; l<2; l++){
							Ind<3> pos(T.I*p->tile_sz+C.i, T.J*p->tile_sz+C.j, l+(T.g>=3)*2);
							// for(int k=0; k<3; k++) arrs[k][pos] = C.E[3*l+k];
							for(int k=0; k<3; k++) arrs[k][pos] = l? C.dEdt[k]: C.dEdn[k];
						}
					}
				}
				for(int k=0; k<3; k++) arrs[k].dump(farrs[axe][k]);
			}
		}
	p->move_last_frame_to_first();
}
//------------------------------------------------------------------------------
void aiw::FarField::finish(const char *path, double w_min, double w_max, int w_sz){
	char pw[1024], pt[1024]; sprintf(pw, "%s/nw/", path); sprintf(pt, "%s/nt/", path);
	::mkdir(pw, 0755); ::mkdir(pt, 0755);
	Sphere<float> sphP(data.rank(), 0, 1); sphP.fill(0.f);
	for(int ni=0; ni<int(data.size()); ni++){ // цикл по направлениям
		File ft("%/%.dat", "w", pt, fill(ni, 4)); ft.printf("#:t Ex Ey Ez\n");
		for(int ti=0; ti<time_max; ti++) ft.printf("%g %lf %lf %lf\n", (ti+.5)*dt, data[ni][ti][0], data[ni][ti][1], data[ni][ti][2]);

		float Ic[w_sz], Is[w_sz], dw = (w_max-w_min)/w_sz;
		for(int iw=0; iw<w_sz; iw++) Ic[iw] = Is[iw] = 0;
		for(int ti=0; ti<time_max; ti++){
			float t = (ti+.5)*dt, P = data[ni][ti]*data[ni][ti]*dt, s, c;
			for(int iw=0; iw<w_sz; iw++){
				sincosf(float((w_min+(iw+.5)*dw)*t), &s, &c);
				Is[iw] += s*P; Ic[iw] += c*P;
			}
			sphP[ni] += P;
		}
		File fw("%/%.dat", "w", pw, fill(ni, 4)); fw.printf("#:omega A phi\n");
		for(int iw=0; iw<w_sz; iw++)
			fw.printf("%g %g %g\n", w_min+(iw+.5)*dw, sqrt(Is[iw]*Is[iw]+Ic[iw]*Ic[iw]), atan2(Is[iw], Ic[iw]));		
	} // конец цикла по направлениям
	sphP.dump(File("%/P.sph", "w", path));
}
//------------------------------------------------------------------------------
//   заполняет буфер
void aiw::FarField::dipole_test(int ti0,     // прошедшее число шагов (потом оно же передается в step)
								const Dipole& dipole){
	// dipole._Ts2 = 1./(dipole.Ts*dipole.Ts);
	for(tile_t& T: *this){
		double dh = T.g<3? -.5*h: .5*h;
		for(cell_t &C: T){
			for(int l=0; l<2; l++){
				Vec<3> rc = C.center; rc[T.axe] += dh*(2*l-1); // ???
				for(int k=0; k<3; k++) C.E[3*l+k] = dipole.E(rc+Ecoord[k], (ti0+C.ti+.5)*dt)[k];
			}
		}
	}
}
//------------------------------------------------------------------------------
bool aiw::FarField::check(){
	// File coord0("coord0.dat", "w"), coord1("coord1.dat", "w");
	for(tile_t& T: *this)
		for(cell_t &C: T)
			for(int l=0; l<2; l++){
				Ind<3> pos;
				pos[T.axe] = T.axe_pos + l*(1-2*(T.g<3));
				pos[T.ai] = p->offset[T.ai] + T.I*p->tile_sz + C.i;
				pos[T.aj] = p->offset[T.aj] + T.J*p->tile_sz + C.j;
				float E[3] = {float(random()), float(random()), float(random())};
				// if(p->set(C.ti, (int*)(&pos), E)) (l?coord1:coord0)("%\n", pos);
				if(!(p->set(C.ti, (int*)(&pos), E))){ WOUT(pos, T.g, T.I, T.J, C.i, C.j, C.ti, l); return false; }
				for(int k=0; k<3; k++) if(E[k]!=C.E[k+3*l]) { WOUT(pos, T.g, T.I, T.J, C.i, C.j, C.ti, k, l, E[k], C.E[k+3*l]); return false; }
			}
	return true;
}
//------------------------------------------------------------------------------
