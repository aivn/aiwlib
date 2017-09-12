# -*- coding: utf-8 -*-
'''Bind Python code to shell.
Copyright (C) 2010, 2012 Anton V. Ivanov <aiv.racs@gmail.com>
This code is released under the GPL3 or later
'''
#-----------------------------------------------------------------------------------------------------------------------
import os, sys, fnmatch, atexit, subprocess, re 
from types import MethodType, FunctionType, BooleanType

_opt, _dot, _end, _bra, _ket, _stdin, _file, _help, _help_all, _eq, _space, _mode = '--', '.', ';', '{', '}', '--', '++', '?', '?+', '=', '', 0
BO_GREED_ARGS, BO_GREED_KW_ARGS = 1, 2
_exception_arglist = []
#_hard_opt_started, _logstream = True, None #_use_kw_args = True
#-----------------------------------------------------------------------------------------------------------------------
class ParseError(Exception) :
    table = [ 'option "%(name)s" unrecognized ( variants=%(variants)s )!', #0
              'can\'t convert "%(value)s" to bool, can use y, Y, yes, Yes, YES, on, On, ON, true, True, TRUE, 1, ' 
              'n, N, no, No, NO, off, Off, OFF, false, False, FALSE, 0', #1
              'terminate option "%(name)s" without args (%(args)s)', #2
              '%(signature)s got multiple values for keyword parameter "%(par)s"', #3
              'incorrectly used %(braket)r in %(name)s', #4
              'argument %(arg)r unrecognized', #5
              'type(%(name)s) in "%(chain)s" is %(T)s, but BASE subclass is expected!', #6
              'activate implicit option %(opt)r by illegal keyword argument %(arg)r' ] #7
    def __init__( self, num, **kw_args ) : self.num = num; self.__dict__.update(kw_args); Exception.__init__( self, ' [E.%i] '%num+self.table[num]%kw_args )
#-----------------------------------------------------------------------------------------------------------------------
def _recognize_word( word, wordlist ) : 
    variants = filter( lambda o : o==word, wordlist )
    if not variants : variants = filter( lambda o : o.startswith(word), wordlist )
    if not variants : reg = re.compile( '^'+'[aAeEiIoOuU]{0,}'.join(word)+'.*' ); variants = filter(reg.search, wordlist)
    if not variants : reg = '*'.join(word)+'*'; variants = filter( lambda o : fnmatch.fnmatch(o, reg), wordlist )
    if len(variants) != 1 : raise ParseError( 0, name=word, variants=variants )
    return variants[0]
def _recognize_option( s, options, aliases={}, use_abbrv=False ) :
    'recognize option $s from $options and $aliases'
    options = filter( lambda s : not s.startswith('_'), list(options) ) + sum( aliases.values(), [] )
    if _opt and s.startswith(_opt) : s, use_abbrv = s[len(_opt):], True
    if use_abbrv : res = _recognize_word( s, options )
    elif s in options : res = s
    else : return
    for k, v in aliases.items() :
        if res in v : return k
    return res
#-----------------------------------------------------------------------------------------------------------------------
def _type_convert( src, tgt ) :
    'convert string $tgt to type of value $src'
    if src is None or type(src)==str : return tgt if tgt!='None' else None #???
    if src.__class__ is BooleanType : 
        if tgt in ( 'y', 'Y', 'yes', 'Yes', 'YES', 'on',  'On',  'ON',  'true',  'True',  'TRUE',  '1' ) : return True
        if tgt in ( 'n', 'N', 'no',  'No',  'NO',  'off', 'Off', 'OFF', 'false', 'False', 'FALSE', '0' ) : return False
        raise ParseError( 1, value=tgt )
    return src.__class__(tgt)
#-----------------------------------------------------------------------------------------------------------------------
def _func_info(f) : 
    'return names, defaults, args-name, kw_args-name for function $f'
    argc, names, defaults, flags = f.func_code.co_argcount, list(f.func_code.co_varnames), f.func_defaults if f.func_defaults else (), f.func_code.co_flags
    if type(f)==MethodType : argc -= 1; del names[0]
#    print argc, names, ( names[argc+1] if flags & 0x04 else None ), ( names[argc+1+bool(flags & 0x04)] if flags & 0x08 else None )
    return names[:argc], list(defaults), ( names[argc] if flags & 0x04 else None ), ( names[argc+bool(flags & 0x04)] if flags & 0x08 else None )
