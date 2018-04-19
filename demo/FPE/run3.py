#!/usr/bin/python
# -*- coding: utf-8 -*-

# импортируем необходимые модули
from math import *
from aiwlib.vec import *       # вектора
from aiwlib.iostream import *  # работа с файлами
from aiwlib.MeshF2 import *    # сетка Mesh<float,2>
from model import *            # модель
from aiwlib.racs import * #[1] подключаем RACS

#[2] создаем объект для взаимодействия с RACS
calc = Calc(fdump=0,           #@ частота сбросов функции распределения
            xv0=vec(0.,0.),    #@ начальные условия
            N=100000,          #@ число частиц
            f_sz=ind(100,100), #@ размер функции распределения
            f_min=vec(-2.,-1), #@ левая нижняя и
            f_max=vec(2.,1))   #@ правая верхняя границы функции распределения

# создаем класс модели и задаем параметры
M = calc.wrap(Model()) #[3] monkey-patch для управления моделью через RACS
M.a, M.b = -1, 1          #@ параметры потенциала
M.A, M.Omega = 0.1, 1.    #@ параметры внешней вынуждающей силы
M.gamma, M.T = 0.1, 0.2   #@ коэффициент затухания и температура
M.h = min(.1, .1/M.Omega) #@ автоматически устанавливаем шаг

# инициализируем функцию распределения
M.f.init(calc.f_sz, calc.f_min, calc.f_max)
M.f.bounds = 0x55 # режим обработки границ функции распределения

#[4] создаем файлы для записи результатов в уникальной директории расчета
if calc.fdump: fout = File(calc.path+'f.msh', 'w')
tvals = open(calc.path+'tvals.dat', 'w')
print>>tvals, '#:t Mx Mv Mxx Mvv Mxv W'
for i in 'Mx:x Mv:v Mxx:x^2 Mvv:v^2 Mxv:xv W:W'.split():
    print>>tvals, r'#:%s.tex=r"\left<%s\right>"'%tuple(i.split(':'))

# задаем и сохраняем начальные условия
M.init(calc.xv0, calc.N)
if calc.fdump: M.f.head = 't=0'; M.f.dump(fout)  
print>>tvals, M.t, M.av

nt, calc.chi = 0, 0
calc.t_min, calc.t_max = 10/M.gamma, 10/M.gamma+40*pi/M.Omega
while M.t<=calc.t_max: # цикл по времени
    M.calc(); nt += 1
    print>>tvals, M.t, M.av
    if calc.fdump and not nt%calc.fdump:
        M.f.head = 't=%g'%M.t; M.f.dump(fout)
    if M.t>=calc.t_min:
        calc.chi += M.av[1]*(cos(M.t*M.Omega)+sin(M.t*M.Omega)*1j)*M.h
    calc.set_progress(M.t/calc.t_max, 'calc') #[5] степень завершенности  
    
calc.chi *= 2/(M.A*(calc.t_max-calc.t_min))
