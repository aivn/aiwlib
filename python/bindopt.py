# -*- coding: utf-8 -*-
'''Bind Python code to shell.
Copyright (C) 2010, 2012, 2014 Anton V. Ivanov, KIAM RAS, Moscow.
This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991) or later.
'''
#-----------------------------------------------------------------------------------------------------------------------
import os, sys, fnmatch, atexit, subprocess, re 
from types import MethodType, FunctionType, BooleanType

# main parametrs for parse
_opt = '--' # start of the explicit option name
_end = ';'  # explicit option terminate
_eq = '='   # symbol for set keywoard argument
_bra, _ket = '{}' # brakets for group options
_stdin = '--'  # read options from stdin (as --)
_file = '++'   # read options from file (as ++ filename)
_lang = 0 # exception language (0: english, 1: russian), see ParseError.table for details

#BO_GREED_ARGS, BO_GREED_KW_ARGS = 1, 2
#_hard_opt_started, _logstream = True, None #_use_kw_args = True
#-----------------------------------------------------------------------------------------------------------------------
class ParseError(Exception):
    table = [ ( 'option "%(name)s" unrecognized, variants: %(variants)s!', 
                'неизвестная опция "%(name)s", варианты %(variants)s!' ), #0
              ( 'can\'t convert "%(value)s" to bool, can use y, Y, yes, Yes, YES, on, On, ON, true, True, TRUE, 1, '
                'n, N, no, No, NO, off, Off, OFF, false, False, FALSE, 0',
                'невозможно преобразовать значение "%(value)s" к булевому типу, можно использовать только значения ' 
                'y, Y, yes, Yes, YES, on, On, ON, true, True, TRUE, 1, '
                'n, N, no, No, NO, off, Off, OFF, false, False, FALSE, 0' ), #1
              ( 'terminate option "%(name)s" without args (%(args)s)', 
                'пропущены обязательные аргументы (%(args)s) при задании опции "%(name)s"' ), #2
              ( 'argument %(arg)r unrecognized', 
                'аргумент %(arg)r нераспознан' ), #3
              ( 'incorrectly used %(braket)r in %(name)s', 
                'некорректное использование %(braket)r в %(name)s' ), #4
              ( 'attribute %(name)s.%(attr)s isn\'t function', 
                'атритбут %(name)s.%(attr)s не является функцией' ), #5
              ( 'incorrect implicit option %(opt)r',
                'некорректно задана неявная опция %(opt)r' ), #6
              ( 'incorrect argument %(arg)r',
                'некорректный аргумент %(arg)r' ), #7
              ( 'arguments %(args)s not defined',
                'аргументы %(args)s не определены' ) #8
#              ( '%(signature)s got multiple values for keyword parameter "%(par)s"', 
#                '%(signature)s получила несколько значений для именованного параметра "%(par)s"' ), #4
              # 'activate implicit option %(opt)r by illegal keyword argument %(arg)r' 
              ]
    arglist = []
    def __init__(self, num, **kw_args): 
        self.num = num; self.__dict__.update(kw_args) 
        Exception.__init__(self, ' [E.%i] '%num+self.table[num][_lang]%kw_args)
#-----------------------------------------------------------------------------------------------------------------------
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
def _recognize_argument(s, args, aliases, use_abbrv):
    'recognize argument $s from $args and $aliases'
    args = filter(lambda s: not s.startswith('_'), list(args) ) + sum( aliases.values(), [])
#    if _opt and s.startswith(_opt): s, use_abbrv = s[len(_opt):], True
    if use_abbrv: res = _recognize_word(s, args)
    elif s in args: res = s
    else: return
    for k, v in aliases.items():
        if res in v: return k
    return res
#-----------------------------------------------------------------------------------------------------------------------
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
#-----------------------------------------------------------------------------------------------------------------------
def _func_info(f): 
    'return names, defaults, args-name, kw_args-name for function $f'
    argc, names, flags = f.func_code.co_argcount, list(f.func_code.co_varnames), f.func_code.co_flags
    defaults = f.func_defaults if f.func_defaults else ()
    if type(f)==MethodType: argc -= 1; del names[0]
#    print argc, names, ( names[argc+1] if flags & 0x04 else None ), ( names[argc+1+bool(flags & 0x04)] if flags & 0x08 else None )
    return ( names[:argc], list(defaults), (names[argc] if flags & 0x04 else None), 
             (names[argc+bool(flags & 0x04)] if flags & 0x08 else None) )
