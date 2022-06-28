//------------------------------------------------------------------------------
//  this code of Larry Jones from https://www.thecodingforums.com/threads/c-code-for-converting-ibm-370-floating-point-to-ieee-754.438469/
//------------------------------------------------------------------------------
/* ibm2ieee - Converts a number from IBM 370 single precision floating
point format to IEEE 754 single precision format. For normalized
numbers, the IBM format has greater range but less precision than the
IEEE format. Numbers within the overlapping range are converted
exactly. Numbers which are too large are converted to IEEE Infinity
with the correct sign. Numbers which are too small are converted to
IEEE denormalized numbers with a potential loss of precision (including
complete loss of precision which results in zero with the correct
sign). When precision is lost, rounding is toward zero (because it's
fast and easy -- if someone really wants round to nearest it shouldn't
be TOO difficult). */

#include <sys/types.h>
#ifndef AIW_WIN32
#include <netinet/in.h>

void ibm2ieee(void *to, const void *from, int len)
{
	register unsigned fr; /* fraction */
	register int exp; /* exponent */
	register int sgn; /* sign */

	for (; len-- > 0; to = (char *)to + 4, from = (char *)from + 4) {
		/* split into sign, exponent, and fraction */
		fr = ntohl(*(long *)from); /* pick up value */
		sgn = fr >> 31; /* save sign */
		fr <<= 1; /* shift sign out */
		exp = fr >> 25; /* save exponent */
		fr <<= 7; /* shift exponent out */

		if (fr == 0) { /* short-circuit for zero */
			exp = 0;
			goto done;
		}

		/* adjust exponent from base 16 offset 64 radix point before first digit
		   to base 2 offset 127 radix point after first digit */
		/* (exp - 64) * 4 + 127 - 1 == exp * 4 - 256 + 126 == (exp << 2) - 130 */
		exp = (exp << 2) - 130;
		
		/* (re)normalize */
		while (fr < 0x80000000) { /* 3 times max for normalized input */
			--exp;
			fr <<= 1;
		}

		if (exp <= 0) { /* underflow */
			if (exp < -24) /* complete underflow - return properly signed zero */
				fr = 0;
			else /* partial underflow - return denormalized number */
				fr >>= -exp;
			exp = 0;
		} else if (exp >= 255) { /* overflow - return infinity */
			fr = 0;
			exp = 255;
		} else { /* just a plain old number - remove the assumed high bit */
			fr <<= 1;
		}

	done:
		/* put the pieces back together and return it */
		*(unsigned *)to = (fr >> 9) | (exp << 23) | (sgn << 31);
	}
}

/* ieee2ibm - Converts a number from IEEE 754 single precision floating
point format to IBM 370 single precision format. For normalized
numbers, the IBM format has greater range but less precision than the
IEEE format. IEEE Infinity is mapped to the largest representable
IBM 370 number. When precision is lost, rounding is toward zero
(because it's fast and easy -- if someone really wants round to nearest
it shouldn't be TOO difficult). */

void ieee2ibm(void *to, const void *from, int len)
{
	register unsigned fr; /* fraction */
	register int exp; /* exponent */
	register int sgn; /* sign */

	for (; len-- > 0; to = (char *)to + 4, from = (char *)from + 4) {
		/* split into sign, exponent, and fraction */
		fr = *(unsigned *)from; /* pick up value */
		sgn = fr >> 31; /* save sign */
		fr <<= 1; /* shift sign out */
		exp = fr >> 24; /* save exponent */
		fr <<= 8; /* shift exponent out */
		
		if (exp == 255) { /* infinity (or NAN) - map to largest */
			fr = 0xffffff00;
			exp = 0x7f;
			goto done;
		}
		else if (exp > 0) /* add assumed digit */
			fr = (fr >> 1) | 0x80000000;
		else if (fr == 0) /* short-circuit for zero */
			goto done;
		
		/* adjust exponent from base 2 offset 127 radix point after first digit
		   to base 16 offset 64 radix point before first digit */
		exp += 130;
		fr >>= -exp & 3;
		exp = (exp + 3) >> 2;

		/* (re)normalize */
		while (fr < 0x10000000) { /* never executed for normalized input */
			--exp;
			fr <<= 4;
		}

	done:
		/* put the pieces back together and return it */
		fr = (fr >> 8) | (exp << 24) | (sgn << 31);
		*(unsigned *)to = htonl(fr);
	}
}
//------------------------------------------------------------------------------
//  end of Larry Jones code fragment
//------------------------------------------------------------------------------
#endif // AIW_WIN32


#include "../include/aiwlib/segy"

