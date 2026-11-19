# -*- coding: utf-8 -*-
'''Copyright (C) 2013, 2020 Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''
#---------------------------------------------------------------------------------------------------------
import time as _time
import sys; sys.modules['racslib.mytime'] = sys.modules[__name__]

#def _import_hook(*args, **kw_args):
#    'patch for change racslib.mytime module to unpickling old data'
#    if args[0]=='racslib.mytime': import sys; return sys.modules[__name__]
#    return _builtins_import(*args, **kw_args)    
#_builtins_import, __builtins__['__import__'] = __import__, _import_hook
#---------------------------------------------------------------------------------------------------------
class BaseTimeException(Exception): pass
class IllegalInitTimeValue(BaseTimeException): pass
class IllegalInitDateValue(BaseTimeException): pass
class IllegaMuleTimeValue(BaseTimeException): pass

try: long(1)
except: long = int
#---------------------------------------------------------------------------------------------------------
class Time(object): 
#    format = ...
    precision = 3
    def __init__(self, val=0, **d):
        if d and val: raise IllegalInitTimeValue('what use, "val=%r" or "%s"?'%(val, ', '.join(['%s=%r'%(k,v) for k, v in d.items()])))
        if d: self.h, self.m, self.s = int(d.get('h',0)), int(d.get('m',0)), float(d.get('s',0)); self.setval()
        elif type(val) in (int, float, long): self.val = float(val)
        elif isinstance(val, Time): self.val = val.val
        elif type(val)==str:
            pm, val = (-1, val[1:]) if val.startswith('-') else (1, val)
            try:
                if not ':' in val:  self.val = pm*float(val) #secs
                elif val.count(':')==1: h, m = map(float,val.split(':')); self.val = pm*(3600*h+60*m)
                elif val.count(':')==2: h, m, s = map(float,val.split(':')); self.val = pm*(3600*h+60*m+s)
                else: raise IllegalInitTimeValue(val)
            except ValueError as e: raise IllegalInitTimeValue(val)
        else:  raise IllegalInitTimeValue(val)
        self.h, self.m, self.s = int(abs(self.val)/3600), int(abs(self.val)/60)%60, self.val%60
    def __getattr__(self, attr): return 0
    def setval(self): self.val = self.h*3600+self.m*60+self.s
    def __call__(self, **d): dd = dict(self.__dict__); del dd['val']; dd.update(d); return Time(**dd)
    def __float__(self): return self.val
    def __int__(self):   return int(self.val)
    def __getstate__(self): return self.val
    def __setstate__(self, val):  self.__init__(val)
    def __str__(self):
        if not hasattr(self, 's'): return '---'
        if not self.s: return '-'*(self.val<0)+'%i:%02i'%(self.h, self.m)
        if ('%0*.*f'%(2+bool(self.precision)+self.precision, self.precision, self.s)).startswith('60'):
            return '-'*(self.val<0)+'%i:%02i'%(self.h, self.m+1)
        return '-'*(self.val<0) + "%i:%02i:%0*.*f"%(self.h, self.m, 2+bool(self.precision)+self.precision, self.precision, self.s)
    def __repr__(self): return "Time('%s')"%self
    def __neg__(self): return Time(-self.val)
    def __pos__(self): return Time(self.val)
    def __abs__(self): return Time(abs(self.val))
    def __add__(self, other):
        try:
            if isinstance(other, Time): return Time(self.val+other.val)
            elif isinstance(other, Date): return Date(self.val+other.val)
            try: return self+Time(other)
            except IllegalInitTimeValue as e: return self+Date(other)
        except: return self 
    def __sub__(self, other): return self+(-other)
    def __radd__(self, other): return self+other
    def __rsub__(self, other): return -self+other
    def __mul__(self, other): 
        if type(other) in (int, float, long, str): return Time(self.val*float(other))
        raise IllegaMuleTimeValue(other)
    def __div__(self, other):        
        if type(other) is str and ':' in other: return self.val/Time(other).val
        elif type(other) in (int, float, long, str): return Time(self.val/float(other))
        elif isinstance(other, Time): return self.val/other.val
        raise IllegaMuleTimeValue(other)
    def __mod__(self, other): return int(self.val)%other #???
    def __rmul__(self, other): return self*other
    def __cmp__(self, other):
        try: return cmp(self.val, Time(other).val) #precision???
        except: return False
    def __rcmp__(self, other): return cmp(Time(other).val, self.val2) #precision???
    def __nonzero__(self): return bool(self.val)
#---------------------------------------------------------------------------------------------------------
class Date(object):
    def __init__(self, val=None, **d): 
        if d: 
            for a, v in zip('YMDhms', _time.localtime(_time.time())[:6]): setattr(self, a, d.get(a, v))
            self.setval(); return
        if val==None: val = _time.time()
        if type(val) in (int, float, long): self.val = float(val)
        elif isinstance(val, Date): self.val = val.val
        elif type(val)==str:
            if not ':' in val and val.count('.')<=1: self.val = float(val) #secs
            else:
                try: self.val = _time.mktime(_time.strptime(val, '%Y.%m.%d-%H:%M:%S' if ':' in val else '%Y.%m.%d'))
                except: raise IllegalInitDateValue(val)
        else: raise IllegalInitDateValue(val)
        self.Y, self.M, self.D, self.h, self.m, self.s = _time.localtime(self.val)[:6]
    def setval(self): self.val = _time.mktime(_time.strptime('%02i.%02i.%02i-%02i:%02i:%02g'%(self.Y,self.M,self.D,self.h,self.m,self.s), '%Y.%m.%d-%H:%M:%S'))
    def __call__(self, **d): dd=dict(self.__dict__); del dd['val']; dd.update(d); return Date(**dd)
    def __float__(self): return self.val
    def __int__(self):   return int(self.val)
    def __getstate__(self): return self.val
    def __setstate__(self, val): self.__init__(val)
#    def __str__(self): return '%04i.%02i.%02i-%02i:%02i:%02i'%(self.Y, self.M, self.D, self.h, self.m, self.s) if 
    def __str__(self): return ('%(Y)04i.%(M)02i.%(D)02i'+'-%(h)02i:%(m)02i:%(s)02i'*bool(self.h or self.m or self.s))%self.__dict__ 
    def __repr__(self): return "Date('%s')"%self
    def __add__(self, other):
        #if type(other) in (int,float,long): return Date(self.val+other)
        if isinstance(other, Date): raise BaseTimeException('Forbidden operation: %r + %r'%(self,other))
        if isinstance(other, Time): return Date(self.val+other.val)
        return self+Time(other)
    def __sub__(self, other): 
        #if type(other) in (int,float,long): return Date(self.val-other)
        if isinstance(other, Date): return Time(self.val-other.val)
        if isinstance(other, Time): return Date(self.val-other.val)
        try: return self-Time(other)
        except IllegalInitTimeValue as e: return self-Date(other)
    def __radd__(self, other): return self+other
    def __rsub__(self, other): return Date(other) - self
    def __cmp__(self, other): return cmp(self.val, Date(other).val)
    def __mod__(self, other): return int(self.val)%other
#---------------------------------------------------------------------------------------------------------
class RunTime:
    def __init__(self): self.start, self.progress, self.runtime, self.diff = _time.time(), 0., 0., 0.; self.call = self.start
    def __call__(self, progress): 
        t = _time.time(); diff = (t-self.call)/(progress-self.progress if progress else 1.) 
        self.diff = .5*(diff+self.diff) if self.diff else diff
        self.runtime, self.call, self.progress = t - self.start + self.diff*(1.-progress), t, progress
    def __str__(self): return str(Time(self.runtime))
#---------------------------------------------------------------------------------------------------------
import re
_re_date = re.compile(r'(?:_\d\d\d\d_\d\d_\d\d|(?:_\d\d){,3})(?:_(?:_\d\d){,3})?')
_re_time = re.compile(r'_(\d{1,}Y)?(\d{1,}(?:M|W))?'+r'(\d{1,}%s)?'*4%tuple('Dhms'))
_units_D = {'Y':31579200, 'M':2635200, 'W':604800, 'D':86400, 'h':3600, 'm':60, 's':1}
is_date = lambda d: getattr(_re_date.search(d), 'group', lambda: None)()==d
is_time = lambda t: getattr(_re_time.search(t), 'group', lambda: None)()==t and len(t)>1

class DateInterval:
    'date is [_DD|_MM_DD|_YYYY_MM_DD][__hh[_mm[_ss]]] or _[?Y][?M|W][?D][?h][?m][?s]'
    def __init__(self, **D): 
        self.__dict__.update(D)
        if '_cdate' in D: self.cdate, self.mdate = self._cdate in self, self._mdate in self
    def setDate(self, date, cdate, mdate): 
        'date is [_DD|_MM_DD|_YYYY_MM_DD][__hh[_mm[_ss]]]'
        d, t = ([ [ int(i) for i in s.split('_') if i ] for s in date.split('__')]+[[]])[:2]
        self._min, self._time = Date(**dict([('h',0),('m',0),('s',0)]+zip('DMY', reversed(d))+zip('hms', t))).val, None
        self._delta = (3600,60,1)[len(t)-1] if t else 86400; self._max = Date(self._min+self._delta)
        self._cdate, self._mdate, self.cdate, self.mdate = cdate, mdate, cdate in self, mdate in self 
        return self
    def setTime(self, date, cdate, mdate): 
        'date is _[?Y][?M|W][?D][?h][?m][?s]'
        date = [ (i[-1], int(i[:-1])) for i in _re_time.match(date).groups() if i ]
        self._delta, self._time = _units_D[date[-1][0]], sum([ _units_D[k]*v for k, v in date ])
        self._min = Date(**dict([ (i,int(i in 'MD')) for i in 'YMDDhms'['YMWDhms'.index(date[-1][0])+1:] ])).val-self._time; self._max = self._min+self._delta
        self._cdate, self._mdate, self.cdate, self.mdate = cdate, mdate, cdate in self, mdate in self 
        return self
    def __nonzero__(self): return self.mdate
    def __contains__(self, other): return self._min<=float(other) and self._max>=float(other)
    def __add__(self, x): 
        d = x*self._delta if type(x) in (int,long,float) else x.val if isinstance(x, Time) else x._time if isinstance(x, DateInterval) else None
        return DateInterval(**dict(self.__dict__.items()+[('_max', self._max+d),('_time', self._time+d if self._time else None)]))
    def __sub__(self, x):
        d = x*self._delta if type(x) in (int,long,float) else x.val if isinstance(x, Time) else x._time if isinstance(x, DateInterval) else None
        return DateInterval(**dict(self.__dict__.items()+[('_min', self._min-d),('_time', self._time-d if self._time else None)]))
    def __str__(self): return '%s--%s'%(Date(self._min), Date(self._max))
#    def __repr__(self): return 'DateIiterval(_min=int(%s), _max=int(%s), _time= '%(Date(self._min), Date(self._max))            
#---------------------------------------------------------------------------------------------------------
#print time(600), Date(), Date()+60, Date()-(Date()-600)
#t = Date()-(Date()-600)
#print t.s, t, t(h=2), Date()-Date(h=0,m=0,s=0)
