# -*- coding: utf-8 -*-
'''Bind Python code to shell.
Copyright (C) 2010, 2012, 2014, 2016 Anton V. Ivanov, KIAM RAS, Moscow.
This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991) or later.
'''
#-------------------------------------------------------------------------------
import os, sys, fnmatch, atexit, subprocess, re 
from types import MethodType, FunctionType, BooleanType

# main parametrs for parse
_opt = '--' # start of the explicit option name
_end = '@'  # explicit option terminate
_eq = '='   # symbol for set keywoard argument
_bra, _ket = '{}' # brakets for group options
_stdin = '--'  # read options from stdin (as --)
_file = '++'   # read options from file (as ++ filename)
_lang = 0 # exception language (0: english, 1: russian), see ParseError.table for details
#-------------------------------------------------------------------------------
class ParseError(Exception):
    table = [('option "%(name)s" unrecognized, variants: %(variants)s!', 
              'неизвестная опция "%(name)s", варианты %(variants)s!'), #0
             ('can\'t convert "%(value)s" to bool, can use y, Y, yes, Yes, YES, on, On, ON, true, True, TRUE, 1, '
                'n, N, no, No, NO, off, Off, OFF, false, False, FALSE, 0',
                'невозможно преобразовать значение "%(value)s" к булевому типу, можно использовать только значения ' 
                'y, Y, yes, Yes, YES, on, On, ON, true, True, TRUE, 1, '
                'n, N, no, No, NO, off, Off, OFF, false, False, FALSE, 0'), #1
              ('terminate option "%(name)s" without args (%(args)s)', 
               'пропущены обязательные аргументы (%(args)s) при задании опции "%(name)s"'), #2
              ( 'incorrect implicit option %(opt)r',
                'некорректно задана неявная опция %(opt)r' ), #3
              ('incorrectly used %(braket)r in %(name)s', 
               'некорректное использование %(braket)r в %(name)s'), #4
              ('argument %(arg)r unrecognized in %(name)s', 
               'аргумент %(arg)r нераспознан в %(name)s') #5
             ]
    arglist = []
    def __init__(self, num, **kw_args): 
        self.num = num; self.__dict__.update(kw_args) 
        Exception.__init__(self, ' [E.%i] '%num+self.table[num][_lang]%kw_args)
#-------------------------------------------------------------------------------
def _recognize_word(word, wordlist):
    '''Принимает слово и список допустимых вариантов (слов). Возвращает слово из wordlist:
    1) если есть единственное слово совпадающее с word
    2) иначе, если в wordlist есть единственное слово начинающееся с word    
    3) иначе, если в wordlist есть единственное слово, которое может быть получено из word путем вставки и добавления в конец 
    произвольного числа символов
    4) иначе возбуждается исключение E.0'''
    variants = filter(lambda o: o==word, wordlist)
    if not variants: variants = filter(lambda o: o.startswith(word), wordlist)
    if not variants: reg = re.compile('^'+'[aAeEiIoOuU]{0,}'.join(word)+'.*'); variants = filter(reg.search, wordlist)
    if not variants: reg = '*'.join(word)+'*'; variants = filter(lambda o: fnmatch.fnmatch(o, reg), wordlist)
    if len(variants) != 1: raise ParseError(0, name=word, variants=variants)
    return variants[0]
#-------------------------------------------------------------------------------
def _recognize_argument(s, arglist, aliases, use_abbrv):
    '''распознает аргумент s, который может быть задан в виде аббревиатуры и 
    находиться либо в списке аргументов args либо в словаре синонимов aliases.
    Возвращает либо распознанный аргумент (обязательно из arglist ?) либо None (в случае неудачи)'''
    args = filter(lambda s: not s.startswith('_'), list(arglist))+sum(aliases.values(), [])
    try:
        if use_abbrv: res = _recognize_word(s, args) 
        elif s in args: res = s
        else: raise ParseError(0, name=s, variants=[])
        for k, v in aliases.items():
            if res in v: return k
        return res
    except ParseError, e: pass
