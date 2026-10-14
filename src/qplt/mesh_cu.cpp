/**
 * Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include <omp.h>
#include <sstream>
#include "../../include/aiwlib/interpolations"
#include "../../include/aiwlib/qplt/mesh"
#include "../../include/aiwlib/qplt/mesh_cu"
using namespace aiw;

//------------------------------------------------------------------------------
#ifdef __NVCC__
// Frames live in host memory (as in the CPU build); only the frame being rendered in XD mode is copied to the device.
// cudaMallocManaged for every frame caused random illegal-address faults on Windows (WDDM) and limited the total
// size of loaded frames to the GPU memory.
namespace { char *dev_data = nullptr; size_t dev_sz = 0; const char *dev_src = nullptr; }  // device copy of host frame dev_src
static void dev_data_reset(){ cudaFree(dev_data); dev_data = nullptr; dev_sz = 0; dev_src = nullptr; }
#endif //__NVCC__
void aiw::QpltMesh::data_free_impl(){  // выгружает данные из памяти
#ifdef __NVCC__
	if(mem_ptr && mem_ptr==dev_src) dev_src = nullptr;  // the device copy is stale now
#endif //__NVCC__
	WERR(this); mem.reset(); mem_ptr = nullptr;
}
void aiw::QpltMesh::data_load_impl(){  // загружает данные в память
#ifndef __NVCC__
	fin->seek(mem_offset);
	mem = fin->mmap(data_sz, 0);
	mem_ptr = (char*)(mem->get_addr());
#else //__NVCC__
	data_load_cuda();  // host memory, see mesh.cpp
#endif //__NVCC__
	// WERR(this, mem.get(), mem_limit);
	// WOUT(mem_offset, mem->get_addr());
}
//------------------------------------------------------------------------------
#ifdef __NVCC__
__constant__ char plt_cu[sizeof(aiw::QpltMeshPlotter3D<7>)];
#define plt_cu_ (*((const aiw::QpltMeshPlotter3D<AID>*)plt_cu)) 

template <int AID> __global__ void plotXD(int* image){
	int x = threadIdx.x+blockIdx.x*blockDim.x, y = threadIdx.y+blockIdx.y*blockDim.y;
	if(plt_cu_.Nx<=x || plt_cu_.Ny<=y) return;
	
	const char *nb[6] = {nullptr};  // ???

	int x0 = plt_cu_.ibmin[0]+x, y0 = plt_cu_.ibmin[1]+y, cID;  Vecf<2> r;
	for(cID=0; cID<3; cID++) if(plt_cu_.flats[cID].image2flat(x0, y0, r)) break;
	if(cID==3){ image[x+y*plt_cu_.Nx] = 0xFFFFFFFF; return; }

	auto& flat = plt_cu_.flats[cID];
	Ind<2> pos;  Vecf<2> X; flat.mod_coord(r, pos, X);  const char* ptr = flat.get_ptr(pos);
	Ind<3> pos3d; flat.pos2to3(pos, pos3d); 
	Vecf<4> C;  auto ray = plt_cu_.vtx.trace(cID, X);
	constexpr int DIFF = (AID>>3)&7; // какой то static method в accessor?
	if(DIFF) for(int k=0; k<6; k++){ int a = k/2, d = (k%2*2-1)*plt_cu_.dsgn[a], p = pos3d[a]+d; nb[k] = ptr + (0<=p && p<plt_cu_.bbox[a] ? d*plt_cu_.deltas[a]: 0); }
#ifdef QPLT_CU_CHECK  // debug: verify every voxel access lies inside the frame data
#define QPLT_CU_CHECK_PTR(P, WHAT) if((P)<plt_cu_.data_lo || (P)>=plt_cu_.data_hi){ \
		printf("QPLT_CU_CHECK %s out of frame data: px=(%d,%d) cID=%d pos=(%d,%d) pos3d=(%d,%d,%d) bbox=(%d,%d,%d) gID=%d off=%lld size=%lld\n", WHAT, x, y, cID, pos[0], pos[1], \
			   pos3d[0], pos3d[1], pos3d[2], plt_cu_.bbox[0], plt_cu_.bbox[1], plt_cu_.bbox[2], ray.gID, (long long)((P)-plt_cu_.data_lo), (long long)(plt_cu_.data_hi-plt_cu_.data_lo)); \
		image[x+y*plt_cu_.Nx] = 0xFF00FF; return; }
	QPLT_CU_CHECK_PTR(ptr, "start");
#else
#define QPLT_CU_CHECK_PTR(P, WHAT)
#endif
	float sum_w = 0, f0 = 0;  if(plt_cu_.D3mingrad) plt_cu_.accessor.conv<AID>(ptr, (const char**)nb, &f0);
	while(1){
		QPLT_CU_CHECK_PTR(ptr, "step");
    //if(x==300 && y==300) printf("%i: %g\n", __LINE__, sum_w);
		float f; plt_cu_.accessor.conv<AID>(ptr, (const char**)nb, &f);
		if(f==f && (!plt_cu_.D3mingrad || fabs(f0-f)>plt_cu_.cr_grad)){  // NaN voxels are transparent, as in the CPU version (f==f: isnan misbehaved on Windows GPUs)
			Vecf<4> dC = plt_cu_.color(f);
			float w = /*(1+10*fabs(f0-f)*_df)*/dC[3]*ray.len*plt_cu_._max_len*(1-sum_w);
			if(sum_w+w<plt_cu_.lim_w){ C += dC*w; sum_w += w; }
			else { C += dC*(plt_cu_.lim_w-sum_w); break; }
		}
		f0 = f;
		if(++pos3d[ray.gID]>=plt_cu_.bbox[ray.gID]){ /*C = C + QpltColor::rgb_t(color(f)).inv()*(.99-sum_w);*/ break; }  // переходим в следующий воксель, проверяем границу
		ptr += plt_cu_.deltas[ray.gID];	 				
		if(DIFF) for(int k=0; k<6; k++){ int a = k/2, d = (k%2*2-1)*plt_cu_.dsgn[a], p = pos3d[a]+d; nb[k] = ptr + (0<=p && p<plt_cu_.bbox[a] ? d*plt_cu_.deltas[a]: 0); }
		ray.next();  
	}
	// image[x+y*Nx] = sum_w? colorF2I(C): 0xFFFFFFFF;
	image[x+y*plt_cu_.Nx] = colorF2I(C); 
}
#endif //__NVCC__
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMeshPlotter3D<AID>::plot(int *image) const {
	// for(int i=0; i<int(sizeof(*this)/8-1); i++) printf("%lx\n", ((uint64_t*)this)[i]);
	// for(int i=0; i<1; i++) printf("%lx\n", ((uint64_t*)this)[i]);
#ifndef __NVCC__ // CPU render
	WERR(Nx, Ny); 
	int cID = 0;  constexpr int DIFF = (AID>>3)&7; // какой то static method в accessor?
	
#pragma omp parallel for firstprivate(cID)
	for(int y=0; y<Ny; y++){
		const char *nb[6] = {nullptr};  // ???
		int y0 = ibmin[1]+y;
		for(int x=0; x<Nx; x++){
			int x0 = ibmin[0]+x; Vecf<2> r;
			if(!flats[cID].image2flat(x0, y0, r)){
				bool miss = true;
				for(int i=1; i<3; i++) if(flats[(cID+i)%3].image2flat(x0, y0, r)){ miss = false; cID = (cID+i)%3; break; }
				if(miss){ image[x+y*Nx] = 0xFFFFFFFF; continue; }
			}
			auto & flat = flats[cID];
			Ind<2> pos;  Vecf<2> X; flat.mod_coord(r, pos, X);  const char* ptr = flat.get_ptr(pos);
			Ind<3> pos3d; flat.pos2to3(pos, pos3d); 
			Vecf<4> C;  auto ray = vtx.trace(cID, X);
			if(DIFF) for(int k=0; k<6; k++){ int a = k/2, d = (k%2*2-1)*dsgn[a], p = pos3d[a]+d; nb[k] = ptr + (0<=p && p<bbox[a] ? d*deltas[a]: 0); }
			float sum_w = 0, f0 = 0;  if(D3mingrad) accessor.conv<AID>(ptr, (const char**)nb, &f0);
			while(1){
				// WEXT(pos, pos3d, cID, flat.axis[0], flat.axis[1]);
				WASSERT(Ind<3>()<= pos3d && pos3d<bbox, "incorrect pos3d", x, y, r, pos, X, pos3d, bbox, cID, ray.fID, ray.gID, ray.f, ray.g, ray.len); 
				float f = 0; accessor.conv<AID>(ptr, (const char**)nb, &f);
				// if(std::isnan(f)) continue; // прозрачность nan?
				if(!std::isnan(f) && (!D3mingrad || fabs(f0-f)>cr_grad)){
					Vecf<4> dC = color(f);
					float w = /*(1+10*fabs(f0-f)*_df)*/dC[3]*ray.len*_max_len*(1-sum_w);
					if(sum_w+w<lim_w){ C += dC*w; sum_w += w; }
					else { C += dC*(lim_w-sum_w); break; }
				}
				f0 = f;
				if(++pos3d[ray.gID]>=bbox[ray.gID]){ /*C = C + QpltColor::rgb_t(color(f)).inv()*(.99-sum_w);*/ break; }  // переходим в следующий воксель, проверяем границу
				ptr += deltas[ray.gID];
				if(DIFF) for(int k=0; k<6; k++){ int a = k/2, d = (k%2*2-1)*dsgn[a], p = pos3d[a]+d; nb[k] = ptr + (0<=p && p<bbox[a] ? d*deltas[a]: 0); }
 				
				ray.next();  
			}
			// image[x+y*Nx] = sum_w? colorF2I(C): 0xFFFFFFFF;
			image[x+y*Nx] = colorF2I(C); 
		}
	}
#else  //__NVCC__ GPU render
	// on any CUDA error: report it, reset the device (an illegal address error is sticky) and return a blank image
	int *image_cu = nullptr;  cudaError_t err;
#define QPLT_CU_TRY(CALL, WHAT) if((err = (CALL)) != cudaSuccess){ \
		fprintf(stderr, "qplt: CUDA %s failed: %s (%i), XD image skipped\n", WHAT, cudaGetErrorString(err), int(err)); \
		cudaFree(image_cu); dev_data_reset(); cudaDeviceReset(); for(int i=0; i<Nx*Ny; i++) image[i] = 0xFFFFFFFF; return; }

	// 1. копируем данные фрейма на GPU (если там еще не лежит копия этого фрейма)
	size_t sz = data_hi-data_lo;
	if(dev_src!=data_lo || dev_sz<sz){
		if(dev_sz<sz){ dev_data_reset(); QPLT_CU_TRY(cudaMalloc((void**)&dev_data, sz), "cudaMalloc (frame data)"); dev_sz = sz; }
		QPLT_CU_TRY(cudaMemcpy(dev_data, data_lo, sz, cudaMemcpyHostToDevice), "cudaMemcpy (frame data)");
		dev_src = data_lo;
	}

	// 2. иницализируем плоттер (указатели на данные переводим на копию на GPU) и изображение
	QpltMeshPlotter3D<AID> plt = *this;
	for(int i=0; i<3; i++) plt.flats[i].ptr0 = dev_data + (flats[i].ptr0-data_lo);
	plt.data_lo = dev_data;  plt.data_hi = dev_data+sz;
	QPLT_CU_TRY(cudaMemcpyToSymbol(plt_cu, &plt, sizeof(plt), 0, cudaMemcpyHostToDevice), "cudaMemcpyToSymbol (plotter)");
	QPLT_CU_TRY(cudaMalloc((void**)&image_cu, size_t(Nx)*Ny*4), "cudaMalloc (image)");

	// 3. запускаем ядро
	plotXD<AID><<<dim3(Nx/16+1, Ny/16+1), dim3(16,16)>>>(image_cu);
	QPLT_CU_TRY(cudaGetLastError(), "kernel launch");

	// 4. копируем изображение с GPU в res
	QPLT_CU_TRY(cudaMemcpy(image, image_cu, size_t(Nx)*Ny*4, cudaMemcpyDeviceToHost), "kernel / cudaMemcpy (image)");
	cudaFree(image_cu);
