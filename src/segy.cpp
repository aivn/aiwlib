#include "../include/aiwlib/segy"

/**
 * Copyright (C) 2010-2016 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

using namespace aiw;
//------------------------------------------------------------------------------
//   read operations
//------------------------------------------------------------------------------
int aiw::segy_raw_read(IOstream &S, std::list<std::vector<float> > &data, std::vector<Vecf<8> > &heads, 
					   size_t count, bool read_data){
	int max_sz = 0;
	while(count--){
		SegyTraceHead tr; if(!tr.load(S)) break; 
		heads.push_back(tr.PV|tr.PP|tr.dt|tr.trace_sz);
		if(read_data){
			data.push_back(std::vector<float>());
			data.back().resize(tr.trace_sz);
			S.read(&(data.back()[0]), tr.trace_sz*4);
			if(max_sz<tr.trace_sz) max_sz = tr.trace_sz;
		} else S.seek(tr.trace_sz*4, SEEK_CUR);
	}
	return max_sz;
}
//------------------------------------------------------------------------------
Mesh<float, 2> aiw::segy_read_geometry(IOstream &&S, bool read_file_head, size_t count){
	if(read_file_head) S.seek(3600);
	std::list<std::vector<float> > data; std::vector<Vecf<8> > heads;
	segy_raw_read(S, data, heads, count, false);
	Mesh<float, 2> geometry; geometry.init(ind(8, heads.size()));
	for(Ind<2> pos; pos^=geometry.bbox(); ++pos) geometry[pos] = heads[pos[1]][pos[0]];
	return geometry;
}
//------------------------------------------------------------------------------
Mesh<float, 2> aiw::segy_read(IOstream &&S, Mesh<float, 2> &data, bool read_file_head, size_t count){
	if(read_file_head) S.seek(3600);
	std::list<std::vector<float> > rdata; std::vector<Vecf<8> > heads;
	int max_sz = segy_raw_read(S, rdata, heads, count, true);
	Mesh<float, 2> geometry; geometry.init(ind(8, heads.size()));
	for(Ind<2> pos; pos^=geometry.bbox(); ++pos) geometry[pos] = heads[pos[1]][pos[0]];
	data.init(ind(max_sz, rdata.size())); data.fill(0.f);
	Ind<2> pos;
	for(auto V=rdata.begin(); V!=rdata.end(); ++V){
		for(pos[0]=0; pos[0]<int(V->size()); pos[0]++) data[pos] = (*V)[pos[0]];
		pos[1]++;
	}
	// data.swap(0, 1); //???
	return geometry;
}
//------------------------------------------------------------------------------
Mesh<float, 3> aiw::segy_read(IOstream &&S, Mesh<float, 3> &data){
	SegyFileHead H; H.load(S);
	std::list<std::vector<float> > rdata; std::vector<Vecf<8> > heads;
	int max_sz = segy_raw_read(S, rdata, heads, -1, true);
	Mesh<float, 3> geometry; geometry.init(ind(8, H.profile_sz, heads.size()/H.profile_sz));
	for(Ind<3> pos; pos^=geometry.bbox(); ++pos) geometry[pos] = heads[pos[1]+pos[2]*H.profile_sz][pos[0]];
	data.init(ind(max_sz, H.profile_sz, rdata.size()/H.profile_sz)); data.fill(0.f);
	Ind<2> pos;
	for(auto V=rdata.begin(); V!=rdata.end() and (pos^=data.bbox()(0,1));){
		for(int i=0; i<int(V->size()); i++) data[i|pos] = (*V)[i];
		++pos; ++V;
	}
	return geometry;
}
//------------------------------------------------------------------------------
//   write operations
//------------------------------------------------------------------------------
void aiw::segy_write(IOstream &&S, const Mesh<float, 1> &data, Vec<2> PV, Vec<3> PP){
	SegyTraceHead tr; tr.PV = PV|0.; tr.PP = PP; tr.trace_sz = data.bbox()[0]; tr.dt = data.step[0]; tr.dump(S);
	for(int i=0; i<data.bbox()[0]; i++) S.write(&(data[ind(i)]), 4);
}
//------------------------------------------------------------------------------
void aiw::segy_write(IOstream &&S, Mesh<float, 2> data, Vec<2> PV, Vec<3> PP0, double rotate, bool write_file_head){
	if(write_file_head){
		SegyFileHead fh; fh.dt = data.step[0]; fh.trace_sz = data.bbox()[0]; fh.profile_sz = data.bbox()[1]; fh.dump(S);
	}
	Vec<3> delta = vec(cos(rotate), sin(rotate), 0.)*data.step[1];
	for(int ix=0; ix<data.bbox()[1]; ix++){ 
		SegyTraceHead tr; tr.PV = PV|0.; tr.trace_sz = data.bbox()[0]; tr.dt = data.step[0]; tr.PP = PP0 + delta*ix; tr.dump(S);
		for(int iz=0; iz<data.bbox()[0]; iz++) S.write(&(data[ind(iz, ix)]), 4);
	}
}
//------------------------------------------------------------------------------
void aiw::segy_write(IOstream &&S, Mesh<float, 3> data, Vec<2> PV, Vec<3> PP0, double rotate, bool write_file_head){
	if(write_file_head){
		SegyFileHead fh; fh.dt = data.step[0]; fh.trace_sz = data.bbox()[0]; fh.profile_sz = data.bbox()[1]; fh.dump(S);
	}
	Vec<3> delta_x = vec(cos(rotate), sin(rotate), 0.)*data.step[1], delta_y = vec(-sin(rotate), cos(rotate), 0.)*data.step[2];
	for(int iy=0; iy<data.bbox()[2]; iy++) 
		for(int ix=0; ix<data.bbox()[1]; ix++){ 
			SegyTraceHead tr; tr.PV = PV|0.; tr.trace_sz = data.bbox()[0]; tr.dt = data.step[0]; tr.PP = PP0 + delta_x*ix + delta_y*iy; tr.dump(S);
			for(int iz=0; iz<data.bbox()[0]; iz++) S.write(&(data[ind(iz, ix, iy)]), 4);
		}
}
//------------------------------------------------------------------------------
//   file head
//------------------------------------------------------------------------------
aiw::SegyFileHead::SegyFileHead(){
	for(int i=0; i<3600; i++) head[i] = 0;
	dt = 0; trace_sz = 0; profile_sz = 0;
}
//------------------------------------------------------------------------------
void aiw::SegyFileHead::dump(aiw::IOstream &S){
	set_int16(12, profile_sz); // chislo trass v seysmogramme (ne v faile!) 12-13
	set_int16(16, dt*1e6+.5);  // shag diskretizacii, mks 16-17
	set_int16(20, trace_sz);   // chislo otschetov v trasse 21-22
	set_int16(24, 5);	       // FORMAT
	S.write(head, 3600);
}
//------------------------------------------------------------------------------
void aiw::SegyFileHead::load(aiw::IOstream &S){
	S.read(head, 3600);
	profile_sz = get_int16(12); // chislo trass v seysmogramme (ne v faile!) 12-13
	dt = get_int16(16)*1e-6;    // shag diskretizacii, mks 16-17
	trace_sz = get_int16(20);   // chislo otschetov v trasse 21-22
}
//------------------------------------------------------------------------------
//   trace head
//------------------------------------------------------------------------------
aiw::SegyTraceHead::SegyTraceHead(){
	for(int i=0; i<400; i++) head[i] = 0;
	dt = 0; trace_sz = 0;
}
//------------------------------------------------------------------------------
void aiw::SegyTraceHead::dump(aiw::IOstream &S){
	set_int32(40, PP[2]+.5); //40 relief PP
	set_int32(72, PV[0]+.5); 
	set_int32(76, PV[1]+.5); 
	set_int32(80, PP[0]+.5); 
	set_int32(84, PP[1]+.5); 
	set_int16(114, trace_sz);  //114 chislo otschetov v trasse
	set_int16(116, dt*1e6+.5); //116 shag diskretizacii, mks
	set_int16(118, 1);         //118 FORMAT
	S.write(head, 400);
}
//------------------------------------------------------------------------------
bool aiw::SegyTraceHead::load(aiw::IOstream &S){
	if(S.read(head, 400)!=400) return false;
	PP[2] = get_int32(40); //40 relief PP
	PV[0] = get_int32(72); 
	PV[1] = get_int32(76); 
	PP[0] = get_int32(80); 
	PP[1] = get_int32(84); 
	trace_sz = get_int16(114); //114 chislo otschetov v trasse
	dt = get_int16(116)*1e-6;  //116 shag diskretizacii, mks
	return true;
}
//------------------------------------------------------------------------------
void aiw::SegyTraceHead::write(aiw::IOstream &S, float *data){ dump(S); S.write(data, trace_sz*4); }
//------------------------------------------------------------------------------
aiw::Mesh<float, 1> aiw::SegyTraceHead::read(aiw::IOstream &S){
	load(S); Mesh<float, 1> res; res.init(ind(trace_sz));
	S.read(&(res[ind(0)]), trace_sz);
	return res;
}
//------------------------------------------------------------------------------
