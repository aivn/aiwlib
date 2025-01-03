# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
'''Copyright (C) 2003-2017, 2023, 2024 Anton V. Ivanov <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0

  опции командной строки:
  key=value --- задает значение параметра расчета key, значение конвертируется 
к типу значения параметра по умолчанию. 
  key=@expression --- задает значение параметра расчета key, значение вычисляется 
функцией eval в словаре модуля math и на основе уже заданных параметров, затем 
конвертируется к типу значения параметра по умолчанию.
Возможно задание серии значений параметра как 
  key=[expression]  --- expression вычисляется функцией eval
  key=[x1,x2..xn]   --- хаскелль-стиль для арифметической прогрессии
  key=[x1:step..xn] --- арифметическая прогрессия с шагом step
  key=[x1@step..xn] --- геометрическая прогрессия с множителем step
  key=[x1#size..xn] --- арифметическая прогрессия из size элементов
  key=[x1^size..xn] --- геометрическая прогрессия из size элементов
Значения x1, xn, step, size вычисляются функцией eval в словаре модуля math и уже 
заданных аргументах. Параметры x1 и xn всегда включаются в серию. При явном задании 
параметра step шаг всегда корректируется для точного попадания в xn. Параметр size 
должен быть целочисленным  (не менее двух). Если серии заданы для нескольких 
параметров, вычисляются все возможные комбинации значений (декартово произведение).

  tag+ --- добавляет тэг tag

  -h|--help --- показать эту справку и выйти

Для всех параметров возможно дублирование. Для обычных параметров актуальным является 
последнее значение. Для серийных параметров при дублировании серии объединяются и 
и сортируются. 
Значения параметров по умолчанию могут быть изменены при вызове 
конструктора расчета Calc (за исключением параметров daemonize и copies), но параметры 
командной строки их перекрывают.
Для булевых параметров имя параметра экивалентно заданию значения True, кроме того
можно использовать значения  Y|y|YES|Yes|yes|ON|On|on|TRUE|True|true|V|v|1 или
N|n|NO|No|no|OFF|Off|off|FALSE|False|false|X|x|0. Длинные имена параметров 
могут задаваться как с одним, так и с двумя минусами, короткие имена
только с одним. Параметр path задается без минусов.

  -r|--repo=PATH --- задает путь к репозитрию для создания расчета
    path=PATH --- явно задает путь к директории расчета, если расчет существовал 
                  словарь расчета будет обновлен из директории расчета
  -p|--clean-path[=N] --- очищать явно заданную директорию расчета, при этом
                          словарь расчета не обновляется из директории
  -s|--symlink[=Y] --- создавать символическую ссылку '_' на последнюю 
                       директорию расчета
  -d|--daemonize[=N] --- "демонизировать" расчет при запуске (освободить терминал, 
                         вывод будет перенаправлен в logfile в директории расчета),
                         "демонизация" происходит при создание экземпляра класса Calc
  -S|--statechecker[=Y] --- запускать демона, фиксирующего  в файле .RACS аварийное
                            завершение расчета
  -c|--copies=1 --- число копий процесса при проведении расчетов с серийными 
                    параметрами
  -e|--on-exit[=Y] --- по завершении расчета автоматически фиксировать время работы,
                       состояние расчета и сохранять измения параметров на диск
  -n|--calc-num=3 --- число знаков в номере расчета (в текущем дне) при 
                      автоматической генерации имени директории расчета 
  -a|--auto-pull[=Y] --- автоматически сохранять все параметры расчета из 
                         контролируемых расчетом объектов.
  -m|--commit-sources[=Y] --- сохранять исходные коды расчета
  -z|--zip[=N] --- при построении очереди расчетов вместо декартового произведения 
                   последовательностей значений параметров использовать функцию zip 
  -q|--queue filename.dat --- формирует очередь заданий из файла filename.dat 
                              в формате gplt
  -D|--delone umesh l_cr --- формирует очередь заданий для досчета (уточнения) на 
                             основе неравномерной сетки. Сетка должна быть представлена 
                             файлами umesh.dat (узлы в формате gplt) и umesh.cells 
                             (индексы вершин ячеек), полученными например при помощи 
                             racs ... --delone ...  Файл umesh.dat должен содержать
                             минимум четрые колонки --- две колонки в пространстве
                             неоднородной сетки, две колонки в пространстве управляющих
                             параметров. l_cr задает критическую длину ребра, более длинные
                             ребра разбиваются пополам и формируют задания в пространстве
                             управляющих параметров.  

  -T|--title=TITLE --- задать имя серии
  -R|--repeat=.racs/... --- повторить серию
  -C|--continue=.racs/started-... --- продолжить серию (должен быть единственным аргументом!)
'''
#  --mpi[=N] --- для серийного запуска из под MPI. Вызывает MPI_Init/Finalize, 
#                создает уникальные директории расчетов, обеспечивает 
#                распределение заданий
#-------------------------------------------------------------------------------
#import os, sys, math, time, inspect, socket, thread, atexit
import os, sys, math, time, inspect, socket, atexit
try: import cPickle as pickle
except: import pickle
import aiwlib.gtable 
import aiwlib.calc as calc
import aiwlib.mixt as mixt
import aiwlib.sources as sources
from aiwlib.calc import Calc
#from math import * #???
#-------------------------------------------------------------------------------
#   hooks
#-------------------------------------------------------------------------------
def _calc_configure(self):
    if calc._racs_params['_symlink'] and not (os.path.exists(calc._racs_params['_repo']) 
                                              and os.path.samefile(os.getcwd(), calc._racs_params['_repo'])):
        try:
            if os.path.islink('_') or os.path.exists('_'): os.remove('_')
            os.symlink(self.path, '_')
        except Exception as e: print(e, file=sys.stderr)
    if calc._racs_params['_statechecker']:
        import inspect
        f = open('/tmp/racs-schk-%i'%os.getpid(), 'w')
        print('''#!/usr/bin/python3 -S
import os, sys, pickle, time, socket
d2s = lambda t: time.strftime('%Y.%m.%d-%X', time.localtime(t))
''', file=f)
        print(inspect.getsource(mixt.get_login), file=f)
        print(inspect.getsource(mixt.mk_daemon), file=f)
        print(inspect.getsource(mixt.set_output), file=f)
        print('''mk_daemon(); set_output()
while 1:
    if '%i' in os.listdir('/proc/'): time.sleep(10)
    else: 
        R = pickle.load(open(%r+'.RACS', 'rb'))
        if R['statelist'][-1][0]=='started':
            R['statelist'].append(('killed', get_login(), socket.gethostname(), d2s(time.time()), 'racs-statechecker'))
            pickle.dump(R, open(%r+'.RACS', 'wb'), protocol=1); os.utime(%r, None)
        break
        '''%(os.getpid(), self.path, self.path, self.path), file=f)
        f.close(); os.chmod(f.name, 0o700); os.system(f.name); os.remove(f.name)
    if calc._racs_params['_on_exit']: atexit.register(_on_exit, self)
    if calc._racs_params['_commit_sources']:
        try: self.md5sum = sources.commit(self.path)
        except Exception as e: print>>sys.stderr, e
    if calc._racs_params['_daemonize'] or (calc._arg_seqs and calc._racs_params['_copies']>1) or calc._racs_params['_mpi']: 
        mixt.set_output(self.path+'logfile')
    self.commit() #???
