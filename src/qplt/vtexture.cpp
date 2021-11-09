/**
 * Copyright (C) 2020-21 Antov V. Ivanov  <aiv.racs@gmail.com>  with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/iostream"
#include "../../include/aiwlib/qplt/vtexture"
using namespace aiw;
//------------------------------------------------------------------------------
//   |...\   mesh      /
//   |    \    / \    /
//   |     \  /   \  /
//   |      \/     \/
//   |      |\voxel/  
//   |      | \   /      
//   |      |  \ / start coords
// --+------+---0------------->
// start  vstart         image
//

/*    / \
     /  /|
    |\ / |
    | V  |
    | | /
     \|/

*/

aiw::VTexture::VTexture(const QpltPlotter &plt){
	Matr<2, 2, float> MM[3];  // это матрицы перехода от общих координат к координатам флэта как f = MM[fID]*r
	for(int i=0; i<3; i++) flips[i] = plt.icenter&(1<<i);
	for(int fID=0; fID<3; fID++){
		// File fee("vtx/ee%.dat", "w", fID);
		const auto &f = plt.get_flat(fID);
		for(int i=0; i<2; i++) axis[fID][i] = f.axis[i];
		axis[fID][2] = 3 - axis[fID][0] - axis[fID][1];
		
		Vecf<2> ee[2] = { plt.orts[f.axis[0]], plt.orts[f.axis[1]] }; // fee("0 0\n%\n\n0 0\n%\n", ee[0], ee[1]);
		for(int i=0; i<2; i++){   // это стандартная операция, неплохо бы ее вынести в какую то фнукцию?
			Vecf<2> perp(-ee[i][1], ee[i][0]); if(perp*ee[1-i]<0) perp = -perp;
			perp /= perp*ee[1-i]; MM[fID].row(1-i, perp);
		}
		C0[fID] = MM[fID]*plt.orts[axis[fID][2]];
		// C1[fID] = MM[fID]*C0[fID];

		// проверяем MM --- работает!
		/*
		File fMM("vtx/gf%.dat", "w", fID); fMM.printf("#:x y fx fy x2 y2\n");
		Vecf<2> A(-1.f), B(1.f); // A <<= ee[0]; A <<= ee[1];  B >>= ee[0]; B >>= ee[1];
		for(int i=0; i<101; i++) for(int j=0; j<101; j++){
				Vecf<2> r = A + ((B-A)&vecf(i*1e-2, j*1e-2)), f = MM[fID]*r;
				if(Vecf<2>()<=f && f<=Vecf<2>(1.f)) fMM("% % %\n", r, f, MM[fID].inv()*f);
			} 
		*/	
 	}
	// exit(1);

	// f = MM[i]*r --> r = MM[i].inv()*f, g = MM[j]*(r-off[j]) = MM[j]*MM[i].inv()*f - MM[j]*off[j]
	for(int i=0; i<3; i++) for(int j=0; j<3; j++) M[i][j] = MM[j]*MM[i].inv();  
	// plt.scale не имеет значения (если нет переспективы), а вот plt.bbox очень важен. Должна быть какая то однородная система координат?

	/* 
	// тут нужно вывести в .dat  файл флэты и пр, и вообще как то все протестить
	File ff[3][3]; for(int i=0; i<3; i++) for(int j=0; j<3; j++){ ff[i][j] = File("vtx/%-%.dat", "w", i, j); ff[i][j].printf("#:fx fy x1 y1 gx gy x2 y2 l\n"); }
	for(int fID=0; fID<3; fID++){
		for(int i=0; i<51; i++)
			for(int j=0; j<51; j++){
				auto I = trace(fID, vecf(i*2e-2, j*2e-2));
				ff[fID][I.axe]("% % % % %\n", I.fpos, MM[fID].inv()*I.fpos, I.next_fpos, M[fID][I.axe]*I.next_fpos, I.len);
			}
	}
	exit(1);
	 */
}
void aiw::VTexture::Iterator::conf(){ // настраивает axe, len, next_fpos
	next_fpos = fpos - vtx->C0[flat];
	if(Vecf<2>()<=next_fpos && next_fpos<Vecf<2>(1.f)) axe = flat; // это можно оптимизировать, для некоторых флэтов это невозможно
	else for(int i=1; i<3; i++){
			axe = (flat+i)%3;
			next_fpos = vtx->M[flat][axe]*fpos - vtx->C0[axe];
			if(Vecf<2>()<=next_fpos && next_fpos<Vecf<2>(1.f)) break; // вторая проверка лишняя?
			// WASSERT(i!=2, "oops...", flat, axe, fpos, next_fpos);
		}	
	// len = vtx->Alen[flat][axe] +  vtx->Blen[flat][axe]*fpos;
	// len = flat==axe? 1.f : 1-fpos.min();
	// len = flat==axe? 1.f : 1+next_fpos.min()-fpos.max();
	len = 1+next_fpos.min()-fpos.max(); if(len>1) len = 1;
	// WASSERT(-1e-6<=len && len<=1.000001, "incorrect len", len, fpos, next_fpos, axe, flat);
}

