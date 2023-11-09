# CLIF --- Command Line Interface Factorization
#
# Copyright (C) 2023 Antov V. Ivanov  <aiv.racs@gmail.com> 
# Licensed under the Apache License, Version 2.0

'''Преобразует последовательность строк (как правило аргументов командной строки). 

Задание переменной в текущем пространстве имен --- ИМЯ#=ЗНАЧЕНИЕ
Задание цикла --- переменная@ последовательность значений через пробел { тело цикла }
Задание макроса --- имя_макроса{ тело макроса }, команда  имя_макроса{ }   удаляет макрос/переменную
Вызов макроса --- имя_макроса#, макросы могут вызываться друг из друга но зацикливание блокировано,
                  переменные трактуются как макросы из одного элемента
Переменные, значения счетчиков циклов и элементы тела макроса по возможности преобразуются к int/float.
Подстановка переменных в каждый элемент последовательности производится однократно непосредственно 
            перед вставкой при помощи метода str.format по следующим правилам:
            1) если форматирование привело к исключению KeyError, производится попытка вычислить
               содержимое {...} при помощи eval(..., math.__dict__, текущее_пространство_имен)
            2) если вычисление не удалось, фрагмент {...} остается без изменений  
При подстановке макросы трактуются как списки.  

Например name#=value ...{name}...  ==> ...value..., a#=2 b#=c {a*b} ==> cc          
'''

# позиционные арагументы для format --- значения последнего цикла? номер итерации через _<key>?
# сообщения об ошибках?
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
def _cast(arg):
    try: return int(arg)
    except: pass
    try: return float(arg)
    except: return arg
#-------------------------------------------------------------------------------    
import math;  _math_dict = dict(math.__dict__)
#-------------------------------------------------------------------------------    
def CLIF(src_seq, scope={}, scope_upd={}):
    'CLIF(src_seq) ==> dst_seq'
    dst_seq, pos, scope = [],  0, dict(scope); scope.update(scope_upd)
    while pos<len(src_seq):
        el = src_seq[pos]; pos += 1
        if '#=' in el: k, v = el.split('#=', 1); scope[k] = _cast(v)
        elif el and el[-1]=='@':   # цикл
            key, bra = el[:-1], src_seq.index('{', pos)
            ket = _ket_index(src_seq, bra+1);  body = src_seq[bra+1: ket]
            while pos<bra:
                dst_seq += CLIF(body, scope, {key: _cast(src_seq[pos])})
                pos += 1
            pos = ket+1
        elif el.endswith('{'):  # создание нового макроса, пустой макрос убирается
            key, ket = el[:-1], _ket_index(src_seq, pos)
            if ket==pos and key in scope: del scope[key]
            else: scope[key] = [_cast(x) for x in src_seq[pos:ket]]
            pos = ket+1
        elif el.endswith('#'):  # применение макроса, переменные трактуются как макрос из одного элемента
            key = el[:-1]; body = scope[key]
            dst_seq += CLIF(body if type(body) is list else [body], scope, {key: key})
        else:  # добавление очередного элемента, макросы трактуются как списки
            #if el.startswith('@'): dst_seq.append(eval('f%r'%el[1:], _math_dict, _UniversalScope(scope))); continue
            tmp_scope = dict(scope)
            while 1:  # игнорируем некорректные подстановки путем замены на самих себя, убрать этот режим?
                try: dst_seq.append(el.format(**tmp_scope)); break
                except ValueError: dst_seq.append(el); break
                except KeyError as ex:
                    key = ex.args[0]
                    try: tmp_scope[key] = eval(key, _math_dict, scope)  # или tmp_scope?
                    except: tmp_scope[key] = '{%s}'%key
    return dst_seq
CLIF.__doc__ += '\n'+__doc__
#-------------------------------------------------------------------------------    
if __name__ == '__main__':
    import sys
    print(CLIF(sys.argv[1:]))
#-------------------------------------------------------------------------------    