#-----------------------------------------------------------------------------------------------------------------------
def _signature( f, n=None ) :
    'return signature of function $f with name $n'
    names, defs, args, kw_args = _func_info(f)
    S = ', '.join( names[:len(names)-len(defs)]+[ '%s=%r'%i for i in zip( names[-len(defs):], defs ) ]+ ['*%s'%args]*bool(args) +['**%s'%kw_args]*bool(kw_args) )
    return ( f.__name__ if n==None else n ) + ( '(%s)'%S if S else '()' )
#-----------------------------------------------------------------------------------------------------------------------
#_is_name = lambda a : not a.startswith('_') and a.replace('_','').isalnum()
_is_name = lambda a : a and a[0].isalpha() and a.replace('_','').isalnum()
_is_ex_opt = lambda a : _opt and a.startswith(_opt) and _is_name( a[len(_opt):] )
_is_ex_opt_or_name = lambda a : _is_name(a) or _is_ex_opt(a)
_is_key_eq_val = lambda a : _eq and _eq in a and not a[a.index(_eq):].startswith(_eq+_eq)
#_opt_list = lambda self : set( dir(self)+self._opt_list+getattr( self, '__swig_setmethods__', {} ).keys() )-set(self._ignore_list)
_opt_list = lambda self : ( set( dir(self)+self._opt_list+self.__swig_setmethods__.keys() )-set(['this']) if hasattr( self, '__swig_setmethods__' ) else
                            set( dir(self)+self._opt_list ) )-set(self._ignore_list)
#-----------------------------------------------------------------------------------------------------------------------
def _read_opt_args( self, name, func, arglist, outlog, implicit_opt ) :
    func = func if type(func) in (MethodType,FunctionType) else func.__call__
    names, defs, args_name, kw_args_name = _func_info(func); defs, args, kw_args, pos, alen = dict(zip( names[-len(defs):], defs )), [], {}, 0, len(names)-len(defs)
    while arglist : 
        if _space and _space in arglist[0] : arglist[:1] = arglist[0].split( _space, 1 )
        a = arglist[0]
        if a in ( _help, _help_all ) : #help
            _show_help( func, a==_help_all, [(name,func)]+kw_args.items()+[('',i) for i in args] )
            del arglist[0]; return 
        if a in ( _end, _bra, _ket, _stdin, _file ) : #terminate
            if pos<alen : raise ParseError( 2, name=name, args=', '.join([ k for k in names[:alen] if not k in kw_args ]) )
            break
        if _is_key_eq_val(a) and _is_ex_opt_or_name(a[:a.index(_eq)]) : # parametr = value
            k, v = a.split( _eq, 1 )
            try: k = _recognize_option( k, filter( lambda i : not i in kw_args, names ) ) #<-use_abbrv
            except ParseError, e : k = None
            if implicit_opt and not k and not kw_args_name and not _mode&BO_GREED_KW_ARGS and not pos : raise ParseError( 7, opt=name, arg=a )
            if not k and kw_args_name : k = a.split(_eq)[0][( len(_opt) if _opt and a.startswith(_opt) else 0 ):]
            elif not k and pos>=alen and not _mode&BO_GREED_KW_ARGS : break
        else :  k, v = ( names[pos] if pos<len(names) else '', a ) # k='' for *L
        if k in kw_args : raise ParseError( 3, signature=_signature( func, name ), par=k )
        if pos>=alen and not k and ( ( _is_ex_opt(a) and not _mode&BO_GREED_ARGS ) or not ( pos==len(names) and args_name ) ) : break  # ???
        if not k and args_name and pos==len(names) : args.append( arglist.pop(0) )
        else :
            if not k : k, v = names[pos], a
            kw_args[k] = _type_convert( defs.get(k), v ); del arglist[0] 
            pos = min(filter( lambda i : not names[i] in kw_args, range(len(names)) )+[len(names)])
    if pos<alen : raise ParseError( 2, name=name, args=', '.join([ k for k in names[:alen] if not k in kw_args ]) )
    args, kw_args = [ kw_args[n] for n in names[:pos] ]+args, filter( lambda kv : not kv[0] in names[:pos], kw_args.items() ); res = func( *args, **dict(kw_args) )
    if outlog : loglist[-1] += ' '+' '.join([ '%s%s%r'%( k, _eq, v ) for k, v in kw_args ])+' '+' '.join(map( repr, args ))  
    return res