#-----------------------------------------------------------------------------------------------------------------------
def _signature(f, n=None):
    'return signature of function $f with name $n'
    names, defs, args, kw_args = _func_info(f)
    S = ', '.join( names[:len(names)-len(defs)]+[ '%s=%r'%i for i in zip(names[-len(defs):], defs) 
                                                  ]+['*%s'%args]*bool(args)+['**%s'%kw_args]*bool(kw_args) )
    return (f.__name__ if n==None else n)+('(%s)'%S if S else '()')
#-----------------------------------------------------------------------------------------------------------------------
_is_name = lambda a: a and a[0].isalpha() and a.replace('_','').isalnum()
_is_ex_opt = lambda a: _opt and a.startswith(_opt) and _is_name(a[len(_opt):])
_is_key_eq_val = lambda a: _eq and _eq in a and not a[a.index(_eq):].startswith(_eq+_eq) and _is_name(a.split(_eq,1)[0])
_opt_list = lambda self: ( set( dir(self)+self._opt_list+self.__swig_setmethods__.keys() )-set(['this']) \
                               if hasattr(self, '__swig_setmethods__') else \
                               set( dir(self)+self._opt_list ) )-set(self._ignore_list)
#-----------------------------------------------------------------------------------------------------------------------
# вся функциональность всегда реализуется через BASE.__call__
# при запуске опций с параметрами для чтения формируется новый экземпляр BASE.__call__ с атрибутами отвечающими параметрам опции
# т.о. возможно чтение неограниченного числа позиционных параметров и именованных параметров
# если не была открыта скобка парсер терминируется концом списка аргументов, концом строки (что экививалентно) либо символом _end
# параметры опций всегда задаются в виде key=value
# АЛГОРИТМ:
#    обработка специальных аргументов
#    если аргумент явная опция (--имя):
#         в single mode терминироваться 
#         распознать (нечетко) опцию, запустить новый BASE.__call__ 
#    иначе, если аргумент имеет вид именованного параметра: 
#           если не принимается словарь именованных параметров: распознать (нечетко) параметр и установить его значение
#           иначе: попытаться распознать и установить параметр, при неудаче добавить в словарь именованных параметров
#    иначе, если есть order: попытаться установить как неименованный параметр   
#    иначе, попытаться распознать как опцию
#    иначе, если есть неявная опция: активировать неявную опцию
#    иначе выкинуть ошибку
# как запускать eval на значение, в каком пространстве имен?
# как отображать поля в справке?

def _check_implicit_opt(self):
    if self._implicit_opt:
        names, defaults, args, kw_args = _func_info(getattr(self, self._implicit_opt)) if \
            hasattr(self, self._implicit_opt) and callable(getattr(self, self._implicit_opt)) else [None]*4
        if not names and not args: raise ParseError(6, opt=self._implicit_opt)
#-----------------------------------------------------------------------------------------------------------------------
def _call_method(self, name, method, arglist, istream): 
    if isinstance(method, BASE): method(arglist, name, istream, mode=0)
    else:
        argnames, defaults, pos_args_name, kw_args_name = _func_info(method)
        parser = BASE(**dict( zip(argnames, [UnknownValue()]*(len(argnames)-len(defaults))+defaults) ))
        parser.__doc__ = method.__doc__
        pos_args, kw_args = ([] if pos_args_name else None), ({} if kw_args_name else None)
        parser(arglist, name, istream, order=argnames, args=pos_args, kw_args=kw_args, mode=0)
        if any(v.__class__ is UnknownValue for v in parser.__dict__.values()): 
            raise ParseError(8, args=', '.join(k for k, v in parser.__dict__.items() if v.__class__ is UnknownValue))
        if kw_args: parser.__dict__.update(kw_args)
        method(*(pos_args if pos_args_name else []), **dict(i for i in parser.__dict__.items() if i[0][0]!='_'))
#-----------------------------------------------------------------------------------------------------------------------
#   MAKE HELP
#-----------------------------------------------------------------------------------------------------------------------
class _HelpNamespace: 
    def __init__(self, obj): self.obj, self.L = obj, []
    def __getitem__(self, key): 
        self.L.append(key.rstrip('=')) 
        v = getattr(self.obj, self.L[-1], '???') 
        return '%s=%r'%(self.L[-1], v) if key[-1]=='=' else v