#-------------------------------------------------------------------------------
def _init_hook(self):
    self.add_state('started', os.getpid())
    self._starttime = time.time()
    if 'path' in self.__dict__: 
        if calc._racs_params['_clean_path'] and os.path.exists(self.path): os.system('rm -rf %r'%self.path)
        if calc._racs_params['_mpi']==2: 
            self.path = os.path.join(self.path, '%%0%ii/'%len(str(mpi_proc_count()))%mpi_proc_number())
        if not os.path.exists(self.path): os.makedirs(self.path)
        _calc_configure(self)
calc._init_hook = _init_hook
#-------------------------------------------------------------------------------
def _make_path_hook(self):
    if calc._racs_params['_mpi']<2 or (calc._racs_params['_mpi']==2 and mpi_proc_number()==0): 
        self.path = mixt.make_path(calc._racs_params['_repo']%self, calc._racs_params['_calc_num']) 
    if calc._racs_params['_mpi']==2: # раздаем путь группе MPI процессов
        if mpi_proc_number()==0:
            for p in range(1, mpi_proc_count()): mpi_send(self.path, p)
        else: self.path = mpi_recv(0)[0]
        self.path += '%%0%ii/'%len(str(mpi_proc_count()))%mpi_proc_number(); os.makedirs(self.path)
    _calc_configure(self)
    return self.path
