# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

from PyQt5 import QtWidgets, QtGui, QtCore
#-------------------------------------------------------------------------------
class Rect:
    def __init__(self, bmin, bmax, **kw_args): 
        self.bmin, self.bmax, self.plots = list(map(int, bmin)), list(map(int, bmax)), []; self.__dict__.update(kw_args)
        self.bbox = [self.bmax[0]-self.bmin[0], self.bmax[1]-self.bmin[1]]
    def shift(self, d): d = tuple(map(int, d)); self.bmin[0] += d[0]; self.bmax[0] += d[0]; self.bmin[1] += d[1]; self.bmax[1] += d[1]
    def extend(self, d): d = tuple(map(int, d)); self.bbox[0] += d[0]; self.bmax[0] += d[0]; self.bbox[1] += d[1]; self.bmax[1] += d[1]
    def check(self, event): return self.bmin[0]<=event.x()<=self.bmax[0] and self.bmin[1]<=event.y()<=self.bmax[1]
    def press(self, canvas, event): self.xy0 = event.x(), event.y(); return self
    def touch(self, canvas, event): self.xy = event.x(), event.y(); canvas.update(); return True
    def dblclick(self, canvas, event): pass
    #---------------------------------------------------------------------------
    def text(self, x, y, text, align='', color=None): self.plots.append(('text', int(x), int(y), text, align, color)) # align = l|r|t|b или их комбинации        
    def line(self, x1, y1, x2, y2, color=None):
        if color: self.plots.append(('setPen', QtGui.QPen(getattr(QtCore.Qt, color))))
        self.plots.append(('drawLine', int(x1), int(y1), int(x2), int(y2)))
    def make_up(self, canvas):
        self.im = QtGui.QImage(canvas.im); paint = QtGui.QPainter(self.im)
        paint.setFont(QtGui.QFont(canvas.win.font.currentText(), canvas.win.font_sz.value()))
        for a in self.plots:
            if a[0]=='text':
                x, y, text, align, color = a[1:]
                rect, d = paint.boundingRect(x, y, 1000, 1000, 0, text), [0, 0]
                if 'l' in align and 'r' in align: d[0] = -int(rect.width()/2)
                elif 'r' in align: d[0] = -rect.width()
                if 't' in align and 'b' in align: d[1] = -int(rect.height()/2)
                elif 'b' in align: d[1] = -rect.height()
                rect = QtCore.QRect(rect.x()+d[0], rect.y()+d[1], rect.width(), rect.height())
                paint.fillRect(rect, QtCore.Qt.white)
                if color: paint.setPen(QtGui.QPen(getattr(QtCore.Qt, color)))
                paint.drawText(rect.x(), rect.y(), rect.width(), rect.height(), 0, text)
            else: getattr(paint, a[0])(*a[1:])
        self.plots = []
        return self.im
