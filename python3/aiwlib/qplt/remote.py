# -*- mode: Python; coding: utf-8 -*-
'''Copyright (C) 2020-22 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

__all__ = ['factory', 'Connect']
import os, sys, struct, paramiko, atexit, math

#-------------------------------------------------------------------------------
connect_table, keys_table, command, mem_limit = {}, {}, 'bin/qplt-remote', 0

#---   read config file   ------------------------------------------------------
try: config, kmode = open(os.path.expanduser('~/.qplt')), False
except: config = []
for l in config:
    l = l.strip()
    if not l or l[0]=='#': continue
    if l[-1]==':': kmode = l=='KEYS:'; continue
    if not kmode: continue
    l = l.split()
    for k in l[:-1]:
        a, h = k.split('=', 1) if '=' in k else (k, k)
        u, h = h.split('@', 1) if '@' in h else (None, h)
        h, p = h.split('/', 1) if '/' in h else (h, None)
        keys_table[a] = (u, h, p, l[-1])
#-------------------------------------------------------------------------------
sshconfig = paramiko.SSHConfig()
try: sshconfig.parse(open(os.path.expanduser('~/.ssh/config')))  # set path in .qplt???
except: print('File ~/.ssh/config not loaded')
#-------------------------------------------------------------------------------
def factory(fname, host, **params):
    if not host in connect_table: connect_table[(host, params.get('port'))] = Connect(host, **params)
    return connect_table[(host, params.get('port'))].load_frames(fname)
#-------------------------------------------------------------------------------
class Connect:
    def __init__(self, host, **params):
        conf = sshconfig.lookup(host)
        self.host, self.client, host = host, paramiko.SSHClient(), conf['hostname']
        self.client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        if host in keys_table:
            user, host, port, key = keys_table[host]; params['pkey'] = paramiko.RSAKey.from_private_key_file(key)
            if user and not 'user' in params: params['user'] = user
            if port and not 'port' in params: params['port'] = int(port)
        if not 'port' in params and 'port' in conf: params['port'] = int(conf['port'])
        if not 'user' in params and 'user' in conf: params['user'] = conf['user']
        if 'user' in params: params['username'] = params.pop('user')
        self.client.connect(hostname=host, compress=True, **params) # password=secret
        self.cout, self.cin, self.cerr = self.client.exec_command(command)
        atexit.register(self.close)
        if mem_limit>0: self.send('m', float(mem_limit))
        #print(self.cerr.readlines())
    def load_frames(self, fname):
        self.send('o', fname); N, res = self.recv('i')[0], []
        for i in range(N):
            L = _load_frames_from_single_file(self)
            if L: res.append(L)
        return res
    def close(self):
        self.cout.write('E'); self.cout.flush(); self.client.close() #; print('BYE')
        print('FINAL REPORT %r:\n'%self.host, ' '.join(self.cerr.readlines()))
    #---------------------------------------------------------------------------
    #   base protocol
    #---------------------------------------------------------------------------
    def send(self, prefix, *args):
        # print('>>>', prefix, args)
        s = b''.join([struct.pack('i', x) if type(x) in (int, bool) else struct.pack('f', x) if type(x) is float else
                      struct.pack('i', len(x))+(bytes(x, 'utf8') if type(x) is str else x) if type(x) in (str, bytes)
                      else struct.pack('i'*len(x), *x)  if type(x[0]) is int else struct.pack('f'*len(x), *x) for x in args])
        ##print('>>>', bytes(prefix, 'utf8')+s)
        self.cout.write(bytes(prefix, 'utf8')+s); self.cout.flush()
    def recv(self, types): # i, f, s or Xi, Xf for arrays
        try:
            R, sz = [], None #; print('<<<', types)
            for t in types:
                if sz: R.append(struct.unpack(t*sz, self.cin.read(4*sz))); sz = None
                elif t=='i': R.append(struct.unpack('i', self.cin.read(4))[0])
                elif t=='f': R.append(struct.unpack('f', self.cin.read(4))[0])
                elif t=='s': R.append(self.cin.read(struct.unpack('i', self.cin.read(4))[0]))
                else: sz = int(t)
            # print('<<<', R)
            return R
        except: print('RECV %s FAILED:\n'%self.host, ''.join(self.cerr.readlines()), R); raise
#-------------------------------------------------------------------------------
def _load_frames_from_single_file(connect, frameID0=0):
    szs, L = connect.recv('ii'), []
    for i in range(szs[1]): L.append(QpltContainer(szs[0], frameID0+i, connect))
    return L
#-------------------------------------------------------------------------------
#   remote container
#-------------------------------------------------------------------------------
class QpltContainer:
    #static double mem_limit;  // лимит на размер памяти, в GB
    def features(self): return self._features
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
    def pos2coord(self, pos, axe): return  self._bmin[axe]*self._step[axe]**(pos+.5) if self._logscale&1<<axe else self._bmin[axe]+self._step[axe]*(pos+.5)
    def coord2pos(self, coord, axe): return int(math.log(coord/self._bmin[axe])*self._rstep[axe] if self._logscale&1<<axe else (coord-self._bmin[axe])*self._rstep[axe])
    def __init__(self, fileID, frameID, connect):
        self._fileID, self._frameID, self._connect = fileID, frameID, connect
        self._fname, self._dim, self._szT, self._head, self._info, self._bbox, self._bmin, self._bmax, self._logscale, self._step, self._rstep, self._features = \
            connect.recv('siiss6i6f6fi6f6fi')
        self._anames = connect.recv('s'*self._dim); self._fname = bytes(connect.host+':', 'utf8')+self._fname
    def plotter(self, mode, f_opt, f_lim,  paletter, arr_lw, arr_spacing,  nan_color, ctype, Din, mask, offset, diff, vconv, minus,  
		axisID, sposf, bmin, bmax, faai, th_phi, cell_aspect, D3deep):
        self._connect.send('p', self._fileID, self._frameID, mode,
                           f_opt, f_lim,  paletter, arr_lw, arr_spacing,  nan_color, ctype, Din, mask, offset, diff, vconv, minus,  
	                   axisID, sposf, bmin, bmax, faai, th_phi, cell_aspect, D3deep)
        return QpltPlotter(self, self._connect, axisID)
    def check_change_file(self): self._connect.send('t', self._fileID, self._frameID); return self._connect.recv('i')[0]
    def load_next_frames(self):  self._connect.send('n', self._fileID, self._frameID); return _load_frames_from_single_file(self._connect, self._frameID+1)        
    def reload_all_frames(self): self._connect.send('r', self._fileID, self._frameID); return _load_frames_from_single_file(self._connect)
    def free_self(self): pass
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
    def __init__(self, container, connect, axisID):
        self.container, self._connect, self._axisID, self._flats = container, connect, tuple(axisID), []
        self._ID, self._dim, self._bbox, self._bmin, self._bmax, self._f_lim, fsz  = connect.recv('ii3i3f3f2fi')
        for i in range(fsz): self._flats.append(QpltFlat(connect))
    def set_image_size(self, xy1, xy2):
        self._connect.send('s', self._ID, xy1, xy2)
        self.center, self.ibmin, self.ibmax = self._connect.recv('2i2i2i')
        for f in self._flats: f.a, f.b, f.c, f.d, f.nX, f.nY = self._connect.recv('2i2i2i2i2f2f')
    def plot(self):
        self._connect.send('P', self._ID)
        return self._connect.cin.read(4*(self.ibmax[0]-self.ibmin[0])*(self.ibmax[1]-self.ibmin[1]))
    def get(self, xy): self._connect.send('g', self._ID, xy); return self._connect.recv('s')[0]
#-------------------------------------------------------------------------------
class QpltFlat:
    def __init__(self, connect):  self.axis, self.bounds, self.bmin, self.bmax = connect.recv('2ii2f2f')
#-------------------------------------------------------------------------------

