# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2017  Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

from Tkinter import *
from tkFileDialog import *
from math import *
import os, sys, PIL.Image, PIL.ImageTk, aiwlib.plot2D
from aiwlib.plot2D import *
#-------------------------------------------------------------------------------
TkVar = lambda x: {bool: BooleanVar, int: IntVar, str: StringVar, float: DoubleVar}[type(x)](value=x)
get_widget_sz = lambda widget: tuple(map(int, widget.winfo_geometry().split('+')[0].split('x')))
#-------------------------------------------------------------------------------
def num2str(x):
    'приводит число к самому лаконичному варианту записи в виде строки'
    s1 = '%g'%x
    try: p  = floor(log10(abs(x)))
    except: p = 0
    s2 = '%ge%g'%(x*10**-p, p)
    return s1 if len(s1)<=len(s2) else s2
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
def make_tics(limits, logscale, N, orient, font_sz):
    'возвращает [(tic_pos, tic_text)... ], max_tic_sz'
    ticsL = calc_tics(limits[0], limits[1], logscale) # несколько вариантов расстановки тиков
    tszs = [sum((1 if orient else len(num2str(t)))*font_sz for t in L) for L in ticsL] # суммарные размеры тиков
    tics = sorted(ticsL[min([(abs(float(tszs[i])/N-[.6, .4][orient]), i) for i in range(len(tszs))])[1]]) # набор значений тиков    
    if not tics: tics = limits
    if logscale:
        stics = [] if limits[0]==tics[0] else calc_tics_normalscale(limits[0], tics[0], tics[0]/10)
        for a, b in zip(tics[:-1], tics[1:]): stics += calc_tics_normalscale(a, b, 10**floor(log10(a)))
        stics += [] if limits[1]==tics[-1] else calc_tics_normalscale(tics[-1], limits[1], tics[-1])
    else: stics = calc_tics_normalscale(limits[0], limits[1], (tics[1]-tics[0])/10)
    t2p = lambda t: int(N/log(limits[1]/limits[0])*log(t/limits[0]) if logscale else abs(N/(limits[1]-limits[0])*(t-limits[0]))) # ковертер знаечния в позицию
    return [(t2p(t), num2str(t)) for t in tics], (max(len(num2str(t))*font_sz for t in tics) if orient else font_sz), map(t2p, stics)
#-------------------------------------------------------------------------------
class VarWidget:
    def get(self): return self.var.get()
    def set(self, x): self.var.set(x)
    def __bool__(self): return bool(self.var.get())
    def __int__(self): return int(self.var.get())
    def __str__(self): return str(self.var.get())
    def __float__(self): return float(self.var.get())    
#-------------------------------------------------------------------------------
class aiwCheck(VarWidget):
    'Checkbutton и ассоциированная с ним переменная'
    def __init__(self, root, text, value=False, **kw_args):
        self.var = TkVar(value)
        self.check = Checkbutton(root, variable=self.var, text=text,
                                 **dict(filter(lambda i: not type(i[1]) is dict, kw_args.items())))
        for kv in filter(lambda i: type(i[1]) is dict, kw_args.items()): getattr(self.check, kv[0])(**kv[1])
    def config(self, **kw_args): self.check.config(**kw_args)
    def state(self, state): self.check.config(state=['disabled', 'normal', 'readonly'][state])
#-------------------------------------------------------------------------------
class aiwScale(VarWidget):
    'Scale и ассоциированная с ним переменная'
    def __init__(self, root, text, value=0, **kw_args):
        self.var = TkVar(value)
        self.scale = Scale(root, variable=self.var, label=text,
                           **dict(filter(lambda i: not type(i[1]) is dict, kw_args.items())))
        for kv in filter(lambda i: type(i[1]) is dict, kw_args.items()): getattr(self.scale, kv[0])(**kv[1])
    def config(self, **kw_args): self.scale.config(**kw_args)
    def state(self, state): self.scale.config(state=['disabled', 'normal', 'readonly'][state])
#-------------------------------------------------------------------------------
class aiwEntry(VarWidget):
    'поле Entry и ассоциированная с ним переменная, опицонально метка слева'
    def __init__(self, root, value='', command=None, label='', **kw_args):
        self.var = TkVar(value)
        if label:
            self.frame = root = Frame(root); self.label = Label(root, text=label)
            if 'pack' in kw_args: root.pack(**kw_args.pop('pack'))
            if 'grid' in kw_args: root.grid(**kw_args.pop('grid'))
        self.entry = Entry(root, textvariable=self.var,
                           **dict(filter(lambda i: not type(i[1]) is dict, kw_args.items())))
        for kv in filter(lambda i: type(i[1]) is dict, kw_args.items()): getattr(self.entry, kv[0])(**kv[1])
        if label: self.entry.pack(side=RIGHT, anchor=E); self.label.pack(side=RIGHT, anchor=E)
        if command: self.entry.bind('<Return>', command)
    def config(self, **kw_args): self.entry.config(**kw_args)
    def state(self, state): self.entry.config(state=['disabled', 'normal', 'readonly'][state])
