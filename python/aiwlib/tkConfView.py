# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2017-2020  Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0

Этот модуль полностью создает панель с настройками отображения для uplt
'''

import os, aiwlib.view
from tkFileDialog import *
from tkWidgets import *
#-------------------------------------------------------------------------------
#  save and animate
#-------------------------------------------------------------------------------
class SaveAnimate:
    def __init__(self, panel, content, row=0, column=0):  
        self._frame1 = Frame(panel); self._frame1.grid(row=row, column=column, sticky=W) 
        Button(self._frame1, text='save', command=self.save_image).pack(side=LEFT)
        self.asave, self.atype = aiwCheck(self._frame1, 'by:', False, pack={'side':LEFT}), TkVar('---')
        self.menu = OptionMenu(self._frame1, self.atype, '---'); self.menu.pack(side=RIGHT)

        self._frame2 = aiwFrame(panel, row=row+1, column=column, sticky=W)
        self.run, self.counter = aiwCheck(self._frame2, 'movie', False, command=self.run_animate, pack={'side':LEFT}), 0
        self.amin, self.astep, self.amax = [aiwEntry(self._frame2, (0,1,0)[i], label=('', ':', ':')[i], pack={'side':LEFT}, width=[4,3,5][i]) for i in (0,1,2)]
        self.content = content
    def config_scrolls(self):
        'определяет scrollbar-ов и настраивает меню'
        self.scrolls, scrolls = {}, []
        if len(self.content.data)>1: scrolls.append('file'); self.scrolls['file'] = self.content.main.file_num
        if len(self.content.data[self.content.ifile])>2: scrolls.append('frame'); self.scrolls['frame'] = self.content.main.frame_num
        for s in self.content.main.slices[:self.content.conf.dim-2]: scrolls.append(s.cget('label').split()[0]); self.scrolls[scrolls[-1]] = s
        if self.run.get() and not self.atype.get() in scrolls: self.run.set(False) # надо остановить анимацию
        if not scrolls: self._frame2.hide(); self.asave['state'] = 'disabled'; self.run.set(False)
        else: self._frame2.show(); self.asave.state(1)
        OptionMenu.__init__(self.menu, self._frame1, self.atype, *(scrolls if scrolls else ['---'])); self.menu.pack(side=RIGHT)
        if not self.atype.get() in scrolls: self.atype.set((scrolls+['---'])[0])
    def run_animate(self):
        if self.run.get(): self.content.replot()
    def on_replot(self):
        'вызывается из общего replot для анимации, если анимация продолжается возвращает True'
        if self.run.get():
            sc = self.scrolls[self.atype.get()]
            if self.asave.get():
                if not self.counter: os.system('rm -f %s*'%self.tmpdir)
                self.content.canvas.dump2png(self.tmpdir+'%06i.png'%self.counter)
            self.counter += 1;  sc.set(sc.get()+self.astep.get())
            if sc.get()<(min(sc.cget('to'), self.amax.get()) if self.amax.get() else sc.cget('to')): return True # это должно вызывать wroot.after_idle(replot)
            else:
                self.counter = 0; self.run.set(False); sc.set(self.amax.get())
                if self.asave.get():
                    dstfile = asksaveasfilename(defaultextension='.avi', filetypes=['AVI {.avi}', 'MPEG {.mpeg}'])
                    if dstfile: os.system('ffmpeg -i %s/%%06d.png -qscale 1 -y '%self.tmpdir+dstfile)
    def save_image(self):
        'запрашивает имя файла и сохраняет изображение в форматах .ps, .pdf или .png'
        fname = asksaveasfilename(defaultextension='.png', filetypes=['PNG {.png}', 'PDF {.pdf}', 'PostScript {.ps}'])
        if not fname: return
        if fname.endswith('.ps'): self.content.canvas.postscript(file=fname)
        elif fname.endswith('.pdf'):
            tmp = '/tmp/%s'%os.getpid(); self.content.canvas.postscript(file=tmp+'.ps');
            os.system('epstopdf %s.ps && mv %s.pdf "%s" && rm %s.ps'%(tmp, tmp, fname, tmp))
        elif fname.endswith('.png'): self.content.canvas.dump2png(fname)
        else: print fname, '--- unknown image type'
#-------------------------------------------------------------------------------
class ColorConf:
    paletters = dict((k[:-4], getattr(aiwlib.view, k)) for k in dir(aiwlib.view) if k.endswith('_pal'))
    def __init__(self, panel, content, row=2, column=0):
        frame = Frame(panel); frame.grid(row=row, column=column, sticky=E)
        self.f_min = aiwEntry(frame, 0., self.replot, label='f', pack={'side':LEFT, 'anchor':E}, width=10)
        self.f_max = aiwEntry(frame, 0., self.replot, label=':', pack={'side':RIGHT, 'anchor':E}, width=10)

        frame = Frame(panel); frame.grid(row=row+1, column=column, sticky=W)        
        self.autoscale, self.old_autoscale = aiwCheck(frame, 'autoscale', True, command=self.replot, pack={'side':LEFT}), False
        self.pal = TkVar('rainbow'); OptionMenu(frame, self.pal, *self.paletters.keys()).pack(side=RIGHT)
        self.pal.trace('w', self.replot)

        frame = Frame(panel); frame.grid(row=row+2, column=column, sticky=W)        
        self.logscale = aiwCheck(frame, 'logsc.', command=self.replot, grid={'row':0, 'column':0})
        self.modulus = aiwCheck(frame, 'module', command=self.replot, grid={'row':0, 'column':1})
        self.invert = aiwCheck(frame, 'invert', command=self.replot, grid={'row':0, 'column':2})

        self.content = content
    # def set_limits(self, f_min, f_max): self.autoscale.set(False); self.f_min.set(f_min); self.f_max.set(f_max)
    def get_pal(self): return self.paletters[self.pal.get()]
    def scale_limits(self, mul):
        if mul==0: lim = max(abs(float(x.get())) for x in (self.f_min, self.f_max)); ab = [-lim, lim]
        elif mul==-1: ab = [self.f_min.get(), 0]
        elif mul==1: ab = [0, self.f_max.get()]
        else:
            ab = [float(x.get())*mul for x in (self.f_min, self.f_max)]; sign = [-1 if x<0 else 1 for x in ab]
            let = ['%e'%x for x in map(abs, ab)]; ab = [s*float(('10' if x[0]>'5' else x[0])+'e'+x.split('e')[1]) for x, s in zip(let, sign)]
            if ab[0]==ab[1]: return
        self.set_limits(ab[0], ab[1])
    def get_limits(self):
        f_min, f_max = self.f_min.get(), self.f_max.get()
        if f_min==f_max: f_min -= .5; f_max += .5
        return f_min, f_max
    def set_limits(self, f_min, f_max): self.autoscale.set(0); self.f_min.set(f_min); self.f_max.set(f_max); self.replot()        
    def get(self):
        rz = [self.f_min.get(), self.f_max.get()]
        if rz[0]==rz[1]: rz[0] -= .5; rz[1] += .5
        if self.autoscale.get():
            if self.invert.get(): rz = -rz[1], -rz[0]
            if self.modulus.get(): rz = map(abs, rz)
        #f_min.set(rz[0]); f_max.set(rz[1])
        if self.logscale.get() and rz[0]<=0.: f_min.set(1e-16)
        color = aiwlib.view.CalcColor()
        for p in 'logscale modulus invert'.split(): setattr(color, p, getattr(self, p).get())
        color.init(self.get_pal(), rz[0], rz[1])
        #color.magn = color2.magn = msh.sizeof_cell_type==2 
        #if color.magn: magn_pal_init()
        return color
    def calc_min_max(self):
        if self.autoscale.get(): lim = self.content.f_min_max(); self.f_min.set('%g'%lim[0]); self.f_max.set('%g'%lim[1])
    def replot(self, *args):
        if self.autoscale.get()!=self.old_autoscale: self.old_autoscale = self.autoscale.get(); self.calc_min_max()
        self.content.plot_preview(); self.content.plot_canvas()
#-------------------------------------------------------------------------------
class MainConf:        
    def __init__(self, panel, panel_sz, content, row=5, column=0):
        self.content = content
        frame = Frame(panel); frame.grid(row=row, column=column)  # access to cell field
        self.offset_in_cell = aiwEntry(frame, 0, self.access_replot, 'offset in cell', pack={'side':LEFT}, width=3)
        self.type_in_cell = TkVar('float'); self.type_in_cell.trace('w', self.access_replot)
        OptionMenu(frame, self.type_in_cell, 'float', 'double').pack(side=RIGHT)

        self.ss_frame = aiwFrame(panel, row=row+1, column=column)  # show size & step
        self.size = [aiwEntry(self.ss_frame, 0,  label=('size', '')[i], grid={'row':0, 'column':i}, width=9, state='readonly') for i in (0, 1)]
        self.step = [aiwEntry(self.ss_frame, 0., label=('step', '')[i], grid={'row':1, 'column':i}, width=9, state='readonly') for i in (0, 1)]

        frame = Frame(panel); frame.grid(row=row+2, column=0)
        self.flip = [aiwCheck(frame, ('X', 'Y flip   ')[i], command=self.flip_replot, grid={'row':0, 'column':i}) for i in (0,1)]
        self.cell_bound = aiwCheck(frame, 'show cells', command=self.cell_bound_replot, grid={'row':0, 'column':3, 'sticky':E})

        self.interp_frame, self.interp = aiwFrame(panel, row=row+3, column=0), [TkVar('const'), TkVar('const'), 'const', 'const']
        Label(self.interp_frame, text='interp').pack(side=LEFT)
        for i in (0,1):
            OptionMenu(self.interp_frame, self.interp[i], *'const linear cubic betas'.split()).pack(side=LEFT)
            self.interp[i].trace('w', self.interp_replot)

        self.axes_frame = aiwFrame(panel, row=row+4, column=column)  # show axes <<<
        self.axes = [TkVar('X'), TkVar('Y'), 'X', 'Y']; Label(self.axes_frame, text='axes').pack(side=LEFT) # значения осей и их старые значения
        for i in (0, 1): self.axes[i].trace('w', self.change_axes)        
        self.axes_menu, self.axes_list = [OptionMenu(self.axes_frame, self.axes[i], *'XYZ') for i in (0, 1)], ['X', 'Y', 'Z']
        for om in self.axes_menu: om.pack(side=LEFT)
        
        self.sph_frame = aiwFrame(panel, row=row+5, column=column, sticky=E)  # show sphere
        self.sph_phi0 = aiwEntry(self.sph_frame, 0, self.sph_replot, 'sphere phi0', grid={'row':0, 'column':0}, width=3)
        self.sph_rank = aiwEntry(self.sph_frame, 0,  label='rank', grid={'row':0, 'column':1}, width=1, state='readonly')
        self.mollweide, self.sph_interp = [aiwCheck(self.sph_frame, ('mollweide', 'interp')[i], False,
                                                    command=self.sph_replot, grid={'row':1, 'column':i}) for i in (0,1)]

        self.file_num = aiwScaleSH(panel, 'file number', command=self.content.file_replot, length=panel_sz-2, showvalue=YES, #<<<
                                   orient='horizontal', to=0, grid={'row':row+6, 'column': column}) # relief=RAISED, ) 
        self.frame_num = aiwScaleSH(panel, 'frame number', command=self.content.frame_replot, length=panel_sz-2, showvalue=YES,
                                    orient='horizontal', to=0, grid={'row':row+7, 'column': column}) #relief=RAISED, )
        frame = Frame(panel); frame.grid(row=row+8, column=column)   # slices
        self.slices = [aiwScaleSH(frame, '', command=self.slice_replot, length=panel_sz-2, showvalue=YES,
                                  orient='horizontal', to=0, grid={'row':i, 'column': 0}) for i in range(content.max_dim)]
        for s in self.slices: s.hide()

        #self._update_mode =  False
    def change_axes(self, *args):
        #if self._update_mode: return # блокировка настройки из self.update
        axe = int(args[0]==self.axes[1]._name)   # номер оси которая была изменена
        old, self.axes[2+axe] = self.axes[2+axe], self.axes[axe].get()  # запоминаем последнее значение
        if self.axes[0].get()==self.axes[1].get(): self.axes[1-axe].set(old)
        elif self.axes[axe].get()!=old:
            self.config_axes(self.content.conf)
            if self.content.color.autoscale.get(): self.content.color.calc_min_max()
            self.content.plot_preview(); self.content.plot_canvas()
    def config_axes(self, conf):
        'вызывается при изменении осей для того же объекта сетки'
        axes, isc = [a.get() for a in self.axes[:2]], 0
        for i in range(conf.dim):
            if conf.name(i) in axes:
                j = axes.index(conf.name(i))
                conf.axes[j] = i
                self.size[j].set(str(conf.size[i]))
                self.step[j].set('%g'%conf.step[i])
                self.flip[j].set(conf.get_flip(j))
                self.interp[j].set(['const',  'linear', 'cubic', 'betas'][conf.get_interp(j)])
            else:
                sc = self.slices[isc]; isc += 1
                sc.config(to=conf.size[i]-1, label='%s [%i](%g:%g)'%(conf.name(i), conf.size[i], conf.bmin0[i], conf.bmax0[i]))
                sc.set(conf.get_slice_pos(i))
    def update(self, conf):
        'настраивает все элементы согласно conf и content'
        self.ss_frame.set_visible(conf.features&conf.opt_step_size)
        for i in (0,1): self.flip[i].state(bool(conf.features&conf.opt_flip))
        self.cell_bound.state(bool(conf.features&conf.opt_cell_bound))
        self.interp_frame.set_visible(conf.features&conf.opt_interp)
        self.axes_frame.set_visible(conf.features&conf.opt_axes)
        self.sph_frame.set_visible(conf.features&conf.opt_mollweide)

        if len(self.content.data)<=1: self.file_num.hide()
        else: self.file_num.show(); self.file_num.config(to=len(self.content.data)-1)
        if len(self.content.data[self.content.ifile])<=2: self.frame_num.hide()
        else: self.frame_num.show(); self.frame_num.config(to=len(self.content.data[self.content.ifile])-2)

        if not conf.features&conf.opt_axes:
            self.axes_frame.hide()
            for s in self.slices: s.hide()
        else:
            self.axes_frame.show()
            for s in self.slices[:conf.dim-2]: s.show()
            for s in self.slices[conf.dim-2:]: s.hide()
            
            axes = [conf.name(i) for i in range(conf.dim)]
            for a, m in zip(self.axes, self.axes_menu): OptionMenu.__init__(m, self.axes_frame, a, *axes); m.pack(side=LEFT)
            # что делать если в новой сетке другой набор осей?
        self.config_axes(conf)
    #--- простые перерисовки ---
    def access_replot(self, *args):
        self.content.conf.offset_in_cell = int(self.offset_in_cell.get())
        self.content.conf.type_in_cell = {'float':0, 'double':1}[self.type_in_cell.get()]
        self.content.replot()
    def flip_replot(self):
        for i in (0,1): self.content.conf.set_flip(i, self.flip[i].get())
        self.content.plot_preview(); self.content.plot_canvas()
    def interp_replot(self, *args):
        if self.interp[2]==self.interp[0].get() and self.interp[3]==self.interp[1].get(): return
        self.interp[2] = self.interp[0].get(); self.interp[3] = self.interp[1].get()
        for i in (0,1): self.content.conf.set_interp(i, {'const':0, 'linear':1, 'cubic':2, 'betas':3}[self.interp[i].get()])
        self.content.plot_preview(); self.content.plot_canvas()
    def cell_bound_replot(self): self.content.conf.cell_bound = self.cell_bound.get(); self.content.plot_canvas()
    def sph_replot(self, *args):
        try:
            print self.content.conf.sph_phi0, repr(self.sph_phi0.get())
            self.content.conf.sph_phi0 = float(self.sph_phi0.get())
        except: pass
        self.content.conf.mollweide, self.content.conf.sph_interp  = self.mollweide.get(), self.sph_interp.get()
        self.content.replot()
    def slice_replot(self, *args):
        for i in range(self.content.conf.dim-2): self.content.conf.set_slice_pos(self.slices[i].cget('label').split()[0], self.slices[i].get())
        #if self.content.color.autoscale.get(): self.content.color.calc_min_max()
        #self.content.plot_preview(); self.content.plot_canvas()
        self.content.replot()
#-------------------------------------------------------------------------------
class PlotConf: # шрифт и размеры тиков, скрывать их по умолчанию, показывать по какой то кнопке?
    def __init__(self, panel, content, row=14, column=0):
        frame = Frame(panel); frame.grid(row=row, column=column)
        self.frame = aiwFrame(panel, row=row+1, column=column) 
        self.settings = aiwCheck(frame, 'show settings      ', True, command=self.frame.switch, pack={'side':LEFT, 'anchor':W})
        Button(frame, text='replot', command=content.file_replot).pack(side=RIGHT, anchor=E)

        self.title = aiwEntry(self.frame, '%(head)s', content.plot_canvas, 'title', width=19, pack={'side':TOP, 'anchor':E})
        self.x_label, self.y_label, self.z_label = [aiwEntry(self.frame, '', content.plot_canvas, 'XYZ'[i]+' label', width=17,
                                                             pack={'side':TOP, 'anchor':E}) for i in (0,1,2)]

        self.font = aiwEntry(self.frame, 'FreeMono 14', content.plot_canvas, 'font', width=19, pack={'side':TOP, 'anchor':E})

        frame = Frame(self.frame); frame.pack(side=TOP) 
        self.tic_len, self.tic_width, self.border, self.pal_xsz = [aiwEntry(frame, v, content.plot_canvas, l, width=w, pack={'side':LEFT}) for v, l, w in
                                                                   [(10, 'tic sz', 2), (1, '', 1), (1, ' border', 1), (30, ' pal.sz', 2)]]
    def get_font(self): tfont = self.font.get().split(); tfont[1] = int(tfont[1]); return tfont 
    def get(self): return {'font': self.get_font(), 'tic_sz': (self.tic_len.get(), self.tic_width.get()), 'border': self.border.get()}
#-------------------------------------------------------------------------------
# как то оптимизировать replot в заивисмости от того кто его вызывал?
