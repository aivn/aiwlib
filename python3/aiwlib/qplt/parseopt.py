# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2022 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

__all__ = ['parse_sys_argv', 'apply_sys_argv', 'dump_conf', 'load_conf']

import sys
from PyQt5 import QtGui
sys_argv = []
opts0 = '-2d -3d -xd -ln -abs -inv -noln -noabs -noinv -tas -notas'.split()
opts1 = '''-v -u -slice -flip -interp -density -opacity -mingrad -lbx -lby -lbz -lbf -ttl -font 
-rf -rx -ry -rz -r0 -r1 -r2 -r3 -r4 -r5 -pal -opal -palsz -tics -border -nan -conf -file -frame'''.split()
celltypes = ['float', 'double', 'bool', 'uint8_t', 'int8_t', 'uint16_t', 'int16_t', 'uint32_t', 'int32_t', 'uint64_t', 'int64_t']
#paletters = ['grey', 'inv_grey', 'black_red', 'green_blue', 'neg_pos1', 'neg_pos2', 'positive', 'rainbow', 'inv_rainbow', 'color']
axe2int = lambda axe: 'xyz'.index(axe) if axe in 'xyz' else int(axe)
#-------------------------------------------------------------------------------
def parse_sys_argv():
    'извлекает и сохраняет все настройки отображения из sys.argv'
    i = 1; del sys_argv[:]
    while i<len(sys.argv):
        if sys.argv[i] in opts0: sys_argv.append(sys.argv.pop(i))
        elif sys.argv[i] in opts1: sys_argv.append(sys.argv.pop(i)); sys_argv.append(sys.argv.pop(i))
        else: i += 1
#-------------------------------------------------------------------------------
def apply_sys_argv(canvas):
    'применяет сохраненные настройки отображения к канвасу'
    load_opts(canvas, sys_argv)
    if(sys.argv): canvas.full_replot()
    del sys_argv[:]
#-------------------------------------------------------------------------------
def dump_conf(canvas, path):
    fout, win, dim = open(path, 'w'), canvas.win, canvas.container.get_dim();  D3 = win.D3.currentIndex(); canvas.win2faai()
    fout.write('#%s\n-frame %s\n'%(canvas.container.fname().decode(), canvas.container.frame()))
    fout.write(['-2d\n', '-3d\n', '-xd\n'][D3])
    fout.write('-ln\n' if win.logscale.isChecked() else '-noln\n')
    fout.write('-abs\n' if win.modulus.isChecked() else '-noabs\n')
    fout.write('-inv\n' if win.invert.isChecked() else '-noinv\n')
    fout.write('-tas\n' if win.autoscale_tot.isChecked() else '-notas\n')
    fout.write('-rf :\n' if win.autoscale_tot.isChecked() else '-rf %s:%s\n'%(win.f_min.text(), win.f_max.text()))
    for a in range(dim): fout.write('-r%s '%a+(':\n' if canvas.autolim(a) else '%s:%s\n'%(canvas.bmin[a], canvas.bmax[a])))    
    fout.write('-v %g:%g\n'%tuple(canvas.th_phi))
    fout.write('-u %s:%s:%s\n'%(''.join(list(map(str, canvas.axisID))), win.offset0.text(), celltypes[win.celltype.currentIndex()]))
    for i, p in enumerate(canvas.sposf):
        if not canvas.autopos(i): fout.write('-slice %i%s\n'%(i, p))
    flips = ''.join([str(i) for i in range(dim) if canvas.get_flip(i)])
    if flips: fout.write('-flip %s\n'%flips)
    fout.write('-interp %s\n'%''.join([str(canvas.get_interp(axe)) for axe in range(dim)]))
    fout.write('-density %s\n-opacity %s\n-mingrad %s\n'%(win.density.value(), win.opacity.value(), win.mingrad.value()))
    for a in 'xyzf': fout.write('-lb%s %s\n'%(a, getattr(win, a+'_text').text()))
    fout.write('-ttl %s\n'%win.title_text.text())
    fout.write('-font %s:%s\n'%(win.font.currentText(), win.font_sz.value()))
    fout.write('-pal %s\n'%win.paletter.itemText(win.paletter.currentIndex()))
    fout.write('-opal %s\n'%''.join('01'[bool(canvas.D3tmask&(1<<i))] for i in range(canvas.pal_len)))
    fout.write('-palsz %s:%s:%s\n'%(win.pal_space.text(), win.pal_width.text(), win.pal_height.text()))
    fout.write('-tics %s:%s\n'%(win.tics_length.text(), win.tics_width.text()))
    fout.write('-border %s\n'%win.border_width.text())
    fout.write('-nan %s\n'%win.nan_color.text())
    fout.write('-frame %s\n'%win.framenum.value())
