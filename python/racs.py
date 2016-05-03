# -*- coding: utf-8 -*-
'''модуль для отработки некоторых возможностей нового варианта RACS, 
идеология https://ru.wikipedia.org/wiki/Monkey_patch

Позволяет менять из командной строки:
1) заданные в теле скрипта параметры как key=value
2) вызовы методов как @func1=func2 (не реализовано)
3) аргументы методов как ... (не ясен синтаксис)
'''
import os, sys, pickle, math, re, time, socket, racslib, atexit
#-------------------------------------------------------------------------------
def _daemonize(logfile):
    'демонизирует процесс, stdout и stderr перенаправляются в logfile'
    if os.fork() : sys.exit()
    if type(logfile) is str : logfile = open( logfile, 'w' ) 
    stdin = open('/dev/null') 
    os.setsid() 
    os.dup2( stdin.fileno(),   sys.stdin.fileno()  ) 
    os.dup2( logfile.fileno(), sys.stdout.fileno() ) 
    os.dup2( logfile.fileno(), sys.stderr.fileno() )
#-------------------------------------------------------------------------------
_opt = '-'
def _recognize_word( word, wordlist ) : 
    variants = filter( lambda o : o==word, wordlist )
    if not variants : variants = filter( lambda o : o.startswith(word), wordlist )
    if not variants : reg = re.compile( '^'+'[aAeEiIoOuU]{0,}'.join(word)+'.*' ); variants = filter(reg.search, wordlist)
    if not variants : reg = '*'.join(word)+'*'; variants = filter( lambda o : fnmatch.fnmatch(o, reg), wordlist )
    if len(variants) != 1 : raise Exception('Unknown key "%s", variants=%s', word, variants)
    return variants[0]
def _recognize_option( s, options, aliases={}, use_abbrv=False ) :
    'recognize option $s from $options and $aliases'
    options = filter( lambda s : not s.startswith('_'), list(options) ) + sum( aliases.values(), [] )
    if _opt and s.startswith(_opt) : s, use_abbrv = s[len(_opt):], True
    if use_abbrv : res = _recognize_word( s, options )
    elif s in options : res = s
    else : return
    for k, v in aliases.items() :
        if res in v : return k
    return res
#-------------------------------------------------------------------------------
def _make_unique_path(base_path, num=3):
    'make_unique_path(base_path, num=3) ---> return base_pathXXX'
    path, name = os.path.split(os.path.abspath(os.path.expanduser(os.path.expandvars(base_path))))
    if not path : path= './'
    try: lpath = os.listdir(path)
    except OSError, e : lpath = ['']
    n = max([0]+map(lambda j: int(j[len(name):]), filter(lambda i: i.startswith(name) and i[len(name):].isdigit(), lpath)))+1
    return os.path.join(path, name+"%%0%si"%num%n)
#-------------------------------------------------------------------------------
def GetLogin() :
    try : return os.getlogin()
    except OSError, e : return os.environ.get('USER')
#-------------------------------------------------------------------------------
_last_except_report = None
def _except_report( stderr=sys.stderr, short=False ):
    if sys.exc_info() != (None,None,None) : last_type, last_value, last_traceback = sys.exc_info()
    else: last_type, last_value, last_traceback = sys.last_type, sys.last_value, sys.last_traceback
    if short : 
        try: descript = [ '%s in %r : %s\n'%( last_type.__name__, last_traceback.tb_next.tb_frame.f_code.co_filename, last_value ) ]
        except: short = False
    if not short :
        tb, descript = last_traceback, []
        while tb :
            fname, lno = tb.tb_frame.f_code.co_filename, tb.tb_lineno
            descript.append('\tFile "%s", line %s, in %s\n'%(fname, lno, tb.tb_frame.f_code.co_name))
            tb = tb.tb_next
        descript.append('%s : %s\n'%(last_type.__name__, last_value))
    global _last_except_report
    #if hasattr( stderr, 'writelines' ) and _last_except_report!=descript : stderr.writelines(descript)
    _last_except_report = descript; return descript
#-----------------------------------------------------------------------------
def size2string( sz ) :
    if not sz : return '0'
    d, p = filter( lambda i : sz/i[0], ((1,'%iB'),(2**10,'%iK'),(2**20,'%.1fM'),(2**30,'%.1fG'),(2**40,'%.2fT')) )[-1]
    return p%(float(sz)/d)
