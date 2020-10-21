/**
 * Copyright (C) 2020 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <omp.h>
#include "../include/aiwlib/binary_format"
using namespace aiw;
//------------------------------------------------------------------------------
// Mesh:   head [[axes] [typeinfo] align out_value bmin bmax axesbit<<31|typeinfobit<<30|logscale]    D        szT     box
// Sphere: head [[axes] [typeinfo] align                     axesbit<<31|typeinfobit<<30|int(0)]      int(0)   szT  R
// Z-cube: head [[axes] [typeinfo] align           bmin bmax axesbit<<31|typeinfobit<<30|logscale]    1<<30|D  szT  R  
// AMR:    head [[axes] [typeinfo] align           bmin bmax axesbit<<31|typeinfobit<<30|logscale]    1<<31|D  szT  R  box
//------------------------------------------------------------------------------
bool aiw::BinaryFormat::dump(IOstream& S) const {
	int DD = D&15;  // реальная размерность
	if(DD<0 || szT<=0 || (bool(bmin)^bool(bmax))) WRAISE("incorrect dump parametrs ", D, DD, szT, bmin, bmax);
	Packer buf; buf.push_str(head); buf.dump(uint8_t(0)); 
	if(axes) for(int i=0; i<DD; i++) buf.dump(axes[i]);
#ifdef AIW_TYPEINFO
	if(tinfo) buf.dump(tinfo);
#else
	bool tinfo = false;
	//	printf("******** TYPEINFO IGNORED ********\n");
#endif //AIW_TYPEINFO
	int align_pos = buf.size();
			
	if(out_value) buf.write(out_value, szT);
	if(bmin && bmax){ buf.write(bmin, DD*8); buf.write(bmax, DD*8); }
	if(logscale!=unused) buf.dump(logscale|(uint32_t(bool(axes))<<31)|(uint32_t(bool(tinfo))<<30));
	else buf.dump((uint32_t(bool(axes))<<31)|(uint32_t(bool(tinfo))<<30));

	//	std::cout<<"*********** "<<((uint32_t(bool(axes))<<31)|(uint32_t(bool(tinfo))<<30))<<" ********************\n";
	
	buf.set_align(align_pos, (3+(R!=unused)+bool(box)*DD)*4, 64); // head_sz, D, szT, [R], [DD]
	S.dump(buf, D, szT); if(R!=unused){ S.dump(R); }  if(box) S.write(box, DD*4);
	// std::cout<<buf.size()<<' '<<S.tell()<<'\n';
	return true; // ???
}
//------------------------------------------------------------------------------
bool aiw::BinaryFormat::load(IOstream& S){
	size_t s = S.tell(); Packer buf; if(!S.load(buf)) return false;  
	if(buf.size()) head = (const char*)buf.data();  else head = "";
	
	int rD, rszT, rR; bool rOK = S.load(rD, rszT); if(rOK && R!=unused) rOK = S.load(rR);
	if(!rOK || (D!=-1 && D!=rD) || (szT!=-1 && rszT!=szT) || (rszT<=0) || (R!=unused && R!=-1 && rR!=R)){ S.seek(s); return false; }
	D = rD; szT = rszT; int DD = D&15; if(R!=unused) R = rR;	
	if(box && int(S.read(box, DD*4))!=DD*4){  S.seek(s); return false;  }
	
	size_t h_sz = head.size(); buf.seek(h_sz+1); bool set_bounds = false;
	if(h_sz+4<=buf.size()){
		uint32_t tail = 0; buf.get(buf.size()-4, tail);
		if(tail&(1<<31) && axes) for(int i=0; i<DD; i++) if(!buf.load(axes[i])) break; 

		// WOUT(tail&(1<<30));
#ifdef AIW_TYPEINFO
		if(tail&(1<<30)) rOK = buf.load(tinfo); 
#endif //AIW_TYPEINFO
		if(logscale!=unused) logscale = tail&~(3<<30); 
		if(out_value && bmin && bmax && buf.tail_sz()>=size_t(DD*16+4+szT)){  buf.rseek(4+DD*16+szT); buf.read(out_value, szT); }
		if(bmin && bmax && buf.tail_sz()>=size_t(DD*16+4)){  buf.rseek(4+DD*16); buf.read(bmin, DD*8); buf.read(bmax, DD*8); set_bounds = true; }
	}
	if(!set_bounds && bmin && bmax && box) for(int i=0; i<DD; i++){  ((double*)bmin)[i] = 0; ((double*)bmax)[i] = ((int*)box)[i];  }
	return true;
}
//------------------------------------------------------------------------------
