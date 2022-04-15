# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

#-------------------------------------------------------------------------------
table, params, __all__ = [], [], ['get_frame', 'get_params', 'table_size', 'file_size', 'load_files', 'get_frames_list']

import glob, sys, json

try: from . import core
except ImportError as e: core = False; print('Module qplt.core is not available:', e)
if core: core.qplt_global_init()

try: from . import remote
except ImportError as e: remote = False; print('Module qplt.remote is not available:', e)
#-------------------------------------------------------------------------------
def table_size(): return len(table)
def file_size(fileID): return len(table[fileID])
def get_frame(fileID, frameID): return table[fileID][frameID]
def get_params(fileID): return params[fileID]
def get_frames_list(fileID): return table[fileID]
# убрать фрейм/файл из таблицы, обновить фрейм/файл?
#-------------------------------------------------------------------------------
def load_local_file(fname, touch_mode, P):  # --> frames count
    if core: 
        for f in glob.glob(fname):
            L = list(core.factory(bytes(f, 'utf8')))  # <-- std::vector<BaseContainer*>
            if L:
                for f in L: f.touch_mode = touch_mode
                touch_mode = None; table.append(L); params.append(P) 
                if not getattr(core, 'mem_limit', None) is None: core.QpltContainer.mem_limit.fset(core.mem_limit); core.mem_limit = None
            else: print('load [', fname, '] --- skipped')
    else: print('\tModule qplt.core is not imported, local file [', fname, '] skipped')
#-------------------------------------------------------------------------------
def load_remote_file(fname, touch_mode, **server):  # --> frames count
    if remote:
        u, p = server.get('user', ''), server.get('port')
        for L in remote.factory(fname, **server):
            if L:
                for f in L: f.touch_mode = touch_mode
                touch_mode = None; table.append(L); params.append({'file': (u+'@')*bool(u)+server['host']+('/%s'%p)*(not p is None)+':'+fname})
            else: print('load', fname, server, '--- skipped')
    else: print('\tModule qplt.remote is not imported, file [', fname, '] by connect', server, ' skipped')
#-------------------------------------------------------------------------------
def parse_server(arg):
    server = {}
    if '@' in arg: server['user'], arg = arg.split('@')
    if '/' in arg: arg, server['port'] = arg.split('/'); server['port'] = int(server['port'])
    server['host'] = arg
    return server
#-------------------------------------------------------------------------------
def load_files(*args):
    '''-f <PATH.json> -t[<NUM>] -T[<NUM>] -m<NUM> [[user@]host[/port]:]path | [user@]host[/port]: paths ... | :  
в люом порядке, отдельное указание сервера действует на все последующие файлы, сбрасывается двоеточием'''
    server, touch_mode = None, None
    for arg in args:
        tpos = len(table)
        if arg==':': server = None; continue
        elif arg.startswith('-m'):
            try:
                mem_limit = float(arg[2:])
                if remote: remote.mem_limit = mem_limit
                if core and not hasattr(core, 'mem_limit'): core.mem_limit = mem_limit
            except ValueError as e: print(e, '--- skipped incorrect mem_limit argumet', arg)
        elif arg.startswith('-t'): touch_mode = (1, float(arg[2:]) if len(arg)>2 else .5)
        elif arg.startswith('-T'): touch_mode = (2, float(arg[2:]) if len(arg)>2 else .5)
        elif arg.endswith(':'): server = parse_server(arg[:-1]); continue
        elif ':' in arg: srv, fname = arg.split(':', 1); load_remote_file(fname, touch_mode, **parse_server(srv)); touch_mode = None
        elif arg.startswith('-f'):
            try:
                for D in json.load(open(arg[2:])): load_local_file(D['file'], touch_mode, D); touch_mode = None
            except: print('json file %r ingnored'%arg[2:])
        else: load_remote_file(arg, **server) if server else load_local_file(arg, touch_mode, {'file':arg}); touch_mode = None
        for f in table[tpos:]: print('Load', f[0].fname().decode(), '---', len(f), 'frames')
    print('Totally loaded', sum(len(f) for f in table), 'frames from', len(table), 'files')
#-------------------------------------------------------------------------------

