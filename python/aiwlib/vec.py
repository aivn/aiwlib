# -*- coding: utf-8 -*-
'''Copyright (C) 2016-2017 Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

import struct, math
from swig import *
#-------------------------------------------------------------------------------
#   C++ TYPES TABLE
#-------------------------------------------------------------------------------
#                    C++-type                 T   D  py-type  S     sz  unpack struct tuple    pack to strucr tuple
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
                     int: 'int', long: 'int64_t', float: 'double', complex:'std::complex<double>'
}
for k, v in _cxx_types_table.items():
    if type(k) is str: _cxx_types_table[v[:2]] = k
    #if k in 'int32_t int64_t double std::complex<double>'.split(): _cxx_types_table[v[2]] = k
del k, v
#-------------------------------------------------------------------------------
#   patch swig table
#-------------------------------------------------------------------------------
_vec_types_table, _swig_modules, _vec_swig_type = {}, [], []
def view_swig_modules():
    for stt in _swig_modules: stt.out_table()
_get_D = lambda V: int(V.split('<')[1].split('>')[0].split(',')[0])
_get_T = lambda V: V.split('|')[0].split(',',1)[1].rsplit('>',1)[0].strip() if ',' in V.split('|')[0] \
    else 'float' if 'Vecf' in V else'double' if 'Vec' in V else 'int'

def checkout_swig_types_table(stt):
    if stt in _swig_modules: return 
    _swig_modules.append(stt); patchL = []
    for i in range(stt.size()):
        V = stt.get_item(i) #; print i, V
        if not V: continue
        if V.split()[0] in ('PVec', 'aiw::PVec') and not _vec_swig_type: _vec_swig_type[:] = stt, i  # find self
        elif V.split()[0] in ('Vec<', 'Vecf', 'Ind<', 'aiw::Vec<', 'aiw::Vecf<', 'aiw::Ind<'):
            try: D, T = _get_D(V), _get_T(V)
            except: continue
            _vec_types_table[T,D] = (stt, i) # overload types?
            patchL.append(i)
            # print i, D, repr(T), repr(V)
    for i in patchL: stt.patch(i, *_vec_swig_type)
def checkout_swig_modules():
    stt0 = stt = SwigTypesTable()
    while 1:
        checkout_swig_types_table(stt)
        stt = stt.next()
        if stt==stt0: break
checkout_swig_modules()

def import_hook(*args, **kw_args):
    M = builtins_import(*args, **kw_args)    
    checkout_swig_modules()
    return M
builtins_import, __builtins__['__import__'] = __import__, import_hook
#-------------------------------------------------------------------------------
#   SOME FUNCTIONS
#-------------------------------------------------------------------------------
def _decltype(a, b):
    'take two objects, return two C++ types (as strings)'
    ab, iT = [a, b], (0 if not hasattr(a, 'T') else 1 if not hasattr(b, 'T') else 2)
    if iT<2 and type(ab[iT]) in (tuple, list):
        L = [ _cxx_types_table[_cxx_types_table[type(x)]] for x in ab[iT] ]
        ab[iT] = _cxx_types_table[max(l[0] for l in L), max(l[1] for l in L)]
    elif iT<2: ab[iT] = _cxx_types_table[type(ab[iT])]
    TT = [_cxx_types_table[x if type(x) is str else x.T] for x in ab]
    return _cxx_types_table[max(TT[0][0], TT[1][0]), max(TT[0][1], TT[1][1])]
#-------------------------------------------------------------------------------
_is_vec = lambda X: type(X) in (list, tuple) or hasattr(X, 'T')
def _2sz(X, sz):
    if len(X)==sz: return X._getdata() if hasattr(X, '_getall') else X
    if len(X)==1: return (X[0],)*sz
    raise Exception('incorrect %r size, size=%i expected'%(X, sz))
def _conv(self, other): sz = max(len(self), len(other)); return zip(_2sz(self, sz), _2sz(other, sz))
_2tuple = lambda X: X._getdata() if hasattr(X, '_getall') else tuple(X) if type(X) in (tuple, list) else (X,) #???
#-------------------------------------------------------------------------------
#   class Vec (python implementation)
#-------------------------------------------------------------------------------
class Vec:
    _is_aiwlib_vec = True
    def _T(self): return self.T if hasattr(self, 'T') else _get_T(get_swig_type(self.this))
    def _D(self): return self.D if hasattr(self, 'D') else _get_D(get_swig_type(self.this))
    def __init__(self, *args, **kw_args):
        #print self.__class__.__name__, '*******', args, kw_args, '******'
        self._swig_init()
        if len(args)==1: 
            if isinstance(args[0], Vec) and not 'T' in kw_args: args, kw_args['T'] = args[0], args[0].T
            elif args[0].__class__ in (list, tuple) or isinstance(args[0], Vec): args = args[0]
#            elif type(args[0]) is str: args = eval(args[0]) #???
            else: args = args*kw_args.get('D', 1)
        elif len(args)==0: args = (0.,)*kw_args.get('D', 0)
        self.D, self.T = kw_args.get('D', len(args)), kw_args.get('T', 'double') # разные типы args?
        if len(args)!=self.D: raise Exception('Vec<%i,%s>%r --- incorrect args length %i'%(self.D, self.T, args, len(args)))
        #_vec_types_table.get((self.T, self.D), lambda x:None)(self.this)
        if (self.T, self.D) in _vec_types_table: cxx_m, i = _vec_types_table[self.T,self.D]; cxx_m.set_type(self.this, i)
        #if self._T() in ('int', 'int32_t'): self.__class__ = Ind
        #if self._T()=='float': self.__class__ = Vecf
        # print _vec_types_table, self.T, self.D
        # как то контролировать/узнавать тип вектора на основе get_swig_type(self.this)? Когда может вызываться конструктор?
        self._setdata(args)
        #print '*******', self.D, self.T, self.__repr__(), '******'
    #---------------------------------------------------------------------------
    def __len__(self): return self._D()
    def __str__(self): return ' '.join(map(str, self._getdata()))
    def __repr__(self): return ('Ind(%s)' if self._T() in ('int', 'int32_t') else 'Vecf(%s)' if self._T()=='float'
                                else 'Vec(%s)' if self._T()=='double' else 'Vec(%%s,T=\'%s\')'%self._T()
                                )%','.join(map(repr, self._getdata()))
    #---------------------------------------------------------------------------
    def _getdata(self):
        'return Vec data as list'
        T, C, pyT, let, szT, unpack, pack = _cxx_types_table[self._T()]; D = self._D()
        data = struct.unpack(let*D, pull_vec_data(self, 0, szT*D))
        return [unpack(data[i:i+C]) for i in range(0, D*C, C)]
    def _setdata(self, data):
        'set Vec data'
        if len(data)!=self._D(): raise Exception("incorrect length %r, %i expected"%(data, self._D()))
        T, C, pyT, let, szT, unpack, pack = _cxx_types_table[self._T()]; D = self._D()
        push_vec_data(self, 0, struct.pack(let*D, *sum(map(pack, map(pyT, data)), ())), szT*D)    
    #---------------------------------------------------------------------------
    def __getitem__(self, i):
        D = self._D()
        if type(i) is slice: return map(self._getdata().__getitem__, range(*i.indices(D)))
        if i<0: i+= D
        if i<0 or D<=i: raise IndexError(i)
        T, C, pyT, let, szT, unpack, pack = _cxx_types_table[self._T()]
        return unpack(struct.unpack(let, pull_vec_data(self, szT*i, szT)))
    def __setitem__(self, i, val):
        D = self._D()
        if type(i) is slice:
            data = self._getdata()
            for j in range(*i.indices(D)): data[j] = val[j]
            self._setdata(data)
            return
        if i<0: i+= D
        if i<0 or D<=i: raise IndexError(i)
        T, C, pyT, let, szT, unpack, pack = _cxx_types_table[self._T()]
        push_vec_data(self, i*szT, struct.pack(let, *pack(pyT(val))), szT)    
    #---------------------------------------------------------------------------
    def __getstate__(self):
        T, C, pyT, let, szT, unpack, pack = _cxx_types_table[self._T()]; D = self._D()
        return struct.pack('BBh', T, C, D)+pull_vec_data(self, 0, szT*D)
    def __setstate__(self, state):
        self._swig_init() #???
        sT, sC, self.D = struct.unpack('BBh', state[:4])    
        self.T = _cxx_types_table[sT, sC]
        T, C, pyT, let, szT, unpack, pack = _cxx_types_table[self.T]
        if szT*self.D!=len(state)-4: raise Exception('incorrect state size')
        push_vec_data(self, 0, state[4:], szT*self.D)
        cxx_m, i = _vec_types_table.get((self.T, self.D), (0,0))
        if cxx_m: cxx_m.set_type(self.this, i)
    #---------------------------------------------------------------------------
    def __lt__(self, other): return all(a< b for a, b in _conv(self, other)) 
    def __le__(self, other): return all(a<=b for a, b in _conv(self, other)) 
    def __gt__(self, other): return all(a> b for a, b in _conv(self, other)) 
    def __ge__(self, other): return all(a>=b for a, b in _conv(self, other)) 
    def __eq__(self, other): return all(a==b for a, b in _conv(self, other)) 
    def __ne__(self, other): return any(a!=b for a, b in _conv(self, other)) 
    #---------------------------------------------------------------------------
    # operator *
    def __mul__(a, b):
        if _is_vec(b): return sum(x*y for x, y in _conv(a, b))
        return Vec(*[x*b for x in a._getdata()], T=_decltype(a, b))
    def __rmul__(a, b):
        if _is_vec(b): return sum(y*x for x, y in _conv(a, b))
        return Vec(*[b*x for x in a._getdata()], T=_decltype(a, b))
    def __imul__(a, b):
        if _is_vec(b): raise Exception('incorrect second argument in %r *= %r'%(a, b))
        for i in range(len(a)): a[i] = a[i]*b
    #---------------------------------------------------------------------------
    # operator /
    def __div__ (a, b): return Vec(*([x/y for x, y in _conv(a, b)] if _is_vec(b) else
                                     [x/b for x in a._getdata()]), T=_decltype(a, b)) 
    def __rdiv__(a, b): return Vec(*([y/x for x, y in _conv(a, b)] if _is_vec(b) else
                                     [b/x for x in a._getdata()]), T=_decltype(a, b)) 
    def __imul__(a, b):
        if _is_vec(b): raise Exception('incorrect second argument in %r /= %r'%(a, b))
        for i in range(len(a)): a[i] = a[i]/b
    #---------------------------------------------------------------------------
    # operator &
    def __and__(a, b):  return Vec(*([x*y for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __rand__(a, b): return Vec(*([y*x for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __iand__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = a[i]*b[i]
    #---------------------------------------------------------------------------
    # operator +
    def __add__(a, b):  return Vec(*([x+y for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __radd__(a, b): return Vec(*([y+x for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __iadd__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = a[i]+b[i]
    #---------------------------------------------------------------------------
    # operator -
    def __neg__(self): return Vec(*([-x for x in self._getdata()]), T=self._T()) 
    def __sub__(a, b): return  Vec(*([x-y for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __rsub__(a, b): return Vec(*([y-x for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __isub__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = a[i]-b[i]
    #---------------------------------------------------------------------------
    # operators | and ()
    def __or__  (a, b): return Vec(*(a._getdata()+_2tuple(b)), T=_decltype(a, b)) 
    def __ror__ (a, b): return Vec(*(_2tuple(b)+a._getdata()), T=_decltype(a, b))
    def __call__(self, *args): 
        return Vec(*[self[i] for i in (args[0] if len(args)==1 and (args[0].__class__ in (list, tuple) or 
                                                                    (isinstance(args[0], Vec) and args[0].T=='int')) 
                                       else args)], T=self._T())
    #---------------------------------------------------------------------------
    # operators <<, <<=, >>, >>=
    def __lshift__ (a, b): return Vec(*([min(x,y) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __rlshift__(a, b): return Vec(*([min(y,x) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __rshift__ (a, b): return Vec(*([max(x,y) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __rrshift__(a, b): return Vec(*([max(y,x) for x, y in _conv(a, b)]), T=_decltype(a, b)) 
    def __ilshift__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = min(a[i], b[i])
    def __irshift__(a, b):
        b = _2sz(b, len(a))
        for i in range(len(a)): a[i] = max(a[i], b[i])
    #---------------------------------------------------------------------------
    # periodic ???
    def circ(self): return Vec(*(self._getdata()[l%self._D():]+self._getdata()[:l%self._D()]), T=self._T()) #???
    def abs(self): return sum([x*x for x in self._getdata()])**.5
    def pow(self, p): Vec(*[x**p for x in self._getdata()])
    def fabs(self): return Vec(*map(abs, self._getdata()))
    def ceil(self): return Vec(*map(math.ceil, self._getdata()), T=self._T())
    def floor(self): return Vec(*map(math.floor, self._getdata()), T=self._T())
#    def round(self): return Vec(*[math.round(x) for x in self._getdata()], T=self._T()) ???
    def fmod(self, b): return Vec(*([math.fmod(x, y) for x, y in zip(self._getdata(), b)] 
                                    if (b.__class__ in (list, tuple) or isinstance(b, Vec)) and len(b)==self._D()
                                    else [math.fmod(x, b) for x in self._getdata()]), T=_decltype(self, b))
    def min(self): return min(self._getdata())
    def max(self): return max(self._getdata())
    def imin(self): return min(zip(self._getdata(), range(self._D())))[1]
    def imax(self): return max(zip(self._getdata(), range(self._D())))[1]
    def sum(self): return sum(self._getdata())
    def nan(self): return Ind(*map(math.isnan, self._getdata()))
    def inf(self): return Ind(*map(math.isinf, self._getdata()))
    def cknan(self): return any(map(math.isnan, self._getdata()))
    def ckinf(self): return any(map(math.isinf, self._getdata()))
    def prod(self): return reduce(lambda a, b: a*b, self.__getdata())
    def __nonzero__(self): return all(self._getdata())    
    def __hash__(self): return hash(tuple(self._getdata()))
    def contains(self, x): return x in tuple(self._getdata())
    def __contains__(self, x): return x in tuple(self._getdata())
    def reverse(self): L = self._getdata(); L.reverse(); return Vec(*L, T=self._T())
    def sort(self): L = self._getdata(); L.sort(); return Vec(*L, T=self._T())
    #---------------------------------------------------------------------------
    def __mod__(a, b):
        ab = _conv(a, b)
        if len(ab)==2: return ab[0][0]*ab[1][1]-ab[1][0]*ab[0][1]
        elif len(ab)==3: return Vec(*[ab[1][0]*ab[2][1]-ab[2][1]*ab[1][1], 
                                      ab[2][0]*ab[0][1]-ab[0][1]*ab[2][1], 
                                      ab[0][0]*ab[1][1]-ab[1][1]*ab[0][1]], T=_decltype(a, b))
        else: raise Exception('incorrect arguments in vectors product %r%%%r'%(a, b))
    #---------------------------------------------------------------------------
    def __rmod__(a, b):
        if type(b) in (list, tuple): return -(a%b)
        if a._T() not in ('int', 'int32_t'): raise Exception('incorrect rvalue type in %r%%%r'%(x, self))
        r = Ind(D=a._D())
        for i in range(a._D()): r[i] = b%a[i]; b /= a[i]
        return r
    #---------------------------------------------------------------------------
    def __ixor__(self, Up):
        if self._T() not in ('int', 'int32_t'): raise Exception('incorrect lvalue type in %r^=%r'%(self, Up))
        for i in range(self._D()-1):
            if self[i]<Up[i]: return True 
            else: self[i] = 0; self[i+1] += 1 
        return self[self._D()-1]<Up[D-1]
    #---------------------------------------------------------------------------
    def __sizeof__(self): return self.D*_cxx_types_table[self.T][4]
    #def __del__(self): destroy_swig_object(self.this)
    def __del__(self): _vec_swig_type[0].set_type(self.this, _vec_swig_type[1])
#-------------------------------------------------------------------------------
def angle(a, b, c):    
    ab, bc = b-a, c-b; ab /= ab.abs(); bc /= bc.abs()
    return math.acos(ab*bc)
#-------------------------------------------------------------------------------
#   FINAL ACTIONS
#-------------------------------------------------------------------------------
#del PVec.__swig_destroy__ # ugly workaround!!!
PVec._swig_init = PVec.__init__
for k, v in Vec.__dict__.items():
    if k not in ('__doc__',): setattr(PVec, k, v)
PVec.__name__, Vec = 'Vec', PVec; del PVec

vec = lambda *args, **kw_args: Vec(*args, T=_cxx_types_table[type(args[0])], **kw_args)

class Ind(Vec):
    def __init__(self, *args, **kw_args): kw_args['T'] = 'int'; Vec.__init__(self, *args, **kw_args)
ind = lambda *args, **kw_args: Ind(*args, **kw_args)

class Vecf(Vec):
    def __init__(self, *args, **kw_args): kw_args['T'] = 'float'; Vec.__init__(self, *args, **kw_args)
vecf = lambda *args, **kw_args: Vecf(*args, **kw_args)

__all__ = ['Vec', 'vec', 'Ind', 'ind', 'Vecf', 'vecf', 'angle']
#-------------------------------------------------------------------------------
#add_swig_types_table(SwigTypesTable())
#print '======================================================'
