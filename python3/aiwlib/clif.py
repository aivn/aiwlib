# CLIF --- Command Line Interface Factorization
#
# Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com> 
# Licensed under the Apache License, Version 2.0

'''Преобразует последовательность строк (как правило аргументов командной строки). 

Задание переменной в текущем пространстве имен --- ИМЯ#=ЗНАЧЕНИЕ
Задание цикла --- переменная@ последовательность значений через пробел { тело цикла }
Задание макроса --- имя_макроса{ тело макроса }, команда  имя_макроса{ }   удаляет макрос
Вызов макроса --- имя_макроса#, макросы могут вызываться друг из друга, но зацикливание блокировано,
                  переменные трактуются как макросы из одного элемента
Подстановка переменных через format python3 производится однократно непосредственно перед вставкой, 
            например name#=value ...{name}...  будет преобразовано в ...value...
            при этом макросы трактуются как списки.
'''
#-------------------------------------------------------------------------------
def _ket_index(seq, pos):
    'ищет закрывающую скобку }'
    counter = 1
    while pos<len(seq):
        if seq[pos].endswith('{'): counter += 1
        if seq[pos]=='}': counter -= 1
        if not counter: return pos
        pos += 1
    raise Exception('ket } not found!')
#-------------------------------------------------------------------------------    
def CLIF(src_seq, scope={}, scope_upd={}):
    'CLIF(src_seq) ==> dst_seq'
    dst_seq, pos, scope = [],  0, dict(scope); scope.update(scope_upd)
    while pos<len(src_seq):
        el = src_seq[pos]; pos += 1
        if '#=' in el: scope.__setitem__(*el.split('#=', 1))
        elif el and el[-1]=='@':   # цикл
            key, bra = el[:-1], src_seq.index('{', pos)
            ket = _ket_index(src_seq, bra+1);  body = src_seq[bra+1: ket]
            while pos<bra:
                dst_seq += CLIF(body, scope, {key: src_seq[pos]})
                pos += 1
            pos = ket+1
        elif el.endswith('{'):  # создание нового макроса, пустой макрос убирается
            key, ket = el[:-1], _ket_index(src_seq, pos)
            if ket==pos and key in scope: del scope[key]
            else: scope[key] = src_seq[pos:ket]
            pos = ket+1
        elif el.endswith('#'):  # применение макроса, переменные трактуются как макрос из одного элемента
            key = el[:-1]; body = scope[key]
            dst_seq += CLIF(body if type(body) is list else [body], scope, {key: key})
        else: dst_seq.append(el.format(**scope))  # добавление очередного элемента, макросы трактуются как списки
    return dst_seq
CLIF.__doc__ += '\n'+__doc__
#-------------------------------------------------------------------------------    
if __name__ == '__main__':
    import sys
    print(CLIF(sys.argv[1:]))
#-------------------------------------------------------------------------------    
