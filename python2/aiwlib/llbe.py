# -*- coding: utf-8 -*-
#
# Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com>
# Licensed under the Apache License, Version 2.0
# 
# Landau-Lifshitz-Bloch Equation work and CMD coeff.

from math import *

Z = lambda p: 4*pi*(sinh(p)/p if fabs(p)>1e-4 else 1+p*p/6.)   # int_sph e^{m p} dm
L = lambda p: 1/tanh(p)-1./p if fabs(p)>1e-4 else p/3.         # Langevien function
# 1./(p*p)-1./sinh(p)**2 при p-->inf: ~ 1/p**2
dLdp = lambda p: 1/3.-p*p/15. if fabs(p)<1e-6 else 1./(p*p)-1./sinh(p)**2 if fabs(p)<40 else 1./(p*p)

def invL(M, newton=True):
    if fabs(M)<1e-6: return M*3
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

#Sigma_z = lambda M: M2acc(M)-1
#Sigma_z = lambda M: M2(M)-1

eta2zeta = lambda M, eta: (eta-M**2)/(1.-M**2) if M<1 and eta<1 else 0
zeta2eta = lambda M, zeta: zeta+(1-zeta)*M**2

# approximations
M2_MM = lambda MM: 1./3+ (0.4115 + (0.0303+ (0.3523 - 0.1261*MM)*MM)*MM)*MM  # <m_{||p}^2>(<m>**2) = 1-2*M/p ~ 1/3
M2 = lambda M: M2_MM(M*M)
M3 = lambda M: .6026*M*(1+.00669*cosh(5.288*M))
M2acc = lambda M: 1-2*M/invL(M) if fabs(M)>1e-6 else 1./3

def XiH(M, H):   # Hdiss
    MM = sum(x**2 for x in M)
    if MM<1e-6: return [-2.*x/3 for x in H]
    _MM, M2av, res = 1./MM, M2_MM(MM), [0.]*3
    for i in (0,1,2):
        for j in (0,1,2):
            delta, nn = (i==j), M[i]*M[j]*_MM
            res[i] += ((M2av*(3*nn-delta) + delta-nn)*.5 - delta)*H[j]
    return res

def Xi(M):  # Xi_xx, yy, zz, xy, xz, yz
    MM = sum(x**2 for x in M)
    if MM<1e-6: return [-2./3]*3+[0.]*3
    _MM, M2av, res = 1./MM, M2_MM(MM), [None]*6
    for i, j, k in [(0,0,0), (1,1,1), (2,2,2), (0,1,3), (0,2,4), (1,2,5)]:
        delta, nn = (i==j), M[i]*M[j]*_MM
        res[k] = (M2av*(3*nn-delta) + delta-nn)*.5 - delta
    return res		       
Xi_zz = lambda M: M2(M)-1
    
def Theta(M, nK):
    MM = sum(x**2 for x in M)
    if MM<1e-6: return [0.]*3  # но это неточно???
    m = sqrt(MM);  MnK = sum(x*y for x, y in zip(M, nK))
    beta = MnK/m; m3m = M3(m)
    # M%(M%nK) = M*MnK - nK*MM
    return [a*((m3m/m-1.)*(3.*beta*beta-1.)*.5 + m3m/MM*beta*MnK) - b*m3m*beta  for a, b in zip(M, nK)]

Upsilon = lambda M, eta: (1-eta)/(1.-M**2)*(1.-M2(M))*.5*(1+ ((0.3684 + 0.1873*eta)*eta - (0.3236+0.2523*eta)*M**2)*eta)
def Upsilon2(mu, eta):
    p = invL(mu, 0)
    return  mu*(1-eta)/((1-mu**2)*p)*(1 + 0.448413*eta**2 + 0.308645*eta**3 -0.475256*eta*mu**2 -0.291626*eta**2*mu**2)

Psi = lambda m, eta, MnK: (0.46134*m*m - 1.3836*MnK*MnK)*(1.-eta)*m

def Lambda(M, eta):
    MM, EE = M*M, eta*eta;
    return (1.-eta)/(1.-MM)*( -0.6639 + (-0.7617 +0.2689*M +0.3472*MM)*eta + (0.2718 - 1.367*eta +0.5078*EE -0.418*M +1.833*MM)*EE)


# 		inline Vecf<3> Kprec(const Vecf<3> &M, const Vecf<3> &nK){
# 			// float MM = M*M; return (.59256f + (.21515f+.2008f*MM)*MM)*(M*nK)*(M%nK);
# 			float MM = M*M; return (.5926f + (.215f+.2f*MM)*MM)*(M*nK)*(M%nK);
# 		}
# 		inline float Wanis(const Vecf<3> &M, const Vecf<3> &nK){
# 			float MM = M*M;
# 			if(MM<1e-6) return 1.f/3;
# 			float MnK = M*nK, MnK2 = MnK*MnK/MM, m2 = M2_MM(MM);
# 			return m2*MnK2 + (1.f-m2)*(1.f-MnK2)*.5f;
# 		}
		
# 		// corr. MD
# 		inline float Q3(float eta){ return -(0.69279f + (-0.24455f + (1.1055f -0.53462f*eta)*eta)*eta)*eta*(1-eta); }