#-------------------------------------------------------------------------------
class Plot2D(Canvas):
    'рисует цветом функции с зарамочным оформлением'
    paletters = dict((k[:-4], getattr(aiwlib.plot2D, k)) for k in dir(aiwlib.plot2D) if k.endswith('_pal'))
    def wsize(self): return get_widget_sz(self)
    #---------------------------------------------------------------------------
    def _mouse_left_press(self, e):
        x, y = self.canvasx(e.x), self.canvasy(e.y); self._left_press = [x, y]
        for xy0, xy1, t, f in self.mouses:
            if t=='select' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]:
                self._sel_region = xy0, xy1; self._on_move, self._on_release, sticky = f
                if sticky:
                    self._sel_sticky = (sticky, xy1[sticky=='y'])
                    self._left_press[sticky=='y'] = xy0[sticky=='y']
                break
    def _mouse_move(self, e):
        x, y = self.canvasx(e.x), self.canvasy(e.y)
        if self._on_move: 
            if self._sel_region:
                x = min(max(self._sel_region[0][0], x), self._sel_region[1][0])
                y = min(max(self._sel_region[0][1], y), self._sel_region[1][1])
            if self._sel_sticky:
                if self._sel_sticky[0]=='x': x = self._sel_sticky[1]
                else: y = self._sel_sticky[1]
            self._on_move(self, x, y)
            return
        for xy0, xy1, t, f in self.mouses:
            if t=='move' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]: f(self, x, y); break 
    def _mouse_left_release(self, e):
        x, y = self.canvasx(e.x), self.canvasy(e.y)
        if not self._on_move and (abs(x-self._left_press[0])<=1 and abs(y-self._left_press[1])<=1):
            for xy0, xy1, t, f in self.mouses:
                if t=='left' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]: f(self, x, y); break            
        elif self._on_release:
            self._on_release(self, x, y); self._on_move, self._on_release, self._sel_sticky, self._sel_region = [None]*4
            return
    def _mouse_right(self, e): 
        x, y = self.canvasx(e.x), self.canvasy(e.y)
        for xy0, xy1, t, f in self.mouses:
            if t=='right' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]: f(self, x, y); break 
    def sel_xy2coord(self, x, y): return [float(x-self._sel_region[0][0])/(self._sel_region[1][0]-self._sel_region[0][0]),
                                          1-float(y-self._sel_region[0][1])/(self._sel_region[1][1]-self._sel_region[0][1])]
    #---------------------------------------------------------------------------
    def __init__(self, root, **kw_args):
        Canvas.__init__(self, root, **kw_args)
        self.picts, self.mouses, self._on_move, self._on_release = {}, [], None, None
        self.bind('<ButtonPress-1>', self._mouse_left_press)
        self.bind('<ButtonRelease-1>', self._mouse_left_release)
        self.bind('<Button-3>', self._mouse_right)
        self.bind('<Motion>', self._mouse_move)
        self._sel_sticky, self._sel_region = None, None
    def _add_pict(self, tag, xy0, xy1, border, plotter):
        if tag in self.picts: self.del_pict(tag)
        image = PIL.Image.new('RGB', (xy1[0]-xy0[0], xy1[1]-xy0[1]))
        plotter(ImagePIL(image))
        cimage = PIL.ImageTk.PhotoImage(image=image)
        self.create_image(xy0[0], xy0[1], image=cimage, anchor=NW, tag=tag)
        #if border: self.create_rectangle(xy0[0]+border/2, xy0[1]+border/2, xy1[0]-border/2, xy1[1]-border/2, width=border, tag=tag)
        if border: self.create_rectangle(xy0[0], xy0[1], xy1[0], xy1[1], width=border, tag=tag)
        self.picts[tag] = (xy0, xy1, image, cimage) #, data, color)        
    def add_pict(self, tag, xy0, xy1, data, color, border=1):
        'рисует data в прямоугольнике xy0:xy1'
        self._add_pict(tag, xy0, xy1, border, lambda im: plot2D(data, color, im))
        return tag
    def del_pict(self, tag): self.delete(tag); del self.picts[tag]
    def _add_tics(self, tag, orient, xy0, xy1, tics, stics, font, tic_sz):
        'orient=0|1, tics=[(tic_pos, tic_text)... ], font=("...", sz, [...])'
        for p, t in tics:
            if orient:
                self.create_line(xy1[0]-tic_sz[0], xy1[1]-p, xy1[0], xy1[1]-p, width=tic_sz[1], tag=tag)
                y = min(xy1[1]-p, xy1[1]-font[1]/2) if p==tics[0][0] else max(xy1[1]-p, xy0[1]+font[1]/2) if p==tics[-1][0] else xy1[1]-p
                self.create_text(xy1[0]-tic_sz[0], y, text=t, anchor='e', font=font, tag=tag)
            else:
                self.create_line(xy0[0]+p, xy0[1], xy0[0]+p, xy0[1]+tic_sz[0], width=tic_sz[1], tag=tag)
                #x = min(xy0[0]+p, xy1[0]-len(t)*font[1]/2) if p==tics[-1][0] else xy0[0]+p
                #if p==tics[-1][0]: continue
                self.create_text(xy0[0]+p, xy0[1]+tic_sz[0], text=t, anchor='n', font=font, tag=tag)
        for p in stics:
            if orient: self.create_line(xy1[0]-tic_sz[0]/2, xy1[1]-p, xy1[0], xy1[1]-p, width=tic_sz[1], tag=tag)
            else: self.create_line(xy0[0]+p, xy0[1], xy0[0]+p, xy0[1]+tic_sz[0]/2, width=tic_sz[1], tag=tag)
    def add_tics(self, tag, orient, xyN, side, limits, logscale=False, font=('FreeMono', 14), tic_sz=(5,1)):
        'orient=0|1, xyN=(x0,y0,N), side=0|1, (левее/правее или выше/ниже линии xyN), limits=(min,max), возвращает толщину'
        tics, max_tic_sz, stics = make_tics(limits, logscale, xyN[2], orient, font[1])
        xy0, xy1 = [xyN[0], xyN[1]], [xyN[0]+xyN[2]*(1-orient), xyN[1]+xyN[2]*orient]
        if side: xy0[1-orient] -= max_tic_sz+tic_sz[0]
        else: xy1[1-orient] += max_tic_sz+tic_sz[0]
        self._add_tics(tag, orient, xy0, xy1, tics, stics, font, tic_sz)
        return max_tic_sz+tic_sz[0]
    def plot_pal(self, tag, palname, orient, xyN, side, pal_sz, limits, logscale=False, font=('FreeMono', 14), tic_sz=(5,1), border=1):
        'рисует палитру и проставляет тики, orient=0|1, xyN=(x0,y0,N), side=0|1, (левее/правее или выше/ниже линии xyN), limits=(min,max), возвращает толщину'
        if tag in self.picts: self.del_pict(tag)
        tics, max_tic_sz, stics = make_tics(limits, logscale, xyN[2], orient, font[1])
        # формируем bbox для картинки, прилегающий к линии
        xyp0, xyp1 = [xyN[0], xyN[1]], [xyN[0]+xyN[2]*(1-orient), xyN[1]+xyN[2]*orient]
        if side: xyp0[1-orient] -= pal_sz
        else: xyp1[1-orient] += pal_sz         
        self._add_pict(tag, xyp0, xyp1, border, lambda im: plot_paletter(self.paletters[palname], im, bool(orient)))
        # формируем bbox для тиков, прилегающих к картинке
        xyt0, xyt1 = (([xyp0[0], xyp1[1]], list(xyp1)), (list(xyp0), [xyp0[0], xyp1[1]]))[orient] 
        if side: xyt0[1-orient] -= max_tic_sz+tic_sz[0]
        else: xyt1[1-orient] += max_tic_sz+tic_sz[0]
        self._add_tics(tag, orient, xyt0, xyt1, tics, stics, font, tic_sz)
        return max_tic_sz+tic_sz[0]+pal_sz
    def save_image(self, *args):
        'запрашивает имя файла и сохраныет изображение в форматах .ps или .pdf'
        fname = asksaveasfilename(defaultextension='.pdf', filetypes=['PDF {.pdf}', 'PostScript {.ps}'])
        if not fname: return
        psname = fname.rsplit('.', 1)[0]+'.ps'
        self.postscript(file=psname)
        if fname.endswith('.pdf'): os.system('epstopdf "%s" && rm "%s"'%(psname, psname))
    def mouse_move(self, xy0, xy1, action): self.mouses.append((xy0, xy1, 'move', action))
    def mouse_left(self, xy0, xy1, action): self.mouses.append((xy0, xy1, 'left', action))
    def mouse_right(self, xy0, xy1, action): self.mouses.append((xy0, xy1, 'right', action))
    def mouse_select(self, xy0, xy1, move_act, final_act, sticky=None): self.mouses.append((xy0, xy1, 'select', (move_act, final_act, sticky)))
    def mouse_clear(self): del self.mouses[:]
#-------------------------------------------------------------------------------
