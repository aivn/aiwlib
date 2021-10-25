# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2020-21 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

from PyQt5 import QtWidgets, QtGui, QtCore
import os, sys, time
from .factory import *
from .tics import *
from .mouse import *
#-------------------------------------------------------------------------------
class Canvas(QtWidgets.QWidget):
    # каждый раз класс создается заново (императивно) или обновляется то что было создано (декларативно)?
    def __init__(self, win):
        super().__init__(win)
        # plt_type: 1 бит - надо ли создавать картинку,  2 бит - надо ли масштабировать картинку, 3 бит - надо ли рисовать мышь
        #self.win, self.im, self.plt_type, self.mouse_coord, self.select_mode = win, None, 1, None, 0  
        #self.X, self.Y, self.im = [0]*3, [0]*3, None  # границы подобластей в пикселях, нужны для обработки событий мыши и пр
        self.win, self.im, self.D3, self.mouse_coord, self.plotter = win, None, False, None, None
        self.th_phi, self.slices_table = [70., 60.], [] # номера осей отображаемых на ползунках срезов
        self.axisID, self.sposf, self.bmin, self.bmax, self.faai = [0, 1, 2], [0.]*6, [0.]*6, [0.]*6, ((1<<12)-1)<<6  #faai: 6b флипы, 12b autoscale, 12b интерполяция
        self.paletters = dict((i.split('.')[0], QtGui.QImage(os.path.dirname(__file__)+'/pals/'+i)) for i in os.listdir(os.path.dirname(__file__)+'/pals'))

        #self.setCursor(QtCore.Qt.BlankCursor)
        self.setMouseTracking(True)
        self.show()
    def get_ico(self, name, sz): return QtGui.QIcon(QtGui.QPixmap(self.paletters[name].scaled(sz[1], sz[0]).transformed(QtGui.QTransform().rotate(90))))
    #--------------------------------------------------------------------------
    def autopos(self, axe): return bool(self.faai&(1<<(6+2*axe)))
    #def autolim(self, axe): return bool(self.faai&(1<<(7+2*self.axisID[axe])))
    def autopos_on(self, axe):  self.faai |=   1<<(6+2*axe)  #; print('ON', axe, self.autopos(axe), self.axisID)
    def autopos_off(self, axe): self.faai &= ~(1<<(6+2*axe)) #; print('OFF', axe, self.autopos(axe), self.axisID)
    def autolim_on(self, axe):  self.faai |=   1<<(7+2*self.axisID[axe])
    def autolim_off(self, axe): self.faai &= ~(1<<(7+2*self.axisID[axe]))
    #--------------------------------------------------------------------------
    def faai2win(self):
        for i in range(2+bool(self.win.D3.currentIndex())):
            a = self.axisID[i]
            getattr(self.win, '%sflip'%'xyz'[i]).setChecked(bool(self.faai & (1<<a)))
            getattr(self.win, '%sinterp'%'xyz'[i]).setCurrentIndex((self.faai>>(18+2*i))&3)            
    def win2faai(self):
        for i in range(2+bool(self.win.D3.currentIndex())):
            a = self.axisID[i]
            if getattr(self.win, '%sflip'%'xyz'[i]).isChecked(): self.faai |= 1<<a
            else: self.faai &= ~(1<<a)
            interp = getattr(self.win, '%sinterp'%'xyz'[i]).currentIndex()
            self.faai &= ~(3<<(18+2*a)); self.faai |= interp<<(18+2*a)
            #print('interp=', interp, 'faai=', self.faai)
    #--------------------------------------------------------------------------
    def save(self):
        path, ok = QtWidgets.QFileDialog.getSaveFileName(self, caption='Save Image As...', filter='Images (*.png)')
        if ok:
            if not path.endswith('.png') and not path.endswith('.PNG'): path += '.png'
            self.grab().save(path, 'PNG')
            #pixmap = self.canvas.grab()
            #pixmap_to_save = pixmap.scaled(self.w.value(), self.h.value(), \
            #    transformMode = Qt.SmoothTransformation if self.smooth.isChecked() else Qt.FastTransformation)
            #pixmap_to_save.save(path, 'PNG')
    #--------------------------------------------------------------------------
    def heavy_replot(self, *args):
        print('heavy_replot',  args)
        win = self.win  
        if file_size(win.filenum.value())==1: win.fr_framenum.hide()
        else: win.fr_framenum.show(); win.framenum.setMaximum(file_size(win.filenum.value()))
        self.container = get_frame(win.filenum.value(), win.framenum.value())
        win.setWindowTitle('qplt: %s[%i]'%(self.container.fname().decode(), self.container.frame()))
        win.cellsize.setText(str(self.container.get_szT()))
        anames = [self.container.get_axe(i).decode() for i in range(self.container.get_dim())]
        for w in (win.D3, win.D3scale_mode): w.setEnabled(len(anames)>=3)
        if any(a>len(anames) for a in self.axisID): self.axisID = [0, 1, 2]  #???
        for i in (0, 1, 2)[:len(anames)]: w = getattr(win, 'xyz'[i]+'_axe'); w.clear(); w.addItems(anames); w.setCurrentIndex(self.axisID[i])
        self.faai2win()        
        for i in range(self.container.get_dim()):
            sl, sz = getattr(win, 'slicenum%i'%(i+1)), self.container.get_bbox(i); sl.setMaximum(sz)
            sl.setValue(0 if self.autopos(i) else max(min(1, self.container.coord2pos(self.sposf[i], i)+1), sz))
        for i in range(self.container.get_dim(), 6): getattr(win, 'fr_slice%i'%(i+1)).hide()            
        self.show_axis_info()
        self.light_replot()
    #--------------------------------------------------------------------------
    def slice_replot(self, pos, axe):
        print('slice_replot',  pos, axe)
        if pos: self.autopos_off(axe); self.sposf[axe] = self.container.pos2coord(pos-1, axe)
        else: self.autopos_on(axe)
        self.light_replot()
    #--------------------------------------------------------------------------
    def light_replot(self, *args):
        print('light_replot',  args)
        #if not hasattr(self, 'container'): return
        #canvas.plt_type |= 1; canvas.update()
        win, self.D3 = self.win, self.win.D3.currentIndex()  and self.container.get_dim()>2
        for i in range(2+bool(win.D3.currentIndex() and self.container.get_dim()>2)):  getattr(win, 'xyz'[i]+'size').setText(str(self.container.get_bbox(self.axisID[i])))
        for i in range(self.container.get_dim()):
            if i in self.axisID[:2+self.D3]: getattr(win, 'fr_slice%i'%(i+1)).hide(); continue
            getattr(win, 'fr_slice%i'%(i+1)).show()
            getattr(win, 'slicelbl%i'%(i+1)).setText(self.container.get_axe(i).decode()+'[%i]'%getattr(win, 'slicenum%i'%(i+1)).value())
        if self.D3: win.z_frame.show()
        else: win.z_frame.hide()

        try: accessor_mask = int(win.cellmask.text(), base=16)
        except: accessor_mask = 0; print('bad mask', win.cellmask.text())

        if self.plotter: self.plotter.free()
        self.win2faai()
        self.plotter = self.container.plotter(win.D3.currentIndex()*(self.container.get_dim()>2), #mode
                                              # int f_opt: 2 бита autoscale, 1 бит logscale, 1 бит модуль
                                              win.autoscale.isChecked()|(win.autoscale_tot.isChecked()<<1)|(win.logscale.isChecked()<<2)|(win.modulus.isChecked()<<3),    
                                              [float(win.f_min.text()), float(win.f_max.text())], # float f_lim[2]
                                              bytes(win.paletter.itemText(win.paletter.currentIndex()), 'utf8'), #  const char* paletter
                                              [int(win.arr_length.text()), int(win.arr_width.text())], # int arr_lw[2],
                                              float(win.arr_spacing.text()), int(win.nan_color.text(), base=16),  # float arr_spacing, int nan_color
                                              win.celltype.currentIndex(), win.celldim.currentIndex()+1, accessor_mask, # int ctype, int Din, int mask
                                              [int(getattr(win, 'offset%i'%i).text()) for i in (0,1,2)], # int offset[3], 
                                              win.diff.currentIndex(), win.vconv.currentIndex(), win.invert.isChecked(), # int diff, int vconv, bool minus
                                              self.axisID, self.sposf, self.bmin, self.bmax, self.faai, # 6 бит флипы, 12 бит autoscale, 12 бит интерполяция
                                              self.th_phi, [float(getattr(win, 'D3cell_'+i).text()) for i in 'xyz'], #float th_phi[2], float cell_aspect[3]
                                              win.D3scale_mode.currentIndex()) #int D3scale_mode
        self.update()
        win.statusbar.clearMessage(); win.statusbar.showMessage(getattr(self, 'status', ''))
    #--------------------------------------------------------------------------
    def show_axis_info(self):
        if self.win.show_axis_info.isChecked():
            for i in range(self.container.get_dim()):
                getattr(self.win, 'axe%i'%(1+i)).setText(self.container.get_axe(i).decode())
                getattr(self.win, 'min%i'%(1+i)).setText('%g'%self.container.get_bmin(i))
                getattr(self.win, 'max%i'%(1+i)).setText('%g'%self.container.get_bmax(i))
                getattr(self.win, 'step%i'%(1+i)).setText('%g'%self.container.get_step(i))
                getattr(self.win, 'logsc%i'%(1+i)).setText('V'*self.container.get_logscale(i))
                getattr(self.win, 'fr_axe%i'%(1+i)).show()
            for i in range(self.container.get_dim(), 8): getattr(self.win, 'fr_axe%i'%(1+i)).hide()
        else:
            for i in range(1,9): getattr(self.win, 'fr_axe%i'%i).hide()
    #--------------------------------------------------------------------------
    #def leaveEvent(self, event):
    #    self.plt_type &= 3
    #    self.update()
    def paintEvent(self, event):
        #   title  x0
        # y0 ------+-
        #          |
        #          |
        #          |
        # y1 ------+-
        #          | (x_sz, y_sz)
        
        print('paintEvent', event)
        t0, T = time.time(), 0. #; self.D3 = (self.win.D3.currentIndex() and self.container.get_dim()>2)
        win, plotter = self.win, self.plotter;  wsz = win.centralwidget.size(); sz_x, sz_y, y0 = wsz.width()-262, wsz.height(), 0
        tl, pw, bw, ps  = int(win.tics_length.text()), int(win.pal_width.text()), int(win.border_width.text()), int(win.pal_space.text())
        
        image = QtGui.QImage(sz_x, sz_y, QtGui.QImage.Format_RGB888)
        image.fill(0xFFFFFF)
        self.setGeometry(262, 0, sz_x, sz_y) #???
        paint = QtGui.QPainter(image)
        paint.setFont(QtGui.QFont(win.font.currentText(), win.font_sz.value()))        
        h_font = text_sz('1', paint, True)

        # рисуем титул
        if win.title_text.text(): paint.drawText(0, 0, sz_x, h_font, QtCore.Qt.AlignBottom|QtCore.Qt.AlignHCenter, win.title_text.text()); y0 = h_font*2
        if win.autoscale.isChecked():  win.f_min.setText('%g'%plotter.get_f_min()); win.f_max.setText('%g'%plotter.get_f_max())

        if win.show_pal.isChecked() and pw>0:  # рисуем палитру            
            pal_height = int(sz_y*float(win.pal_height.text()));  pal = Rect((0, sz_y/2-pal_height/2), (pw, sz_y/2+pal_height/2))
            tics, max_tic_sz, stics = make_tics([plotter.get_f_min(), plotter.get_f_max()], win.logscale.isChecked(), pal.bbox[1], True, paint, False)
            x1 = sz_x-ps-pw-2*tl-max_tic_sz-h_font*2*bool(win.z_text.text());  pal.move([x1+ps, 0])
            paint.drawImage(pal.bmin[0], pal.bmin[1], self.paletters[win.paletter.itemText(win.paletter.currentIndex())].scaled(pal.bbox[0], pal.bbox[1]))
            paint.setPen(QtGui.QPen(QtCore.Qt.black, bw)); paint.drawRect(pal.bmin[0], pal.bmin[1], pw, pal.bbox[1])
            paint.setPen(QtGui.QPen(QtCore.Qt.black, int(win.tics_width.text())))
            for y in stics: paint.drawLine(pal.bmin[0]+pw, pal.bmax[1]-y, pal.bmin[0]+pw+tl, pal.bmax[1]-y)
            for y, t in tics:
                paint.drawLine(pal.bmin[0]+pw, pal.bmax[1]-y, pal.bmin[0]+pw+2*tl, pal.bmax[1]-y)
                paint.drawText(pal.bmin[0]+pw+2*tl, pal.bmax[1]-y-h_font/2, max_tic_sz, h_font, QtCore.Qt.AlignVCenter, t)
            if win.z_text.text():
                paint.rotate(-90)
                paint.drawText(-pal.bmax[1], sz_x-h_font, pal.bbox[1], h_font, QtCore.Qt.AlignHCenter, win.z_text.text())
                paint.rotate(90)
        else: pal, x1 = False, sz_x
                    
        #if hasattr(self, 'plotter'):
        if self.D3: #--- 3D mode ----------------------------------------------------
            #main = Rect(0, y0, x_sz-pal.bmin[0]-ps, y_sz)
            x0, y1, plotter, extend = 0, sz_y, self.plotter, [0]*4
            plotter.set_image_size([x0,y0], [x1,y1])
            paint.setPen(QtGui.QPen(QtCore.Qt.black, bw))
            axis_modes = [(axe, getattr(win, '%stics3D'%'xyz'[axe]).currentIndex(), getattr(win, '%s_text'%'xyz'[axe]).text()) for axe in (0,1,2)]

            for i in range(plotter.flats_sz()):
                f = plotter.get_flat(i)
                for j in range(4):
                    if not f.bounds&(3<<2*j): continue
                    a, b = getattr(f, 'abcd'[j]), getattr(f, 'abcda'[j+1])
                    axe, mode, lbl = axis_modes[f.axis[j%2]]
                    text, ext, lines, lbl_pos = make_tics3D((plotter.get_bmin(axe), plotter.get_bmax(axe)), plotter.get_logscale(axe), a, b, plotter.center, 
                                                            tl, paint, getattr(self.win, '%sflip'%'xyz'[axe]).isChecked(), lbl*(mode!=0), axe==2)
                    extend = list(map(max, extend, ext))
            plotter.set_image_size([x0+extend[0],y0+extend[1]], [x1-extend[2],y1-extend[3]])
            x0, y0 = plotter.ibmin;  x1, y1 = plotter.ibmax
            t1 = time.time();  paint.drawImage(x0, y0, QtGui.QImage(plotter.plot(), x1-x0, y1-y0, QtGui.QImage.Format_RGB32));  T = time.time()-t1

            #paint.setPen(QtGui.QPen(QtCore.Qt.gray))
            for i in range(plotter.flats_sz()):
                f = plotter.get_flat(i)
                for j in range(4):
                    a, b = getattr(f, 'abcd'[j]), getattr(f, 'abcda'[j+1])  
                    paint.drawLine(*(a+b))
                    if not f.bounds&(3<<2*j): continue
                    axe, mode, lbl = axis_modes[f.axis[j%2]]
                    text, ext, lines, lbl_pos = make_tics3D((plotter.get_bmin(axe), plotter.get_bmax(axe)), plotter.get_logscale(axe), a, b, plotter.center, 
                                                            tl, paint, getattr(self.win, '%sflip'%'xyz'[axe]).isChecked(), lbl*(mode!=0), axe==2)
                    for l in lines: paint.drawLine(*l)
                    for p, t in text: paint.drawText(p[0], p[1], 1000, 1000, 0, t)
                    if lbl_pos:
                        if axe==2:
                            paint.translate(lbl_pos[0], lbl_pos[1]); paint.rotate(-90)
                            paint.drawText(0, 0, 1000, 1000, 0, lbl)
                            paint.rotate(90); paint.translate(-lbl_pos[0], -lbl_pos[1])
                        else: paint.drawText(lbl_pos[0], lbl_pos[1], 1000, 1000, 0, lbl)
            #paint.setPen(QtGui.QPen(QtCore.Qt.black))
        else:  #--- 2D mode ----------------------------------------------------
            y1 = sz_y - 2*tl - h_font*(1+2*bool(win.x_text.text()))
            paint.setPen(QtGui.QPen(QtCore.Qt.black, int(win.tics_width.text())))

            # рисуем ось Y
            tics, max_tic_sz, stics = make_tics([plotter.get_bmin(1), plotter.get_bmax(1)], plotter.get_logscale(1), y1-y0, True, paint, win.yflip.isChecked())            
            x0 = max_tic_sz+2*tl+h_font*2*bool(win.y_text.text())
            for y in stics: paint.drawLine(x0-tl, y1-y, x0, y1-y)
            for y, t in tics:
                paint.drawLine(x0-2*tl, y1-y, x0, y1-y)
                paint.drawText(x0-2*tl-max_tic_sz, y1-y-h_font/2, max_tic_sz, h_font, QtCore.Qt.AlignRight|QtCore.Qt.AlignVCenter, t)
            if win.y_text.text(): paint.rotate(-90);  paint.drawText(-y1, 0, y1-y0, h_font, QtCore.Qt.AlignHCenter, win.y_text.text());  paint.rotate(90)

            # рисуем ось X
            tics, max_tic_sz, stics = make_tics([plotter.get_bmin(0), plotter.get_bmax(0)], plotter.get_logscale(0), x1-x0, False, paint, win.xflip.isChecked())
            for x in stics: paint.drawLine(x0+x, y1, x0+x, y1+tl)
            for x, t in tics:                
                paint.drawLine(x0+x, y1, x0+x, y1+2*tl)
                paint.drawText(x0+x-max_tic_sz/2, y1+2*tl, max_tic_sz, h_font, QtCore.Qt.AlignBottom|QtCore.Qt.AlignHCenter, t)
            if win.x_text.text(): paint.drawText(x0, sz_y-h_font, x1-x0, h_font, QtCore.Qt.AlignBottom|QtCore.Qt.AlignHCenter, win.x_text.text())

            paint.setPen(QtGui.QPen(QtCore.Qt.black, bw));  paint.drawRect(x0, y0, x1-x0, y1-y0)
            plotter.set_image_size([x0,y0], [x1,y1])
            t1 = time.time();  paint.drawImage(x0, y0, QtGui.QImage(plotter.plot(), x1-x0, y1-y0, QtGui.QImage.Format_RGB32));  T = time.time()-t1
        #--- end of 2D/3D modes ------------------------------------------------
        
        self.im, image = image, self.im
        paint = QtGui.QPainter(self)
        paint.drawImage(0, 0, self.im)
        
        win.statusbar.clearMessage()
        win.statusbar.showMessage('x'.join('[%i]'%self.container.get_bbox(i) for i in range(self.container.get_dim()))+' replot %0.2g/%0.2g sec'%(T, time.time()-t0))
    #---------------------------------------------------------------------------
    def mousePressEvent(self, event):
        x, y = event.x(), event.y()
        if self.D3: self.mouse_coord = (x, y)
    def mouseMoveEvent(self, event):
        x, y = event.x(), event.y()
        if self.mouse_coord:
            self.th_phi[0] -= (y-self.mouse_coord[1])*.1
            self.th_phi[1] += (x-self.mouse_coord[0])*.1
            self.mouse_coord = (x, y)
            self.light_replot()
    def mouseReleaseEvent(self, event): self.mouse_coord = False
#-------------------------------------------------------------------------------