#-------------------------------------------------------------------------------
def load_opts(canvas, opts):
    iopt, win = 0, canvas.win
    while iopt<len(opts):
        if opts[iopt] in opts0: k, v = opts[iopt], ''; iopt += 1
        elif opts[iopt] in opts1 and iopt<len(opts)-1: k, v = opts[iopt], opts[iopt+1]; iopt += 2
        else: print('WARNING: incorret option %r'%opts[i]); iopt += 1; continue
        #print(iopt, k, v)
        if k in ['-2d', '-3d', '-xd']: win.D3.setCurrentIndex(['-2d', '-3d', '-xd'].index(k)); continue
        if k in ['-ln', '-noln']: win.logscale.setChecked(k=='-ln'); continue
        if k in ['-abs', '-noabs']: win.modulus.setChecked(k=='-abs'); continue
        if k in ['-inv', '-noinv']: win.invert.setChecked(k=='-inv'); continue
        if k in ['-tas', '-notas']: win.autoscale_tot.setChecked(k=='-tas'); continue
        if k=='-rf' and v==':':  win.autoscale.setChecked(True); continue
        elif k=='-rf' and v!=':':
            print(k, v)
            v = v.split(':'); win.f_min.setText(v[0]); win.f_max.setText(v[1])
            win.autoscale.setChecked(False)
            continue
        if len(k)==3 and k[:2]=='-r' and k[-1] in '012345xyz': # [<MIN>]:[<MAX>] --- задает пределы  отрисовки по осям 
            if v==':': canvas.autolim_on(axe2int(k[-1]))
            else: axe, lim = axe2int(k[-1]), v.split(':'); canvas.autolim_off(axe); canvas.bmin[axe] = float(lim[0]); canvas.bmax[axe] = float(lim[1]) 
            continue
        if k=='-v': canvas.th_phi = list(map(float, v.split(':'))); continue
        if k=='-u':
            axis, offset, ctype = v.split(':')
            if axis:
                canvas.axisID = list(map(axe2int, axis))
                for i, a in enumerate(canvas.axisID): getattr(win, 'xyz'[i]+'_axe').setCurrentIndex(a)
            if offset: win.offset0.setText(offset)
            if ctype: win.celltype.setCurrentIndex(celltypes.index(ctype))
            continue
        if k=='-slice':
            axe, pos = axe2int(v[0]), float(v[1:])
            canvas.autopos_off(axe); canvas.sposf[axe] = pos
            continue
        if k=='-flip' : # <AXIS>     --- включает разворот осей (например -flip xy или -flip 01)
            for axe in v: canvas.set_flip(axe2int(axe), True); print(canvas.get_flip(axe2int(axe)))
            canvas.faai2win()
            continue
        if k=='-interp': # <INTERP> --- задает интерполяцию, число 0...3 на каждую ось
            for axe, interp in enumerate(v): canvas.set_interp(axe, int(interp))
            continue
        if k=='-density': win.density.setValue(int(v)); continue # <DENSITY>
        if k=='-opacity': win.opacity.setValue(int(v)); continue # <OPACITY>
        if k=='-mingrad': win.mingrad.setValue(int(v)); continue # <MINGRAD>
        if len(k)==4 and k.startswith('-lb') and k[-1] in 'xyzf': getattr(win, k[-1]+'_text').setText(v); continue  # <TEXT> --- устанавливает метки по осям
        if k=='-ttl': win.title_text.setText(v); continue # <TEXT>  --- устанавливает заголовок
        if k=='-font':  # <NAME>:<SZ>   --- задает шрифт
            f, sz = v.strip().split(':'); sz = int(sz)
            win.font.setCurrentFont(QtGui.QFont(f, sz))
            win.font_sz.setValue(sz)
            continue 
        if k=='-pal': win.paletter.setCurrentText(v); continue # <PALETTER> --- задает палитру  
        if k=='-opal': canvas.D3tmask = sum(int(k)<<i for i, k in enumerate(v)); continue 
        if k=='-palsz': # <SPACE>:<WIDTH>:<HEIGHT> --- задает размеры палитры (по умолчанию 20:20:0.8)
            space, width, height = v.split(':')
            if space: win.pal_space.setText(space)
            if width: win.pal_width.setText(width)
            if height: win.pal_height.setText(height)
            continue
        if k=='-tics': # <LEN>:<WIDTH>
            l, w = v.split(':')
            if l: win.tics_length.setText(l)
            if w: win.tics_width.setText(w)
            continue 
        if k=='-border': win.border_width.setText(v); continue # <WIDTH>
        if k=='-nan': win.nan_color.setText(v); continue # <COLOR> --- задает цвет nan-ов (0x7F7F7F по умолчанию)
        if k=='-file': win.filenum.setValue(int(v)); continue # <NUM>  --- задает номер файла (источника) в списке, по умолчанию 0
        if k=='-frame': win.framenum.setValue(int(v)); continue # <NUM> --- задает номер кадра в  выбранном файле, по умолчанию 0
        if k=='-conf': load_conf(canvas, v); continue # <PATH> --- загружает .qplt файл с параметрами отрисовки
        raise Exception('incorrect option '+k)
#-------------------------------------------------------------------------------
def load_conf(canvas, path):
    fin, opts = open(path), []
    for l in fin:
        if not l.strip() or l[0]=='#': continue
        k, v = l.split(' ', 1) if ' ' in l else (l, '')
        k, v = k.strip(), v.strip()
        opts.append(k)
        if v: opts.append(v)
    load_opts(canvas, opts)    
#-------------------------------------------------------------------------------
