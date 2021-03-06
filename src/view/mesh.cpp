/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

// #include "../../include/aiwlib/view/images"
#include "../../include/aiwlib/view/mesh"
// #include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/segy"

	
using namespace aiw;
// File logfile("uplt.log", "w");
//------------------------------------------------------------------------------
std::list<BaseView*> aiw::BaseView::view_table;
double aiw::BaseView::max_mem_Gsz = 4; // максимально допустимый суммарный размер данных в гигабайтах
//------------------------------------------------------------------------------
void aiw::BaseView::mem_load_prepare(){   // расчищает память за счет давно неиспользуемых объектов
	// printf(">>> %p %i %gG lim %gG\n", this, int(view_table.size()), mem_sz*1e-9, max_mem_Gsz);
	double tot_mem_Gsz = mem_sz*1e-9;
	for(auto p: view_table){ if(p==this) return; else tot_mem_Gsz += p->mem_sz*1e-9; }
	while(!view_table.empty() && tot_mem_Gsz>max_mem_Gsz){
		// printf("pop %p %gG\n", view_table.back(), view_table.back()->mem_sz*1e-9);
		tot_mem_Gsz -= view_table.back()->mem_sz*1e-9;
		view_table.back()->mem_free(); view_table.pop_back();
	}
	view_table.push_front(this);
}
//------------------------------------------------------------------------------
bool aiw::MeshView::load(IOstream &S){
	size_t s = S.tell(); BinaryFormat bf; bf.init(*this); bf.D = -1;
	if(!bf.load(S) || bf.D&(0xFFFF<<16) || !(bf.D&0xFF)){ S.seek(s); return false; }
	head = bf.head; D = bf.D; szT = bf.szT; logscale = bf.logscale; 
	sz = 1; for(int i=0; i<D; i++) sz *= box[i];
	// WOUT(head, D, szT, sz, logscale, box, bmin, bmax, s, S.tell());
	// mem = S.mmap(sz*szT, 0);
	fin = S.copy(); mem_offset = S.tell(); mem_sz = sz*szT; S.seek(mem_sz, 1);  mem.reset();
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
	segy = true; fin = S.copy(); mem_offset = S.tell();
	mem_load_prepare();  // мы пока не знаем размер данных?
	Mesh<float, 3> data; Mesh<float, 3> geometry = segy_read(S, data);
	D = 3; szT = 4; sz = 1; for(int i=0; i<D; i++){ box[i] = data.bbox()[i]; sz *= box[i]; }
	mem = data.mem; mem_sz = sz*4;
	bmin[0] = 0; bmax[0] = geometry[ind(6,0,0)]*data.bbox()[0]; this->set_step(0);
	for(int i=0; i<2; i++){
		bmin[i+1] = (geometry[ind(i,0,0)]+geometry[ind(3+i,0,0)])/2;
		bmax[i+1] = (geometry[i|(geometry.bbox()(1,2)-ind(1))]+geometry[(3+i)|(geometry.bbox()(1,2)-ind(1))])/2;
		this->set_step(i+1);
	}
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
std::string aiw::MeshView::get(const ConfView& conf, aiw::Vec<2> r){
	mem_load();
	char buf[1024]; double x  = access_t(*this, conf)(r);
	snprintf(buf, 1023, "%g\n[%i,%i]", x, coord2pos(r[0], conf.axes[0]), coord2pos(r[1], conf.axes[1]));
	return buf;
}
//------------------------------------------------------------------------------
Vec<2> aiw::MeshView::f_min_max(const ConfView &conf){ // вычисляет min-max, как это делать для preview?
	mem_load(); access_t access(*this, conf);
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
		// logfile.printf("%g %g %g %i\n", f_min, f_max, *(const float*)(access.ptr), conf.cfa.typeID);
	}	
	return Vec<2>(f_min, f_max);
}
//------------------------------------------------------------------------------
aiw::Vec<2> aiw::MeshView::f_min_max_tot(const ConfView &conf){
	mem_load();  const char *ptr = (const char*)(mem->get_addr());
	float f_min = 0, f_max = 0; 
#pragma omp parallel for reduction(min:f_min) reduction(max:f_max)
	for(size_t i=0; i<sz; i++){
		float f = conf.cfa.get_f(ptr+i*szT);
		if(f_min>f) f_min = f; 
		if(f_max<f) f_max = f; 
	}
	return Vec<2>(f_min, f_max);
}
//------------------------------------------------------------------------------
void aiw::MeshView::plot(const ConfView &conf, Image &image, const CalcColor &color){
	mem_load();
	// double t0 = omp_get_wtime();
	// WOUT(conf.axes);
	// WOUT(conf.cfa.typeID, conf.cfa.offset, conf.cfa.label, szT);
	for(int i=0; i<D; i++) if(i!=conf.axes[0] && i!=conf.axes[1] && (conf.bmax[i]<bmin[i] || bmax[i]<conf.bmin[i])) return;
	
	int xsz = image.size[0], ysz = image.size[1];
	Vec<2> im_step = (conf.max_xy()-conf.min_xy())/image.size; Vec<2> im_bmin = conf.min_xy() + .5*im_step;

	/*
	  if(conf.plot3D){
	  VTexture vtx = conf.vtx;
	  vtx.theta = M_PI/6; vtx.phi = M_PI/3;
	  vtx.image_sz = image.size;
	  vtx.box = box(0, 1, 2);
	  vtx.init();
	  const char* ptr = (const char*)(mem->get_addr());
#pragma omp parallel for 
for(int y=0; y<ysz; y++){
for(int x=0; x<xsz; x++){
auto I = vtx.trace(ind(x, y));
						if(!I){ image.set_pixel(ind(x,y), Ind<3>(255)); continue; }
						Vecf<3> c; float l = 0.;
						while(I && l<10){
						float f = conf.cfa.get_f(ptr+I.data*szT);
							if(color.min<=f && f<=color.max){
								float dl = I.len();
								if(dl){
									float w = conf.alpha*dl/(l+dl);
									c = c*(1-w)+w*color(f);
									l += dl;
								}
							}
							++I;
						}
						image.set_pixel(ind(x,y), Ind<3>(c.round())<<ind(255));
					}
				}				
			} else
*/
	{ // plot 2D
		access_t f(*this, conf); 
		//WOUT(f.cfa.typeID, f.cfa.offset);
		// обходим изображение всегда "по шерсти", выигрыш от openMP примерно вдвое
		// int S = 0;
#pragma omp parallel for // reduction(+:S) 
		for(int y=0; y<ysz; y++){
			Ind<2> pos; Vec<2> X, r; r[1] = im_bmin[1]+y*im_step[1];
			if(r[1]<bmin[conf.axes[1]] || bmax[conf.axes[1]]<r[1]) continue; // ???
			f.mod_coord(1, r[1], pos[1], X[1]);
			for(int x=0; x<xsz; x++){
				r[0] = im_bmin[0]+x*im_step[0];
				if(r[0]<bmin[conf.axes[0]] || bmax[conf.axes[0]]<r[0]) continue; // ???
				f.mod_coord(0, r[0], pos[0], X[0]);
				image.set_pixel(ind(x,y), color(interpolate(f, pos, X, f.interp)));
				// S += color(interpolate(f, pos, X, f.interp)).sum();
			}
		}
		// printf(">>> %g\n", omp_get_wtime()-t0);
	}
	// printf("MeshView::plot image=%ix%i, mesh=%ix%i, plottime=%g\n", image.size[0], image.size[1], f.box[0], f.box[1], omp_get_wtime()-t0);
	// cell_bound ???
}
//------------------------------------------------------------------------------
void aiw::MeshView::preview(const ConfView& conf0, Image &image, const CalcColor &color){  // рассчитан на маленькое изображение, учитывает только flip
	ConfView conf = conf0; conf.uncrop();
	plot(conf, image, color);
}
//------------------------------------------------------------------------------
void aiw::MeshView::get_line(const ConfView &conf, aiw::Vec<2> point, int axe, std::vector<float>& X, std::vector<float>& Y){
	mem_load(); 
	Ind<16> pos = coord2pos(conf.slice); for(int i=0; i<2; i++) pos[conf.axes[i]] = coord2pos(point[i], conf.axes[i]);
	X.resize(box[axe]); Y.resize(box[axe]);  const char *ptr = (const char*)(mem->get_addr());
	size_t mul = szT; for(int i=0; i<D; i++){ if(i!=axe){ ptr += mul*pos[i]; } mul *= box[i]; }
	mul = szT; for(int i=0; i<axe; i++) mul *= box[i];
	for(int i=0; i<box[axe]; i++){
		X[i] = pos2coord(i, axe);
		Y[i] = conf.cfa.get_f(ptr+i*mul);
	}
};
//------------------------------------------------------------------------------
