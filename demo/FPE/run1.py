#!/usr/bin/python
# -*- coding: utf-8 -*-

# импортируем необходимые модули
from model import *            # модель
from aiwlib.vec import *       # вектора
from aiwlib.iostream import *  # работа с файлами
from aiwlib.MeshF2 import *    # сетка Mesh<float,2>

# создаем класс модели и задаем параметры
M = Model()
M.a, M.b = -1, 1
M.A, M.Omega = 0., 0.
M.gamma, M.T = 0.1, 0.05
M.h = 0.1

# инициализируем функцию распределения
M.f.init(ind(100,100), vec(-2.,-1.), vec(2.,1.))
M.f.bounds = 0x55 # режим обработки границ функции распределения

# создаем файлы для записи результатов
fout = File('f1.msh', 'w')
tvals = open('tvals1.dat', 'w')
print>>tvals, '#:t Mx Mv Mxx Mvv Mxv W'
for i in 'Mx:x Mv:v Mxx:x^2 Mvv:v^2 Mxv:xv W:W'.split():
    print>>tvals, r'#:%s.tex=r"\left<%s\right>"'%tuple(i.split(':'))

# задаем и сохраняем начальные условия
M.init(vec(0.,0.), 100000)
M.f.head = 't=0'; M.f.dump(fout) 
print>>tvals, M.t, M.av

while M.t<50: # цикл по времени
    M.calc()
    print>>tvals, M.t, M.av
    M.f.head = 't=%g'%M.t; M.f.dump(fout)
