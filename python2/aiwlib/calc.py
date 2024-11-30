# -*- coding: utf-8 -*-
'''Copyright (C) 2002-2017, 2023 Anton V. Ivanov <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

import os, sys, time, cPickle, socket, shutil, json 
import aiwlib.mixt as mixt 
import aiwlib.chrono as chrono
try: from aiwlib.mpi4py import *
except ImportError, e: pass
#-------------------------------------------------------------------------------
#_is_swig_obj = lambda X: all([hasattr(X, a) for a in ('this', 'thisown', '__swig_getmethods__', '__swig_setmethods__')])
_is_swig_obj = lambda X: "<type 'SwigPyObject'>" in [str(type(X)), str(type(getattr(X, 'this', None)))]
_rtable, _G, ghelp = [], {}, [] # таблица для замыкания рекурсии, глобальная таблица и справка  
_ignore_list = 'path statelist runtime progress args md5sum _wrap _comments _profiler tags _starttime this thisown'.split()
_racs_params = {} # параметры RACS (репозиторий, демонизация расчета, символическая ссылка и т.д.)
_racs_cl_params = set() # имена параметров RACS заданные в командной строке 
_cl_args, _cl_tags = [], [] # аргументы командной строки не обработанные RACS и тэги из командной строки  
_args_from_racs = [] # значения параметров полученные из RACS при разборе аргументов командной строки
_arg_seqs, _arg_order, _queue =  {}, [], None # словарь с параметрами для пакетного запуска, последовательность имен параметров, очередь заданий
_help_mode = False
#-------------------------------------------------------------------------------
def _init_hook(self): pass
def _make_path_hook(self): 
    self.path = mixt.make_path(_racs_params['_repo']%self, _racs_params['_calc_num']) 
    return self.path
#-------------------------------------------------------------------------------
def _expr_preproc(expr):
    'обрабатывает $ и (%...%) в выражении'
    pass
#-------------------------------------------------------------------------------
class Calc:
    '''Работа с расчетом (записью в базе) --- создание уникальной директории расчета 
    и сохранение/восстановление параметров в файле .RACS'''
    #---------------------------------------------------------------------------
    def __init__(self, **D):
        self._comments, self._profiler, self._set_progress_time = {}, {}, time.time()
        self.runtime, self.progress, self.statelist, self.args, self._wraps, self.tags = chrono.Time(0.), 0., [], list(sys.argv), [], set(_cl_tags)
        for k, v in D.items(): # обработка аргументов конструктора
            if k in _racs_params and not k in _racs_cl_params: _racs_params[k] = v
            elif not k in _racs_params:
                if type(v) is tuple and len(v)==2 and type(v[1]) is str and v[1].startswith('#'): self._comments[k], v = ' '+v[1][1:], v[0]
                self.__dict__[k] = v
        if _help_mode: return
        #-----------------------------------------------------------------------
        #   серийный запуск и демонизация расчета
        #-----------------------------------------------------------------------
        global _args_from_racs
        if _racs_params.get('_mpi', 0)==3:
            if mpi_proc_number()==0: # головной процесс
                for q in reduce(lambda L, a: [l+[(a,x)] for x in _arg_seqs[a] for l in L], _arg_order, [[]]):
                    data, proc = mpi_recv(-1) # получаем запрос на задание
                    path = mixt.make_path(_racs_params['_repo']%self, _racs_params['_calc_num'])
                    mpi_send(_args_from_racs+q+[('path', path)], proc)
                else: 
                    for p in range(1, mpi_proc_count()): mpi_send(None, p)
                    mpi_finalize(); sys.exit()                
            else:
                while 1:
                    mpi_send((socket.gethostname(), mixt.get_login(), os.getpid()), 0)
                    _args_from_racs = mpi_recv(0)[0]
                    if not _args_from_racs: mpi_finalize(); sys.exit()
                    pid = os.fork()
                    if not pid: break
                    os.waitpid(-1, 0)
        elif _arg_seqs or _queue:
            _base_args_from_racs, t_start, n_start, n_finish, pid = list(_args_from_racs), time.time(), 0, 0, os.getpid()
            if _queue: queue = [q+[('racs_master', pid)] for q in _queue]  # декартово произведение с последовательностями параметров?
            elif _racs_params['_zip']: queue = [q+[('racs_master', pid)] for q in zip(*[[(a, x) for x in _arg_seqs[a]] for a in _arg_order])]
            else: queue = reduce(lambda L, a: [l+[(a,x)] for x in _arg_seqs[a] for l in L], _arg_order, [[('racs_master', pid)]])
            if _racs_params['_daemonize']: mixt.mk_daemon()
            if not os.path.exists('.racs'): os.mkdir('.racs')                
            if not os.path.exists('/tmp/racs'): os.system('mkdir /tmp/racs; chmod a+rwx /tmp/racs')
            stitle = _racs_params['_title'] if _racs_params.get('_title') else str(os.getpid())
            copies, pids, logfile, smode, lenQ = _racs_params['_copies'], [], '.racs/started-%s'%stitle, 'Running the queue', len(queue)
            if '_continue' in _racs_params:
                old_log, runs, irun, finishes = _racs_params['_continue'], {}, 0, []
                old_tasks = [l[:-1] for l in open(old_log) if not l.startswith('# ')]
                for i, p in [(j, l.split()[1]) for j, l in enumerate(old_tasks) if l[0]!='#']:
                    if p[0]=='+': runs[int(p[1:])] = (irun, i); irun += 1  # номер открытой задачи отвечает номеру задачи в очереди
                    else: finishes.append(runs.pop(int(p[1:]))[0])         # добавляем этот номер в список закрытых задач
                n_start = n_finish = len(finishes)
                for p in reversed(sorted(finishes)): del queue[p]
                #print finishes, runs, queue
                for p in runs.values(): old_tasks[p[1]] = '#>>>'+old_tasks[p[1]]
                old_tasks += ['#>>>%s -%s сlosed on continuation of the queue %r'%(chrono.Date(), p, stitle) for p in runs.keys()]
                if old_log.startswith('.racs/started-'):
                    os.rename(old_log, '.racs/stopped-'+old_log.split('-', 1)[1])
                    if os.path.exists(old_log+'.log'): os.rename(old_log+'.log', '.racs/stopped-'+old_log.split('-', 1)[1]+'.log')
                    for l in os.listdir('/tmp/racs/'):
                        if not os.path.exists('/tmp/racs/'+l): os.remove('/tmp/racs/'+l)
                smode = 'Continued the queue (original %i tasks)'%lenQ
                #del p, old_log, runs, irun, finishes
            print smode, 'of %i tasks in %i threads, master PID=%i, logfile="%s"'%(len(queue), copies, os.getpid(), logfile)
            streams, OUT = [sys.stdout, open(logfile, 'w')], lambda msg: [(s.write(msg+'\n'), s.flush()) for s in streams]
            symlink = '/tmp/racs/started-%s.%i'%(stitle, int(([0]+[s.rsplit('.', 1)[1] for s in os.listdir('/tmp/racs/')
                                                                   if s.startswith('started-%s.'%stitle) and s[len('started-%s.'%stitle):].isdigit()])[-1])+1)   
            os.symlink(os.path.abspath(logfile), symlink)
            if _racs_params['_daemonize']: mixt.set_output(logfile+'.log')
            OUT('# %s@%s:%s repo=%r tasks=%i threads=%i PID=%i\n# '%(mixt.get_login(), socket.gethostname(), os.getcwd(),
                                                                     _racs_params['_repo'], lenQ, copies, os.getpid())+' '.join(sys.argv))            
            for a in _arg_order: OUT('#   %s: %s'%(a, _arg_seqs[a]))
            finish_msg = lambda: OUT('%s -%i %g%% %s %s'%(chrono.Date(), p, 100.*n_finish/lenQ, mixt.time2string(time.time()-t_start),
                                                          mixt.time2string((time.time()-t_start)*lenQ/n_finish)))
            if '_continue' in _racs_params: map(OUT, old_tasks)
            for q in queue:
                if len(pids)==copies:
                    p = os.waitpid(-1, 0)[0];  
                    pids.remove(p); n_finish += 1; time.sleep(1); finish_msg() #<<< for lock append finish_msg on clusters?
                _args_from_racs = _base_args_from_racs+q #+[('master', os.getpid())]
                pid = os.fork()
                if not pid: break
                pids.append(pid)
                n_start += 1
                OUT('%s +%i %g%% %s'%(chrono.Date(), pid, 100.*n_start/lenQ, ' '.join('%s=%r'%i for i in q[1:])))
            else:
                while(pids): p = os.waitpid(-1, 0)[0]; pids.remove(p); n_finish += 1; time.sleep(1); finish_msg() #<<< for lock append finish_msg on clusters?
                if _racs_params['_daemonize']: streams[0].close(); os.rename(logfile+'.log', '.racs/finished-%s.log'%stitle)
                streams[1].close(); os.rename(logfile, '.racs/finished-%s'%stitle)
                os.system('rm -f '+symlink)
                sys.exit()
        elif _racs_params.get('_daemonize', False): mixt.mk_daemon()
        #-----------------------------------------------------------------------
        if 'path' in dict(_args_from_racs): self.__dict__['path'] = dict(_args_from_racs)['path']
        if 'path' in self.__dict__: # подготовка пути
            self.path = mixt.normpath(self.path)
            if self.path[-1]!='/': self.path += '/'
            if os.path.exists(self.path+'.RACS'): self.__dict__.update(cPickle.load(open(self.path+'.RACS')))
        for k, v in _args_from_racs: # накат сторонних параметров            
            if k in self.__dict__: v = mixt.string2bool(v) if type(self.__dict__[k]) is bool else self.__dict__[k].__class__(v)
            self.__dict__[k] = v
        _init_hook(self)
    # def __repr__(self): return 'RACS(%r)'%self.path 
    # def __str__(self): return '@'+self.path #???
    #---------------------------------------------------------------------------
    #def __setattr__(self, k, v): # monkey patch
    #    if not k in self._init_keys and k in _args_from_racs:
    #        v_cl  = _args_from_racs.pop(k)
    #        self.__dict__[k] = mixt.string2bool(v_cl) if type(v) is bool else v.__class__(v_cl)
    #    else: self.__dict__[k] = v
    #---------------------------------------------------------------------------
    def par_dict(self, *ignore_list): 
        return dict([(k, v) for k, v in self.__dict__.items() if k[0]!='_' and not k in _ignore_list+list(ignore_list)])
    #---------------------------------------------------------------------------
    def __getattr__(self, attr):
        'нужен для создания уникальной директории расчета по первому требованию (ленивые вычисления)'
        if attr=='path':
            if _help_mode:
                print '\nAvailable options:'
                mpl = max(len(k) for k in self._comments.keys())
                for k, v in sorted(self._comments.items()): print k, ' '*(mpl-len(k)), v
                exit()
            return _make_path_hook(self)
        raise AttributeError(attr)
    #---------------------------------------------------------------------------
    def commit(self): 
        'Сохраняет содержимое расчета в базе'
        #print dict(filter(lambda i:i[0][0]!='_' and i[0]!='path', self.__dict__.items())).keys()
        if os.path.exists(self.path+'.RACS'): os.remove(self.path+'.RACS') # ??? for update mtime of self.path ???
        data = dict(filter(lambda i:(i[0][0]!='_' and i[0]!='path') or i[0] in ('_profiler', '_lambda'), self.__dict__.items()))
        try: cPickle.dump(data,  open(self.path+'.RACS', 'w'))
        except:
            for k, v in data.items():
                try: cPickle.dumps(v)
                except Excepstion as e: print e, '--- %s skipped'%k; del data[k]
            cPickle.dump(data,  open(self.path+'.RACS', 'w'))
        if self.__dict__.get('_comments') and not os.path.exists(self.path+'/.comments'):
            json.dump(self._comments, open(self.path+'/.comments', 'w'), indent=2, sort_keys=1, ensure_ascii=0)
        os.utime(self.path, None); self._old_progress = self.progress # for racs cache refresh
        if _racs_params.get('_mpi', -1)==2 and mpi_proc_number()==0: shutil.copyfile(self.path+'.RACS', self.path.rsplit('/', 2)[0]+'/.RACS')
    #---------------------------------------------------------------------------
    def add_state(self, state, info=None, host=socket.gethostname(), login=mixt.get_login()):
        'Устанавливает статус расчета, НЕ вызывает commit()'
        import mixt, chrono 
        if not state in ('waited','activated','started','finished','stopped','suspended'):
            raise Exception('unknown status "%s" for "%s"'%(state, self.path if hasattr(self, 'path') else '???'))
        if info==None and state=='started': info = os.getpid()
        if info==None and state=='stopped': info = ''.join(mixt.except_report(None))
        if not hasattr(self, 'statelist'): self.statelist = []
        self.statelist.append((state, login, host, chrono.Date())+((info,) if info!=None else ()))
    def set_state(self, state, info=None, host=socket.gethostname(), login=mixt.get_login()):
        'Устанавливает статус расчета, вызывает commit()'
        self.add_state(state, info, host, login); self.commit()
    #---------------------------------------------------------------------------
    def set_progress(self, progress, prompt='',  runtime=-1):
        'Устанавливает progress и runtime, выводит по возможности progressbar'
        if not hasattr(self, 'statelist'): self.statelist = []
        runtime = (chrono.Date()-self.statelist[-1][3] if self.statelist else 0.) if runtime<0 else chrono.Time(runtime)
        self.__dict__['progress'], self.__dict__['runtime'] = progress, runtime
        if  time.time()-self._set_progress_time>.2:
            self._set_progress_time = time.time()
            if os.path.exists(self.path+'.RACS'): self.commit()
            if os.isatty(sys.stdout.fileno()):
                width = int(os.popen('stty size 2> /dev/null').readline().split()[1])
                progress = max(0, min(1, progress)); plen, text = int(width*progress+.5), '[ '+prompt+str(runtime)
                text += '/%s ETA %s'%(runtime/progress, runtime/progress-runtime) if progress else '/???'
                for t, c, n in reversed(sorted(v+[k] for k, v in self._profiler.items())):
                    c1, c2 = str(c), ('%.2g'%c).replace('+0', '').replace('+', '')
                    a = ' %s:%s(%i%%)'%(n, (c1 if len(c1)<=len(c2) else c2), 100*t/float(runtime))
                    if len(text+a)<width: text += a
                    else: break
                text += ' '*(width-len(text)-1)+']'
                sys.stdout.write('\r\033[7m'+text[:plen]+'\033[0m'+text[plen:]); sys.stdout.flush()
    #---------------------------------------------------------------------------
    # def commit_sources( self, *L ) : 
    #    'Сохраняет файлы с исходным кодом программы в базе'
    #    self.md5sources = sources.commit( self.path+'.sources.tar.gz', self.path+'.bzr_status', *L )
    #---------------------------------------------------------------------------
    def get_size(self): 'Размер расчета в байтах'; return int(os.popen("du -bs "+self.path).readline().split()[0])
    #---------------------------------------------------------------------------
    def push(self, X, ignore_list=[], _prefix='', **kw_args):
        '''устанавливает аттрибуты объекта X согласно объекту расчета, 
        параметры расчета имеют более высокий приоритет, чем параметры kw_args'''
        ignore_list = _ignore_list+(ignore_list.split() if type(ignore_list) is str else ignore_list)+['this', 'thisown']
        params = self.par_dict(*ignore_list).items()
        if _prefix: params = filter(lambda i: i[0].startswith(_prefix), params)
        if type(X) is dict: X.update(kw_args); X.update(params)
        elif hasattr(X, '__swig_setmethods__'): 
            for k, v in kw_args.items()+filter(lambda i: i[0] in X.__swig_setmethods__, params): setattr(X, k, v)
        else: 
            for k, v in kw_args.items()+params: setattr(X, k, v) 
    #---------------------------------------------------------------------------
    def pull(self, X, ignore_list=[], _prefix='', **kw_args):
        '''устанавливает аттрибуты объекта расчета согласно объекту X, 
        параметры kw_args имеют более высокий приоритет, чем параметры расчета
        автоматически устанавливаются аттрибуты имеющие методы __get/setstate__ 
        (но не имеющие аттрибута _racs_pull_lock) или не-являющиеся объектами swig'''
        ignore_list = _ignore_list+(ignore_list.split() if type(ignore_list) is str else ignore_list)+['this','thisown']
        if type(X) is dict: 
            for k, v in X.items():
                if not k in ignore_list and type(v) in (int,float,long,bool): self[k] = v
        else:
            for k in X.__dict__.keys()+getattr(X, '__swig_getmethods__', {}).keys()+filter(lambda k: type(getattr(X.__class__, k, None)) is property, dir(X)):
                if not k in ignore_list+['__doc__']: 
                    v = getattr(X, k)
                    if all([hasattr(v, '__%setstate__'%a) for a in 'gs']+
                           [not hasattr(v, '_racs_pull_lock') and not hasattr(v, '__racs_pull_denied__')]
                           ) or not _is_swig_obj(v): self[_prefix+k] = v
        for k, v in kw_args.items(): self[k] = v
    #---------------------------------------------------------------------------
    _except_report_table = []
    def __call__(self, expr):
        #name = getattr(expr, 'co_filename', expr) 
        try:
            if type(expr) is str: 
                if expr.startswith('@'): k, v = expr.split('=', 1); self[k] = v
                elif expr.startswith('$'):
                    if expr[-1]!='!': return ' '.join(os.popen(expr[1:]%self).readlines()).strip()
                    else:
                        expr, G, L = (expr[1:-2], dict(self.__dict__), _G) if expr.endswith('!!') else (expr[1:-1], _G, self)
                        exec(' '.join(os.popen(expr%self).readlines()).strip(), G, L); return
                elif expr.endswith('!!'): exec(expr[:-2], dict(self.__dict__), _G)  
                elif expr.endswith('!'): exec(expr[:-1], _G, self) 
                elif expr.endswith('+'): self.__dict__.setdefault('tags', set()).add(expr[:-1]) 
                else: return eval(expr, _G, self) 
            else: return eval(expr, dict(self.__dict__), _G) if expr.co_filename.endswith('!!') else eval(expr, _G, self)
        except Exception, e: 
            if _G['on_racs_call_error']==0: raise
            elif _G['on_racs_call_error'] in (1, 2): 
                report = ''.join(mixt.except_report(None, short=_G['on_racs_call_error']-1))
                if not report in self._except_report_table: self._except_report_table.append(report)
    #---------------------------------------------------------------------------
    #def __getitem__(self, arg):
    #    'для форматирования строки'
    #    if '?' in arg:
    #        flag, arg = arg.split('?', 1)
    #        return arg%self if eval(flag, dict(math.__dict__), self.__dict__) else ''
    #    auto = arg.endswith('=')
    #    if auto: arg = arg[:-1]
    #    val = eval(arg, dict(math.__dict__), self.__dict__)
    #    if auto and val.__class__==bool: return ',%s'%arg if val else ''
    #    if isinstance(val, float): val = '%g'%val
    #    return ',%s=%s'%(arg, val) if auto else val
    def __getitem__(self, key):
        if key=='self': return self
        if key in self.__dict__: return self.__dict__[key]
        if key in self.__dict__.get('tags', []): return True
        if key in _G or key in __builtins__: raise KeyError(key)        
        ak = '@'+key; c = key if not key.replace('_','').isalnum() else self.__dict__[ak] if ak in self.__dict__ else _G.get(ak)

        if key and not (key[0]=='_' or key[0].isalpha() and key.replace('_','0').isalnum()): return self(key)
        if c!= None:
            if key in _rtable: print>>sys.stderr, 'For "%s" recursion cropped'%key; return
            try: _rtable.append(key); return self(c)
            finally: _rtable.pop()
        if key=='runtime': return Time(0.) #???
	if key=='statelist': return
        if key and key[0]=='_'  and key[-1]=='_' and key[1].isalpha() and key.replace('_','0').isalnum():
            return key[1:-1] in self.__dict__ or key[1:-1] in self.__dict__.get('tags', [])
        #for r, a in _getitem_rules: 
        #    if r(key, self): return a(key, self)
        report = 'KeyError: %r is not defined\n'%key
        if not report in self._except_report_table: self._except_report_table.append(report)
        #raise KeyError(key)        
    #---------------------------------------------------------------------------
    def __setitem__(self, key, val): self.__dict__[key] = val # блокировать доступ к statelist и пр???
    def __delitem__(self, key): del self.__dict__[key]
    def get(self, name, value=None): return self[name] if name in self.__dict__ else value
    def __contains__(self, key): return key in self.__dict__
    def wrap(self, core, prefix='', progress=None): return _Wrap(self, core, prefix, progress)
    #---------------------------------------------------------------------------
#    def __setattr__(self, attr, value):
#        if attr in self.__dict__: self.__dict__[attr] = value.__class__(self.__dict__[attr])
#        else: self.__dict__[attr] = value
#-------------------------------------------------------------------------------
_set_par_out = [1]
class _Wrap:
    _all_wrap = True
    def __init__(self, calc, core, prefix, progress):
        self.__dict__.update([('_calc', calc), ('_core', core), ('_prefix', prefix), ('_progress', progress), ('_set_attrs', set())])
        if hasattr(core, 'this'): self.__dict__['this'] = core.this # easy link to SWIG class O_O!
        if _racs_params['_auto_pull']: calc._wraps.append(self) # for exit hook
        for k, p in filter(lambda kp: not kp[0] in _ignore_list and isinstance(kp[1], property), [(k, getattr(core.__class__, k, None)) for k in dir(core)]):
            if p.__doc__: calc._comments[prefix+k] = p.__doc__
            if not _help_mode and prefix+k in self._calc.__dict__: setattr(self, k, calc.__dict__[self._prefix+k]); self._set_attrs.add(k)
    def __getattr__(self, attr):
        res = getattr(self._core, attr)
        #if isinstance(getattr(self._core.__class__, attr), property): return res
        #elif callable(res): return _WrapAttr(res, self._prefix+attr, self)
        if callable(res) or hasattr(res, 'this'): return _WrapAttr(res, self._prefix+attr, self)
        return res
    def __setattr__(self, attr, value):
        if attr in self._set_attrs:
            print('\033[7mset parameter %r to %r is overlapped by %r from RACS\033[0m'%(attr, value, self._calc.__dict__[self._prefix+attr]))
            return 
        if getattr(self._core, attr).__class__ is bool and type(value) is str: value = mixt.string2bool(value)
        else:
            dst = getattr(self._core, attr); dst_t = dst.__class__               
            if getattr(dst, '_is_aiwlib_vec', False): value = dst_t(value, D=dst._D(), T=dst._T())
            elif type(value) is str and dst_t is int: value = dst_t(float(value))
            elif type(value) is tuple:
                try: value = dst_t(value)
                except: value = dst_t(*value)
            else: value = dst_t(value)        
        self._calc.__dict__[attr] = value
        if _set_par_out[0]: print('set parameter %r to %r'%(attr, value))
        setattr(self._core, attr, value)
#-------------------------------------------------------------------------------
class _WrapAttr:
    _progress_errors = []
    def __init__(self, core, name, wrap): self._core, self._name, self._wrap = core, name, wrap
    def __getattr__(self, attr):
        res = getattr(self._core, attr)
        if _Wrap._all_wrap and (callable(res) or hasattr(res, 'this')): return _WrapAttr(res, self._name+'.'+attr, self._wrap)
        return res
    def __call__(self, *args, **kw_args):
        try:
            all_wrap, _Wrap._all_wrap = _Wrap._all_wrap, False
            t0 = time.time();  res = self._core(*args, **kw_args); _set_par_out[0] = 0 
            pf = self._wrap._calc._profiler.setdefault(self._name, [0, 0]);  pf[0] += time.time()-t0; pf[1] += 1   # суммарное время работы и число вызовов
            if self._wrap._progress:
                try: self._wrap._calc.set_progress(self._wrap._progress())
                except Exception as E:
                    E = str(E)
                    if not E in self._progress_errors: print 'set progress in %s failed:'%self._name, E; self._progress_errors.append(E)
            return res
        finally: _Wrap._all_wrap = all_wrap
#-------------------------------------------------------------------------------
__all__ = ['Calc']