#-------------------------------------------------------------------------------
class UnknownValue: pass
def _type_convert(src, tgt):
    'convert string $tgt to type of value $src'
    if src.__class__ is UnknownValue: return tgt
    if src is None or type(src)==str: return tgt if tgt!='None' else None #???
    if src.__class__ is BooleanType: 
        if tgt in ('y', 'Y', 'yes', 'Yes', 'YES', 'on',  'On',  'ON',  'true',  'True',  'TRUE',  '1'): return True
        if tgt in ('n', 'N', 'no',  'No',  'NO',  'off', 'Off', 'OFF', 'false', 'False', 'FALSE', '0'): return False
        raise ParseError(1, value=tgt)
    return src.__class__(tgt)
#-------------------------------------------------------------------------------
def _func_info(f): 
    '''возвращает кортеж: список имен аргументов функции, список значений по умолчанию, 
    имена аргументов *args (либо None) и **kw_args (либо None)'''
    argc, names, flags = f.func_code.co_argcount, list(f.func_code.co_varnames), f.func_code.co_flags
    defaults = f.func_defaults if f.func_defaults else ()
    if type(f)==MethodType: argc -= 1; del names[0]
    return (names[:argc], list(defaults), (names[argc] if flags & 0x04 else None), 
            (names[argc+bool(flags & 0x04)] if flags & 0x08 else None))
#-------------------------------------------------------------------------------
def _signature(f, n=None):
    'return signature (as string) of function $f with name $n'
    names, defs, args, kw_args = _func_info(f)
    if f.__doc__ and f.__doc__[0]=='@': names = map(lambda t, n: t.__name__+' '+n if t else n, 
                                                    eval(f.__doc__[1:].split('\n')[0]), names)
    S = ', '.join( names[:len(names)-len(defs)]+[ '%s=%r'%i for i in zip(names[-len(defs):], defs) 
                                                  ]+['*%s'%args]*bool(args)+['**%s'%kw_args]*bool(kw_args) )
    return (f.__name__ if n==None else n)+('(%s)'%S if S else '()')
#-------------------------------------------------------------------------------
_is_name = lambda a: a and a[0].isalpha() and a.replace('_','').isalnum() # имя опции
_is_ex_opt = lambda a: _opt and a.startswith(_opt) and _is_name(a[len(_opt):]) # явная опция
_is_arg = lambda a: _is_name(a) or _is_ext_opt(a) # имя опции либо явная опция
_is_key_eq_val = lambda a: _eq and _eq in a and not a[a.index(_eq):].startswith(_eq+_eq) and _is_arg(a.split(_eq,1)[0]) # [--]k=v
_opt_list = lambda self: set(dir(self)+self._opt_list # список опций для объекта self
                             +getattr(self, '__swig_setmethods__', {}).keys())-set(['this', 'thisown']+self._ignore_list)
