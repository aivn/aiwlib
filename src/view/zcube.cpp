/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/view/images"
#include "../../include/aiwlib/view/zcube"
#include "../../include/aiwlib/binary_format"

using namespace aiw;
//------------------------------------------------------------------------------
bool aiw::ZCubeView::load(IOstream &S){
	size_t s = S.tell(); BinaryFormat bf; bf.init((BaseMesh<16>&)*this); bf.D = -1;  bf.R = -1;  bf.box = nullptr;
	if(!bf.load(S) || (bf.D>>16)!=(1<<14)){ S.seek(s); return false; }
	head = bf.head; D = bf.D&0xFF; R = bf.R; szT = bf.szT; logscale = bf.logscale;
	for(int i=0; i<D; i++) box[i] = 1<<R; 
	sz = uint64_t(1)<<(R*D);  mem = S.mmap(sz*szT, 0);
	for(int i=0; i<D; i++) this->set_step(i);
 
	cfa_list = bf.tinfo.get_access();
	return true;
}
//------------------------------------------------------------------------------
void aiw::ZCubeView::get_conf(ConfView &conf, bool firstcall) const {  // настраивает conf (с учетом crop)
	conf.dim = D; // че делать если пришел dim!=D?!
	for(int i=0; i<D; i++){
		if(firstcall || (conf.bmin[i]==conf.bmin0[i] && conf.bmax[i]==conf.bmax0[i])){ conf.bmin[i] = bmin[i]; conf.bmax[i] = bmax[i]; }
		if(firstcall) conf.slice[i] = conf.bmin[i] = bmin[i];
		conf.bmin0[i] = bmin[i]; conf.bmax0[i] = bmax[i]; conf.step[i] = step[i]; conf.size[i] = box[i];
	}
	conf.logscale = logscale; conf.mod_crop = true;
	conf.crop(vec(0.,0.), vec(1.,1.));  // для приведения к границам ячеек

	for(int i=0; i<D; i++) if(anames[i].size()) conf.anames[i] = anames[i]; else conf.anames[i] = "XYZABCDEFGHIJKLM"[i];

	conf.features =  ConfView::opt_axes|ConfView::opt_flip|ConfView::opt_crop|ConfView::opt_cell_bound|ConfView::opt_interp|ConfView::opt_step_size;
	conf.cfa_list = cfa_list;    conf.cfa_xfem_list.clear();
}
//------------------------------------------------------------------------------
aiw::ZCubeView::access_t::access_t(const ZCubeView &data, const ConfView &conf){
	ptr = (const char*)data.mem->get_addr(); szT = data.szT; D = data.D; R = data.R; axes = conf.axes;
	bmin = conf.bmin(conf.axes); step = data.step(conf.axes); rstep = data.rstep(conf.axes);  
	logscale = bool(data.logscale&(1<<conf.axes[0]))+2*bool(data.logscale&(1<<conf.axes[1]));
	interp = (conf.get_interp(0)<<4)|conf.get_interp(1);
	double r[data.D]; for(int i=0; i<data.D; i++) r[i] = conf.slice[i];
	for(int i=0; i<2; i++) r[axes[i]] = bmin[i];	
	int pos[data.D]; for(int i=0; i<data.D; i++) pos[i] = data.coord2pos(r[i], i); // позиция нижнего левого угла
	for(int i=0; i<2; i++){
		int a = axes[i]; off[i] = pos[a];  box[i] = data.coord2pos(conf.bmax[a], a)-pos[a]; // +.5 ???
		if(conf.flipped&1<<a){
			flip[i] = -1; off[i] += box[i]-1; bmin[i] = conf.bmax[a];
			if(logscale&1<<i){ step[i] = 1./step[i]; rstep[i] = 1./log(step[i]); }
			else { step[i] = -step[i]; rstep[i] = -rstep[i]; }
		} else flip[i] = 1;
	}
	for(int i=0; i<D; i++){
		if(i==axes[0] || i==axes[1]) continue;
		ptr += (interleave_bits(D, pos[i], R)<<i)*szT;
	}
	cfa = conf.cfa;
}
//------------------------------------------------------------------------------
std::string aiw::ZCubeView::get(const ConfView& conf, aiw::Vec<2> r) const {
	char buf[1024]; Ind<2> pos; double x  = access_t(*this, conf)(r, &pos);
	snprintf(buf, 1023, "%g\n[%i,%i]", x, pos[0], pos[1]);
	return buf;
}
//------------------------------------------------------------------------------
Vec<2> aiw::ZCubeView::f_min_max(const ConfView &conf) const { // вычисляет min-max, как это делать для preview?
#ifdef EBUG
	double t0 = omp_get_wtime();
#endif //EBUG
	access_t access(*this, conf);
	// WOUT(mem->get_addr(), (void*)(access.ptr), access.D, access.R, access.szT, access.flip[0], access.flip[1]);
	// WOUT(access.box, access.off, access.axes, access.bmin, access.step, access.rstep);

	float f_min = access[ind(0,0)], f_max = f_min; int xsz = access.box[0], ysz = access.box[1];
	// #pragma omp parallel for reduction(min:f_min) reduction(max:f_max)
	for(int y=0; y<ysz; y++) for(int x=0; x<xsz; x++){
			float f = access[ind(x, y)];
			if(f_min>f) f_min = f;
			if(f_max<f) f_max = f;
		}
	WOUT(omp_get_wtime()-t0);
	return Vec<2>(f_min, f_max);
}
//------------------------------------------------------------------------------
