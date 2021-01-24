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
        frame = Frame(panel); frame.grid(row=row, column=column, sticky=W) 
        Button(frame, text='save', command=self.save_image).pack(side=LEFT)
        self.asave, self.atype = aiwCheck(frame, 'by:', False, pack={'side':LEFT}), aiwOptionMenu(frame, pack={'side': RIGHT})

        self.aframe = aiwFrame(panel, row=row+1, column=column, sticky=W)
        self.run, self.counter = aiwCheck(self.aframe, 'movie', False, command=self.run_animate, pack={'side':LEFT}), 0
        self.amin, self.astep, self.amax = [aiwEntry(self.aframe, (0,1,0)[i], label=('', ':', ':')[i], pack={'side':LEFT}, width=[4,3,5][i]) for i in (0,1,2)]
        self.content = content
    def config_scrolls(self):
        'определяет словарь scrolls и настраивает меню'
        self.scrolls, scrolls = {}, []
        if len(self.content.data)>1: scrolls.append('file'); self.scrolls['file'] = self.content.main.file_num
        if len(self.content.data[self.content.ifile])>2: scrolls.append('frame'); self.scrolls['frame'] = self.content.main.frame_num
        for s in self.content.main.slices[:self.content.conf.dim-2]: scrolls.append(s.cget('label').split()[0]); self.scrolls[scrolls[-1]] = s
        if self.run.get() and not self.atype.get() in scrolls: self.run.set(False) # надо остановить анимацию
        if not scrolls: self.aframe.hide(); self.asave.state(0); self.run.set(False)
        else: self.aframe.show(); self.asave.state(1)
        self.atype.set_items(scrolls if scrolls else ['---'])
        self.scrolls_list = scrolls
    def next_scroll(self, *args):
        if self.scrolls_list: self.atype.set(self.scrolls_list[(self.scrolls_list.index(self.atype.get())+1)%len(self.scrolls_list)])
    def prev_scroll(self, *args):
        if self.scrolls_list: self.atype.set(self.scrolls_list[(self.scrolls_list.index(self.atype.get())-1)%len(self.scrolls_list)])
    def move_scroll(self, d):
        if self.scrolls:
            sc = self.scrolls[self.atype.get()]
            sc.set((sc.get()+d)%int(sc.cget('to')+1))
            if self.atype.get()=='file': self.content.file_replot()
            elif self.atype.get()=='frame': self.content.frame_replot()
            else: self.content.main.slice_replot()
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
    def save_image(self, *args):
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
        self.autoscale = aiwCheck(frame, 'f', True, command=self.replot, pack={'side':LEFT})
        self.f_min = aiwEntry(frame, 0., self.replot, label='', pack={'side':LEFT, 'anchor':E}, width=10)
        self.f_max = aiwEntry(frame, 0., self.replot, label=':', pack={'side':RIGHT, 'anchor':E}, width=10)
        self.f_min.state(0); self.f_max.state(0)

        frame = Frame(panel); frame.grid(row=row+1, column=column, sticky=W)        
        #self.autoscale, self.old_autoscale = aiwCheck(frame, 'autoscale', True, command=self.replot, pack={'side':LEFT}), False
        self.total_autoscale = aiwCheck(frame, 'tot.scale', False, command=self.replot, pack={'side':LEFT})
        self.pal = aiwOptionMenu(frame, items=self.paletters.keys(), trace=self.replot, default='rainbow', pack={'side':RIGHT})

        frame = Frame(panel); frame.grid(row=row+2, column=column, sticky=W)        
        self.logscale = aiwCheck(frame, 'logsc.', command=self.replot, grid={'row':0, 'column':0})
        self.modulus = aiwCheck(frame, 'module', command=self.replot, grid={'row':0, 'column':1})
        self.invert = aiwCheck(frame, 'invert', command=self.replot, grid={'row':0, 'column':2})

        self.content = content
    # def set_limits(self, f_min, f_max): self.autoscale.set(False); self.f_min.set(f_min); self.f_max.set(f_max)
    def get_pal(self, paletter=None): return self.paletters[paletter if paletter else self.pal.get()]
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
        rz = [self.f_min.get(), self.f_max.get()]
        if rz[0]==rz[1]: rz[0] -= .5; rz[1] += .5
        if self.autoscale.get():
            if self.invert.get(): rz = -rz[1], -rz[0]
            if self.modulus.get():
                if rz[0]*rz[1]<0: rz[1] = max(map(abs, rz)); rz[0] = 0
                else: rz = sorted(map(abs, rz))
        if self.logscale.get():
            if rz[0]<=0.: rz[0] = 1e-18
            if rz[1]<=0.: rz[1] = 1e-16
        return rz
    def set_limits(self, f_min, f_max): self.autoscale.set(0); self.f_min.set(f_min); self.f_max.set(f_max); self.replot()        
    def get(self, paletter=None):
        color = aiwlib.view.CalcColor()
        for p in 'logscale modulus invert'.split(): setattr(color, p, bool(getattr(self, p).get()))
        color.init(self.get_pal(paletter), *self.get_limits())
        #color.magn = color2.magn = msh.sizeof_cell_type==2 
        #if color.magn: magn_pal_init()
        return color
    def calc_min_max(self):
        if self.autoscale.get(): lim = self.content.f_min_max(self.total_autoscale.get()); self.f_min.set('%g'%lim[0]); self.f_max.set('%g'%lim[1])
    def replot(self, *args):
        self.f_min.state(not self.autoscale.get()); self.f_max.state(not self.autoscale.get())
        #if self.autoscale.get(): self.old_autoscale = self.autoscale.get();
        self.calc_min_max(); self.content.plot_preview(); self.content.plot_canvas()
