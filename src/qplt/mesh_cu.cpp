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
void aiw::QpltMesh::data_free_impl(){ WERR(this); mem.reset(); }  // выгружает данные из памяти
void aiw::QpltMesh::data_load_impl(){                 // загружает данные в память
	fin->seek(mem_offset);
#ifndef __NVCC__
	mem = fin->mmap(data_sz, 0);
#else //__NVCC__
	mem = std::shared_ptr<T>(new CuMemAlloc(data_sz)); fin->fread(mem->get_addr(), data_sz);
#endif //__NVCC__
	WERR(this, mem.get(), mem_limit);
	// WOUT(mem_offset, mem->get_addr());
}
//------------------------------------------------------------------------------
template <int AID> void aiw::QpltMeshPlotter3D<AID>::plot(int *image) const {
#ifndef __NVCC__ // CPU render
	WERR(Nx, Ny); 
	int cID = 0;
#pragma omp parallel for firstprivate(cID)
	for(int y=0; y<Ny; y++){
		char *nb[6] = {nullptr};  // ???
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
			float sum_w = 0, f0;  if(D3mingrad) accessor.conv<AID>(ptr, (const char**)nb, &f0);
			while(1){
				// WEXT(pos, pos3d, cID, flat.axis[0], flat.axis[1]);
				WASSERT(Ind<3>()<= pos3d && pos3d<bbox, "incorrect pos3d", x, y, r, pos, X, pos3d, bbox, cID, ray.fID, ray.gID, ray.f, ray.g, ray.len); 
				float f; accessor.conv<AID>(ptr, (const char**)nb, &f);
				if(!D3mingrad || fabs(f0-f)>cr_grad){
					float w = /*(1+10*fabs(f0-f)*_df)*/ray.len*_max_len*(1-sum_w);
					if(sum_w+w<lim_w){ C += color(f)*w; sum_w += w; }
					else { C += color(f)*(lim_w-sum_w); break; }
				}
				f0 = f;
				if(++pos3d[ray.gID]>=bbox[ray.gID]){ /*C = C + QpltColor::rgb_t(color(f)).inv()*(.99-sum_w);*/ break; }  // переходим в следующий воксель, проверяем границу
				ptr += deltas[ray.gID];	 				
				ray.next();  
			}
			// image[x+y*Nx] = sum_w? 0xFFFFFFFF-colorF2I(C): 0xFFFFFFFF;
			image[x+y*Nx] = sum_w? colorF2I(C): 0xFFFFFFFF;
		}
	}
#else  //__NVCC__ GPU render
		// 1. иницализируем плоттер и изображение
		CuPlotter<PAID> cpu_plt;

		//2. запускаем ядро
		CuPlotter<PAID> cpu_plt;
		int *image; cudaMalloc((void**)&image, im_size.prod());
		plotX(... image);    
		
		cudaMemcpy(&(im.buf[0]), image, im.buf.size(), cudaMemcpyDeviceToHost); // 3. копируем изображение с GPU в res
#endif //__NVCC__
}
//------------------------------------------------------------------------------

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