#-----------------------------------------------------------------------------------------------------------------------
str_len = lambda S: int(reduce(lambda r, c: r+1./(1+(not type(S) is unicode and ord(c)>127))+4*(c=='\t'), S, 0))
def _out_format_text(text, first_prefix, other_prefix, tty_sz):
    u'''Форматирует text по ширине tty_sz. Первая строка выводится с first_prefix, остальные с other_prefix.
    Пустая строка воспринимается как начало абзаца (дополнительные 4 пробела). 
    %Ведущие пробелы и символы табуляции удаляются. Строки начинающиеся с # пропускаются.
    Любой спецсимвол в начале строки фиксирует начало строки.'''
    prefix, sz = first_prefix, 0
    for l in [ m.strip().replace('\t', ' ') for m in text.split('\n') if not m.lstrip().startswith('#') ]:
        if not l or (ord(l[0])<=127 and not l[0].isalnum() and sz): sys.stdout.write('\n'); sz = 0
        if not l: prefix = other_prefix+' '*4; continue
        while l:
            if sz: sys.stdout.write(' '); sz += 1
            else: sz = str_len(prefix); sys.stdout.write(prefix); prefix = other_prefix
            isp = (l[:tty_sz-sz].rfind(' ') if ' ' in l[:tty_sz-sz] else 0) if sz+str_len(l)>tty_sz else min(len(l), tty_sz-sz)
            if not isp: isp = min(len(l), tty_sz-sz)
            # if isp<len(l) and l[isp-1]!=' ': l = l[:isp-1]+'-'+l[isp-1:]
            sys.stdout.write(l[:isp]); sz += str_len(l[:isp]); l = l[isp:].lstrip()
            if l or sz==tty_sz: sys.stdout.write('\n'); sz = 0
    sys.stdout.write('\n'); sys.stdout.flush()
#-----------------------------------------------------------------------------------------------------------------------
def _out_help(self, opt, detail, tty_sz):
    v, k = getattr(self, opt), ', '.join(self._aliases.get(opt, [opt]))
    if k==opt and k[0]=='_': return
    doc, p1, p2 = (v.__doc__ if v.__doc__ else ''), '', ''        
    if not detail: doc, p1, p2 = doc.split('\n')[0], '+ ', '  '
    if isinstance(v, BASE): _out_format_text(opt+' --- '*bool(doc)+doc, p1, p2, tty_sz)
    elif callable(v): _out_format_text(_signature(v, opt)+' --- '*bool(doc)+doc, p2, p2, tty_sz)
    else: print p2+'%s=%r'%(k, v)     
