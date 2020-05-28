/**
 * Copyright (C) 2020 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <sstream>
#include "../include/aiwlib/typeinfo"
using namespace aiw;
//------------------------------------------------------------------------------
std::vector<aiw::CellFieldAccess> aiw::TypeInfo::get_access() const {
	std::vector<aiw::CellFieldAccess> res;
	if(root.Tname.size()) add_array(res, root, CellFieldAccess(), 0); 
	return res;
}
//------------------------------------------------------------------------------
std::vector<aiw::CellFieldAccess> aiw::TypeInfo::get_xfem_access() const {
	std::vector<aiw::CellFieldAccess> res;
	for(int i=0; i<int(xfem_root.size()); i++){
		CellFieldAccess cfa; cfa.xfem_field = i; cfa.label = xfem_root[i].fname; // cfa.label += "[][]";
		cfa.xfem_szT = xfem_szT[i]; cfa.xfem_dim = xfem_root.size();
		add_array(res, xfem_root[i], cfa, 0);
	}
	return res;
}
//------------------------------------------------------------------------------
// эта функция разворачивает массив
void aiw::TypeInfo::add_array(std::vector<aiw::CellFieldAccess>& res, const field_t &f, CellFieldAccess cfa, int idim) const {
	/*
	if(idim<int(f.dim.size()-1) && f.dim[idim]==-1){ cfa.label += "[]"; cfa.dyn_off = cfa.offset; cfa.dyn_szT = f.szT; cfa.dyn_dim = 1; idim++; }
	if(idim<int(f.dim.size()-1) && f.dim[idim]==-1){ cfa.label += "[]"; idim++; }
	if(int(f.dim.size())==idim){ add_field(res, f, cfa); return; }

	int szA = f.szT; for(int i=idim+1; i<int(f.sim.size()); i++) szA *= f.dim[i];
	for(int i=0; i<f.dim[idim]; i++){
		CellFieldAccess cfa1 = cfa; cfa1.offset += i*szA;
		char buf[256]; buf[snprintf(255, buf, "[%i]", i)] = 0; cfa1.label += buf; add_array(res, f, cfa1, idim+1);
	}
	*/
	if(f.dim.empty()){ add_field(res, f, cfa); return; }
	int D = f.dim.size(), pos[D], mul[D]; for(int i=0; i<D; i++){ pos[i] = 0; mul[D-i-1] = i? mul[D-i]*f.dim[D-i]: f.szT; }
	while(pos[0]<f.dim[0]){
		CellFieldAccess cfa1 = cfa;  std::stringstream buf; buf<<cfa1.label;  // здесь нам нужно поправить offset и label		
		for(int i=0; i<D; i++){ cfa1.offset += mul[i]*pos[i]; buf<<'['<<pos[i]<<']'; }
		cfa1.label = buf.str();		
		add_field(res, f, cfa1);  pos[D-1]++;
		for(int i=D-1; i>0; i--)
			if(pos[i]==f.dim[i]){ pos[i] = 0; pos[i-1]++; }
			else break;
	}
	// if(cfa.dyn_szT){ cfa.dyn_arrs.push_back(ind(cfa.dyn_szT, cfa.dyn_dim, cfa.dyn_off)); cfa.dyn_szT = cfa.dyn_dim = cfa.dyn_off = 0; };  
}
//------------------------------------------------------------------------------
// эта функция добавляет поле с уже развернутым массивом (для каждой ячейки массива)
void aiw::TypeInfo::add_field(std::vector<aiw::CellFieldAccess>& res, const field_t &f, CellFieldAccess cfa) const {
	auto I0 = table.find(f.Tname);
	if(I0==table.end()){ cfa.typeID = f.typeID; res.push_back(cfa); } // это базовый тип
	else { // нужно нырять дальше
		const struct_info_t &si = I0->second;
		for(auto I: si.fields){
			CellFieldAccess cfa1 = cfa;
			cfa1.offset += I.first; if(cfa1.label.size()){ cfa1.label += '.'; } cfa1.label += I.second.fname;
			add_array(res, I.second, cfa1, 0);
		}
	}
}
//------------------------------------------------------------------------------
