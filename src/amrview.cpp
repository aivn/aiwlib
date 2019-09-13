/**
 * Copyright (C) 2019 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../include/aiwlib/amrview"
using namespace aiw;
//------------------------------------------------------------------------------
aiw::AdaptiveMeshView::iterator aiw::AdaptiveMeshView::find(aiw::Ind<2> pos){  // произвольный доступ по срезу
	iterator I; I.msh = this; I.bmin = pos; I.bmax = pos+ind(1<<max_rank); I.mask = I.offset = 0;
	// ищем тайл сетки нулевого ранга и позицию P внутри него (на самой мелкой сетке)
	Ind<Dmax> P; int j = 0, T0 = 0, s = 1, N = 1<<(R+max_rank);
	uint64_t offset0 = 0, mask0 = 0, maskR = ~0, maskD = ~0; maskR >>= 64-R*D; maskD >>= 64-D;
	for(int i=0; i<D; i++){
		P[i] = slice[i]; if(P[i]==-1){ P[i] = swap? pos[1-j]: pos[j]; I.axes[swap?1-j:j] = i; j++; }
		T0 += s*(P[i]/N); P[i] %= N; if(i) s *= box[i-1];
		uint64_t f = interleave_bits(D, P[i], R+max_rank)<<i;
		offset0 |= f; if(slice[i]==-1) for(int j=0; j<R+max_rank; j++) mask0 |= 1<<(j*D+i); 
	}
	I.tile = tiles[T0]; I.offset = offset0>>(max_rank*D); I.mask = mask0>>(max_rank*D);

	// поиск внутри тайла
	while(I.tile->split[I.offset]){
		int f = (offset0>>(D*(R+max_rank-I.tile->rank-1)))&maskD;
		I.tile = I.tile->child[f];
		for(int i=0; i<D; i++){ if(f&(1<<i)) I.bmin[i] = (I.bmin[i]+I.bmax[i])/2; else I.bmax[i] = (I.bmin[i]+I.bmax[i])/2; }
		I.offset = (offset0>>((max_rank-I.tile->rank)*D))&maskR; I.mask = (mask0>>((max_rank-I.tile->rank)*D))&maskR;		
	}
	return I;
}
//------------------------------------------------------------------------------
void aiw::AdaptiveMeshView::iterator::operator ++ (){
	uint32_t imask = ~mask, fix = offset&imask;
	offset = (((offset|imask)+1)&mask)|fix;
	int csz = 1<<(msh->max_rank-tile->rank);
	if(offset==fix) *this = msh->find(ind(tile->pos[axes[0]]+(csz<<msh->R), tile->pos[axes[1]]));  // тайл закончился
	else { 
		for(int i=0; i<2; i++){  // считаем bmin, bmax
			bmin[i] = tile->pos[axes[i]] + de_interleave_bits(msh->D, offset>>axes[i], msh->R)*csz;
			bmax[i] = bmin[i]+csz;
		}
		if(tile->split[offset]) *this = msh->find(bmin);  // ячейка разбита
	} 
}
//------------------------------------------------------------------------------
//void aiw::AdaptiveMeshView::clear(){}
//------------------------------------------------------------------------------
template <typename T> void read_vector(aiw::IOstream& S, std::vector<T> &V, int sz){ V.resize(sz); S.read(V.data(), sz*sizeof(T)); }
void read_vector(aiw::IOstream& S, std::vector<bool> &V, int sz){
	int bsz = sz/64?sz/64:1; uint64_t buf[bsz]; S.read(buf, bsz*8);
	V.resize(sz); for(int i=0; i<sz; i++) V[i] = buf[i/64]&(1<<(1%64));
}
//------------------------------------------------------------------------------
bool aiw::AdaptiveMeshView::load(aiw::IOstream&& S, bool use_mmap, bool raise_on_error){
	std::string h; int rD=-1, rszT=-1, rR=-1;  size_t s = S.tell(); S>h>rD>rszT>rR;
	if(S.tell()-s!=16+h.size()){ S.seek(s); return false; }
	if(!(rD&(1<<31))){ 
		S.seek(s); 
		if(raise_on_error){ WRAISE("incorrect AdaptiveMeshView::load(): ", rD, rszT, rR, S.name, S.tell(), h); }
		else return false;
	}
	head = h.c_str(); R = rR; D = rD; szT = rszT;
	box = ind(0); S.read(&box, D*4);

	/*
	for(int i=0; i<Dmax; i++){ bmin[i] = 0; bmax[i] = box[i]; } // int logscale_= 0;
	if(h.size()>head.size()+4+D*16){
		int i = this->head.size(), i0 = h.size()-(4+D*16); while(i<i0 && h[i]==0) i++;
		if(i==i0){
			memcpy(&bmin, h.c_str()+i0, D*8); i0 += D*8;
			memcpy(&bmax, h.c_str()+i0, D*8); i0 += D*8;
			memcpy(&this->logscale, h.c_str()+i0, 4);
		}
	}
	//	this->set_axes(bmin_, bmax_, logscale_);
	*/

	int tiles_sz = box[0]; for(int i=1; i<D; i++) tiles_sz *= box[i];
	tiles.resize(tiles_sz); htiles.clear(); ltiles.clear(); max_rank = 0;
	for(int Ti=0; Ti<tiles_sz; Ti++){ // цикл по тайлам нулевого ранга
		size_t stell = S.tell();  S>rD>rszT>rR; if(rD!=D || rszT!=szT || rR!=R){
			S.seek(stell); WERR(rD, D, rszT, szT, rR, R);
			return false;
		}

		int h_sz, hl_sz, rID; S>h_sz>hl_sz>rID;
		std::vector<tile_t*> tbl(hl_sz, 0);
		std::vector<heavy_tile_t*> hts(h_sz-1);
		std::vector<light_tile_t*> lts(hl_sz-h_sz);

		for(int i=1; i<h_sz; i++){ htiles.emplace_back(); tbl[i] = hts[i-1] = &htiles.back(); }
		for(int i=h_sz; i<hl_sz; i++){ ltiles.emplace_back(); tbl[i] = lts[i-h_sz] = &ltiles.back(); }
		tiles[Ti] = tbl[rID];  int I; uint32_t far_ch_sz; uint16_t ch_use; 
		for(heavy_tile_t *t: hts){			
			S>t->rank>ch_use>far_ch_sz>I; t->parent = tbl[I]; for(int i=0; i<(1<<D); i++){ S>I; t->child[i] = tbl[I]; }
			if(max_rank<t->rank) max_rank = t->rank;
			read_vector(S, t->usage, 1<<(R*D));
			read_vector(S, t->ghost, 1<<(R*D));
			S.seek(1<<((R-1)*D-2), 1); // пропускаем chunks
			read_vector(S, t->data, (1<<(R*D))*szT);
		}
		for(light_tile_t *t: lts){
			S>t->rank>ch_use>I; t->parent = tbl[I]; S>I; t->page = hts[I-1]; for(int i=0; i<(1<<D); i++){ S>I; t->child[i] = tbl[I]; }
			if(max_rank<t->rank) max_rank = t->rank;
			read_vector(S, t->usage, 1<<(R*D));
			read_vector(S, t->ghost, 1<<(R*D));
			read_vector(S, t->chunks, 1<<((R-1)*D));
		}
		for(tile_t *t: tbl){ // считаем split по тайлам
			t->split.resize(1<<(R*D), false);
			for(int i=0; i<(1<<D); i++){
				tile_t *c = t->child[i];
				if(c) for(int j=0; j<(1<<(D*(R-1))); j++) for(int k=0; k<(1<<D); k++) if(c->usage[(j<<D)+k]){ t->split[(i<<(1<<((R-1)*D)))+j] = true; break; }
			}
		}
	} // конец цикла по тайлам нулевого ранга
	int Tbox = 1<<(R*max_rank);
	for(int Ti=0; Ti<tiles_sz; Ti++){ // цикл по тайлам нулевого ранга, здесь надо посчитать bmin и step по дереву
		int x = Ti;	for(int i=0; i<D; i++){  tiles[Ti]->pos[i] = x%box[i]*Tbox; x /= box[i]; }
		tiles[Ti]->set_child_pos(D, max_rank-1);
	} // конец цикла по тайлам нулевого ранга
	return true;
}
//------------------------------------------------------------------------------
