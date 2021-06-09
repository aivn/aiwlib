# Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
# Licensed under the Apache License, Version 2.0

from PyQt5 import QtWidgets, QtGui, QtCore
import time
from tics import *
#-------------------------------------------------------------------------------
class Canvas(QtWidgets.QWidget):
    # каждый раз класс создается заново (императивно) или обновляется то что было создано (декларативно)?
    def __init__(self, win):
        super().__init__(win)
        # plt_type: 1 бит - надо ли создавать картинку,  2 бит - надо ли масштабировать картинку, 3 бит - надо ли рисовать мышь
        self.win, self.plt_type, self.mouse_coord, self.select_mode = win, 1, (0, 0), 0  
        self.X, self.Y, self.im = [0]*3, [0]*3, None  # границы подобластей в пикселях, нужны для обработки событий мыши и пр
        self.slices_table = []  # номера осей отображаемых на срезах
        self.setCursor(QtCore.Qt.BlankCursor)
        self.setMouseTracking(True)
        self.show()
    def save(self, path):
        path, ok = QtWidgets.QFileDialog.getSaveFileName(self, caption='Save Image As...', filter='Images (*.png)')
        if ok:
            if not path.endswith('.png') and not path.endswith('.PNG'): path += '.png'
            self.grab().save(path, 'PNG')
            #pixmap = self.canvas.grab()
            #pixmap_to_save = pixmap.scaled(self.w.value(), self.h.value(), \
            #    transformMode = Qt.SmoothTransformation if self.smooth.isChecked() else Qt.FastTransformation)
            #pixmap_to_save.save(path, 'PNG')
    #--------------------------------------------------------------------------
    def leaveEvent(self, event):
        self.plt_type &= 3
        self.update()
    def paintEvent(self, event):
        t0, T = time.time(), 0.
        win = self.win; wsz = win.centralwidget.size(); sz_x, sz_y = wsz.width()-262, wsz.height(); self.X[2], self.Y[2] = sz_x, sz_y
        tl, pw, bw, ps  = int(win.tics_length.text()), int(win.pal_width.text()), int(win.border_width.text()), int(win.pal_space.text())
        
        color.arr_length, color.arr_width, color.arr_spacing = int(win.arr_length.text()), int(win.arr_width.text()), float(win.arr_spacing.text())
        if color.arr_length<=color.arr_width: color.arr_width = int(color.arr_length/1.2);  win.arr_width.setText(str(color.arr_width))
        color.modulus, color.nan_color = win.modulus.isChecked(), int(win.nan_color.text(), base=16)
        color.init(bytes(win.paletter.itemText(win.paletter.currentIndex()), 'utf8'),
                   float(win.f_min.text()), float(win.f_max.text()), win.logscale.isChecked())

        if factory.get_file_sz(win.filenum.value())==1: win.fr_framenum.hide()
        else: win.framenum.setMaximum(factory.get_file_sz(win.filenum.value())-1); win.fr_framenum.show()
        global mesh;  mesh = factory.get_frame(win.filenum.value(), win.framenum.value())

        if self.plt_type&1 or self.im is None:
            #---- рисуем главное изображение -----
            image = QtGui.QImage(sz_x, sz_y, QtGui.QImage.Format_RGB888)
            image.fill(0xFFFFFF)
            
            win.cellsize.setText(str(mesh.get_szT()))
            
            scene.autoscale, scene.autoscale_tot, scene.D3scale_mode = win.autoscale.isChecked(), win.autoscale_tot.isChecked(), win.D3scale_mode.currentIndex()
            for i in 'xyz': setattr(scene, 'd'+i, float(getattr(win, 'D3cell_'+i).text()))            
            scene.set_flip(0, win.xflip.isChecked()); scene.set_interp(0, win.xinterp.currentIndex()); scene.set_axe(0, win.x_axe.currentIndex())
            scene.set_flip(1, win.yflip.isChecked()); scene.set_interp(1, win.yinterp.currentIndex()); scene.set_axe(1, win.y_axe.currentIndex())
            scene.set_flip(2, win.zflip.isChecked()); scene.set_interp(2, win.zinterp.currentIndex()); scene.set_axe(2, win.z_axe.currentIndex())
            

            #accessor = qplt.QpltAccessor()
            accessor.Din, accessor.diff, accessor.vconv = win.celldim.currentIndex()+1, win.diff.currentIndex(), win.vconv.currentIndex()
            for i in range(3): accessor.set_offset(i, int(getattr(win, 'offset%i'%i).text()))
            accessor.ctype, accessor.minus = win.celltype.currentIndex(), win.invert.isChecked()
            try: accessor.mask = int(win.cellmask.text(), base=16)
            except: accessor.mask = 0; print('bad mask', win.cellmask.text())
            if not accessor.check(): print('invalid accessor'); return
                
            win.xsize.setText(str(mesh.get_bbox0(scene.get_axe(0))))
            win.ysize.setText(str(mesh.get_bbox0(scene.get_axe(1))))
            win.zsize.setText(str(mesh.get_bbox0(scene.get_axe(2))))
            show_axes_info()

            self.slices_table, sl_axe = [], 0  # номера осей отображаемых на срезах
            for i in range(1, mesh.get_dim()-1-scene.D3):                
                while sl_axe in [scene.get_axe(k) for k in range(2+scene.D3)]: sl_axe += 1
                sl, sz = getattr(win, 'slicenum%i'%i), mesh.get_bbox0(sl_axe)
                if sz>1:
                    getattr(win, 'fr_slice%i'%i).show(); sl.setMaximum(sz-1)
                    #scene.set_pos(sl_axe, mesh.pos2coord(sl_axe, sl.value()))
                    sl.setValue(mesh.coord2pos(sl_axe, scene.get_pos(sl_axe)))
                    getattr(win, 'slicelbl%i'%i).setText(mesh.get_axe_name(sl_axe).decode()+'[%i]'%sl.value())
                    self.slices_table.append(sl_axe)
                    #scene.set_pos(scene.get_axe(2), ...  ???)
                else: getattr(win, 'fr_slice%i'%i).hide()
                sl_axe += 1
                if sl_axe>=mesh.get_dim(): break
            for i in range(mesh.get_dim()-1-scene.D3, 7): getattr(win, 'fr_slice%i'%i).hide()  # скрываем лишние slices

            
            mesh.prepare(scene, accessor, color)
            #print('color limits', color.get_min(), color.get_max())
            
            paint = QtGui.QPainter(image)
            paint.setFont(QtGui.QFont(win.font.currentText(), win.font_sz.value()))
            h_font = text_sz('1', paint, True)
            paint.setPen(QtGui.QPen(QtCore.Qt.black, bw))

            # рисуем титул
            if win.title_text.text():
                paint.drawText(0, 0, sz_x, h_font, QtCore.Qt.AlignBottom|QtCore.Qt.AlignHCenter, win.title_text.text())
                self.Y[0] = h_font*2
            else: self.Y[0] = 0

            self.Y[1] = sz_y-h_font-2*tl  
            if win.x_text.text(): self.Y[1] -= h_font*2

            if win.show_pal.isChecked() and pw>0:  # рисуем палитру
                tics, max_tic_sz, stics = make_tics([color.get_min(), color.get_max()], color.get_logscale(), self.Y[1]-self.Y[0], True, paint, False)
                self.X[1] = sz_x - ps - pw - 2*tl - max_tic_sz - h_font*2*bool(win.z_text.text())                
                paint.drawRect(self.X[1]+ps, self.Y[0], pw-bw, self.Y[1]-self.Y[0])                
                t1 = time.time()
                pal_im = qplt.QpltImage(pw-2*bw, self.Y[1]-self.Y[0]-bw) #; pal_im.fill(0xFF)
                color.plot_pal(bytes(win.paletter.itemText(win.paletter.currentIndex()), 'utf8'), pal_im, True)
                T += time.time()-t1
                #qplt_im.dump(bytes('pal.ppm', 'utf8'))
                paint.drawImage(self.X[1]+ps+bw, self.Y[0]+bw, QtGui.QImage(pal_im.buf, pal_im.Nx, pal_im.Ny, QtGui.QImage.Format_RGB32))                
                paint.setPen(QtGui.QPen(QtCore.Qt.black, int(win.tics_width.text())))
                for y in stics: paint.drawLine(self.X[1]+ps+pw, self.Y[1]-y, self.X[1]+ps+pw+tl, self.Y[1]-y)
                for y, t in tics:
                    paint.drawLine(self.X[1]+ps+pw, self.Y[1]-y, self.X[1]+ps+pw+2*tl, self.Y[1]-y)
                    paint.drawText(self.X[1]+ps+pw+2*tl, self.Y[1]-y-h_font/2, max_tic_sz, h_font, QtCore.Qt.AlignVCenter, t)
                if win.z_text.text():
                    paint.rotate(-90)
                    paint.drawText(-self.Y[1], sz_x-h_font, self.Y[1]-self.Y[0], h_font, QtCore.Qt.AlignHCenter, win.z_text.text())
                    paint.rotate(90)
            else: self.X[1] = sz_x
                
            # рисуем ось Y
            paint.setPen(QtGui.QPen(QtCore.Qt.black, int(win.tics_width.text())))
            tics, max_tic_sz, stics = make_tics([mesh.get_bmin(1), mesh.get_bmax(1)], mesh.get_logscale(1),
                                                self.Y[1]-self.Y[0], True, paint, scene.get_flip(1))
            self.X[0] = max_tic_sz+2*tl+h_font*2*bool(win.y_text.text())
            for y in stics: paint.drawLine(self.X[0]-tl, self.Y[1]-y, self.X[0], self.Y[1]-y)
            for y, t in tics:
                paint.drawLine(self.X[0]-2*tl, self.Y[1]-y, self.X[0], self.Y[1]-y)
                paint.drawText(self.X[0]-2*tl-max_tic_sz, self.Y[1]-y-h_font/2, max_tic_sz, h_font, QtCore.Qt.AlignRight|QtCore.Qt.AlignVCenter, t)
            if win.y_text.text():
                paint.rotate(-90)
                paint.drawText(-self.Y[1], 0, self.Y[1]-self.Y[0], h_font, QtCore.Qt.AlignHCenter, win.y_text.text())
                paint.rotate(90)

            # рисуем ось X
            tics, max_tic_sz, stics = make_tics([mesh.get_bmin(0), mesh.get_bmax(0)], mesh.get_logscale(0),
                                                self.X[1]-self.X[0], False, paint, scene.get_flip(0))
            for x in stics: paint.drawLine(self.X[0]+x, self.Y[1], self.X[0]+x, self.Y[1]+tl)
            for x, t in tics:                
                paint.drawLine(self.X[0]+x, self.Y[1], self.X[0]+x, self.Y[1]+2*tl)
                paint.drawText(self.X[0]+x-max_tic_sz/2, self.Y[1]+2*tl, max_tic_sz, h_font, QtCore.Qt.AlignBottom|QtCore.Qt.AlignHCenter, t)
            if win.x_text.text(): paint.drawText(self.X[0], self.Y[2]-h_font, self.X[1]-self.X[0], h_font, QtCore.Qt.AlignBottom|QtCore.Qt.AlignHCenter, win.x_text.text())

            # рисуем главное изображение
            paint.setPen(QtGui.QPen(QtCore.Qt.black, bw))
            paint.drawRect(self.X[0], self.Y[0], self.X[1]-self.X[0], self.Y[1]-self.Y[0])
            
            t1 = time.time()
            qplt_im = qplt.QpltImage(self.X[1]-self.X[0]-bw, self.Y[1]-self.Y[0]-bw)#; qplt_im.fill(0xFF)
            mesh.plot(scene, accessor, color, qplt_im)
            T += time.time()-t1
            im = QtGui.QImage(qplt_im.buf, qplt_im.Nx, qplt_im.Ny, QtGui.QImage.Format_RGB32) #888)
            paint.drawImage(self.X[0]+bw, self.Y[0]+bw, im)
            # paint.drawImage(self.X[0]+bw, self.Y[0]+bw, QtGui.QImage(qplt_im.buf, qplt_im.Nx, qplt_im.Ny, QtGui.QImage.Format_RGB888))
    
            if scene.autoscale:  win.f_min.setText('%g'%color.get_min()); win.f_max.setText('%g'%color.get_max())
            
            self.im, image = image, self.im
            self.plt_type &= 6
            #--------------------------------------
        #if self.plt_type&2: print('RESIZE REPLOT!'); self.plt_type &= 5
        #self.plt_type = 1

        self.setGeometry(262, 0, sz_x, sz_y)
        paint = QtGui.QPainter(self)
        paint.drawImage(0, 0, self.im)

        if self.plt_type&4: # mouse move
            for coord, C in [(self.mouse_coord, QtCore.Qt.red)]+([(self.mouse_press_coord, QtCore.Qt.green)] if self.select_mode else []):
                x, y = coord; xy = (x-self.X[0])/(self.X[1]-self.X[0]), (self.Y[1]-y)/(self.Y[1]-self.Y[0])
                paint.setPen(QtGui.QPen(QtCore.Qt.gray, 1))
                if (not self.select_mode and self.X[1]<x and self.Y[0]<y and y<self.Y[1]) or self.select_mode&1:  # палитра
                    paint.drawLine(self.X[1]+ps, y, self.X[2], y); paint.setPen(C) 
                    paint.drawText(self.X[1], y, self.X[2]-self.X[1], 1000, QtCore.Qt.AlignRight, '%g'%color.get_f(xy[1]))
                if (not self.select_mode and x<self.X[1] and self.Y[0]<y and y<self.Y[1]) or self.select_mode&4:  # выбор по Y
                    paint.drawLine(self.X[0], y, self.X[1], y); paint.setPen(C)
                    paint.drawText(0, y-500, self.X[0], 1000, QtCore.Qt.AlignVCenter, '%g\n[%g]'%(mesh.get_coord(1, xy[1]), mesh.get_pos(1, xy[1])))                
                if (not self.select_mode and self.Y[0]<y and self.X[0]<x and x<self.X[1]) or self.select_mode&2:  # выбор по X
                    paint.setPen(QtCore.Qt.gray); paint.drawLine(x, self.Y[0], x, self.Y[1]);  paint.setPen(C) 
                    paint.drawText(x-500, self.Y[2]-500, 1000, 500, QtCore.Qt.AlignHCenter|QtCore.Qt.AlignBottom,
                                   '%g\n[%g]'%(mesh.get_coord(0, xy[0]), mesh.get_pos(0, xy[0])))
                if not self.select_mode and self.Y[0]<y and y<self.Y[1] and self.X[0]<x and x<self.X[1]:
                    text = mesh.get(scene, accessor, xy[0], xy[1]).decode()
                    rect = paint.boundingRect(x, y, 1000, 1000, 0, text)
                    paint.fillRect(rect, QtCore.Qt.white)
                    paint.setPen(QtCore.Qt.red); paint.drawText(x, y, rect.width(), rect.height(), 0, text)

        show_status = not hasattr(self, 'status')
        if T: self.status = 'x'.join('[%i]'%mesh.get_bbox0(i) for i in range(mesh.get_dim()))+' replot %0.2g/%0.2g sec'%(T, time.time()-t0)
        if show_status: win.statusbar.showMessage(self.status)
    #--------------------------------------------------------------------------        
    def mouseMoveEvent(self, event):
        self.plt_type |= 4
        self.mouse_coord = event.x(), event.y()
        self.update()
    def mousePressEvent(self, event):
        x, y = event.x(), event.y()
        if self.X[1]<x and self.Y[0]<y and y<self.Y[1]: self.select_mode = 1   # палитра
        if self.X[0]<x and x<self.X[1] and y>self.Y[0]: self.select_mode = 2   # X
        if x<self.X[1] and self.Y[0]<y and y<self.Y[1]: self.select_mode |= 4  # Y
        if self.select_mode: self.mouse_press_coord = x, y
    def mouseReleaseEvent(self, event):
        if self.select_mode:
            X = [self.mouse_press_coord[0], min(max(event.x(), self.X[0]), self.X[1])]
            Y = [self.mouse_press_coord[1], min(max(event.y(), self.Y[0]), self.Y[1])]
            for i in (0, 1): X[i], Y[i] = (X[i]-self.X[0])/(self.X[1]-self.X[0]), (self.Y[1]-Y[i])/(self.Y[1]-self.Y[0])
            X.sort(); Y.sort(); win = self.win
        if self.select_mode==1:  # палитра
            win.autoscale.setChecked(False)
            #color = qplt.QpltColor(bytes(win.paletter.itemText(win.paletter.currentIndex()), 'utf8'),
            #                       float(win.f_min.text()), float(win.f_max.text()), win.logscale.isChecked())
            win.f_min.setText('%g'%color.get_f(Y[0]))
            win.f_max.setText('%g'%color.get_f(Y[1]))
        if self.select_mode&6: mesh = factory.get_frame(win.filenum.value(), win.framenum.value())
        if self.select_mode&2: scene.set_min_max(0, mesh.get_coord(0, X[0]), mesh.get_coord(0, X[1]))  # ось X            
        if self.select_mode&4: scene.set_min_max(1, mesh.get_coord(1, Y[0]), mesh.get_coord(1, Y[1]))  # ось Y
        print('select [%g:%g]x[%g:%g]'%(scene.get_min(0), scene.get_max(0), scene.get_min(1), scene.get_max(1)))
        if self.select_mode: self.plt_type, self.select_mode = 1, 0; self.update()
    def mouseDoubleClickEvent(self, event):
        self.plt_type, x, y = 1, event.x(), event.y()
        if self.X[1]<x and self.Y[0]<y and y<self.Y[1]:  # вернуть автошкалирование палитры
            self.win.autoscale.setChecked(True)
            self.update()
        upd = False
        if self.X[0]<x and x<self.X[1] and y>self.Y[0]: scene.set_autoscale(0, True); upd = True  # вернуть автошкалирование по X
        if x<self.X[1] and self.Y[0]<y and y<self.Y[1]: scene.set_autoscale(1, True); upd = True  # вернуть автошкалирование по Y
        if upd: self.update()
    #---------------------------------------------------------------------------



