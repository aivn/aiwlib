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

	Mesh<char, 3> nodes(...); // сетка узлов и tile-ов
	nodes.fill(0);

	
	// отмечаем все узлы попавшие в fig - nodes[i] = 1
	// находим все внутренние tile (nodes[i] |= 2) и граничные (у которых хотя бы один узел внутри, nodes[i] |= 4)
	// расширяем множество граничных tile-ов наружу (nodes[i] |= 8)
	// проходим по всем tile, считаем число внутренних (для расчета числа атомов), для граничных поверяем все атомы,
	//          если хотя бы один атом внутри - учитываем его
	// завести в tile_t std::vector<bool> указывающий на активность атома (иначе теряется информация о геометрии)!
	// сделать общие для всех tile-ов массивы доп связей, полей и т.д.?

	tile_t tile; 
	tile.plat = &(lats[lat]);
	// добавлем tile-ы
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
