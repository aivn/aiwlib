# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
from socket import *
from .mixt import except_report
import sys, struct, fnmatch, thread
#-------------------------------------------------------------------------------
#   TRANSPORT PROTOCOL
#-------------------------------------------------------------------------------
_pack_format_table = { int: lambda x: 'i', 
                       bool: lambda x: 'i', 
                       float: lambda x: 'd', 
                       str: lambda x: '%ss'%len(x) 
                       }
class PackException(Exception): pass
def _pack_error(x): raise PackException("cann't pack object %r"%x)
#-------------------------------------------------------------------------------
def _pack_for_send(*data):
    fmt = ''.join([ _pack_format_table.get(type(x), _pack_error)(x) for x in data ])
    pack_fmt = 'ii%is'%len(fmt)+fmt
    return struct.pack(pack_fmt, *((struct.calcsize(pack_fmt), len(fmt), fmt)+data))
def _unpack_body(data):
    fmt_sz = struct.unpack_from('i', data, 0)[0]
    fmt = struct.unpack_from('%is'%fmt_sz, data, 4)[0]
    return struct.unpack_from('%is'%fmt_sz+fmt, data, 4)[1:]
#-------------------------------------------------------------------------------
class SocketClosed(Exception): pass
def _send(connect, *data): connect.send(_pack_for_send(*data))
def _recv(connect):
    try: sz = struct.unpack('i', connect.recv(4))[0]-4
    except: raise SocketClosed()
    data = connect.recv(sz) if sz else ''
    while len(data)<sz: data += connect.recv(sz-len(data))
    return _unpack_body(data)
#-------------------------------------------------------------------------------
def _send_result(connect, res):
    if res is None: _send(connect, 'N')
    elif type(res) in (list, tuple): _send(connect , 'T', *res)
    else: _send(connect , 'R', res)
def _recv_result(connect):
    res = _recv(connect)
    if res[0]=='E':
        print>>sys.stderr, '*****   SERVER ERROR   *****'
        for i in res[1:]: print>>sys.stderr, i,
        print>>sys.stderr, '****************************'
        raise Exception(res[-1])
    elif res[0]=='T': return res[1:]
    elif res[0]=='R': return res[1]
#-------------------------------------------------------------------------------
#   SERVER
#-------------------------------------------------------------------------------
def _server_func(connect, IP, UserClass):
    try:
        Q = _recv(connect) # count of init args, [func_name, func_args...]
        UC = UserClass(*Q[1:1+Q[0]])
        if Q[0]!=-1 and Q[0]+1<len(Q): # call function
            func = Q[1+Q[0]]
            if func[0]=='_' or not func in dir(UC): raise Exception('incorrect attribute name %r'%func) 
            _send_result(connect, getattr(UC, func)(*Q[2+Q[0]:])) 
        else:
            _send_result(connect, [ f for f in dir(UC) if f[0]!='_' and callable(getattr(UC, f)) ]) # send method list
            while 1:
                Q = _recv(connect)
                if Q[1][0]=='_' or not Q[1] in dir(UC): raise Exception('incorrect attribute name %r'%Q[1]) 
                if Q[0]=='S': setattr(UC, Q[1], Q[2]); _send_result(connect, None)
                elif Q[0]=='G': _send_result(connect, getattr(UC, Q[1]))
                elif Q[0]=='C': _send_result(connect, getattr(UC, Q[1])(*Q[2:]))
    except SocketClosed, e: pass
    except Exception, e: _send(connect, 'E', *except_report())
    connect.close()
#-------------------------------------------------------------------------------
def mainloop(serverIP, entryIPs, port, UserClass):
    server = socket(AF_INET, SOCK_STREAM); server.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    server.bind((serverIP, port)); server.listen(5)
    while 1 :
        connect, address = server.accept(); IP = address[0]
        if any(fnmatch.fnmatch(IP, p) for p in entryIPs): thread.start_new_thread(_server_func, (connect, IP, UserClass))
        else: _send(connect, 'E', 'Connect from %s to %s port %s locked'%(IP, servIP, port))
#-------------------------------------------------------------------------------
#   CLIENT
#-------------------------------------------------------------------------------
def rpc_call(IP, port, UC_args, function, *args):
    connect = socket(AF_INET, SOCK_STREAM)
    connect.connect((IP, port))
    _send(connect, len(UC_args), *tuple(UC_args)+(function,)+args)
    return _recv_result(connect)
#-------------------------------------------------------------------------------
class Client:
    def __init__(self, IP, port, *args): 
        self.__dict__['_connect'] = socket(AF_INET, SOCK_STREAM)
        self._connect.connect((IP, port))
        _send(self._connect, len(args), *args)
        self.__dict__['_methods'] = _recv_result(self._connect)
    def __setattr__(self, attr, value):
        _send(self._connect, 'S', attr, value)
        _recv_result(self._connect)
    def __getattr__(self, attr):
        if attr in self._methods: 
            def _call(*args):
                _send(self._connect, 'C', attr, *args)
                return _recv_result(self._connect)
            return _call                
        _send(self._connect, 'G', attr)
        return _recv_result(self._connect)
#-------------------------------------------------------------------------------
