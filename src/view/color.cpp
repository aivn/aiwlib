/**
 * Copyright (C) 2015, 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/view/color"
using namespace aiw;

const float *all_paletters[] = {grey_pal, inv_grey_pal, black_red_pal, green_blue_pal, neg_pos1_pal,
								neg_pos2_pal, positive_pal, rainbow_pal, color_pal, inv_rainbow_pal, cyclic_pal, nullptr};
//------------------------------------------------------------------------------
std::string aiw::CalcColor::pack() const {
	Packer P; P.dump(max_rgb, min, max, nan_color, pal_ID, cyclic, logscale, modulus, invert, magn);
	return P;
}
//------------------------------------------------------------------------------
void aiw::CalcColor::unpack(const std::string &S){
	Packer P(S); int pID = 0; P.load(max_rgb, min, max, nan_color, pID, cyclic, logscale, modulus, invert, magn);
	if(magn) magn_pal_init(max_rgb);
	init(all_paletters[pID], min, max);
}
//------------------------------------------------------------------------------
void aiw::CalcColor::init(float const *pal_, float min_, float max_){
	pal = pal_;
	for(len_pal=0; pal[len_pal]>=0.; ++len_pal){}
	len_pal /= 3; len_pal--; // ???
	if(logscale && min_<=0) min_ = 1e-16;
	min = min_; max = max_; mul = logscale? 1./log(max_/min_) : 1./(max_-min_);
	for(pal_ID=0; all_paletters[pal_ID]; pal_ID++){
		bool res = true;
		for(int i=0; i<3+len_pal*3 && all_paletters[pal_ID][i]>=0; i++) if(pal[i]!=all_paletters[pal_ID][i]){
				res = false; break; }
		if(res && all_paletters[pal_ID][3+len_pal*3]==-1) break;
	}
}
//------------------------------------------------------------------------------
Ind<3> *aiw::CalcColor::magn_pal = nullptr;
void aiw::magn_pal_init(int max_rgb){
	if(aiw::CalcColor::magn_pal) return;
	sph_init_table(5);
	aiw::CalcColor::magn_pal = new Ind<3>[sph_cells_num(5)];
	for(size_t i=0; i<sph_cells_num(5); ++i){
		const Vec<3> &n = sph_cell(i, 5); Ind<3> &c = aiw::CalcColor::magn_pal[i]; // 1,0,0, 1,.5,0,   1,1,0, 0,1,0,   0,1,1, 0,0,1,   1,0,1,
		//       yellow
		//       (1,1,0)
		//  green       red
		// (0,1,0)    (1,0,0)
		//        blue
		//       (0,0,1)
		float phi = atan2f(n[1], n[0])*M_2_PI;
		/*
	  float S, V; if(n[2]<0){ S = 1.; V = 1+n[2]; } else { S = 1-n[2]; V = 1.; }
	  float Vmin = (1-S)*V, H = phi-floor(phi);
	  float a = (V-Vmin)*H;
				float Vinc = Vmin+a, Vdec = V-a;
				
				if(phi<-1) c = vec(Vmin, Vdec, Vinc); // (0,1,0) ==> (0,0,1) зеленый ==> синий
				else if(phi<0) c = vec(Vinc, Vmin, Vdec); // (0,0,1) ==> (1,0,0) синий ==> красный
				else if(phi<1)  c = vec(V, Vinc, Vmin); // (1,0,0) ==> (1,1,0) красный ==> желтый
				else c = vec(Vdec, V, Vmin); // (1,1,0) ==> (0,1,0) желтый ==> зеленый
				*/
				
		if(phi<-1){  c[1] = pow(-1-phi,1.2)*max_rgb; c[2] = max_rgb-c[1]; } // (0,1,0) ==> (0,0,1) зеленый ==> синий
		else if(phi<0){ c[2] = pow(-phi,.8)*max_rgb; c[0] = max_rgb-c[2]; } // (0,0,1) ==> (1,0,0) синий ==> красный
		else if(phi<1){ c[0] = max_rgb;  c[1] = phi*max_rgb;         } // (1,0,0) ==> (1,1,0) красный ==> желтый
		else{  c[1] = max_rgb;  c[0] = pow(2-phi, .4)*max_rgb; } // (1,1,0) ==> (0,1,0) желтый ==> зеленый
		
				
		// if(n[2]<0) c *= sqrt(1+n[2]); else c += (ind(max_rgb)-c)*n[2]*n[2]*n[2];
		if(n[2]<0) c *= 1+.85*n[2]; else c += (ind(max_rgb)-c)*n[2]*n[2]*.9;
		// return c; // *max_rgb; //>>ind(0)<<ind(max_rgb);
	}
}
//------------------------------------------------------------------------------
