#!/usr/bin/python

import sys

fact = lambda n: float(reduce(int.__mul__, range(1, n+1)) if n else 1)
pow_xc = lambda c, n: Polinome(*[c**(n-i)*fact(n)/fact(i)/fact(n-i) for i in range(n+1)]) # (x+c)**n
extend = lambda L1, L2: map(lambda a, b: (0. if a is None else a, 0. if b is None else b), L1, L2) 


class Polinome:
    def __init__(self, *P): self.P = map(float, P)
    def __str__(self): return str(reduce(lambda R, c: '%s+x*(%s)'%(c, R), reversed(self.P))).replace('(0.0+', '(').replace('+x*(1.0)', '+x').replace('+x*(-1.0)', '-x')
    def __mul__(self, other): return Polinome(*[a*other for a in self.P])
    def __add__(self, other): return Polinome(*[a+b for a, b in extend(self.P, other.P)])
    def __sub__(self, other): return Polinome(*[a-b for a, b in extend(self.P, other.P)])
    def offset(self, c): # ==> P(x+c)
        return sum([pow_xc(c, n)*p for n, p in enumerate(self.P)], Polinome())
    def __call__(self, x): return Polinome(sum([p*x**n for n, p in enumerate(self.P)]))
    def integrate(self, i): #i - center polinome, ==> [P-, P+]
        PI = Polinome(0, *[p/(n+1.) for n, p in enumerate(self.P)])
        return [PI.offset(.5)-PI(i-.5), PI(i+.5)-PI.offset(-.5)]

print '    template <int K> inline double bspline(double x){ WRAISE("oops...", K); return 0; }\n'

Plist = [Polinome(1.)]
for k in range(1, int(sys.argv[1])+1):
    print '    template <> inline double bspline<%i>(double x){\n    x = fabs(x);'%k
    ##for i in range(not k%2, 1+k/2): print '    if(x<%g) return %s;'%(i+.5*(k%2), Plist[i])
    for i in range(k/2, k): print '        if(x<%g) return %s;'%(i-k*.5+1, Plist[i])
    print '        return 0.;\n    }'
    L = [P.integrate(i-k*.5+.5) for i, P in enumerate(Plist)]
    Plist = [L[0][0]]+[a[1]+b[0] for a, b in zip(L[:-1], L[1:])]+[L[-1][1]]

#for i, P in enumerate(Plist):
#    for j in range(100):
#        x = -.5*len(Plist)+i+j*1e-2
#        print x, P(x)
