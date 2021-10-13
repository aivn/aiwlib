# -*- encoding: utf-8 -*-
'''AST classes for operations

Copyright (C) 2002-2013, 2017 Anton V. Ivanov <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0
'''

from nodes import *
#-------------------------------------------------------------------------------
#   PARSE
#-------------------------------------------------------------------------------
class ASTError(Exception): pass
_bra, _ket, nan = '%s%%', '%%%s', float('nan')
#-------------------------------------------------------------------------------
def parse_bk(ev):
    'convert ?%...%? ===> (?_bk(...))'
    for bk in '() {} <> [] ||'.split():
        if ev.count(_bra%bk[0])!=ev.count(_ket%bk[1]): raise ASTError('uncomplete brackets %s'%bk)
    return reduce(lambda S, bk: S.replace(_bra%bk[0], '('+bk[1:-1]+'(').replace(_ket%bk[-1], '))'), '(crc_bk) {fig_bk} <ang_bk> |fabs| [rm_bk]'.split(), ev)
#-------------------------------------------------------------------------------
#   EXPAND SYNTAX
#-------------------------------------------------------------------------------
class IfElseOp(BaseOp):
    priority, p_pyt = 12, '%s if %s else %s'
    def __init__(self, expr_y, cond, expr_n): self.expr_y, self.cond, self.expr_n = expr_y, cond, expr_n
    def __repr__(self): return 'IfElseOp( %r, %r, %r )'%(self.expr_y, self.cond, self.expr_n)
    def _bk(self, patt, f=lambda x:x): return ( f(self.expr_y) if getattr( self.expr_y, 'priority', 0 )<=self.priority else patt%f(self.expr_y),                     
                                                f(self.cond) if getattr( self.cond, 'priority', 0 )<=self.priority else patt%f(self.cond), 
                                                f(self.expr_n) if getattr( self.expr_n, 'priority', 0 )<=self.priority else patt%f(self.expr_n) )
    def _get_vars(self): return sum([ getattr( x, '_get_vars', lambda:[] )() for x in (self.expr_y, self.cond, self.expr_n) ], [])
    def __call__(self, *args, **kw_args): return IfElseOp(*[ x(*args, **kw_args) if isinstance(x, BaseOp) else x for x in (self.expr_y, self.cond, self.expr_n) ])    

def _ifch(*args):    
    chain = []
    def wrap(*args):
        if len(args)==2: chain.append(args); return wrap
        elif len(args)==3: return reduce( lambda R, a: IfElseOp(a[0], a[1], R), reversed(chain), IfElseOp(*args) )
        else: raise Exception('incorrect args count in if_chain')
    return wrap(*args)
spaceOp['ifch'] = _ifch
#-------------------------------------------------------------------------------
class JoinOp(BaseOp):
    def __init__(self, *args): self.args = args
    def __str__(self): return ''.join(map(str, self.args))
    def __repr__(self): return 'JoinOp(%s)'%', '.join(map(repr, self.args))
    def __call__(self, *args, **kw_args): return JoinOp(*[ x(*args, **kw_args) if isinstance(x, BaseOp) else x for x in self.args ])    
#-------------------------------------------------------------------------------
class EmptyVar(BaseOp):
    def __str__(self): raise Exception('can not convert to string EMptyVar object!')
    def __repr__(self): return '_'
    def __getitem__(self, args): return JoinOp(*args)
    def __call__(self, arg, bk='()'): return spaceOp['%s_bk'%brakets[str(bk)]](arg)
OrOp.__or__ = lambda self, other : self.a.Or(other) if isinstance(self.b, EmptyVar) else OrOp(self,other)
AndOp.__and__ = lambda self, other : self.a.And(other) if isinstance(self.b, EmptyVar) else AndOp(self,other)
spaceOp['_'] = EmptyVar()
#-------------------------------------------------------------------------------
#   C++ OUT RULES
#-------------------------------------------------------------------------------
BaseOp.__cpp__, PowOp.p_cpp = lambda self: getattr( self, 'p_cpp', self.p_pyt )%self._bk('(%s)'), 'pow(%s, %s)'
BoolAndOp.p_cpp, BoolOrOp.p_cpp, Not.p_cpp, FloordivOp.p_cpp = '(%s)&&(%s)', '(%s)||(%s)', '!(%s)', '%s/%s'
Var.__cpp__ = lambda self: getattr(self, 'n_cpp', self.name)
IfElseOp.__cpp__ =  lambda self: '(%s)?(%s):(%s)'%(self.cond, self.expr_y, self.expr_n)
#-------------------------------------------------------------------------------
class LocalNamespace:
    def __init__(self, Var=Var): self.Var = Var
    def __getitem__(self, key): return spaceOp[key] if key in spaceOp else self.Var(key)
#-------------------------------------------------------------------------------
convert = lambda expr, Var=Var: eval(parse_bk(expr), {}, LocalNamespace())
#-------------------------------------------------------------------------------