#-------------------------------------------------------------------------------
# вся функциональность всегда реализуется через Parser.__call__
# опция с одним аргументом экививалента параметру
# допускается запись имя-параметра=значение либо имя-параметра значение 
# чтение аргументов опции всегда имеет более высокий приоритет чем чтение имени следующей опции, аргументы читаются
#     пока не закончится строка либо пока не будет проведено явное терминирование опции через _end/_ket
#     либо (неявное терминирование) пока не будут установлены все параметры, т.о. опция не может быть неявно терминирована
#     параметром вида key=value
# при запуске опций с параметрами, для чтения аргументов формируется новый экземпляр Parser.__call__ 
#     с атрибутами отвечающими параметрам опции, т.о. возможно чтение неограниченного числа позиционных параметров и 
#     именованных параметров
# если не была открыта скобка парсер терминируется концом списка аргументов, концом строки (что экививалентно) либо символом _end
# АЛГОРИТМ:
#    обработать специальные аргументы
#    если аргумент вида [-]key=value:
#         попытаться распознать (нечетко?) key
#             если key метод вызвать его (как именно?)
#             иначе установить поле key
#         при неудаче 
#             если есть kw_args добавить в kw_args
#             иначе если есть orders добавить в orders
#             иначе выкинуть исключение №0 (через raise)
#    иначе, если аргумент вида [-]key
#           если не принимается словарь именованных параметров: распознать (нечетко?) параметр и установить его значение
#           иначе: попытаться распознать и установить параметр, при неудаче добавить в словарь именованных параметров
#    иначе, если есть order: попытаться установить как неименованный параметр   
#    иначе, если есть неявная опция: активировать неявную опцию
#    иначе выкинуть ошибку
# как запускать eval на значение, в каком пространстве имен?
# как отображать поля в справке?
#-------------------------------------------------------------------------------
def _call_method(self, name, method, arglist, istream): 
    'разбор аргументов и запуск для метода method с приглашением name, начальный список имен arglist, входной поток istream' 
    if isinstance(method, Parser): method(arglist, name, istream, mode=0)
    else:
        argnames, defaults, pos_args_name, kw_args_name = _func_info(method)
        parser = Parser(**dict( zip(argnames, [UnknownValue()]*(len(argnames)-len(defaults))+defaults) ))
        parser.__doc__ = method.__doc__
        pos_args, kw_args = ([] if pos_args_name else None), ({} if kw_args_name else None)
        parser(arglist, name, istream, order=list(argnames), args=pos_args, kw_args=kw_args, mode=0)
        if any(v.__class__ is UnknownValue for v in parser.__dict__.values()): 
            raise ParseError(2, args=', '.join(k for k, v in parser.__dict__.items() if v.__class__ is UnknownValue), name=name)
        if kw_args: parser.__dict__.update(kw_args)
        if method.__doc__ and method.__doc__.startswith('@'):
            T = eval(method.__doc__.split('\n')[0][1:])
            for k, t in zip(argnames, T): setattr(parser, k, _type_convert(t(), getattr(parser, k)))
        method(*(pos_args if pos_args_name else []), **dict(i for i in parser.__dict__.items() if i[0][0]!='_'))
