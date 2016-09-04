# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
'''опции командной строки:
  key=value --- задает значение параметра расчета key, значение конвертируется 
к типу значения параметра по умолчанию. 
  key=@expression --- задает значение параметра расчета key, значение вычисляется 
функцией eval в словаре модуля math и конвертируется к типу значения параметра 
по умолчанию.
Возможно задание серии значений параметра как 
  key=[expression]  --- expression вычисляется функцией eval в словаре модуля math
  key=[x1,x2..xn]   --- хаскелль-стиль для арифметической прогрессии
  key=[x1:step..xn] --- арифметическая прогрессия с шагом step
  key=[x1@step..xn] --- геометрическая прогрессия с множителем step
  key=[x1#size..xn] --- арифметическая прогрессия из size элементов
  key=[x1^size..xn] --- геометрическая прогрессия из size элементов
Значения x1, xn, step, size вычисляются функцией eval в словаре модуля math.
Параметры x1 и xn всегда включаются в серию. При явном задании параметра step шаг 
всегда корректируется для точного попадания в xn. Параметр size должен
быть целочисленным  (не менее двух). Если серии заданы для нескольких параметров,
вычисляются все возможные комбинации значений (декартово произведение).

  -h|--help --- показать эту справку и выйти

Для всех параметров (кроме серийных) возможно дублирование, актуальным является 
последнее значение. Значения параметров по умолчанию могут быть изменены при вызове 
конструктора расчета Calc (за исключением параметров daemonize и copies), но параметры 
командной строки их перекрывают.
Для булевых параметров имя параметра экивалентно заданию значения True, кроме того
можно использовать значения  Y|y|YES|Yes|yes|ON|On|on|TRUE|True|true|V|v|1 или
N|n|NO|No|no|OFF|Off|off|FALSE|False|false|X|x|0. Длинные имена параметров 
могут задаваться как с одним, так и с двумя минусами, короткие имена
только с одним. Параметр path задается без минусов.

  --repo=PATH --- задает путь к репозитрию для создания расчета
    path=PATH --- явно задает путь к директории расчета, если расчет сущестовал 
                  словарь расчета будет обновлен из директории расчета
  -p|--clean-path[=Y] --- очищать явно заданную директорию расчета, при этом
                          словарь расчета не обновляется из директории
  -s|--symlink[=Y] --- создавать символическую ссылку '_' на последнюю 
                       директорию расчета
  -d|--daemonize[=N] --- "демонизировать" расчет при запуске (освободить терминал, 
                         вывод будет перенаправлен в logfile в директории расчета),
                         "демонизация" происходит при импорте модуля racs
  -S|--statechecker[=Y] --- запускать демона, фиксирующего  в файле .RACS аварийное
                            завершение расчета
  -c|--copies=1 --- число копий процесса при проведении расчетов с серийными 
                    параметрами
  -e|--on-exit[=Y] --- по завершениии расчета автоматически фиксировать время работы,
                       состояние расчета и сохранять измения параметров на диск
  -n|--calc-num=3 --- число знаков в номере расчета (в текущем дне) при 
                      автоматической генерации имени директории расчета 
  -P|--auto-pull[=Y] --- автоматически сохранять все параметры расчета из 
                         контролируемых расчетом объектов.
'''
#-------------------------------------------------------------------------------
import os, sys, math, gtable, calc
from calc import Calc
from math import * #???
#-------------------------------------------------------------------------------
#   hooks
#-------------------------------------------------------------------------------
def _init_hook(self):
    if 'path' in self.__dict__: 
        if os.path.exists(self.path) and calc._racs_params['_clean_path']: os.system('rm -rf %s/*'%self.path)
        if not os.path.exists(self.path): os.makedirs(self.path)
        if calc._racs_params['_on_exit']: atexit.register(_on_exit, self)
        if calc._racs_params['_daemonize']: mixt.set_output(self.path+'logfile')    
