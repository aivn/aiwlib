# -*- coding: utf-8 -*-
# Copyright (C) 2016 Antov V. Ivanov, KIAM RAS, Moscow.
# This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)

import struct
from swig import *
#-------------------------------------------------------------------------------
#   C++ TYPES TABLE
#-------------------------------------------------------------------------------
#                    C++ type                 T  D py-type  S     sz  unpack struct tuple    pack to strucr tuple
_cxx_types_table = { 'char':                 (10, 1, int,     'c',  1,  lambda t:t[0],         lambda x:(x,)            ),
                     'unsigned char':        (20, 1, int,     'B',  1,  lambda t:t[0],         lambda x:(x,)            ),
                     'int8_t':               (10, 1, int,     'b',  1,  lambda t:t[0],         lambda x:(x,)            ),
                     'uint8_t':              (20, 1, int,     'B',  1,  lambda t:t[0],         lambda x:(x,)            ),
                     'short':                (30, 1, int,     'h',  2,  lambda t:t[0],         lambda x:(x,)            ),
                     'int16_t':              (30, 1, int,     'h',  2,  lambda t:t[0],         lambda x:(x,)            ),
                     'unsigned short':       (40, 1, int,     'H',  2,  lambda t:t[0],         lambda x:(x,)            ),
                     'uint16_t':             (40, 1, int,     'H',  2,  lambda t:t[0],         lambda x:(x,)            ),
                     'int':                  (50, 1, int,     'i',  4,  lambda t:t[0],         lambda x:(x,)            ),
                     'int32_t':              (50, 1, int,     'i',  4,  lambda t:t[0],         lambda x:(x,)            ),
                     'usigned int':          (60, 1, long,    'I',  4,  lambda t:t[0],         lambda x:(x,)            ),
                     'uint32_t':             (60, 1, long,    'I',  4,  lambda t:t[0],         lambda x:(x,)            ),
                     'long':                 (70, 1, long,    'l',  8,  lambda t:t[0],         lambda x:(x,)            ),
                     'int64_t':              (70, 1, long,    'l',  8,  lambda t:t[0],         lambda x:(x,)            ),
                     'unsigned long':        (80, 1, long,    'L',  8,  lambda t:t[0],         lambda x:(x,)            ),
                     'uint64_t':             (80, 1, long,    'L',  8,  lambda t:t[0],         lambda x:(x,)            ),
                     'float':                (90, 1, float,   'f',  4,  lambda t:t[0],         lambda x:(x,)            ),
                     'double':               (95, 1, float,   'd',  8,  lambda t:t[0],         lambda x:(x,)            ),
                     'std::complex<float>':  (90, 2, complex, 'ff', 8,  lambda t:t[0]+t[1]*1j, lambda x:(x.real, x.imag)),
                     'std::complex<double>': (95, 2, complex, 'dd', 16, lambda t:t[0]+t[1]*1j, lambda x:(x.real, x.imag)),
                     int: 'int8_t', long: 'int64_t', float: 'float', complex:'std::complex<float>'
                 }
for k, v in _cxx_types_table.items():
    if type(k) is str: _cxx_types_table[v[:2]] = k
    #if k in 'int32_t int64_t double std::complex<double>'.split(): _cxx_types_table[v[2]] = k
del k, v
#-------------------------------------------------------------------------------
#   patch swig table
#-------------------------------------------------------------------------------
_vec_types_table = {}
def add_swig_types_table(cxx_m):
    patchL = []
    for i in range(cxx_m.size()):
        V = cxx_m.get_item(i)
        if V.split()[0]=='PVec': _PVec._swig_type = i # find self
        elif V.split()[0] in ('Vec<', 'Ind<', 'aiw::Vec<', 'aiw::Ind<'):
            D = int(V.split('<')[0].split('>')[0].split(',')[0])
            T = V.split(',',1)[1].rsplit('>',1)[0].strip()
            if not T: T = 'double' if 'Vec' in V else 'int'        
            # _vec_types_table[T,D] = (cxx_m, i) # overload types?
            _vec_types_table[T,D] = lambda self, cxx_m=cxx_m, i=i: cxx_m.set_type(cxx_m, i) # overload types?
            patchL.append(i)
    for i in patchL: cxx_m.patch(i, _PVec._swig_type)
add_swig_types_table(SwigTypesTable())
#-------------------------------------------------------------------------------
#   SOME FUNCTIONS
#-------------------------------------------------------------------------------
def _decltype(a, b):
    'take two objects, return two C++ types (as strings)'
    ab, iT = [a, b], (0 if not hasattr(a, 'T') else 1 if not hasattr(b, 'T') else 2)
    if iT<2 and type(ab[iT]) in (tuple, list):
        L = [ _cxx_types_table[_cxx_types_table[type(x)]] for x in ab[iT] ]
        ab[iT] = _cxx_types_table[max(l[0] for l in L), max(l[1] for l in L)]
    elif iT<2: ab[iT] = _cxx_types_table[type(x)]
    TT = [_cxx_types_table[x.T] for x in ab]
    return _cxx_types_table[max(TT[0][0], TT[1][0]), max(TT[0][1], TT[1][1])]