#-----------------------------------------------------------------------------------------------------------------------
def _add_subopt_log( a, mode ) :
    if not mode : loglist[-1] += ' '+( a.split(_dot)[0]  if _dot else a )
    elif a : loglist.append( a.split(_dot)[0] if _dot else a ) 
#-----------------------------------------------------------------------------------------------------------------------
class BASE :
    _implicit_opt, _aliases, _always_use_abbrv, _opt_list, _ignore_list = None, {}, [], [], []
    def __init__( self, **D ) : [ setattr( self, k, v ) for k, v in D.items() ]          
    def __call__( self, arglist, name=None, istream=None, mode=-1 ) : #mode =  -1 (default) | 0 (single) | 1 (open braket)
        first_call = True; name = name if name else self.__class__.__name__         
        while mode or first_call :
            first_call = False 
            try : 
                _kw_stack.append([self]+[ _opt+i+( ' ' if not isinstance( getattr(self,r), BASE ) 
                                                   else ( '' if getattr( getattr(self,r), '_implicit_opt', '' ) else '.' ) ) 
                                          for i, r in zip(_opt_list(self),_opt_list(self))+
                                          sum([[ (j,k) for j in l ] for k, l in self._aliases.items() ],[]) if i[0]!='_' ])
                while not arglist and istream : # read options from istream
                    L, read_next_line = [], True
                    while read_next_line :
                        l = raw_input( '... ' if L else '%s> '%name )+'\n' if os.isatty(istream.fileno()) and os.isatty(sys.stdout.fileno()) else istream.readline()
                        read_next_line = l and l.rstrip().endswith('\\'); L.append( l.rstrip()[:-1] if read_next_line else l.rstrip() )
                    #arglist.extend([ i[:-1] for i in os.popen( 'for i in %s \ndo echo "$i" \ndone'%' '.join(L) ).readlines() ])
                    shell = subprocess.Popen( os.environ['SHELL'], stdout=subprocess.PIPE, stdin=subprocess.PIPE ) 
                    shell.stdin.write( 'for i in %s \ndo echo "$i" \ndone\n'%' '.join(L) ); shell.stdin.close()
                    arglist.extend([ i[:-1] for i in shell.stdout.readlines() ]); del shell
                    if not l : break
                if not arglist : break #return ?
                if _space and _space in arglist[0] : arglist[:1] = arglist[0].split( _space, 1 )
                if arglist[0] in ( _end, _bra, _ket, _stdin, _file, _help, _help_all ) :
                    a = arglist.pop(0)
                    if a==_stdin : L = []; loglist.append(repr(_stdin)); self( L, name, sys.stdin ); arglist[:0] = L
                    if a==_file : L = []; loglist.append( "%r %r"%( _file, arglist[0] ) ); self( L, name, open(arglist.pop(0)) ); arglist[:0] = L
                    if ( a==_bra and mode ) or ( a==_ket and mode-1 ) : raise ParseError( 4, name=name, braket=a )
                    if a==_bra : loglist.append(repr(_bra)); mode += 1
                    if a==_ket : loglist.append(repr(_ket)); break #return ?
                    if a in ( _help, _help_all ): 
                        _show_help( self, a==_help_all, [] ); del arglist[:] 
                        if not mode : del loglist[-1]; break #return ?
                    continue
                a, opt = arglist[0], None; aend = min(( a.index(_eq) if _is_key_eq_val(a) else len(a) ), ( a.index(_dot) if _dot and _dot in a else len(a) )) 
                if _is_ex_opt_or_name(a[:aend]) : opt = _recognize_option( a[:aend], _opt_list(self), self._aliases )
                if opt and _eq and a[aend:].startswith(_eq) : # parametr = value
                    del arglist[0]; attr = getattr( self, opt ) 
                    if callable(attr) : _read_opt_args( self, opt, attr, a.split( _eq, 1 )[1:], False, False )
                    else : setattr( self, opt, _type_convert( attr, a.split( _eq, 1 )[1] ) )
                    loglist.append(a); continue
                if opt and _dot and a[aend:].startswith(_dot) : # opt.subopt...
                    arglist[0] = a.split( _dot, 1 )[1]; attr = getattr( self, opt ) 
                    if isinstance( attr, BASE ) : _add_subopt_log( a, mode ); attr( arglist, name=opt, istream=istream, mode=0 ); continue
                    else : raise ParseError( 6, name=opt, chain=a, T=attr.__class__.__name__ )
                if opt : del arglist[0]; lopt, implicit_opt = a, False
                else : opt, lopt, implicit_opt = self._implicit_opt, '', True # activate ipmlicit option 
                if not opt : raise ParseError( 5, arg=a )
                attr = getattr( self, opt )
                if isinstance( attr, BASE ) : _add_subopt_log( lopt, mode ); attr( arglist, name=opt, istream=istream, mode=0 )
                elif callable(attr) : 
                    try: loglist.append(lopt); res = _read_opt_args( self, opt, attr, arglist, True, implicit_opt )
                    except : del loglist[-1]; raise
                    if isinstance( res, BASE ) : res( arglist, name=opt, istream=istream, mode=0 )
                elif not arglist or arglist[0] in ( _end, _bra, _ket, _stdin, _file ) : raise ParseError( 2, name=opt, args='' )
                elif arglist[0] in ( _help, _help_all ) : del arglist[0]; print '%s=%r'%( opt, attr )
                else : setattr( self, opt, _type_convert( attr, arglist[0] ) ); loglist.append('%s %r'%( lopt, arglist.pop(0) ))
                #if not mode : break #return ?
            except EOFError, e : sys.stdout.write('\n'); return
            except KeyboardInterrupt, e :
                if not mode : del loglist[-1]
                if mode and istream and os.isatty(istream.fileno()) and os.isatty(sys.stdout.fileno()) : print '^C'
                else : raise
            except Exception, e :
                if not mode : del loglist[-1]
                if mode and istream and os.isatty(istream.fileno()) and os.isatty(sys.stdout.fileno()) : 
                    lt, lv, tb = ( sys.last_type, sys.last_value, sys.last_traceback ) if sys.exc_info() == (None,)*3 else sys.exc_info()
                    while tb and tb.tb_frame.f_code.co_filename != __file__.rstrip('c') : 
                        print 123
                        sys.stderr.write( '\tFile "%s", line %s, in %s\n'%( tb.tb_frame.f_code.co_filename, tb.tb_lineno, tb.tb_frame.f_code.co_name ) )
                        tb = tb.tb_next; 
                    sys.stderr.write( '>>> %s <<< %s\n%s:%s\n'%( a, ' '.join(map( repr, arglist)),  lt.__name__, lv ) )
                else : _exception_arglist[:] = arglist; raise 
                del arglist[:]
            finally : del _kw_stack[-1]
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
#   HELP
#-----------------------------------------------------------------------------------------------------------------------
def _show_help_old( self, showall, level=[] ) : 
    ns, prefix, L, R =  _help_namespace(self), ''.join([ '|  ' if i else '   ' for i in level ]), _opt_list(self), []
    if self.__doc__ : print '\n'.join([ prefix+l for l in ( self.__doc__%ns ).split('\n') ])
    L = [ ( ', '.join(filter( lambda k : k[0]!='_', [i]+self._aliases.get(i,[]) )), i ) for i in L if i not in ns.L ] 
    L = [ ( i[0], getattr( self, i[1] ) ) for i in L if i[0] ]; L.sort()
    for i in range(len(L)) :
        n, o = L[i]
        if level : level[-1] = any([ isinstance( k[1], BASE ) for k in L[i+1:] ])
        prefix = ''.join([ '|  ' if j else '   ' for j in level ]) 
        if isinstance( o, BASE ) : 
            if showall : sys.stdout.write( ''.join([ '|  ' if i else '   ' for i in level[:-1] ])+'+--'*bool(level)+n+'\n' ); _show_help( o, True, level=level+[1] )
            else : print '+', n, '--- %s'%o.__doc__%_help_namespace(o)*bool(o.__doc__)
        elif callable(o) : 
            print ( prefix if showall else '  ' )+_signature( o if type(o) in ( MethodType, FunctionType ) else o.__call__, n ), '--- %s'%o.__doc__ if o.__doc__ else ''
        else : print ( prefix if showall else '  ' )+'%s=%r'%(n,o)    
    if self._implicit_opt and any([ o[0]!='_' for o in [self._implicit_opt]+self._aliases.get(self._implicit_opt, []) ]): 
        print prefix+'SET %r AS IMPLICIT OPTION'%', '.join( self._aliases.get( self._implicit_opt, [self._implicit_opt] ) )
    if any((_opt, _end, _bra, _ket, _stdin, _file, _help, _help_all)) and showall and not level : print 'Also, use the sequence', ', '.join(['%r '%k+t for t, k in [
                ('as a prefix option',_opt), ('to complete the argument list options',_end), 
                ('to begin the argument group suboptions',_bra), ('to finish the argument group suboptions',_ket),
                ('to read the options from stdin',_stdin), (' FILENAME to read the options from FILENAME',_file), ('to show help',_help),
                ('to show full help',_help_all)] if k])

