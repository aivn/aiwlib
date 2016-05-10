# -*- coding: utf-8 -*-
import calc, mixt, gzip
#-------------------------------------------------------------------------------
def parse(ev): 
    '''parse('ev![!] либо [~|^][title=][$]ev[?|%|#][+|-]') возвращает кортеж csfh из 4-х значений
    compile(ev, ev, 'eval'|'exec') --- объект кода, в поле co_filename содежится выражение или заголовок
    sort=-1|0|1 --- режим сортировки (обратная, отсутсвует, прямая)
    fltr=yes(1)|no(0)|linebreak(-1)|distinct(-2) --- режим фильтрации
    hide=1|0 --- скрывать/показывать результат (колонку выборки)

    Параметры кортежа задаются следующим образом:
    ~ev ==> sort=1 (по возрастанию), ^ev ==> sort=-1 (по убыванию), else sort=0
    ?   ==> fltr=1, фильтровать по результату, по умолчанию скрывать результат
    %   ==> fltr=-1, добавлять пустую строку (горизонтальную линию в таблицу) при изменении значения выражения
    #   ==> fltr=-2, убирать строки с одинаковыми значениями выражения кроме первого вхождения (аналог функции DISTINCT в SQL)
    !   ==> выполнять выражение функцией exec для расчета, скрывать результат (колонку выборки)
    !!  ==> выполнять выражение функцией exec в глобальном пространстве имен, скрывать результат (колонку выборки)
    -|+ ==> hide=1|0 --- скрывать/показывать результат (колонку выборки) 
    '''
    if not type(ev) is str: raise Exception('type of parse argument=%r must be str, not %r'%(ev, type(ev)))
    sort, fltr, hide, evex = 0, 0, None, 0
    while ev.endswith('!'): ev, hide, evex = ev[:-1], 1, evex+1
    else:
        if ev[0] in '~^': sort, ev =  1-'~^'.index(ev[0])*2, ev[1:]
        if ev[-1] in '+-': ev, hide = ev[:-1], ev[-1]=='-'
        if ev[-1] in '?%#': ev, hide, fltr = ev[:-1], (hide, ev[-1]=='?')[hide==None], 1-'? %#'.index(ev[-1])
        title, ev = ev.split('=', 1) if mixt.is_name_eq(ev) else (ev, ev)
    return compile('" ".join(os.popen("%s"%%self).readlines()).strip()'%ev[1:] if ev.startswith('$') and not evex else ev, 
                   ev+'!'*evex if evex else title, 'exec' if evex else 'eval'), sort, fltr, hide
