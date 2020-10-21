/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/view/images"
#include "../../include/aiwlib/view/mesh"
// #include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/segy"

using namespace aiw;
//------------------------------------------------------------------------------
bool aiw::MeshView::load(IOstream &S){
	size_t s = S.tell(); BinaryFormat bf; bf.init(*this); bf.D = -1;
	if(!bf.load(S) || bf.D&(0xFFFF<<16) || !(bf.D&0xFF)){ S.seek(s); return false; }
	head = bf.head; D = bf.D; szT = bf.szT; logscale = bf.logscale; 
	sz = 1; for(int i=0; i<D; i++) sz *= box[i];
	// WOUT(head, D, szT, sz, logscale, box, bmin, bmax, s, S.tell());
	
	mem = S.mmap(sz*szT, 0);
	// WOUT(S.tell());
 
	s = S.tell(); int32_t sz2 = 0; S.load(sz2);  // try read old aivlib mesh format (deprecated)
	if(S.tell()-s==4 && sz2==-int(D*24+4+szT)){ S.read(&bmin, D*8); S.read(&bmax, D*8); S.read(&step, D*8); S.seek(szT, 1); logscale = 0;  } 
	else  S.seek(s); 
	for(int i=0; i<D; i++) this->set_step(i);
	// for(auto i: bf.tinfo.get_access()) std::cout<<i.label<<' '<<i.offset<<'\n';
	// std::cout<<bf.tinfo;
#ifdef AIW_TYPEINFO
	cfa_list = bf.tinfo.get_access();
#endif //AIW_TYPEINFO
	segy = false;
	return true;
}
//------------------------------------------------------------------------------
//bool aiw::MeshView::load_as_sgy(IOstream &S){ if(load(S)){ sgy = true; return true; } else return false; }
bool aiw::MeshView::load_from_segy(IOstream &S){
	Mesh<float, 3> data; Mesh<float, 3> geometry = segy_read(S, data);
	D = 3; szT = 4; sz = 1; for(int i=0; i<D; i++){ box[i] = data.bbox()[i]; sz *= box[i]; }
	bmin[0] = 0; bmax[0] = geometry[ind(6,0,0)]*data.bbox()[0]; this->set_step(0);
	for(int i=0; i<2; i++){
		bmin[i+1] = (geometry[ind(i,0,0)]+geometry[ind(3+i,0,0)])/2;
		bmax[i+1] = (geometry[i|(geometry.bbox()(1,2)-ind(1))]+geometry[(3+i)|(geometry.bbox()(1,2)-ind(1))])/2;
		this->set_step(i+1);
	}
	mem = data.mem;
	segy = true;
	return true;
}
//------------------------------------------------------------------------------
void aiw::MeshView::get_conf(ConfView &conf, bool firstcall) const {  // настраивает conf (с учетом crop)
	conf.dim = D; // че делать если пришел dim!=D?!
	for(int i=0; i<D; i++){
		if(firstcall || (conf.bmin[i]==conf.bmin0[i] && conf.bmax[i]==conf.bmax0[i])){ conf.bmin[i] = bmin[i]; conf.bmax[i] = bmax[i]; }
		if(firstcall) conf.slice[i] = conf.bmin[i] = bmin[i];
		else {
			if((conf.slice[i]<bmin[i] && bmin[i]<=bmax[i]) || (conf.slice[i]>bmin[i] && bmin[i]>bmax[i])) conf.slice[i] = bmin[i];
			if((conf.slice[i]>bmax[i] && bmin[i]<=bmax[i]) || (conf.slice[i]<bmax[i] && bmin[i]>bmax[i])) conf.slice[i] = bmax[i];
		}
		conf.bmin0[i] = bmin[i]; conf.bmax0[i] = bmax[i]; conf.step[i] = step[i]; conf.size[i] = box[i];
	}
	conf.logscale = logscale; conf.mod_crop = true;
	conf.crop(vec(0.,0.), vec(1.,1.));  // для приведения к границам ячеек
	// if(segy && firstcall){ conf.axes = ind(1,0); conf.set_flip(0, true); conf.segy = true; }

	for(int i=0; i<D; i++) if(anames[i].size()) conf.anames[i] = anames[i]; else conf.anames[i] = "XYZABCDEFGHIJKLM"[i];

	conf.features =  ConfView::opt_axes|ConfView::opt_flip|ConfView::opt_crop|ConfView::opt_cell_bound|ConfView::opt_interp|ConfView::opt_step_size|
		ConfView::opt_segy|ConfView::opt_3D;
	conf.cfa_list = cfa_list;  conf.cfa_xfem_list.clear();
	if(cfa_list.size()==1) conf.cfa = cfa_list[0];
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
	if(conf.segy && !data.segy && conf.axes[1]==0 && conf.get_flip(1)){ powZ2 = true; Z0 = data.box[0]; }
}
//------------------------------------------------------------------------------
std::string aiw::MeshView::get(const ConfView& conf, aiw::Vec<2> r) const {
	char buf[1024]; double x  = access_t(*this, conf)(r);
	snprintf(buf, 1023, "%g\n[%i,%i]", x, coord2pos(r[0], conf.axes[0]), coord2pos(r[1], conf.axes[1]));
	return buf;
}
//------------------------------------------------------------------------------
Vec<2> aiw::MeshView::f_min_max(const ConfView &conf) const { // вычисляет min-max, как это делать для preview?
#ifdef EBUG
	double t0 = omp_get_wtime();
#endif //EBUG
	access_t access(*this, conf);
	// WOUT(access.cfa.typeID, access.cfa.offset);
	float f_min = 0, f_max = 0; int xsz = access.box[0], ysz = access.box[1];
	if(access.powZ2){
		if(abs(access.mul[0])<abs(access.mul[1])){		
#pragma omp parallel for reduction(+:f_min,f_max)
			for(int y=0; y<ysz; y++) for(int x=0; x<xsz; x++){
					float f = conf.cfa.get_f(access.ptr+access.mul[0]*x+access.mul[1]*y)*(access.Z0-y)*(access.Z0-y);
					if(f<0) f_min += f*f; 
					if(f>0) f_max += f*f; 
				}
		} else {
#pragma omp parallel for reduction(+:f_min,f_max)
			for(int x=0; x<xsz; x++) for(int y=0; y<ysz; y++){
					float f = conf.cfa.get_f(access.ptr+access.mul[0]*x+access.mul[1]*y)*(access.Z0-y)*(access.Z0-y);
					if(f<0) f_min += f*f; 
					if(f>0) f_max += f*f; 
				}
		}
		f_min = -3*sqrt(f_min/(xsz*ysz)); f_max = 3*sqrt(f_max/(xsz*ysz));
	} else {
		f_min = f_max = access[ind(0,0)]; 
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
	}	
	WOUT(omp_get_wtime()-t0);
	return Vec<2>(f_min, f_max);
}
//------------------------------------------------------------------------------
