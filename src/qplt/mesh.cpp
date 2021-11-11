/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <omp.h>
#include <sstream>
#include "../../include/aiwlib/interpolations"
#include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/qplt/mesh"
#include "../../include/aiwlib/qplt/vtexture"
using namespace aiw;
//------------------------------------------------------------------------------
//   load data
//------------------------------------------------------------------------------
bool aiw::QpltMesh::load(IOstream &S){
	double t0 = omp_get_wtime();
	BinaryFormat bf;  bf.box = &bbox; Vec<6> bmin_, bmax_;  bf.bmin = &bmin_;  bf.bmax = &bmax_;  bf.axes = anames;  bf.D = -1;
	size_t s = S.tell();  if(!bf.load(S) || bf.D&(0xFFFF<<16) || !(bf.D&0xFF)){ S.seek(s); return false; }
	double t1 = omp_get_wtime(); WERR(t1-t0);
	head = bf.head; dim = bf.D; szT = bf.szT; logscale = bf.logscale; 
	for(int i=0; i<dim; i++) if(anames[i].empty()) anames[i] = default_anames[i];
	
	/*** for(int i=0; i<dim;) if(bbox0[i]==1) for(int j=i+1; j<dim; j++){  // убираем оси с размером 1
				bbox0[j-1] = bbox0[j]; bmin0[j-1] = bmin0[j]; bmax0[j-1] = bmax0[j]; axes[j-1] = axes[j];
				uint32_t m = ~uint32_t(0)<<i; logscale = (logscale&~m)|((logscale&m)>>1);
				dim--;
				} else i++; ***/
	
	size_t sz = szT; for(int i=0; i<dim; i++) sz *= bbox[i];
	mem_sz = sz/1e9;  fin = S.copy();  mem_offset = S.tell(); mem.reset(); if(!S.seek(sz, 1)) return false;  // файл битый, записан не до конца
	
	s = S.tell(); int32_t sz2 = 0; S.load(sz2);  // try read old aivlib mesh format (deprecated)
	if(S.tell()-s==4 && sz2==-int(dim*24+4+szT)){ S.read(&bmin_, dim*8); S.read(&bmax_, dim*8); S.seek(dim*8, 1); S.seek(szT, 1); logscale = 0;  } 
	else  S.seek(s);
	bmin = bmin_; bmax = bmax_; calc_step();

	// for(auto i: bf.tinfo.get_access()) std::cout<<i.label<<' '<<i.offset<<'\n';
	// std::cout<<bf.tinfo;
#ifdef AIW_TYPEINFO
	cfa_list = bf.tinfo.get_access();
#endif //AIW_TYPEINFO
	// segy = false;

	mul[0] = szT; for(int i=1; i<dim; i++) mul[i] = mul[i-1]*bbox[i-1];
	WERR(omp_get_wtime()-t1);
	return true;
}
//------------------------------------------------------------------------------
void aiw::QpltMesh::data_free_impl(){ WERR(this); mem.reset(); }  // выгружает данные из памяти
void aiw::QpltMesh::data_load_impl(){                 // загружает данные в память
	size_t sz = szT; for(int i=0; i<dim; i++) sz *= bbox[i];
	fin->seek(mem_offset); mem = fin->mmap(sz, 0); WERR(this, mem.get(), mem_limit);
	// WOUT(mem_offset, mem->get_addr());
}
//------------------------------------------------------------------------------
QpltPlotter* aiw::QpltMesh::mk_plotter(int mode) { 	this->data_load();  ptr = (char*)(mem->get_addr());  return new QpltMeshPlotter; }
//------------------------------------------------------------------------------
//   calc_t structure
//------------------------------------------------------------------------------
template <int AID> struct aiw::QpltMeshPlotter::calc_t : public QpltFlat {
	QpltAccessor *acc;
	int64_t mul[3]; Ind<2> bbeg, bbox0; Ind<2> bbox; bool diff3plus, diff3minus; // bbox0 лишний, можно обойтись флагами?
	char *ptr0; 
	
	typedef Vecf<QpltAccessor::DOUT<AID>()> cell_type;
	static const int dim = 2;

	char* get_ptr(const Ind<2> &pos) const { char *ptr = ptr0; for(int i=0; i<2; i++){ ptr += mul[i]*pos[i]; } return ptr;}
	cell_type operator[](Ind<2> pos) const {
		for(int i=0; i<2; i++){ if(pos[i]<0){ pos[i] = 0; } if(pos[i]>=bbox0[i]){ pos[i] = bbox0[i]-1; } }
		char *nb[6] = {nullptr}, *ptr = get_ptr(pos); 
		if((AID>>3)&7){  // если необходимо дифференцирование задаем соседей
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
	int interp; float step2D[2]; // , rstep[2];
	void mod_coord(Vecf<2> r, Ind<2> &pos, Vecf<2> &X) const { pos = r; X = r-pos; }
	void mod_coord2D(int axe, float r, int &pos, float &x) const { // вычисляет pos и  x на основе r
		x = r*step2D[axe]; pos = x; x -= pos; 
		// if(interp&(0xF<<(4*(1-axe)))) x = r*rstep[axe] - pos; // *step[axe];
		/***
		  if(interp&(0xF<<(4*(1-axe)))){
		      pos = std::floor(logscale&1<<axe ? log(r/bmin[axe])*rstep[axe]-.5 :(r-bmin[axe])*rstep[axe]-.5); // ???
			  x = logscale&1<<axe ? log(r/(bmin[axe]*pow(step[axe], pos)))*rstep[axe]-.5 : (r-bmin[axe])*rstep[axe]-pos-.5;
		} else pos = logscale&1<<axe ? log(r/bmin[axe])*rstep[axe] :(r-bmin[axe])*rstep[axe];
		***/
	}
	// bool check_in(Vecf<2> r) const { return bbeg[0]<=r[0] && bbeg[1]<=r[1] && r[0]<bbeg[0]+bbox[0] && r[1]<bbeg[1]+bbox[1]; }
	bool check_in(Vecf<2> r) const { return 0<=r[0] && 0<=r[1] && r[0]<bbox[0] && r[1]<bbox[1]; }
	void pos2to3(const Ind<2> &pos2, Ind<3> &pos3) const { for(int i=0; i<2; i++) pos3[axis[i]] = cflips[i]? bbox[i]-1-pos2[i] : pos2[i]; }
};
//------------------------------------------------------------------------------
template <int AID>  aiw::QpltMeshPlotter::calc_t<AID>  aiw::QpltMeshPlotter::get_flat(int flatID) const {
	const QpltMesh* cnt = dynamic_cast<const QpltMesh*>(container);
	calc_t<AID> f; (QpltFlat&)f = QpltPlotter::get_flat(flatID);  f.acc = (QpltAccessor*)&accessor;  f.interp = 0; // жуткая жуть с const--> не const?
	f.ptr0 = (char*)(cnt->mem->get_addr()); for(int i=0; i<cnt->dim; i++) f.ptr0 += cnt->mul[i]*f.spos[i];
	for(int i=0; i<2; i++){
		int a = f.axis[i], a0 = axisID[a];
		f.bbeg[i] = bbeg[a]; f.bbox[i] = bbox[a];
		f.bbox0[i] = cnt->bbox[a0];
		f.step2D[i] = float(f.bbox[i])/im_size[i]; //  cnt->step[a0];
		f.mul[i] = cnt->mul[a0]; // flips ???
		if(flips&(1<<a)){ f.ptr0 += (f.bbox[i]-1)*f.mul[i]; f.mul[i] = -f.mul[i]; }
		f.interp |= ((interp&(3<<f.axis[i]*2))>>2*f.axis[i])<<4*(1-i); // ???
	} // f.axe, f.axe_pos ???
	int a = 3-(f.axis[0]+f.axis[1]), a0 = axisID[a], a_pos = f.spos[a0]; f.mul[2] = cnt->mul[a0];  // 0+1=1-->2 || 1+2=3-->0 || 0+2-->1
	f.diff3minus = flips&(1<<a)? (a_pos+1<cnt->bbox[a0]): (a_pos>0); 
	f.diff3plus  = flips&(1<<a)? (a_pos>0): (a_pos+1<cnt->bbox[a0]);
	// WOUT(f.interp);
	return f;
}
//------------------------------------------------------------------------------
//   implemetation of plotter virtual functions
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMeshPlotter::init_impl(int autoscale){
	Vecf<2> f_lim(color.get_min(), color.get_max());
	if(autoscale&1){
		QpltMesh* cnt = (QpltMesh*)container; // ???
		Ind<6> smin, smax = cnt->bbox; int dim0 = container->get_dim();
		if(autoscale==1){ // по выделенному кубу
			smin = spos; smax = spos; for(int i=0; i<dim0; i++) smax[i]++;
			for(int i=0; i<dim; i++) smax[axisID[i]] += bbox[i]-1;
		}
		// for(int i=container->dim; i<6; i++) smax[i] = smin[i]+1; ???
		Ind<17> LID = smin|smax|AID|accessor.ctype|Ind<3>(accessor.offsets);
		auto I_LID = cnt->flimits.find(LID); 
		if(I_LID!=cnt->flimits.end()) f_lim = I_LID->second; 
		else { // считаем пределы
			bool acc_minus = accessor.minus; accessor.minus = false;
			constexpr int DIFF = (AID>>3)&7; // какой то static method в accessor?
			int ddim = accessor.Ddiff(); constexpr int dout = QpltAccessor::DOUT<AID>(); // размерность дифф. оператора  и выходных данных			
			const char* ptr = (char*)(cnt->mem->get_addr()); for(int i=0; i<dim0; i++) ptr += smin[i]*cnt->mul[i];
			float f_min = HUGE_VAL,  f_max = -HUGE_VAL; 
			Ind<6> sbox = smax-smin; size_t sz = sbox[0]; for(int i=1; i<cnt->dim; i++) sz *= sbox[i];
#pragma omp parallel for reduction(min:f_min) reduction(max:f_max) 
			for(size_t i=0; i<sz; i++){
				int pos[dim]; size_t j = i; const char *ptr1 = ptr, *nb[6] = {nullptr};   
				for(int k=0; k<dim0; k++){ pos[k] = j%sbox[k]; ptr1 += cnt->mul[k]*pos[k]; pos[k] += smin[k]; j /= sbox[k]; }					
				if(DIFF) for(int k=0; k<ddim; k++){
						int a = axisID[k];
						if(pos[a]>0) nb[2*k] = ptr1-cnt->mul[a]; else nb[2*k] = ptr1;
						if(pos[a]<cnt->bbox[a]-1) nb[2*k+1] = ptr1+cnt->mul[a]; else nb[2*k+1] = ptr1;
					}
				Vecf<3> ff; accessor.conv<AID>(ptr1, (const char**)nb, &(ff[0]));
				float fres = dout==1? ff[0]: ff.abs();	f_min = std::min(f_min, fres);  f_max = std::max(f_max, fres);
			}
			f_lim[0] = f_min;  f_lim[1] = f_max;  cnt->flimits[LID] = f_lim;
			accessor.minus = acc_minus;
		} // конец расчета пределов
	}
	if(accessor.minus) f_lim = -f_lim(1,0);
	color.reinit(f_lim[0], f_lim[1]); 
}
//------------------------------------------------------------------------------
// template <int AID> void aiw::QpltMeshPlotter::get_impl(int xy[2], QpltGetValue &res) const   // принимает координаты в пикселях
template <int AID> void aiw::QpltMeshPlotter::get_impl(int xy[2], std::string &res) const {  // принимает координаты в пикселях
	Vecf<2> r; 
	for(int fID=0, sz = flats.size(); fID<sz; fID++) if(flats[fID].image2flat(xy[0], xy[1], r)){
			calc_t<AID> f = get_flat<AID>(fID); Ind<2> pos; Vecf<2> X;
			f.mod_coord(r, pos, X); 
			// WERR(f.a[0], f.a[1], f.nX, f.nY, xy[0], xy[1], r);
			auto v = interpolate(f, pos, X, f.interp);
			std::stringstream S; S/*<<r<<':'*/<<v[0]; for(int i=1; i<v.size(); i++) S<<'\n'<<v[i];
			res = S.str(); break;
			// res.value = S.str(); for(int i=0; i<2; i++) res.xy[i] = container->fpos2coord(bbeg[f.axis[i]]+r[i], axisID[f.axis[i]]); 
			// f.flat2image(vecf(r[0], 0.f), res.a1);  f.flat2image(vecf(r[0], f.bbox[1]), res.a2);
			// f.flat2image(vecf(0.f, r[1]), res.b1);  f.flat2image(vecf(f.bbox[0], r[1]), res.b2);
		}
}
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMeshPlotter::plot_impl(std::string &res) const {
	constexpr int dout = QpltAccessor::DOUT<AID>();
	constexpr int PAID = dout==1? AID: (AID&0x7F)|(3<<6); // если итоговое поле векторное - рисуем его модуль
	QpltImage im(ibmax[0]-ibmin[0], ibmax[1]-ibmin[1]);

	if(dim==2 && flats.size()==1){ // 2D mode
		calc_t<PAID> plt = get_flat<PAID>(0);
#pragma omp parallel for 
		for(int y=0; y<im.Ny; y++){
			Ind<2> pos; Vecf<2> X; plt.mod_coord2D(1, y, pos[1], X[1]);
			for(int x=0; x<im.Nx; x++){
				plt.mod_coord2D(0, x, pos[0], X[0]);
				im.set_pixel(x, y, color(interpolate(plt, pos, X, plt.interp)[0])); // ???
			}
		}
		//im.dump2ppm("/tmp/qplt.ppm");
		if(dout>1){  // векторные поля
			calc_t<AID> plt2 = get_flat<AID>(0);
			Ind<2> N = ind(im.Nx, im.Ny), Na = N/color.arr_length/color.arr_spacing<<plt2.bbox, d = N/Na;
			// WOUT(N, d, Na, plt2.bbox, plt2.diff3minus, plt2.diff3plus, bbeg[2]);
			for(int iy=0; iy<Na[1]; iy++)  for(int ix=0; ix<Na[0]; ix++){
					int x = ix*d[0]+d[0]/2,  y = iy*d[1]+d[1]/2; Ind<2> pos; Vecf<2> X;
					plt2.mod_coord2D(0, x, pos[0], X[0]);	plt2.mod_coord2D(1, y, pos[1], X[1]);
					// if(!y) WOUT(x, pos, X);
					auto v = interpolate(plt2, pos, X, plt.interp);
					color.arr_plot(x, y, (const float*)&v, im);
					// im.set_pixel(x, y, 0xFFFFFF);
				}
		}
	} else if(mode==1) { // pseudo 3D mode
		int fl_sz = flats.size(), cID = 0; calc_t<PAID> calcs[fl_sz]; for(int i=0; i<fl_sz; i++) calcs[i] = get_flat<PAID>(i);
#pragma omp parallel for firstprivate(cID)
		for(int y=0; y<im.Ny; y++){
			int y0 = ibmin[1]+y;
			for(int x=0; x<im.Nx; x++){
				int x0 = ibmin[0]+x; Vecf<2> r;
				if(!calcs[cID].image2flat(x0, y0, r)){
					bool miss = true;
					for(int i=1; i<fl_sz; i++) if(calcs[(cID+i)%fl_sz].image2flat(x0, y0, r)){ miss = false; cID = (cID+i)%fl_sz; break; }
					if(miss){ im.set_pixel0(x, y, 0xFFFFFFFF); continue; }
				}
				Ind<2> pos; Vecf<2> X; calcs[cID].mod_coord(r, pos, X);
				im.set_pixel0(x, y, color(interpolate(calcs[cID], pos, X, calcs[cID].interp)[0]));
				// im.set_pixel0(x, y, 0xFF<<cID*8);
				
			}
		}
	} else { // real 3D mode
		int fl_sz = flats.size(), cID = 0;  VTexture vtx(*this);
		calc_t<PAID> calcs[fl_sz]; for(int i=0; i<fl_sz; i++) calcs[i] = get_flat<PAID>(i);
		int64_t deltas[3]; for(int i=0; i<3; i++) deltas[i] = icenter&(1<<(3-calcs[i].axis[0]-calcs[i].axis[1]))? -calcs[i].mul[2] : calcs[i].mul[2]; // ???
		for(int i=0; i<3; i++) WERR(i, icenter&(1<<(3-calcs[i].axis[0]-calcs[i].axis[1])), icenter, calcs[i].mul[2], deltas[i]);
		// QpltMesh* cnt = (QpltMesh*)container; // ???
		// QpltColor col("rainbow", 0, 3);
#pragma omp parallel for firstprivate(cID)
		for(int y=0; y<im.Ny; y++){
			char *nb[6] = {nullptr};  // ???
			int y0 = ibmin[1]+y;
			for(int x=0; x<im.Nx; x++){
				int x0 = ibmin[0]+x; Vecf<2> r;
				if(!calcs[cID].image2flat(x0, y0, r)){
					bool miss = true;
					for(int i=1; i<fl_sz; i++) if(calcs[(cID+i)%fl_sz].image2flat(x0, y0, r)){ miss = false; cID = (cID+i)%fl_sz; break; }
					if(miss){ im.set_pixel0(x, y, 0xFFFFFFFF); continue; }
				}
				const calc_t<PAID> & flat = calcs[cID];
				Ind<2> pos;  Vecf<2> X; flat.mod_coord(r, pos, X);  const char* ptr = flat.get_ptr(pos);
				Ind<3> pos3d; flat.pos2to3(pos, pos3d); 
				QpltColor::rgb_t C;  auto ray = vtx.trace(cID, X);
				float sum_w = 0, lim_w = .01*D3opacity, _max_len = 1.f/(1+(bbox.max()-1)*(1-.01f*D3density)); 

				// im.set_pixel0(x, y, col(ray.fID+ray.gID*3));
				// im.set_pixel0(x, y, col(ray.len));

				/* */
				while(1){
					WASSERT(Ind<3>()<= pos3d && pos3d<bbox, "incorrect pos3d", x, y, r, pos, X, pos3d, bbox, cID, ray.fID, ray.gID, ray.f, ray.g, ray.len); 
					// WEXT(pos, pos3d, bbox, x, y, cID, sum_w, ray.fID, ray.gID, ptr-cnt->ptr); // std::cerr.flush();
					// if(cID==1 && x==526 && y==160){ WERR(cID,  pos3d, ray.fID, ray.gID, sum_w, ptr-cnt->ptr); std::cerr.flush(); }
					float f; accessor.conv<PAID>(ptr, (const char**)nb, &f);
					if(color.check_in(f)){
						float w = ray.len*_max_len*(1-sum_w);
						if(sum_w+w<lim_w){ C = C + QpltColor::rgb_t(color(f)).inv()*w; sum_w += w; }
						else { C = C + QpltColor::rgb_t(color(f)).inv()*(lim_w-sum_w); break; }
						// v1:
						// if(sum_w+w<lim_w){ C = C*(1-w) + QpltColor::rgb_t(color(f))*w; sum_w += w; }
						// else { C = C*(1-w) + QpltColor::rgb_t(color(f))*(lim_w-sum_w); break; }
					}
					if(++pos3d[ray.gID]>=bbox[ray.gID]) break;  // переходим в следующий воксель, проверяем границу
					ptr += deltas[ray.gID];	 				
					ray.next();  
				}
				im.set_pixel0(x, y, sum_w? 0xFFFFFFFF-C.I: 0xFFFFFFFF);
				// im.set_pixel0(x, y, sum_w? C.I: 0xFFFFFFFF);  // v1
				// im.set_pixel0(x, y, col(len));
				/* */
				// im.set_pixel0(x, y, 0xFF<<cID*8);
				
			}
		}
		WERR("OK");
	}
	// im.dump2ppm("1.ppm");
	/*
		float w=col.w*density*(1.0f - sum.w); col.w = 1;
	    if(sum.w + w < opacityThreshold) sum += col * w;
	    else { sum += col * (opacityThreshold-sum.w); break; }
		т.е. sum += col * w; пока не достигнут порог непрозрачности.
		float4 col; -- это цвет текущей точки:
		в простейшем варианте
		col = im.get_color_for3D(tex3D(data3D_tex, pos_sc.x, pos_sc.y, pos_sc.z));
		
		по умолчанию
		density = 0.5; opacity = 0.95;
		можно менять, opacity = 0..1; density > 0;
	*/	
	res = im.buf;
}
//------------------------------------------------------------------------------