#-----------------------------------------------------------------------------------------------------------------------
class BASE:
    _implicit_opt = None # опция активируемая по умолчанию
    _aliases = {}        # синонимы в виде {'имя опции':['синоним1', 'синоним2', ...]}
    _ignore_list = []    # список игнорируемых (как опции) атрибутов класса
    _opt_list = []       # дополнительный список опций
    def __init__(self, **D): [ setattr(self, k, v) for k, v in D.items() ]          
    def help(self, opt=None): 
        u'выводит детальную справку по опции opt или сводную справку по всему объекту'
        try: tty_sz = int(os.popen('stty size').readline().split()[1])
        except: tty_sz = 80
        if opt: _out_help(self, _recognize_argument(opt, _opt_list(self), self._aliases, True), True, tty_sz) 
        else: 
            hn = _HelpNamespace(self)
            if self.__doc__: _out_format_text(self.__doc__%hn, '', '', tty_sz)
            for k in sorted(set(_opt_list(self))-set(hn.L)): _out_help(self, k, False, tty_sz)
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
        _check_implicit_opt(self)
        first_call, order, name = True, list(order), (name if name else self.__class__.__name__)
        while arglist or mode or first_call:
            first_call = False # for single mode
            try: 
                # настройки readline
                _kw_stack.append([self]+[ _opt+i+( ' ' if not isinstance( getattr(self, r), BASE ) 
                                                   else ( '' if getattr( getattr(self, r), '_implicit_opt', '' ) else '.' ) ) 
                                          for i, r in zip(_opt_list(self), _opt_list(self))+
                                          sum([[ (j,k) for j in l ] for k, l in self._aliases.items() ],[]) if i[0]!='_' ])
                while not arglist and istream: # read options from istream
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
                if arglist[0] in (_end, _bra, _ket, _stdin, _file): # special options work
                    a = arglist.pop(0)
                    if a in (_stdin,_file):
                        L = []; loglist.append(repr(_stdin) if a==_stdin else "%r %r"%(_file, arglist[0]))
                        self(L, name, istream=sys.stdin if a==_stdin else open(arglist.pop(0)), 
                             order=order, args=args, kw_args=kw_args) # mode=mode or default ???
                        arglist[:0] = L
                    elif a==_ket and mode==-1: raise ParseError(4, name=name, braket=a)
                    elif mode:
                        loglist.append(repr(a))
                        if a==_bra: self(arglist, name, istream, order, args, kw_args, 1)
                        if a==_ket: break
                    else:
                        if a==_bra: loglist.append(repr(_bra)); mode = 1
                        if a==_ket: arglist.insert(0, a); break
                        if a==_end: loglist.append(repr(a)); break #return ?
                    continue
                a = arglist[0]
                if _is_ex_opt(a):
                    if mode==0: break
                    optname = _recognize_argument(a[len(_opt):], _opt_list(self), self._aliases, True)
                    if not callable(getattr(self, optname)): raise ParseError(5, name=name, attr=optname)
                    loglist.append(arglist.pop(0))
                    _call_method(self, optname, getattr(self, optname), arglist, istream) 
                    continue
                if _is_key_eq_val(a):
                    key, val = a.split(_eq, 1) 
                    key = _recognize_argument(key, _opt_list(self), self._aliases, kw_args is None)
                    if key: setattr(self, key, _type_convert(getattr(self, key), val))
                    elif not kw_args is None: kw_args[key] = val
                    else: raise ParseError(0, name=key, variants=[])
                    loglist.append(arglist.pop(0)) 
                    continue
                if _is_name(a):
                    optname = _recognize_argument(a, _opt_list(self), self._aliases, False)
                    if optname and callable(getattr(self, optname)):
                        loglist.append(arglist.pop(0))
                        _call_method(self, optname, getattr(self, optname), arglist, istream) 
                        continue
                if self._implicit_opt: 
                    loglist.append(self._implicit_opt)
                    _call_method(self, self._implicit_opt, getattr(self, _implicit_opt), arglist, istream)   
                    continue
                if order:
                    key = order.pop(0)
                    setattr(self, key, _type_convert(getattr(self, key), a))
                    loglist.append(key+_eq+arglist.pop(0)) 
                    continue
                raise ParseError(7, arg=a)
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
#-----------------------------------------------------------------------------------------------------------------------
class RadioSelect(BASE) : 
    def __init__( self, *statelist ) : 
        self.__doc__ = '%(_state)r, variants: '+'|'.join(map( str, statelist ))
        self._statelist, self._state, self._implicit_opt = map(str, statelist), str(statelist[0]), '_set'
    def _set( self, state ) : self._state = _recognize_word( state, self._statelist )
    def __str__(self) : return str(self._state)
    def __nonzero__(self) : return self._state!=self._statelist[0]
#-----------------------------------------------------------------------------------------------------------------------
class CheckSelect(BASE) :
    def __init__( self, **kw_args ) : self.__dict__.update([ ( k, ( bool(v) if k[0]!='_' else v ) ) for k, v in kw_args.items() ])
#-----------------------------------------------------------------------------------------------------------------------
#   READLINE
#-----------------------------------------------------------------------------------------------------------------------
def _rl_completer( l, state ) :
    if state : return _last_complete_list[state]
    if _dot and _dot in l and all(map( _is_ex_opt_or_name, l.split(_dot) )[:-1]) : #??? aliases ???
        sub, start = _kw_stack[-1][0], ''
        for ll in [ i[len(_opt):] if i.startswith(_opt) else i for i in l.split(_dot) ][:-1] : 
            sub, start = getattr( sub, ll, None ) if ll[0]!='_' and isinstance( sub, BASE ) else None, start+_dot+ll
        start = _opt+start[len(_dot):]+_dot; ll = l.split(_dot)[-1]; ll = ll[len(_opt):] if ll.startswith(_opt) else ll
        _last_complete_list[:] = [ start+i for i in dir(sub)+sum( sub._aliases.values(), [] ) if i[0]!='_' and ( not ll or i.startswith( ll ) ) ]
    else : _last_complete_list[:] = filter( lambda i : not l or i.startswith( l if l.startswith(_opt) else _opt+l ), _kw_stack[-1][1:] )
#    B, io = _kw_stack[-1][0], []
#     while isinstance(B,BASE) : 
#         if B._implicit_opt : io.append( B._aliases.get( B._implicit_opt, [B._implicit_opt] )[0] ); B = getattr( B, B._implicit_opt, None )
#         if isinstance(B,BASE) : _last_complete_list.extend([ _opt+_dot.join(io+[i]) for i in sum(B._aliases.values(),dir(B)) 
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
