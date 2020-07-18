/**
 * Copyright (C) 2019-20 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/view/amr"
using namespace aiw;


//------------------------------------------------------------------------------
aiw::AdaptiveMeshView::iterator aiw::AdaptiveMeshView::find(const ConfView &conf, aiw::Ind<2> pos) const {  // произвольный доступ по срезу
	if(!(Ind<2>()<=pos && pos<conf.size(conf.axes))) return iterator();
	iterator I; I.msh = this; I.conf = &conf; I.mask = I.offset = 0;  I.axes[0] = conf.axes[0]; I.axes[1] = conf.axes[1];
	// ищем тайл сетки нулевого ранга T0 и позицию P внутри него (на самой мелкой сетке)
	int P[D]; int T0 = 0, s = 1, N = 1<<(R+max_rank); // смещение на большой сетке, размер тайла нулевого ранга на мелкой сетке
	uint64_t offset0 = 0, mask0 = 0, maskRD = (~uint64_t(0))>>(64-R*D), maskD = (~uint64_t(0))>>(64-D); // смещение на мелкой сетке, R*D бит и D бит
	for(int i=0; i<D; i++) P[i] = coord2pos(conf.slice[0], i);
	for(int i=0; i<2; i++){
		P[I.axes[i]] = pos[i];
		I.boff[i] = coord2pos(conf.bmin[I.axes[i]], I.axes[i]);
		I.bbox[i] = coord2pos(conf.bmax[I.axes[i]], I.axes[i])-I.boff[i];
	}
	for(int i=0; i<D; i++){  T0 += s*(P[i]/N); P[i] %= N; s *= box[i];  offset0 |= interleave_bits(D, P[i], R+max_rank)<<i;  }
	if(T0>int(tiles.size())){ I.tile = nullptr; return I; } // вышли за пределы сетки
	for(int i=0; i<2; i++) for(int k=0; k<R+max_rank; k++) mask0 |= uint64_t(1)<<(k*D+I.axes[i]);  // в mask0 единицы стоят на позициях которые могут меняться
	I.tile = tiles[T0]; I.offset = offset0>>(max_rank*D); I.mask = mask0>>(max_rank*D); // настраиваем I на тайл нулевого ранга
	for(int i=0; i<2; i++){ I.imin[i] = I.tile->pos[I.axes[i]]+de_interleave_bits(D, I.offset>>I.axes[i], R)*(1<<max_rank); I.imax[i] = I.imin[i]+(1<<max_rank); }
	// поиск внутри тайла
	// while(I.tile->split[I.offset]){ // пока есть разбитые ячейки
	while(!I.tile->usage[I.offset] && I.tile->childs[I.offset>>((R-1)*D)]){   // это работает только для непрерывного поля
		int f = (offset0>>(D*(R+max_rank-I.tile->rank-1)))&maskD; // номер дочернего тайла
		I.tile = I.tile->childs[f];	I.offset = (offset0>>((max_rank-I.tile->rank)*D))&maskRD; I.mask = (mask0>>((max_rank-I.tile->rank)*D))&maskRD;		
		for(int i=0; i<2; i++){ if(I.offset&(1<<I.axes[i])) I.imin[i] = (I.imin[i]+I.imax[i])/2; else I.imax[i] = (I.imin[i]+I.imax[i])/2; }
	}
	I.imin -= I.boff; I.imax -= I.boff;
	return I;
}
//------------------------------------------------------------------------------
void aiw::AdaptiveMeshView::iterator::next(){	
	// imin += msh->off; imax += msh->off;  // переходим в глобальную систему координат
	uint32_t imask = ~mask, fix = offset&imask; offset = (((offset|imask)+1)&mask)|fix;
	if(offset==fix){ // текущий тайл закончился
		tile_t *root = tile->root();  Ind<2> p1(root->pos[axes[0]], root->pos[axes[1]]);  // тайл нулевого уровня и его угол
		int dT = 1<<(msh->R + msh->max_rank - tile->rank); // размер тайла текущего уровня
		if(tile->parent){
			int p0[msh->D]; for(int i=0; i<msh->D; i++) p0[i] = (tile->pos[i] - root->pos[i])/dT;  // позиция текущего тайла в тайле нулевого уровня
			offset = 0; mask = 0;
			for(int i=0; i<msh->D; i++){
				offset += interleave_bits(msh->D, p0[i], tile->rank)<<i;
				for(int k=0; k<tile->rank; k++) mask |= 1<<(msh->D*k+i);
			}
			imask = ~mask; fix = offset&imask; offset = (((offset|imask)+1)&mask)|fix;
			if(offset!=fix){ // мы остались в том же тайле нулевого уровня, offset задает следующий тайл для текущего ранга разбиения
				for(int i=0; i<2; i++) p1[i] += de_interleave_bits(msh->D, offset>>axes[i], tile->rank)*dT;
				*this = msh->find(*conf, p1); 
				return;
			}
		} // если мы попали сюда значит тайл нулевого уровня так или иначе закончился, ищем следующий
		dT = 1<<(msh->max_rank+msh->R); p1 /= dT; p1[axes[0]]++;				
		if(p1[axes[0]]>=msh->box[axes[0]]){
			p1[axes[0]] = 0; p1[axes[1]]++;
			if(p1[axes[1]]>=msh->box[axes[1]]){ tile = nullptr; return; } // обход закончен
		}
		*this = msh->find(*conf, p1*dT); 
		return;
	}
	int csz = 1<<(msh->max_rank-tile->rank);
	for(int i=0; i<2; i++){  // считаем imin, imax
		imin[i] = tile->pos[axes[i]] + de_interleave_bits(msh->D, offset>>axes[i], msh->R+tile->rank)*csz;
		imax[i] = imin[i]+csz;
	}
	if(tile->split[offset] || !tile->usage[offset]) *this = msh->find(*conf, imin);
	else{ imin -= boff; imax -= boff; }
}
//------------------------------------------------------------------------------
bool aiw::AdaptiveMeshView::load(aiw::IOstream& S){
	size_t s = S.tell();  BinaryFormat bf; bf.R = -1; bf.logscale = 0;
	bf.bmin = &bmin; bf.bmax = &bmax; bf.box = &box;
	if(!bf.load(S)) return false;
	if(!(bf.D&(1<<31))) {  S.seek(s);  return false; }	
	head = bf.head; D = bf.D&~(1<<31); szT = bf.szT; R = bf.R; logscale = bf.logscale; cfa_list = bf.tinfo.get_access();
	
	int tiles_sz = box[0]; for(int i=1; i<D; i++) tiles_sz *= box[i];
	tiles.resize(tiles_sz); data.resize(tiles_sz); max_rank = 0;
	int flags_sz = 64/(1<<(R*D)); if(flags_sz==0) flags_sz = 1; // по одному биту на ячейку
	for(int Ti=0; Ti<tiles_sz; Ti++){ // цикл по тайлам нулевого ранга
		int rD = -1, rszT = -1, rR = -1;
		size_t stell = S.tell();  S>rD>rszT>rR; if(rD!=D || rszT!=szT || rR!=R){
			S.seek(stell); WERR(rD, D, rszT, szT, rR, R);
			return false;
		}		
		int h_sz, hl_sz, rID; S.seek(4*D, 1); S>h_sz>hl_sz>rID; int l_sz = hl_sz-h_sz;
		// WOUT(Ti, h_sz, hl_sz, rID, D, szT);
		std::vector<tile_t*> tbl(hl_sz, 0);

		root_t &root = data[Ti];
		root.childs.resize((hl_sz-1)*(1<<D));
		root.pos.resize((hl_sz-1)*D);
		root.flags.resize((hl_sz-1)*flags_sz*2, 0);
		root.chunks.resize(l_sz*(1<<(D*(R-1))));
		root.data.resize((h_sz-1)*(1<<(D*R))*szT);
		root.htiles.resize(h_sz-1); for(int i=1; i<h_sz; i++) tbl[i] = &(root.htiles[i-1]); 
		root.ltiles.resize(l_sz);   for(int i=h_sz; i<hl_sz; i++) tbl[i] = &(root.ltiles[i-h_sz]);
		tiles.at(Ti) = tbl.at(rID); // корневой тайл
		for(int i=1; i<hl_sz; i++){
			tbl[i]->childs = &(root.childs[(i-1)*(1<<D)]);
			tbl[i]->pos = &(root.pos[(i-1)*D]);
			tbl[i]->usage.arr =  &(root.flags[(i-1)*(2*flags_sz)]);
			tbl[i]->split.arr =  &(root.flags[(i-1)*(2*flags_sz)+flags_sz]);
		}
		int I; uint32_t far_ch_sz; uint16_t ch_use;
		for(int hti=0; hti<h_sz-1; hti++){
			heavy_tile_t &t = root.htiles[hti]; t.data = &(root.data[hti*(1<<(R*D))*szT]);
			S>t.rank>ch_use>far_ch_sz>I;  t.parent = tbl.at(I); for(int i=0; i<(1<<D); i++){ S>I; t.childs[i] = tbl.at(I); }
			//WOUT(t, t->parent, t->childs[0], t->childs[1], t->childs[2], t->childs[3]);
			if(max_rank<t.rank) max_rank = t.rank;
			S.read(t.usage.arr, flags_sz*8);
			S.seek(flags_sz*8, 1); // пропускаем ghost
			S.seek(1<<((R-1)*D-2), 1); // пропускаем chunks
			S.read(t.data, (1<<(R*D))*szT);
		}		
		for(int lti=0; lti<l_sz; lti++){
			light_tile_t &t = root.ltiles[lti]; t.chunks = &(root.chunks[lti*(1<<(D*(R-1)))]);
			S>t.rank>ch_use>I;
			// if(I<0) WRAISE("", I);
			t.page = I? &(root.htiles.at(I-1)): nullptr; S>I; t.parent = tbl.at(I); for(int i=0; i<(1<<D); i++){ S>I; t.childs[i] = tbl.at(I); }
			if(max_rank<t.rank) max_rank = t.rank;
			//WOUT(t, t->parent, t->page, t->childs[0], t->childs[1], t->childs[2], t->childs[3], t->parent->find_child_ID(t));
			S.read(t.usage.arr, flags_sz*8);
			S.seek(flags_sz*8, 1); // пропускаем ghost
			S.read(t.chunks, (1<<((R-1)*D))*2);
		}
		for(tile_t *t: tbl){ // считаем split по тайлам
			if(!t) continue;
			// t->split.resize(1<<(R*D), false);
			for(int i=0; i<(1<<D); i++){ // цикл по дочерним тайлам
				tile_t *c = t->childs[i]; // дочерний тайл
				if(c) for(int j=0; j<(1<<(D*(R-1))); j++) // цикл по чанкам дочернего тайла
						  for(int k=0; k<(1<<D); k++) // цикл по ячейкам в чанке дочернего тайла
							  if(c->usage[(j<<D)+k]){ t->split.set((i<<((R-1)*D))+j, true); break; }
			}
		}
	} // конец цикла по тайлам нулевого ранга
    int Tbox = 1<<(R+max_rank); // WOUT(R, max_rank, Tbox);
	for(int Ti=0; Ti<tiles_sz; Ti++){ // цикл по тайлам нулевого ранга, здесь надо посчитать bmin и step по дереву
		int x = Ti;	for(int i=0; i<D; i++){  tiles[Ti]->pos[i] = x%box[i]*Tbox; x /= box[i]; }
		// WOUT(Ti, tiles[Ti]->pos);
		tiles[Ti]->set_child_pos(D, R+max_rank-1);
		// WOUT(Ti, tiles[Ti]->pos);
	} // конец цикла по тайлам нулевого ранга
	//WOUT(max_rank);
	for(int i=0; i<D; i++){
		int sz = box[i]*(1<<(R+max_rank));
		step[i] = logscale&1<<i? exp(log(bmax[i]/bmin[i])/sz): (bmax[i]-bmin[i])/sz;
		rstep[i] = logscale&1<<i? 1./log(step[i]): 1./step[i];
	}
	return true;
}
//------------------------------------------------------------------------------
void aiw::AdaptiveMeshView::get_conf(ConfView &conf, bool firstcall) const {  // настраивает conf (с учетом crop)
	conf.dim = D; // че делать если пришел dim!=D?!
	for(int i=0; i<D; i++){
		if(firstcall || (conf.bmin[i]==conf.bmin0[i] && conf.bmax[i]==conf.bmax0[i])){ conf.bmin[i] = bmin[i]; conf.bmax[i] = bmax[i]; }
		if(firstcall) conf.slice[i] = conf.bmin[i] = bmin[i];
		conf.bmin0[i] = bmin[i]; conf.bmax0[i] = bmax[i]; conf.step[i] = step[i]; conf.size[i] = box[i]*(1<<(R+max_rank));
	}
	conf.logscale = logscale; conf.mod_crop = true;
	conf.crop(vec(0.,0.), vec(1.,1.));  // для приведения к границам ячеек

	conf.anames[0] = "X";
	conf.anames[1] = "Y";
	conf.anames[2] = "Z";

	conf.features =  ConfView::opt_axes|ConfView::opt_flip|ConfView::opt_crop|ConfView::opt_cell_bound|ConfView::opt_step_size;
	conf.cfa_list = cfa_list;    conf.cfa_xfem_list.clear();
}
//------------------------------------------------------------------------------
std::string aiw::AdaptiveMeshView::get(const ConfView& conf, aiw::Vec<2> r) const {
	char buf[1024]; int i = coord2pos(r[0], conf.axes[0]), j = coord2pos(r[1], conf.axes[1]); double v =  *find(conf, ind(i, j));
	snprintf(buf, 1023, "%g\n%i %i", v, i, j);
	return buf;
}
//------------------------------------------------------------------------------
aiw::Vec<2> aiw::AdaptiveMeshView::f_min_max(const ConfView &conf) const {  // вычисляет min-max, как это делать для preview?
	auto I = begin(conf); double a = *I, b = a;
	for(; I!=end(); ++I){
		double f = *I;
		if(a>f) a = f;
		if(b<f) b = f;
	}
	return vec(a, b);
}
//------------------------------------------------------------------------------
