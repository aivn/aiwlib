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

# позиционные арагументы для format --- значения последнего цикла? 
# сообщения об ошибках?
#-------------------------------------------------------------------------------
def _cast(arg):
    #try: return eval(arg, {'__builtlins__': None})
    try: return int(arg)
    except: pass
    try: return float(arg)
    except: return arg
#-------------------------------------------------------------------------------
def clif(src_seq, scope={}, scope_upd={}):  # src_sec ==> dst_sec
    def ket_index(pos0): # ищет закрывающую скобку }
        counter = 1
        while pos0<len(src_seq):
            if src_seq[pos0].endswith('{'): counter += 1
            elif src_seq[pos0]=='}': counter -= 1
            if not counter: return pos0
            pos0 += 1
        raise Exception('Ket } not found when processing incoming %s sequence'%src_seq)
    dst_seq, pos, scope = [], 0, dict(scope); scope.update(scope_upd)
    while pos<len(src_seq):        
        el = src_seq[pos]; pos += 1
        if '#=' in el and el.split('#=')[0].isidentifier(): k, v = el.split('#=', 1); scope[k] = _cast(v)
        elif ':=' in el and el.split(':=')[0].isidentifier(): k, v = el.split(':=', 1); scope[k] = _cast(v); dst_seq.append(v)
        elif el and el[-1]=='#' and el[:-1].isidentifier():   # цикл
            key, bra = el[:-1], src_seq.index('{', pos);  ket = ket_index(bra+1);  body = src_seq[bra+1: ket]
            for ival, val in enumerate(clif(src_seq[pos:bra], scope)): dst_seq += clif(body, scope, {key: _cast(val), '_'+key: ival})
            pos = ket+1
        elif el.endswith('{') and el[:-1].isidentifier():  # создание нового макроса, пустой макрос убирается
            key, ket = el[:-1], ket_index(pos)
            if ket==pos and key in scope: del scope[key]
            else: scope[key] = [_cast(x) for x in src_seq[pos:ket]]
            pos = ket+1
        elif el.endswith('{}') and el[:-2] in scope: del scope[el[:-2]]  # удаление макроса или переменной
        elif el and el[0]=='{' and el[-1]=='}' and type(scope.get(el[1:-1].split('[')[0])) is list and (
                not '[' in el or (el[-2]==']' and all(x in '0123456789-:' for x in el.split('[', 1)[1][:-2]) and el.count(':')<=2)):  # подстановка макроса
            key, abd = el[1:-2].split('[') if '[' in el else (el[1:-1], None);  body = scope[key]
            if abd: body = eval('body[%s]'%abd, {'body': body})
            dst_seq += clif(body if type(body) is list else [body], scope, {key: key})
        else:  # добавление очередного элемента, макросы трактуются как списки
            tmp_scope = dict(scope)
            while 1:  # игнорируем некорректные подстановки путем замены на самих себя
                try: dst_seq.append(el.format(**tmp_scope)); break
                except KeyError as ex:
                    key = ex.args[0]
                    try: tmp_scope[key] = eval(key, _math_dict, scope)  
                    except: tmp_scope[key] = '{%s}'%key
                except: dst_seq.append(el); break
    return dst_seq
#-------------------------------------------------------------------------------
import math; _math_dict = dict(math.__dict__)
clif.__doc__ += '\n'+__doc__
#-------------------------------------------------------------------------------    
if __name__ == '__main__':
    import sys
    print(clif(sys.argv[1:]))
#-------------------------------------------------------------------------------    
