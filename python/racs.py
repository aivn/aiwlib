# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
'''опции командной строки:
  key=value --- задает значение параметра расчета key, конвертируется к типу 
значения параметра по умолчанию. Возможно задание серии значений параметра как 
  key=[expression]  --- expression вычисляется функцией eval в словаре модуля math
  key=[x1,x2..xn]   --- хаскелль-стиль для арифметической прогрессии
  key=[x1:step..xn] --- арифметическая прогрессия с шагом step
  key=[x1@step..xn] --- геометрическая прогрессия с множителем step
  key=[x1#size..xn] --- арифметическая прогрессия из size элементов
  key=[x1^size..xn] --- геометрическая прогрессия из size элементов
Значения x1, xn, step, size вычисляются функцией eval в словаре модуля math.
Параметры x1 и xn всегда включаются в серию. При явном задании параметра step шаг 
корректируется при необходимости для точного попадания в xn. Параметр size должен
быть целочисленным  (не менее двух). Если серии заданы для нескольких параметров,
вычисляются все возможные комбинации значений (декартово произведение).

Для всех параметров (кроме серийных) возможно дублирование, актуальным является 
последнее значение. Параметры командной строки перекрывают параметры из файла. 
Для булевых параметров имя параметра экивалентно заданию True. Длинные имена 
параметров могу задаваться как с одним, так и с двумя минусами, короткие имена
только с одним. Параметр path задается без минусов.

  -h, --help --- показать эту справку и выйти

  --repo=PATH --- задает путь к репозитрию для создания расчета
    path=PATH --- явно задает путь к директории расчета, если расчет сущестовал 
                  словарь расчета будет обновлен из директории расчета
  -p|--clean-path[=Y] --- очищать директория явно заданную расчета, при этом
                          словарь расчета не обновляется из директории
   -s|--symlink[=Y] --- создавать символическую ссылку '_' на последнюю 
                        директорию расчета
   -d|--daemonize[=N] --- "демонизировать" расчет при запуске (освободить терминал, 
                          вывод будет перенаправлен в logfile в директории расчета)
   -S|--statechecker[=Y] --- запускать демона, фиксирующего  в файле .RACS аварийное
                             завершение расчета
   -c|--copies[=1] --- число копий процесса при проведении расчетов с серийными 
                       параметрами
   -e|--on-exit[=Y] --- по завершениии расчета автоматически фиксировать время работы,
                        состояние расчета и сохранять измения параметров на диск
   -n|--calc-num[=3] --- число знаков в номере расчета (в текущем дне) при 
                         автоматичской генерации имени директории расчета 
   -P|--auto-pull[=Y] --- автоматически сохранять все параметры расчета из 
                          контролируемых расчетом объектов.

'''
#-------------------------------------------------------------------------------
import os, sys, math, gtable, calc
from calc import *
from math import *
#-------------------------------------------------------------------------------
def _make_path_hook(self):
    calc._make_path(self)
    if Calc._symlink and not os.path.samefile(os.getcwd(), Calc.repo):
        try:
            if os.path.islink('_') or os.path.exists('_'): os.remove('_')
            os.symlink(self.path, '_')
        except Exception, e: print>>sys.stderr, e
    if Calc._statechecker:
        import inspect
        f = open('/tmp/racs-schk-%i'%os.getpid(), 'w')
        print>>f, '''#!/usr/bin/python -S
# -*- coding: utf-8 -*-
import os, sys, cPickle, time, socket
d2s = lambda t: time.strftime('%Y.%%m.%d-%X', time.localtime(t))
'''
        print>>f, inspect.getsource(get_login)
        print>>f, inspect.getsource(mk_daemon)
        print>>f, inspect.getsource(set_output)
        print>>f, '''mk_daemon(); set_output()
while 1:
    if '%i' in os.listdir('/proc/'): time.sleep(10)
    else: 
        R = cPickle.load(open(%r+'.RACS'))
        if R['statelist'][-1][0]=='started':
            R['statelist'].append(('killed', get_login(), socket.gethostname(), d2s(time.time()), 'racs-statechecker'))
            cPickle.dump(R, open(%r+'.RACS', 'w'))
        break
        '''%(os.getpid(), self.path, self.path)
        f.close(); os.chmod(f.name, 0700); os.system(f.name); os.remove(f.name)
    if Calc._on_exit: atexit.register(_on_exit, self)
    if self._daemonize: mixt.set_output(self.path+'logfile')
    return self.path
#-------------------------------------------------------------------------------
def _init_hook(self):
    if 'path' in self.__dict__: 
        if os.path.exists(self.path) and Calc._clean_path: os.system('rm -rf %s/*'%self.path)
        if not os.path.exists(self.path): os.makedirs(self.path)
        if Calc._on_exit: atexit.register(_on_exit, self)
        if self._daemonize: mixt.set_output(self.path+'logfile')    