#undef QPLT_CU_TRY
#endif //__NVCC__
}
//------------------------------------------------------------------------------
#ifdef QWERTY
float f; accessor.conv<PAID>(ptr, (const char**)nb, &f);
if(!D3mingrad || fabs(f0-f)>cr_grad){
	float w = /*(1+10*fabs(f0-f)*_df)*/ray.len*_max_len*(1-sum_w);
	if(sum_w+w<lim_w){ C = C + QpltColor::rgb_t(color(f)).inv()*w; sum_w += w; }
	else { C = C + QpltColor::rgb_t(color(f)).inv()*(lim_w-sum_w); break; }
	// v1:
	// if(sum_w+w<lim_w){ C = C*(1-w) + QpltColor::rgb_t(color(f))*w; sum_w += w; }
	// else { C = C*(1-w) + QpltColor::rgb_t(color(f))*(lim_w-sum_w); break; }
 }
f0 = f;
if(++pos3d[ray.gID]>=bbox[ray.gID]){ /*C = C + QpltColor::rgb_t(color(f)).inv()*(.99-sum_w);*/ break; }  // переходим в следующий воксель, проверяем границу
#endif


	/*
		float w=col.w*density*(1.0f - sum.w); col.w = 1;
	    if(sum.w + w < opacityThreshold) sum += col * w;
	    else { sum += col * (opacityThreshold-sum.w); break; }
		т.е. sum += col * w; пока не достигнут порог непрозрачности.
		float4 col; -- это цвет текущей точки:
		в простейшем варианте
		col = im.get_color_for3D(tex3D(data3D_tex, pos_sc.x, pos_sc.y, pos_sc.z));
		
		по умолчанию
		density = 0.5; opacity = 0.95;
		можно менять, opacity = 0..1; density > 0;
	*/
//------------------------------------------------------------------------------
/*struct QpltMeshPlotter3DInst{ // только для инстацирования всех шаблонов
	QpltAccessor accessor;
	template<int AID> void plot_impl(int *im){ QpltMeshPlotter3D<AID> plt; plt.plot(im); }
	CALL_FUNC(QpltMeshPlotter3DInst, plot);
	void plot(int *im){ accessor.call<call_plot>(*this, im); } 
};
QpltMeshPlotter3DInst qwe;*/
//------------------------------------------------------------------------------
template void aiw::QpltMeshPlotter3D<4>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<5>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<6>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<7>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<18>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<19>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<27>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<26>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<38>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<39>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<68>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<69>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<70>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<71>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<74>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<75>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<82>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<83>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<90>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<91>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<102>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<103>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<134>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<135>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<154>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<155>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<166>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<167>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<196>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<197>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<198>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<199>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<210>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<211>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<218>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<219>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<230>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<231>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<260>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<261>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<274>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<275>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<322>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<323>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<332>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<333>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<334>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<335>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<356>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<357>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<362>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<363>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<370>::plot(int *) const;
template void aiw::QpltMeshPlotter3D<371>::plot(int *) const;
//------------------------------------------------------------------------------
