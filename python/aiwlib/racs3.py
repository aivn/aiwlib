# -*- coding: utf-8 -*-
import os, sys, time, pickle, socket
import aiwlib.chrono as chrono

#-----------------------------------------------------------------------------
def get_login() :
    try: return os.getlogin()
    except OSError as e: return os.environ.get('USER')
#-----------------------------------------------------------------------------
def string2bool(value):
    if value in 'Y y YES Yes yes ON On on TRUE True true V v 1'.split(): return True
    if value in 'N n NO No no OFF Off off FALSE False false X x 0'.split(): return False
    raise Exception('incorrect value=%s for convert to bool, Y|y|YES|Yes|yes|ON|On|on|TRUE|True|true|V|v|1' 
                    ' or N|n|NO|No|no|OFF|Off|off|FALSE|False|false|X|x|0 expected'%value)
normpath = lambda path, *apps: os.path.abspath(os.path.expanduser(os.path.expandvars(os.path.join(path, *apps))))
def make_path(repo, calc_num=3):
    'создает уникальную директорию расчета в репозитории repo на основе текущей даты и времени'
    repo = normpath(repo) #repo%self
    name = time.strftime("c%y_%W_")+str(time.localtime()[6]+1); s = len(name)
    while 1: 
        try: lpath = os.listdir(repo)
        except OSError as e: lpath = ['']
        n = max([0]+[int(p[s:]) for p in lpath if p.startswith(name) and p[s:].isdigit()])+1
        path = os.path.join(repo, name+'%%0%si'%calc_num%n)+'/'
        try: os.makedirs(path)
        except OSError as err: # check collision
            if err.errno!=17: raise
            continue
        break
    return path

class Calc:
    #---------------------------------------------------------------------------
    def __init__(self, **D):
        for a in sys.argv[1:]:
            if not '=' in a: continue
            k, v = a.split('=', 1)
            if not k in D or v.startswith('='): continue
            D[k] = string2bool(v) if type(D[k]) is bool else type(D[k])(v)
        self.__dict__.update(D)
        self.runtime, self.progress, self.args = chrono.Time(), 0., list(sys.argv)
        if not '_repo' in self.__dict__: self._repo = 'repo'
        self.add_state('started')
    #---------------------------------------------------------------------------
    def __getattr__(self, attr):
        'нужен для создания уникальной директории расчета по первому требованию (ленивые вычисления)'
        if attr=='path': self.path = make_path(self._repo); return self.path
        raise AttributeError(attr)
    #---------------------------------------------------------------------------
    def commit(self): 
        'Сохраняет содержимое расчета в базе' 
        #print dict(filter(lambda i:i[0][0]!='_' and i[0]!='path', self.__dict__.items())).keys()
        if os.path.exists(self.path+'.RACS'): os.remove(self.path+'.RACS') # ??? for update mtime of self.path ???
        pickle.dump(dict(filter(lambda i:i[0][0]!='_' and i[0]!='path', self.__dict__.items())), 
                     open(self.path+'.RACS', 'wb'), protocol=0)
        os.utime(self.path, None) # for racs cache refresh
    #---------------------------------------------------------------------------
    def add_state(self, state, info=None, host=socket.gethostname(), login=get_login()):
        'Устанавливает статус расчета, НЕ вызывает commit()'
        if not state in ('waited','activated','started','finished','stopped','suspended'):
            raise Exception('unknown status "%s" for "%s"'%(state, self.path if hasattr(self, 'path') else '???'))
        if info==None and state=='started': info = os.getpid()
        if info==None and state=='stopped': info = '' #.join(mixt.except_report(None))
        if not hasattr(self, 'statelist'): self.statelist = []
        if self.statelist: self.__dict__['runtime'] = chrono.Date()-self.statelist[-1][3]
        if state=='finished': self.__dict__['progress'] = 1.
        self.statelist.append((state, login, host, chrono.Date())+((info,) if info!=None else ()))
    def set_state(self, state, info=None, host=socket.gethostname(), login=get_login()):
        'Устанавливает статус расчета, вызывает commit()'
        self.add_state(state, info, host, login); self.commit()
    def set_progress(self, progress): #, prompt='',  runtime=-1):
        #runtime = (chrono.Date()-self.statelist[-1][3] if self.statelist else 0.) if runtime<0 else chrono.Time(runtime)
        self.__dict__['progress'], self.__dict__['runtime'] = progress, chrono.Date()-self.statelist[-1][3]
        self.commit()
    #---------------------------------------------------------------------------
    