#-------------------------------------------------------------------------------
def _on_exit(calc): 
    import os, time, mixt, chrono
    try:
        runtime = chrono.Time(time.time()-self._starttime)
        print 'RUNTIME %s SIZE %s (.RACS %s) %s'%(runtime, os.popen('du -hs '+calc.path).readline().split()[0],
                                                  mixt.size2string(os.path.getsize(calc.path+'.RACS')), calc.path)
        # calc.update() ???
        if calc.statelist[-1][0]=='started':
            if hasattr(sys, 'last_value'): calc.add_state('stopped')
            else: calc.progress, calc.runtime = 1., runtime; calc.add_state('finished')
    finally: calc.commit()
#-------------------------------------------------------------------------------
if any(o in sys.argv[1:] for o in '-h -help --help'.split()):
    print __doc__
    print ''.join(l for l in open(sys.modules['__main__'].__file__) if '#@' in l),
    sys.exit()
#-------------------------------------------------------------------------------
Calc._starttime, Calc._make_path_hook, Calc._init_hook = time.time(), _make_path_hook, _init_hook
opts = { 'symlink':('s', True), 'daemonize':('d', False), 'statechecker':('S', True), 'repo':('', 'repo'),
         'on-exit':('e', True), 'calc-num':('n', 3), 'auto-pull':('P', True), 'clean-path':('p', True), 'copies':('c', 1) }
for k, v in opts.items(): setattr(Calc, k, v[1])
Calc._qargs, Calc._args, arg_seqs, arg_order, i = [], list(sys.argv[1:]), {}, [], 0
while i<len(Calc._args):
    A = Calc._args[i]
    if mixt.is_name_eq(A) and A.split('=', 1)[1][0]=='[' and A[-1]==']':
        arg, l = A.split('=', 1)
        try: L = eval(l, math.__dict__)
        except SyntaxError, e:
            s = l[1:-1]
            for t in '#^@:,':
                if s.count('..')==1 and s.split('..')[0].count(t)==1:
                    ab, c = s.split('..'); a, b = ab.split(t)
                    a, b, c = [float(eval(x, math.__dict__)) for x in (a, b, c)]
                    if t==',': b, t = b-a, ':'
                    if t==':': b, t = int((c-a)/b+1.5), '#'
                    if t=='#': d = (c-a)/(b-1); L = [a+d*j for j in range(int(b))]
                    if t=='@': b, t = int(log(c/a)/log(b)+1.5), '^' # c=a*d**(n-1)
                    if t=='^': d = exp(log(c/a)/(b-1)); L = [a*d**j for j in range(int(b))]
                    if not L: raise Exception('incorrect step or limits in expression '+A)
                    break
            else: raise Exception('incorrect sequence expression '+A)
        arg_seqs[arg] = L; arg_order.append(arg); del Calc._args[i]
    elif mixt.is_name_eq(A): Calc._qargs.append(tuple(A.split('=', 1))); del Calc._args[i]
    elif A.startswith('-') and A!='-':
        for k, v in opts.items():
            if type(v[1]) is bool and any(A==x for x in ('-'+k, '--'+k, '-'+v[0])):
                setattr(Calc, k, True); del Calc._args[i]; break
            elif any(A.startswith(x+'=') and not A.split('=', 1)[1].startswith('=') for x in ('-'+k, '--'+k, '-'+v[0])):
                x = A.split('=', 1)[1]
                setattr(Calc, k, mixt.string2bool(x) if type(v[1]) is bool else int(x) if type(v[1]) is int else x)
                del Calc._args[i]; break
        else: i += 1
    else: i += 1
#-------------------------------------------------------------------------------
if Calc._daemonize: mixt.mk_daemon()
#-------------------------------------------------------------------------------
if arg_seqs:
    a0, i, copies, pids = arg_order.pop(0), 1, 1, []
    queue = reduce(lambda L, a: [l+[(a,x)] for x in arg_seqs[a] for l in L], arg_order, [[(a0,x)] for x in arg_seqs[a0]])
    print 'Start queue for %i items in %i threads, master PID=%i'(len(queue), copies, os.getpid())
    if Calc._daemonize: set_output()
    for q in queue:
        if len(pids)==copies:
            p = os.waitpid(-1, 0)[0]
            pids.remove(p)
        Calc._qargs += q+[('master', os.getpid())]
        pid = os.fork()
        if not pid: break
        pids.append(pid)
    else:
        while(pids): pids.remove(os.waitpid(-1, 0)[0])
        sys.exit()
#-------------------------------------------------------------------------------
__all__ = ['Calc']