/**
 * Copyright (C) 2010-2016 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

using namespace aiw;

bool aiw::segy_ibm_format = false;
//------------------------------------------------------------------------------
//   read operations
//------------------------------------------------------------------------------
int aiw::segy_raw_read(IOstream &&S, std::list<std::vector<float> > &data, std::vector<Vecf<8> > &heads, 
					   size_t count, bool read_data){
	int max_sz = 0; // WMSG(count, read_data); S.seek(3600);
	while(count--){
		// WMSG(count, S.tell());
		SegyTraceHead tr; if(!tr.load(S)) break; 
		heads.push_back(tr.PV|tr.PP|tr.dt|tr.trace_sz);
		if(read_data){
			data.push_back(std::vector<float>());
			data.back().resize(tr.trace_sz);
#ifndef AIW_WIN32
			if(segy_ibm_format){ float buf[tr.trace_sz]; S.read(buf, tr.trace_sz*4); ibm2ieee(&(data.back()[0]), buf, tr.trace_sz); }
			else
#endif
				S.read(&(data.back()[0]), tr.trace_sz*4);
			if(max_sz<tr.trace_sz) max_sz = tr.trace_sz;
		} else S.seek(tr.trace_sz*4, SEEK_CUR);
		// WMSG(tr.trace_sz, S.tell());
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
	for(auto V=rdata.begin(); V!=rdata.end() and (pos^=data.bbox()(1,2));){
		WASSERT(ind(0)<=pos && pos<data.bbox()(1,2), " ", pos, data.bbox());
		for(int i=0; i<int(V->size()); i++) data[i|pos] = (*V)[i];
		++pos; ++V;
	}
	// Vecf<8> front = geometry[Ind<3>(0)], back = geometry[geometry.bbox()-ind(1)];
	// data.set_axis(front(5, 3, 4), (front(5)+H.dt*data.bbox()[0])|back(3, 4));
	return geometry;
}
//------------------------------------------------------------------------------
//   write operations
//------------------------------------------------------------------------------
void segy_write_trace_data(IOstream &S, const Mesh<float, 1> &data, float z_pow, const std::vector<float> prefix){
	if(prefix.size()) S.write(prefix.data(), 4*prefix.size()); 
	int sz = data.bbox()[0]; float buf[sz]; for(int i=0; i<sz; i++){ buf[i] = z_pow? data[ind(i)]*pow(i+1, z_pow): data[ind(i)]; }
#ifndef AIW_WIN32
	if(segy_ibm_format){ float buf2[sz]; ieee2ibm(buf2, buf, sz); S.write(buf2, 4*sz); }
    else
#endif
		S.write(buf, 4*sz); 
}
//------------------------------------------------------------------------------
template <int D> void mk_zero_prefix(const Mesh<float, D> &data, std::vector<float> &prefix){
	int iz0 = data.bmin[0]/data.step[0]+.5;
	if(iz0>0) prefix.resize(iz0, 0.f);
	WOUT(prefix.size());
}
//------------------------------------------------------------------------------
void aiw::segy_write(IOstream &&S, const Mesh<float, 1> &data, float z_pow, Vecf<2> PV, Vecf<3> PP){
	std::vector<float> prefix; mk_zero_prefix(data, prefix);
	SegyTraceHead tr; tr.PV = PV|0.; tr.PP = PP; tr.trace_sz = data.bbox()[0]+prefix.size(); tr.dt = data.step[0]; tr.dump(S);
	segy_write_trace_data(S, data, z_pow, prefix);
}
//------------------------------------------------------------------------------
void aiw::segy_write(IOstream &&S, const Mesh<float, 2> &data, float z_pow, Vecf<2> PV, Vecf<3> PP0, float rotate, bool write_file_head){
	std::vector<float> prefix; mk_zero_prefix(data, prefix); 
	if(write_file_head){
		SegyFileHead fh; fh.dt = data.step[0]; fh.trace_sz = data.bbox()[0]+prefix.size(); fh.profile_sz = data.bbox()[1]; fh.dump(S);
	}
	Vecf<3> delta = vec(cos(rotate), sin(rotate), 0.)*data.step[1];
	for(int ix=0; ix<data.bbox()[1]; ix++){ 
		SegyTraceHead tr; tr.PV = PV|0.; tr.trace_sz = data.bbox()[0]+prefix.size(); tr.dt = data.step[0]; tr.PP = PP0 + delta*ix; tr.dump(S);
		segy_write_trace_data(S, data.slice<1>(ind(-1, ix)), z_pow, prefix);
	}
}
//------------------------------------------------------------------------------
void aiw::segy_write(IOstream &&S, const Mesh<float, 3> &data, float z_pow, Vecf<2> PV, Vecf<3> PP0, float rotate, bool write_file_head){
	std::vector<float> prefix; mk_zero_prefix(data, prefix);
	if(write_file_head){
		SegyFileHead fh; fh.dt = data.step[0]; fh.trace_sz = data.bbox()[0]+prefix.size(); fh.profile_sz = data.bbox()[1]; fh.dump(S);
	}
	Vecf<3> delta_x = vec(cos(rotate), sin(rotate), 0.)*data.step[1], delta_y = vec(-sin(rotate), cos(rotate), 0.)*data.step[2];
	for(int iy=0; iy<data.bbox()[2]; iy++) 
		for(int ix=0; ix<data.bbox()[1]; ix++){ 
			SegyTraceHead tr; tr.PV = PV|0.; tr.trace_sz = data.bbox()[0]+prefix.size(); tr.dt = data.step[0]; tr.PP = PP0 + delta_x*ix + delta_y*iy; tr.dump(S);
			segy_write_trace_data(S, data.slice<1>(ind(-1, ix, iy)), z_pow, prefix);
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
	set_int16(16, (dt<1? dt*1e6: dt*1e3)+.5);  // shag diskretizacii, mks 16-17 18-19 ???
	set_int16(20, trace_sz);   // chislo otschetov v trasse 21-22
	set_int16(22, trace_sz);   // chislo otschetov v trasse 23-24
	set_int16(24, segy_ibm_format?1:5);	       // FORMAT
	S.write(head, 3600);
}
//------------------------------------------------------------------------------
void aiw::SegyFileHead::load(aiw::IOstream &S){
	S.read(head, 3600);
	profile_sz = get_int16(12); // chislo trass v seysmogramme (ne v faile!) 12-13
	dt = get_int16(16)*1e-6;    // shag diskretizacii, mks 16-17
	trace_sz = get_int16(20);   // chislo otschetov v trasse 21-22
	segy_ibm_format = get_int16(24)!=5; // попытка задать формат автоматически
}
//------------------------------------------------------------------------------
//   trace head
//------------------------------------------------------------------------------
int trID = 1;
aiw::SegyTraceHead::SegyTraceHead(){
	set_int32(0, trID); set_int32(4, trID); trID++;
	for(int i=0; i<240; i++) head[i] = 0;
	for(int i=0; i<4; i++) set_int16(28+2*i, 1);  // buf_one1
	for(int i=0; i<2; i++) set_int16(68+2*i, 1);  // buf_one2
	for(int i=0; i<10; i++) set_int16(88+2*i, 1); // buf_one3
	set_int16(118, 1); // FORMAT
	set_int16(218, 1); // actual
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
	set_int16(116, (dt<1? dt*1e6: dt*1e3)+.5); //116 shag diskretizacii, mks
	S.write(head, 240);
}
//------------------------------------------------------------------------------
bool aiw::SegyTraceHead::load(aiw::IOstream &S){
	if(S.read(head, 240)!=240) return false;
	PP[2] = get_int32(40); //40 relief PP
	PV[0] = get_int32(72); 
	PV[1] = get_int32(76); 
	PP[0] = get_int32(80); 
	PP[1] = get_int32(84); 
	trace_sz = get_int16(114); //114 chislo otschetov v trasse
	dt = get_int16(116)*1e-6;  //116 shag diskretizacii, mks
	return trace_sz>0;
}
//------------------------------------------------------------------------------
void aiw::SegyTraceHead::write(aiw::IOstream &S, const float *data, float z_pow){
	dump(S);
	float buf[trace_sz]; if(z_pow) for(int i=0; i<trace_sz; i++){ buf[i] = data[i]*pow(i+1, z_pow); }
#ifndef AIW_WIN32
	if(segy_ibm_format){ float buf2[trace_sz]; ieee2ibm(buf2, z_pow?buf:data, trace_sz); S.write(buf2, 4*trace_sz); }
    else
#endif
		S.write(z_pow?buf:data, 4*trace_sz); 	
}
//------------------------------------------------------------------------------
aiw::Mesh<float, 1> aiw::SegyTraceHead::read(aiw::IOstream &S){
	//if(!load(S)) return ;
	Mesh<float, 1> res; res.init(ind(trace_sz));
#ifndef AIW_WIN32
	if(segy_ibm_format){ float buf[trace_sz]; S.read(buf, trace_sz*4); ibm2ieee(&(res[ind(0)]), buf, trace_sz); }
	else
#endif
		S.read(&(res[ind(0)]), 4*trace_sz);
	return res;
}
//------------------------------------------------------------------------------
