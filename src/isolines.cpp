/**
 * Copyright (C) 2017 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include <map>
#include <algorithm>
#include "../include/aiwlib/isolines"
using namespace aiw;

//------------------------------------------------------------------------------
struct slot_t{ 
	std::list<Vecf<2> >* l; int mode; 
	slot_t(std::list<Vecf<2> >* l_=nullptr, int mode_=0): l(l_), mode(mode_){}  
	void append(const Vecf<2>& p){ if(mode) l->push_back(p); else l->push_front(p); }
};
typedef std::pair<Vecf<2>, Vecf<2> > segment_t;
//------------------------------------------------------------------------------
void aiw::IsoLines::init(const aiw::Mesh<float, 2> &arr, double z0, double dz, bool logscale){
	std::map<int, std::list<segment_t> > table;
	double _dz = logscale?1./log(dz):1./dz;

	for(int y=1; y<arr.bbox()[1]; ++y){ // цикл по строкам исходного массива
		double v00 = arr[ind(0, y-1)], v01 = arr[ind(0, y)];
		for(int x=1; x<arr.bbox()[0]; ++x){ // цикл вдоль строки исходного массива
			double v10 = arr[ind(x, y-1)], v11 = arr[ind(x, y)]; // получили значения в углах ячейки
			double v_min = std::min({v00, v01, v10, v11}), v_max = std::max({v00, v01, v10, v11}); 
			if(logscale && v_min<1e-30) v_min = 1e-30;
			if(logscale && v_max<1e-30) v_max = 1e-30;
			if(v_min==v_max) continue;
			Vec<2> a = arr.cell_angle(ind(x, y), 0), b = arr.cell_angle(ind(x, y), 1); 
			double cX0 = v10==v00?0.:arr.step[0]/(v10-v00), cX1 = v11==v01?0.:arr.step[0]/(v11-v01);
			double cY0 = v01==v00?0.:arr.step[1]/(v01-v00), cY1 = v11==v10?0.:arr.step[1]/(v11-v10);
			int i_min = ::ceil(logscale?log(v_min/z0)*_dz:(v_min-z0)*_dz),
				i_max = ::floor(logscale?log(v_max/z0)*_dz:(v_max-z0)*_dz); 

			for(int i=i_min; i<=i_max; ++i){ // цикл по уровням
				double v = logscale?z0*pow(dz, i):z0+i*dz;
				if((v00<=v && v<=v10)||(v10<=v && v<=v00)){ 
					Vecf<2> p0(a[0]+(v-v00)*cX0, a[1]); 
					if     ((v00< v && v<=v01)||(v01<=v && v< v00)) table[i].push_back(segment_t(p0, vecf(a[0], a[1]+(v-v00)*cY0)));
					else if((v10< v && v<=v11)||(v11<=v && v< v10)) table[i].push_back(segment_t(p0, vecf(b[0], a[1]+(v-v10)*cY1)));
					else if((v01<=v && v<=v11)||(v11<=v && v<=v01)) table[i].push_back(segment_t(p0, vecf(a[0]+(v-v01)*cX1, b[1])));
				}
				if((v00<v && v<=v01)||(v01<=v && v<v00)){ 
					Vecf<2> p0(a[0], a[1]+(v-v00)*cY0); 
					if     ((v01< v && v<=v11)||(v11<=v && v< v01)) table[i].push_back(segment_t(p0, vecf(a[0]+(v-v01)*cX1, b[1])));
					else if((v10< v && v<=v11)||(v11<=v && v< v10)) table[i].push_back(segment_t(p0, vecf(b[0], a[1]+(v-v10)*cY1)));
				}
				if(((v10< v && v<=v11)||(v11<=v && v< v10))&&((v01< v && v< v11)||(v11< v && v< v01)))
					table[i].push_back(segment_t(vecf(b[0], a[1]+(v-v10)*cY1), vecf(a[0]+(v-v01)*cX1, b[1])));
			} // конец цикла по уровням
			v00 = v10; v01 = v11;
		} // конец цикла вдоль строки исходного массива
	} // конец цикла по строкам исходного массива

	for(auto I = table.begin(); I!=table.end(); ++I){ // цикл по уровням
		std::list<segment_t>& src_level = I->second; std::list<std::list<Vecf<2> > > dst_level;
		typename std::map<Vecf<2>, slot_t> slots;
		for(auto J=src_level.begin(); J!=src_level.end(); ++J){ // цикл по сегментам на уровне
			auto S0 = slots.find(J->first), S1 = slots.find(J->second); 
			if(S0==slots.end() && S1==slots.end()){ 
				dst_level.push_back(std::list<Vecf<2> >()); 
				dst_level.back().push_front(J->first ); slots[J->first]  = slot_t(&dst_level.back(), 0);  
				dst_level.back().push_back( J->second); slots[J->second] = slot_t(&dst_level.back(), 1);  
			}
			else if(S1==slots.end()){ S0->second.append(J->second); slots[J->second] = S0->second; slots.erase(J->first);  }
			else if(S0==slots.end()){ S1->second.append(J->first ); slots[J->first]  = S1->second; slots.erase(J->second); }
			else{ 
				slot_t s0 = S0->second, s1 = S1->second; slots.erase(J->first); slots.erase(J->second);
				if(s0.l==s1.l) s0.append(J->second); 
				else{
					std::list<Vecf<2> > *l0 = s0.l, *l1 = s1.l; 
					if     (!s0.mode &&  s1.mode){ l0 = s1.l; l1 = s0.l; }
					else if( s0.mode &&  s1.mode) l1->reverse(); 
					else if(!s0.mode && !s1.mode) l0->reverse(); 
					l0->splice(l0->end(), *l1);
					for(auto S=slots.begin(); S!=slots.end(); ++S)
						if(S->second.l==l1){ S->second.l = l0; S->second.mode = 1; }
						else if(S->second.l==l0) S->second.mode = 0; 						
					for(auto S=dst_level.begin(); S!=dst_level.end(); ++S) 
						if(&(*S)==l1){ dst_level.erase(S); break; }
				}
			}
		}  // конец цикла по сегментам на уровне
		for(auto J=dst_level.begin(); J!=dst_level.end(); ++J){
			levels.push_back(logscale?z0*pow(dz, I->first):z0+I->first*dz);
			lines.push_back(std::vector<Vecf<2> >()); lines.back().reserve(J->size());
			plines.push_back(&(lines.back()));
			for(auto K=J->begin(); K!=J->end(); ++K) lines.back().push_back(*K);
		}
	}  // конец цикла по уровням
}
//------------------------------------------------------------------------------
