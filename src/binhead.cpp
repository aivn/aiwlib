/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <cmath>
#include <sstream>
#include "../include/aiwlib/debug"
#include "../include/aiwlib/binhead"

//------------------------------------------------------------------------------
// Mesh:   head [[axis] [typeinfo] align out_value bmin bmax axisbit<<31|typeinfobit<<30|logscale]    D        szT     box
// Sphere: head [[axis] [typeinfo] align                     axisbit<<31|typeinfobit<<30|int(0)]      int(0)   szT  R
// Z-cube: head [[axis] [typeinfo] align           bmin bmax axisbit<<31|typeinfobit<<30|logscale]    1<<30|D  szT  R  
// AMR:    head [[axis] [typeinfo] align           bmin bmax axisbit<<31|typeinfobit<<30|logscale]    1<<31|D  szT  R  box
//------------------------------------------------------------------------------
size_t aiw::BinaryHead::get_size() const {
	size_t size = 0; 
	if(type==mesh){ size = szT; for(int i=0; i<dim; i++)  size *= bbox[i]; }
	if(type==sphere) size = (sph_mode? (sph_mode==1? (30l<<2*rank)+2 : 90l<<2*rank) : 60l<<2*rank)*szT; 
	if(type==zcube) size = size_t(szT)<<(dim*rank); 
	return size;
}
//------------------------------------------------------------------------------
void aiw::BinaryHead::dump(aiw::IOstream &S, int align) const {
	if(type==unknown || szT<0) WRAISE("unknown type or szT", szT);
	if(type!=sphere && (dim<0 || dim>=max_dim)) WRAISE("incorrect dim", dim, max_dim);
	
	std::stringstream buf;
	if(!head.empty()) buf.write(head.c_str(), head.size());
	bool faxis = false; for(int i=0; i<dim; i++) if(!axis[i].empty()){ faxis = true; break; }
	if(faxis) for(int i=0; i<dim; i++){
			uint32_t sz = axis[i].size(); buf.write((const char*)&sz, 4);
			if(sz) buf.write(axis[i].c_str(), sz);
		}
	uint32_t tail = type==sphere? 0: logscale;	if(faxis) tail |= uint32_t(1)<<31;
	
	//    head_sz  head, axis       bmin, bmax  tail dim szT             rank                   out_value, bbox
	int tot_sz = 4 + buf.str().size() + dim*16 + 4 + 4 + 4 + 4*(type==sphere || type==zcube) + (szT+4*dim)*(type==mesh);

	if(align){ int dalign = tot_sz%align ? align-tot_sz%align : 0; tot_sz += dalign; for(int i=0; i<dalign; i++) buf.put(0); }
	if(type==mesh) for(int i=0; i<szT; i++) buf.put(0);
	buf.write((const char*)bmin, 8*dim);  buf.write((const char*)bmax, 8*dim); buf.write((const char*)&tail, 4); int h_sz = buf.str().size(); 

	uint32_t D = dim;
	if(type==sphere) D = 0;
	if(type==zcube) D |= 1<<30;

	buf.write((const char*)&D, 4); buf.write((const char*)&szT, 4);

	if(type==mesh) buf.write((const char*)bbox, 4*dim); 
	if(type==sphere){ uint32_t rank_ = rank|(sph_mode<<29); buf.write((const char*)&rank_, 4); }
	if(type==zcube) buf.write((const char*)&rank, 4); 

	// start_offset = S.tell();
	S.write((const char*)&h_sz, 4); S.write(buf.str().c_str(), buf.str().size());
	// data_offset = S.tell();
}
//------------------------------------------------------------------------------
bool aiw::BinaryHead::load(aiw::IOstream &S){  // корректность прочитанного заголовка (тип, размерность и пр.) должна проверяться снаружи
	start_offset = S.tell();
	std::string h; int32_t dim_, szT_;  if(!S.load(h, dim_, szT_)){ S.seek(start_offset); return false; }

	szT = szT_;
	if(dim_==0){ type = sphere; dim = 2; }
	else if(dim_&(1<<30)){ dim = dim_&0xF; type = zcube; }
	else{ dim = dim_; type = mesh; }

	if(szT<=0 || dim<0 || dim>=max_dim){ S.seek(start_offset); return false; }

	if(type==sphere || type==zcube){
		if(!S.load(rank) || rank<0){ S.seek(start_offset); return false; }
		if(type==sphere){ sph_mode = rank>>29; rank &= 0xFF; } // ???
	}
	if(type==mesh && int(S.read(bbox, dim*4))!=dim*4){ S.seek(start_offset); return false; }
	head = h.c_str(); 	data_offset = S.tell();

	if(/*(data_offset-start_offset)%64==0 &&*/ int(h.size()-head.size())>=4+dim*16*(type!=sphere)+szT*(type==mesh)){	
		uint32_t tail; int h_sz = h.size(), off = head.size(); 
		const char* buf = h.c_str(); memcpy(&tail, buf+h_sz-4, 4);
		if(tail&(1<<31)){  // читаем имена осей
			bool faxis = true;
			for(int i=0; i<dim; i++){
				int32_t sz; memcpy(&sz, buf+off, 4); off += 4;
				if(sz>=0 && off+sz<h_sz-4){ axis[i].resize(sz+1, 0); off += sz; if(sz) memcpy(&(axis[i][0]), buf+off, sz); }
				else{ faxis = false;  break; }
			}
			if(!faxis) for(int i=0; i<dim; i++) axis[i].clear(); 
		}
		if(h_sz-4-off>=dim*16){ memcpy(bmin, buf+h_sz-4-dim*16, dim*8); memcpy(bmax, buf+h_sz-4-dim*8, dim*8); }
		logscale = tail &0xFF;
	}
	return true;
}
//------------------------------------------------------------------------------
