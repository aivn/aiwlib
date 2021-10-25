# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

#-------------------------------------------------------------------------------
table, __all__ = [], ['get_frame', 'table_size', 'file_size', 'load_files']

import glob
try: from . import core
except ImportError as e: core = False; print(e)
try: from . import remote
except ImportError as e: remote = False; print(e)
#-------------------------------------------------------------------------------
def table_size(): return len(table)
def file_size(fileID): return len(table[fileID])
def get_frame(fileID, frameID): return table[fileID][frameID]
# убрать фрейм/файл из таблицы, обновить фрейм/файл?
#-------------------------------------------------------------------------------
def load_local_file(fname):  # --> frames count
    if core: 
        for f in glob.glob(fname):
            table.append(core.factory(bytes(f, 'utf8'))) # <-- std::vector<BaseContainer*>
            if not table[-1]: del table[-1]; print('load', fname, '--- skipped')
    else: print('\tqplt core not imported, local file [', fname, '] skipped')
#-------------------------------------------------------------------------------
def load_remote_file(fname, **server):  # --> frames count
    if remote:
        for f in remote.factory(fname, **server):
            if f: table.append(f)
            else: print('load', fname, server, '--- skipped')
    else: print('\tremote module not imported, file [', fname, '] by connect', server, ' skipped')
#-------------------------------------------------------------------------------
def parse_server(arg):
    server = {}
    if '@' in arg: server['user'], arg = arg.split('@')
    if '/' in server: arg, server['port'] = arg.split('/')
    server['host'] = arg
    return server
#-------------------------------------------------------------------------------
def load_files(*args):
    '''[[user@]host[/port]:]path | [user@]host[/port]: paths ... | : 
в люом порядке, отдельное указание сервера действует на все последующие файлы, сбрасывается двоеточием'''
    server = None
    for arg in args:
        tpos = len(table)
        if arg==':': server = None; continue
        elif arg.endswith(':'): server = parse_server(arg[:-1]); continue
        elif ':' in arg: srv, fname = arg.split(':', 1); load_remote_file(fname, **parse_server(srv))
        else: load_remote_file(arg, **server) if server else load_local_file(arg)
        for f in table[tpos:]: print('load', f[0].fname().decode(), '---', len(f), 'frames')
#-------------------------------------------------------------------------------

