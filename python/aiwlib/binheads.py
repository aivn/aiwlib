# -*- mode: Python; coding: utf-8 -*-
'''Read and parse all headers for aiwlib binary formats.

Copyright (C) 2017  Antov V. Ivanov  <aiv.racs@gmail.com>
Licensed under the Apache License, Version 2.0'''

#rename to load? containers? formats?

import struct, gzip, importlib, aiwlib.iostream
#-------------------------------------------------------------------------------
class Mesh:
    def __init__(self, fin, offset, head, D, szT):
        self.fname, self.offset, self.D, self.szT, self.logscale = fin.name, offset, D, szT, 0
        self.bbox = struct.unpack('i'*D, fin.read(4*D))
        self.data, self.size = fin.tell(), reduce(long.__mul__, self.bbox, 1L); self.data_sz = self.size*szT
        fin.seek(self.data_sz, 1); p_end = fin.tell()
        try:
            hsz = struct.unpack('i', fin.read(4))[0]
            if hsz==-(4+szT+24*D):
                self.bmin, self.bmax = [struct.unpack('d'*D, fin.read(8*D)) for i in (0,1)]
                fin.seek(D*8+szT, 1)
        except struct.error, e: fin.seek(p_end)
        self.head, tail_sz = head.split('\0')[0], 4+D*16+szT
        if (not hasattr(self, 'bmin') and len(head)>len(self.head)+tail_sz and 
            head[len(self.head):-tail_sz]=='\0'*(len(head)-len(self.head)-tail_sz)):
            self.bmin = struct.unpack('d'*D, head[-4-16*D:-4-8*D])
            self.bmax = struct.unpack('d'*D, head[-4-8*D:-4])
            self.logscale = struct.unpack('i', head[-4:])[0]
    def load(self, fstream=None):
        if self.szT not in (4,8):
            raise Exception('Uncknow mesh type: fname=%(fname)r, offset=%(offset)i, head=%(head)r, szT=%(szT)i, D=%(D)i, bbox=%(bbox)s'%self.__dict__) 
        name = 'Mesh%s%i'%('FD'[self.szT/4-1], self.D)
        mesh = getattr(importlib.import_module('aiwlib.'+name), name)()
        if not fstream: fstream = aiwlib.iostream.File(self.fname, 'r') # GzFile???
        fstream.seek(self.offset); mesh.load(fstream)
        return mesh        
#-------------------------------------------------------------------------------
class Sphere:
    def __init__(self, fin, offset, head, D, szT):
        self.fname, self.offset, self.head, self.szT, self.R = fin.name, offset, head, szT, struct.unpack('i', fin.read(4))[0] 
        self.data, self.size = fin.tell(), 60L*4**self.R; self.data_sz = self.size*szT
        fin.seek(self.data_sz, 1)
        if fin.tell()!=self.data+self.data_sz: raise Exception('incorrect file length: end=%s, but %s expected'%(fin.tell(), self.data+self.data_sz))
    def load(self):
        if self.szT not in (4,8):
            raise Exception('Uncknow sphere type: fname=%(fname)r, offset=%(offset)i, head=%(head)r, szT=%(szT)i, R=%(R)i'%self.__dict__) 
        name = 'Sphere%s'%'FD'[self.szT/4-1]
        sphere = getattr(importlib.import_module('aiwlib.'+name), name)()
        if not fstream: fstream = aiwlib.iostream.File(self.fname, 'r') # GzFile???
        fstream.seek(self.offset); sphere.load(fstream)
        return sphere        
#-------------------------------------------------------------------------------
def read_frame(fin):
    'Читает один  кадр из файлового потока fin, возвращает заголовок (либо None),  переводит позицию в файле к следующему кадру' 
    try:
        offset = fin.tell()
        hsz = struct.unpack('i', fin.read(4))[0]; head, D, szT = struct.unpack('=%isii'%hsz, fin.read(hsz+8))
        if D>0: return Mesh(fin, offset, head, D, szT)
        elif D==0: return Sphere(fin, offset, head, szT)
    except struct.error, e: return None
#-------------------------------------------------------------------------------
def read_stream(fstream):
    'Читает все кадры из fstream (строка с именем файла либо поток), возвращает список кадров'
    if type(fstream) is str: fstream = (gzip.open if fstream.endswith('.gz') else open)(fstream)
    frames, fr = [], read_frame(fstream)
    while fr: frames.append(fr); fr = read_frame(fstream)
    return frames
#-------------------------------------------------------------------------------
