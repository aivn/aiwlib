/**
 * Copyright (C) 2015, 2017, 2020-2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <omp.h>
// #include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/qplt/imaging"
#include "../../include/aiwlib/vec"
using namespace aiw;

//------------------------------------------------------------------------------
//   class QpltImage
//------------------------------------------------------------------------------
void aiw::QpltImage::fill(int color){
	uint32_t* ptr = (uint32_t*)&(buf[0]);
	for(int i=0, sz=Nx*Ny; i<sz; i++) ptr[i] = color;
}
//------------------------------------------------------------------------------
// прорисовывает цветом color пиксели у которых соседи имеют разные цвета, остальное делает черным
void aiw::QpltImage::grad_bw(int color){
	std::string buf2 = buf;
#pragma omp parallel for
	for(int y=0; y<Ny;  y++){
		const uint32_t *src = ((const uint32_t*)&(buf2[0]))+4*y*Nx, *src1, *src2;
		uint32_t  *dst = (uint32_t*)&(buf[0])+4*y*Nx;
		if(y==0)      src1 = nullptr; else src1 = src-Nx; 
		if(y==Ny-1) src2 = nullptr; else src2 = src+Nx;
		for(int x=0; x<Nx;  x++){
			uint32_t c = src[x];
			if( (x && c!=src[x-1]) || (x<Nx-1 && c!=src[x+1]) || (src1 && c!=src1[x]) || (src2 && c!=src2[x]) ) dst[x] = color;
			else dst[x] = 0;
		}
	}
}
std::string aiw::QpltImage::rgb888() const {
	const uint8_t* src = (const uint8_t*)&(buf[0]);
	std::string res(Nx*Ny*3, 0);   uint8_t *dst = (uint8_t*)&(res[0]);
	for(int i=0, sz=Nx*Ny; i<sz; i++) for(int k=0; k<3; k++) dst[3*i+k] = src[4*i+2-k];
	return res;
}
void aiw::QpltImage::dump2ppm(const char *path) const {
	FILE *fout = fopen(path, "wb"); std::string tmp = rgb888();
	fprintf(fout, "P6\n%i %i\n255\n", Nx, Ny);
	fwrite(tmp.c_str(), Nx*Ny, 3, fout);
	fclose(fout);
}
//------------------------------------------------------------------------------
//   class QpltColor
//------------------------------------------------------------------------------
std::map<std::string, std::vector<aiw::QpltColor::rgb_t> > aiw::QpltColor::table =
	{{"grey", {0, 0xFFFFFF}},
	 {"inv_grey", {0xFFFFFF, 0}},
	 {"black_red", {0, 0xFFFFFF, 0xFF0000}},
	 {"green_blue", {0x00FF00, 0xFFFFFF, 0x0000FF}},
	 {"neg_pos1", {0, 0xFF0000, 0xFF7F00, 0xFFFF00, 0xFFFFFF, 0xFF00FF, 0x0000FF, 0x00FFFF, 0x00FF00}},
	 {"neg_pos2", {0, 0xFFFF00, 0xFF7F00, 0xFF0000, 0xFFFFFF, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF}},
	 {"positive", {0xFFFFFF, 0xFF0000, 0xFF7F00, 0x00FFFF, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF, 0}},
	 {"rainbow",      {0, 0xFF0000, 0xFF7F00, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF, 0xFFFFFF}},
	 {"inv_rainbow",  {0xFFFFFF, 0xFF00FF, 0x0000FF, 0x00FFFF, 0x00FF00, 0xFFFF00, 0xFF7F00, 0xFF0000, 0}},
	 {"color",       {0xFF0000, 0xFF7F00, 0xFFFF00, 0x00FF00, 0x00FFFF, 0x0000FF, 0xFF00FF}}
	};
//------------------------------------------------------------------------------
void aiw::QpltColor::conf(aiw::QpltColor3D *c3D, int tmask) const {
	int i = c3D->up_bound = 0; for(auto s: pal){ c3D->pal[i] = (Vecf<3>(1.f)-s.to_vecf())|bool(tmask&(1<<i)); i++; } c3D->up_bound = i-1;
	c3D->mul = mul; c3D->min = min; c3D->max = max; c3D->logscale = logscale; c3D->modulus = modulus;
	c3D->nan_color = rgb_t(nan_color).to_vecf()|1.f;
}
//------------------------------------------------------------------------------
std::vector<std::string> aiw::QpltColor::get_pals(){
	std::vector<std::string> res; for(auto I: table) res.push_back(I.first);
	return res;
}
bool aiw::QpltColor::add_pal(const char* name, const std::vector<int>& pal){
	if(pal.size()<2 || table.find(name)!=table.end()) return false;
	auto &dst = table[name]; dst.reserve(pal.size()); for(auto I: pal) dst.push_back(I);
	return true;
}
void aiw::QpltColor::plot_pal(const char *pal, QpltImage& im, bool vertical){
	float step = 1./(vertical? im.Ny: im.Nx); QpltColor color(pal, 0., 1.);
	for(int y=0; y<im.Ny; y++) for(int x=0; x<im.Nx; x++) im.set_pixel(x, y, color(((vertical? y: x)+.5)*step)); 
}
QpltImage aiw::QpltColor::plot_pal(const char *pal, int Nx, int Ny, bool vertical){
	QpltImage im(Nx, Ny); plot_pal(pal, im, vertical); return im;
}
//------------------------------------------------------------------------------
bool aiw::QpltColor::init(const char *pal_, float min_, float max_, bool logscale_){
	auto Ipal = table.find(pal_);
	if(Ipal==table.end()) return false;
	logscale = logscale_; pal = Ipal->second; up_bound = pal.size()-1; reinit(min_, max_);
	if((arr_length && arr_width) && (arr_length0!=arr_length || arr_width0!=arr_width)){ arr_length0 = arr_length; arr_width0 = arr_width; arr_init(); }
	return true;
}
void aiw::QpltColor::reinit(float min_, float max_){
	if(modulus){
		float a = fabs(min_), b = fabs(max_);
		min_ = min_*max_<0? 0.f: std::min(a, b);
		max_ = std::max(a, b);
	}
	if(logscale && min_<=0) min_ = 1e-16;  // ???
	if(min_==max_){ min_ -= .5; max_ += .5; }
	min = min_; max = max_; mul = (logscale? 1./log(max/min) : 1./(max-min))*(pal.size()-1);
}
//------------------------------------------------------------------------------
//   QpltColor::arrows
//------------------------------------------------------------------------------
std::map<uint16_t, std::pair<uint16_t, uint32_t> > aiw::QpltColor::arr_table;
std::vector<aiw::QpltColor::arr_point_t> aiw::QpltColor::arr_data; // три байта заняты - две координаты точки (относительно центра стрелки) и цвет
int aiw::QpltColor::arr_length0 = 0, aiw::QpltColor::arr_width0 = 0;
//------------------------------------------------------------------------------
void aiw::QpltColor::arr_init(){
	arr_table.clear(); arr_data.clear();
	int R = arr_length0/2+1;  float rC = arr_width0*.5f; Ind<2> B;  // полудлина стрелки, полуширина стрелки, координаты конца стрелки
	for(B[1]=-R; B[1]<=R; B[1]++) for(B[0]=-R; B[0]<=R; B[0]++) if(B*B<=R*(R+1)){  // цикл по стрелкам, все возможные варианты
				uint32_t start = arr_data.size(), sz = 0;
				Vecf<2> C = -B + B*rC/R, BC = C-B;  // координаты центра круга на конце стрелки и бокс вмещающий стрелку
				Vecf<2> n1, n2;  float lBS2 = 0;  // нормали к образующим конуса и квадрат длины образующей 
				if(BC*BC>rC*rC){
					// r = C+rC*(c, s), c = cos(t), s = sin(t),  t in [0:2pi) --- параметрическое уравнение окружности
					// (r-B)*(c, s) = 0  --- уравнение образующей конуса  --->  BC0*c + BC1*s + rC = 0
					// https://www.wolframalpha.com/input/?i=solve%28a*sin%28x%29+%2B+b*cos%28x%29%2Bc%2C+x%29
					// float t1 = 2*atanf((BC[1]+sqrtf(BC*BC-rC*rC))/(BC[0]-rC)), t2 = 2*atanf((BC[1]-sqrtf(BC*BC-rC*rC))/(BC[0]-rC));
					float t1 = 2*atan2f(BC[1]+sqrtf(BC*BC-rC*rC), BC[0]-rC), t2 = 2*atan2f(BC[1]-sqrtf(BC*BC-rC*rC), BC[0]-rC);
					n1 = vecf(cosf(t1), sinf(t1));  if(n1*C<0) n1 = -n1;
					n2 = vecf(cosf(t2), sinf(t2));  if(n2*C<0) n2 = -n2;
					lBS2 = (C-n1*rC-B)*(C-n1*rC-B);
					// WOUT(B, C, t1, t2, n1, n2);
				}
				Ind<2> p;  float BR2 = .25f/std::max(float(B*B+4*B.abs()), rC*(rC+1));
				for(p[1]=-R; p[1]<=R; p[1]++) for(p[0]=-R; p[0]<=R; p[0]++){  // цикл по точкам бокса вмещающего стрелку
						Ind<2> Bp = p-B;
						if((p-C)*(p-C)<=rC*rC || (lBS2 && Bp*n1>=0 && Bp*n2>=0 && Bp*Bp<lBS2)){
							// arr_data.push_back((p[1]+128)<<16|(p[0]+128)<<8|(int(255*sqrtf(Bp*Bp/(B*B*4.f)))&0xFF));
							arr_data.push_back(arr_point_t({int8_t(p[0]), int8_t(p[1]), uint8_t(255*sqrtf(Bp*Bp*BR2))}));
							// arr_data.push_back(arr_point_t({int8_t(p[0]), int8_t(p[1]), 255}));
							sz++;
						} 
					}  // конец цикла по точкам бокса вмещающего стрелку
				arr_table[ uint16_t(B[1]+128)<<8|uint16_t(B[0]+128) ] = std::make_pair(uint16_t(sz), uint32_t(start));
			}  // конец цикла по стрелкам
	arr_data. shrink_to_fit();
	// тестирование
	/*
	QpltImage im(arr_length0*(1+arr_length0)+1, arr_length0*(1+arr_length0)+1);
	for(int iy=0; iy<=arr_length0; iy++)
		for(int ix=0; ix<=arr_length0; ix++){
			auto I = arr_table.find((uint16_t(iy-arr_length0/2+128)<<8)|uint16_t(ix-arr_length0/2+128));
			if(I==arr_table.end()) continue;
			int cx = ix*arr_length0+arr_length0/2, cy = iy*arr_length0+arr_length0/2;
			// im.set_pixel(cx, cy, 0xFFFFFF);
			for(uint32_t i=I->second.second; i<I->second.second+I->second.first; i++){
				arr_point_t p = arr_data[i];
				im.set_gray_pixel(cx+p.x, cy+p.y, p.c);
			}
		}
	im.dump2ppm("arrs.ppm");
	WOUT(arr_length0, arr_width0);
	*/
}
//------------------------------------------------------------------------------
void aiw::QpltColor::arr_plot(int x, int y, const float *v, QpltImage &im) const {
	float vlen = .5f*arr_length/sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
	int ax = std::max(-arr_length/2, std::min(arr_length/2, int(v[0]*vlen)));
	int ay = std::max(-arr_length/2, std::min(arr_length/2, int(v[1]*vlen)));
	const auto& arr = arr_table[uint16_t(128+ay)<<8|uint8_t(128+ax)];
	if(v[2]>0) for(uint16_t i=0; i<arr.first; i++){ const auto p = arr_data[arr.second+i];	im.set_gray_pixel_chk(x+p.x, y+p.y, p.c); }
	else if(v[2]<0) for(uint16_t i=0; i<arr.first; i++){ const auto p = arr_data[arr.second+i];	im.set_gray_pixel_chk(x+p.x, y+p.y, 255-p.c); }
	else for(uint16_t i=0; i<arr.first; i++){ const auto p = arr_data[arr.second+i];	im.set_gray_pixel_chk(x+p.x, y+p.y, 0); }
}
//------------------------------------------------------------------------------
/*
std::string aiw::QpltColor::pack() const {
	Packer P; P.dump(max_rgb, min, max, nan_color, pal_ID, cyclic, logscale, modulus, invert, magn);
	return P;
}
//------------------------------------------------------------------------------
void aiw::QpltColor::unpack(const std::string &S){
	Packer P(S); int pID = 0; P.load(max_rgb, min, max, nan_color, pID, cyclic, logscale, modulus, invert, magn);
	if(magn) magn_pal_init(max_rgb);
	init(all_paletters[pID], min, max);
}
*/
//------------------------------------------------------------------------------