#-------------------------------------------------------------------------------
class Parser:
    ''
    _implicit_opt = None # опция активируемая по умолчанию
    _aliases = {}        # синонимы в виде {'имя опции':['синоним1', 'синоним2', ...]}
    _ignore_list = []    # список игнорируемых (как опции) атрибутов класса
    _opt_list = []       # дополнительный список опций
    #---------------------------------------------------------------------------
    def __init__(self, **D): 
        L = self.__doc__.split('\n') if self.__doc__ else []
        self._help_table = dict([ (l[1:].split(' ', 1)+[''])[:2] for l in map(str.strip, L) if l.startswith('@') ])
        self.__doc__ = '\n'.join('%%(%s)s'%l.split()[0][1:] if l.strip().startswith('@') else l.replace('%', '%%') for l in L)
        for k, v in D.items():
            if type(v) is tuple and len(v)==2 and type(v[1]) is str and v[1].startswith('#@'): v, self._help_table[k] = v
            setattr(self, k, v)
    #---------------------------------------------------------------------------
    def __call__(self, arglist, name=None, istream=None, order=[], args=None, kw_args=None, mode=-1): 
        '''функция, разбирающая список аргументов командной строки, возвращает None.
        arglist --- список аргументов для разбора
        name --- имя экземпляра класса (отображается в приглашении в интерактивном режиме)
        istream --- поток для чтения опций
        order --- список имен позиционных аргументов
        args --- список для оставшихся позиционных аргументов (аналог *args при вызове функции)
        kw_args --- список для оставшихся именованных аргументов (аналог **kw_args при вызове функции)
        mode --- режим работы (терминирования), -1 (default) | 0 (single) | 1 (open braket)
        '''
        if self._implicit_opt: # проверка корректности установки неявной опциий
            imp_opt = getattr(self, self._implicit_opt, None)
            names, defaults, args, kw_args = _func_info(imp_opt) if callable(imp_opt) else [None]*4
            if not names and not args: raise ParseError(3, opt=self._implicit_opt)

        first_call, order, name = True, list(order), (name if name else self.__class__.__name__)
        while arglist or mode or first_call: # главный цикл
            first_call = False # for single mode
            try: 
                #---------------------------------------------------------------
                #    настройки readline
                #---------------------------------------------------------------
                _kw_stack.append([self]+[ _opt+i+( ' ' if not isinstance( getattr(self, r), Parser ) 
                                                   else ( '' if getattr( getattr(self, r), '_implicit_opt', '' ) else '.' ) ) 
                                          for i, r in zip(_opt_list(self), _opt_list(self))+
                                          sum([[ (j,k) for j in l ] for k, l in self._aliases.items() ],[]) if i[0]!='_' ])
                #---------------------------------------------------------------
                #   read options from istream
                #---------------------------------------------------------------
                while not arglist and istream: 
                    L, read_next_line = [], True
                    while read_next_line:
                        l = raw_input('... ' if L else '%s> '%name)+'\n' if os.isatty(istream.fileno()) \
                            and os.isatty(sys.stdout.fileno()) else istream.readline()
                        read_next_line = l and l.rstrip().endswith('\\') 
                        L.append(l.rstrip()[:-1] if read_next_line else l.rstrip())
                    #arglist.extend([ i[:-1] for i in os.popen( 'for i in %s \ndo echo "$i" \ndone'%' '.join(L) ).readlines() ])
                    shell = subprocess.Popen(os.environ['SHELL'], stdout=subprocess.PIPE, stdin=subprocess.PIPE) 
                    shell.stdin.write('for i in %s \ndo echo "$i" \ndone\n'%' '.join(L)); shell.stdin.close()
                    arglist.extend([ i[:-1] for i in shell.stdout.readlines() ]); del shell
                    if not l: break
                if not arglist: break #return ?
                # ??? if _space and _space in arglist[0]: arglist[:1] = arglist[0].split(_space, 1)
                #---------------------------------------------------------------
                #    обработка специальных опций
                #---------------------------------------------------------------
                if arglist[0] in (_end, _bra, _ket, _stdin, _file): 
                    a = arglist.pop(0)
                    if a in (_stdin, _file):
                        L = []; loglist.append(repr(_stdin) if a==_stdin else "%r %r"%(_file, arglist[0]))
                        self(L, name, istream=sys.stdin if a==_stdin else open(arglist.pop(0)), 
                             order=order, args=args, kw_args=kw_args) # mode=mode or default ???
                        arglist[:0] = L
                    elif a==_ket and mode==-1: raise ParseError(4, name=name, braket=a) # в дефолтном режиме нефига юзать _ket!
                    elif mode:
                        loglist.append(repr(a))
                        if a==_bra: self(arglist, name, istream, order, args, kw_args, 1)
                        if a==_ket: break
                    else:
                        if a==_bra: loglist.append(repr(_bra)); mode = 1
                        if a==_ket: arglist.insert(0, a); break
                        if a==_end: loglist.append(repr(a)); break #return ?
                    continue
                #---------------------------------------------------------------
                #    обработка обычных опций
                #---------------------------------------------------------------
                a = arglist[0]
                if _is_key_eq_val(a):
                    key, val = a.split(_eq, 1) 
                    key = _recognize_argument(key, _opt_list(self), self._aliases, 
                                              kw_args is None and _opt and key.startswith(_opt))
                    if key: 
                        attr = getattr(self, key) 
                        if callable(attr): _call_method(self, key, attr, [val], None)
                        else: setattr(self, key, _type_convert(attr, val))
                    elif not kw_args is None: kw_args[key] = val
                    elif order: key = order.pop(0); setattr(self, key, _type_convert(getattr(self, key), a)) #???
                    else: raise 
                    loglist.append(arglist.pop(0)) 
                    continue
                optname = _recognize_argument(a[len(_opt):], _opt_list(self), self._aliases, _opt and a.startswith(_opt))
                if _is_opt(a) and optname and callable(getattr(self, optname)): 
                    loglist.append(arglist.pop(0))
                    _call_method(self, optname, getattr(self, optname), arglist, istream) 
                    continue
                if order:
                    key = order.pop(0)
                    setattr(self, key, _type_convert(getattr(self, key), a))
                    loglist.append(key+_eq+arglist.pop(0)) 
                    continue
                if _is_opt(a) and optname and len(arglist)>1 and arglist[1] not in (_bra, _ket, _end, _file, _stdin): 
                    val = arglist[1]; del arglist[:2]; loglist.append(a+' '+val)
                    setattr(self, optname, _type_convert(getattr(self, optname), val))
                    continue
                if self._implicit_opt: 
                    loglist.append(self._implicit_opt)
                    _call_method(self, self._implicit_opt, getattr(self, _implicit_opt), arglist, istream)   
                    continue
                raise ParseError(5, arg=a, name=name)
            #-------------------------------------------------------------------
            #   обработка исключений
            #-------------------------------------------------------------------
            except EOFError, e: sys.stdout.write('\n'); return
            except KeyboardInterrupt, e:
                if not mode: del loglist[-1]
                if mode and istream and os.isatty(istream.fileno()) and os.isatty(sys.stdout.fileno()): print '^C'
                else: raise
            except Exception, e:
                if not mode: del loglist[-1]
                if mode and istream and os.isatty(istream.fileno()) and os.isatty(sys.stdout.fileno()): 
                    lt, lv, tb = ( sys.last_type, sys.last_value, sys.last_traceback 
                                   ) if sys.exc_info() == (None,)*3 else sys.exc_info()
                    while tb and tb.tb_frame.f_code.co_filename != __file__.rstrip('c'): 
                        sys.stderr.write('\tFile "%s", line %s, in %s\n'%(
                                tb.tb_frame.f_code.co_filename, tb.tb_lineno, tb.tb_frame.f_code.co_name ))
                        tb = tb.tb_next; 
                    sys.stderr.write('>>> %s <<< %s\n%s:%s\n'%(a, ' '.join(map( repr, arglist)),  lt.__name__, lv))
                else: ParseError.arglist[:] = arglist; raise 
                del arglist[:]
            finally: del _kw_stack[-1]
    #---------------------------------------------------------------------------
    def help(self, opt=None, detailed=False): #, less=True): 
        u'выводит детальную справку по опции opt или сводную справку по всему объекту'
        names = lambda name: ', '.join(filter(lambda x: x[0]!='_', [name]+list(self._aliases.get(name, []))))
        #-----------------------------------------------------------------------
        def doc(name):
            attr, sign = getattr(self, name), names(name)
            if isinstance(Parser, attr): res = [sign+'(...)', (attr.__doc__.split('\n')[0] if attr.__doc__ else '???')]
            elif callable(attr): res = [_signature(attr, sign), 
                                        self._help_table.get(name, attr.__doc__.split('\n', 1)[1] 
                                                             if attr.__doc__ and attr.__doc__[0]=='@' 
                                                             else attr.__doc__ if attr.__doc__ else '???')]
            else: res = ['%s=%r'%(sign, attr), self._help_table.get(name, '???')]
            if not opt and not detailed: res[1] = res[1].split('\n')[0]
            return '\033[2m%s\033[0m --- %s'%res
        #-----------------------------------------------------------------------
        if opt: 
            opt = _recognize_argument(opt, _opt_list(self), self._aliases, True)
            if not opt: raise
            attr = getattr(self, opt)
            if isinstance(Parser, attr): attr.help(detailed=detailed, less=less); return
            text = doc(opt)
        else: 
            D = dict(doc(k) for k in _opt_list(self) if names(k))
            text = self.__doc__%_HelpNamespace(D)
            text = '\n'.join([text]+[D[k] for k in sorted(D.keys())])
        #try: tty_sz = int(os.popen('stty size').readline().split()[1])
        #except: tty_sz = 80
        #for l in text.split('\n'):
        print text            
