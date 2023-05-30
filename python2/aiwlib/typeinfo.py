# -*- coding: utf-8 -*-
'''Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com> with support Kintech Lab
Licensed under the Apache License, Version 2.0'''

import struct
#-----------------------------------------------------------------------------
class Field:
    types = { 'float':(0,4), 'double':(1,8), 'bool':(3,1), 'char':(3,1), 'short':(6,2), 'int':(8,4), 'long':(10,8),
              'uint8_t':(3,1), 'int8_t':(4,1), 'uint16_t':(5,2), 'int16_t':(6,2), 'uint32_t':(7,4), 'int32_t':(8,4), 'uint64_t':(9,8), 'int64_t':(10,8) }
    def __init__(self, offset, type_, dim=(), comment=''):
        if not type(offset) is int: raise Exception('offset=%r --- integer offset expected'%offset)
        if not type_ in self.types: raise Exception('type_=%r --- mast be in %s'%(type_, self.types))
        if not type(dim) in (list, tuple) or any(not type(d) is int or d<=0 for d in dims):
            raise Excetion('dim=%r --- mast be list or tuple of positive integers'%dim)
        self.offset, self.type_, self.dim, self.comment = offset, type_, (tuple(dim) if dim else (1,)), str(comment).replace('\n', ' ')
    def dump(self, name):
        res = '%s %i %s'%(name, self.offset, self.type_)
        if self.dim: res += ' '+'x'.join(map(str, self.dim))
        if self.comment: res += ' #%s'%self.comment
        return res
    def load(line): #--> return name, field
        line = line.split()
        if len(line)<3: raise Exception('line=%s --- minimum 3 values expected'%(line,))
        try: dim = list(map(int, line[3].split('x'))); line.pop(3)
        except: dim = ()
        return line[0], Field(int(line[1]), line[2], dim, ' '.join(line[3:])[1:])
#-----------------------------------------------------------------------------
_is_name = lambda x: x and (x[0].isalpha() or x[0]=='_') and x.replace('_', '').isalnum()
class Struct:
    def __init__(self, _sizeof, **fields):
        self.sizeof, self.fields = int(_sizeof), {}
        for k, v in fields.items(): 
            if not _is_name(k): raise Exception('incorrect struct field name %r'%k)
            self.fields[k] = v if isinstance(v, Field) else Field(*v)
    def dump(self, name): return '\n'.join((['@%s %i'%(name, self.sizeof)] if name else [])+[v.dump(k) for k, v in self.fileds.items()])
    def load(lines): #--> return (name, struct) and pop self code from lines
        if lines[0][0]=='@': name, sizeof = lines.pop(0).split('#')[0].split(); self = Struct(int(sizeof))
        else: name, self = '', Struct(0)
        while lines and lines[0]!='@':
            name, field = Field.load(lines.pop(0))
            if not _is_name(name): raise Exception('incorrect struct field name %r'%name)
            self.table[name] = field
        return self
#-----------------------------------------------------------------------------
class Cell:
    def __init__(_self, **rootfields): _self.table = {}; _self.add_struct('', 0, **rootfields)
    def add_struct(_self, _name, _sizeof, **fields):
        if _name and not _is_name(_name): raise Exception('incorrect struct name %r'%_name)
        self.table[_name] = Struct(_sizeof, **fields)
    def dump(self):  return '\n'.join(v.dump(k) for k, v in self.table.items())
    def load(line):
        cell, lines = Cell(), list(filter(lambda x: x.strip() and x.strip()[0]!='#', line.split('\n')))
        while lines:
            name, struct = Struct.load(lines)
            if not _is_name(name): raise Exception('incorrect struct name %r'%name)
            cell.table[name] = struct
        return cell
    def namespace(self): return [(k, Cursor(self.table, k)) for k in self.table[''].fields]        
#-----------------------------------------------------------------------------
class Cursor:
    def __init__(self, table, field_name=None, struct_name='', offset0=0):
        if not field_name: self.table, self.offset, self.type_, self.dim, self.sizeof = table.table, table.offset, table.type_, table.dim, table.sizeof
        else:
            self.table, field = table, table[struct_name].fields[field_name]
            self.offset, self.type_, self.dim = offset0+field.offset, field.type_, field.dim
            self.sizeof = Field.types[field.type_][1] if field.type_ in Field.types else self.table[field.type_].sizeof
    def get_attr(self, attr): return Cursor(self.table, attr, self.type_, self.offset)
    def get_item(self, index):
        cursor, mul = Cursor(self), self.sizeof
        if not type(index) in (int, list, tuple): raise Exception('incorrect index %r --- int, list or tuple expected'%(index,))
        if type(index) is int: index = (index,)
        if type(index) in (list, tuple) and len(self.dim)!=len(index): raise Exception('incorrect index %s --- %i values expected'%(index, len(self.dim)))
        if not all(0<=i<j for i, j in zip(index, self.dim)): raise Exception('index %s out of range %s'%(index, self.dim))
        for i, s in reversed(zip(index, cursor.dim)): cursor.offset += mul*i; mul *= s
        return cursor
    def bytecode(self):
        if self.dim or not self.type_ in Field.types: raise Exception('can not make bytecode for nonscalar object --- type=%s dim=%s'%(self.type_, self.dim))
        return chr(Field.types[self.type_][0]).encode()+struct.pack('i', self.offset)        
#-----------------------------------------------------------------------------
