#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#from vec import *
import fplt
def list2intarr(args):
    ia=fplt.int_array(len(args));
    for i, v in enumerate(args):
        ia[i] =v
    return ia
class Volume:
    def __init__(self, Input):
        "Инициализирует данные с помощью потока mystream"
        self.Inp = Input
        self.Surf = fplt.Mplt()
        self.load()
        self.frames=[0,]
        self.current_frame = 0
        self.item = 0
        self.cb_auto = True
    def load(self):
        "Загружает следующий кадр, возвращает true если удачно"
        check = self.Surf.load(self.Inp)
        if check:
            #self.app_names = dict( [ (name, i ) for i, name in  enumerate(self.Surf.appends_names.split())])
            #self.items_to_names = dict( [ (i, name ) for i, name in  enumerate(self.Surf.appends_names.split())])
            self.load_on_device()
        return check
    def next(self):
        ''' Переходит к следующему фрейму'''
        pos = self.Inp.tell()
        #print pos
        if self.load():
            self.current_frame += 1
            #print self.current_frame
            if (self.current_frame == len(self.frames)): self.frames.append(pos)
            self.load_on_device()
        else:
            self.set_frame(0)
    def set_frame(self,pos):
        "Переходит к фрейму pos, если он уже был подгружен"
        if pos>= len(self.frames) or pos < -len(self.frames): return
        if pos<0: pos += len(self.frames)
        self.Inp.seek(self.frames[pos])
        self.load()
        self.current_frame = pos
        self.load_on_device()
    def jump(self, pos):
        "Перепрыгивает на pos фреймов вперед, если цель была загружена"
        self.set_frame(self.current_frame + pos)
    def load_on_device(self):
        "Загружает данные на видеокарту"
        self.Surf.load_on_device()
    def get_cb_auto(self):
        "Возвращает значение cb_auto, т.е. проводится ли автошкалирование цвета"
        return self.cb_auto
    def set_cb_auto(self, tf):
        "Устанавливается значение cb_auto, True если проводится автошкалирование цвета"
        self.cb_auto = tf
    def get_frame(self):
        "Возвращает номер фрейма"
        return self.current_frame
    def set_raystep(self, rs):
        self.Surf.set_raystep(rs)
    def set_density(self, dens):
        self.Surf.set_density(dens)
    def set_brightness(self, br):
        self.Surf.set_brightness(br)
    def set_opacity(self, op):
        self.Surf.set_opacity(op)
    def get_raystep(self):
        return self.Surf.get_raystep()
    def get_density(self):
        return self.Surf.get_density()
    def get_brightness(self):
        return self.Surf.get_brightness()
    def get_opacity(self):
        return self.Surf.get_opacity()
    def set_cbrange(self, lower, upper):
        "Устанавливает диапазон цвета, отключает автошкалирование цвета"
        self.cb_auto = False
        self.Surf.set_range(lower, upper)
    def set_cbrange__(self, lower, upper):
        "Устанавливает диапазон значений цвета, не отключая автошкалирование"
        #self.auto_cb = False
        return self.Surf.set_range(lower, upper)
    def get_cbrange(self):
        "Устанавливает диапазон значений цвета, отключает автошкалирование"
        return (self.Surf.min(), self.Surf.max())
    def autoscalecb(self):
        "Автоматически выбирает диапазон цвета, включает автошкалирование цвета"
        self.cb_auto = True
        self.Surf.autoset_minmax()
    def rangemove(self,rel_val, inout):
        '''Перемещает границу цветогого диапазона на значение в доле от текущего диапазона,
        Если inout False, то граница левая иначе — правая. Автошкалирование цвета при этом отключается'''
        self.cb_auto = False
        self.Surf.rangemove(rel_val,inout)
    def extendrange(self, rel_val):
        '''Расширяет цветогой диапазон на значение в доле от текущего диапазона.
        Автошкалирование цвета при этом отключается'''
        self.cb_auto =False
        self.Surf.extendrange(rel_val)
    def autobox(self):
        "Возврашает размеры коробки для данных"
        bbmin,bbmax = fplt.vecf(0,0,0), fplt.vecf(0,0,0)
        self.Surf.get_auto_box(bbmin, bbmax)
        return bbmin, bbmax
    def adjust_cube(self, V):
        self.Surf.adjust_cube(V)
    def display(self,V,spr,tex):
        "Отображает данные, служебная функция"
        #if self.cb_auto:
        #    self.autoscalecb()
        #spr.render(self.Surf, V, tex)
        spr.start()
        tex.use_texture(spr, "pal")
        V.plot(spr)
        self.Surf.plot(spr)
        spr.stop()

def ext_generator(name,*args):
    "Создает внешние методы на основе текущего класса, такое наследование"
    def ext_func(self,*args):
        return getattr(self.Surf, name)(*args)
    ext_func.__name__ = name
    ext_func.__doc__=getattr(Volume, name).__doc__
    return ext_func

