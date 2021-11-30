/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

// #include "../../include/aiwlib/binary_format"
#include "../../include/aiwlib/qplt/accessor"
using namespace aiw;

//------------------------------------------------------------------------------
CU_HD bool aiw::QpltAccessor::check() const {
	if(ctype<0 || 11<ctype) return false;
	if(Din<1 || 3<Din) return false;
	for(int i=0; i<Din; i++) if(offsets[i]<0) return false;
	if(Din>1 && (diff==2 || diff==3)) return false;
	return true;
}
//------------------------------------------------------------------------------
CU_HD int aiw::QpltAccessor::Dout() const {  // размерность выходных данных
	if(diff==0 || diff>=5) return vconv<5? 1: Din; // оператор лапласа не меняет размерность данных
	if(diff==1 || (diff==4 && Din==2) || vconv<5) return 1; // div, rot2d либо включено преобразование
	if(diff==2 || diff==3) return  vconv<5? 1: diff; // градиент
	return -1;  // что то пошло не так?		
}
//------------------------------------------------------------------------------
CU_HD int aiw::QpltAccessor::Ddiff() const { // размерность пространства в котором идет дифференцирование
	if(diff==0) return 0;
	if(diff==2 || diff==5) return 2;
	if(diff==1 || diff==4) return Din;
	return 3;
}
//------------------------------------------------------------------------------