#-------------------------------------------------------------------------------
class _HelpNamespace: 
    def __init__(self, D): self.D = D
    def __getitem__(self, key): return '%s --- %s'%self.D.pop(key)
#-------------------------------------------------------------------------------
class RadioSelect(Parser): 
    def __init__(self, *statelist): 
        self.__doc__ = '%(_state)r, variants: '+'|'.join(map(str, statelist))
        self._statelist, self._state, self._implicit_opt = map(str, statelist), str(statelist[0]), '_set'
    def _set(self, state): self._state = _recognize_word(state, self._statelist)
    def __str__(self): return str(self._state)
    def __nonzero__(self): return self._state!=self._statelist[0]
#-------------------------------------------------------------------------------
class CheckSelect(Parser):
    def __init__(self, **kw_args): self.__dict__.update([ (k, (bool(v) if k[0]!='_' else v)) for k, v in kw_args.items() ])
#-------------------------------------------------------------------------------
#   READLINE
#-------------------------------------------------------------------------------
def _rl_completer( l, state ) :
    if state : return _last_complete_list[state]
    if _dot and _dot in l and all(map( _is_ex_opt_or_name, l.split(_dot) )[:-1]) : #??? aliases ???
        sub, start = _kw_stack[-1][0], ''
        for ll in [ i[len(_opt):] if i.startswith(_opt) else i for i in l.split(_dot) ][:-1] : 
            sub, start = getattr( sub, ll, None ) if ll[0]!='_' and isinstance( sub, Parser ) else None, start+_dot+ll
        start = _opt+start[len(_dot):]+_dot; ll = l.split(_dot)[-1]; ll = ll[len(_opt):] if ll.startswith(_opt) else ll
        _last_complete_list[:] = [ start+i for i in dir(sub)+sum( sub._aliases.values(), [] ) if i[0]!='_' and ( not ll or i.startswith( ll ) ) ]
    else : _last_complete_list[:] = filter( lambda i : not l or i.startswith( l if l.startswith(_opt) else _opt+l ), _kw_stack[-1][1:] )