#-------------------------------------------------------------------------------
_is_vec = lambda X: type(X) in (list, tuple) or hasattr(X,'T')
def __2sz(X, sz):
    if len(X)==sz: return X._getall() if hasattr(X, '_getall') else X
    if len(X)==1: return (X[0],)*sz
    raise Exception('incorrect %r size, size=%i expected'%(X, sz))
def _conv(self, other): sz = max(len(self), len(other)); return zip(_2sz(self, sz), _2sz(other, sz))
_2tuple = lambda X: X._getall() if hasattr(X, '_getall') else tuple(X) if type(X) in (tuple, list) else (X,) #???
#-------------------------------------------------------------------------------
#   class Vec (python implementation)
#-------------------------------------------------------------------------------
_PVec = PVec; del PVec
class Vec(_PVec):
    def __init__(self,*args, **kw_args):
        _PVec.__init__(self)
        self.D, self.T = kw_args.get('D', len(args)), kw_args.get('T', 'double') # разные типы args?
        if len(args)==1: args = args*self.D
        elif len(args)==0: args = (0.,)*self.D
        if len(args)!=self.D: raise Exception('Vec<%i,%s>%r --- incorrect args length %i'%(self.D, self.T, args, len(args)))
        _vec_types_table.get((self.T, self.D), lambda x:None)(self.this)
        # как то контролировать/узнавать тип вектора на основе get_swig_type(self.this)? Когда может вызываться конструктор?
        self._setdata(args)
    #---------------------------------------------------------------------------
    __len__  = lambda self: self.D
    __str__  = lambda self: ' '.join(map(str, self._getall()))
    __repr__ = lambda self: ('Ind(%s)' if self.T=='int' else 'Vec(%s)' if self.T=='double'
                              else 'Vec(%%s,D=\'%s\')'%self.T)%','.join(map(repr, self._getall()))
    #---------------------------------------------------------------------------
    def _getdata(self):
        'return Vec data as list'
        T, C, pyT, let, szT, pack, unpack = _cxx_types_table[self.T]
        data = struct.unpack(let*self.D, pull_vec_data(self, 0, szT*self.D))
        return [unpack(data[i:i+C]) for i in range(0, self.D, C)]
    def _setdata(self, data):
        'set Vec data'
        if len(data)!=self.D: raise Exception("incorrect length %r, %i expected"%(data, self.D))
        T, C, pyT, let, szT, pack, unpack = _cxx_types_table[self.T]
        push_vec_data(self, 0, struct.pack(let*self.D, sum(pack, map(pyT, data), ())), szT*self.D)    
    #---------------------------------------------------------------------------
    def __getitem__(self, i):
        if type(i) is slice: return map(self._getdata().__getitem__, range(*i.indices(self.D)))
        if i<0: i+= self.D
        if i<0 or self.D<=i: raise IndexError(i)
        T, C, pyT, let, szT, pack, unpack = _cxx_types_table[self.T]
        return unpack(struct.unpack(let, pull_vec_data(self, szT*i, szT)))
    def __setitem__(self, i, val):
        if type(i) is slice:
            data = self._getdata()
            for j in range(*i.indices(self.D)): data[j] = val[j]
            self._setdata(data)
            return
        if i<0: i+= self.D
        if i<0 or self.D<=i: raise IndexError(i)
        T, C, pyT, let, szT, pack, unpack = _cxx_types_table[self.T]
        push_vec_data(self, i*szT, struct.pack(let, pack(pyT(val))), szT)    
    #---------------------------------------------------------------------------
    def __getstate__(self):
        T, C, pyT, let, szT, pack, unpack = _cxx_types_table[self.T]
        return struct.pack('BBh', T, C, self.D)+pull_vec_data(self, 0, szT*self.D)
    def __setstate__(self, state):
        _PVec.__init__(self) #???
        sT, sC, self.D = struct.unpack('BBh', state[:4])    
        self.T = _cxx_types_table[sT, sC]
        T, C, pyT, let, szT, pack, unpack = _cxx_types_table[self.T]
        if szT*self.D!=len(state)-4: raise Exception('incorrect state size')
        push_vec_data(self, 0, state[4:], szT*self.D)
        _vec_types_table.get((self.T, self.D), lambda x:None)(self.this)  #???
    #---------------------------------------------------------------------------
    __lt__ = lambda self, other: all(a< b for a, b in _conv(self, other)) 
    __le__ = lambda self, other: all(a<=b for a, b in _conv(self, other)) 
    __gt__ = lambda self, other: all(a> b for a, b in _conv(self, other)) 
    __ge__ = lambda self, other: all(a>=b for a, b in _conv(self, other)) 
    __eq__ = lambda self, other: all(a==b for a, b in _conv(self, other)) 
    __ne__ = lambda self, other: any(a!=b for a, b in _conv(self, other)) 
    #---------------------------------------------------------------------------
    # operator *
    def __mul__(a, b):
        if _is_vec(b): return sum(x*y for x, y in _conv(a, b))
        return Vec(*[x*b for x in a._getall()], T=_decltype(a, b))
    def __rmul__(a, b):
        if _is_vec(b): return sum(y*x for x, y in _conv(a, b))
        return Vec(*[b*x for x in a._getall()], T=_decltype(a, b))
    def __imul__(a, b):
        if _is_vec(b): raise Exception('incorrect second argument in %r *= %r'%(a, b))
        for i in range(len(a)): a[i] = a[i]*b
    #---------------------------------------------------------------------------
    # operator /
    __div__  = lambda a, b: Vec(*([x/y for x, y in _conv(a, b)] if _is_vec(b) else [x/b for x in a._getall()]), T=_decltype(a, b)) 
    __rdiv__ = lambda a, b: Vec(*([y/x for x, y in _conv(a, b)] if _is_vec(b) else [b/x for x in a._getall()]), T=_decltype(a, b)) 
    def __imul__(a, b):
        if _is_vec(b): raise Exception('incorrect second argument in %r /= %r'%(a, b))
        for i in range(len(a)): a[i] = a[i]/b
    #---------------------------------------------------------------------------
    # operator &
    __and__  =  lambda a, b: Vec(*([x*y for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    __rand__ =  lambda a, b: Vec(*([y*x for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __iand__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = a[i]*b[i]
    #---------------------------------------------------------------------------
    # operator +
    __add__   = lambda a, b: Vec(*([x+y for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    __radd__  = lambda a, b: Vec(*([y+x for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __iadd__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = a[i]+b[i]
    #---------------------------------------------------------------------------
    # operator -
    __sub__   = lambda a, b: Vec(*([x-y for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    __rsub__  = lambda a, b: Vec(*([y-x for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __isub__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = a[i]-b[i]
    #---------------------------------------------------------------------------
    # operators | and ()
    __or__   = lambda a, b: Vec(*(a._getall()+_2tuple(b)), T=_decltype(a, b)) 
    __ror__  = lambda a, b: Vec(*(_2tuple(b)+a._getall()), T=_decltype(a, b))
    __call__ = lambda self, *args: PVec(*[self[i] for i in agrs], T=self.T)
    #---------------------------------------------------------------------------
    # operators <<, <<=, >>, >>=
    __lshift__  = lambda a, b: Vec(*([min(x,y) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    __rlshift__ = lambda a, b: Vec(*([min(y,x) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    __rshift__  = lambda a, b: Vec(*([max(x,y) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    __rrshift__ = lambda a, b: Vec(*([max(y,x) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __ilshift__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = min(a[i], b[i])
    def __irshift__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = max(a[i], b[i])
    #---------------------------------------------------------------------------
    # periodic ???
    circ = lambda self, l: Vec(*(self._getall()[l%self.D:]+self._getall()[:l%self.D]), T=self.T) #???
    len = lambda self: sum([x*x for x in self._getall()])**.5
    pow = lambda self, x: PVec(*[x**p for x in self._getall()])
    #mod = lambda self, x: PVec(*[x**p for x in self._getall()])
    fabs = lambda self: PVec(*[abs(x) for x in self._getall()])
    ceil = lambda self: PVec(*[math.ceil(x) for x in self._getall()], T=self.T)
    floor = lambda self: PVec(*[math.floor(x) for x in self._getall()], T=self.T)
    round = lambda self: PVec(*[math.round(x) for x in self._getall()], T=self.T)
    min = lambda self: min(self._getall())
    max = lambda self: max(self._getall())
    sum = lambda self: sum(self._getall())
    nan = lambda self: Ind(*[ math.isnan(x) for x in self._getall()])
    inf = lambda self: Ind(*[ math.isinf(x) for x in self._getall()])
    cnan = lambda self: any([ math.isnan(x) for x in self._getall()])
    cinf = lambda self: any([ math.isinf(x) for x in self._getall()])
    prod = lambda self: reduce(lambda a, b:a*b, self.__getall())
    __nonzero__ = lambda self: all(self._getall())
#-------------------------------------------------------------------------------
def angle(a, b, c):    
    ab, bc = b-a, c-b; ab /= ab.abs(); bc /= bc.abs()
    return math.acos(ab*bc)
#-------------------------------------------------------------------------------
#   FINAL ACTIONS
#-------------------------------------------------------------------------------
vec = lambda *args, **kw_args: Vec(*args, **kw_args)
class Ind(Vec):
    def __init__(self, *args, **kw_args): Vec.__init__(self, *args, T='int', **kwe_args)
    def __rmod__(self, x):
        r = Ind(D=self.D)
        for i in range(self.D): r[i] = x%self[i]; x /= self[i]
        return r
    def __ixor__(self, Up):
        for i in range(self.D-1):
            if self[i]<Up[i]: return True 
            else: self[i] = 0; self[i+1] += 1 
        return self[self.D-1]<Up[D-1]
ind = lambda *args, **kw_args: Ind(*args, **kw_args)
#-------------------------------------------------------------------------------
