/**
 * Copyright (C) 2016 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include "../include/aiwlib/mesh"
#include "../include/aiwlib/magnetics"
#include "../include/aiwlib/binaryio"
using namespace aiw;
//------------------------------------------------------------------------------
void MagneticData::set_geometry(int lat, Figure fig, 
								int periodic_bc_mask,
								Vecf<3> step,  // размеры ячейки		
								Ind<3>  tile_sz,  // размеры tile в ячейках
								Vecf<3> base_r,   // координаты левого нижнего угла ячейки (0,0,0) в глобальной системе координат
								Vecf<3> ort_x, Vecf<3> ort_y){ // ort_z = ort_x%ort_y
	auto L = lats[lat];
	L.tile_sz = tile_sz;
	L.base_r = base_r;
	Vecf<3> ort_z = ort_x%ort_y;
	L.orts[0] = vecf(ort_x[0], ort_y[0], ort_z[0]);
	L.orts[1] = vecf(ort_x[1], ort_y[1], ort_z[1]); 
	L.orts[2] = vecf(ort_x[2], ort_y[2], ort_z[2]);

	if(L.orts[0]!=ort_x || L.orts[1]!=ort_y || L.orts[2]!=ort_z) fig = fig.rotate(base_r, ort_x, ort_y); // ??? transform ???
	Vecf<3> a = fig.get_min(), b = fig.get_max(); 
	for(int i=0; i<3; i++){ float d = ::fmod(base_r[i]-a[i], step[i]); a[i] += d>0?d-step[i]:d; }
	for(int i=0; i<3; i++){ float d = ::fmod(base_r[i]-b[i], step[i]); b[i] += d<0?d+step[i]:d; }

	Mesh<char, 3> nodes; // сетка узлов и tile-ов
	nodes.init(((b-a)/step).round()+Ind<3>(1), a, b+step);
	nodes.fill(0);
	
	for(Ind<3> i; i^=nodes.bbox(); ++i) if(fig.check(nodes.pos2coord(i))) nodes[i] = 1; // отмечаем все узлы попавшие в fig
	for(Ind<3> i; i^=nodes.bbox()-ind(1); ++i){ // внутренние |=2, внешние |=4, граничные |=6
		for(Ind<3> d; d^=Ind<3>(2); ++d) nodes[i] |= nodes[i+d]&1?2:4;
	}
	for(Ind<3> i; i^=nodes.bbox()-ind(1); ++i){ // расширяем множество граничных tile-ов наружу |=8
		if((nodes[i]&6)==6) for(Ind<3> d; d^=Ind<3>(3); ++d){ 
				Ind<3> j = i+d-ind(1);
				if(ind(0)<=j && j<nodes.bbox()) nodes[j] |= 8;
			}
	}
	// проходим по всем tile, считаем число внутренних (для расчета числа атомов), для граничных поверяем все атомы,
	// если хотя бы один атом внутри - учитываем его, добавлем tile-ы
	Mesh<int, 3> tiles_pos; tiles_pos.init(nodes.bbox()-ind(1)); tiles_pos.fill(-1); 
	int last_tile = 0, full_tile_len = lats[lat].sublats.size()*tile_sz.prod(), start_tile = tiles.size();
	for(Ind<3> i; i^=nodes.bbox()-ind(1); ++i){
		if((nodes[i]&10)==0) continue; // 2+8
		tile_t tile; tile.magn_count = 0;
		tile.plat = &(lats[lat]);
		tile.base_r = nodes.pos2coord(i);
		if(nodes[i]&2){ // внутренний tile
			tiles_pos[i] = last_tile++;
			tile.magn_count = tile.plat->tile_sz.prod()*tile.plat->sublats.size();
			tiles.push_back(tile);
			continue;
		}
		std::vector<bool> usage(full_tile_len, false); bool use = false;
		for(uint32_t sl=0; sl<lats[lat].sublats.size(); ++sl){ 
			for(Ind<3> pos; pos^=tile_sz; ++pos) if(fig.check(tile.coord(pos, sl))){
					usage[tile.plat->pos2idx(pos, sl, 0)] = true;
					use = true; tile.magn_count++;
				}
		}
		if(use){
			tiles_pos[i] = last_tile++;
			tiles.push_back(tile);
			tiles.back().usage.swap(usage);
		} 
	}
	
	// устанавливаем связи между tile
	for(Ind<3> i; i^=tiles_pos.bbox()-ind(1); ++i){
		if(tiles_pos[i]==-1) continue;
		tile_t &tile = tiles[start_tile+tiles_pos[i]];
		for(Ind<3> d; d^=Ind<3>(3); ++d){
			Ind<3> j = i+d-ind(1);
			// где то тут должны быть ПГУ
			tile.nb[d[0]+3*d[1]+9*d[2]] = (ind(0)<=j && j<tiles_pos.bbox() && tiles_pos[j]>=0)? 
										   tiles_pos[j]-tiles_pos[i] : tile_t::tile_off;			   
		}
	}
}
//------------------------------------------------------------------------------
void MagneticData::set_interface1(int lat1, int sl_mask1, int lat2, int sl_mask2, float max_link_len, 
								  std::function<void(Vecf<3>, float&, Vecf<3>&)> app_func){
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // внешний цикл по тайлам, t1 --- модифицируемый тайл
		if(lat1>=0 && t1->plat!=&(lats[lat1])) continue;
		Vecf<3> c1 = t1->center(); float r1 = (t1->plat->tile_sz&t1->plat->step).abs()*.5; // характерный размер tile
		for(auto t2=tiles.begin(); t2!=tiles.end(); ++t2){ // внутренний цикл по тайлам
			if(t1->plat==t2->plat || (lat2>=0 && t2->plat!=&(lats[lat2])) 
			   || r1+(t2->plat->tile_sz&t2->plat->step).abs()*.5+max_link_len < (c1-t2->center()).abs()) continue;
			const std::vector<MagneticSubLattice>& sublats1 = t1->plat->sublats, sublats2 = t2->plat->sublats;
			Ind<2> sl_sz(sublats1.size(), sublats2.size());
			for(Ind<2> sl; sl^=sl_sz; ++sl){ // двойной цикл по подрешеткам
				if((sl[0]&sl_mask1)==0 || (sl[1]&sl_mask2)==0) continue;				
				for(Ind<3> pos1; pos1^=t1->plat->tile_sz; ++pos1){  // внешний цикл по ячейкам в тайлах     
					app_DM_link_t appl; appl.src = t1->plat->pos2idx(pos1, sl[0], 0);
					if(t1->usage.size() && !t1->usage[appl.src]) continue;
					Vecf<3> r1 = t1->coord(pos1, sl[0]);
					for(Ind<3> pos2; pos2^=t2->plat->tile_sz; ++pos2){ // внутренний цикл по ячейкам в тайлах     
						appl.dst = t2->plat->pos2idx(pos2, sl[1], 0);
						if(t2->usage.size() && !t2->usage[appl.dst]) continue;
						Vecf<3> r12 = t1->coord(pos2, sl[1])-r1; 
						if(r12.abs()<=max_link_len){
							appl.J = 0.f; appl.D = Vecf<3>(); 
							app_func(r12, appl.J, appl.D);
							if(appl.J && appl.D) t1->app_dm_links.push_back(appl);
							else if(appl.J) t1->app_links.push_back(appl);
						}
					} // конец внутреннего цикла по ячейкам в тайлах
				} // конец внешнего цикла по ячейкам в тайлах
			} // конец двойного цикла по подрешеткам
		} // конец внутреннего цикла по тайлам
	} // конец внешнего цикла по тайлам
}
void MagneticData::set_interface2(int lat1, int sl_mask1, int lat2, int sl_mask2, float max_link_len, 
								  std::function<void(Vecf<3>, float&, Vecf<3>&)> app_func){
	set_interface1(lat1, sl_mask1, lat2, sl_mask2, max_link_len, app_func);
	set_interface1(lat2, sl_mask2, lat1, sl_mask1, max_link_len, app_func);
}
//------------------------------------------------------------------------------
void MagneticData::set_app_H(int lat, int sl_mask, std::function<Vecf<3>(Vecf<3>)> app_func){
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		if(lat>=0 && t1->plat!=&(lats[lat])) continue;
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl){ // цикл по подрешеткам
			if((sl&sl_mask)==0) continue;				
			for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos){  // цикл по ячейкам в тайлах     
				app_H_t H; H.src = t1->plat->pos2idx(pos, sl, 0);
				if(t1->usage.size() && !t1->usage[H.src]) continue;
				H.H = app_func(t1->coord(pos, sl));
				if(H.H) t1->app_H.push_back(H);
			} // конец цикла по ячейкам в тайле
		} // конец цикла по подрешеткам
	} // конец цикла по тайлам
}
//------------------------------------------------------------------------------		
void MagneticData::mem_init(size_t szT_, size_t Nstages_){
	delete [] data;
	szT = szT_; Nstages = Nstages_;
	size_t tot_sz = 0; magn_count = 0;
	for(auto t=tiles.begin(); t!=tiles.end(); ++t) tot_sz += t->plat->tile_sz.prod()*t->plat->sublats.size()*szT;
	data = new char[tot_sz*Nstages]; tot_sz = 0;
	for(auto t=tiles.begin(); t!=tiles.end(); ++t){ 
		t->data = data+tot_sz; size_t Nm = t->plat->tile_sz.prod()*t->plat->sublats.size();
		tot_sz += Nm*Nstages;
		if(t->usage.empty()) magn_count += Nm;
		else for(auto i=t->usage.begin(); i!=t->usage.end(); ++i) if(*i) magn_count++;
	}
}
//------------------------------------------------------------------------------
void MagneticData::magn_init(int lat, int sl_mask, MagneticBaseIC& IC, std::function<void(Vecf<3>, char*)> conv, int stage){
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		if(lat>=0 && t1->plat!=&(lats[lat])) continue;
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl){ // цикл по подрешеткам
			if((sl&sl_mask)==0) continue;				
			for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos){  // цикл по ячейкам в тайлах     
				if(!t1->check(pos, sl)) continue;
				Vecf<3> m = IC(t1->coord(pos, sl));
				if(conv) conv(m, t1->data+t1->plat->pos2idx(pos, sl, stage)*szT);
				else t1->m<Vecf<3> >(pos, sl, stage) = m;
			} // конец цикла по ячейкам в тайле
		} // конец цикла по подрешеткам
	} // конец цикла по тайлам	
}
//------------------------------------------------------------------------------		
//   dump/load data
//------------------------------------------------------------------------------		
void MagneticSubLattice::dump(aiw::IOstream &S) const {
	S<coord<Ms<gamma<alpha<K1sz<K3sz;
	S.write(K1, K1sz*sizeof(MagneticSubLattice::aniso_t));
	S.write(K3, K3sz*sizeof(MagneticSubLattice::aniso_t));
	S<links;
}
void MagneticSubLattice::load(aiw::IOstream &S){
	S>coord>Ms>gamma>alpha>K1sz>K3sz;
	S.read(K1, K1sz*sizeof(MagneticSubLattice::aniso_t));
	S.read(K3, K3sz*sizeof(MagneticSubLattice::aniso_t));
	S>links;
}
//------------------------------------------------------------------------------		
void MagneticData::dump_head(aiw::IOstream &S) const {
	uint64_t data_format = 1704; // ???
	S<head<data_format<szT<Nstages<magn_count<int(lats.size());
	for(uint32_t i=0; i<lats.size(); i++){
		const lattice_t &l = lats[i];
		S<l.step<l.tile_sz<l.base_r<l.orts<int(l.sublats.size());
		for(uint32_t k=0; k<l.sublats.size(); ++k) l.sublats[k].dump(S);
	}
	S<int(tiles.size());
	for(auto t=tiles.begin(); t!=tiles.end(); ++t) 
		S<int(t->plat-&lats[0])<t->nb<t->usage<t->base_r<t->magn_count<t->app_links<t->app_dm_links<t->app_H;
}
//------------------------------------------------------------------------------
void MagneticData::load_head(aiw::IOstream &S, size_t szT_, size_t Nstages_){
	uint64_t data_format; int sz;
	S>head>data_format>szT>Nstages>magn_count>sz; lats.resize(sz);
	for(uint32_t i=0; i<lats.size(); i++){
		lattice_t &l = lats[i]; 
		S>l.step>l.tile_sz>l.base_r>l.orts>sz; l.sublats.resize(sz);
		for(uint32_t k=0; k<l.sublats.size(); ++k) l.sublats[k].load(S);
	}
	S>sz; tiles.resize(sz);
	for(auto t=tiles.begin(); t!=tiles.end(); ++t){ 
		S>sz; t->plat = &(lats[sz]);
		S>t->nb>t->usage>t->base_r>t->magn_count>t->app_links>t->app_dm_links>t->app_H;
	}
}
//------------------------------------------------------------------------------
void MagneticData::dump_data(aiw::IOstream &S, bool pack, std::function<void(const char*, Vecf<3>&)> conv, int stage) const {
	S<time<Hext<pack<stage; Vecf<3> buf[4096]; int cursor=0;
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl) 
			for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos) 
				if(t1->check(pos, sl)){
					if(conv) conv(t1->data+t1->plat->pos2idx(pos, sl, stage)*szT, buf[cursor++]);
					else buf[cursor++] = t1->m<Vecf<3> >(pos, sl, stage);
					if(cursor==4096){ S.write(buf, sizeof(Vecf<3>)*cursor); cursor = 0; }
				}
	} // конец цикла по тайлам	
	if(cursor) S.write(buf, sizeof(Vecf<3>)*cursor);
}
//------------------------------------------------------------------------------
void MagneticData::load_data(aiw::IOstream &S, std::function<void(const Vecf<3>&, char*)> conv, int stage_){
	Vecf<3> buf[4096]; int stage, cursor=0, buf_sz=0; uint64_t read_sz=0; uint8_t pack;
	S>time>Hext>pack>stage; if(stage_>=0) stage = stage_;
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl) 
			for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos) 
				if(t1->check(pos, sl)){
					if(cursor==buf_sz){ 
						buf_sz = magn_count-read_sz<4096?magn_count-read_sz:4096; 
						S.read(buf, sizeof(Vecf<3>)*buf_sz); 
						cursor = 0; read_sz += buf_sz;
					}
					if(conv) conv(buf[cursor++], t1->data+t1->plat->pos2idx(pos, sl, stage)*szT);
					else t1->m<Vecf<3> >(pos, sl, stage) = buf[cursor++];
				}
	} // конец цикла по тайлам	
	if(cursor) S.write(buf, sizeof(Vecf<3>)*cursor);
}
//------------------------------------------------------------------------------
//   for vizualization
//------------------------------------------------------------------------------		
void MagneticData::get_coords(int lat, int sl_mask, std::vector<Vecf<3> > &p) const {
	p.clear();
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		if(lat>=0 && t1->plat!=&(lats[lat])) continue;
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl) 
			if(sl&sl_mask)
				for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos) 
					if(t1->check(pos, sl)) p.push_back(t1->coord(pos, sl));
	} // конец цикла по тайлам	
}
//------------------------------------------------------------------------------
void MagneticData::get_links(std::vector<Vec<2, uint64_t> > &p) const { //только для всех магнитных моментов ???
	p.clear(); 
	std::vector<bool> ulinks(magn_count, false);
	std::vector<uint64_t> offsets(tiles.size()); uint64_t cursor = 0; 
	for(uint32_t i=0; i<tiles.size(); ++i){ offsets[i] = cursor; cursor += tiles[i].magn_count; }
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		Vec<2, uint32_t> app;
		for(uint32_t sl=0; sl<sublats.size(); ++sl){ // цикл по подрешеткам
			for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos){  // цикл по ячейкам в тайлe
				uint32_t idx1 = t1->plat->pos2idx(pos, sl, 0);
				if(t1->usage.size() && !t1->usage[idx1]) continue;
				for(auto l=sublats[sl].links.begin(); l!=sublats[sl].links.end(); ++l){ // цикл по регулярным связям в ячейке
					Ind<3> pos2 = pos+l->offset; int gID = 13, mul = 1;
					for(int k=0; k<3; k++){ // коррекция pos2 и определение номера соседнего tile
						if(pos2[k]<0){ gID -= mul; pos2[k] += t1->plat->tile_sz[k]; }
						else if(pos2[k]>=t1->plat->tile_sz[k]){ gID += mul; pos2[k] -= t1->plat->tile_sz[k]; }
						mul *= 3;
					}
					if(t1->nb[gID]==t1->tile_off) continue;
					auto t2 = t1+t1->nb[gID]; 
					uint32_t idx2 = t2->plat->pos2idx(pos2, l->sl, 0);
					uint64_t dst = offsets[t2-tiles.begin()];
					if(t2->usage.empty()) dst += idx2;
					else { 
						if(!t2->usage[idx2]) continue;
						for(uint32_t k=0; k<idx2; ++k) if(t2->usage[k]) ++dst;
					}
					if(!ulinks[dst]) p.push_back(Vec<2, uint64_t>(cursor, dst));
				} // конец цикла по регулярным связям в ячейке
				while(app[0]<t1->app_links.size() && t1->app_links[app[0]].src<idx1) ++app[0];
				while(app[0]<t1->app_links.size() && t1->app_links[app[0]].src==idx1){ 
					auto l = t1->app_links[app[0]++]; auto t2 = t1+l.tile; 
					uint64_t dst = offsets[t2-tiles.begin()];
					if(t2->usage.empty()) dst += l.dst;
					else for(uint32_t k=0; k<l.dst; ++k) if(t2->usage[k]) ++dst;
					if(!ulinks[dst]) p.push_back(Vec<2, uint64_t>(cursor, dst));
				}
				while(app[1]<t1->app_dm_links.size() && t1->app_dm_links[app[1]].src<idx1) ++app[1];
				while(app[1]<t1->app_dm_links.size() && t1->app_dm_links[app[1]].src==idx1){ 
					auto l = t1->app_dm_links[app[1]++]; auto t2 = t1+l.tile; 
					uint64_t dst = offsets[t2-tiles.begin()];
					if(t2->usage.empty()) dst += l.dst;
					else for(uint32_t k=0; k<l.dst; ++k) if(t2->usage[k]) ++dst;
					if(!ulinks[dst]) p.push_back(Vec<2, uint64_t>(cursor, dst));
				}
				ulinks[cursor++] = true;				
			} // конец цикла по ячейкам в тайле
		} // конец цикла по подрешеткам
	} // конец цикла по тайлам
}
//------------------------------------------------------------------------------
void MagneticData::get_magns(int lat, int sl_mask, std::vector<Vecf<3> > &p, 
							 std::function<void(const char*, Vecf<3>&)> conv, int stage) const {
	p.clear(); Vecf<3> m;
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		if(lat>=0 && t1->plat!=&(lats[lat])) continue;
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl) 
			if(sl&sl_mask)
				for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos) 
					if(t1->check(pos, sl)){ 
						if(conv){ conv(t1->data+t1->plat->pos2idx(pos, sl, stage)*szT, m); p.push_back(m); }
						else p.push_back(t1->m<Vecf<3> >(pos, sl, stage));
					}
	} // конец цикла по тайлам	
}
//------------------------------------------------------------------------------
void MagneticData::get_pack_magns(int lat, int sl_mask, std::vector<uint16_t> &p, 
								  std::function<void(const char*, uint16_t&)> conv, int stage) const {
	p.clear(); uint16_t m;
	for(auto t1=tiles.begin(); t1!=tiles.end(); ++t1){ // цикл по тайлам
		if(lat>=0 && t1->plat!=&(lats[lat])) continue;
		const std::vector<MagneticSubLattice>& sublats = t1->plat->sublats;
		for(uint32_t sl=0; sl<sublats.size(); ++sl) 
			if(sl&sl_mask)
				for(Ind<3> pos; pos^=t1->plat->tile_sz; ++pos) 
					if(t1->check(pos, sl)){ 
						if(conv){ conv(t1->data+t1->plat->pos2idx(pos, sl, stage)*szT, m); p.push_back(m); }
						else p.push_back(t1->m<uint16_t>(pos, sl, stage));
					}
	} // конец цикла по тайлам	
}
//------------------------------------------------------------------------------
