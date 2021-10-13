# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2017,2020  Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

from Tkinter import *
#-------------------------------------------------------------------------------
TkVar = lambda x: {bool: BooleanVar, int: IntVar, str: StringVar, float: DoubleVar}[type(x)](value=x)
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
    def cget(self, *args, **kw_args): return self.scale.cget(*args, **kw_args)
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
class aiwOptionMenu:
    def __init__(self, root, items=['---'], pack=None, grid=None, trace=None, default=None):
        self.var, self.root, self.pack, self.grid = StringVar(), root, pack, grid
        self.menu = OptionMenu(root, self.var, *items)
        self.var.set(default if default else items[0])
        if self.pack: self.menu.pack(**self.pack)
        else: self.menu.grid(**self.grid)
        self._items = items
        if trace: self.var.trace('w', trace) # это должно вызываться самым последним        
    def get(self): return self.var.get()
    def set(self, val): self.var.set(val)
    def set_items(self, L): 
        self._items = L
        OptionMenu.__init__(self.menu, self.root, self.var, *L)
        if self.pack: self.menu.pack(**self.pack)
        else: self.menu.grid(**self.grid)
        if not self.var.get() in L: self.var.set(L[0])
#-------------------------------------------------------------------------------
#  show/hide
#-------------------------------------------------------------------------------
class aiwShowHide:
    def __init__(self, grid_args): self._show, self._grid_args = True, grid_args
    def hide(self):
        if self._show: self.grid_forget(); self._show = False
    def show(self):
        if not self._show: self.grid(**self._grid_args); self._show = True
    def switch(self): (self.hide if self._show else self.show)()
    def set_visible(self, mode):
        if mode: self.show()
        else: self.hide()
#-------------------------------------------------------------------------------
class aiwFrame(Frame, aiwShowHide):
    'обычный Frame но с возможностью скрытия/показа, пакуется на grid'
    def __init__(self, wroot, **grid_args):
        Frame.__init__(self, wroot)
        self.grid(**grid_args)
        self._show, self._grid_args = True, grid_args
#-------------------------------------------------------------------------------
class aiwScaleSH(aiwScale, aiwShowHide):
    def __init__(self, *args, **kw_args):
        self._show, self._grid_args = True, kw_args['grid']
        aiwScale.__init__(self, *args, **kw_args)
    def hide(self):
        if self._show: self.scale.grid_forget(); self._show = False
    def show(self):
        if not self._show: self.scale.grid(**self._grid_args); self._show = True
#-------------------------------------------------------------------------------
