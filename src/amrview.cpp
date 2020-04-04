/**
 * Copyright (C) 2019-20 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../include/aiwlib/amrview"
using namespace aiw;
//------------------------------------------------------------------------------
aiw::AdaptiveMeshView::iterator aiw::AdaptiveMeshView::core_t::find(aiw::Ind<2> pos, AdaptiveMeshView *msh){  // произвольный доступ по срезу
	if(!(Ind<2>()<=pos && pos<msh->fbox)) return iterator();
	iterator I; I.msh = msh; I.mask = I.offset = 0; 
	// ищем тайл сетки нулевого ранга T0 и позицию P внутри него (на самой мелкой сетке)
	Ind<Dmax> P; int j = 0, T0 = 0, s = 1, N = 1<<(R+max_rank); // счетчик осей, смещение на большой сетке, размер тайла нулевого ранга на мелкой сетке
	uint64_t offset0 = 0, mask0 = 0, maskRD = (~uint64_t(0))>>(64-R*D), maskD = (~uint64_t(0))>>(64-D); // смещение на мелкой сетке, R*D бит и D бит
	for(int i=0; i<D; i++){ // цикл по координатам
		P[i] = msh->slice[i]; if(P[i]==-1){ P[i] = msh->swap? pos[1-j]: pos[j]; I.axes[msh->swap?1-j:j] = i; j++; }
		T0 += s*(P[i]/N); P[i] %= N; s *= box[i];  offset0 |= interleave_bits(D, P[i], R+max_rank)<<i;
		if(msh->slice[i]==-1) for(int k=0; k<R+max_rank; k++) mask0 |= uint64_t(1)<<(k*D+i);  // в mask0 единицы стоят на позициях которые могут меняться
	}
	if(T0>int(tiles.size())){ I.tile = nullptr; return I; } // вышли за пределы сетки
	I.tile = tiles[T0]; I.offset = offset0>>(max_rank*D); I.mask = mask0>>(max_rank*D); // настраиваем I на тайл нулевого ранга
	for(int i=0; i<2; i++){ I.imin[i] = I.tile->pos[I.axes[i]]+de_interleave_bits(D, I.offset>>I.axes[i], R)*(1<<max_rank); I.imax[i] = I.imin[i]+(1<<max_rank); }
	// поиск внутри тайла
	while(I.tile->split[I.offset]){ // пока есть разбитые ячейки
		int f = (offset0>>(D*(R+max_rank-I.tile->rank-1)))&maskD; // номер дочернего тайла
		I.tile = I.tile->childs[f];	I.offset = (offset0>>((max_rank-I.tile->rank)*D))&maskRD; I.mask = (mask0>>((max_rank-I.tile->rank)*D))&maskRD;		
		for(int i=0; i<2; i++){ if(I.offset&(1<<I.axes[i])) I.imin[i] = (I.imin[i]+I.imax[i])/2; else I.imax[i] = (I.imin[i]+I.imax[i])/2; }
	}
	return I;
}
//------------------------------------------------------------------------------
void aiw::AdaptiveMeshView::iterator::next(){	
	// imin += msh->off; imax += msh->off;  // переходим в глобальную систему координат
	uint32_t imask = ~mask, fix = offset&imask; offset = (((offset|imask)+1)&mask)|fix;
	if(offset==fix){ // текущий тайл закончился
		tile_t *root = tile->root();  Ind<2> p1(root->pos[axes[0]], root->pos[axes[1]]);  // тайл нулевого уровня и его угол
		int dT = 1<<(msh->core->R + msh->core->max_rank - tile->rank); // размер тайла текущего уровня
		if(tile->parent){
			Ind<Dmax> p0 = (tile->pos - root->pos)/dT;  // позиция текущего тайла в тайле нулевого уровня
			offset = 0; mask = 0;
			for(int i=0; i<msh->core->D; i++){
				offset += interleave_bits(msh->core->D, p0[i], tile->rank)<<i;
				for(int k=0; k<tile->rank; k++) mask |= 1<<(msh->core->D*k+i);
			}
			imask = ~mask; fix = offset&imask; offset = (((offset|imask)+1)&mask)|fix;
			if(offset!=fix){ // мы остались в том же тайле нулевого уровня, offset задает следующий тайл для текущего ранга разбиения
				for(int i=0; i<2; i++) p1[i] += de_interleave_bits(msh->core->D, offset>>axes[i], tile->rank)*dT;
				*this = msh->core->find(p1, msh); imin -= msh->off;  imax -= msh->off; 
				return;
			}
		} // если мы попали сюда значит тайл нулевого уровня так или иначе закончился, ищем следующий
		dT = 1<<(msh->core->max_rank+msh->core->R); p1 /= dT; p1[axes[0]]++;				
		if(p1[axes[0]]>=msh->core->box[axes[0]]){
			p1[axes[0]] = 0; p1[axes[1]]++;
			if(p1[axes[1]]>=msh->core->box[axes[1]]){ tile = nullptr; return; } // обход закончен
		}
		*this = msh->core->find(p1*dT, msh);  imin -= msh->off;  imax -= msh->off;
		return;
	}
	int csz = 1<<(msh->core->max_rank-tile->rank);
	for(int i=0; i<2; i++){  // считаем imin, imax
		imin[i] = tile->pos[axes[i]] + de_interleave_bits(msh->core->D, offset>>axes[i], msh->core->R+tile->rank)*csz;
		imax[i] = imin[i]+csz;
	}
	if(tile->split[offset] || !tile->usage[offset]) *this = msh->core->find(imin, msh);
	imin -= msh->off; imax -= msh->off; 
	// *this = msh->core->find(imin, msh);
}
//------------------------------------------------------------------------------
bool aiw::AdaptiveMeshView::iterator::tile_bound(int axe) const { // axe - номер оси + 1 со знаком (-слева, +справа) т.е. -/+1, -/+2
	uint32_t D = msh->core->D, R = msh->core->R, Dbits = 1<<axes[abs(axe)-1], RDbits = (~uint32_t(0))>>(32-R*D);
	uint32_t off = offset, zm = (zmasks[D-1]<<axes[abs(axe)-1])&RDbits;
	for(tile_t *t = tile; t; t=t->childs[off>>((R-1)*D)]){
		if((axe<0 && !(zm&off))||(axe>0 && (off&zm)==zm)) return true;
		off = (axe<0? off<<D: (off<<D)|Dbits)&RDbits;
	}
	return false;
}
//void aiw::AdaptiveMeshView::clear(){}
//------------------------------------------------------------------------------
template <typename T> void read_vector(aiw::IOstream& S, std::vector<T> &V, int sz){ V.resize(sz); S.read(V.data(), sz*sizeof(T)); }
void read_vector(aiw::IOstream& S, std::vector<bool> &V, int sz){
	int bsz = sz/64?sz/64:1; uint64_t buf[bsz]; S.read(buf, bsz*8); // WOUT(buf[0]);
	V.resize(sz); for(int i=0; i<sz; i++) V[i] = buf[i/64]&(uint64_t(1)<<(i%64));
}
//------------------------------------------------------------------------------
bool aiw::AdaptiveMeshView::core_t::load(aiw::IOstream&& S, bool use_mmap, bool raise_on_error){
	std::string h; int rD=-1, rszT=-1, rR=-1;  size_t s = S.tell(); S>h>rD>rszT>rR;
	if(S.tell()-s!=16+h.size()){ S.seek(s); return false; }
	if(!(rD&(1<<31))){ 
		S.seek(s); 
		if(raise_on_error){ WRAISE("incorrect AdaptiveMeshView::load(): ", rD, rszT, rR, S.name, S.tell(), h); }
		else return false;
	}
	head = h.c_str(); R = rR; D = rD&~(1<<31); szT = rszT;
	box = ind(0); S.read(&box, D*4);

	for(int i=0; i<Dmax; i++){ bmin[i] = 0; bmax[i] = box[i]; } // int logscale_= 0;
	if(h.size()>head.size()+4+D*16){
		int i = this->head.size(), i0 = h.size()-(4+D*16); while(i<i0 && h[i]==0) i++;
		if(i==i0){
			memcpy(&bmin, h.c_str()+i0, D*8); i0 += D*8;
			memcpy(&bmax, h.c_str()+i0, D*8); i0 += D*8;
			// memcpy(&this->logscale, h.c_str()+i0, 4);
		}
	}
	
	int tiles_sz = box[0]; for(int i=1; i<D; i++) tiles_sz *= box[i];
	tiles.resize(tiles_sz); htiles.clear(); ltiles.clear(); max_rank = 0;
	for(int Ti=0; Ti<tiles_sz; Ti++){ // цикл по тайлам нулевого ранга
		size_t stell = S.tell();  S>rD>rszT>rR; if(rD!=D || rszT!=szT || rR!=R){
			S.seek(stell); WERR(rD, D, rszT, szT, rR, R);
			return false;
		}

		int h_sz, hl_sz, rID; S.seek(4*D, 1); S>h_sz>hl_sz>rID;
		//WOUT(h_sz, hl_sz, rID);
		std::vector<tile_t*> tbl(hl_sz, 0);
		std::vector<heavy_tile_t*> hts(h_sz-1);
		std::vector<light_tile_t*> lts(hl_sz-h_sz);

		for(int i=1; i<h_sz; i++){ htiles.emplace_back(); tbl[i] = hts[i-1] = &htiles.back(); }
		for(int i=h_sz; i<hl_sz; i++){ ltiles.emplace_back(); tbl[i] = lts[i-h_sz] = &ltiles.back(); }
		tiles[Ti] = tbl[rID];  int I; uint32_t far_ch_sz; uint16_t ch_use;
		//WOUT(rID, tbl[rID]);
		//for(int i=0; i<hl_sz; i++) WOUT(i, tbl[i]);
		for(heavy_tile_t *t: hts){			
			S>t->rank>ch_use>far_ch_sz>I;  t->parent = tbl[I]; for(int i=0; i<(1<<D); i++){ S>I; t->childs[i] = tbl[I]; }
			//WOUT(t, t->parent, t->childs[0], t->childs[1], t->childs[2], t->childs[3]);
			if(max_rank<t->rank) max_rank = t->rank;
			read_vector(S, t->usage, 1<<(R*D));
			read_vector(S, t->ghost, 1<<(R*D));
			S.seek(1<<((R-1)*D-2), 1); // пропускаем chunks
			read_vector(S, t->data, (1<<(R*D))*szT); //for(int i=0; i<int(t->data.size()/4); i++) WOUT(i, *(float*)&(t->data[i*4]));
		}
		for(light_tile_t *t: lts){
			S>t->rank>ch_use>I; t->page = hts[I-1]; S>I; t->parent = tbl[I]; for(int i=0; i<(1<<D); i++){ S>I; t->childs[i] = tbl[I]; }
			if(max_rank<t->rank) max_rank = t->rank;
			//WOUT(t, t->parent, t->page, t->childs[0], t->childs[1], t->childs[2], t->childs[3], t->parent->find_child_ID(t));
			read_vector(S, t->usage, 1<<(R*D)); //for(int i=0; i<int(t->usage.size()); i++) WOUT(i, t->usage[i]);
			read_vector(S, t->ghost, 1<<(R*D));
			read_vector(S, t->chunks, 1<<((R-1)*D));
		}
		for(tile_t *t: tbl){ // считаем split по тайлам
			if(!t) continue;
			t->split.resize(1<<(R*D), false);
			for(int i=0; i<(1<<D); i++){ // цикл по дочерним тайлам
				tile_t *c = t->childs[i]; // дочерний тайл
				if(c) for(int j=0; j<(1<<(D*(R-1))); j++) // цикл по чанкам дочернего тайла
						  for(int k=0; k<(1<<D); k++) // цикл по ячейкам в чанке дочернего тайла
							  if(c->usage[(j<<D)+k]){ t->split[(i<<((R-1)*D))+j] = true; break; }
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
	for(int i=0; i<D; i++) step[i] = (bmax[i]-bmin[i])/(box[i]*(1<<(R+max_rank)));
	return true;
}
//------------------------------------------------------------------------------
bool aiw::AdaptiveMeshView::load(aiw::IOstream& S, bool use_mmap, bool raise_on_error){
	core_t *core2 = new core_t;
	if(core2->load(std::move(S), use_mmap, raise_on_error)){
		core.reset(core2);
		dim = core->D;
		slice = ind(0); slice[0] = slice[1] = -1;
		set_axes(0, 1);
		return true;
	}
	delete core2; return false;
}
//------------------------------------------------------------------------------
aiw::Vec<2> aiw::AdaptiveMeshView::min_max(){
	auto I = begin(); double a = *I, b = *I;
	for(; I!=end(); ++I){
		if(a>*I) a = *I;
		if(b<*I) b = *I;
		// WOUT(I.imin, I.imax, I.rank(), *I);
	}
	// WOUT(a, b, offset_in_cell, bmin, bmax, box_, fbox, core->max_rank);
	return vec(a, b);
}
//------------------------------------------------------------------------------
void aiw::AdaptiveMeshView::set_axes(int ax, int ay){
	axes[0] = ax;  axes[1] = ay; swap = ax>ay;
	for(int i=0; i<2; i++){
		bmin[i] = core->bmin[axes[i]];
		bmax[i] = core->bmax[axes[i]];
		step[i] = core->step[axes[i]];
		off[i] = 0;
		fbox[i] = box_[i] = core->box[axes[i]]*(1<<(core->R+core->max_rank));
	}
}
//------------------------------------------------------------------------------
AdaptiveMeshView aiw::AdaptiveMeshView::slice2(aiw::Ind<3> spos){
	AdaptiveMeshView amv = *this; int j=0;
	for(int i=0; i<3 && j<2; i++) if(spos[i]==-1) amv.axes[j++] = i;
	amv.set_axes(amv.axes[0], amv.axes[1]);
	return amv;
}
//------------------------------------------------------------------------------
AdaptiveMeshView aiw::AdaptiveMeshView::transpose(int, int){
	AdaptiveMeshView amv = *this;
	amv.swap = !swap;
	for(int i=0; i<2; i++){
		amv.bmin[i] = bmin[1-i]; 
		amv.bmax[i] = bmax[1-i]; 
		amv.step[i] = step[1-i]; 
		amv.off[i]  = off[1-i];
		amv.box_[i] = box_[1-i];
		amv.axes[i] = axes[1-i];
		amv.fbox[i] = fbox[1-i];
	}
	return amv;
}
//------------------------------------------------------------------------------
AdaptiveMeshView aiw::AdaptiveMeshView::crop(aiw::Ind<2> a, aiw::Ind<2> b, Ind<2> d){
	WOUT(a, b, d, off, bmin, bmax, step, box_, fbox);
	// a = find(a).imin; b = find(b-ind(1)).imax;
	// WOUT(a, b, d, off, bmin, bmax, step, box_, fbox);
	AdaptiveMeshView amv = *this;
	amv.off += a; amv.box_ = b-a;
	amv.bmin = core->bmin(axes[0], axes[1])+(step&amv.off);
	amv.bmax = amv.bmin + (amv.step&amv.box_); 
	WOUT(amv.off, amv.bmin, amv.bmax, amv.step, amv.box_, amv.fbox);
	return amv;
}
//------------------------------------------------------------------------------
