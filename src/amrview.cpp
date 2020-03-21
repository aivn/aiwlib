/**
 * Copyright (C) 2019 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../include/aiwlib/amrview"
using namespace aiw;
//------------------------------------------------------------------------------
aiw::AdaptiveMeshView::iterator aiw::AdaptiveMeshView::find(aiw::Ind<2> pos){  // произвольный доступ по срезу
	if(!(Ind<2>()<=pos && pos<bbox())) return iterator();
	iterator I; I.msh = this; I.bmin = pos; I.bmax = pos+ind(1<<max_rank); I.mask = I.offset = 0; 
	// ищем тайл сетки нулевого ранга T0 и позицию P внутри него (на самой мелкой сетке)
	Ind<Dmax> P; int j = 0, T0 = 0, s = 1, N = 1<<(R+max_rank); // счетчик осей, смещение на большой сетке, размер тайла нулевого ранга на мелкой сетке
	uint64_t offset0 = 0, mask0 = 0, maskR = (~uint64_t(0))>>(64-R*D), maskD = (~uint64_t(0))>>(64-D); // смещение на мелкой сетке, R*D бит и D бит
	for(int i=0; i<D; i++){ // цикл по координатам
		P[i] = slice[i]; if(P[i]==-1){ P[i] = swap? pos[1-j]: pos[j]; I.axes[swap?1-j:j] = i; j++; }
		T0 += s*(P[i]/N); P[i] %= N; s *= box[i];
		uint64_t f = interleave_bits(D, P[i], R+max_rank)<<i; offset0 |= f;
		if(slice[i]==-1) for(int k=0; k<R+max_rank; k++) mask0 |= uint64_t(1)<<(k*D+i); 
	}
	if(T0>int(tiles.size())){ I.tile = nullptr; return I; }
	I.tile = tiles[T0]; I.offset = offset0>>(max_rank*D); I.mask = mask0>>(max_rank*D); // настраиваем I на тайл нулевого ранга
	// поиск внутри тайла
	while(I.tile->split[I.offset]){ // пока есть разбитые ячейки
		int f = (offset0>>(D*(R+max_rank-I.tile->rank-1)))&maskD; // номер дочернего тайла
		I.tile = I.tile->childs[f];	I.offset = (offset0>>((max_rank-I.tile->rank)*D))&maskR; I.mask = (mask0>>((max_rank-I.tile->rank)*D))&maskR;		
		// for(int i=0; i<D; i++){ if(I.offset&(1<<i)) I.bmin[i] = (I.bmin[i]+I.bmax[i])/2; else I.bmax[i] = (I.bmin[i]+I.bmax[i])/2; }
	}
	return I;
}
//------------------------------------------------------------------------------
void aiw::AdaptiveMeshView::iterator::operator ++ (){	
	uint32_t imask = ~mask, fix = offset&imask, off0 = offset; 
	offset = (((offset|imask)+1)&mask)|fix;
	if(offset==fix){ // текущий тайл закончился
		// Ind<2> start(-1); tile = tile->next_tile(msh, axes, start);
		// if(!tile) return; // обход закончен

		tile_t *root = tile->root();
		Ind<2> p1(root->pos[axes[0]], root->pos[axes[1]]);  // угол тайла нулевого уровня (будущая позиция точки)
		if(tile->parent){
			Ind<Dmax> p0 = (tile->pos - root->pos)/(1<<(msh->R + msh->max_rank - tile->rank));  // исходная позиция в тайле нулевого уровня
			offset = 0; mask = 0;
			for(int i=0; i<msh->D; i++){
				offset += interleave_bits(msh->D, p0[i], tile->rank)<<i;
				for(int k=0; k<tile->rank; k++) mask |= 1<<(msh->D*k+i);
			}
			imask = ~mask; fix = offset&imask; offset = (((offset|imask)+1)&mask)|fix;
			if(offset!=fix){
				for(int i=0; i<2; i++) p1[i] += de_interleave_bits(msh->D, offset>>axes[i], tile->rank)*(1<<(msh->R+msh->max_rank-tile->rank));
				*this = msh->find(p1);
				return;
			}
		}
		p1 /= (1<<(msh->max_rank+msh->R)); p1[axes[0]]++;				
		if(p1[axes[0]]>=msh->box[axes[0]]){
			p1[axes[0]] = 0; p1[axes[1]]++;
			if(p1[axes[1]]>=msh->box[axes[1]]){ tile = nullptr; return; } // обход закончен
		}
		*this = msh->find(p1*(1<<(msh->max_rank+msh->R))); 
		return;
	}

	int csz = 1<<(msh->max_rank-tile->rank);
	for(int i=0; i<2; i++){  // считаем bmin, bmax
		bmin[i] = tile->pos[axes[i]] + de_interleave_bits(msh->D, offset>>axes[i], msh->R)*csz;
		bmax[i] = bmin[i]+csz;
	}
	if(tile->split[offset] || !tile->usage[offset]) *this = msh->find(bmin);
}
//------------------------------------------------------------------------------
//void aiw::AdaptiveMeshView::clear(){}
//------------------------------------------------------------------------------
template <typename T> void read_vector(aiw::IOstream& S, std::vector<T> &V, int sz){ V.resize(sz); S.read(V.data(), sz*sizeof(T)); }
void read_vector(aiw::IOstream& S, std::vector<bool> &V, int sz){
	int bsz = sz/64?sz/64:1; uint64_t buf[bsz]; S.read(buf, bsz*8); // WOUT(buf[0]);
	V.resize(sz); for(int i=0; i<sz; i++) V[i] = buf[i/64]&(uint64_t(1)<<(i%64));
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
	head = h.c_str(); R = rR; D = rD&~(1<<31); szT = rszT;
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
	return true;
}
//------------------------------------------------------------------------------
aiw::Vec<2> aiw::AdaptiveMeshView::min_max(){
	auto I = begin(); double a = *I, b = *I;
	for(; I!=end(); ++I){
		if(a>*I) a = *I;
		if(b<*I) b = *I;
	}
	return vec(a, b);
}
//------------------------------------------------------------------------------