calc._init_hook = _init_hook
#-------------------------------------------------------------------------------
def _make_path_hook(self):
    self.path = mixt.make_path(calc._racs_params['_repo']%self, calc._racs_params['_calc_num']) 
    if calc._racs_params['_symlink'] and not os.path.samefile(os.getcwd(), calc._racs_params['_repo']):
        try:
            if os.path.islink('_') or os.path.exists('_'): os.remove('_')
            os.symlink(self.path, '_')
        except Exception, e: print>>sys.stderr, e
    if calc._racs_params['_statechecker']:
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
    if calc._racs_params['_on_exit']: atexit.register(_on_exit, self)
    if calc._racs_params['_daemonize']: mixt.set_output(self.path+'logfile')
    return self.path
calc._make_path_hook = _make_path_hook
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
#   parse command line options
#-------------------------------------------------------------------------------
if any(o in sys.argv[1:] for o in '-h -help --help'.split()):
    print __doc__
    print ''.join(l for l in open(sys.modules['__main__'].__file__) if '#@' in l),
    sys.exit()
#-------------------------------------------------------------------------------
opts = { 'symlink':('s', True), 'daemonize':('d', False), 'statechecker':('S', True), 'repo':('', 'repo'),
         'on-exit':('e', True), 'calc-num':('n', 3), 'auto-pull':('P', True), 'clean-path':('p', True), 'copies':('c', 1) }
for k, v in opts.items(): calc._racs_params['_'+k.replace('-', '_')] = v[1]
calc._cl_args, arg_seqs, arg_order, i = list(sys.argv[1:]), {}, [], 0
while i<len(calc._cl_args):
    A = calc._cl_args[i]
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
        arg_seqs[arg] = L; arg_order.append(arg); del calc._cl_args[i]
    elif mixt.is_name_eq(A): 
        k, v = A.split('=', 1)
        if v.startswith('@'): v = eval(v[1:], math.__dict__)
        calc._args_from_racs.append((k, v)); del calc._cl_args[i]
    elif A.startswith('-') and A!='-':
        for k, v in opts.items():
            if type(v[1]) is bool and any(A==x for x in ('-'+k, '--'+k, '-'+v[0])):
                calc._racs_params[k] = True; _racs_cl_params.add(k); del calc._cl_args[i]; break
            elif any(A.startswith(x+'=') and not A.split('=', 1)[1].startswith('=') for x in ('-'+k, '--'+k, '-'+v[0])):
                x = A.split('=', 1)[1]
                calc._racs_params[k] = mixt.string2bool(x) if type(v[1]) is bool else int(x) if type(v[1]) is int else x
                _racs_cl_params.add(k); del calc._cl_args[i]; break
        else: i += 1
    else: i += 1
#-------------------------------------------------------------------------------
if calc._racs_params['_daemonize']: mixt.mk_daemon()
#-------------------------------------------------------------------------------
if arg_seqs:
    #a0, i, copies, pids = arg_order.pop(0), 1, 1, []
    #queue = reduce(lambda L, a: [l+[(a,x)] for x in arg_seqs[a] for l in L], arg_order, [[(a0,x)] for x in arg_seqs[a0]])
    copies, pids = calc._racs_params['copies'], []
    queue = reduce(lambda L, a: [l+[(a,x)] for x in arg_seqs[a] for l in L], arg_order, [[('racs_master', os.getpid())]])
    print 'Start queue for %i items in %i threads, master PID=%i'(len(queue), copies, os.getpid())
    if calc._racs_params['_daemonize']: set_output()
    for q in queue:
        if len(pids)==copies:
            p = os.waitpid(-1, 0)[0]
            pids.remove(p)
        calc._args_from_racs += q #+[('master', os.getpid())]
        pid = os.fork()
        if not pid: break
        pids.append(pid)
    else:
        while(pids): pids.remove(os.waitpid(-1, 0)[0])
        sys.exit()
#-------------------------------------------------------------------------------
__all__ = ['Calc']
