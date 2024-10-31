# Copyright (C) 2024 Antov V. Ivanov  <aiv.racs@gmail.com> 
# Licensed under the Apache License, Version 2.0

import os, sys, subprocess, collections, time

def gplt(*args, **kw_args):
    '''**kw_args позволяет передавать опции с одним аргументом, такие опции добавляются  в начало команды (кроме опции to), 
их порядок при добавлении не определен, '-' перед именем опции добавляется автоматически. 
При задании пределов отрисовки через kw_args значение должно быть строкой или кортежем из двух элементов, 
если какой то из пределов не задается он должен быть задан как None.
Значением опций U и u иожет быть как строка так и кортеж/список строк объединяемых через пробел.

В args допускаются следующие значения:
  Строка добавляется как обычный аргумент командной строки gplt.
  Число добавляется для отрисовки как безымянная линия/плоскость серого цвета.
  Кортеж или список из нескольких чисел и строки трактуется как метка, где числа координаты метки.
  Кортеж или список из нескольких чисел трактуется как координаты начала и конца стрелки.
  Последовательность (кортеж, список, генератор и т.д.) из строк и кортежей  с числами трактуется как данные, 
     если первый элемент яляется строкой то он @-аттрибуты (если начинается с @) либо имя файла (если НЕ начинается с #:) 
     при отображении, идущие за ним элементы записываются во временный файл, кортежи с числами преобразуются к строкам
     (числа разделяются пробелами, None переводятся в nan), пустой элемент означает пустую строку.
  Словарь с полями {'@':@-атрибуты-отрисовки-без-символа-@, '#:':строка-или-кортеж/список-со-строками-заголовка-без-символов-#:, 
                    '[]':последовательность-строк-с-данными или  имя-столбца1:данные-столбца1, имя-столбца2:...}  
     в случае передачи столбцов последовательность столбцов берется из заголовка #:, если заданы строки через [] столбцы игнорируются
'''
    try:
        cmdline, tmpfiles = ['gplt'], []
        for k, v in kw_args.items():
            if k in ('rx', 'ry', 'rz', 'rx2', 'ry2', 'rcb', 'rt', 'ru', 'rv'):
                cmdline += ['-'+k, v if type(v) is str else '%s:%s'%tuple('' if x is None else x for x in v)]
            elif k in ('u', 'U'): cmdline += ['-'+k, v if type(v) is str else ' '.join(map(str, v))]
            elif k!='to': cmdline += ['-'+k, str(v)]
        for a in args:
            if type(a) is str: cmdline.append(a)
            elif type(a) in (float, int): cmdline += ['-fn', '%s@gray,='%a]
            elif type(a) in (list, tuple) and len(a)>=3 and all(type(x) in (int, float) for x in a[:-1]) and type(a[-1]) is str:
                cmdline += ['-lbl', ','.join(map(str, a[:-1]))+':'+a[-1]]  # метка
            elif type(a) in (list, tuple) and len(a)>=4 and all(type(x) in (int, float) for x in a):
                cmdline += ['-arw', ','.join(map(str, a[:len(a)/2]))+':'+','.join(map(str, a[len(a)/2:]))]  # стрелка
            elif type(a) is dict:
                fout = open('/tmp/gplt-py--%s--%s.dat'%(os.getpid(), time.time()), 'w'); tmpfiles.append(fout.name)
                dat, head = ('@'+a['@'] if '@' in a else ''), a.get('#:')
                if head and type(head) is str: print('#:'+head, file=fout); head = head.split()
                elif head:
                    L, head = list(head), []
                    for l in L:
                        if not '=' in l: head = l.split()
                        print('#:'+l, file=fout)
                if '[]' in a:
                    for l in a['[]']:
                        print((l if type(l) is str else ' '.join('nan' if x is None else str(x) for x in l)), file=fout)
                else:
                    for l in zip(*[a[c] for c in head]): print(' '.join('nan' if x is None else str(x) for x in l), file=fout)
                fout.close(); cmdline.append(fout.name+dat)                
            elif isinstance(a, collections.abc.Iterable):  # строчки с данными
                fout, dat = open('/tmp/gplt-py--%s--%s.dat'%(os.getpid(), time.time()), 'w'), None; tmpfiles.append(fout.name)
                for l in a:
                    if dat is None:
                        if type(l) is str and l and l[0]!='#': dat = l if l[0]=='@' else '@=%s!'%l.strip(); continue
                        else: dat = ''
                    print((l if type(l) is str else ' '.join('nan' if x is None else str(x) for x in l)), file=fout)
                fout.close(); cmdline.append(fout.name+dat)
            else: raise Exception('incorrect argument', a)            
        if 'to' in kw_args: cmdline += ['-to', kw_args['to']]
        print('run gplt with args', cmdline[1:])
        try: subprocess.Popen(cmdline).wait()
        except KeyboardInterrupt: pass
    finally:
        for f in tmpfiles: os.remove(f)
        
# добавить каких то объектов для структурирования последовательности команд?
