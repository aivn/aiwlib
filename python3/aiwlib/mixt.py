# -*- coding: utf-8 -*-
'''Copyright (C) 2013 Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''
#-----------------------------------------------------------------------------
import sys, os, time, fnmatch
#-----------------------------------------------------------------------------
#   OS/SYS
#-----------------------------------------------------------------------------
def get_login() :
    try: return os.getlogin()
    except OSError as e: return os.environ.get('USER')
#-----------------------------------------------------------------------------
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
#-----------------------------------------------------------------------------
def mk_daemon():
    'начинает демонизацию'
    if os.fork(): sys.exit()
    os.setsid(); sys.stdin = open('/dev/null') 
    os.dup2(sys.stdin.fileno(), sys.__stdin__.fileno()) 
def set_output(logfile='/dev/null'):
    'перенаправляет stdout и stderr в logfile'
    if type(logfile) is str: logfile = open(logfile, 'w') 
    os.dup2(logfile.fileno(), sys.stdout.fileno()) 
    os.dup2(logfile.fileno(), sys.stderr.fileno())
    sys.logfile = logfile
def daemonize(logfile='/dev/null'):
    'демонизирует процесс, stdout и stderr перенаправляются в logfile'
    mk_daemon(); set_output(logfile)
#-----------------------------------------------------------------------------
_last_except_report = None
def except_report(stderr=sys.stderr, short=False):
    if sys.exc_info()!=(None, None, None): last_type, last_value, last_traceback = sys.exc_info()
    else: last_type, last_value, last_traceback = sys.last_type, sys.last_value, sys.last_traceback 
    if short: 
        try: descript = ['%s in %r : %s\n'%(last_type.__name__, last_traceback.tb_next.tb_frame.f_code.co_filename, last_value)]
        except: short = False
    if not short:
        tb, descript = last_traceback, []
        while tb:
            fname, lno = tb.tb_frame.f_code.co_filename, tb.tb_lineno
            descript.append('\tFile "%s", line %s, in %s\n'%(fname, lno, tb.tb_frame.f_code.co_name))
            tb = tb.tb_next
        descript.append('%s : %s\n'%(last_type.__name__, last_value))
    global _last_except_report
    if hasattr(stderr, 'writelines') and _last_except_report!=descript: stderr.writelines(descript)
    _last_except_report = descript; return descript
#-----------------------------------------------------------------------------
normpath = lambda path, *apps: os.path.abspath(os.path.expanduser(os.path.expandvars(os.path.join(path, *apps))))
get_md5sums = lambda Lf: [ (i.split()[0], i.split(' ',1)[1].strip()) for i in 
                           os.popen('md5sum -b '+' %s'*len(Lf)%tuple(Lf)).readlines() ]
#-----------------------------------------------------------------------------
is_name = lambda x: x and (x[0]=='_' or x[0].isalpha()) and x.replace('_','').isalnum()
is_name_eq = lambda x: '=' in x and (lambda a, b: is_name(a) and not b.startswith('='))(*x.split('=', 1))
#-----------------------------------------------------------------------------
#   OUT
#-----------------------------------------------------------------------------
str_len = lambda S: int(reduce(lambda r, c: r+1./(1+(ord(c)>127))+4*(c=='\t'), S, 0))
time2string = lambda x, precision=3: "%i:%02i:%0*.*f"%(int(x/3600), int(x/60)%60, 2+bool(precision)+precision, precision, x%60)
#string2time = lambda x: reduce(lambda S, v: S+float(v[0])*v[1], map(None, x.split(':'), (3600, 60, 1)), 0.)
string2time = lambda x: reduce(lambda S, v: S+float(v[0])*v[1], zip(x.split(':'), (3600, 60, 1)), 0.)
date2string = lambda t: time.strftime("%Y.%m.%d-%X", time.localtime(t))
string2date = lambda s: time.mktime(time.strptime(s, "%Y.%m.%d-%X"))
def string2bool(value):
    if value in 'Y y YES Yes yes ON On on TRUE True true V v 1'.split(): return True
    if value in 'N n NO No no OFF Off off FALSE False false X x 0'.split(): return False
    raise Exception('incorrect value=%s for convert to bool, Y|y|YES|Yes|yes|ON|On|on|TRUE|True|true|V|v|1' 
                    ' or N|n|NO|No|no|OFF|Off|off|FALSE|False|false|X|x|0 expected'%value)
def size2string(sz):
    for d, p in ((2**40,'%.2fT'), (2**30,'%.1fG'), (2**20,'%.1fM'), (2**10,'%.1fK'), (1,'%iB')):
        if sz>=d: return p%(float(sz)/d)
    return '0'
def hashable(x):
    try: hash(x); return True
    except: return False
def get_tty_width(stream=sys.stdout, default=80):
    try: return int(os.popen('stty size 2> /dev/null').readline().split()[1]) if stream.isatty() else default if default else 80
    except: return default if default else 80
def compare(name, patterns):
    if name=='' and '' not in patterns: return False #patch for '' in '*'
    return any([fnmatch.fnmatch(name, p) for p in patterns])
#-----------------------------------------------------------------------------
class ProgressBar:
    def __init__(self, stdout=sys.stdout, interact=False, tty_width=None): 
        self.start, self.stdout, self.interact, self.progress  = time.time(), stdout, interact, 0
        self.width = get_tty_width(stdout, tty_width)
    def clean(self): self.start, self.progress = time.time(), 0 #; self.stdout.write( '\r'+' '*self.width ); self.stdout.flush()
    def out(self, progress, prompt=''):
        if self.interact or self.stdout.isatty():
            t = time.time()-self.start
            if progress>0: prompt +=' %s from %s ['%(time2string(t, 0), time2string(t/progress, 0))
            else: prompt += '['
            new_p = int(progress*(self.width-len(prompt)-4)+.5)
            if new_p != self.progress: 
                self.progress = new_p 
                self.stdout.write('\r'+prompt+'#'*new_p+' '*(self.width-len(prompt)-new_p-1) + ']') 
                self.stdout.flush()
    def close(self, prompt='', result='OK'):
        prompt += ' %s ['%time2string(time.time()-self.start)
        self.stdout.write('\r'+prompt+'#'*(self.width-len(prompt)-3-len(result))+'] '+result+' \n') 
        self.stdout.flush()
#-----------------------------------------------------------------------------
def table2strlist(LL, pattern=None, s_line=1, s_empty=2, s_bound=1, max_len=None, conv2str=str):
    '''преобразует таблицу (список списков строк) LL в список форматированных строк. 
