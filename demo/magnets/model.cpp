#include "model.hpp"
//------------------------------------------------------------------------------
Model::Model():data(1){} // одна решетка на область
//------------------------------------------------------------------------------
void Model::init(){
	magn_BCC_init(data.config_lat(0), J, alpha, gamma); // настраиваем единственную решетку как ОЦК
	data.config_lat(0)[0].add_aniso1(nK, K);
	data.config_lat(0)[1].add_aniso1(nK, K);
	data.set_geometry(0, Figure(),
					  vecf(1),  // step - размеры ячейки		
					  tile_sz,  // размеры tile в ячейках
					  max_tsz,  // максимальный размер области, в тайлах
					  periodic_bc, // битовая маска периодических граничных условий
					  vecf(0),  // base_r, координаты левого нижнего угла базовой ячейки в глобальной системе координат
					  ind(0));  // base_cl, положение базовой ячейки в области (задается в ячейках)
	data.mem_init(sizeof(Vecf<3>), // szT --- размер одного магнитного момента
				  4);              // Nstages --- число стадий численной схемы 
	MagnUniformIC IC; IC.m0 = Ma/Ma.abs();
	data.magn_init(0,   // решетка
				   1,   // sl_mask=1<<0, битовая маска указывающая какие подрешетки инициализируем 
				   IC); // начальные условия
	IC.m0 = Mb/Mb.abs();
	data.magn_init(0,   // решетка
				   2,   // sl_mask=1<<1, битовая маска указывающая какие подрешетки инициализируем 
				   IC); // начальные условия

	dfa.init(df_rank); dfb.init(df_rank); df.init(df_rank);
	t = 0.;
}
//------------------------------------------------------------------------------
void Model::calc_RK4(int Nit){
	for(int it=0; it<Nit; it++){
		t += h;
	}
}
//------------------------------------------------------------------------------
void Model::dump_head(){
	//		void get_coords(int lat, int sl, std::vector<Vecf<3> >&) const;
}
//------------------------------------------------------------------------------
void Model::dump_frame(){
	//	void get_magns(int lat, int sl, std::vector<Vecf<3> >&, 
	//			   std::function<void(const char*, Vecf<3>&)> conv=0, int stage=0) const;
	//	void get_pack_magns(int lat, int sl, std::vector<uint16_t>&, 
	//						std::function<void(const char*, uint16_t&)> conv=0, int stage=0) const;
}
//------------------------------------------------------------------------------

