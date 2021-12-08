#!/usr/bin/python3

import os, sys

fname = sys.argv[1]
src, itable = open(fname).readlines(), []

if fname.endswith('_wrap.cxx'):
    for i0, l in enumerate(src):
        l = l.split()
        if 'SWIG_init' in l: itable.append((i0, l[-1]))
        if len(l)==3 and l[0]=='#define' and l[1]=='SWIG_name': break
    name = l[2][1:-1]
    src[i0:i0+1] = ['#ifndef EBUG\n', '#define SWIG_name "%s"\n'%name, '#else\n', '#define SWIG_name "_dbg%s"\n'%name, '#endif\n']
    for i, n in reversed(itable):
        src[i:i+1] = ['#ifndef EBUG\n', '#define SWIG_init %s\n'%n, '#else\n', '#define SWIG_init %s_dbg_%s\n'%tuple(n.rsplit('_', 1)), '#endif\n']
        
    open(fname, 'w').writelines(src)

elif fname.endswith('.py'):
    name = '_'+os.path.basename(fname).split('.')[0]
    for i0, l in enumerate(src):
        if l.split()[-2:] == ['import', name]: itable.append(i0)
    for i in reversed(itable[:2]):
        l = src[i]; p, l = l[:len(l)-len(l.lstrip())], ' '.join(l.split()[:-1])
        src[i:i+1] = [p+"if os.environ.get('debug')=='on': "+l+' _dbg%s as %s\n'%(name, name), p+'else: %s %s\n'%(l, name)]

    try: os.makedirs(os.path.dirname(sys.argv[2]))
    except OSError: pass
    open(sys.argv[2], 'w').writelines(['try: import os, sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())\n', 'except: pass\n']+src)