#-------------------------------------------------------------------------------
class Select:
    'Построение выборки по базе RACS'
    _i, autocommit = 0, False # счетчик строки, автоматическое сохранение изменений ???
    def __init__( self, fromL=['./'], # список обрабатываемых репозиториев
                  ev_list=[],         # список выражений
                  progressbar=None,   # экземляр класса ProgressBar
                  c_size=False,       # автоматически определять суммарный размер выборки
                  check_tree=True ):  # опускаться вниз по дереву каталогов
        self._L, self._ev_list, self.c_size, self.c_runtime, self.progressbar = [], [], c_size, 0, progressbar
        if not fromL: self._L, self.head, self._ev_list = [], [], []; return
        self.fromL, starttime, cshfL = [fromL] if type(fromL)==str else list(fromL), time.time(), map(parse, ev_list)
        self.head, self._ev_list = [c[0].co_filename for c in csfhL if not c[3]], [c for c in csfhL if not c[3]]        
        if self.progressbar: self.progressbar.clean()
        old_ = calc._G.get('_'); cal._G['_'] = self
	#-----------------------------------------------------------------------
        def vizit(dirname, start, part):
            # if read_cache : READ_CACHE( dirname ) #???
            LL = filter(os.path.isdir, [os.path.join(dirname, p) for p in os.listdir(dirname)])
            if LL: part /= len(LL)
            for p in sorted(LL):
                if os.path.exists(os.path.join(dirname, p, '.RACS')):
                    R = calc.Calc(path=os.path.join(dirname, p)) # try?
                    l = [R]; self._L.append(l); Select._i += 1
                    # R.__dict__['rpath'], R.__dict__['repo'] = R.path[len(repository):], repository # ???
                    for c, s, f, h in csfhL:
                        l.append(R(c))
                        if f==1 and not l[-1]: self._L.pop(-1); SELECT._i -= 1; break
                elif check_tree: vizit(p, start, part)
                start += part
                if self.progressbar: self.progressbar.out(start, dirname+' ')
            return start
	#-----------------------------------------------------------------------
        start, self.fromL = 0., filter(os.path.isdir, self.fromL) 
        for repository in fromL: start = vizit(repository, start, 1./len(self.fromL)) 
        #repository = os.path.abspath( os.path.expanduser( os.path.expandvars(chain2afuse(repository)) ) )+'/'
        _after_calc(self._L, csfhL); SELECT._i = 0; self._recalc_ts()
        if old_: calc._G['_'] = old_
        else: del calc._G['_']
        if self.progressbar: self.progressbar.close('select ')
	self.runtime = time.time()-starttime
    #---------------------------------------------------------------------------
    def __len__(self): return len(self._L)
    def nodes(self, begin=None, end=None): 'возвращает список расчетов'; return [ l[0] for l in self._L[begin:end] if l ]
    def _recalc_ts(self, calc_size=False): #recalc c_runtime and c_size
        if calc_size or self.c_size: self.c_size = sum([ l[0].get_size() for l in self._L if l ])
        self.c_runtime = sum([ getattr(l[0], 'runtime', 0) for l in self._L if l ])        
    #---------------------------------------------------------------------------
    def astable(self, head=True, tw=None): 
        'вернуть выборку в виде таблицы (как список строк)'
        return mixt.table2strlist([None, self.head]*head+[None]+[l[1:] if l else None for l in self._L]+[None], max_len=tw)
    def __str__(self): return ''.join(self.astable())
    def asdata(self, head=True, patt='', fname=None): 
        'вернуть (как список строк) или записать выборку, в формате .dat-файла'
        if type(patt) is str: patt = patt.split()
        D = reduce(lambda D, C: dict([(k, D.get(k, set())|set([v])) for k, v in C.par_dict().items()
                                      if hashable(v) and mixt.compare(k, patt) ]), self.nodes(), {}) if patterns else {}
        R = ['#: %s = %r\n'%(k, list(S)[0]) for k, S in D.items() if len(S)==1] + mixt.table2strlist( 
            [['#:'+self.head[0]]+self.head[1:]]*bool(head)+[ l[1:] if l else ['']*len(self.head) for l in self._L ],
            'l'*len(self.head))
        if fname: (gzip.open if fname.endswith('.gz') else open)(fname, 'w').writelines(map(str.rstrip, R)); return []
        else: return map(str.rstrip, R)
    def paths(self, fname=''):
        'Возвращает пути (к расчету или файлу), проверяя на их на существование'
        return [l[0].path+fname for l in self._L if l and os.path.exists(l[0].path+fname)]
    #---------------------------------------------------------------------------
    def Xcommit(self): 'сохраняет изменения в расчетах на диск'; [l[0].commit() for l in self._L if l]
    def Xremove(self):
        'удаляет все записи из выборки и из базы'
        starttime = time.time(); import shutil
        for p in [l[0].path for l in self._L if l]:
            try: shutil.rmtree(p)
            except Exception, e: print e
        del self._L[:]
        self.runtime = time.time()-starttime
    #---------------------------------------------------------------------------
    def get_keys(self, mode='or'):
        'список всех параметров привязанных к расчетам выборки, допустимые режимы "|", "&", "^", "or", "and", "xor"'
        starttime = time.time()
        keys = [l[0].par_dict('statelist', 'args', 'runtime', 'progress').keys() for l in self._L if l]
        S = reduce(getattr(set, '__%s__'%{'|':'or', '&':'and', '^':'xor'}.get(mode, mode)), keys, set(keys.pop(0)))
        self.runtime = time.time()-starttime
        return sorted(list(S))
    
    def get_values_summary(self, *patterns):   #???
        'все значения выражений и ключей для расчетов выборки'
        D, i, starttime = {}, 0., time.time()
        if self.progressbar : self.progressbar.clean()
        for l in self._L :
            if not l : continue