/*
void aiw::VTexture::VTexture(const QpltPlotter &plt){
	// 1. определяем размер текстуры, для этого надо определить размер вокселя в пикселях и как то разбораться с системой координат
	for(int fi=0; fi<plt->flats_sz(); fi++){
		auto f = plt->get_flat(fi);
		Vecf<2> pp[4]; for(int i=0; i<4; i++) pp[i] = f.ppf[i];
		// 1.1. ищем центральную вершину, общую для всех трех флэтов. Лучше ее номер посчитать заранее в Plotter::init. Эта вершина будет являться началом коррдинат
		// 1.2. нам надо схлопнуть каждый из флэтов вдоль ребра в bbox раз, фактически у нас есть три оси (ребра флэтов), флэт задается парой номеров ребер
		// 1.3. после этого можно определить размер текстуры как размер изображения*размер новых флэтов/размер старых флэтов
		// еще нам надо понять как преобразовывать rpos в точку текстуры. Номер флэта мы получаем в trace, для флэта есть флипы по каждой из двух осей???
		
	}
	
	// 2. заполняем текстуру
	// в приниципе текстура может быть дефрагментирована, но непонятно нафига
}

void aiw::VTexture::init(){    // определяет параметры следующие далее
	nZ = vec(-sin(theta)*cos(phi), -sin(theta)*sin(phi), -cos(theta));
	nX = vec(-sin(phi), cos(phi), 0);
	nY = vec(-cos(theta)*cos(phi), -cos(theta)*sin(phi), sin(theta));
	Vec<2> a0, b0; for(Ind<3> pos; pos^=ind(2); ++pos){ Vec<2> p((pos&box)*nX, (pos&box)*nY); a0 <<= p; b0 >>= p; } // пределы проекции сетки
	Vec<2> ab0 = b0 - a0; start = a0 +(offset&ab0);  nX *= scale[0];  nY *= scale[1]; // сдвигаем и шкалируем изображение
	step = ab0/image_sz&scale; _step = 1./step; // start -= step*.5;

	Vec<2> a[4], b[4]; vstart = a[0] = b[0] = prj(Vec<3>());
	for(Ind<3> pos; pos^=ind(2); ++pos){ Vec<2> p = prj(pos); a[0] <<= p; b[0] >>= p; }  // пределы проекции центрального и соседних вокселей
	vstart -= a[0];
	// vstart = a[0];
	vtx_sz = (b[0]-a[0])/step+ind(2);  fragments.resize(vtx_sz.prod());  // размер текстуры
	
	int64_t mul = 1;  
	for(int i=0; i<3; i++){
		if(nZ[i]<-1e-6) grains[i] = -1;    // расположение граней вокселя относительно экрана, -1 --- левая грань дальняя
		else if(nZ[i]>1e-6) grains[i] = 1;                                                 //  +1 --- правая грань дальняя 
		else { grains[i] = 0; deltas[i] = 0; a[i+1] = a[0]; b[i+1] = b[0]; }               //   0 --- грань || nZ		
		deltas[i] = mul*grains[i]; mul *= box[i];
		Vec<3> dr; dr[i] = grains[i]; a[i+1] = b[i+1] = prj(dr);
		for(Ind<3> pos; pos^=ind(2); ++pos){ Vec<2> p = prj(pos+dr); a[i+1] <<= p; b[i+1] >>= p; } 
	}

	// WOUT(vtx_sz, grains[0], grains[1], grains[2], step);
	// for(int i=0; i<4; i++) WOUT(i, a[i], b[i], b[i]-a[i]);
	// exit(0);

	Matr<3, 3> M[3];  // матрицы для проекции пикселя на грани вокселя, их надо проверять на nan ???
	for(int i=0; i<3; i++){
		if(!grains[i]) continue;
		M[i](0, i) = 1;	for(int x=0; x<3; x++){ M[i](1, x) = nX[x];  M[i](2, x) = nY[x]; }
		prjM[i] = M[i].inv();
	}
	// for(int i=0; i<3; i++) std::cout<<M[i]<<'\n'<<prjM[i]<<"\n\n";
	// exit(0);

	// File fvtx("vtx.dat", "w");
	// fvtx.printf("#:i px py l max_az min_bz ag bg\n");
	
	for(Ind<2> pos; pos^=vtx_sz; ++pos){
		fragment_t &f = fragments[pos[0]+pos[1]*vtx_sz[0]];
		Vec<2> p = (pos&step)+a[0]; // + step*.5; // ???
		bool first = true;  int ag = 0, bg = 0;	double max_az = 0, min_bz = 0;
		for(int i=0; i<3; i++){
			if(!grains[i]) continue;
			double az = iprj(p, i, .5*(1-grains[i]))*nZ, bz = iprj(p, i, .5*(1+grains[i]))*nZ;
			// WOUT(i, iprj(p, i, .5*(1-grains[i])), iprj(p, i, .5*(1+grains[i])), az, bz, max_az, min_bz);
			if(max_az<az || first){	max_az = az; ag = i; }
			if(min_bz>bz || first){	min_bz = bz; bg = i; }
			first = false;
		}
		// WOUT(pos, p, ag, bg, max_az, min_bz, min_bz-max_az);
		if(!(min_bz>=max_az)){ f.len = 0; continue; }
		f.len = min_bz-max_az;
		// if(pos[0]==1) fvtx.printf("%i %g %g %g %g %g %i %i\n", pos[1], p[0], p[1], f.len, max_az, min_bz, ag, bg);
		f.set_axe(bg);
		Ind<2> ij = (p-a[bg+1])/step; f.set_off(ij[0]+ij[1]*vtx_sz[0]);
		// f.d_off = (p-a[bg+1]) - (step&ij);
		f.d_off = (p-a[bg+1])/step - ij; // в разных областях это одинаковое только для одинаковых размеров box по осям???

		f.p = p;

		
		p += step*.5;  first = true;  ag = bg = 0;	max_az = min_bz = 0;
		for(int i=0; i<3; i++){
			if(!grains[i]) continue;
			double az = iprj(p, i, .5*(1-grains[i]))*nZ, bz = iprj(p, i, .5*(1+grains[i]))*nZ;
			// WOUT(i, iprj(p, i, .5*(1-grains[i])), iprj(p, i, .5*(1+grains[i])), az, bz, max_az, min_bz);
			if(max_az<az || first){	max_az = az; ag = i; }
			if(min_bz>bz || first){	min_bz = bz; bg = i; }
			first = false;
		}
		
		
		f.ax_len = (grains[ag]*nX[ag]/sqrt(1-nX[ag]*nX[ag]) - grains[bg]*nX[bg]/sqrt(1-nX[bg]*nX[bg]))*step[0];  // здесь деление на ноль 
		f.ay_len = (grains[ag]*nY[ag]/sqrt(1-nY[ag]*nY[ag]) - grains[bg]*nY[bg]/sqrt(1-nY[bg]*nY[bg]))*step[1];  //    можно не проверять
	}
	box.prod(data_sz);
	// exit(0);
}
//------------------------------------------------------------------------------
aiw::VTexture::iterator aiw::VTexture::trace(Ind<2> ij) const {
	iterator I; Vec<2> p = (ij&step) + .5*step;
	bool first = true;  int ag = 0;  double max_az = 0;  Vec<3> r_ij;
	for(int i=0; i<3; i++){
		if(!grains[i]) continue;
		Vec<3> r = iprj(p, i, (1-grains[i])/2*box[i]);  double az = r*nZ;
		if(max_az<az || first){ max_az = az;  ag = i;  r_ij = r; }
		first = false;
	}
	Ind<3> pos; for(int i=0; i<3; i++) if(fabs(r_ij[i]-round(r_ij[i]))<1e-6) pos[i] = round(r_ij[i]); else pos[i] = floor(r_ij[i]);
	pos[ag] -= (1-grains[ag])/2;
	// if((p-prj(r_ij)).abs()>1e-6){ WOUT(ij, p, r_ij, prj(r_ij), (p-prj(r_ij)).abs()); exit(0); }

	I.i = pos[0]; I.j = pos[1]; I.k = pos[2]; I.px = p[0]; I.py = p[1]; I.rx = r_ij[0]; I.ry = r_ij[1]; I.rz = r_ij[2]; I.g = ag;
	if(ind(0)<=pos && pos<box){
		//Vec<3> dr = r_ij - pos; Vec<2> dp = vec(dr*nX, dr*nY) + vstart; // знаки ???
		//Ind<2> ixy = dp&_step;
		Vec<2> p0 = prj(pos), dp = p-p0+vstart; // угол вокселя
		Ind<2> ixy = dp&_step; // вот тут может возникать ошибка округления
		I.ixy = ixy;
		// if(!(ind(0)<=ixy && ixy<vtx_sz)) WRAISE("", ij, p, r_ij, pos, box, ixy, vtx_sz);
		if(ind(0)<=ixy && ixy<vtx_sz){
			I.cursor = fragments.data()+ixy[0]+ixy[1]*vtx_sz[0];
		// if(I.cursor->len){ // это неправильно, точное попаание в угол не должно приводить к остановке!!!

			I.z = r_ij*nZ;  I.off_x = dp[0]-ixy[0]*step[0]; I.off_y = dp[1]-ixy[1]*step[1];

			I.data = pos[0] + int64_t(box[0])*(pos[1] + box[1]*pos[2]);
			// WOUT(ij, vstart, ixy, vtx_sz);
			
			I.fragments = fragments.data();
			I.vtx_width = vtx_sz[0];
			I.deltas = deltas;
			I.data_sz = data_sz;
		} else { I.cursor = nullptr; WOUT(ij, p, ixy, vtx_sz, r_ij, start, vstart); exit(0); }//  I.z = nanf(""); WOUT(1); }
	} else { I.cursor = nullptr; } // I.z = nanf(""); WOUT(pos, ag, grains[ag], r_ij); }
	return I;
}
//------------------------------------------------------------------------------
*/
