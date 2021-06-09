# Copyright (C) 2021 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
# Licensed under the Apache License, Version 2.0

from PyQt5 import QtWidgets, QtGui, QtCore
#-------------------------------------------------------------------------------
#   frontend
#-------------------------------------------------------------------------------
def plot_x_axe(plt, xcoords, ycoord, xlims, tics_length): pass
#-------------------------------------------------------------------------------
def plot_y_axe(plt, ycoords, xcoord, ylims, tics_length, rigth_align): pass
#-------------------------------------------------------------------------------
#  backend
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------


class Canvas:
    # каждый раз класс создается заново (императивно) или обновляется то что было создано (декларативно)?
    def __init__(self, w, h): pass
    def save(self, path): pass

    def plot_line(): pass
    def plot_text(): pass

    def plot_axe(): pass
    def plot_image(): pass
