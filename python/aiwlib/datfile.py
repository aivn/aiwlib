#!/usr/bin/python
# -*- coding: utf-8 -*-
import os, sys, gzip, math
#-------------------------------------------------------------------------------
#   WRITE DATFILE
#-------------------------------------------------------------------------------
class DatFile:
    def __init__(self, name, mode='w', gz=False, autoflush=True): # maxtime, autoprogress?
        self.name, self.autoflush, path, self._wh = name, autoflush, os.path.dirname(name), False
        if path and not os.path.exists(path): os.makedirs(path)
        self._stream, self.header, self._exprL, self._options = (gzip.open if gz else open)(self.name, mode), [], [], {}
    def flush(self): self._stream.flush()
    def __call__(self, key, expr=None, **kw_args):
        self.header.append(key); self._options[key] = kw_args
        if expr: print>>self._stream, '# %s = %s'%(key, expr)
        else: expr = key
        self._exprL.append(compile(expr, expr, 'eval'))
        return self
    def out(self): 
        if not self._wh:
            print>>self._stream, '\n'.join([
                    '#:'+'; '.join(['%s.%s=%r'%(key, k, o) for k, o in self._options[key].items()]) 
                    for key in self.header if self._options[key]])
            print>>self._stream, '#:', ' '.join(self.header)
            self._wh = True
        try: raise Exception('call for get last stack frame')
        except: G, L = sys.exc_info()[2].tb_frame.f_back.f_globals, sys.exc_info()[2].tb_frame.f_back.f_locals
        print>>self._stream, ' '.join([str(eval(e, G, L)) for e in self._exprL])
        if self.autoflush: self._stream.flush()
#-----------------------------------------------------------------------------------------------------------
#   READ DATFILE
#-----------------------------------------------------------------------------------------------------------
def _read_header(fin):
    D, H, s = {}, { 0:'x', 1:'y', 2:'z' }, fin.readline()
    while s and ( s.startswith('#') or not s.strip() ):
        if s.startswith('#:') :
            if '=' in s:
                try: exec( s[2:].strip(), G, D )
                except: pass
            else: l = s[2:].strip().split(); H.update([ ( i, l[i] ) for i in range(len(l)) ]) 
        s = fin.readline()
    return D, list([ H[i] for i in sorted(H.keys()) ]), s
#-----------------------------------------------------------------------------------------------------------
class datAccess:
    def __init__( self, path ): self.path = path
    def __nonzero__(self): return True
    def __str__(self): return self.path
    def __getattr__( self, attr ):
        if attr in ('head','params'): 
            self.params, self.head, s = _read_header(( gzip.open if self.path.endswith('.gz') else open )(self.path)) 
            return self.__dict__[attr]
        if attr=='data' :
            fin = ( gzip.open if self.path.endswith('.gz') else open )(self.path)
            self.params, self.head, s = _read_header(fin)
            self.data = [ map( float, l.split() ) for l in [s]+fin.readlines() ]
            return self.data            
        raise AttributeError(attr)
    def __getitem__( self, key ): return self.data[key[1]][key[0]] if type(key) is tuple else self.data[key]
#-----------------------------------------------------------------------------------------------------------
class _datGroup:
    def __init__( self, start, names ): self.start, self.names = start, names
    def __nonzero__(self): return True
    def __str__(self): return ' '.join([ self.start+i for i in self.names ])
    def __getattr__( self, attr ): return self[attr]
    def __getitem__( self, key ):
        if type(key) is int : return datAccess( self.start, self.names[key] ) 
        names = filter( lambda n : n.startswith(key) and ( n==key or n[len(key)]=='.' ), self.names )
        if len(names)==1 : return datAccess( self.start+names[0] )
        if len(names)>1 : return _datGroup( self.start+key+'.', names )
        raise KeyError(key)        
#-----------------------------------------------------------------------------------------------------------
is_group = lambda name, self : filter( lambda n: n.startswith(name) and ( n==name or n[len(name)]=='.' ), os.listdir(self.path) )
def mk_group( name, self ):
    L = is_group( name, self )
    if len(L)==1 : return _datGroup(self.path+L[0]+'/', os.listdir(self.path+L[0])) if os.path.isdir(self.path+L[0]) else datAccess(self.path+L[0]) 
    return _datGroup( self.path, L )
#-----------------------------------------------------------------------------------------------------------
