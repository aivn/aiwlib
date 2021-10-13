# -*- coding: utf-8 -*-
'''Copyright (C) 2003-2017 Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

import os, sys, hashlib, gzip
from mixt import *
#-------------------------------------------------------------------------------
def get_py_sources(is_user=lambda f: not f.startswith('/usr/')):
    'возвращает словарь вида {module-name: module-source-path-file} для всех модулей, отмеченных в sys.modules'
    sources, wdir = {}, os.getcwd()
    for name, module in sys.modules.items():
        if not hasattr(module, '__file__'): continue
        mfile = os.path.normpath(module.__file__)
        if mfile.endswith('.pyc'): mfile = mfile[:-1]
        if not os.path.exists(mfile): continue
        if os.path.commonprefix((mfile, wdir))==wdir: mfile = mfile[len(wdir)+1:]
        if is_user(mfile): sources[name] = mfile
    return sources
#-------------------------------------------------------------------------------
def get_so_sources(name):
    'возвращает список исходных файлов для C++-модуля name, собранного при помощи SWIG и aiwlib'
    makefile = getattr(sys.modules[name], '__makefile__', None)
    if not makefile or not name in sys.modules: return []
    olddir, makedir = os.getcwd(), os.path.dirname(makefile)
    if makedir: os.chdir(makedir)
    try: return os.popen('make -f "%s" sources'%makefile).readline().split()    
    finally: os.chdir(olddir)
#-------------------------------------------------------------------------------
def get_sources(add_src_list=[]):
    'возвращает список исходных файлов проекта, включая исходные файлы C++-модулей собранных при помощи SWIG и aiwlib'
    srcD, srcL = get_py_sources(), []
    for n, f in srcD.items():
        if not n in srcD: continue
        if f.endswith('.so') and os.path.basename(n)[0]=='_': 
            del srcD[n], srcD[n[1:]]
            srcL += get_so_sources(n)
    return add_src_list+srcL+srcD.values()
#-------------------------------------------------------------------------------
def commit(path, add_src_list=[]):
    'создает файлы path/.src.tgz и path/.src.md5, возвращает md5-сумму незапакованного архива с исходниками'
    srcL = get_sources(add_src_list) 
    srcS, statL = ' '.join('"%s"'%f for f in srcL), map(os.stat, srcL)
    md5L = os.popen('md5sum '+srcS).readlines()
    open(path+'.src.md5', 'w').write(
        ''.join(time.strftime('%s %%Y.%%m.%%d-%%H:%%M:%%S %s'%(s[6], l), time.gmtime(s[8])) for s, l in zip(statL, md5L)))
    tar = os.popen('tar -Pc '+srcS).read()
    md5 = hashlib.md5(tar).hexdigest()
    gzip.open(path+'.src.tgz', 'w').write(tar)
    return md5
#-------------------------------------------------------------------------------
class SrcTable: 
    def __init__(self, path): pass # кэширование по md5-sum?
#-------------------------------------------------------------------------------
class SrcFile:  
    def __init__(self, name, size, mtime, md5): pass # кэширование по md5-sum?
    def __getitem__(self, i): pass # возвращает i-ю строку
    def rect(self, y_min, y_max, x_min, x_max): pass
#-------------------------------------------------------------------------------
