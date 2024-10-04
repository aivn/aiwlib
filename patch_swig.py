#!/usr/bin/python3

import os, sys

fname = sys.argv[1] #if len(sys.argv)==3 or sys.argv[1].endswith('_wrap.cxx') else sys.argv[1][:-3]+'_wrap.cxx'
src, itable = open(fname).readlines(), []

if fname.endswith('_wrap.cxx'):
    for i0, l in enumerate(src):
        l = l.split()
        if 'SWIG_init' in l: itable.append((i0, l[-1]))
        if len(l)==3 and l[0]=='#define' and l[1]=='SWIG_name': break
    name = l[2][1:-1]
    src[i0:i0+1] = ['// aiwlib patch <<<\n','#ifndef EBUG\n', '#define SWIG_name "%s"\n'%name,
                    '#else\n', '#define SWIG_name "_dbg%s"\n'%name, '#endif\n', '// >>>\n']
    for i, n in reversed(itable):
        src[i:i+1] = ['// aiwlib patch <<<\n', '#ifndef EBUG\n', '#define SWIG_init %s\n'%n,
                      '#else\n', '#define SWIG_init %s_dbg_%s\n'%tuple(n.rsplit('_', 1)), '#endif\n', '// >>>\n']

    n_cast = 0
    for i, l in enumerate(src):
        if 'SWIG_NewPointerObj' in l and '>(result))' in l and not 'static_cast' in l and ('aiw::Vec<' in l or 'aiw::Ind<' in l):
            a, b = l.find('aiw::Vec<' if 'aiw::Vec<' in l else 'aiw::Ind<'), l.find('>(result)),'); v = l[a:b+1] #; print(v)
            src[i] = l.replace('(result)', '(static_cast< const %s &>(result))'%v)
            n_cast += 1
            
    open(fname, 'w').writelines(src)
    print('\033[7m    file', fname, 'patched  (%i casts added)  \033[0m'%n_cast)

elif fname.endswith('.py'):
    name = '_'+os.path.basename(fname).split('.')[0]
    for i0, l in enumerate(src):
        if l.split()[-2:] == ['import', name]: itable.append(i0)
    for i in reversed(itable[:2]):
        l = src[i]; p, l = l[:len(l)-len(l.lstrip())], ' '.join(l.split()[:-1])
        src[i:i+1] = [p+"if os.environ.get('debug')=='on': "+l+' _dbg%s as %s  # aiwlib patch\n'%(name, name), p+'else: %s %s  # aiwlib patch\n'%(l, name)]

    try: os.makedirs(os.path.dirname(sys.argv[2]))
    except OSError: pass
    open(sys.argv[2], 'w').writelines(['try: import os, sys; sys.setdlopenflags(0x00100|sys.getdlopenflags())  # aiwlib patch\n', 'except: pass  # aiwlib patch\n']+src)
    print('\033[7m    file', fname, 'patched and copied to', sys.argv[2], '   \033[0m')
