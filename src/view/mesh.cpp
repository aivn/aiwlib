/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/view/images"
#include "../../include/aiwlib/view/mesh"
#include "../../include/aiwlib/binary_format"

using namespace aiw;
//------------------------------------------------------------------------------
bool aiw::MeshView::load(IOstream &S){
	BinaryFormat bf; bf.init(*this); bf.D = -1; if(!bf.load(S)) return false;
	head = bf.head; D = bf.D; szT = bf.szT; logscale = bf.logscale;  size_t s = S.tell(); 
	sz = 1; for(int i=0; i<D; i++) sz *= box[i];
	mem = S.mmap(sz*szT, 0);
 
	s = S.tell(); int32_t sz2 = 0; S>sz2;  // try read old aivlib mesh format (deprecated)
	if(S.tell()-s==4 && sz2==-int(D*24+4+szT)){ S.read(&bmin, D*8); S.read(&bmax, D*8); S.read(&step, D*8); S.seek(szT, 1); logscale = 0;  } 
	else  S.seek(s); 
	for(int i=0; i<D; i++) this->set_step(i);

	// for(auto i: bf.tinfo.get_access()) std::cout<<i.label<<' '<<i.offset<<'\n';
	cfa_list = bf.tinfo.get_access();
	sgy = false;	
	return true;
}

bool aiw::MeshView::load_as_sgy(IOstream &S){ if(load(S)){ sgy = true; return true; } else return false; }
bool aiw::MeshView::load_from_sgy(IOstream &S){ return false; }
//------------------------------------------------------------------------------
void aiw::MeshView::get_conf(ConfView &conf, bool firstcall) const {  // настраивает conf (с учетом crop)
	conf.dim = D; // че делать если пришел dim!=D?!
	for(int i=0; i<D; i++){
		if(firstcall || (conf.bmin[i]==conf.bmin0[i] && conf.bmax[i]==conf.bmax0[i])){ conf.slice[i] = conf.bmin[i] = bmin[i]; conf.bmax[i] = bmax[i]; }
		conf.bmin0[i] = bmin[i]; conf.bmax0[i] = bmax[i]; conf.step[i] = step[i]; conf.size[i] = box[i];
	}
	conf.logscale = logscale; conf.mod_crop = true;
	conf.crop(vec(0.,0.), vec(1.,1.));  // для приведения к границам ячеек
	if(sgy && firstcall){ conf.axes = ind(1,0); conf.set_flip(0, true); }

	conf.anames[0] = "X";
	conf.anames[1] = "Y";
	conf.anames[2] = "Z";

	conf.features =  ConfView::opt_axes|ConfView::opt_flip|ConfView::opt_crop|ConfView::opt_cell_bound|ConfView::opt_interp|ConfView::opt_step_size;
	conf.cfa_list = cfa_list;
}
//------------------------------------------------------------------------------
aiw::MeshView::access_t::access_t(const MeshView &data, const ConfView &conf){
	ptr = (const char*)data.mem->get_addr();
	bmin = conf.bmin(conf.axes); step = data.step(conf.axes); rstep = data.rstep(conf.axes); // box = data.box(conf.axes);
	logscale = bool(data.logscale&(1<<conf.axes[0]))+2*bool(data.logscale&(1<<conf.axes[1]));
	interp = (conf.get_interp(0)<<4)|conf.get_interp(1);
	double r[data.D]; for(int i=0; i<data.D; i++) r[i] = conf.slice[i];
	for(int i=0; i<2; i++) r[conf.axes[i]] = conf.bmin[conf.axes[i]];
	int pos[data.D]; for(int i=0; i<data.D; i++) pos[i] = data.coord2pos(r[i], i); 
	int mul_[data.D];  mul_[0] = data.szT; ptr += mul_[0]*pos[0];
	for(int i=1; i<data.D; i++){ mul_[i] = mul_[i-1]*data.box[i-1]; ptr += mul_[i]*pos[i]; }
	for(int i=0; i<2; i++){
		int a = conf.axes[i]; mul[i] = mul_[a];
		box[i] = data.coord2pos(conf.bmax[a], a)-pos[a]; // +.5 ???
		if(conf.flipped&1<<a){
			ptr += mul[i]*(box[i]-1); mul[i] = -mul[i]; bmin[i] = conf.bmax[a];
			if(logscale&1<<i){ step[i] = 1./step[i]; rstep[i] = 1./log(step[i]); }
			else { step[i] = -step[i]; rstep[i] = -rstep[i]; }
		}
	}
	cfa = conf.cfa;
}
//------------------------------------------------------------------------------
Vec<2> aiw::MeshView::f_min_max(const ConfView &conf) const { // вычисляет min-max, как это делать для preview?
	double t0 = omp_get_wtime();
	access_t access(*this, conf);
	float f_min = access[ind(0,0)], f_max = f_min; int xsz = access.box[0], ysz = access.box[1];
	if(abs(access.mul[0])<abs(access.mul[1])){		
#pragma omp parallel for reduction(min:f_min) reduction(max:f_max)
		for(int y=0; y<ysz; y++) for(int x=0; x<xsz; x++){
				float f = conf.cfa.get_f(access.ptr+access.mul[0]*x+access.mul[1]*y);
				if(f_min>f) f_min = f;
				if(f_max<f) f_max = f;
			}
	} else {
#pragma omp parallel for reduction(min:f_min) reduction(max:f_max)
		for(int x=0; x<xsz; x++) for(int y=0; y<ysz; y++){
				float f = conf.cfa.get_f(access.ptr+access.mul[0]*x+access.mul[1]*y);
				if(f_min>f) f_min = f;
				if(f_max<f) f_max = f;
			}
	}
	WOUT(omp_get_wtime()-t0);
	return Vec<2>(f_min, f_max);
}
//------------------------------------------------------------------------------
