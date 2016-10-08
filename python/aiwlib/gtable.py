# -*- coding: utf-8 -*-
'Конфигурация таблицы calc._G'
import os, time, fnmatch, math, mixt, chrono
from calc import _G, ghelp
def use(key, expr, doc): 'добавляет значение в глобальную таблицу'; _G[key] = expr; ghelp.append('%s : %s'%(key, doc))
#-------------------------------------------------------------------------------
use('os', os, 'module os') 
use('timemodule', time, 'module time') 
use('Date', chrono.Date, 'объект даты') 
use('Time', chrono.Time, 'объект интервала времени')
use('inf', float('inf'), 'float("inf")') 
use('nan', float('nan'), 'float("nan")') 
#use( 'reg', reg, 'класс region')
#use('K', 2**10, '2**10') #??? 
#use('M', 2**20, '2**20') #???
#use('G', 2**30, '2**30') #???
#use('T', 2**40, '2**40') #???
use('@size', 'int(os.popen("du -bs "+path).readline().split()[0])', 'int, размер расчета в байтах')
use('@hsize', 'os.popen("du -hs "+path).readline().split()[0]', 'размер расчета в "человеческом" представлении')
use('@lendir', 'len(os.listdir(path))', 'число файлов и папок в директории расчета')
use('time2string', mixt.time2string, 'time as float --> time as string "hh:mm:ss.sss"')
use('string2time', mixt.string2time, 'time as string "hh:mm:ss.sss" --> time as float')
use('date2string', mixt.date2string, 'date as float --> date as string "YYYY.MM.DD-hh:mm:ss.sss"')
use('string2date', mixt.string2date, 'date as string "YYYY.MM.DD-hh:mm:ss.sss" --> date as float')
use('size2string', mixt.size2string, 'size as string for human')
#                   '@catalog': 'os.path.dirname(path)', 
#                   '@name_ext':'os.path.basename(path)',
#                   '@name':'min(name_ext.rsplit(".",1)[0],name_ext.split(".dat")[0],name_ext.split(".tar")[0])',
#                   '@ext':'name_ext[len(name):]',
use('@calc', 'os.path.basename(path[:-1])', 'уникальное имя расчета в репозитории')
use('@user', 'statelist[-1][1]', 'последний пользователь изменивший состояние расчета')
use('@host', 'statelist[-1][2]', 'последний хост изменивший состояние расчета')
use('@mdate', 'Date(statelist[-1][3])', 'последняя дата изменения состояние расчета')
use('@cdate', 'Date(statelist[0][3])', 'дата создания расчета')
use('@sdate', 'Date(filter( lambda s : s[0]=="started", statelist )[0][3])', 'дата запуска расчета')
use('@adate', 'Date(filter( lambda s : s[0]=="activated", statelist )[-1][3])', 'дата активации расчета')
use('@logsize', 'lambda path=path : os.path.getsize(path+"logfile")', 'размер логфайла в байтах')
#use('sources', (lambda path, pattern='*' : filter( lambda s : fnmatch.fnmatch(s[0],pattern),  
#                                                   map(lambda l:( l[-1], 
#                                                                date2string(time.mktime(time.strptime(l[3]+' '+l[4],'%Y-%m-%d %H:%M'))), 
#                                                                  int(l[2]) ), 
#                                                       map( str.split, os.popen('tar -ztvf %s.sources.tar.gz'%path).readlines() ) ) ) ),
#     'sources(path,pattern="*") --- создает список исходных файлов отвечаюших шаблону pattern для расчета c путем path' )
#use('@status', 'statelist[-1][0]','текущий статус расчета "waited"|"activated"|"started"|"stopped"|"finished"|"killed"|"suspended"')
use('@status', 'statelist[-1][0]', 'текущий статус расчета "started"|"stopped"|"finished"|"killed"')
use('@PID' , '(None,statelist[-1][-1])[statelist[-1][0]=="started"]', 'PID расчета (int если запущен, иначе None)')
#use('@waited', 'statelist[-1][0]=="waited"', 'True для расчета ожидающего запуска')
#use('@activated', 'statelist[-1][0]=="activated"', 'True для расчета активированного планировщиком')
use('@started', 'statelist[-1][0]=="started"', 'True для запущенного расчета')
use('@stopped', 'statelist[-1][0]=="stopped"', 'True для остановленного расчета (обычно вследствии ошибки)')
use('@finished', 'statelist[-1][0]=="finished"', 'True для успешно завершенного расчета')
#use( '@suspended', 'statelist[-1][0]=="suspended"', 'True для приостановленного (пользователем) расчета' )
use('@killed', 'statelist[-1][0]=="killed"', 'True для убитого (некорректно завершенного по неизвестным причинам) расчета')
use('@mutates', '"-".join(map( lambda s : s[0], statelist ))', 'строка с последовательностью изменений статуса расчета')
#use('@rewaite', 'mutates.count("activated-waited")', 'число сбросов из состояния активации в состояние ожидания' )
use('@state',
    '("%*.*f%% %s from %s"%(2,1,100*progress,time2string(runtime,0),time2string(runtime/(progress+1e-24),0)),"---")[not progress]',
    'состояние расчета --- время счета, прогресс, предполагаемое общее время счета')
use('@lefttime', 'Time(runtime*(1./progress-1))', 'предполагаемое время до завершения расчета')
use('@finishdate', 'Date(statelist[-1][3]+runtime/progress if started else statelist[-1][3])',
    'предполагаемая дата завершения расчета')
use('fnmatch', fnmatch.fnmatch, 'fnmatch(string, pattern) --- функция проверки соответствия шаблону')
use('@logfile', '" ".join(open(path+"logfile").readlines()).strip() if os.path.exists(path+"logfile") else "---"',
    'выдает содержимое logfile')
#use( 'priority', 0, 'приоритет запуска' ) #???
use('on_racs_call_error', 2, 'действия при ошибке в методе _RACS.__call__: 0 --- остановка, 1 --- полный отчет, 2 --- краткий отчет, 3 --- игнорировать') #???
#-------------------------------------------------------------------------------
class _Region:
    def __init__(self, a, b): self.a, self.b = a, b
    def __getitem__(self, d): return _Region(self.a-d, self.b+d)
    def __radd__(self, other): return _Region(other+self.a, other+self.b)
    def __eq__(self, other): return self.a<=other and other<=self.b
use('EPS', _Region(0,0), 'объект интевала')
#-------------------------------------------------------------------------------
for k, v in math.__dict__.items() : 
    if k[0]!='_': use(k, v,  'число %k'%k if type(k) is float else v.__doc__.replace('\n',' '))
del k, v, use
#-------------------------------------------------------------------------------
