# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2017,2020-2021  Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

import tempfile
from math import *
from Tkinter import *
import os, sys, time #, PIL.Image, PIL.ImageTk, PIL.ImageDraw, PIL.ImageFont
from aiwlib.view import *
from aiwlib.vec import *
#-------------------------------------------------------------------------------
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
    t2p = lambda t: int(N/log(limits[1]/limits[0])*log(t/limits[0]) if logscale else abs(N/(limits[1]-limits[0])*(t-limits[0]))) # конвертер значения в позицию
    try: return [(t2p(t), num2str(t)) for t in tics], (max(len(num2str(t))*font_sz for t in tics) if orient else font_sz), map(t2p, stics)
    except ValueError as e: return [(0, num2str(t)) for t in tics], (max(len(num2str(t))*font_sz for t in tics) if orient else font_sz), [0]*len(stics)
#-------------------------------------------------------------------------------
class Plot2D(Canvas):
    'рисует цветом функции с зарамочным оформлением'
    def wsize(self): return get_widget_sz(self)
    #---------------------------------------------------------------------------
    def _mouse_left_press(self, e):
        x, y = self.canvasx(e.x), self.canvasy(e.y); self._left_press = [x, y]
        for xy0, xy1, t, f in self.mouses:
            if t=='select' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]:
                self._sel_region = list(xy0), list(xy1); self._on_move, self._on_release, sticky, reg = f
                if sticky:
                    self._sel_sticky = (sticky, reg[1] if reg else xy1[sticky=='y'])
                    self._left_press[sticky=='y'] = reg[0] if reg else xy0[sticky=='y']
                    if reg: self._sel_region[0][sticky=='y'], self._sel_region[1][sticky=='y'] = reg
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
            if t=='move':
                if xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]: f[0](self, (x,y), xy0, xy1)
                else: f[1](self)
    def _mouse_left_release(self, e):
        x, y = self.canvasx(e.x), self.canvasy(e.y)
        if not self._on_move and (abs(x-self._left_press[0])<=1 and abs(y-self._left_press[1])<=1):
            for xy0, xy1, t, f in self.mouses:
                if t=='left' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]: f(self, x, y); break            
        elif self._on_release:
            if self._sel_sticky:
                if self._sel_sticky[0]=='x': x = self._sel_sticky[1]
                else: y = self._sel_sticky[1]
            self._on_release(self, x, y); self._on_move, self._on_release, self._sel_sticky, self._sel_region = [None]*4
            return
    def _mouse_right(self, e): 
        x, y = self.canvasx(e.x), self.canvasy(e.y)
        for xy0, xy1, t, f in self.mouses:
            if t=='right' and xy0[0]<=x<=xy1[0] and xy0[1]<=y<=xy1[1]: f(self, (x,y), xy0, xy1); break 
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
        self._content = []
    #--- wrap base func ---
    def create_rectangle(self, *args, **kw_args): self._content.append(('rect', args, kw_args)); Canvas.create_rectangle(self, *args, **kw_args)
    def create_line(self, *args, **kw_args): self._content.append(('line', args, kw_args)); Canvas.create_line(self, *args, **kw_args)
    def create_text(self, *args, **kw_args): self._content.append(('text', args, kw_args)); Canvas.create_text(self, *args, **kw_args)
    def delete(self, tag):
        if tag=='all': self._content = []
        else: self._content = filter(lambda i: i[2].get('tag')!=tag, self._content)
        Canvas.delete(self, tag)
    #----------------------
    def _add_pict(self, tag, xy0, xy1, border, plotter, image=None):
        if tag in self.picts: self.del_pict(tag)
        #image = PIL.Image.new('RGB', (xy1[0]-xy0[0], xy1[1]-xy0[1]))
        #plotter(ImagePIL(image))
        #cimage = PIL.ImageTk.PhotoImage(image=image)
        if not image: image = Image(ind(xy1[0]-xy0[0], xy1[1]-xy0[1]))
        plotter(image)
        #        print len(image.buf), len(image.buf)-image.head_sz, image.size.prod()*3
        try: cimage = PhotoImage(width=xy1[0]-xy0[0], height=xy1[1]-xy0[1], format='PPM', data=image.buf)
        except TclError, e:
            print 'TCLError', e, '---> forced to use the temporary file P6.ppm ;-((('
            open('P6.ppm', 'wb').write(image.buf)
            cimage = PhotoImage(width=xy1[0]-xy0[0], height=xy1[1]-xy0[1], format='PPM', file='P6.ppm') 
        self.create_image(xy0[0], xy0[1], image=cimage, anchor=NW, tag=tag)
        #if border: self.create_rectangle(xy0[0]+border/2, xy0[1]+border/2, xy1[0]-border/2, xy1[1]-border/2, width=border, tag=tag)
        if border: self.create_rectangle(xy0[0], xy0[1], xy1[0], xy1[1], width=border, tag=tag)
        self.picts[tag] = (xy0, xy1, image, cimage) #, data, color)        
    def add_pict(self, tag, xy0, xy1, plotter, border=1, image=None):
        'рисует data в прямоугольнике xy0:xy1'
        self._add_pict(tag, xy0, xy1, border, plotter, image)
        return tag
    def del_pict(self, tag): self.delete(tag); del self.picts[tag]
    def _add_tics(self, tag, orient, xy0, xy1, tics, stics, font, tic_sz, font_scale):
        'orient=0|1, tics=[(tic_pos, tic_text)... ], font=("...", sz, [...])'
        fsz = font[1] #int(font[1]*font_scale)
        for p, t in tics:
            if orient:
                self.create_line(xy1[0]-tic_sz[0], xy1[1]-p, xy1[0], xy1[1]-p, width=tic_sz[1], tag=tag)
                y = min(xy1[1]-p, xy1[1]-fsz/2) if p==tics[0][0] else max(xy1[1]-p, xy0[1]+fsz/2) if p==tics[-1][0] else xy1[1]-p
                self.create_text(xy1[0]-tic_sz[0], y, text=t, anchor='e', font=font, tag=tag)
            else:
                self.create_line(xy0[0]+p, xy0[1], xy0[0]+p, xy0[1]+tic_sz[0], width=tic_sz[1], tag=tag)
                #x = min(xy0[0]+p, xy1[0]-len(t)*fsz/2) if p==tics[-1][0] else xy0[0]+p
                #if p==tics[-1][0]: continue
                self.create_text(xy0[0]+p, xy0[1]+tic_sz[0], text=t, anchor='n', font=font, tag=tag)
        for p in stics:
            if orient: self.create_line(xy1[0]-tic_sz[0]/2, xy1[1]-p, xy1[0], xy1[1]-p, width=tic_sz[1], tag=tag)
            else: self.create_line(xy0[0]+p, xy0[1], xy0[0]+p, xy0[1]+tic_sz[0]/2, width=tic_sz[1], tag=tag)
    def add_tics(self, tag, orient, xyN, side, limits, logscale=False, font=('FreeMono', 14), tic_sz=(5,1), border=None, font_scale=1.):
        'orient=0|1, xyN=(x0,y0,N), side=0|1, (левее/правее или выше/ниже линии xyN), limits=(min,max), возвращает толщину'
        tics, max_tic_sz, stics = make_tics(limits, logscale, xyN[2], orient, int(font[1]*font_scale))
        xy0, xy1 = [xyN[0], xyN[1]], [xyN[0]+xyN[2]*(1-orient), xyN[1]+xyN[2]*orient]
        if side: xy0[1-orient] -= max_tic_sz+tic_sz[0]
        else: xy1[1-orient] += max_tic_sz+tic_sz[0]
        self._add_tics(tag, orient, xy0, xy1, tics, stics, font, tic_sz, font_scale)
        return max_tic_sz+tic_sz[0]
    def plot_pal(self, tag, paletter, orient, xyN, side, pal_sz, limits, logscale=False, font=('FreeMono', 14), tic_sz=(5,1), border=1, font_scale=1.):
        'рисует палитру и проставляет тики, orient=0|1, xyN=(x0,y0,N), side=0|1, (левее/правее или выше/ниже линии xyN), limits=(min,max), возвращает толщину'
        if tag in self.picts: self.del_pict(tag)
        tics, max_tic_sz, stics = make_tics(limits, logscale, xyN[2], orient, int(font[1]*font_scale))
        # формируем bbox для картинки, прилегающий к линии
        xyp0, xyp1 = [xyN[0], xyN[1]], [xyN[0]+xyN[2]*(1-orient), xyN[1]+xyN[2]*orient]
        if side: xyp0[1-orient] -= pal_sz
        else: xyp1[1-orient] += pal_sz         
        self._add_pict(tag, xyp0, xyp1, border, lambda im: plot_paletter(paletter, im, bool(orient)))
        # формируем bbox для тиков, прилегающих к картинке
        xyt0, xyt1 = (([xyp0[0], xyp1[1]], list(xyp1)), (list(xyp0), [xyp0[0], xyp1[1]]))[orient] 
        if side: xyt0[1-orient] -= max_tic_sz+tic_sz[0]
        else: xyt1[1-orient] += max_tic_sz+tic_sz[0]
        self._add_tics(tag, orient, xyt0, xyt1, tics, stics, font, tic_sz, font_scale)
        return max_tic_sz+tic_sz[0]+pal_sz
    def dump2png(self, fname):        
        try: import PIL.Image, PIL.ImageDraw, PIL.ImageFont
        except:
            print 'Trying to use postscript and "convert" utility, install Python Imaging Library to speed up ...',; sys.stdout.flush()
            ps, ext = os.path.splitext(fname); ps += '.ps'
            self.postscript(file=ps)
            if ext and fname!=ps:
                os.system('gs -q -dNOPAUSE -dBATCH -dSAFER -r600 -dEPSCrop -dDownScaleFactor=6 -dGraphicsAlphaBits=4 -sDEVICE=png16m -sOutputFile="%s" %s'%(fname, ps))
                os.remove(ps); print 'OK'
            return
        img = PIL.Image.new('RGB', self.wsize(), (255, 255, 255)) #2png
        draw = PIL.ImageDraw.Draw(img)
        #help(draw)
        for T, a, kw in self._content:
            if kw.get('tag') in ('pal_move', 'msh_move'): continue
            if T=='text':
                a, txt, font = list(a), kw['text'], kw['font'] # с т.з. draw a задано относительно верхнего левого угла
                try: font = PIL.ImageFont.truetype(font[0]+'.ttf', font[1])
                except: font = PIL.ImageFont.truetype('./fonts/'+font[0]+'.ttf', font[1]) # for windows
                sz, anchor = draw.textsize(txt, font=font), kw.get('anchor', '')
                a[0] -= sz[0]/2; a[1] -= sz[1]/2 # центрируем текст относительно a
                if 'e' in anchor: a[0] -= sz[0]/2
                if 'w' in anchor: a[0] += sz[0]
                if 's' in anchor: a[1] -= sz[1]/2
                if 'n' in anchor: a[1] += sz[1]/2
                draw.text(a[:2], txt, fill=(0,0,0), font=font) 
            elif T=='line': draw.line(a[:4], width=1, fill=(0,0,0)) 
            elif T=='rect':
                draw.line((a[0], a[1], a[0], a[3]), width=1, fill=(0,0,0)) 
                draw.line((a[0], a[3], a[2], a[3]), width=1, fill=(0,0,0)) 
                draw.line((a[2], a[3], a[2], a[1]), width=1, fill=(0,0,0)) 
                draw.line((a[2], a[1], a[0], a[1]), width=1, fill=(0,0,0)) 
        for xy0, xy1, image, cimage in self.picts.values():
            tmpout = tempfile.NamedTemporaryFile(suffix='.ppm', delete=False); tmpout.write(image.buf); tmpout.close()
            img.paste(PIL.Image.open(tmpout.name), xy0+xy1); os.remove(tmpout.name)
        img.save(fname)
    def mouse_move(self, xy0, xy1, action): self.mouses.append((xy0, xy1, 'move', action))
    def mouse_left(self, xy0, xy1, action): self.mouses.append((xy0, xy1, 'left', action))
    def mouse_right(self, xy0, xy1, action): self.mouses.append((xy0, xy1, 'right', action))
    def mouse_select(self, xy0, xy1, move_act, final_act, sticky=None, reg=None): self.mouses.append((xy0, xy1, 'select', (move_act, final_act, sticky, reg)))
    def mouse_clear(self): del self.mouses[:]
#-------------------------------------------------------------------------------