#-------------------------------------------------------------------------------
_one2coord = lambda a, b, ls, x: a*(b/a)**x if ls else a+(b-a)*x
#-------------------------------------------------------------------------------
# только make_up реально рисует в container.im, все остальные указывают через настройки что именно рисовать (кроме release)
class MousePaletter(Rect):
    def check(self, event): return self.bmin[0]<=event.x() and self.bmin[1]<=event.y()<=self.bmax[1]
    def y2f(self, y): return _one2coord(self.f_min, self.f_max, self.logscale, (self.bmax[1]-y)/self.bbox[1])
    def line_text(self, y, f, color='red'): self.line(self.bmin[0], y, self.x1, y, color); self.text(self.x1, y, '%g'%f, 'rb', color)
    def touch(self, canvas, event):
        x, y = event.x(), event.y();  self.line_text(y, self.y2f(y))
        #canvas.setCursor(QtCore.Qt.SizeVerCursor if self.bmin[0]<x<self.bmax[0] else QtCore.Qt.ArrowCursor)
        if self.bmin[0]<x<self.bmax[0]: self.text(x, y, 'Z', 'rb', 'red')
        return True
    def press(self, canvas, event): # если кнопка левая начинаем выделение, иначе выключаем/включаем выделение
        if(event.buttons()&1): self.y0 = event.y(); self.f0 = self.y2f(self.y0); return self  # левая кнопка
        if(event.buttons()&2): # правая кнопка
            if event.x()>self.bmax[0]:  # autoscale on/off
                win = canvas.win
                if win.autoscale.isChecked(): win.f_min.setText(canvas.f_lim[0]); win.f_max.setText(canvas.f_lim[1]); win.autoscale.setChecked(False)
                else: canvas.f_lim = [win.f_min.text(), win.f_max.text()]; win.autoscale.setChecked(True)
                canvas.replot()
            elif canvas.D3==2:    # transparency on/off
                n = int((int((self.bmax[1]-event.y())/self.bbox[1]*(canvas.pal_len-1)*2+.5)+1)//2)
                canvas.D3tmask = canvas.D3tmask&~(1<<n) if canvas.D3tmask&(1<<n) else canvas.D3tmask|(1<<n)
                canvas.replot()
    def move(self, canvas, event): y = event.y(); self.line_text(self.y0, self.f0, 'green'); self.line_text(y, self.y2f(y)); canvas.update()
    def release(self, canvas, event):  # должен окончательно настроить канвас и вернуть True если нужна перерисовка
        f0, f1 = self.f0, self.y2f(event.y())
        if f0==f1: return False
        canvas.f_lim = ['%g'%f0, '%g'%f1] if f0<f1 else ['%g'%f1, '%g'%f0] 
        win = canvas.win; win.autoscale.setChecked(False)
        win.f_min.setText(canvas.f_lim[0]); win.f_max.setText(canvas.f_lim[1])
        return True
    def wheel(self, canvas, event):
        x, y, win = event.x(), event.y(), canvas.win
        if x<self.bmin[0]: return False
        f0, f1 = float(win.f_min.text()), float(win.f_max.text())
        sign = -1 if (event.angleDelta().y()<0)^(f0<f1) else 1
        if self.bmin[0]<x<self.bmax[0] and self.bmin[1]<y<self.bmax[1]: w_min = (self.bmax[1]-y)/self.bbox[1]; w_max = 1-w_min  # веса зумирования
        elif y<self.bmin[1]: w_max, w_min = 1, 0
        elif y>self.bmax[1]: w_max, w_min = 0, 1
        else: w_min, w_max = -1, 1
        if self.logscale: f0, f1 = f0*pow(f1/f0, -.1*w_min*sign), f1*pow(f1/f0, .1*w_max*sign)
        else: f0, f1 = f0-.1*(f1-f0)*w_min*sign, f1+.1*(f1-f0)*w_max*sign
        canvas.f_lim = ['%g'%f0, '%g'%f1] if f0<f1 else ['%g'%f1, '%g'%f0]
        win.autoscale.setChecked(False)
        win.f_min.setText(canvas.f_lim[0]); win.f_max.setText(canvas.f_lim[1])
        canvas.replot(); return True
#-------------------------------------------------------------------------------
class MouseFlat2D(Rect):
    def check(self, event): return event.x()<self.bmax[0] and self.bmin[1]<event.y()
    def p2f(self, axe, i): return _one2coord(self.xy_min[axe], self.xy_max[axe], self.logscale[axe],
                                             (self.bmax[1]-i)/self.bbox[1] if axe else (i-self.bmin[0])/self.bbox[0])
    def line_text_x(self, i, f, color='red'): self.line(i, self.bmin[1], i, self.bmax[1], 'gray'); self.text(i, self.bmax[1], '%g'%f, 'lt', color)
    def line_text_y(self, i, f, color='red'): self.line(self.bmin[0], i, self.bmax[0], i, 'gray'); self.text(0, i, '%g'%f, 'lb', color)
    def touch(self, canvas, event):
        x, y, c = event.x(), event.y(), 0
        #canvas.setCursor(QtCore.Qt.SizeVerCursor if x<self.bmin[0]/2 else QtCore.Qt.SizeHorCursor if (self.bmax[1]+canvas.im.height())/2<y else QtCore.Qt.ArrowCursor)
        if x<self.bmin[0]/2 or (self.bmax[1]+canvas.im.height())/2<y: self.text(x, y, 'Z', 'rb', 'red')
        if self.bmin[0]<x<self.bmax[0]: self.line_text_x(x, self.p2f(0, x)); c += 1
        if self.bmin[1]<y<self.bmax[1]: self.line_text_y(y, self.p2f(1, y)); c += 1
        if c==2: self.text(x, y, self.getval([x, y]).decode(), 'rb', 'red')
        return bool(self.plots)
    def press(self, canvas, event): # если кнопка левая начинаем выделение, иначе выключаем/включаем выделение
        if(event.buttons()&1): self.xy0 = [event.x(), event.y()]; return self  # левая кнопка
        if(event.buttons()&2):  # переклюдчаем выделение
            xy, replot = [event.x(), event.y()], False
            for a in (0, 1):
                if self.bmin[a]<xy[a]<self.bmax[a]:
                    if canvas.autolim(a): canvas.autolim_off(a)
                    else: canvas.autolim_on(a)
                    replot = True
            if replot: canvas.replot()
    def move(self, canvas, event):
        if self.bmin[0]<self.xy0[0]<self.bmax[0]: self.line(self.xy0[0], self.bmin[1], self.xy0[0], self.bmax[1], 'gray')
        if self.bmin[1]<self.xy0[1]<self.bmax[1]: self.line(self.bmin[0], self.xy0[1], self.bmax[0], self.xy0[1], 'gray')
        if self.touch(canvas, event): canvas.update(); return True
    def release(self, canvas, event):  # должен окончательно настроить канвас и вернуть True если нужна перерисовка
        xy0, xy1, c = self.xy0, [event.x(), event.y()], False
        for a in (0, 1):
            if xy0[a]==xy1[a] or xy0[a]<self.bmin[a]: continue
            canvas.autolim_off(a); c, A = True, canvas.axisID[a] 
            lim = [self.p2f(a, xy0[a]), self.p2f(a, xy1[a])]; inv = (lim[0]<lim[1])^(canvas.bmin[A]<canvas.bmax[A])
            canvas.bmin[A], canvas.bmax[A] = lim[inv], lim[1-inv]
        return c
    def wheel(self, canvas, event):
        xy, replot = [event.x(), event.y()], False
        if self.bmax[0]<xy[0] or xy[1]<self.bmin[1]: return False
        # print(xy, bzoom)
        for a in (0,1):
            if not (self.bmin[a]<=xy[a]<=self.bmax[a]): continue
            f0, f1, A = self.xy_min[a], self.xy_max[a], canvas.axisID[a]; sign = -1 if (event.angleDelta().y()<0)^(f0<f1) else 1
            F0, F1 = canvas.container.get_bmin(A), canvas.container.get_bmax(A)
            zoom = (xy[0]<self.bmin[0]/2 or self.bmin[0]<xy[0]) if a else (xy[1]<self.bmax[1] or (self.bmax[1]+canvas.im.height())/2<xy[1])
            if zoom: w_min = (self.bmax[1]-xy[1] if a else xy[0]-self.bmin[0])/self.bbox[a];  w_max = 1-w_min 
            else: w_min, w_max = -1, 1
            f0, f1 = f0-.1*(f1-f0)*w_min*sign, f1+.1*(f1-f0)*w_max*sign
            if zoom:
                f0 = max(F0, f0) if F0<F1 else min(F0, f0)
                f1 = min(F1, f1) if F0<F1 else max(F1, f1)
            elif ((f0<F0 or F1<f1) if F0<F1 else (f1<F1 or F0<f0)): continue
            canvas.bmin[A], canvas.bmax[A], replot = f0, f1, True;  canvas.autolim_off(a)
        if replot: canvas.replot()
        return replot    
#-------------------------------------------------------------------------------
class MouseFlat3D(Rect):
    def __init__(self, flat, center, bbox, logscale, getval):
        self.plots, self.flat, self.bbox, self.ee, self.bb, self.getval, self.cflips, self.pp = [], flat, bbox, [None]*2, [None]*2, getval, [0]*2, [0]*2
        ci = max((((flat.bounds>>((i-1)%4*2))&1)+((flat.bounds>>(i*2))&1), i) for i in range(4))[1]
        self.uplims = [flat.bmin[a] if (ci if ci<2 else 5-ci)&(1<<a) else flat.bmax[a] for a in (0, 1)]  # пределы флэта которые смотрят на пользователя
        self.c, self.cc = _Vec(*getattr(flat, 'abcd'[ci])), [None]*2; nC = center-self.c  # c, cc - вершина и орты отвечающие ВНЕШНИМ граням флэта
        for i in (0,1):
            self.bb[i] = self.ee[i] = _Vec(*getattr(flat, 'bd'[i]))-flat.a; self.ee[i] = self.ee[i]/bbox[i] # bb --- смещение || грани флэта в пикселях
            self.cc[i] = getattr(flat, ('bd', 'ac', 'db', 'ca')[ci][i])-self.c; self.cc[i] *= 1/bbox[i] #/abs(self.cc[i])
            self.cflips[i] = self.cc[i]*self.ee[i]<0  # развороты осей при переходе от сист. коорд. свяазнных с flat.a к self.c
            self.pp[i] = _Vec(self.cc[i][1], -self.cc[i][0]); self.pp[i] *= 1/abs(self.pp[i])
            if self.pp[i]*nC<0: self.pp[i] = -self.pp[i]
        self.ccbb, self.rot, self.sel, self.logscale = [v/abs(v)**2 for v in self.cc], None, None, logscale
    def event2rpos(self, event):  # ==> rpos, mask (битовая маска, какие оси валидны)
        xy0 = _Vec(event.x(), event.y()); xyA = xy0-self.flat.a
        rpos = [xyA[0]*self.flat.nX[i] + xyA[1]*self.flat.nY[i] for i in (0, 1)]
        if 0<=rpos[0]<=self.bbox[0] and 0<=rpos[1]<=self.bbox[1]: return rpos, (0,1)  # точно находимся внутри флэта
        xyC, axis = xy0-self.c, []
        for i in (0,1):
            if xyC*self.pp[i]>=0: continue
            rpos[i] = xyC*self.ccbb[i] #/(self.cc[i]) #/self.bbox[i]
            if self.cflips[i]: rpos[i] = self.bbox[i]-rpos[i] 
            if 0<=rpos[i]<=self.bbox[i]: axis.append(i) # ортогональная проекция на ВНЕШНЮЮ ось
        return rpos, axis
    def line_in(self, rpos, axe, color='gray'): #рисует линию проходящую через точку rpos ПЕРПЕНДИКУЛЯРНО оси axe, возвращает коорд. в пикс. на ВНЕШНЕЙ грани
        xy0 = _Vec(*self.flat.a)+self.ee[axe]*rpos[axe];  xy1 = xy0+self.bb[1-axe]
        self.line(xy0.x, xy0.y, xy1.x, xy1.y, color)
        return list(self.c+self.cc[axe]*(self.bbox[axe]-rpos[axe] if self.cflips[axe] else rpos[axe]))
    def check(self, event): return bool(self.event2rpos(event)[1])
    def touch(self, canvas, event):
        x, y = event.x(), event.y(); rpos, axis = self.event2rpos(event) #; print(rpos, axis)
        if len(axis)==1:
            self.line(x, y, *self.line_in(rpos, axis[0]), color='black')
            #canvas.setCursor(QtCore.Qt.SizeBDiagCursor if self.pp[axis[0]]*(_Vec(event.x(), event.y())-self.c)<-50 else QtCore.Qt.ArrowCursor)
            if self.pp[axis[0]]*(_Vec(x, y)-self.c)<-40: self.text(x, y, 'Z', 'rb', 'red')
        else: self.line_in(rpos, axis[0]); self.line_in(rpos, axis[1])
        if axis==(0,1): self.text(x, y, self.getval([x, y]).decode(), 'rb', 'red')
        return bool(self.plots)
    def press(self, canvas, event):  # если кнопка левая начинаем выделение/вращение, иначе выключаем/включаем выделение
        rpos, axis = self.event2rpos(event) 
        if(event.buttons()&1):
            if len(axis)==2: self.rot = [event.x(), event.y()]     # вращение
            else: self.sel = rpos, axis[0], [event.x(), event.y()] # начинаем выделение
            return self
        if(event.buttons()&2):  # переключаем выделение
            for a in axis:
                A = self.flat.axis[a]
                if canvas.autolim(A): canvas.autolim_off(A)
                else: canvas.autolim_on(A)
            if axis: canvas.replot()
    def move(self, canvas, event):
        if self.rot:
            x, y = event.x(), event.y()
            canvas.th_phi[0] -= (y-self.rot[1])*.1
            canvas.th_phi[1] += (x-self.rot[0])*.1
            self.rot = x, y
            canvas.replot()
        elif self.sel:
            self.line(*(self.sel[2]+self.line_in(*self.sel[:2])), color='red')
            rpos, axis = self.event2rpos(event)
            if axis: self.line(event.x(), event.y(), *self.line_in(rpos, axis[0]), color='green')
            canvas.update()
    def release(self, canvas, event):  # должен окончательно настроить канвас и вернуть True если нужна перерисовка
        if self.rot: self.rot = None; return
        rpos0, a = self.sel[:2];  rpos1, a1 = self.event2rpos(event); self.sel, A = None, self.flat.axis[a]
        if not a1 or rpos0[a]==rpos1[a] or a!=a1[0]: return  # or xy0[a]<self.bmin[a]: continue
        canvas.autolim_off(A); AA = canvas.axisID[A] 
        lim = [self.p2f(a, rpos0[a]), self.p2f(a, rpos1[a])]; inv = (lim[0]<lim[1])^(canvas.bmin[AA]<canvas.bmax[AA])
        canvas.bmin[AA], canvas.bmax[AA] = lim[inv], lim[1-inv]
        return True
    def dblclick(self, canvas, event):
        rpos, axis = self.event2rpos(event) 
        if not bool(event.buttons()&1): return        
        if len(axis)==1:        
            a = axis[0]; A =self.flat.axis[a]; AA = canvas.axisID[A]
            lim = [canvas.container.get_bmin(AA), canvas.container.get_bmax(AA)] if canvas.autolim(A)  else [canvas.bmin[AA], canvas.bmax[AA]]
            if rpos[a]<canvas.plotter.get_bbox(A)/2: canvas.bmin[AA], canvas.bmax[AA] = lim[0], (lim[0]+lim[1])/2
            else: canvas.bmin[AA], canvas.bmax[AA] = (lim[0]+lim[1])/2, lim[1]
            canvas.autolim_off(A)
            canvas.replot()
        else:
            a = 3-self.flat.axis[0]-self.flat.axis[1]; A = canvas.axisID[a]; canvas.sposf[A] = canvas.uplims[a]; canvas.autopos_off(A)
            if a!=2: canvas.axisID[a], canvas.axisID[2] = canvas.axisID[2], A
            canvas.win.D3.setCurrentIndex(0)
            canvas.full_replot()
    def p2f(self, axe, fpos): return _one2coord(self.flat.bmin[axe], self.flat.bmax[axe], self.logscale[axe], fpos/self.bbox[axe])
    def make_up(self, container): return Rect.make_up(self, container) if self.plots else container.make_up() #container.light_replot()
    def wheel(self, canvas, event):
        rpos, axis = self.event2rpos(event)
        if not axis: return False
        replot = False
        for a in axis:
            A = self.flat.axis[a]; AA = canvas.axisID[A];  F0, F1 = canvas.container.get_bmin(AA), canvas.container.get_bmax(AA)
            f0, f1 = (self.flat.bmin[a], self.flat.bmax[a]) if canvas.autolim(A) else (canvas.bmin[AA], canvas.bmax[AA])
            sign, zoom = (-1 if (event.angleDelta().y()<0)^(f0<f1) else 1), not -40<self.pp[a]*(_Vec(event.x(), event.y())-self.c)<0            
            if zoom: w_min = rpos[a]/self.bbox[a];  w_max = 1-w_min 
            else: w_min, w_max = -1, 1
            #print('<<<', self.flat.axis[a], zoom, f1-f0)
            ff = f1-f0; f0, f1 = f0-.1*ff*w_min*sign, f1+.1*ff*w_max*sign
            if zoom:
                f0 = max(F0, f0) if F0<F1 else min(F0, f0)
                f1 = min(F1, f1) if F0<F1 else max(F1, f1)
            elif ((f0<F0 or F1<f1) if F0<F1 else (f1<F1 or F0<f0)): continue
            #print('>>>', self.flat.axis[a], zoom, f1-f0)
            canvas.bmin[AA], canvas.bmax[AA], replot = f0, f1, True;  canvas.autolim_off(A)
        if replot: canvas.replot()
        return replot    
#-------------------------------------------------------------------------------
class _Vec:
    def __init__(self, x, y): self.x, self.y = x, y
    def __repr__(self): return '_Vec(%r, %r)'%(self.x, self.y)
    def __getitem__(self, i):
        if 0<=i<=1: return self.y if i else self.x
        raise IndexError(i)
    def __setitem__(self, i, v): setattr(self, 'xy'[i], v)
    def __mul__(self, r): return _Vec(self.x*r, self.y*r) if type(r) in (int, float) else self.x*r[0]+self.y*r[1]
    def __rmul__(self, r): return self*r
    def __truediv__(self, r): return _Vec(self.x/r, self.y/r)
    def __imul__(self, r): self.x *= r; self.y *= r; return self
    def __add__(self, r): return _Vec(self.x+r[0], self.y+r[1])
    def __radd__(self, r): return _Vec(self.x+r[0], self.y+r[1])
    def __sub__(self, r): return _Vec(self.x-r[0], self.y-r[1])
    def __rsub__(self, r): return _Vec(r[0]-self.x, r[1]-self.y)
    def __neg__(self): return _Vec(-self.x, -self.y)
    def __abs__(self): return (self.x**2+self.y**2)**.5
#-------------------------------------------------------------------------------
