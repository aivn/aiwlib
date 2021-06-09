# Copyright (C) 2017,2020-21 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
# Licensed under the Apache License, Version 2.0

from math import *
#-------------------------------------------------------------------------------
def text_sz(text, paint, vertical):
    rect = paint.boundingRect(0, 0, 1000, 1000, 0, text)
    return rect.height() if vertical else rect.width()
#-------------------------------------------------------------------------------
def make_tics3D(limits, logscale, xy1, xy2, paint, flip):  # 
    '''возвращает [(tic_pos, tic_text, QtAlign,)], max_tic_sz, [subtics_pos...]
tic_pos и subtics_pos задаются как кортежи (x1, y1, x2, y2), первая двойка в tic_pos задает координаты текста 
тики всегда расположены справа-снизу от вектора xy1-->xy2. Координаты xy1,2 задаются в пикселях
'''
    ticsL = calc_tics(limits[0], limits[1], logscale) # несколько вариантов расстановки тиков
    #tszs = [sum(text_sz(num2str(t), paint, vertical) for t in L) for L in ticsL] # суммарные размеры тиков
    tszs = [sum(text_sz(t, paint, vertical) for t in num2strL(L, logscale)) for L in ticsL] # суммарные размеры тиков
    tics = sorted(ticsL[min([(abs(float(tszs[i])/N-[.6, .4][vertical]), i) for i in range(len(tszs))])[1]]) # набор значений тиков    
    if not tics: tics = limits
    if logscale:
        stics = [] if limits[0]==tics[0] else calc_tics_normalscale(limits[0], tics[0], tics[0]/10)
        for a, b in zip(tics[:-1], tics[1:]): stics += calc_tics_normalscale(a, b, 10**floor(log10(a)))
        stics += [] if limits[1]==tics[-1] else calc_tics_normalscale(tics[-1], limits[1], tics[-1])
    else: stics = calc_tics_normalscale(limits[0], limits[1], (tics[1]-tics[0])/10)
    t2p0 = lambda t: int(N/log(limits[1]/limits[0])*log(t/limits[0]) if logscale else abs(N/(limits[1]-limits[0])*(t-limits[0]))) # конвертер значения в позицию
    t2p = lambda t: N-t2p0(t) if flip else t2p0(t)
    #try: return [(t2p(t), num2str(t)) for t in tics], max(text_sz(num2str(t), paint, False) for t in tics), map(t2p, stics)
    #except ValueError as e: return [(0, num2str(t)) for t in tics], max(text_sz(num2str(t), paint, False) for t in tics), [] #[0]*len(stics)
    L = num2strL(tics, logscale)
    try: return [(t2p(t), l) for t, l in zip(tics, L)], max(text_sz(l, paint, False) for l in L), map(t2p, stics)
    except ValueError as e: return [(0, l) for l in L], max(text_sz(l, paint, False) for t in L), [] #[0]*len(stics)
#-------------------------------------------------------------------------------
def make_tics(limits, logscale, N, vertical, paint, flip):
    'возвращает [(tic_pos, tic_text)... ], max_tic_sz, [subtics_pos...]'
    ticsL = calc_tics(limits[0], limits[1], logscale) # несколько вариантов расстановки тиков
    #tszs = [sum(text_sz(num2str(t), paint, vertical) for t in L) for L in ticsL] # суммарные размеры тиков
    tszs = [sum(text_sz(t, paint, vertical) for t in num2strL(L, logscale)) for L in ticsL] # суммарные размеры тиков
    tics = sorted(ticsL[min([(abs(float(tszs[i])/N-[.6, .4][vertical]), i) for i in range(len(tszs))])[1]]) # набор значений тиков    
    if not tics: tics = limits
    if logscale:
        stics = [] if limits[0]==tics[0] else calc_tics_normalscale(limits[0], tics[0], tics[0]/10)
        for a, b in zip(tics[:-1], tics[1:]): stics += calc_tics_normalscale(a, b, 10**floor(log10(a)))
        stics += [] if limits[1]==tics[-1] else calc_tics_normalscale(tics[-1], limits[1], tics[-1])
    else: stics = calc_tics_normalscale(limits[0], limits[1], (tics[1]-tics[0])/10)
    t2p0 = lambda t: int(N/log(limits[1]/limits[0])*log(t/limits[0]) if logscale else abs(N/(limits[1]-limits[0])*(t-limits[0]))) # конвертер значения в позицию
    t2p = lambda t: N-t2p0(t) if flip else t2p0(t)
    #try: return [(t2p(t), num2str(t)) for t in tics], max(text_sz(num2str(t), paint, False) for t in tics), map(t2p, stics)
    #except ValueError as e: return [(0, num2str(t)) for t in tics], max(text_sz(num2str(t), paint, False) for t in tics), [] #[0]*len(stics)
    L = num2strL(tics, logscale)
    try: return [(t2p(t), l) for t, l in zip(tics, L)], max(text_sz(l, paint, False) for l in L), map(t2p, stics)
    except ValueError as e: return [(0, l) for l in L], max(text_sz(l, paint, False) for t in L), [] #[0]*len(stics)
#-------------------------------------------------------------------------------
def num2strL(L, logscale): 
    'единообразно приводит список чисел к самому лаконичному варианту записи в виде строк'
    L1, L2 = ['%g'%x for x in L], [] #; S1 = sum(map(len, L1))
    for x in L:
        try: p  = floor(log10(abs(x)))
        except: p = 0
        L2.append('%ge%g'%(x*10**-p, p))
    if logscale:  return [a if len(a)<=len(b) else b for a, b in zip(L1, L2)]
    L3 = [x.rstrip('0') for x in L1]
    LL = [len(a)-len(b) for a, b in zip(L1, L3)]; minLL = min(LL) if LL else 0
    if minLL>2: eX = 'e%i'%minLL; return [x+'0'*(l-minLL)+eX for x, l in zip(L3, LL)]  # единообразно заменяем нули на конце на eX
    #return [a if len(a)<=len(b) else b for a, b in zip(L1, L2)]
    return L1 if sum(map(len, L1))<=sum(map(len, L2)) else L2
#-------------------------------------------------------------------------------
def calc_tics_normalscale(a, b, h):
    'возвращает разбиение по шагам h'
    if b<a: a, b = b, a
    A, B, L = floor(a/h)*h, ceil(b/h)*h, []
    while A<=B:
        if a<=A<=b: L.append(0. if abs(A)<h/100 else A)
        A += h
    return L    
def calc_tics(a, b, logscale):
    'принимает границы диапазона и тип шкалы, возвращает несколько вариантов расстановки тиков парами'
    if b<a: a, b = b, a
    if logscale:
        A0, B, res = 10**floor(log10(a)), 10**ceil(log10(b)), []
        for n in range(1, 5):
            A, L = A0, []
            while A<=B:
                if a<=A<=b: L.append(A)
                A *= 10**n
            res.append(L)            
    else:
        H, res = 10**ceil(log10(b-a)), []
        for h in [H, H/2, H/4, H/5, H/10, H/20, H/40, H/50, H/100, H/200, H/500, H/100]:
            L = calc_tics_normalscale(a, b, h)
            if len(L)<=20: res.append(L)
    return res
#-------------------------------------------------------------------------------
