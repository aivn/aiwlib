/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/view/images"
#include "../../include/aiwlib/view/umesh3D"
#include "../../include/aiwlib/binary_format"

using namespace aiw;

//------------------------------------------------------------------------------
void aiw::UnorderedMesh3DHead::cell_t::init(const std::vector<Vec<3> > &coord_table){
	for(int i=0; i<4; i++) coords[i] = &coord_table[nodes[i]];
	bmin = bmax = *coords[0]; for(int i=1; i<4; i++){ bmin <<= *coords[i];  bmax >>= *coords[i]; }
	for(int i=0; i<4; i++){
		_h[i] = (*coords[(i+2)%4]-*coords[(i+1)%4])%(*coords[(i+3)%4]-*coords[(i+1)%4]);
		if(_h[i]*(*coords[i]-*coords[(i+1)%4])<0) _h[i] = -_h[i];
		_h[i] /= _h[i]*(*coords[i]-*coords[(i+1)%4]);
	}
}
//------------------------------------------------------------------------------
bool aiw::UnorderedMesh3DHead::load(IOstream &S){
	BinaryFormat bf; bf.D = 3|(1<<29); bf.R = -1; bf.axes = anames; int box[3] = {0}; bf.box = box; if(!bf.load(S)) return false; 
	head = bf.head; nodes.resize(box[0]); cells.resize(box[1]); faces_sz = box[2];
	uint64_t phys_sz = 0; S>phys_sz; phys.resize(phys_sz*bf.R); S.read(phys.data(), phys.size());
	S.read(nodes.data(), nodes.size()*sizeof(Vec<3>));
	for(cell_t &cell: cells){ int pID = 0; S>pID>cell.nodes>cell.faces; cell.phys = phys.data()+bf.R*pID; cell.init(nodes); }
	S.seek(faces_sz*20, 1);  // таблицу инцендентности для граней пропускаем (3 узла и две ячейки)

	bmin = bmax = nodes[0]; for(const Vec<3> &node: nodes){ bmin <<= node; bmax >>= node; }
	cfa_list = bf.tinfo.get_access();  cfa_xfem_list = bf.tinfo.get_xfem_access();
	return true;
}
//------------------------------------------------------------------------------
bool aiw::UnorderedMesh3DView::load(IOstream &S, const UnorderedMesh3DHead &mesh_){
	mesh = &mesh_;  size_t mem_sz = ~size_t(0); S>head>mem_sz; if(mem_sz==~size_t(0)) return false;
	size_t s = S.tell(); mem = S.mmap(mem_sz, 0); if(S.tell()!=long(s+mem_sz)) return false;
	nodes = (const int*)(mem->get_addr());
	cells = nodes+mesh->nodes.size();
	faces = cells+mesh->cells.size();
	data = (const char*)(faces+mesh->faces_sz+1);

	// for(int i=0; i<mesh->nodes.size(); i++) WOUT(i, mesh->nodes[i], *(double*)(data+28*i+4), *(double*)(data+28*i+12), *(double*)(data+28*i+20));
	// for(int i=0; i<mesh->nodes.size(); i++) WOUT(i, mesh->nodes[i], *(double*)(data+nodes[i]+4), *(double*)(data+nodes[i]+12), *(double*)(data+nodes[i]+20));
	return true;
}
//------------------------------------------------------------------------------
void aiw::UnorderedMesh3DView::get_conf(ConfView &conf, bool firstcall) const {  // настраивает conf (с учетом crop)
	conf.dim = 3; // че делать если пришел dim!=D?!
	for(int i=0; i<3; i++){
		if(firstcall || (conf.bmin[i]==conf.bmin0[i] && conf.bmax[i]==conf.bmax0[i])){ conf.bmin[i] = mesh->bmin[i]; conf.bmax[i] = mesh->bmax[i]; }
		if(firstcall) conf.slice[i] = conf.bmin[i] = mesh->bmin[i];
		conf.bmin0[i] = mesh->bmin[i]; conf.bmax0[i] = mesh->bmax[i]; conf.step[i] = (mesh->bmax[i]-mesh->bmin[i])/999; conf.size[i] = 999;
	}
	conf.mod_crop = false;
	conf.crop(vec(0.,0.), vec(1.,1.));  // для приведения к границам ячеек

	for(int i=0; i<3; i++) if(mesh->anames[i].size()) conf.anames[i] = mesh->anames[i]; else conf.anames[i] = "XYZ"[i];

	conf.features =  ConfView::opt_axes|ConfView::opt_flip|ConfView::opt_crop|ConfView::opt_cell_bound;
	conf.cfa_list = mesh->cfa_list;  conf.cfa_xfem_list = mesh->cfa_xfem_list;
}
//------------------------------------------------------------------------------
void set_ff(Vec<2> &ff, float f, bool &first){
	if(first && !::isnan(f)){ ff[0] = ff[1] = f; first = false; }
	else{
		if(ff[0]>f) ff[0] = f;
		if(ff[1]<f) ff[1] = f;
	}
}
//------------------------------------------------------------------------------
Vec<2> aiw::UnorderedMesh3DView::f_min_max(const ConfView &conf) const { // вычисляет min-max, как это делать для preview?
#ifdef EBUG
	double t0 = omp_get_wtime();
#endif //EBUG
	int si; for(si=0; si<3; si++) if(si!=conf.axes[0] && si!=conf.axes[1]) break; // номер оси по которой строится срез
	double s = conf.slice[si]; bool first = true; Vec<2> ff; float w[4] = {0, 0, 0, 0};
	Vec<2> min_xy = conf.bmin(conf.axes), max_xy = conf.bmax(conf.axes);
	for(int cid=0; cid<int(mesh->cells.size()); cid++){
		const UnorderedMesh3DHead::cell_t &cell = mesh->cells[cid];
		if(cell.bmin[si]<=s && s<=cell.bmax[si] && min_xy<=cell.bmax(conf.axes) && cell.bmin(conf.axes)<=max_xy){
			if(conf.xfem_mode==0) for(int i=0; i<4; i++) set_ff(ff, conf.cfa.get_f(data+nodes[cell.nodes[i]]), first);
			else if(conf.xfem_mode==1) set_ff(ff, conf.cfa.get_f(data+cells[cid]), first);
			else if(conf.xfem_mode==3) set_ff(ff, conf.cfa.get_f(cell.phys), first);
			else { int i=0; for(int j=1; j<4; j++){ if(w[i]>w[j]) i = j; } set_ff(ff, conf.cfa.get_f(data+faces[cell.faces[i]]), first); } // ищем ближайшую к r грань
		}
	}
	WOUT(omp_get_wtime()-t0);
	return ff;
}
//------------------------------------------------------------------------------
float aiw::UnorderedMesh3DView::get(const ConfView &conf, Vec<2> r) const {
	int si; for(si=0; si<3; si++) if(si!=conf.axes[0] && si!=conf.axes[1]) break; // номер оси по которой строится срез
	Vec<3> r3; r3[si] = conf.slice[si]; for(int i=0; i<2; i++) r3[conf.axes[i]] = r[i];
	float w[4];
	for(int cid=0; cid<int(mesh->cells.size()); cid++){
		const UnorderedMesh3DHead::cell_t &cell = mesh->cells[cid];
		if(cell.bmin[si]<=r3[si] && r3[si]<=cell.bmax[si] && cell.weights(r3, w)){
			float f = 0;
			if(conf.xfem_mode==0) for(int i=0; i<4; i++) f += w[i]*conf.cfa.get_f(data+nodes[cell.nodes[i]]);
			else if(conf.xfem_mode==1) f = conf.cfa.get_f(data+cells[cid]);
			else if(conf.xfem_mode==3) f = conf.cfa.get_f(cell.phys);
			else { int i=0; for(int j=1; j<4; j++){ if(w[i]>w[j]) i = j; } f = conf.cfa.get_f(data+faces[cell.faces[i]]); } // ищем ближайшую к r грань
			return f;
		}
	}
	return nanf("");
}
//------------------------------------------------------------------------------
