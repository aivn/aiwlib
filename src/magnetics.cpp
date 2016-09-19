/**
 * Copyright (C) 2016 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include "../include/aiwlib/mesh"
#include "../include/aiwlib/magnetics"
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
	nodes.init(((b-a)/step).round()+Indx<3>(1), a, b+step);
	nodes.fill(0);
	
	for(Ind<3> i; i^=nodes.bbox(); ++i) if(fig.check(nodes.pos2coord(i))) nodes[i] = 1; // отмечаем все узлы попавшие в fig
	for(Ind<3> i; i^=nodes.bbox()-ind(1); ++i){ // внутренние |=2, внешние |=4, граничные |=6
		for(Ind<3> d; d^=Ind<3>(2); ++d) nodes[i] |= nodes[i+d]&1?2:4;
	}
	for(Ind<3> i; i^=nodes.bbox()-ind(1); ++i){ // расширяем множество граничных tile-ов наружу |=8
		if(nodes[i]&6==6) for(Ind<3> d; d^=Ind<3>(3); ++d){ 
				Ind<3> j = i+d-ind(1);
				if(ind(0)<=j && j<nodes.bbox()) nodes[j] |= 8;
			}
	}
	// проходим по всем tile, считаем число внутренних (для расчета числа атомов), для граничных поверяем все атомы,
	// если хотя бы один атом внутри - учитываем его, добавлем tile-ы
	Mesh<int, 3> tiles_pos; tiles_pos.init(nodes.bbox()-ind(1)); tiles_pos.fill(-1); 
	int last_tile = 0, full_tile_len = lats[lat].sublats.size()*tile_sz.prod(), start_tile = tiles.size();
	for(Ind<3> i; i^=nodes.bbox()-ind(1); ++i){
		if(nodes[i]&10==0) continue; // 2+8
		tile_t tile; 
		tile.plat = &(lats[lat]);
		tile.base_r = nodes.pos2coord(i);
		if(nodes[i]&2){ // внутренний tile
			total_count += full_tile_len;
			tiles_pos[i] = last_tile++;
			tiles.push_back(tile);
			continue;
		}
		std::vector<bool> usage(full_tile_len, false); bool use = false;
		for(uint32_t sl=0; sl<lats[lat].sublats.size(); ++sl){ 
			for(Ind<3> pos; pos^=tile_sz; ++pos) if(fig.check(tile.coord(pos, sl))){
					usage[tile.plat->pos2idx(pos, sl, 0)] = true;
					use = true; total_count++; 
				}
		}
		if(one){
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
void MagneticData::set_interface(int lat1, int sl1, int lat2, int sl2, int max_count, float max_len, 
								 void (*setJDfunc)(Vecf<3>, Vecf<3>, float&, Vecf<3>&)){
}
//------------------------------------------------------------------------------
// void set_app_H(int lat, int sl, const BaseInitConditions<Vecf<3> > &); ???
void MagneticData::set_app_H(int lat, int sl, Vecf<3> (*setH)(Vecf<3>)){
}
//------------------------------------------------------------------------------		
void MagneticData::mem_init(int szT, int Nstages_){
}
//------------------------------------------------------------------------------
void MagneticData::magn_init(int lat, int sublat, BaseFunctor<Vecf<3>, Vecf<3> > &,
							 void (*conv)(const Vecf<3>&, char*)=nullptr, int stage=0){
}
//------------------------------------------------------------------------------		
void MagneticData::dump_head(aiw::IOstream &) const {
}
//------------------------------------------------------------------------------
void MagneticData::load_head(aiw::IOstream &){
}
//------------------------------------------------------------------------------
void MagneticData::dump_data(aiw::IOstream &, bool pack, void (*conv)(const char*, Vecf<3>&)=nullptr, int stage=0) const {
}
//------------------------------------------------------------------------------
void MagneticData::load_data(aiw::IOstream &, bool pack, void (*conv)(const Vecf<3>&, char*)=nullptr, int stage=0){
}
//------------------------------------------------------------------------------
void MagneticData::get_coords(int lat, int sl, std::vector<Vecf<3> >&) const {
}
//------------------------------------------------------------------------------
void MagneticData::get_links(std::vector<Vec<2, uint64_t> >&) const { //только для всех магнитных моментов ???
}
//------------------------------------------------------------------------------
void MagneticData::get_magns(int lat, int sl, std::vector<Vecf<3> >&, 
							 void (*conv)(const char*, Vecf<3>&)=nullptr, int stage=0) const {
}
//------------------------------------------------------------------------------
void MagneticData::get_pack_magns(int lat, int sl, std::vector<Vecf<3> >&, 
								  void (*conv)(const char*, char*)=nullptr, int stage=0) const {
}
//------------------------------------------------------------------------------
