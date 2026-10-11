'''Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

import struct, math

bytecode = lambda x: x.bytecode() if isinstance(x, BaseOp) else chr(64).encode()+struct.pack('f', float(x))
#-----------------------------------------------------------------------------
class BaseOp:
    def __pos__(self): return self
    def __neg__(self): return NegOp(self)
    def __add__(self, other): return AddOp(self, other)
    def __sub__(self, other): return SubOp(self, other)
    def __mul__(self, other): return MulOp(self, other)
    def __truediv__(self, other): return DivOp(self, other)
    def __pow__(self, other):
        if other==2: return Pow2Op(self)
        #if type(other) is int and other>0: return IPowOp(self, other)
        return PowOp(self, other)
    def __radd__(self, other): return AddOp(other, self)
    def __rsub__(self, other): return SubOp(other, self)
    def __rmul__(self, other): return MulOp(other, self)
    def __rtruediv__(self, other): return DivOp(other, self)
    def __rpow__(self, other): return PowOp(other, self)
    # def abs(self): pass и др операции над векторами?
#-----------------------------------------------------------------------------
class UnaryOp(BaseOp):
    def __init__(self, a): self.a = a
    def __str__(self): return self.op%self.a
    def bytecode(self): return bytecode(self.a)+chr(self.code).encode()
    
class NegOp(UnaryOp): op, code = '-(%s)', 20    
class Pow2Op(UnaryOp): op, code = '(%s)**2', 21

class FuncSqrt(UnaryOp):  op, code = 'sqrt(%s)',  22    
class FuncFabs(UnaryOp):  op, code = 'fabs(%s)',  23    
class FuncExp(UnaryOp):   op, code = 'exp(%s)',   24    
class FuncLog(UnaryOp):   op, code = 'log(%s)',   25    
class FuncSin(UnaryOp):   op, code = 'sin(%s)',   26    
class FuncCos(UnaryOp):   op, code = 'cos(%s)',   27    
class FuncTan(UnaryOp):   op, code = 'tan(%s)',   28    
class FuncSinh(UnaryOp):  op, code = 'sinh(%s)',  29    
class FuncCosh(UnaryOp):  op, code = 'cosh(%s)',  30    
class FuncTanh(UnaryOp):  op, code = 'tanh(%s)',  31    
class FuncAsin(UnaryOp):  op, code = 'asin(%s)',  32    
class FuncAsinh(UnaryOp): op, code = 'asinh(%s)', 33    
class FuncAcos(UnaryOp):  op, code = 'acos(%s)',  34    
class FuncAcosh(UnaryOp): op, code = 'acosh(%s)', 35    
class FuncAtan(UnaryOp):  op, code = 'atan(%s)',  36    
class FuncAtanh(UnaryOp): op, code = 'atanh(%s)', 37    
class FuncLog10(UnaryOp): op, code = 'log10(%s)', 38    
#-----------------------------------------------------------------------------
class BinaryOp(BaseOp):
    def __init__(self, a, b): self.a, self.b = a, b
    def __str__(self): return self.op%(self.a, self.b)
    def bytecode(self): return bytecode(self.a)+bytecode(self.b)+chr(self.code).encode()

class AddOp(BinaryOp): op, code = '(%s)+(%s)', 50
class SubOp(BinaryOp): op, code = '(%s)-(%s)', 51
class MulOp(BinaryOp): op, code = '(%s)*(%s)', 52
class DivOp(BinaryOp): op, code = '(%s)/(%s)', 53
class PowOp(BinaryOp): op, code = '(%s)**(%s)', 54
class FuncAtan2(BinaryOp): op, code = 'atan2(%s,%s)', 55
#class IPowOp(BinaryOp): op, code = '**', 35 #???
#-----------------------------------------------------------------------------
vars_table = {}
class Var(BaseOp):
    def __init__(self, name, cursor): vars_table[id(self)] = (name, cursor)
    def __del__(self): del vars_table[id(self)]
    def __str__(self): return vars_table[id(self)][0]
    def __getattr__(self, attr): name, cursor = vars_table[id(self)]; return Var(name+'.'+attr, cursor.get_attr(attr))
    def __getitem__(self, index):
        name, cursor = vars_table[id(self)]
        return Var(name+'[%s]'%(str(index)[1:-1] if type(index) in (list, tuple) else index), cursor.get_item(index))
    def bytecode(self): return vars_table[id(self)][1].bytecode()
#-----------------------------------------------------------------------------
def Eval(expr, cell):
    ns = dict(['pi': math.pi, 'e': math.e]+[(name, Var(name, cursor)) for name, cursor in cell.namespace()]
              +[(k[4:].lower(), v) for k, v in globals().items() if k.startswith('Func')])
    return bytecode(eval(expr, {}, ns))
#-----------------------------------------------------------------------------
