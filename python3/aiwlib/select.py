# -*- coding: utf-8 -*-
'''Copyright (C) 2003-2017, 2023 Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''
import os, sys, time, gzip, pickle, fnmatch, math
from . import mixt, gtable, calc, chrono
from functools import reduce
#-------------------------------------------------------------------------------
def parse(ev): 
    '''parse('ev![!] либо [~|^][title=][$]ev[?|%|#][-] либо tag+') возвращает кортеж csfh из 4-х значений
    compile(ev, ev, 'eval'|'exec') --- объект кода, в поле co_filename содежится выражение или заголовок
    sort=-1|0|1 --- режим сортировки (обратная, отсутсвует, прямая)
    fltr=yes(1)|no(0)|linebreak(-1)|distinct(-2) --- режим фильтрации
    hide=1|0 --- скрывать/показывать результат (колонку выборки)

    Параметры кортежа задаются следующим образом:
    ~ev ==> sort=1 (по возрастанию), ^ev ==> sort=-1 (по убыванию), else sort=0
      ? ==> fltr=1, фильтровать по результату, по умолчанию скрывать результат
      % ==> fltr=-1, добавлять пустую строку (горизонтальную линию в таблицу) при изменении значения выражения
      # ==> fltr=-2, убирать строки с одинаковыми значениями выражения кроме первого вхождения (аналог функции DISTINCT в SQL)
      ! ==> выполнять выражение функцией exec для расчета, скрывать результат (колонку выборки)
     !! ==> выполнять выражение функцией exec в глобальном пространстве имен, скрывать результат (колонку выборки)
      + ==> добавить тэги (разделенные запятой), скрывать результат (колонку выборки)
      - ==> hide=1|0 --- скрывать/показывать результат (колонку выборки) 
    Как альтернатива $... допускается использовать в выражении expr конструкции вида
    ...[/shell-command/]..., при этом shell-command форматируется по словарю расчета
    запускается и заменяется результатом выполнения в виде строки без \\n.
    '''
    if not type(ev) is str: raise Exception('type of parse argument=%r must be str, not %r'%(ev, type(ev)))
    if ev[-1]=='+': return ev, 0, 0, 1
    sort, fltr, hide, evex = 0, 0, None, 0
    while ev.endswith('!'): ev, hide, evex = ev[:-1], 1, evex+1
    if not evex:
        if ev[0] in '~^': sort, ev =  1-'~^'.index(ev[0])*2, ev[1:]
        if ev[-1] in '+-': ev, hide = ev[:-1], ev[-1]=='-'
        if ev[-1] in '?%#': ev, hide, fltr = ev[:-1], (hide, ev[-1]=='?')[hide==None], 1-'? %#'.index(ev[-1])
        title, ev = ev.split('=', 1) if mixt.is_name_eq(ev) else (ev, ev)
    if ev[0]!='$' and ev.count('[/')==ev.count('/]'): 
        ev = ev.replace('[/', '(" ".join([l.strip() for l in os.popen("""').replace('/]', '"""%self).readlines()]))')
    return (ev+'!'*evex if ev.startswith('$') and evex else
            compile('(" ".join(l.strip() for l in os.popen(%r%%self).readlines()))'%ev[1:] if ev.startswith('$') and not evex else ev, 
                    ev+'!'*evex if evex else title, 'exec' if evex else 'eval')), sort, fltr, hide
#-------------------------------------------------------------------------------
class SelCalc(calc.Calc):
    def __init__(self, path, D): self.__dict__.update(D); self.__dict__['path'] = path
#-------------------------------------------------------------------------------
class Select:
    'Построение выборки по базе RACS'
    _i, autocommit = 0, False # счетчик строки, автоматическое сохранение изменений ???
    def __init__(self, fromL=['./'], # список обрабатываемых репозиториев
                 ev_list=[],         # список выражений
                 progressbar=None,   # экземляр класса ProgressBar
                 c_size=False,       # автоматически определять суммарный размер выборки
                 check_tree=True,    # опускаться вниз по дереву каталогов
                 table=None):        # готовая таблица значений (отменяет обход репозиториев)
        self._L, self.c_size, self.c_runtime, self.progressbar, self.ring = [], c_size, 0, progressbar, []
        if not fromL and table is None: self._L, self.head, self._ev_list = [], [], []; return

        ev_list, ev_ring = [e for e in ev_list if e[-1]!=':'], [e[:-1] for e in ev_list if e[-1]==':']
        if ev_ring:
            ring_D = {} # { кортеж-значений-параметров: ячейка-в-ring }  и текущая ячейка в ring
            ring_keys = [(e.split('=', 1)[0] if mixt.is_name_eq(e) else e) for e in ev_ring]
            ring_expr = [(e.split('=', 1)[1] if mixt.is_name_eq(e) else e) for e in ev_ring]
                        
        fromL, starttime, csfhL = ([fromL] if type(fromL)==str else list(fromL)), time.time(), list(map(parse, ev_list))
        self.head, self._ev_list = [c[0].co_filename for c in csfhL if not c[3]], [c for c in csfhL if not c[3]]
        if self.progressbar: self.progressbar.clean()
        old_ = calc._G.get('_'); calc._G['_'] = self
	#-----------------------------------------------------------------------
        def vizit(dirname, start, part, repo):
            if dirname[-1]!='/': dirname += '/'
            LL = list(filter(os.path.isdir, list(map(dirname.__add__, os.listdir(dirname)))))
            if LL: part /= len(LL)
            #-------------------------------------------------------------------
            #   cache work
            #-------------------------------------------------------------------
            cache_name = dirname+'.RACS-CACHE'
            try: cache, cache_mtime = pickle.load(open(cache_name, 'rb'), encoding='bytes'), os.path.getmtime(cache_name)
            except: cache, cache_mtime = {}, 0; print(dirname, '--- cache corrupted!', file=sys.stderr)
            CL, KL = set(map(os.path.basename, LL)), set(cache.keys())
            for c in KL-CL: del cache[c]
            cache_delta = list(CL-KL|set([os.path.basename(p) for p in LL if os.path.getmtime(p)>cache_mtime and os.path.exists(p+'/.RACS')
                                          and os.path.getmtime(p+'/.RACS')>cache_mtime])) if cache else list(map(os.path.basename, LL))
            cache_refresh = False
            if cache_delta: part /= 2; cache_part = part/len(cache_delta)
            for c in sorted(cache_delta):
                try: 
                    X = pickle.load(open(dirname+c+'/.RACS', 'rb'), encoding='bytes'); cache_refresh = True
                    cache[c] = dict((k.decode() if type(k) is bytes else k, v.decode() if type(v) is bytes else v) for k, v in X.items())
                except Exception as exception: print(c, exception) 
                start += cache_part
                if self.progressbar: self.progressbar.out(start, 'refresh cache '+dirname+' ')
            if cache_refresh or not cache:
                try: pickle.dump(cache, open(cache_name, 'wb')) # after commit ???
                except Exception as exception: print(dirname, '--- cache not dumped!', exception, file=sys.stderr)
            #-------------------------------------------------------------------
            #   main cicle
            #-------------------------------------------------------------------
            for p in sorted(LL):
                D = cache.get(os.path.basename(p))
                if D:
                    R = SelCalc(p+'/', D); R.__dict__['_repo'] = repo
                    if ev_ring: self._L = ring_D.setdefault(tuple(map(R, ring_expr)), [])
                    l = [R]; self._L.append(l); Select._i += 1
                    for c, s, f, h in csfhL:
                        l.append(R(c))
                        if s: m, p = math.frexp(l[-1]); l[-1] = math.ldexp(int(m*1e7+(.5 if m>0 else -.5 if m<0 else 0))/1e7, p)
                        if f==1 and not l[-1]: self._L.pop(-1); Select._i -= 1; break
                elif check_tree: vizit(p, start, part, repo)
                start += part
                if self.progressbar: self.progressbar.out(start, dirname+' ')
            return start
	#-----------------------------------------------------------------------
        start, self.fromL = 0., list(filter(os.path.isdir, fromL))
        calc.Calc._except_report_table.extend(['repository "%s" not found\n'%r for r in fromL if not os.path.isdir(r)])
        if table is None:
            for repository in self.fromL: start = vizit(repository, start, 1./len(self.fromL), repository)
        else:
            for D in table:
                R = SelCalc(None, D); R.__dict__['_repo'] = None
                if ev_ring: self._L = ring_D.setdefault(tuple(map(R, ring_expr)), [])
                l = [R]; self._L.append(l); Select._i += 1
                for c, s, f, h in csfhL:
                    l.append(R(c))
                    if s: m, p = math.frexp(l[-1]); l[-1] = math.ldexp(int(m*1e7+(.5 if m>0 else -.5 if m<0 else 0))/1e7, p)
                    if f==1 and not l[-1]: self._L.pop(-1); Select._i -= 1; break                

        if ev_ring: self.ring_keys = ring_keys
        else: ring_D = { (): self._L }
        for rk, L in sorted(ring_D.items()):
            _after_calc(L, list(csfhL)); Select._i = 0; self._L = L; self._recalc_ts()
            U = dict(self._L[0][0].__dict__) if self._L else {} # определение совпадающих параметров в выборке
            for l in [_f for _f in self._L[1:] if _f]:
                for k, v in list(U.items()):
                    if not k in l[0].__dict__ or l[0][k]!=v: del U[k]
                if not U: break
            if ev_ring: U.update(list(zip(ring_keys, rk)))
            self.upar = SelCalc(None, U) # поддержка в delslice и т.д.?
            if ev_ring: self.ring.append([self._L, list(self.head), self.upar, self.c_size, self.c_runtime])
            self.ring_pos = len(self.ring)-1     
            
        if old_: calc._G['_'] = old_
        else: del calc._G['_']
        if self.progressbar: self.progressbar.close('select ')
        self.runtime = time.time()-starttime
    #---------------------------------------------------------------------------
    def __len__(self): return len([_f for _f in self._L if _f]) #???
    def nodes(self, begin=None, end=None): 'возвращает список расчетов'; return [ l[0] for l in self._L[begin:end] if l ]
    def _recalc_ts(self, calc_size=False): #recalc c_runtime and c_size
        if calc_size or self.c_size: self.c_size = sum([ l[0].get_size() for l in self._L if l ])
        self.c_runtime = sum([ getattr(l[0], 'runtime', 0) for l in self._L if l ])        
        # print([ getattr(l[0], 'runtime', 0) for l in self._L if l ][0].__dict__)
    def click(self):
        'поворот кольца выборок (если есть)'
        if self.ring:
            self.ring[self.ring_pos][:] = self._L, self.head, self.upar, self.c_size, self.c_runtime
            self.ring_pos = (self.ring_pos+1)%len(self.ring)
            self._L, self.head, self.upar, self.c_size, self.c_runtime = self.ring[self.ring_pos]
    def report(self):        
        rt = sum(l[-1] for l in self.ring) if len(self.ring)>1 else self.c_runtime
        sz = sum(l[-2] for l in self.ring) if len(self.ring)>1 else self.c_size
        ln = sum(len([_f for _f in l[0] if _f]) for l in self.ring) if len(self.ring)>1 else len(self)
        return 'selected %i items [%s]: %s total'%(ln, mixt.time2string(self.runtime), (mixt.size2string(sz)+', ' if sz else '')+mixt.time2string(rt))
    #---------------------------------------------------------------------------
    def astable(self, head=True, tw=None, colored=True): 
        'вернуть выборку в виде таблицы (как список строк)'
        colors = {'None':'1;33', 'nan':'1;33', 'stopped':'1;31', 'killed':'1;7;31', 'started':'1;33'}
        conv2str = lambda x:'\033[%sm%s\033[m'%(colors[x.strip()], x) if x.strip() in colors else x
        return mixt.table2strlist([None, self.head]*head+[None]+[l[1:] if l else None for l in self._L]+[None], 
                                  max_len=tw, conv2str=(conv2str if colored else str))
    def __str__(self): return ''.join(self.astable())
    def asdata(self, head=True, patt='', fname=None): 
        'вернуть (как список строк) или записать выборку, в формате .dat-файла'
        if type(patt) is str: patt = patt.split()
        D = reduce(lambda D, C: dict([(k, D.get(k, set())|set([v])) for k, v in list(C.par_dict().items())
#                                      if hashable(v) and mixt.compare(k, patt) ]), self.nodes(), {}) if patt else {}
                                      if mixt.compare(k, patt) ]), self.nodes(), {}) if patt else {}
        H = [ h.strip().split('\n') for h in self.head ]
        if min(list(map(len, H)))>1: # была проведена кластеризация
            X = {}; [X.setdefault(h[1], len(X)) for h in H[1:]]
            HL = [H[0][0]]+[h[0]+str(X[h[1]]) for h in H[1:]]
        else: HL = [h[0] for h in H]
        R = ['#: %s = %r\n'%(k, list(S)[0]) for k, S in list(D.items()) if len(S)==1] + mixt.table2strlist( 
            [['#:'+HL[0]]+HL[1:]]*bool(head)+[ l[1:] if l else ['']*len(self.head) for l in self._L ],
            'l'*len(self.head))
        R = [l.lstrip(' ') for l in R]
        if min(list(map(len, H)))>1:  # была проведена кластеризация
            p_in = max(i for i, l in enumerate(R) if l and l.startswith('#:')) # последняя строка заголовка
            if len(X)==len(H)-1: R[p_in+1:p_in+1] = ['#: %s.txt = "%s=%s"\n'%(h[0]+str(X[h[1]]), H[0][1], h[1]) for h in H[1:]]
            else: R[p_in+1:p_in+1] = ['#: %s.txt = "%s, %s=%s"\n'%(h[0]+str(X[h[1]]), h[0], H[0][1], h[1]) for h in H[1:]]
        if len(self.ring)>1: R[0:0] = [ '#:%s = %r\n'%(k, self.upar.__dict__[k]) for k in self.ring_keys ]
        if fname:
            fname %= self.upar; dname = os.path.dirname(fname)
            if dname and not os.path.exists(dname): os.makedirs(dname)
            (gzip.open if fname.endswith('.gz') else open)(fname, 'w').writelines(R); return []
        else: return R
    def ascommands(self, *tail):
        tail = ' '+' '.join(map(str, tail))+'\n'
        cnv = lambda x: "@'%s'"%repr(tuple(x)).replace(' ','') if repr(x)[:3] in ('Vec', 'Ind') or type(x) in (list, tuple) else repr(x) 
        return [p[0].args[0]+''.join([' %s=%s'%(k, cnv(v)) for k, v in zip(self.head, p[1:])])+tail for p in self._L if p]
    def paths(self, patterns=['']):
        'Возвращает пути (к расчету или файлу), проверяя на их на существование'
        return sum([[l[0].path+p for p in patterns if os.path.exists(l[0].path+p)] for l in self._L if l], []) #and os.path.exists(l[0].path+fname)]
    def paths2py(self, patterns=['']):
        'Возвращает пути (к расчету или файлу) в формате списка Python, проверяя на их на существование'
        return repr(self.paths(patterns))
    #---------------------------------------------------------------------------
    def delone(self, path=None):
        from scipy.spatial import Delaunay
        points = [l[1:] for l in self._L if l]
        cells, edges = Delaunay(points), {} # ij: len
        # calc stat
        #angles = []
        for c in cells:
            tr = [points[i] for i in c]            
            S = (tr[1][0]-tr[0][0])*(tr[2][1]-tr[0][1]) - (tr[1][1]-tr[0][1])*(tr[2][0]-tr[0][0]) #a[0]*b[1]-a[1]*b[0]
            for i in (0, 1, 2):
                ij, l = tuple(sorted([c[i], c[(i+1)%3]])),  ((tr[i][0]-tr[(i+1)%3][0])**2 + (tr[i][1]-tr[(i+1)%3][1])**2)**.5
                if not ij in edges: edges[ij] = l
                #angles.append(acos(.5*S/l))
        edge_av = sum(edges.values())/len(edges)
        stat = '%i points, %i cells, edge_min=%g, edge_max=%g, edge_av=%g, edge_sigma=%g'%(
            len(points), len(cells), min(edges.values()), max(edges.values()), edge_av, (sum((e-edge_av)**2 for e in list(edges.values()))/len(edges))**.5)
        print(stat)
                
        if path:
            fname %= self.upar; dname = os.path.dirname(fname)
            if dname and not os.path.exists(dname): os.makedirs(dname)
            H = [h.strip().split('\n')[0] for h in self.head]
            R = mixt.table2strlist([['#:'+H[0]]+H[1:]] + points, 'l'*len(self.head))
            open(fname+'.dat', 'w').writelines([l.lstrip(' ') for l in R])
            open(fname+'.cells', 'w').writelines(['%i %i %i\n'%tuple(c) for c in cells])
            open(fname+'.edges', 'w').writelines(['%g %g\n%g %g\n\n\n'%(points[e[0]][0], points[e[0]][1], points[e[1]][0], points[e[1]][1]) for e in list(edges.keys())])
            open(fname+'.stat', 'w').write(stat+'\n')
    #---------------------------------------------------------------------------
    def Xcommit(self): 'сохраняет изменения в расчетах на диск'; [l[0].commit() for l in self._L if l]
    def Xremove(self):
        'удаляет все записи из выборки и из базы'
        starttime = time.time(); import shutil
        for p in [l[0].path for l in self._L if l]:
            try: shutil.rmtree(p)
            except Exception as e: print(e)
        del self._L[:]
        self.runtime = time.time()-starttime
    def Xcopy(self, repo, *patterns):
        '''копирует все записи выборки в repo разрешая конфликты имен, если паттерны заданы то проверка идет до первого совпадения, 
        для игнорирования паттерн должен начинаться со знака -'''
        if repo[-1]!='/': repo += '/'
        for c in [l[0] for l in self._L if l]:
            n0, R = os.path.basename(c.path[:-1]), repo%c
            if not os.path.exists(R): os.makedirs(R)
            n, i, L = n0, 1, os.listdir(R)
            while n in L: n = n0+'_d%i'%i; i += 1
            dst = R+n+'/'; os.mkdir(dst)
            if patterns:
                L = []
                for f in os.listdir(c.path):
                    for p in patterns:
                        if p.startswith('-') and fnmatch.fnmatch(f, p[1:]): break
                        if not p.startswith('-') and fnmatch.fnmatch(f, p): L.append(f); break
                if not '.RACS' in L: L.append('.RACS')
            else: L = os.listdir(c.path)
            for f in L: os.system('cp -r %r %r'%(c.path+f, dst))
    #---------------------------------------------------------------------------
    def get_keys(self, mode='or', *patterns):
        'список всех параметров привязанных к расчетам выборки, допустимые режимы "|", "&", "^", "or", "and", "xor"'
        starttime = time.time()
        if not self._L: return []
        keys = [set(l[0].par_dict('statelist', 'args', 'runtime', 'progress').keys()) for l in self._L if l]
        S = reduce(getattr(set, '__%s__'%{'|':'or', '&':'and', '^':'xor'}.get(mode, mode)), keys, keys.pop(0))
        if patterns: S = [n for n in S if mixt.compare(n, patterns)]
        self.runtime = time.time()-starttime
        return sorted(list(S))    
    def get_values_summary(self, *patterns):   #???
        'все значения выражений и ключей для расчетов выборки'
        D, i, starttime = {}, 0., time.time()
        if self.progressbar : self.progressbar.clean()
        for l in self._L :
            if not l : continue
#            print l[0]._path_dict[''][0]._path_dict
            for k, v in list(l[0].par_dict( ['args','path','runtime','progress','statelist'] ).items())  :
                if not k.startswith('@') and ( not self.head or compare( k, patterns ) ) : D.setdefault( k, set() ).add( v )
            for k, v in zip( self.head, l[1:] ) : 
                if not patterns or compare( k, patterns ) : D.setdefault( k, set() ).add( v )
            if self.progressbar : self.progressbar.out( i/len(self), 'get_values_summary ' ); i+=1
        D = dict(( ( k, sorted(v) ) for k, v in list(D.items()) ))
        self.runtime = time.time() - starttime
        if self.progressbar : self.progressbar.close( 'get_values_summary ' )
        return D
    def get_types_summary( self ) :  
        'все типы значений выражений и ключей для расчетов выборки'
        D, i, starttime = {}, 0., time.time()
        if self.progressbar : self.progressbar.clean()
        for l in self._L :
            if not l : continue
            for k, v in list(l[0].par_dict( ['args','path','runtime','progress','statelist'] ).items()) + list(zip( self.head, l[1:] )) :
                if k.startswith('@') : continue
                r = D.setdefault( k, [] )
                if v.__class__ not in r : r.append(v.__class__)
            if self.progressbar : self.progressbar.out( i/len(self), 'get_types_summary ' ); i+=1
#        for l in D.values() : l.sort()
        self.runtime = time.time() - starttime
        if self.progressbar : self.progressbar.close( 'get_types_summary ' )
        return D
    #---------------------------------------------------------------------------
    def clusteringXY(self):
        'разворачивает X-колонку горизонтально, кластеризуя значения по X и Y, в итоге получается 2D таблица'
        X, Y = {}, {}  # значения колонок X, Y в виде {значение: порядковый-номер-от-нуля} и число колонок
        v2k = lambda v: tuple(v) if v.__class__ in (set, list) else v
        for l in [_f for _f in self._L if _f]: X.setdefault(v2k(l[1]), len(X)); Y.setdefault(v2k(l[2]), len(Y))
        R = [ [None, None]+['nan']*len(X)*(len(self.head)-2) for i in range(len(Y)) ] # результирующая таблица
        for y, i in list(Y.items()): R[i][1] = y
        for l in [_f for _f in self._L if _f]:
            ix, iy = X[v2k(l[1])], Y[v2k(l[2])]
            for k, v in enumerate(l[3:]): R[iy][2+ix+k*len(X)] = v
        self.head[1] += '\n'+self.head[0]; del self.head[0]
        self.head[1:] = [ h+('\n%g'%x if type(x) is float else '\n%s'%x) for h in self.head[1:] for x in
                          [l[0] for l in sorted(list(X.items()), key=lambda k:k[1])] ]
        self._L = R        
    #---------------------------------------------------------------------------
    # ??? что то странное ???
    def __getitem__(self, i): 
        if type(i) is tuple: r, i = i[0], i[1]+Select._i; return self._L[i][r] if self._L[i] else None
        i += Select._i; return self._L[i][0] if self._L[i] else None
    def __setitem__( self, i, x ) : self._L[i[1]+Select._i][i[0]] = x
    def __delitem__( self, i ) : 
        if self._L[i] : 
            if self.c_size : self.c_size -= self._L[i][0].get_size() 
            self.c_runtime -= getattr(self._L[i][0], 'runtime', 0) 
            del self._L[i]
            if i and i<len(self._L) and not self._L[i-1] and not self._L[i] : del self._L[i] # double separator delete
    def __getslice__( self, begin, end ) : 
        S = Select(None); S.progress, S.runtime, S._L = self.progress, self.runtime, self._L[begin:end]
        S._recalc_ts(self.c_size); return S
    def __delslice__(self, begin, end): self._L[begin:end] = [None] if None in self._L[begin:end] else []; self._recalc_ts()
#-------------------------------------------------------------------------------
def _after_calc( L,         # выборка (список строк)
                 csfhL,     # список результатов работы ф-ии parse
                 shift=1,   # начальный столбец для обработки
                 sepS=[] ): # позиции горизонтальных сепараторов как id объектов Calc ПЕРЕД сепаратором
    'постобработка выборки --- сортировка, вставка сепараторов, выбрасывание лишних столбцов и строк'
    # SORTING
    for ie in [i for i in range(len(csfhL)) if csfhL[i][1]]: L.sort(key=lambda l: l[ie+1], reverse=(csfhL[ie][1]<0)) 

    # DISTINCT 
    distL, i = [ie for ie in range(len(csfhL)) if csfhL[ie][2]==-2], 1 
    if distL:
        while i<len(L):
            for ie in distL:
                if L[i][ie+shift]==L[i-1][ie+shift]: del L[i]; break
            else: i += 1

    # INSERT SEPARATORS
    sepL, i = [e for e in range(len(csfhL)) if csfhL[e][2]==-1], 1
    if sepL or sepS: 
        while i<len(L): 
            l1, l2 = L[i-1], L[i]
            if l1 and l2 and id(l1[0]) in sepS: L.insert(i, None); i += 1; continue 
            for ie in sepL:
                if l1 and l2 and l1[ie+shift]!=l2[ie+shift]: L.insert(i, None); i += 1; continue 
            i += 1

    # HIDE COLUMNS        
    hideL = [e for e in range(len(csfhL)-1, -1, -1) if csfhL[e][3]]
    if hideL:
        for l in L:
            if not l: continue
            for ie in hideL: del l[ie+shift]
#-------------------------------------------------------------------------------