#-------------------------------------------------------------------------------
class MainConf:
    ctypes = 'float double bool char uint8_t int8_t uint16_t int16_t uint32_t int32_t  uint64_t int64_t'.split()
    def __init__(self, panel, panel_sz, content, row=5, column=0):
        self.raw_access_frame = aiwFrame(panel, row=row, column=column)  # access to cell field
        self.offset_in_cell = aiwEntry(self.raw_access_frame, 0, self.access_replot, 'offset in cell', pack={'side':LEFT}, width=3)
        self.type_in_cell = aiwOptionMenu(self.raw_access_frame, items=self.ctypes, pack={'side':RIGHT}, trace=self.access_replot)

        self.cfa_frame = aiwFrame(panel, row=row+1, column=column)
        self.cfa_menu = aiwOptionMenu(self.cfa_frame, trace=self.access_replot, pack={'side':LEFT})
        self.cfa_ibit = aiwEntry(self.cfa_frame, -1, self.access_replot, label='ibit',pack={'side':RIGHT}, width=2)
        
        self.xfem_frame = aiwFrame(panel, row=row+2, column=column); Label(self.xfem_frame, text='xfem').pack(side=LEFT)
        self.xfem_pos = [aiwEntry(self.xfem_frame, 0, self.access_replot, '', pack={'side':RIGHT}, width=2) for i in (0,1)]
        self.xfem_mode = aiwOptionMenu(self.xfem_frame, trace=self.access_replot, items='node cell face phys'.split(), pack={'side':RIGHT})
        
        
        self.ss_frame = aiwFrame(panel, row=row+3, column=column)  # show size & step
        self.size = [aiwEntry(self.ss_frame, 0,  label=('size', '')[i], grid={'row':0, 'column':i}, width=9, state='readonly') for i in (0, 1)]
        self.step = [aiwEntry(self.ss_frame, 0., label=('step', '')[i], grid={'row':1, 'column':i}, width=9, state='readonly') for i in (0, 1)]

        frame = Frame(panel); frame.grid(row=row+4, column=0)
        self.flip = [aiwCheck(frame, ('X', 'Y flip   ')[i], command=self.flip_replot, grid={'row':0, 'column':i}) for i in (0,1)]
        self.cell_bound = aiwCheck(frame, 'show cells', command=self.cell_bound_replot, grid={'row':0, 'column':3, 'sticky':E})

        self.interp_frame = aiwFrame(panel, row=row+5, column=0); Label(self.interp_frame, text='interp').pack(side=LEFT)
        self.interp = [aiwOptionMenu(self.interp_frame, trace=self.interp_replot, items='const linear cubic betas'.split(), pack={'side':LEFT})
                       for i in (0,1)]+['const']*2

        self.axes_frame, self.axes_list = aiwFrame(panel, row=row+6, column=column), ['X', 'Y', 'Z']  # show axes <<<
        self.axes = [aiwOptionMenu(self.axes_frame, items='XYZ', trace=self.change_axes, default=i,
                                   pack={'side':LEFT}) for i in 'XY']+['X', 'Y']  # значения осей и их старые значения
        Label(self.axes_frame, text='axes').pack(side=LEFT)  
        
        self.sph_frame = aiwFrame(panel, row=row+7, column=column, sticky=E)  # show sphere
        self.sph_phi0 = aiwEntry(self.sph_frame, 0, self.sph_replot, 'sphere phi0', grid={'row':0, 'column':0}, width=3)
        self.sph_rank = aiwEntry(self.sph_frame, 0,  label='rank', grid={'row':0, 'column':1}, width=1, state='readonly')
        self.mollweide, self.sph_interp = [aiwCheck(self.sph_frame, ('mollweide', 'interp')[i], False,
                                                    command=self.sph_replot, grid={'row':1, 'column':i}) for i in (0,1)]

        frame = Frame(panel); frame.grid(row=row+8, column=column)   # seg-Y, 3D etc
        self.segy = aiwCheck(frame, 'seg-Y   ', command=self.segy_replot, grid={'row':0, 'column':0})
        self.plot3D = aiwCheck(frame, '3D', command=self.plot3D_replot, grid={'row':0, 'column':1})
        self.alpha = aiwEntry(frame, 0.5,  label='alpha', grid={'row':0, 'column':2}, width=5)
        
        self.file_num = aiwScaleSH(panel, 'file number', command=content.file_replot, length=panel_sz-2, showvalue=YES, #<<<
                                   orient='horizontal', to=0, grid={'row':row+9, 'column': column}) # relief=RAISED, ) 
        self.frame_num = aiwScaleSH(panel, 'frame number', command=content.frame_replot, length=panel_sz-2, showvalue=YES,
                                    orient='horizontal', to=0, grid={'row':row+10, 'column': column}) #relief=RAISED, )
        frame = Frame(panel); frame.grid(row=row+11, column=column)   # slices
        self.slices = [aiwScaleSH(frame, '', command=self.slice_replot, length=panel_sz-2, showvalue=YES,
                                  orient='horizontal', to=0, grid={'row':i, 'column': 0}) for i in range(content.max_dim)]
        for s in self.slices: s.hide()

        self.content = content
    def change_axes(self, *args):
        axe = int(args[0]==self.axes[1].var._name)   # номер оси которая была изменена
        if self.axes[0].get()==self.axes[1].get():  axe = 1-axe; self.axes[axe].set(self.axes[2] if self.axes[0].get()==self.axes[3] else self.axes[3])
        elif self.axes[axe].get()!=self.axes[2+axe]:
            self.config_axes(self.content.conf)
            if self.content.color.autoscale.get(): self.content.color.calc_min_max()
            self.content.plot_preview(); self.content.plot_canvas()
        self.axes[axe+2] = self.axes[axe].get()
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
                sc.config(to=conf.size[i]-1, label='%s %i [%g:%g] %g'%(conf.name(i), conf.size[i], conf.bmin0[i], conf.bmax0[i], conf.slice[i]))
                sc.set(conf.get_slice_pos(i))
    def next_axe(self, axe):
        axes = map(self.content.conf.name, range(self.content.conf.dim))
        self.axes[axe].set(axes[(axes.index(self.axes[axe].get())+1)%len(axes)])
    def prev_axe(self, axe):
        axes = map(self.content.conf.name, range(self.content.conf.dim))
        self.axes[axe].set(axes[(axes.index(self.axes[axe].get())-1)%len(axes)])
    def next_interp(self, axe):
        L = ['const',  'linear', 'cubic', 'betas']
        self.interp[axe].set(L[(L.index(self.interp[axe].get())+1)%len(L)])
    def prev_interp(self, axe):
        L = ['const',  'linear', 'cubic', 'betas']
        self.interp[axe].set(L[(L.index(self.interp[axe].get())-1)%len(L)])        
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
            for m in self.axes[:2]: m.set_items(axes)
            # что делать если в новой сетке другой набор осей?
        self.config_axes(conf)
        
        if self.content.conf.cfa_xfem_list: self.xfem_frame.show()
        else: self.xfem_frame.hide()
        if self.content.conf.cfa_list or self.content.conf.cfa_xfem_list: self.raw_access_frame.hide(); self.cfa_frame.show()
        if self.content.conf.cfa_xfem_list and self.xfem_mode.get()!='phys': 
            cfaL = [i.label for i in self.content.conf.cfa_xfem_list]
            self.cfa_menu.set_items(cfaL)
            self.content.conf.cfa = self.content.conf.cfa_xfem_list[cfaL.index(self.cfa_menu.get())]
        elif len(self.content.conf.cfa_list)>1: 
            cfaL = [i.label for i in self.content.conf.cfa_list]
            self.cfa_menu.set_items(cfaL)
            self.content.conf.cfa = self.content.conf.cfa_list[cfaL.index(self.cfa_menu.get())]
        elif len(self.content.conf.cfa_list)==1:  self.raw_access_frame.hide(); self.cfa_frame.hide(); self.xfem_frame.hide()
        else: 
            self.raw_access_frame.show(); self.cfa_frame.hide(); self.xfem_frame.hide()
            self.offset_in_cell.set(self.content.conf.cfa.offset)
            self.type_in_cell.set(self.ctypes[self.content.conf.cfa.typeID])

        self.segy.state(bool(conf.features&conf.opt_segy))
        self.plot3D.state(bool(conf.features&conf.opt_3D) and conf.dim==3)
        self.alpha.state(bool(conf.features&conf.opt_3D) and conf.dim==3)
    #--- простые перерисовки ---
    def access_replot(self, *args):
        if self.content.conf.cfa_xfem_list and self.xfem_mode.get()!='phys': 
            cfaL = [i.label for i in self.content.conf.cfa_xfem_list]
            self.cfa_menu.set_items(cfaL)
            self.content.conf.cfa = self.content.conf.cfa_xfem_list[cfaL.index(self.cfa_menu.get())]
            self.content.conf.cfa.ibit = self.cfa_ibit.get()
            self.content.conf.cfa.xfem_pos[0], self.content.conf.cfa.xfem_pos[1] = self.xfem_pos[0].get(), self.xfem_pos[1].get()
            self.content.conf.xfem_mode = 'node cell face phys'.split().index(self.xfem_mode.get())
        elif self.content.conf.cfa_list:
            cfaL = [i.label for i in self.content.conf.cfa_list]
            self.cfa_menu.set_items(cfaL)
            self.content.conf.cfa = self.content.conf.cfa_list[cfaL.index(self.cfa_menu.get())]
            self.content.conf.cfa.ibit = self.cfa_ibit.get()
            self.content.conf.xfem_mode = 'node cell face phys'.split().index(self.xfem_mode.get())
        else:            
            self.content.conf.cfa.offset = int(self.offset_in_cell.get())
            self.content.conf.cfa.typeID = self.ctypes.index(self.type_in_cell.get())
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
        try: self.content.conf.sph_phi0 = float(self.sph_phi0.get())
        except: pass
        self.content.conf.mollweide, self.content.conf.sph_interp  = bool(self.mollweide.get()), bool(self.sph_interp.get())
        self.content.replot()
    def slice_replot(self, *args):
        conf = self.content.conf
        for sc in self.slices[:conf.dim-2]:
            lbl = sc.cget('label'); coord = conf.set_slice_pos(lbl.split()[0], sc.get())
            sc.config(label=lbl.rsplit(' ', 1)[0]+' %g'%coord)
        #if self.content.color.autoscale.get(): self.content.color.calc_min_max()
        #self.content.plot_preview(); self.content.plot_canvas()
        self.content.replot()
    def segy_replot(self, *args):
        if self.segy.get():
            self.axes[1].set(self.axes[1]._items[0])
            self.content.conf.segy = True;
            self.flip[1].set(True); self.content.conf.set_flip(1, True)
        else: self.content.conf.segy = False 
        self.content.replot()
    def plot3D_replot(self, *args):
        self.content.conf.plot3D = self.plot3D.get()
        self.content.replot()
