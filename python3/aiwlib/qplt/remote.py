# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2020-21 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

__all__ = ['factory', 'Connect']
import os, sys, struct, paramiko, atexit

#-------------------------------------------------------------------------------
connect_table, command = {}, 'aiwlib/bin/qplt-remote'

def factory(fname, host, **params):
    if not host in connect_table: connect_table[(host, params.get('port'))] = Connect(host, **params)
    return connect_table[(host, params.get('port'))].load_frames(fname)
#-------------------------------------------------------------------------------
class Connect:
    def __init__(self, host, **params):
        if 'user' in params: params['username'] = params.pop('user')
        self.client, self.host = paramiko.SSHClient(), host
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.client.connect(hostname=host, **params) # password=secret
        self.cout, self.cin, self.cerr = self.client.exec_command(command)
        atexit.register(self.close)
        #print(self.cerr.readlines())
    def load_frames(self, fname):
        self.send('o', fname); N, res = self.recv('i')[0], []
        for i in range(N):
            szs, L = self.recv('ii'), []
            for i in range(szs[1]): L.append(QpltContainer(szs[0], i, self))
            if L: res.append(L)
        return res
    def close(self): self.cout.write('E'); self.cout.flush(); self.client.close() #; print('BYE')
    #---------------------------------------------------------------------------
    #   base protocol
    #---------------------------------------------------------------------------
    def send(self, prefix, *args):
        #print('>>>', prefix, args)
        s = b''.join([struct.pack('i', x) if type(x) in (int, bool) else struct.pack('f', x) if type(x) is float else
                      struct.pack('i', len(x))+(bytes(x, 'utf8') if type(x) is str else x) if type(x) in (str, bytes)
                      else struct.pack('i'*len(x), *x)  if type(x[0]) is int else struct.pack('f'*len(x), *x) for x in args])
        #print('>>>', bytes(prefix, 'utf8')+s)
        self.cout.write(bytes(prefix, 'utf8')+s); self.cout.flush()
    def recv(self, types): # i, f, s or Xi, Xf for arrays
        R, sz = [], None #; print('<<<', types)
        for t in types:
            if sz: R.append(struct.unpack(t*sz, self.cin.read(4*sz))); sz = None
            elif t=='i': R.append(struct.unpack('i', self.cin.read(4))[0])
            elif t=='f': R.append(struct.unpack('f', self.cin.read(4))[0])
            elif t=='s': R.append(self.cin.read(struct.unpack('i', self.cin.read(4))[0]))
            else: sz = int(t)
        #print('<<<', R)
        return R
#-------------------------------------------------------------------------------
#   remote container
#-------------------------------------------------------------------------------
class QpltContainer:
    #static double mem_limit;  // лимит на размер памяти, в GB
    def fname(self): return self._fname
    def frame(self): return self._frameID 		
    def get_dim(self): return self._dim
    def get_szT(self): return self._szT
    def get_head(self): return self._head
    def get_info(self): return self._info
    def get_bbox(self, axe): return self._bbox[axe]
    def get_bmin(self, axe): return self._bmin[axe]
    def get_bmax(self, axe): return self._bmax[axe]
    def get_logscale(self, axe): return self._logscale&(1<<axe)
    def get_axe(self, i): return self._anames[i] 
    def get_step(self, axe): return self._step[axe]
    #float fpos2coord(float fpos, int axe) const;
    #float pos2coord(int pos, int axe) const;
    #int coord2pos(float coord, int axe) const;
    def __init__(self, fileID, frameID, connect):
        self._fileID, self._frameID, self._connect = fileID, frameID, connect
        self._fname, self._dim, self._szT, self._head, self._info, self._bbox, self._bmin, self._bmax, self._logscale, self._step = connect.recv('siiss6i6f6fi6f')
        self._anames = connect.recv('s'*self._dim); self._fname = bytes(connect.host+':', 'utf8')+self._fname
    def plotter(self, mode, f_opt, f_lim,  paletter, arr_lw, arr_spacing,  nan_color, ctype, Din, mask, offset, diff, vconv, minus,  
		axisID, sposf, bmin, bmax, faai, th_phi, cell_aspect, D3scale_mode):
        self._connect.send('p', self._fileID, self._frameID, mode,
                           f_opt, f_lim,  paletter, arr_lw, arr_spacing,  nan_color, ctype, Din, mask, offset, diff, vconv, minus,  
	                   axisID, sposf, bmin, bmax, faai, th_phi, cell_aspect, D3scale_mode)
        return QpltPlotter(self, self._connect, axisID)
#-------------------------------------------------------------------------------
class QpltPlotter: 
    def free(self): self._connect.send('q', self._ID)
    def get_dim(self): return self._dim
    def get_bbox(self, axe): return self._bbox[axe]
    def get_bmin(self, axe): return self._bmin[axe]
    def get_bmax(self, axe): return self._bmax[axe]
    def get_logscale(self, axe): return self.container.get_logscale(self._axisID[axe])
    def get_step(self, i): return self.container.get_step(self._axisID[i])
    def get_axe(self, i): return self.container.get_axe(self._axisID[axe])
    def get_axeID(self, axe): return self._axisID[axe]		
    def get_f_min(self): return self._f_lim[0]
    def get_f_max(self): return self._f_lim[1]
    def flats_sz(self): return len(self._flats)
    def get_flat(self, i): return self._flats[i]
    def get(self, xy): pass
    def __init__(self, container, connect, axisID):
        self.container, self._connect, self._axisID, self._flats = container, connect, tuple(axisID), []
        self._ID, self._dim, self._bbox, self._bmin, self._bmax, self._f_lim, fsz  = connect.recv('ii3i3f3f2fi')
        for i in range(fsz): self._flats.append(QpltFlat(connect))
    def set_image_size(self, xy1, xy2):
        self._connect.send('s', self._ID, xy1, xy2)
        self.center, self.ibmin, self.ibmax = self._connect.recv('2i2i2i')
        for f in self._flats: f.a, f.b, f.c, f.d = self._connect.recv('2i2i2i2i')
    def plot(self):
        self._connect.send('P', self._ID)
        return self._connect.cin.read(4*(self.ibmax[0]-self.ibmin[0])*(self.ibmax[1]-self.ibmin[1]))
#-------------------------------------------------------------------------------
class QpltFlat:
    def __init__(self, connect): self.axis, self.bounds, self.bmin, self.bmax = connect.recv('2ii2f2f')
#-------------------------------------------------------------------------------

