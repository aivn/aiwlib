# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2020-21 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''
#-------------------------------------------------------------------------------
class Rect:
    def __init__(self, bmin, bmax):
        self.bmin, self.bmax = list(map(int, bmin)), list(map(int, bmax))
        self.bbox = [self.bmax[0]-self.bmin[0], self.bmax[1]-self.bmin[1]]
    def move(self, d): d = tuple(map(int, d)); self.bmin[0] += d[0]; self.bmax[0] += d[0]; self.bmin[1] += d[1]; self.bmax[1] += d[1]
    def resize(self, d): d = tuple(map(int, d)); self.bbox[0] += d[0]; self.bmax[0] += d[0]; self.bbox[1] += d[1]; self.bmax[1] += d[1]
    def check(self, pos): pass
#-------------------------------------------------------------------------------