#-------------------------------------------------------------------------------
class PlotConf: # шрифт и размеры тиков, скрывать их по умолчанию, показывать по какой то кнопке?
    def __init__(self, panel, content, row=17, column=0):
        frame = Frame(panel); frame.grid(row=row, column=column)
        self.frame = aiwFrame(panel, row=row+1, column=column) 
        self.settings = aiwCheck(frame, 'show settings      ', False, command=self.frame.switch, pack={'side':LEFT, 'anchor':W})
        Button(frame, text='replot', command=content.file_replot).pack(side=RIGHT, anchor=E)

        self.title = aiwEntry(self.frame, '%(head)s', content.plot_canvas_e, 'title', width=19, pack={'side':TOP, 'anchor':E})
        self.x_label, self.y_label, self.z_label = [aiwEntry(self.frame, '', content.plot_canvas_e, 'XYZ'[i]+' label', width=17,
                                                             pack={'side':TOP, 'anchor':E}) for i in (0,1,2)]

        self.font = aiwEntry(self.frame, 'FreeMono 14', content.plot_canvas_e, 'font', width=19, pack={'side':TOP, 'anchor':E})

        frame = Frame(self.frame); frame.pack(side=TOP) 
        self.tic_len, self.tic_width, self.border, self.pal_xsz = [aiwEntry(frame, v, content.plot_canvas_e, l, width=w, pack={'side':LEFT}) for v, l, w in
                                                                   [(10, 'tic sz', 2), (1, '', 1), (1, ' border', 1), (30, ' pal.sz', 2)]]
        self.font_scale = aiwEntry(self.frame, 1., content.plot_canvas_e, 'canvas font scale', width=5, pack={'side':TOP, 'anchor':E})
        self.frame.hide()
    def get_font(self): tfont = self.font.get().split(); tfont[1] = int(tfont[1]); return tfont 
    def get(self): return {'font': self.get_font(), 'tic_sz': (self.tic_len.get(), self.tic_width.get()), 'border': self.border.get(), 'font_scale': self.font_scale.get()}
#-------------------------------------------------------------------------------
# как то оптимизировать replot в заивисмости от того кто его вызывал?
