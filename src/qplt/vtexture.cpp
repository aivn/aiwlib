/**
 * Copyright (C) 2020-21 Antov V. Ivanov  <aiv.racs@gmail.com>  with support Kintech Lab
 * Licensed under the Apache License, Version 2.0
 **/

#include "../../include/aiwlib/iostream"
#include "../../include/aiwlib/qplt/vtexture"
using namespace aiw;

/* 

      ee[]                     f in [0:1] = MM[fID]*r --- система координат в верхнем флэте
       |                       r = MM[fID].inv()*f --- общая система координат в плоскости сцены
      /+=================+|    g = MM[gID]*r - MM[gID]*ee[gID] --- система координат нижнего (следующего) флэта
     /||                /||    ID --- номер вектора ee ортогонального плоскости флэта
    //||               / ||    
   // ||              /  ||    главный флэт - тот который имеет пересечение с дальним   
  /+--||-------------+   ||    для пересечения не-главных флэтов ---    len = g.min()+1-f.max()
  ||  ||             |   ||    для пересечния главного с неглавными --- len = g[xxx] или 1-f[xxx], причем всегда координаты неглавного и номер оси один и тот же
  ||  ||             |   ||          но какая то фигня получается с треугольными областями
  ||  ||             |   ||          
  ||  |+=================+/--ee[]    другой вариант - строим плоскость по трем точкам. Всегда работаем с f. Две точки на внешней границе флэта, (0,1)|(1,0) и (1,1)
  ||  // center      |  //           третья точка либо (0,0) либо нижний флэт и ее как то надо искать;-(  
  || //              | //   
  ||//               |//
  |+=================+/
  /
ee[]
*/

//------------------------------------------------------------------------------
void aiw::VTexture::init(const QpltPlotter &plt){
	main_flat = 0;
	Matr<2, 2, float> MM[3];  // это матрицы перехода от общих координат к координатам флэта как f = MM[fID]*r
	for(int fID=0; fID<3; fID++){
		// File fee("vtx/ee%.dat", "w", fID);
		const auto &f = plt.get_flat(fID);
		int axis[3] = { f.axis[0], f.axis[1], 3-f.axis[0]-f.axis[1] }; 
		for(int i=0; i<2; i++) flips[fID][i] = plt.icenter&(1<<axis[i]);
		
		Vecf<2> ee[3]; for(int i=0; i<3; i++) ee[i] = plt.orts[axis[i]]; // fee("0 0\n%\n\n0 0\n%\n", ee[0], ee[1]);
		for(int i=0; i<2; i++){   // это стандартная операция, неплохо бы ее вынести в какую то фнукцию?
			Vecf<2> perp(-ee[i][1], ee[i][0]); if(perp*ee[1-i]<0) perp = -perp;
			perp /= perp*ee[1-i]; MM[fID].row(1-i, perp);
		}
		C0[fID] = MM[fID]*plt.orts[axis[2]];
		if(Vecf<2>()<=-C0[fID] && -C0[fID]<=Vecf<2>(1.f)) main_flat = fID;  
		else laxis[fID] = ee[1]*ee[2] < ee[0]*ee[2];
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
		// WERR(fID, main_flat, laxis[fID]);
 	}
	// exit(1);

	// f = MM[i]*r --> r = MM[i].inv()*f, g = MM[j]*(r-off[j]) = MM[j]*MM[i].inv()*f - MM[j]*off[j] = M[i][j]*f - C0[j]
	for(int i=0; i<3; i++) for(int j=0; j<3; j++) M[i][j] = MM[j]*MM[i].inv();

	// plt.scale не имеет значения (если нет переспективы), а вот plt.bbox очень важен. Должна быть какая то однородная система координат?

	/*
	// тут нужно вывести в .dat  файл флэты и пр, и вообще как то все протестить
	File ff[3][3]; for(int i=0; i<3; i++) for(int j=0; j<3; j++){ ff[i][j] = File("vtx/%-%.dat", "w", i, j); ff[i][j].printf("#:x1 y1 l fx fy gx gy\n"); }
	for(int fID=0; fID<3; fID++){
		for(int i=0; i<101; i++)
			for(int j=0; j<101; j++){
				auto I = trace(fID, vecf(i*1e-2, j*1e-2));
				ff[fID][I.gID]("% % % %\n", MM[fID].inv()*I.f, I.len, I.f, I.g);
			}
	}
	// exit(1);
	*/
}
//------------------------------------------------------------------------------
