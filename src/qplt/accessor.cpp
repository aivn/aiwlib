/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

// #include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/qplt/accessor"
using namespace aiw;

//------------------------------------------------------------------------------
bool aiw::QpltAccessor::check() const {
	if(ctype<0 || 11<ctype) return false;
	if(Din<1 || 3<Din) return false;
	for(int i=0; i<Din; i++) if(offsets[i]<0) return false;
	if(Din>1 && (diff==2 || diff==3)) return false;
	return true;
}
//------------------------------------------------------------------------------
int aiw::QpltAccessor::Dout() const {  // размерность выходных данных
	if(diff==0 || diff>=5) return vconv<5? 1: Din; // оператор лапласа не меняет размерность данных
	if(diff==1 || (diff==4 && Din==2) || vconv<5) return 1; // div, rot2d либо включено преобразование
	if(diff==2 || diff==3) return  vconv<5? 1: diff; // градиент
	return -1;  // что то пошло не так?		
}
//------------------------------------------------------------------------------
int aiw::QpltAccessor::Ddiff() const { // размерность пространства в котором идет дифференцирование
	if(diff==0) return 0;
	if(diff==2 || diff==5) return 2;
	if(diff==1 || diff==4) return Din;
	return 3;
}
//------------------------------------------------------------------------------
template <> float aiw::QpltAccessor::conv<0>(const char *ptr) const { return *(float*)ptr; }
template <> float aiw::QpltAccessor::conv<1>(const char *ptr) const {
	if(ctype==1) return *(double*)ptr; 
	if(ctype==2) return *(bool*)ptr;
	if(mask && ctype%2){
		typedef int mask_t;
		mask_t x = 0, r = 0; int j = 0, sz = 0;
		switch(ctype){
		case 3:  x =  *(uint8_t*)ptr; sz =  8; break;
		case 5:  x = *(uint16_t*)ptr; sz = 16; break; 
		case 7:  x = *(uint32_t*)ptr; sz = 32; break;
	    default: x = *(uint64_t*)ptr; sz = 64; 
		}
		for(int i=0; i<sz; i++){
			mask_t m = mask_t(1)<<i;
			if(m&mask) r |= mask_t(bool(m&x))<<(j++);
		}
		return r;
	}
	switch(ctype){
	case 3:  return   *(uint8_t*)ptr;
	case 4:  return    *(int8_t*)ptr;
	case 5:  return  *(uint16_t*)ptr;
	case 6:  return   *(int16_t*)ptr;
	case 7:  return  *(uint32_t*)ptr;
	case 8:  return   *(int32_t*)ptr;
	case 9:  return  *(uint64_t*)ptr;
	default:  return  *(int64_t*)ptr;
	}
	return 0.f;
}
//------------------------------------------------------------------------------