calc._make_path_hook = _make_path_hook
#-------------------------------------------------------------------------------
def _on_exit(self): 
    if hasattr(self, '_run4stat'):
        connect = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        connect.connect(('127.0.0.1', self._run4stat[0]))
        s = pickle.dumps([self._run4stat[1]]+[getattr(self, i) for i in self._run4stat[2:]], protocol=1) 
        connect.send('%08i'%len(s)+s); connect.close()
        return 
    import os, time
    from . import mixt, chrono
    try:
        for wrap in self._wraps: self.pull(wrap._core, _prefix=wrap._prefix) 
        runtime = chrono.Time(time.time()-self._starttime)
        # self.update() ???
        if self.statelist and self.statelist[-1][0]=='started':
            if hasattr(sys, 'last_value'): self.add_state('stopped')
            else: self.progress, self.runtime = 1., runtime; self.add_state('finished')
    finally: self.commit()
    rsz = os.path.getsize(self.path+'.RACS') 
    rcolor = ('' if rsz<4096 else ';33' if rsz<4096*8 else ';31' if rsz<4096*32 else ';37;41' if rsz<4096*256 else ';37;5;41')+'m%s'
    print('\nRUNTIME \033[1m%s\033[0m SIZE \033[1m%s\033[0m (.RACS \033[1%s\033[0m) %s'%(
        runtime, os.popen('du -hs '+self.path).readline().split()[0], rcolor%mixt.size2string(rsz), self.path))
#-------------------------------------------------------------------------------
def run4stat(self, _count, _copies=1, _mkdir=True, **params):
    '''проводит _count одинаковых расчетов для набора статистики, 
    запуская не более _copies одновременно, для каждого расчета создается
    своя директория path+номер расчета (если не указано _nodir). 
    Словарь params содержит параметры с результатми сбора статистики (ключи словаря) попадающие
    в итоговый расчет. Значениями словаря являются функции собирающие статистику. Имена аргументов
    функций отвечают параметрам, передаваемым через TCP/IP с дочерних расчетов в головной расчет, 
    функции получают их в виде отдельных списков. Значения этих же параметров сохраняются в отдельном 
    файле .stat в формате pickle в виде словаря {имя-параметра:список-значений}.'''
    path = self.path # создаем путь для головного расчета
    cargs = set(sum([inspect.getargspec(f) for f in params.values()], [])) # список параметров для передачи
    stat = dict([(k, [None]*_count) for k in cargs]) # словарь с результатами
    #------------ запуск сервера --------------------------
    serv, port = socket.socket(AF_INET,SOCK_STREAM), 2048
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    while 1:
        try: serv.bind(('127.0.0.1', port)); break  
        except(socket.error, e): port += 1
    serv.listen(5)
    def recv_data():
        while cnum<_count or pids: #???
            connect, addr = serv.accept()
            data = pickle.loads(int(connect.recv(connect.recv(8)))); c = data.pop(0)
            for k, v in zip(cargs, data): stat[k][c] = v
    thread.start_new_thread(recv_data, ())
    #-------------- цикл по расчетам ---------------------
    pids = []
    for cnum in xrange(_count):
        if len(pids)==_copies:
            p = os.waitpid(-1, 0)[0]
            pids.remove(p)
        pid = os.fork()
        if not pid: 
            if _dir: self.path += '%%0%ii/'%(math.log10(_count)+1)%cnum; os.mkdir(self.path)
            self._run4stat = [port, cnum]+cargs
            return # очередной расчет запущен
        self.set_progress(float(cnum)/_count)
        pids.append(pid)
    while(pids): pids.remove(os.waitpid(-1, 0)[0])
    #-------------- окончание расчета --------------------
    pickle.dump(stat, open(path+'.stat', 'wb'), protocol=1)
    for k, f in params.items(): setattr(self, k, f(*dict([(i, stat[i]) for i in inspect.getargspec(f)])))
    sys.exit()
