/**
 * Copyright (C) 2016-17 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include "../../include/aiwlib/magnets/lattice"
using namespace aiw;
//------------------------------------------------------------------------------
void aiw::MagnSubLattice::dump(IOstream &S) const {
	S<coord<Ms<gamma<alpha<K1sz<K3sz;
	S.write(K1, K1sz*sizeof(MagnAniso));
	S.write(K3, K3sz*sizeof(MagnAniso));
	S<links;
}
void aiw::MagnSubLattice::load(IOstream &S){
	S>coord>Ms>gamma>alpha>K1sz>K3sz;
	S.read(K1, K1sz*sizeof(MagnAniso));
	S.read(K3, K3sz*sizeof(MagnAniso));
	S>links;
}
//------------------------------------------------------------------------------
void aiw::magn_CC_init(std::vector<MagnSubLattice> &L, float J, float alpha, float gamma, float Ms){   // кубическая решетка
	L.resize(1);
	L[0].coord = vecf(0,0,0);

	L[0].add_link(ind(-1, 0, 0), 0, J);
	L[0].add_link(ind( 1, 0, 0), 0, J);
	L[0].add_link(ind( 0,-1, 0), 0, J);
	L[0].add_link(ind( 0, 1, 0), 0, J);
	L[0].add_link(ind( 0, 0,-1), 0, J);
	L[0].add_link(ind( 0, 0, 1), 0, J);

	L[0].alpha = alpha;
	L[0].gamma = gamma;
	L[0].Ms = Ms;
}
//------------------------------------------------------------------------------
void aiw::magn_BCC_init(std::vector<MagnSubLattice> &L, float J, float alpha, float gamma, float Ms){  // ОЦК решетка
	L.resize(2);
	L[0].coord = vecf( 0, 0, 0);
	L[1].coord = vecf(.5,.5,.5);

	L[0].add_link(ind( 0, 0, 0), 1, J);
	L[0].add_link(ind(-1, 0, 0), 1, J);
	L[0].add_link(ind( 0,-1, 0), 1, J);
	L[0].add_link(ind( 0, 0,-1), 1, J);
	L[0].add_link(ind( 0,-1,-1), 1, J);
	L[0].add_link(ind(-1, 0,-1), 1, J);
	L[0].add_link(ind(-1,-1, 0), 1, J);
	L[0].add_link(ind(-1,-1,-1), 1, J);

	L[1].add_link(ind( 0, 0, 0), 0, J);
	L[1].add_link(ind( 1, 0, 0), 0, J);
	L[1].add_link(ind( 0, 1, 0), 0, J);
	L[1].add_link(ind( 0, 0, 1), 0, J);
	L[1].add_link(ind( 0, 1, 1), 0, J);
	L[1].add_link(ind( 1, 0, 1), 0, J);
	L[1].add_link(ind( 1, 1, 0), 0, J);
	L[1].add_link(ind( 1, 1, 1), 0, J);

	for(int i=0; i<2; i++){
		L[i].alpha = alpha;
		L[i].gamma = gamma;
		L[i].Ms = Ms;
	}
}
//------------------------------------------------------------------------------
void aiw::magn_FCC3_init(std::vector<MagnSubLattice> &L, float J, float alpha, float gamma, float Ms){ // неполная ГЦК решетка
	L.resize(3);
	L[0].coord = vecf( 0,.5,.5);
	L[1].coord = vecf(.5, 0,.5);
	L[2].coord = vecf(.5,.5, 0);

	L[0].add_link(ind( 0, 0, 0), 1, J);
	L[0].add_link(ind(-1, 0, 0), 1, J);
	L[0].add_link(ind( 0, 1, 0), 1, J);
	L[0].add_link(ind(-1, 1, 0), 1, J);

	L[0].add_link(ind( 0, 0, 0), 2, J);
	L[0].add_link(ind(-1, 0, 0), 2, J);
	L[0].add_link(ind( 0, 0, 1), 2, J);
	L[0].add_link(ind(-1, 0, 1), 2, J);

	
	L[1].add_link(ind( 0, 0, 0), 0, J);
	L[1].add_link(ind( 0,-1, 0), 0, J);
	L[1].add_link(ind( 1, 0, 0), 0, J);
	L[1].add_link(ind( 1,-1, 0), 0, J);

	L[1].add_link(ind( 0, 0, 0), 2, J);
	L[1].add_link(ind( 0,-1, 0), 2, J);
	L[1].add_link(ind( 0, 0, 1), 2, J);
	L[1].add_link(ind( 0,-1, 1), 2, J);


	L[2].add_link(ind( 0, 0, 0), 1, J);
	L[2].add_link(ind( 0, 0,-1), 1, J);
	L[2].add_link(ind( 0, 1, 0), 1, J);
	L[2].add_link(ind( 0, 1,-1), 1, J);

	L[2].add_link(ind( 0, 0, 0), 0, J);
	L[2].add_link(ind( 0, 0,-1), 0, J);
	L[2].add_link(ind( 1, 0, 0), 0, J);
	L[2].add_link(ind( 1, 0,-1), 0, J);

	for(int i=0; i<3; i++){
		L[i].alpha = alpha;
		L[i].gamma = gamma;
		L[i].Ms = Ms;
	}
}
//------------------------------------------------------------------------------
void aiw::magn_FCC4_init(std::vector<MagnSubLattice> &L, float J, float alpha, float gamma, float Ms){ // полная ГЦК решетка
	L.resize(4);
	L[0].coord = vecf( 0,.5,.5);
	L[1].coord = vecf(.5, 0,.5);
	L[2].coord = vecf(.5,.5, 0);
	L[3].coord = vecf( 0, 0, 0);

	L[0].add_link(ind( 0, 0, 0), 1, J);
	L[0].add_link(ind(-1, 0, 0), 1, J);
	L[0].add_link(ind( 0, 1, 0), 1, J);
	L[0].add_link(ind(-1, 1, 0), 1, J);

	L[0].add_link(ind( 0, 0, 0), 2, J);
	L[0].add_link(ind(-1, 0, 0), 2, J);
	L[0].add_link(ind( 0, 0, 1), 2, J);
	L[0].add_link(ind(-1, 0, 1), 2, J);

	L[0].add_link(ind( 0, 0, 0), 3, J);
	L[0].add_link(ind( 0, 1, 0), 3, J);
	L[0].add_link(ind( 0, 0, 1), 3, J);
	L[0].add_link(ind( 0, 1, 1), 3, J);

	
	L[1].add_link(ind( 0, 0, 0), 0, J);
	L[1].add_link(ind( 0,-1, 0), 0, J);
	L[1].add_link(ind( 1, 0, 0), 0, J);
	L[1].add_link(ind( 1,-1, 0), 0, J);

	L[1].add_link(ind( 0, 0, 0), 2, J);
	L[1].add_link(ind( 0,-1, 0), 2, J);
	L[1].add_link(ind( 0, 0, 1), 2, J);
	L[1].add_link(ind( 0,-1, 1), 2, J);

	L[1].add_link(ind( 0, 0, 0), 3, J);
	L[1].add_link(ind( 1, 0, 0), 3, J);
	L[1].add_link(ind( 0, 0, 1), 3, J);
	L[1].add_link(ind( 1, 0, 1), 3, J);


	L[2].add_link(ind( 0, 0, 0), 1, J);
	L[2].add_link(ind( 0, 0,-1), 1, J);
	L[2].add_link(ind( 0, 1, 0), 1, J);
	L[2].add_link(ind( 0, 1,-1), 1, J);

	L[2].add_link(ind( 0, 0, 0), 0, J);
	L[2].add_link(ind( 0, 0,-1), 0, J);
	L[2].add_link(ind( 1, 0, 0), 0, J);
	L[2].add_link(ind( 1, 0,-1), 0, J);

	L[2].add_link(ind( 0, 0, 0), 3, J);
	L[2].add_link(ind( 1, 0, 0), 3, J);
	L[2].add_link(ind( 0, 1, 0), 3, J);
	L[2].add_link(ind( 1, 1, 0), 3, J);


	L[3].add_link(ind( 0, 0, 0), 0, J);
	L[3].add_link(ind( 0,-1, 0), 0, J);
	L[3].add_link(ind( 0, 0,-1), 0, J);
	L[3].add_link(ind( 0,-1,-1), 0, J);

	L[3].add_link(ind( 0, 0, 0), 1, J);
	L[3].add_link(ind(-1, 0, 0), 1, J);
	L[3].add_link(ind( 0, 0,-1), 1, J);
	L[3].add_link(ind(-1, 0,-1), 1, J);

	L[3].add_link(ind( 0, 0, 0), 2, J);
	L[3].add_link(ind(-1, 0, 0), 2, J);
	L[3].add_link(ind( 0,-1, 0), 2, J);
	L[3].add_link(ind(-1,-1, 0), 2, J);

	
	for(int i=0; i<4; i++){
		L[i].alpha = alpha;
		L[i].gamma = gamma;
		L[i].Ms = Ms;
	}
}
//------------------------------------------------------------------------------
