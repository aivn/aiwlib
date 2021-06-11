/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <sstream>
#include "../../include/aiwlib/interpolations"
#include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/qplt/mesh"
using namespace aiw;
//------------------------------------------------------------------------------
bool aiw::QpltMesh::load(IOstream &S){
	BinaryFormat bf;  bf.box = &bbox0;  bf.bmin = &bmin0;  bf.bmax = &bmax0;  bf.axes = axes;  bf.D = -1;
	size_t s = S.tell();  if(!bf.load(S) || bf.D&(0xFFFF<<16) || !(bf.D&0xFF)){ S.seek(s); return false; }
	head = bf.head; dim0 = dim = bf.D; szT = bf.szT; logscale = bf.logscale;  
	for(int i=0; i<dim0; i++) if(axes[i].empty()) axes[i] = default_axes[i];
	for(int i=0; i<dim;) if(bbox0[i]==1) for(int j=i+1; j<dim; j++){  // убираем оси с размером 1
				bbox0[j-1] = bbox0[j]; bmin0[j-1] = bmin0[j]; bmax0[j-1] = bmax0[j]; axes[j-1] = axes[j];
				uint32_t m = ~uint32_t(0)<<i; logscale = (logscale&~m)|((logscale&m)>>1);
				dim--;
			} else i++;
	
	size_t sz = szT; for(int i=0; i<dim; i++) sz *= bbox0[i];
	mem_sz = sz/1e9;  fin = S.copy();  mem_offset = S.tell(); S.seek(sz, 1);  mem.reset();

	s = S.tell(); int32_t sz2 = 0; S.load(sz2);  // try read old aivlib mesh format (deprecated)
	if(S.tell()-s==4 && sz2==-int(dim*24+4+szT)){ S.read(&bmin0, dim*8); S.read(&bmax0, dim*8); S.seek(dim*8, 1); S.seek(szT, 1); logscale = 0;  } 
	else  S.seek(s);
	
	// for(int i=0; i<D; i++) this->set_step(i);
	// for(auto i: bf.tinfo.get_access()) std::cout<<i.label<<' '<<i.offset<<'\n';
	// std::cout<<bf.tinfo;
#ifdef AIW_TYPEINFO
	cfa_list = bf.tinfo.get_access();
#endif //AIW_TYPEINFO
	// segy = false;

	mul0[0] = szT; for(int i=1; i<dim; i++) mul0[i] = mul0[i-1]*bbox0[i-1];
	return true;
}
//------------------------------------------------------------------------------
void aiw::QpltMesh::data_free_impl(){ mem.reset(); }  // выгружает данные из памяти
void aiw::QpltMesh::data_load_impl(){                 // загружает данные в память
	size_t sz = szT; for(int i=0; i<dim; i++) sz *= bbox[i];
	fin->seek(mem_offset); mem = fin->mmap(sz, 0);
	WOUT(mem_offset, mem->get_addr());
}
//------------------------------------------------------------------------------
template <int AID> struct aiw::QpltMesh::calc_t{
	QpltAccessor *acc;
	uint64_t mul[3]; int bbeg[2], bbox0[2]; Ind<2> bbox; bool diff3plus, diff3minus;
	char *ptr0;
	
	typedef Vecf<QpltAccessor::DOUT<AID>()> cell_type;
	static const int dim = 2;

	cell_type operator[](Ind<2> pos) const {
		for(int i=0; i<2; i++){ if(pos[i]<0) pos[i] = 0; if(pos[i]>=bbox0[i]) pos[i] = bbox0[i]-1; }
		char *nb[6] = {nullptr}, *ptr = ptr0; for(int i=0; i<2; i++) ptr += mul[i]*pos[i];
		if((AID>>3)&7){
			for(int i=0; i<2; i++){
				if(bbeg[i]+pos[i]>0) nb[2*i] = ptr-mul[i]; else nb[2*i] = ptr;
				if(bbeg[i]+pos[i]<bbox0[i]-1) nb[2*i+1] = ptr+mul[i]; else nb[2*i+1] = ptr;
			}
			if(diff3minus) nb[4] = ptr-mul[2]; else nb[4] = ptr;
			if(diff3plus)  nb[5] = ptr+mul[2]; else nb[5] = ptr;
		}
	    cell_type f; acc->conv<AID>(ptr, (const char**)nb, &(f[0]));
		return f;
	}
	int interp; float step[2]; // , rstep[2];
	void mod_coord(int axe, float r, int &pos, double &x) const { // вычисляет pos и  x на основе r
		x = r*step[axe]; pos = x; x -= pos; 
		// if(interp&(0xF<<(4*(1-axe)))) x = r*rstep[axe] - pos; // *step[axe];
		/*
		  if(interp&(0xF<<(4*(1-axe)))){
		      pos = std::floor(logscale&1<<axe ? log(r/bmin[axe])*rstep[axe]-.5 :(r-bmin[axe])*rstep[axe]-.5); // ???
			  x = logscale&1<<axe ? log(r/(bmin[axe]*pow(step[axe], pos)))*rstep[axe]-.5 : (r-bmin[axe])*rstep[axe]-pos-.5;
		} else pos = logscale&1<<axe ? log(r/bmin[axe])*rstep[axe] :(r-bmin[axe])*rstep[axe];
		*/
	}

};
//------------------------------------------------------------------------------
template <int AID>  aiw::QpltMesh::calc_t<AID>  aiw::QpltMesh::get_calc(QpltAccessor &acc, QpltScene &scene, Ind<2> im_sz){
	// WOUT(AID, QpltAccessor::DOUT<AID>(), acc.Dout());
	calc_t<AID> plt;  plt.acc = &acc; plt.ptr0 = ptr0;
	for(int i=0; i<3; i++) plt.mul[i] = mul[i];
	for(int i=0; i<2; i++){ plt.bbeg[i] = scene.get_flip(i)? bbox0[aI[i]]-bbeg[i]-1: bbeg[i]; plt.bbox0[i] = bbox0[aI[i]]; plt.bbox[i] = bbox[i]; } // ???
	// plt.diff3minus = scene.get_flip(2)? bbeg[2]+1<bbox0[aI[2]]: bbeg[2]>0;
	// plt.diff3plus  = scene.get_flip(2)? bbeg[2]>0: bbeg[2]+bbox[2]<bbox0[aI[2]];
	plt.diff3minus = scene.get_flip(2)? spos[aI[2]]+1<bbox0[aI[2]]: spos[aI[2]]>0;
	plt.diff3plus  = scene.get_flip(2)? spos[aI[2]]>0: spos[aI[2]]+1<bbox0[aI[2]];
	plt.interp = scene.get_interp(0)<<4|scene.get_interp(1);
	for(int i=0; i<2; i++) plt.step[i] = float(bbox[i])/im_sz[i];
	return plt;
}
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMesh::prepare_impl(QpltAccessor &acc, QpltScene &scene, QpltColor &color){
	this->data_load();  ptr0 = (char*)(mem->get_addr());  
	
	// 1. рассчитываем множители и начальное смещение		
	Ind<8> smin, smax; 
	for(int a=0; a<dim; a++) if(a!=aI[0] && a!=aI[1] && (!scene.D3 || a!=aI[2])){				
			ptr0 += mul0[a]*spos[a];  smin[a] = spos[a];  smax[a] = smin[a]+1;
		}
	char *ptr1 = ptr0;  // указывает на нижний угол области по которой считаются пределы функции
	for(int a=0; a<2+scene.D3; a++){
		mul[a] = mul0[aI[a]]; ptr0 += mul[a]*bbeg[a]; ptr1 += mul[a]*bbeg[a];
		smin[aI[a]] = bbeg[a]; smax[aI[a]] = bbeg[a]+bbox[a];
		if(scene.get_flip(a)){ ptr0 += (bbox[a]-1)*mul[a]; mul[a] = -mul[a]; }
	}
	if(!scene.D3) mul[2] = mul0[aI[2]];
	if(acc.diff) for(int a=0; a<3; a++) acc.rsteps[a] = bbox[a]/(bmax[a]-bmin[a]);  // logscale ???		
	// WOUT(smin, smax, bmin, bmax, bbeg, bmin0, bmax0, bbox, bbox0);
	// WOUT(mul0[0], mul0[1], mul0[2], mul[0], mul[1], mul[2], aI[0], aI[1], aI[2]);

	//	init_segfault_hook();
	// 2. определяем пределы для функции
	if(scene.autoscale){
		if(scene.autoscale_tot){ smin = ind(0); smax = bbox0; ptr1 = (char*)(mem->get_addr()); }
		Ind<21> LID = smin|smax|AID|acc.ctype|acc.get_offset(0)|acc.get_offset(1)|acc.get_offset(2);
		auto I_LID = flimits.find(LID); Vecf<2> f_lim;
		if(I_LID!=flimits.end()) f_lim = I_LID->second; 
		else { // считаем пределы
			constexpr int DIFF = (AID>>3)&7; // какой то static method в accessor?
			char *nb[6] = {nullptr}; int ddim = acc.Ddiff(); constexpr int dout = QpltAccessor::DOUT<AID>(); // размерность дифф. оператора  и выходных данных			
			// 	WOUT(DIFF, ddim, dout, (void*)ptr0, (void*)ptr1, aI[0], aI[1], aI[2], smax, smin);
			if(DIFF) for(int i=0; i<ddim; i++){
					if(smin[aI[i]]>0) nb[2*i] = ptr1-mul0[aI[i]]; else nb[2*i] = ptr1;
					if(smin[aI[i]]<bbox0[aI[i]]-1) nb[2*i+1] = ptr1+mul0[aI[i]]; else nb[2*i+1] = ptr1;
				}
			WOUT(AID, DIFF, ddim, dout, (void*)ptr1, (void*)(nb[0]), (void*)(nb[1]), (void*)(nb[2]), (void*)(nb[3]), (void*)(nb[4]), (void*)(nb[5]));
			Vecf<3> f; acc.conv<AID>(ptr1, (const char**)nb, &(f[0])); float f_min = f.abs(), f_max = f_min;
			Ind<8> sbox = smax-smin; size_t sz = sbox[0]; for(int i=1; i<dim; i++) sz *= sbox[i];
#pragma omp parallel for reduction(min:f_min) reduction(max:f_max)
			for(size_t i=0; i<sz; i++){
				int pos[dim]; size_t j = i; char *ptr2 = ptr1, *nb2[6] = {nullptr};   
				for(int k=0; k<dim; k++){ pos[k] = j%sbox[k]; ptr2 += mul0[k]*pos[k]; pos[k] += smin[k]; j /= sbox[k]; }					
				if(DIFF) for(int k=0; k<ddim; k++){
						if(pos[aI[k]]>0) nb2[2*k] = ptr2-mul0[aI[k]]; else nb2[2*k] = ptr2;
						if(pos[aI[k]]<bbox0[aI[k]]-1) nb2[2*k+1] = ptr2+mul0[aI[k]]; else nb2[2*k+1] = ptr2;
					}
				//				WEXC(sz, i, pos[0], pos[1], pos[2], (void*)ptr1, (void*)ptr2, (void*)(nb2[0]), (void*)(nb2[1]), (void*)(nb2[2]), (void*)(nb2[3]), (void*)(nb2[4]), (void*)(nb2[5]));
				Vecf<3> ff; acc.conv<AID>(ptr2, (const char**)nb2, &(ff[0]));
				if(i==15000) WOUT(AID, DIFF, ff);
				float fres = dout==1? ff[0]: ff.abs();					
				if(f_min>fres){ f_min = fres; }  if(f_max<fres){ f_max = fres; }
			}
			f_lim[0] = f_min;  f_lim[1] = f_max;  flimits[LID] = f_lim;
		}
		color.reinit(f_lim[0], f_lim[1]); 
	    // WOUT(acc.Din, acc.Dout(), AID, color.get_min(), color.get_max());
	}
}
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMesh::plot_impl(QpltAccessor &acc, QpltScene &scene, QpltColor &color, QpltImage &im){
	// if(acc.Dout()>1) return;  // мы пока не умеем рисовать векторные поля ;-(
	if(!scene.D3){ // 2D mode
		constexpr int dout = QpltAccessor::DOUT<AID>();
		constexpr int PAID = dout==1? AID: (AID&0x7F)|(3<<6); // если итоговое поле векторное - рисуем его модуль 
		calc_t<PAID> plt = get_calc<PAID>(acc, scene, ind(im.Nx, im.Ny));
#pragma omp parallel for 
		for(int y=0; y<im.Ny; y++){
			Ind<2> pos; Vec<2> X; plt.mod_coord(1, y, pos[1], X[1]);
			for(int x=0; x<im.Nx; x++){
				plt.mod_coord(0, x, pos[0], X[0]);
				// if(!y) WOUT(x, pos, X);
				im.set_pixel(x, y, color(interpolate(plt, pos, X, plt.interp)[0]));
			}
		}
		//im.dump2ppm("/tmp/qplt.ppm");
		if(dout>1){
			calc_t<AID> plt2 = get_calc<AID>(acc, scene, ind(im.Nx, im.Ny));
			Ind<2> N = ind(im.Nx, im.Ny), Na = N/color.arr_length/color.arr_spacing<<plt2.bbox, d = N/Na;
			WOUT(N, d, Na, plt2.bbox, plt2.diff3minus, plt2.diff3plus, bbeg[2]);
			for(int iy=0; iy<Na[1]; iy++)  for(int ix=0; ix<Na[0]; ix++){
					int x = ix*d[0]+d[0]/2,  y = iy*d[1]+d[1]/2; Ind<2> pos; Vec<2> X;
					plt2.mod_coord(0, x, pos[0], X[0]);	plt2.mod_coord(1, y, pos[1], X[1]);
					// if(!y) WOUT(x, pos, X);
					auto v = interpolate(plt2, pos, X, plt.interp);
					color.arr_plot(x, y, (const float*)&v, im);
					// im.set_pixel(x, y, 0xFFFFFF);
				}
		}
	} else { // 3D mode

		
		
	}
}
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMesh::get_impl(QpltAccessor& acc, QpltScene& scene, float x, float y, std::string &res){
	calc_t<AID> plt = get_calc<AID>(acc, scene, ind(1, 1));
	Ind<2> pos; Vec<2> X; plt.mod_coord(0, x, pos[0], X[0]); plt.mod_coord(1, y, pos[1], X[1]); auto f = interpolate(plt, pos, X, plt.interp);
	std::stringstream S; S<<f[0]; for(int i=1; i<f.size(); i++) S<<'\n'<<f[i];
	res = S.str();
}
//------------------------------------------------------------------------------