#calc.Calc.run4stat = run4stat
#-------------------------------------------------------------------------------
#   parse command line options
#-------------------------------------------------------------------------------
if any(o in sys.argv[1:] for o in '-h -help --help'.split()): calc._help_mode = True; del sys.argv[1:]; print(__doc__)
#-------------------------------------------------------------------------------
opts = { 'symlink':('s', True), 'daemonize':('d', False), 'statechecker':('S', True), 'repo':('r', 'repo'),
         'on-exit':('e', True), 'calc-num':('n', 3), 'auto-pull':('a', True), 'clean-path':('p', False), 
         'copies':('c', 1), 'commit-sources':('m', True), 'mpi':('', False), 'title':('T', ''), 'zip':('z', False) }
for k, v in opts.items(): calc._racs_params['_'+k.replace('-', '_')] = v[1]
calc._cl_args, calc._arg_seqs, calc._arg_order, i, repeat_mode = list(sys.argv[1:]), {}, [], 0, False
while i<len(calc._cl_args):
    A = calc._cl_args[i]
    if A.endswith('+'): calc._cl_tags.append(A[:-1]); del calc._cl_args[i]
    elif mixt.is_name_eq(A) and A.split('=', 1)[1][0]=='[' and A[-1]==']':
        arg, l = A.split('=', 1)
        try: L = eval(l, math.__dict__, dict(calc._args_from_racs))
        except SyntaxError as e:
            s = l[1:-1]
            for t in '#^@:,':
                if s.count('..')==1 and s.split('..')[0].count(t)==1:
                    ab, c = s.split('..'); a, b = ab.split(t)
                    a, b, c = [float(eval(x, math.__dict__, dict(calc._args_from_racs))) for x in (a, b, c)]
                    if t==',': b, t = b-a, ':'
                    if t==':': b, t = int((c-a)/b+1.5), '#'
                    if t=='#': d = (c-a)/(b-1); L = [a+d*j for j in range(int(b))]
                    if t=='@': b, t = int(math.log(c/a)/math.log(b)+1.5), '^' # c=a*d**(n-1)
                    if t=='^': d = math.exp(math.log(c/a)/(b-1)); L = [a*d**j for j in range(int(b))]
                    if not L: raise Exception('incorrect step or limits in expression '+A)
                    L = [float('%g'%x) for x in L]
                    break
            else: L = s.split() #raise Exception('incorrect sequence expression '+A) ???
        if arg in calc._arg_seqs:  calc._arg_seqs[arg] += L; calc._arg_seqs[arg].sort()
        else:  calc._arg_seqs[arg] = L; calc._arg_order.append(arg)
        del calc._cl_args[i]; calc._args_from_racs = filter(lambda i: i[0]!=arg, calc._args_from_racs)
    elif mixt.is_name_eq(A): 
        k, v = A.split('=', 1)
        if k in calc._arg_seqs: del calc._arg_seqs[k]; calc._arg_order.remove(k)
        if v.startswith('@'): v = eval(v[1:], math.__dict__, dict(calc._args_from_racs))
        calc._args_from_racs.append((k, v)); del calc._cl_args[i]
    elif A in ('-q', '-queue', '--queue'):
        D = {}
        for l in open(calc._cl_args[i+1]):
            if l.startswith('#:') and '=' in l: exec(l[2:], math.__dict__, D)
            elif l.startswith('#:') and not '=' in l: H = l[2:].split()
            elif l[0]!='#' and l.strip(): calc._queue.append(D.items()+zip(H, map(float, l.split())))
        del D, calc._cl_args[i:i+2]
    elif A in ('-D', '-delone', '--delone'):
        l_cr, points, edges, cells = float(calc._cl_args[i+2]), [], set(), [map(int, l.split()) for l in open(calc._cl_args[i+1]+'.cells')]
        for l in open(calc._cl_args[i+1]+'.dat'):
            if l.startswith('#:') and not '=' in l: H = l[2:].split()
            elif l[0]!='#' and l.strip(): points.append(map(float, l.split()))
        for c in cells:
            tr = [points[j] for j in c]            
            for j in (0, 1, 2):
                ij = tuple(sorted([c[j], c[(j+1)%3]])) 
                if not ij in edges:
                    edges.add(ij)
                    if ((tr[j][0]-tr[(j+1)%3][0])**2 + (tr[j][1]-tr[(j+1)%3][1])**2)**.5 <= l_cr:
                        calc._queue.append([(H[k], .5*(points[ij[0]][k]+points[ij[1]][k])) for k in (2, 3)])        
    elif A.startswith('-') and A!='-':
        for k, v in opts.items()+[('repeat', ('R', '')), ('continue', ('C', ''))]:
            if type(v[1]) is bool and any(A==x for x in ('-'+k, '--'+k, '-'+v[0])):
                calc._racs_params['_'+k] = True; calc._racs_cl_params.add('_'+k); del calc._cl_args[i]; break
            elif any(A.startswith(x+'=') and not A.split('=', 1)[1].startswith('=') for x in ('-'+k, '--'+k, '-'+v[0])):
                x = A.split('=', 1)[1]
                if k=='continue' and len(sys.argv)!=2: raise Exception('The argument %r must be unique!'%A)
                if k=='repeat' and (A!=sys.argv[1] or repeat_mode): raise Exception('The argument %r must be firts!'%A)
                if k in ('continue', 'repeat'):
                    fsrc = open(x); fsrc.readline(); aargs = fsrc.readline().split()[2:] # проблемы с пробелами ???
                    if k=='repeat': aargs.append('--title=') # сбрасываем название серии если было
                    else: calc._racs_params['_continue'] = x
                    calc._cl_args[0:1] = aargs; del sys.argv[1]; sys.argv[1:1] = aargs
                    repeat_mode = True; break
                calc._racs_params['_'+k] = mixt.string2bool(x) if type(v[1]) is bool else int(x) if type(v[1]) is int else x
                calc._racs_cl_params.add('_'+k); del calc._cl_args[i]; break
        else: i += 1;
    else: i += 1
if calc._racs_params['_mpi']: 
    calc._racs_params['_daemonize'] = False
    import ctypes; _mpi = ctypes.CDLL('libmpi.so', ctypes.RTLD_GLOBAL)
    try: from aiwlib.mpi4py import *
    except ImportError as e: print>>sys.stderr, '\033[37;1;41m Try "make mpi4py" in aiwlib/ directory? \033[0m'; raise
    mpi_init()
    if not calc._arg_seqs: atexit.register(mpi_finalize)
    calc._racs_params['_mpi'] = 3 if calc._arg_seqs and mpi_proc_count()>1 else 2 if mpi_proc_count()>1 else 1
#-------------------------------------------------------------------------------
__all__ = ['Calc']