#    B, io = _kw_stack[-1][0], []
#     while isinstance(B,Parser) : 
#         if B._implicit_opt : io.append( B._aliases.get( B._implicit_opt, [B._implicit_opt] )[0] ); B = getattr( B, B._implicit_opt, None )
#         if isinstance(B,Parser) : _last_complete_list.extend([ _opt+_dot.join(io+[i]) for i in sum(B._aliases.values(),dir(B)) 
#                                                              if i[0]!='_' and i.startswith(l) ])
#     if io : _last_complete_list.append( _opt+_dot.join(io)+' '+l )
    _last_complete_list.extend(filter( lambda i : i.startswith(l), rl_keywords ))
    path = os.path.dirname( os.path.expandvars( os.path.expanduser( l ) ) ); d, f = os.path.split( l )
    if not path : path = './'
    if os.path.isdir( path ) : _last_complete_list.extend([ p+'/'*os.path.isdir(p) for p in 
                                                            [ os.path.join( d, i ) for i in os.listdir(path) if not f or i.startswith( f ) ] ])
    return _last_complete_list[0]
#-----------------------------------------------------------------------------------------------------------------------
loglist, _kw_stack = [], []
if os.isatty(sys.stdout.fileno()) and os.isatty(sys.stdin.fileno()) :
    try:
        import readline
        _history_path, _last_complete_list, rl_keywords = os.path.expanduser( "~/.%s.history"%os.path.basename(sys.argv[0]) ), [], []
        atexit.register( lambda hp=_history_path : readline.write_history_file(hp) )
        if os.path.exists( _history_path ) : readline.read_history_file( _history_path )
        readline.parse_and_bind('tab: complete') 
        readline.set_completer( _rl_completer )
        readline.set_completer_delims(' ')
    except ImportError, e: print 'Module readline not found'
#-----------------------------------------------------------------------------------------------------------------------

#TODO
# поддержка списков и словарей
# help - @option --- string позволяет повесить справку на option