pattern --- паттерн аналогичный заголовку таблиц LaTeX (|lrt|), 
s_line, s_empty, s_bound --- число пробелов до вертикальных линий, между колонками без линий и по краям (если нет линий)'''
    L0 = filter(None, LL)
    if not L0 or not L0[0]: return []
    if pattern==None: pattern = ('l|'*len(L0[0]))[:-1]
    width_list = reduce(lambda wl, l: map(lambda i, j: max([i]+map(str_len, str(j).split('\n'))), wl, l) if l else wl, LL, [])
    P = reduce(lambda s, arg: s.replace(*arg), 
               [('r', '%s'), ('c', '%s'), ('l', '%s'), ('|', ' '*s_line+'|'+' '*s_line), ('s%', 's'+' '*s_empty+'%')], 
               pattern).strip()
    P = ' '*s_bound*(P[0]!='|')+P+' '*s_bound*(P[-1]!='|') 
    just = [ str.rjust if j=='r' else str.center if j=='c' else str.ljust for j in pattern.replace('|', '') ]
    hline = (P%tuple([ ' '*w for w in width_list ])).replace(' ', '-').replace('|', '+')[:max_len]
    if len(width_list)!=len(just): raise Exception('illegal pattern or table size')
    R = []
    for L in LL:
        if L == None: R.append(hline); continue
        L = map(str, L) #L = map(conv2str, L)
        strnum = max([ l.count('\n')+1 for l in L ]); L = [ (l.split('\n')+['']*strnum)[:strnum] for l in L ]
        for snum in range(strnum): R.append(( P%tuple([ conv2str(j(l[snum], w)) 
                                                        for j, l, w in zip(just, L, width_list) ]) )[:max_len])
    return [ l+'\n' for l in R ]
#---------------------------------------------------------------------------------------------------------
