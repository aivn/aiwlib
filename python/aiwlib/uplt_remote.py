# -*- mode: Python; coding: utf-8 -*-
import os, struct

#-------------------------------------------------------------------------------
_connects = {}
def uplt_remote(host, command):
    if not host in _connects: _connects[host] = UpltConnect(host, command)
    return _connects[host]
#-------------------------------------------------------------------------------
def _send(cout, prefix, *args):
    s = ''.join([struct.pack('i', x) if type(x) is int else struct.pack('d', x) if type(x) is float else struct.pack('i%is'%len(x), len(x), x) for x in args])
    #print '>>>', repr(prefix+s)
    cout.write(prefix+s); cout.flush();
def _recv(cin, types): # i, s or f
    R = []
    for t in types:
        if t=='i': R.append(struct.unpack('i', cin.read(4))[0])
        elif t=='f': R.append(struct.unpack('d', cin.read(8))[0])
        else: R.append(cin.read(struct.unpack('i', cin.read(4))[0]))
    #print '<<<', types, R
    return R
#-------------------------------------------------------------------------------
class UpltConnect:
    def __init__(self, host, command): self.cout, self.cin = os.popen2('ssh %s %s'%(host, command)); self.sz = 0
    def open(self, fname):
        _send(self.cout, 'o', fname)
        count, L = _recv(self.cin, 'i')[0], []
        for i in range(count): L.append(_recv(self.cin, 'is'))
        if L: self.sz +=1
        return [UpltFrame(self, self.sz-1, i, dh[0], dh[1]) for i, dh in enumerate(L)]
#-------------------------------------------------------------------------------
class UpltFrame:
    def __init__(self, connect, dID, fID, dim, head):
        self.connect, self.dID, self.fID, self._dim, self.head = connect, dID, fID, dim, head
    def dim(self): return self._dim
    def get_conf(self, conf, firstcall=False):
        _send(self.connect.cout, 'c', self.dID, self.fID, conf.pack(), int(firstcall))
        conf.unpack(_recv(self.connect.cin, 's')[0])
    def f_min_max(self, conf): 
        _send(self.connect.cout, 'f', self.dID, self.fID, conf.pack())
        return _recv(self.connect.cin, 'ff')
    def get(self, conf, r):
        _send(self.connect.cout, 'g', self.dID, self.fID, conf.pack(), r[0], r[1])
        return _recv(self.connect.cin, 'f')[0]        
    def preview(self, conf, image, color): 
        _send(self.connect.cout, 'p', self.dID, self.fID, conf.pack(), color.pack(), image.size[0], image.size[1])
        image.load(self.connect.cin.read(image.size.prod()*3))
        print 'remote preview OK'
    def plot(self, conf, image, color):
        _send(self.connect.cout, 'P', self.dID, self.fID, conf.pack(), color.pack(), image.size[0], image.size[1])
        image.load(self.connect.cin.read(image.size.prod()*3))
        print 'remote plot OK'
#-------------------------------------------------------------------------------