time2string = lambda x, precision=3: "%i:%02i:%0*.*f"%(int(x/3600), int(x/60)%60, 2+bool(precision)+precision, precision,x%60)
#-----------------------------------------------------------------------------
def _on_exit(calc, starttime):
    import os, time, racslib
    try:
        finishtime = time.time(); runtime = racslib.Time(finishtime-starttime)
        print 'RUNTIME %s SIZE %s (.RACS %s) %s'%(time2string(finishtime-starttime),
                                                  os.popen('du -hs '+calc.path).readline().split()[0],
                                                  size2string(os.path.getsize(calc.path+'.RACS')), calc.path)
        #calc.update()
        if calc.statelist[-1][0]=='started':
            if hasattr(sys, 'last_value'):
                calc.statelist.append(('stopped', GetLogin(), socket.gethostname(),
                                       racslib.Date(time.time()), ''.join(_except_report())))
            else: #calc.set_state('finished') #; calc.set_progress(1., runtime)
                calc.statelist.append(('finished', GetLogin(), socket.gethostname(), racslib.Date(time.time())))
        calc.__dict__['runtime'] = runtime
    finally: calc.dump()
#---------------------------------------------------------------------------------------------------------
_starttime = time.time()
class Calc:
    def __init__(self, **D):
        'разбирает аргументы командной строки'
        self.__dict__.update(D)
        for i in sys.argv[1:]:
            if '=' in i: k, v = i.split('=', 1); setattr(self, k, self.__dict__.get(k, v).__class__(v))
        self.statelist = [ ('started', GetLogin(), socket.gethostname(), racslib.Date(time.time()), str(os.getpid())) ]
        # os.system( 'racs-killer %i '%os.getpid()+self.path )
    def __getitem__(self, arg):
        'для форматирования строки'
        if '?' in arg:
            flag, arg = arg.split('?', 1)
            return arg%self if eval(flag, dict(math.__dict__), self.__dict__) else ''
        auto = arg.endswith('=')
        if auto: arg = arg[:-1]
        val = eval(arg, dict(math.__dict__), self.__dict__)
        if auto and val.__class__==bool: return ',%s'%arg if val else ''
        if isinstance(val, float): val = '%g'%val
        return ',%s=%s'%(arg, val) if auto else val
#    def __setattr__(self, attr, value):
#        if attr in self.__dict__: self.__dict__[attr] = value.__class__(self.__dict__[attr])
#        else: self.__dict__[attr] = value
    def dump(self):
        D = dict(self.__dict__); del D['path']
        pickle.dump(D, open(self.path+'.RACS', 'w'))    
    #def wrap(self, core): return Wrap(self, core)
    def wrap(self, core): return _Wrap(self, core)
    def mk_path(self, repo):
        'создает уникальную директорию расчета на основе текущей даты и времени'
        while 1: 
            self.path = _make_unique_path(os.path.join(repo, time.strftime("c%y_%U_%w")))+'/'
            try: os.makedirs(self.path)
            except OSError, err: # check collision
                if err.errno!=17: raise
            break
        # CONFIG SYMLINK "_/"
        if not os.path.samefile(os.getcwd(), repo):
            if os.path.islink('_') or os.path.exists('_'): os.remove('_')
            os.symlink(self.path, '_')
        self.dump()
        os.system('racs-killer %i '%os.getpid()+self.path)
        if '--daemonize' in sys.argv: _daemonize(self.path+'logfile')
        atexit.register(_on_exit, self, _starttime)
        return self.path
#-------------------------------------------------------------------------------
class _Wrap: 
    def __init__(self, calc, core):
        self.__dict__['_calc'], self.__dict__['_core'] = calc, core
        if hasattr(core, 'this'): self.__dict__['this'] = core.this # easy link to SWIG class O_O!
    def __getattr__(self, attr): return getattr(self._core, attr)
    def __setattr__(self, attr, value):
        if attr in self._calc.__dict__: # перекрываем значениe по умолчанию
            value = self._calc.__dict__[attr] # через getattr?
            if getattr(self._core, attr).__class__==bool:
                if value in 'Y y YES Yes yes ON On on TRUE True true V v 1'.split(): value = True
                elif value in 'N n NO No no OFF Off off FALSE False false X x 0'.split(): value = False
                else: raise Exception('incorrect boolean value=%s'%value)
            else: value = getattr(self._core, attr).__class__(value)
        self._calc.__dict__[attr] = value
        setattr(self._core, attr, value)
#-------------------------------------------------------------------------------