#            print l[0]._path_dict[''][0]._path_dict
            for k, v in l[0].par_dict( ['args','path','runtime','progress','statelist'] ).items()  :
                if not k.startswith('@') and ( not self.head or compare( k, patterns ) ) : D.setdefault( k, set() ).add( v )
            for k, v in zip( self.head, l[1:] ) : 
                if not patterns or compare( k, patterns ) : D.setdefault( k, set() ).add( v )
            if self.progressbar : self.progressbar.out( i/len(self), 'get_values_summary ' ); i+=1
        D = dict(( ( k, sorted(v) ) for k, v in D.items() ))
        self.runtime = time.time() - starttime
        if self.progressbar : self.progressbar.close( 'get_values_summary ' )
        return D
    def get_types_summary( self ) :  
        'все типы значений выражений и ключей для расчетов выборки'
        D, i, starttime = {}, 0., time.time()
        if self.progressbar : self.progressbar.clean()
        for l in self._L :
            if not l : continue
            for k, v in l[0].par_dict( ['args','path','runtime','progress','statelist'] ).items() + zip( self.head, l[1:] ) :
                if k.startswith('@') : continue
                r = D.setdefault( k, [] )
                if v.__class__ not in r : r.append(v.__class__)
            if self.progressbar : self.progressbar.out( i/len(self), 'get_types_summary ' ); i+=1
#        for l in D.values() : l.sort()
        self.runtime = time.time() - starttime
        if self.progressbar : self.progressbar.close( 'get_types_summary ' )
        return D
    #---------------------------------------------------------------------------
    # ??? что то странное ???
    def __getitem__(self, i): 
        if type(i) is tuple: r, i = i[0], i[1]+SELECT._i; return self._L[i][r] if self._L[i] else None
        i += SELECT._i; return self._L[i][0] if self._L[i] else None
    def __setitem__( self, i, x ) : self._L[i[1]+SELECT._i][i[0]] = x
    def __delitem__( self, i ) : 
        if self._L[i] : 
            if self.c_size : self.c_size -= self._L[i][0].get_size() 
            self.c_runtime -= getattr(self._L[i][0], 'runtime', 0) 
            del self._L[i]
            if i and i<len(self._L) and not self._L[i-1] and not self._L[i] : del self._L[i] # double separator delete
    def __getslice__( self, begin, end ) : 
        S = SELECT(None); S.progress, S.runtime, S._L = self.progress, self.runtime, self._L[begin:end]
        S._recalc_ts(self.c_size); return S
    def __delslice__(self, begin, end): self._L[begin:end] = [None] if None in self._L[begin:end] else []; self._recalc_ts()
#-------------------------------------------------------------------------------
def _after_calc( L,         # выборка (список строк)
                 csfhL,     # список результатов работы ф-ии parse
                 shift=1,   # начальный столбец для обработки
                 sepS=[] ): # позиции горизонтальных сепараторов как id объектов Calc ПЕРЕД сепаратором
    'постобработка выборки --- сортировка, вставка сепараторов, выбрасывание лишних столбцов и строк'
    # SORTING
    for ie in filter(lambda i: csfhL[i][1], range(len(csfhL))): L.sort(key=lambda l: l[ie+1], reverse=(csfhL[ie][1]<0)) 

    # DISTINCT 
    distL, i = filter(lambda ie: csfhL[ie][2]==-2, range(len(csfhL))), 1 
    if distL:
        while i<len(L):
            for ie in distL:
                if L[i][ie+shift]==L[i-1][ie+shift]: del L[i]; break
            else: i += 1

    # INSERT SEPARATORS
    sepL, i = filter(lambda e: csfhL[e][2]==-1, range(len(csfhL))), 1
    if sepL or sepS: 
        while i<len(L): 
            l1, l2 = L[i-1], L[i]
            if l1 and l2 and id(l1[0]) in sepS: L.insert(i, None); i += 1; continue 
            for ie in sepL:
                if l1 and l2 and l1[ie+shift]!=l2[ie+shift]: L.insert(i, None); i += 1; continue 
            i += 1

    # HIDE COLUMNS        
    hideL = filter(lambda e: csfhL[e][3], range(len(csfhL), 0, -1))
    if hideL:
        for l in L:
            if not l: continue
            for ie in hideL: del l[ie+shift]
#-------------------------------------------------------------------------------
