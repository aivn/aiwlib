# -*- encoding: utf-8 -*-
'''AST classes for operations

Copyright (C) 2002-2013, 2017 Anton V. Ivanov <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0
'''
#-------------------------------------------------------------------------------
#   BASE
#-------------------------------------------------------------------------------
# приводит объект к строке отбрасывает ведущий ноль при необходимости
_N2P = lambda x: ('%g'%x if abs(x)>=1 or x==0 else ('%g'%x)[1:] if x>0 else '-'+('%g'%x)[2:]) if x.__class__ in (int, float, long) else str(x)
class BaseOp(object):
    priority, nocmm, format = -1, 0, 'pyt' # приоритет, не-коммутативность и формат вывода по умолчанию
    def __str__(self): return getattr(self, '__%s__'%base.format)()
    def __pyt__(self): return self.p_pyt%self._bk('(%s)', _N2P)
    def __txt__(self): return getattr(self, 'p_txt', self.p_pyt )%self._bk('(%s)', _N2P)
#-------------------------------------------------------------------------------
#   CONTAINERS
#-------------------------------------------------------------------------------
class UnaryOp(BaseOp):
    def __init__(self, a): self.a = a
    def __repr__(self): return '%s(%r)'%(self.__class__.__name__, self.a)
    def _bk(self, patt, f=lambda x: x): return f(self.a) if getattr(self.a, 'priority', 0)<=self.priority else patt%f(self.a)
    def _get_vars(self): return getattr(self.a, '_get_vars', lambda:[])()
    def __call__(self, *args, **kw_args): return self.__class__(self.a(*args, **kw_args) if isinstance(self.a, BaseOp) else self.a)
#-------------------------------------------------------------------------------
class BinaryOp(BaseOp):
    def __init__(self, a, b): self.a, self.b = a, b
    def __repr__(self): return '%s(%r, %r)'%(self.__class__.__name__, self.a, self.b)
    def _bk(self, patt, f=lambda x: x): return ( f(self.a) if getattr(self.a, 'priority', 0)<=self.priority else patt%f(self.a), 
                                                 f(self.b) if getattr(self.b, 'priority', 0)<=self.priority-self.nocmm else patt%f(self.b) )
    def _get_vars(self): return getattr(self.a, '_get_vars', lambda: [])()+getattr(self.b, '_get_vars', lambda: [])()
    def __call__(self, *args, **kw_args): return self.__class__(*[x(*args, **kw_args) if isinstance(x, BaseOp) else x for x in (self.a, self.b)])
#-------------------------------------------------------------------------------
#   OPERATIONS
#-------------------------------------------------------------------------------
_op_list = [( p[0] if p[1].isalpha() else p[:2],
              p[1+(not p[1].isalpha()):-3],
              (UnaryOp, BinaryOp)[p[-3]==':'],
              int(p[-2]), p[-1]=='n' ) for p in 
            '''**pow:1n -neg.2c +pos.2c ~inv.2c *mul:3c /div:3n //floordiv:3n %mod:3n +add:4c -sub:4n 
            <<lshift:5n >>rshift:5n &and:6c |or:7c ^xor:8c ==eq:9c !=ne:9c <lt:9n <=le:9n >gt:9n >=ge:9n'''.split()] #[(op, name, Unary|Binary, priority, nocmm)]
for op, name, base, priority, nocmm in _op_list:
    cname = name.capitalize()+'Op'
    cobj = type(cname, (base,), {'priority':priority, 'nocmm':nocmm, 'p_pyt':'%s'*(base is BinaryOp)+op+'%s'}) 
    globals()[cname] = cobj
    if base is BinaryOp:
        setattr(BaseOp, '__%s__'%name,  lambda a, b, cobj=cobj: cobj(a, b))
        setattr(BaseOp, '__r%s__'%name, lambda a, b, cobj=cobj: cobj(b, a))
    else: setattr(BaseOp, '__%s__'%name, lambda self, cobj=cobj: cobj(self))
class BoolAndOp(BinaryOp): priority, nocmm, p_pyt = 11, 0, '%s and %s'
class BoolOrOp(BinaryOp):  priority, nocmm, p_pyt = 11, 0, '%s or %s'
class Not(UnaryOp): priority, nocmm, p_pyt = 10, 0, 'not %s'
BaseOp.And, BaseOp.Or = lambda a, b: BoolAndOp(a, b), lambda a, b: BoolOrOp(a, b) #???
#-------------------------------------------------------------------------------
#   FUNCS
#-------------------------------------------------------------------------------
spaceOp = dict([ (n, type(n, (UnaryOp,), {'prioity':0, 'p_pyt':n+'(%s)', 'p_txt':n+'%s',
                                          '_bk':(lambda self, patt, f=(lambda x: x): f(self.a))})) for n in 
                 'acos acosh asin asinh atan atanh cos cosh exp fabs floor log log10 sin sinh sqrt tan tanh'.split() ])
brakets = {'()': 'crc_bk', '{}':'fig_bk', '<>':'ang_bk', '[]':'rm_bk'}
for bk, n in brakets.items(): 
    spaceOp[n] = type(n, (UnaryOp,), {'prioity':0, 'p_pyt':n+'(%s)', 'p_txt':'%s'.join(bk), '_bk':(lambda self, patt, f=(lambda x: x): f(self.a))})
spaceOp['fabs'].p_txt, spaceOp['Not'] = '|%s|', Not
#-------------------------------------------------------------------------------
#  VARIABLE
#-------------------------------------------------------------------------------
class Var(BaseOp):
    def __init__(self, name): self.name = name
    def __pyt__(self): return self.name
    def __txt__(self): return self.name
    def __repr__(self): return 'Var(%r)'%self.name
    def __call__(self, *args, **kw_args): return Var(kw_args.get(self.name,self.name))
#-------------------------------------------------------------------------------
