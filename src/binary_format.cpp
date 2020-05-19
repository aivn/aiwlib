/**
 * Copyright (C) 2020 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include "../include/aiwlib/binary_format"
using namespace aiw;
//------------------------------------------------------------------------------
// Mesh:   head [[axes] [typeinfo] align out_value bmin bmax axesbit<<31|typeinfobit<<30|logscale]    D        szT     box
// Sphere: head [[axes] [typeinfo] align                     axesbit<<31|typeinfobit<<30|int(0)]      int(0)   szT  R
// Z-cube: head [[axes] [typeinfo] align           bmin bmax axesbit<<31|typeinfobit<<30|logscale]    1<<30|D  szT  R  
// AMR:    head [[axes] [typeinfo] align           bmin bmax axesbit<<31|typeinfobit<<30|logscale]    1<<31|D  szT  R  box
//------------------------------------------------------------------------------
void aiw::BinaryFormat::dump(IOstream& S) const {
	int DD = D&15;  // реальная размерность
	if(DD<=0 || szT<=0 || (bool(bmin)^bool(bmax))) WRAISE("incorrect dump parametrs ", D, DD, szT, bmin, bmax);
	Packer buf; buf.push_str(head); buf<uint8_t(0); 
	if(axes) for(int i=0; i<DD; i++) buf<axes[i];
	if(tinfo) buf<tinfo;
	int align_pos = buf.size();
			
	if(out_value) buf.write(out_value, szT);
	if(bmin && bmax){ buf.write(bmin, DD*8); buf.write(bmax, DD*8); }
	if(logscale!=unused) buf<(logscale|(uint32_t(bool(axes))<<31)|(uint32_t(bool(tinfo))<<30));
	else buf<((uint32_t(bool(axes))<<31)|(uint32_t(bool(tinfo))<<30));

	int tot_sz = buf.size()+(3+(R!=unused)+bool(box)*DD)*4;
	buf.insert_zero(align_pos, tot_sz%64);
	S<buf<D<szT; if(R!=unused){ S<R; }  if(box) S.write(box, DD*4);
}
//------------------------------------------------------------------------------
bool aiw::BinaryFormat::load(IOstream& S){
	size_t s = S.tell(); Packer buf; if(!buf.load(S)) return false;
	if(buf.size()) head = (const char*)buf.data();  else head = "";

	int rD = -2, rszT = -2, rR = -2; S>rD>rszT; if(R!=unused) S>rR;
	if((D!=-1 && D!=rD)||(szT!=-1 && rszT!=szT)||(rszT<=0)||(R!=unused && rR!=R)){ S.seek(s); return false; }
	D = rD; szT = rszT; int DD = D&15; if(R!=unused) R = rR;	
	if(box && int(S.read(box, DD*4))!=DD*4){  S.seek(s);  return false;  }
	
	size_t h_sz = head.size(); buf.seek(h_sz+1); bool set_bounds = false;
	if(h_sz+4<=buf.size()){
		uint32_t tail = 0; buf.get(buf.size()-4, tail);
		if(tail&(1<<31) && axes) for(int i=0; i<DD; i++) buf>axes[i];
		if(tail&(1<<30)) buf>tinfo; 
		if(logscale!=unused) logscale = tail&~(3<<30); 
		if(out_value && bmin && bmax && buf.tail_sz()>=DD*16+4+szT){  buf.rseek(4+DD*16+szT); buf.read(out_value, szT); }
		if(bmin && bmax && buf.tail_sz()>=DD*16+4){  buf.rseek(4+DD*16); buf.read(bmin, DD*8); buf.read(bmax, DD*8); set_bounds = true; }
	}
	if(!set_bounds && bmin && bmax && box) for(int i=0; i<DD; i++){  ((double*)bmin)[i] = 0; ((double*)bmax)[i] = ((int*)box)[i];  }
	return true;
}
//------------------------------------------------------------------------------
