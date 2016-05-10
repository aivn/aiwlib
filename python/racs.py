# -*- coding: utf-8 -*-
#---------------------------------------------------------------------------------------------------------
'''опции командной строки:
  KEY=VALUE: задает значение параметра KEY (возможно задание макроса как @KEY для последующей работы),
             VALUE вычисляется функцией eval в глобальном пространстве имен math и локальном прострастве
             имен уже заданных параметров
  --path[=]PATH: явно задает путь к расчету, при этом заданные РАНЕЕ параметры вида KEY=VALUE, в т.ч. 
                 параметры по умолчанию в исходном файле Python будут сброшены
  --repo[=]PATH: задает путь к репозитрию для создания расчета
  --dst-repo[=]SSH-PATH: задает путь к репозитрию, куда расчет должен быть перемещен после завершения
  --[no]check-clones: [не]генерировать исключение, если в репозитории (dst-repo или repo) есть такой же расчет
  --clone-of[=]SSH-PATH: использовать в качестве образца параметры расчета находящегося в SSH-PATH
  --[no]wait: создать расчет в репозитории dst-repo или repo со статусом waited, но не запускать
  --[no]commit-sources: [не]сохранять исходный код

  --suite PARAMS VALUES: создать серию расчетов в репозитории dst-repo или repo со статусом waited
                         PARAMS --- имена изменяемых параметров через запятую, VALUES --- строка кода
                         на Python обрабатываемая функцией eval, дающая список кортежей со значениями 
                         параметров, по одному кортежу для каждого расчета серии

  --[no]daemonize: [не]"демонизировать" расчет при запуске (освободить терминал, вывод будет перенаправлен 
                   в logfile в директории расчета)
 Взаимодействие с демоном racs-dad (актуальны при <<демонизации>> расчета):
  --dad-part[=]PART: насколько расчет загружает вычислитель 0<PART<=1.0
  --dad-priority[=]PRIORITY: приоритет запуска расчета
  --dad-timeout[=]TIMEOUT: задержка между запросами к демону racs-dad


  -h, --help: показать эту справку и выйти
  -v, --verbose: вывести подробную информацию о разборе аргументов и выйти

PATH обычный путь в файловой системе, SSH-PATH допускает указание цепочки хостов вида host1:host2:...:path-on-hostN,
вдоль всей цепочки должна быть настроена ssh авторизация по открытому ключу, на последнем хосте должен быть установлен 
RACS. 
'''

import os, sys, math, calc
from calc import *
from math import *
#Calc._starttime, Calc._symlink, Calc._statechecker, Calc._on_exit = time.time(), True, True, True
Calc._starttime, Calc.args = time.time(), sys.argv
#-----------------------------------------------------------------------------
if '-h' in sys.argv or '--help' in sys.argv:
    print __doc__
    print ''.join(l for l in open(sys.modules['__main__'].__file__) if '#@' in l),
    sys.exit()
#-----------------------------------------------------------------------------
Calc._qargs, arg_seqs, arg_order, i = [], {}, [], 1
while i<len(sys.argv):
    A = sys.argv[i]
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
        arg_seqs[arg] = L; arg_order.append(arg); del sys.argv[i]
    elif mixt.is_name_eq(A): Calc._qargs.append(tuple(A.split('=', 1))); i += 1
    else: i += 1
#-----------------------------------------------------------------------------
Calc._daemonize = '-d' in sys.argv or '--daemonize' in sys.argv
if Calc._daemonize:
    if '-d' in sys.argv: sys.argv.remove('-d')
    else: sys.argv.remove('--daemonize')
    mixt.mk_daemon()
#-----------------------------------------------------------------------------
if arg_seqs:
    a0, i, copies, pids = arg_order.pop(0), 1, 1, []
    queue = reduce(lambda L, a: [l+[(a,x)] for x in arg_seqs[a] for l in L], arg_order, [[(a0,x)] for x in arg_seqs[a0]])
    while i<len(sys.argv):
        if sys.argv[i].startswith('-c=') or sys.argv[i].startswith('--copies='): copies = int(sys.argv.pop(i).split('=',1)[1])
        else: i += 1
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
#-----------------------------------------------------------------------------
def _make_path_hook(self):
    calc._make_path(self)
    if Calc._symlink and not os.path.samefile(os.getcwd(), Calc.repo):
        if os.path.islink('_') or os.path.exists('_'): os.remove('_')
        os.symlink(self.path, '_')
    if Calc._statechecker:
        import inspect
        f = open('/tmp/racs-schk-%i'%os.getpid(), 'w')
        print>>f, '''#!/usr/bin/python -S
# -*- coding: utf-8 -*-
import os, sys, cPickle, time, socket
d2s = lambda t: time.strftime('%Y.%%m.%d-%X', time.localtime(t))
'''
        print>>f, inspect.getsource(get_login)
        print>>f, inspect.getsource(daemonize)
        print>>f, '''daemonize()
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
__all__ = ['Calc']
