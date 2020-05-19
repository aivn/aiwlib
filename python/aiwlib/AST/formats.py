# -*- encoding: utf-8 -*-
'''AST convertions to some formats

Copyright (C) 2002-2013, 2017 Anton V. Ivanov <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0
'''
from nodes import *
from nodes import _N2P
#-------------------------------------------------------------------------------
#   gnuplot
#-------------------------------------------------------------------------------
def add_gp_format():
    BaseOp.__gp__ = BaseOp.__pyt__
    FloordivOp.__gp__ = lambda self: '%s/%s'%self._bk('(%s)', _N2P)
    spaceOp['fabs'].__gp__ = lambda self: 'abs(%s)'%self.a
#-------------------------------------------------------------------------------
#   LaTeX
#-------------------------------------------------------------------------------
nan = float('nan')
def _n2f( x, pattern, decimal_dot=',' ): a, p = ( ('%g'%x).split('e')+[''] )[:2]; a=a.replace('.',decimal_dot); return pattern%(a,p) if p else a
frac_table = dict( sum([ [ ( float(i)/j, '\\frac{%i}{%i}'%(i,j) ), ( -float(i)/j, '-\\frac{%i}{%i}'%(i,j) ) ] 
                         for j in range(10,1,-1) for i in range(9,0,-1) if float(i)/j!=i/j ], [] ) )
_N2T = lambda x: frac_table.get( x, '{\\rm nan}' if x is nan else _n2f( x, '{%s\\cdot 10^{%s}}' ) ) if x.__class__==float else str(x) 
tex_specials = '''alpha beta gamma delta epsilon varepsilon zeta eta theta vartheta iota kappa lambda mu nu xi pi varphi rho varrho sigma varsigma tau upsilon phi chi
psi omega Gamma Delta Theta Lambda Xi Pi Sigma Upsilon Phi Psi Omega inf ast partial angle triangle square divideontimes boxtimes perp parallel'''.split()
#-------------------------------------------------------------------------------
def _split_alpha_digit(S):
    b, e, L = 0, 0, []
    while e<len(S) :
        if e and S[e].isalpha() !=  S[e-1].isalpha() : L.append(S[b:e]); b = e
        e += 1
    if b!=e : L.append(S[b:e])
    return L
#-------------------------------------------------------------------------------
def _c_name( name, _c_symb, out_pattern ):
    L = sum( map( _split_alpha_digit, name.split('_') ), [] )
#    if len(L[0])>1 and not L[0] in tex_specials : L = [L[0][0],L[0][1:]]+L[1:]
    L, P = map(_c_symb, L), map(str.isdigit, L)
    if len(L)==1: return L[0]
    for i in range(1, len(L)-1): L[i] += ',' if P[i]==P[i+1] else ''
    return out_pattern%( L[0], ' '.join(L[1:]) )
name2tex = lambda n: _c_name( n, lambda x : '\\'+x if x in tex_specials else '{\\rm %s}'%x if x.isalpha() and len(x)>1 else x, '%s_{%s}' )
#-------------------------------------------------------------------------------
def add_tex_format():
    BaseOp.__tex__ = lambda self: getattr( self, 'p_tex', self.p_pyt )%self._bk( '\\left( %s \\right)', _N2T )
    for c, p in [(ModOp,'\\%%'), (EqOp,'='), (NeOp,'\\neq'), (LeOp,'\\leq'), (GeOp,'\\geq'), (OrOp,'\\or'), (AndOp,'\\and')]: c.p_tex = '%s'+p+'%s'
    FloordivOp.p_tex = '\\frac{\\displaystyle %s}{\\displaystyle %s}'
    PowOp.__tex__ = lambda self: self.a.__tex__(_N2T(self.b)) if hasattr(self.a, 'f_tex') else '{%s}^{%s}'%self._bk(r'\left(%s\right)', _N2T)
    MulOp.__tex__ = lambda self: ( r'%s\cdot\left(%s\right)' if isinstance(self.b, (NegOp, PosOp, InvOp)) else '%s\\,%s' )%self._bk(r'\left(%s\right)', _N2T)

    for k, v in spaceOp.items(): 
        if k.endswith('_bk'): v.p_tex = '\\left'+v.p_txt[:-1]+'\\right'+v.p_txt[-1]
        elif k=='fabs': v.p_tex = '\\left|%s\\right|'
        else: 
            v.f_tex = { 'asin':'arcsin', 'acos':'arccos', 'tan':'tg', 'atan':'arctg', 'log':'ln', 'log10':'log', 'tanh':'th', 'sinh':'sh', 'cosh':'ch' }.get(k,k)
            v.__tex__ = lambda self, pw='': '\\'+self.f_tex+'^{%s}'%pw+( r'{\left(%s\right)}' if getattr(self.a,'priority',0)>MulOp.priority else '{%s}' )%_N2T(self.a)
#-------------------------------------------------------------------------------