str_len = lambda S : int(reduce( lambda r, c : r+1./(1+(ord(c)>127))+4*(c=='\t'), S, 0 ))
def _show_help( self, showall, R ): 
    'в минимальной версии показывается только первая строка документации для каждого элемента, строки начинающиеся с # игнорируются, новый параграф начинается с "-"'
    doc = lambda o: [ l.strip() for l in ( o.__doc__%_help_namespace(o) if isinstance(o, BASE) else o.__doc__ ).split('\n')[:None if showall else 1] 
                      if not l.strip().startswith('#') ] if o.__doc__ else []
    if isinstance(self, BASE):
        if self.__doc__ : R.append(( '', doc(self) )) #<<< doc header
        ns = _help_namespace(self) 
        L = [ ( ', '.join(filter( lambda k : k[0]!='_', [i]+self._aliases.get(i,[]) )), i ) for i in _opt_list(self) if i not in ns.L ] 
        L = [ ( i[0], getattr( self, i[1] ) ) for i in L if i[0] ]; L.sort()
    else: L, R = R, []
    for n, o in L:
        n, d = (' + ' if isinstance(o, BASE) or ( type(o) not in (bool,str,int,long,float) and o.__doc__ and '\n' in o.__doc__ ) else '   ')+n, doc(o)
        if isinstance(o, BASE): R.append(( n+': ', d ))
        elif callable(o): R.append(( _signature( o if type(o) in ( MethodType, FunctionType ) else o.__call__, n )+': ', d ))
        elif not isinstance(o, BASE): R.append(( n+'=%r'%o, [''] ))
    if isinstance(self, BASE) and self._implicit_opt and any([ o[0]!='_' for o in [self._implicit_opt]+self._aliases.get(self._implicit_opt, []) ]): 
        R.append(( '', ['SET %r AS IMPLICIT OPTION'%', '.join( self._aliases.get( self._implicit_opt, [self._implicit_opt] ) )] ))
    if any((_opt, _end, _bra, _ket, _stdin, _file, _help, _help_all)):  
        R.append(( '', ['Also, use the sequence'+ 
                        ', '.join(['%r '%k+t for t, k in [('as a prefix option',_opt), ('to complete the argument list options',_end), 
                                                          ('to begin the argument group suboptions',_bra), ('to finish the argument group suboptions',_ket),
                                                          ('to read the options from stdin',_stdin), (' FILENAME to read the options from FILENAME',_file), 
                                                          ('to show help',_help), ('to show full help',_help_all)] if k])] ))
    try: width = int( os.popen('stty size').readline().split()[1] )
    except: width = 80
    for p, L in R:
        sys.stdout.write(p); lp, buf = str_len(p), []
        while L:
            while L and not L[0].startswith('-'): buf += L.pop(0).split(' ')
            while buf:
                while buf and lp+str_len(buf[0])+1<width : sys.stdout.write(buf[0]+' '); lp += str_len(buf.pop(0))+1
                lp = ( 6 if p else 0 )+4*bool(not buf); sys.stdout.write( '\n'+' '*lp*bool( L or buf ) ) 
            if L: L[0] = L[0][1:]
    sys.stdout.flush()
#-----------------------------------------------------------------------------------------------------------------------
class _help_namespace : 
    def __init__( self, obj ) : self.obj, self.L = obj, []
    def __getitem__( self, key ) : self.L.append(key.rstrip('=')); v = getattr( self.obj, self.L[-1], '???' ); return '%s=%r'%(self.L[-1],v) if key[-1] == '=' else v
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
