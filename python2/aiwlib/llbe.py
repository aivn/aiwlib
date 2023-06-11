# -*- coding: utf-8 -*-
#
# Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
# 
# Landau-Lifshitz-Bloch Equation work and CMD coeff.

from math import *

Z = lambda p: 4*pi*(sinh(p)/p if fabs(p)>1e-4 else 1+p*p/6.)   # int_sph e^{m p} dm
L = lambda p: 1/tanh(p)-1./p if fabs(p)>1e-4 else p/3.         # Langevien function
dLdp = lambda p: 1./(p*p)-1./sinh(p)**2 if fabs(p)>1e-6 else 1/3.-p*p/15.

def invL(M, newton=True):
    if fabsf(M)<1e-6: return M*3
    if not newton:
        pa, pb = 0., 20.
        for i in range(20):
            if L((pa+pb)/2)<M: pa = (pa+pb)/2
            else: pb = (pa+pb)/2
        return (pa+pb)/2
    p = 1.
    for i in range(10):
	Lp = 1/tanh(p)-1./p if fabs(p)>1e-4 else p/3.
	if p>400 or fabs(Lp-M)<1e-6: break
	p = p-(Lp-M)/dLdp(p)
	if fabs(p)<1e-6: return p
    return p

def Zinv(z):
    pa, pb = 0., 20.
    for i in range(20):
        if Z((pa+pb)/2)<z: pa = (pa+pb)/2
        else: pb = (pa+pb)/2
    return (pa+pb)/2


eta2zeta = lambda M, eta: (eta-M**2)/(1.-M**2)
zeta2eta = lambda M, zeta: zeta+(1-zeta)*M**2

# approximations
M2_MM = lambda MM: 1./3+ (0.4115 + (0.0303+ (0.3523 - 0.1261*MM)*MM)*MM)*MM  # <m_{||p}^2>(<m>**2) = 1-2*M/p ~ 1/3
M2 = lambda M: M2_MM(M*M)
M3 = lambda M: .6026*M*(1+.00669*cosh(5.288*M))

Upsilon = lambda M, eta: (1-eta)/(1.-M**2)*(1.-M2(M))*.5*(1+ ((0.3684 + 0.1873*eta)*eta - (0.3236+0.2523*eta)*M**2)*eta)


# 		inline Vecf<3> Hdiss(const Vecf<3> &M, const Vecf<3> &H){
# 			float MM = M*M;  if(MM<1e-6) return H*(-2.f/3);
# 			float _MM = 1.f/MM,  M2av = M2_MM(MM);
# 			Vecf<3> res;
# 			for(int i=0; i<3; i++) for(int j=0; j<3; j++){
# 					float delta = (i==j), nn = M[i]*M[j]*_MM;
# 					res[i] += ((M2av*(3*nn-delta) + delta-nn)*.5f - delta)*H[j];
# 				}
# 			return res;			
# 		}
# 		inline Vecf<3> Kprec(const Vecf<3> &M, const Vecf<3> &nK){
# 			// float MM = M*M; return (.59256f + (.21515f+.2008f*MM)*MM)*(M*nK)*(M%nK);
# 			float MM = M*M; return (.5926f + (.215f+.2f*MM)*MM)*(M*nK)*(M%nK);
# 		}
# 		inline Vecf<3> Kdiss(const Vecf<3> &M, const Vecf<3> &nK){ 
# 			float MM = M*M; if(MM<1e-6f) return Vecf<3>(); // но это неточно???
# 			float m = sqrt(MM), _m = 1.f/m, beta = M*nK*_m, m3m = M3(m);
# 			// return M*(.069f/m*sinh(3.39f*m)*(1.f-m)*(1.f-3.f*beta)) + M%(M%nK)*( 0.6022f/m*(1.f+0.00669f*cosh(5.288f*m))*beta );
# 			return M*((m3m*_m-1.f)*(3.f*beta*beta-1.f)*.5) + (M%(M%nK))*(m3m/MM*beta);
# 		}
# 		inline float Wanis(const Vecf<3> &M, const Vecf<3> &nK){
# 			float MM = M*M;
# 			if(MM<1e-6) return 1.f/3;
# 			float MnK = M*nK, MnK2 = MnK*MnK/MM, m2 = M2_MM(MM);
# 			return m2*MnK2 + (1.f-m2)*(1.f-MnK2)*.5f;
# 		}
		
# 		// corr. MD
# 		inline float Lambda(float M, float eta){
# 			float MM = M*M, EE = eta*eta;
# 			return (1.f-eta)/(1.f-MM)*( -0.6639f + (-0.7617f +0.2689f*M +0.3472f*MM)*eta + (0.2718f - 1.367f*eta +0.5078f*EE -0.418f*M +1.833f*MM)*EE);
# 		}
# 		inline float Psi(float M, float eta, float MnK){ return (0.46134f*M*M - 1.3836f*MnK*MnK)*(1.f-eta)*M; }
# 		inline float Q3(float eta){ return -(0.69279f + (-0.24455f + (1.1055f -0.53462f*eta)*eta)*eta)*eta*(1-eta); }
