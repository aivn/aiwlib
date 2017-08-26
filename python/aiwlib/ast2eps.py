# -*- encoding: utf-8 -*-
'''EPS settings for SYMBALG

Copyright (C) 2002-2013 Anton V. Ivanov, KIAM RAS, Moscow.
This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991) or later
'''
#-----------------------------------------------------------------------------------------------------------
from expressions import *

BaseOp.__eps__ = lambda self: getattr( self, 'p_eps', getattr( self, 'p_txt', self.p_pyt ) )%self._bk('(%s)', _N2E)
PowOp.__eps__ = lambda self: self.a.__eps__(_N2E(self.b)) if hasattr(self.a, 'f_tex') else '{%s}^{%s}'%self._bk('(%s)', _N2E)
MulOp.p_eps = '%s{/Symbol *}%s'

for c, p in [(MulOp,'*'), (NeOp,'\\271'), (GeOp,'\\263'), (OrOp,'\\or'), (AndOp,'\\and')]: c.p_tex = '%s'+p+'%s'
FloordivOp.p_tex = '\\frac{\\displaystyle %s}{\\displaystyle %s}'

for k, v in namespace.items(): 
    if k.endswith('_bk'): v.p_tex = '\\left'+v.p_txt[:-1]+'\\right'+v.p_txt[-1]
    elif k=='fabs': v.p_tex = '\\left|%s\\right|'
    else: 
        v.f_tex = { 'asin':'arcsin', 'acos':'arccos', 'tan':'tg', 'atan':'arctg', 'log':'ln', 'log10':'log', 'tanh':'th', 'sinh':'sh', 'cosh':'ch' }.get(k,k)
        v.__eps__ = lambda self, pw='': self.f_tex+'^{%s}'%pw+( r'{\left(%s\right)}' if getattr(self.a,'priority',0)>MulOp.priority else '{%s}' )%_N2T(self.a)
#-----------------------------------------------------------------------------------------------------------
eps_specials = { 'alpha':'a', 'beta':'b','gamma':'g','delta':'d','epsilon':'e','varepsilon':'e','zeta':'z','eta':'h','theta':'q','vartheta':'J',
                 'iota':'i','kappa':'k','lambda':'l','mu':'m','nu':'n','xi':'x','pi':'p','varphi':'j','rho':'r','varrho':'r','sigma':'s','varsigma':'V',
                 'tau':'','upsilon':'u','phi':'f','chi':'c','psi':'y','omega':'w','Gamma':'G','Delta':'D','Theta':'Q','Lambda':'L','Xi':'X','Pi':'P',
                 'Sigma':'S','Upsilon':'\\241','Phi':'F','Psi':'Y','Omega':'W','inf':'\\245', 'ast':'*','partial':'\\266', 'leq':'\\243', 'geq':'\\263', 
                 'neq':'\\271' }
#-----------------------------------------------------------------------------------------------------------
#< 341
#> 361
#sqrt \\326

