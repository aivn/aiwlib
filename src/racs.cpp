/**
 * Copyright (C) 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

//#include <mpi.h>
#include <omp.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <wait.h>
#include <utime.h>
#include <ctime>
#include <fstream>
#include "../include/aiwlib/configfile"
#include "../include/aiwlib/mixt"
#include "../include/aiwlib/racs"
#include "../include/aiwlib/mpi4py"
using namespace aiw;

const char *racs_help = "опции командной строки:\n"
	"  key=value --- задает значение параметра расчета key, возможно задание серии значений параметра как\n"
	"  key=[x1,x2,...,xn]  --- перечисление значений через запятую\n"
	"  key=[x1,x2..xn]     --- хаскелль-стиль для арифметической прогрессии\n"
	"  key=[x1:step..xn] --- арифметическая прогрессия с шагом step\n"
	"  key=[x1@step..xn] --- геометрическая прогрессия с множителем step\n"
	"  key=[x1#size..xn] --- арифметическая прогрессия из size элементов\n"
	"  key=[x1^size..xn] --- геометрическая прогрессия из size элементов\n"
	"Значения x1, xn, step приводятся к типу double. Параметры x1 и xn\n"
	"всегда включаются в серию. При явном задании параметра step шаг всегда\n"
	"корректируется для точного попадания в xn. Параметр size должен быть\n"
	"целочисленным  (не менее двух). Если серии заданы для нескольких\n" 
	"параметров, вычисляются все возможные комбинации значений (декартово произведение).\n"

	"\n  -h|--help --- показать эту справку и выйти\n\n"

	"Для всех параметров (кроме серийных) возможно дублирование, актуальным является\n"
	"последнее значение. Для булевых параметров имя параметра экивалентно заданию значения True, кроме того\n"
	"можно использовать значения  Y|y|YES|Yes|yes|ON|On|on|TRUE|True|true|V|v|1 или\n"
	"N|n|NO|No|no|OFF|Off|off|FALSE|False|false|X|x|0.\n\n"

	"  -r|--repo=PATH --- задает путь к репозитрию для создания расчета\n"
    "  path=PATH --- явно задает путь к директории расчета\n" 
	"  -p|--clean-path[=Y] --- очищать явно заданную директорию расчета\n"
	"  -s|--symlink[=Y] --- создавать символическую ссылку '_' на последнюю\n" 
	"                       директорию расчета\n"
	"  -S|--statechecker[=Y] --- запускать демона, фиксирующего  в файле .RACS аварийное\n"
	"                            завершение расчета\n"
	"  -c|--copies=1 --- число копий процесса при проведении расчетов с серийными\n"
	"                    параметрами\n"
	"  --mpi[=N] --- для запуска из под MPI.\n\n";
//------------------------------------------------------------------------------
bool startswith(const std::string &str, const char *substr){
	for(size_t i=0; substr[i]; i++) if(i==str.size() || substr[i]!=str[i]) return false;
	return true;
}
bool parse_opt(const std::string& arg, std::string patt, bool &opt){
	if(startswith(arg, (patt+"=").c_str())){ std::stringstream buf(arg.substr(patt.size()+1, arg.size())); scanf_obj(buf, opt); return true; }
	else if(startswith(arg, patt.c_str())){ opt = true; return true; }
	return false;
}
template <typename T> bool parse_opt(const std::string& arg, std::string patt, T &opt){
	if(startswith(arg, patt.c_str())){ std::stringstream buf(arg.substr(patt.size(), arg.size())); scanf_obj(buf, opt); return true; }
	return false;
}
void set_queue_item(std::vector<int> &qpos, const std::vector<std::string> &qkeys,
					const std::vector<std::vector<double> >& qgrid, std::map<std::string, std::string> &arg_params){
	for(size_t i=0; i<qpos.size(); ++i){
		std::stringstream buf; buf<<qgrid[i][qpos[i]];
		arg_params[qkeys[i]] = buf.str();
	} 
}
bool next_queue_item(std::vector<int> &qpos, const std::vector<std::vector<double> >& qgrid){
	//if(qpos.back()>=int(qgrid.back().size())) return false;
	qpos[0]++; 
	for(int i=0; i<int(qpos.size())-1; i++)
		if(qpos[i]<int(qgrid[i].size())) break;
		else{ qpos[i] = 0; qpos[i+1]++; }
	return qpos.back()<int(qgrid.back().size());
}
//------------------------------------------------------------------------------
aiw::RacsCalc::RacsCalc(int argc, const char **argv){
	bool clean_path = true, use_mpi = false, schecker=true;
	int copies = 1;
	std::map<std::string, std::vector<double> > qparams;
	std::list<std::string> qparams_order;
	for(int i=0; i<argc; i++) args.push_back(argv[i]);
	for(int i=1; i<argc; i++){
		std::string a = argv[i];
		if(a=="-h" || a=="--help"){ std::cout<<racs_help; exit(0); }
		if(a.back()=='+'){ a.resize(a.size()-1); tags.push_back(a); continue; }
		if(parse_opt(a, "-r=", repo) || parse_opt(a, "--repo=", repo) || parse_opt(a, "path=", path_) ||
		   parse_opt(a, "-p", clean_path) || parse_opt(a, "--clean-path", clean_path) ||
		   parse_opt(a, "-s", this->symlink) || parse_opt(a, "--symlink", this->symlink) ||
		   parse_opt(a, "-S", schecker) || parse_opt(a, "--statechecker", schecker) ||
		   parse_opt(a, "-c=", copies) || parse_opt(a, "--copies=", copies) ||
		   parse_opt(a, "--mpi", use_mpi)) continue;
		size_t eq_pos = 0; while(eq_pos<a.size() && a[eq_pos]!='=') ++eq_pos;
		if(eq_pos<a.size()){
			std::string key = a.substr(0, eq_pos), val = a.substr(eq_pos+1, a.size());
			if(val.front()=='[' and val.back()==']'){ // подготовка очереди
				std::vector<double> &L = qparams[key]; qparams_order.push_back(key);
				if(arg_params.find(key)!=arg_params.end()) arg_params.erase(key);
				std::stringstream buf(val.substr(1, val.size()-2));
				size_t dd = val.find("..");
				if(dd<val.size()){
					double a, b, c=::atof(val.substr(dd+2, val.size()-dd-3).c_str()); char t; buf>>a>>t>>b; // проверить состояние ошибки buf!!!
					if(t==','){ b -= a; t = ':'; }
                    if(t==':'){ b = int((c-a)/b+1.5); t = '#'; }
                    if(t=='#'){ double d = (c-a)/(b-1); for(int j=0; j<int(b); j++) L.push_back(a+d*j); }
                    if(t=='@'){ b = int(log(c/a)/log(b)+1.5); t = '^'; } // c=a*d**(n-1)
                    if(t=='^'){ double d = exp(log(c/a)/(b-1)); for(int j=0; j<int(b); j++){ L.push_back(a); a *= d; } }
					if(L.empty()) WRAISE("incorrect step or limits in expression: ", a, key, val, a, b, c, t);
				} else do { double x; buf>>x; L.push_back(x); } while(buf.get()==','); //???
			} else{
				arg_params[key] = val;
				if(qparams.find(key)!=qparams.end()) qparams.erase(key);
				auto I=qparams_order.begin(); while(I!=qparams_order.end()) if(*I==key) qparams_order.erase(I++); else ++I;
			}
		}
	}
	if(qparams.empty()){ // нет пакетного запуска
		if(!path_.empty() && clean_path && system(("rm -rf "+path_+"/*").c_str()));
		if(!path_.empty()) calc_configure();
	} else { // есть пакетный запуск
		std::vector<std::string> qkeys(qparams.size());          // имена праметров
		std::vector<std::vector<double> > qgrid(qparams.size()); // сетка значений параметров
		int len_queue=1, n_start=0, n_finish=0; 
		{ int i=0; for(auto K=qparams_order.begin(); K!=qparams_order.end(); ++K){
				auto I = qparams.find(*K); len_queue *= I->second.size();
				qkeys[i] = I->first; qgrid[i++].swap(I->second);
			}
		}
		std::vector<int> qpos(qparams.size(), 0);  // позиция в сетке значений для пакетного запуска
		if(use_mpi){
			MPI_Init(&argc, (char***)&argv);
			int procID = mpi_proc_number(), proc_count = mpi_proc_count();
			if(procID==0){ // головной процесс
				std::ostream *logs[2]; out_preambule(logs, "", argc, argv, proc_count-1, qkeys, qgrid);
				schecker = clean_path = false;
				do { // цикл по заданиям
					aiw_mpi_msg_t msg; mpi_recv(msg, -1); // получаем запрос на задание, но на самом деле нам интересен только процесс от которого он пришел
					std::stringstream sbuf; for(size_t i=0; i<qpos.size(); ++i) sbuf<<qpos[i]<<' ';
					set_queue_item(qpos, qkeys, qgrid, arg_params); 				  
					path_ = ""; sbuf<<path(); mpi_send(sbuf.str(), msg.source); // формируем задание и отправляем его вычислителю
					out_start_task(logs, msg.source, double(++n_start)/len_queue, qpos, qkeys, qgrid);
				} while(next_queue_item(qpos, qgrid)); // конец цикла по заданиям
				for(int i=1; i<proc_count; i++) mpi_send("bye", i); // рассылаем сообщение о завершении работы
				out_close_log(logs, "");
			} else { // вычислитель
				char hostname[256]; ::gethostname(hostname, 255); 
				while(1){
					mpi_send(hostname, 0); // посылаем запрос на задание
					aiw_mpi_msg_t msg; mpi_recv(msg, 0); // получаем задание
					if(msg.data=="bye") break;
					if(fork()){	int wstatus;  waitpid(-1, &wstatus, 0); } // головной процесс
					else{ // дочерний процесс
						std::stringstream buf(msg.data);
						for(int i=0; i<int(qpos.size()); i++) buf>>qpos[i];
						path_ = buf.str().substr(int(buf.tellg())+1, 4095);
						set_queue_item(qpos, qkeys, qgrid, arg_params);  				  
						set_state("started");
						return;
					}
				}
			}
			MPI_Finalize();
			exit(0);
		} else { // not MPI
			std::ostream *logs[2]; out_preambule(logs, "/tmp/", argc, argv, copies,  qkeys, qgrid);
			std::list<pid_t> pids; int wstatus;
			do{
				if(int(pids.size())==copies){
					pid_t pid = waitpid(-1, &wstatus, 0);
					pids.remove(pid); out_finish_task(logs, pid, double(++n_finish)/len_queue);
				}
				pid_t pid = fork();
				if(!pid){ set_queue_item(qpos, qkeys, qgrid, arg_params); set_state("started"); return; } // дочерний процесс
				pids.push_back(pid); out_start_task(logs, pid, double(++n_start)/len_queue, qpos,  qkeys, qgrid);
			} while(next_queue_item(qpos, qgrid));
			while(pids.size()){ pid_t pid = waitpid(-1, &wstatus, 0); pids.remove(pid); out_finish_task(logs, pid, double(++n_finish)/len_queue); }
			out_close_log(logs, "/tmp/"); exit(0);
		}
	}
	set_state("started");
}
//------------------------------------------------------------------------------
aiw::RacsCalc::~RacsCalc(){
	if(!path_.empty()){
		// runtime = omp_get_wtime()-starttime;
		if(std::uncaught_exception()) set_state("stopped");
		else{ progress = 1.; set_state("finished"); }
		commit();
	}
}
//------------------------------------------------------------------------------
const std::string& aiw::RacsCalc::path(){
	if(path_.empty()){ // создаем уникальную директорию расчета
		std::string frepo = format_string(repo.c_str(), arg_params); // ???
		path_ = make_path(frepo.c_str());
		if(path_.empty()) WRAISE("cann't make unique path for calc: ", repo, frepo);
		calc_configure();
	}
	return path_;
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::calc_configure(){ // конфигурация расчета когда известен путь - настраиваем демона для завершения и т.д.
	if(this->symlink){ ::unlink("_"); if(::symlink(path().c_str(), "_")) WARNING("can't create symlink '_': ", path(), strerror(errno)); }
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::set_state(const std::string& state){
	if(state=="started") starttime = omp_get_wtime();
	state_t st; st.state = state; st.date = std::time(nullptr);
	statelist.push_back(st);
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::push(){ // push parametrs to controls
	for(auto I=controls.begin(); I!=controls.end(); ++I) (*I)->update(*this);
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::commit(){
	Pickle P = pickle_dict();	
	P("progress", progress)("runtime", pickle_class("aiwlib.chrono", "Time", false)<<runtime)("md5sum", md5sum);	
	{ Pickle L = pickle_list(); for(auto I=args.begin(); I!=args.end(); ++I) L<<*I; P("args", L); }	
	{ Pickle L = pickle_set();  for(auto I=tags.begin(); I!=tags.end(); ++I) L<<*I; P("tags", L); }	
	{ Pickle L = pickle_list();
		char hostname[256]; ::gethostname(hostname, 255);
		for(auto I=statelist.begin(); I!=statelist.end(); ++I){
			L<<pickle_tuple(I->state, getenv("USER"), hostname,	pickle_class("aiwlib.chrono", "Date", false)<<I->date, getpid());
		}
		P("statelist", L);
	}		
	for(auto I=controls.begin(); I!=controls.end(); ++I) (*I)->pickle(P);
	for(auto I=params.begin(); I!=params.end(); ++I) I->second->pickle(I->first, P);	
	std::ofstream(path()+".RACS")<<P.buf.str()<<'.';
	utime(path_.c_str(), nullptr);
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::set_progess(double progress_){
	progress = progress_; runtime = omp_get_wtime()-starttime; 
	commit();
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::out_preambule(std::ostream **logs, const char *logdir, int argc, const char **argv, int copies,
								  const std::vector<std::string> &qkeys, const std::vector<std::vector<double> > &qgrid){
	int len_queue = 1; for(auto I=qgrid.begin(); I!=qgrid.end(); ++I) len_queue *= I->size();
	char hostname[256], logfile[256]; ::gethostname(hostname, 255); snprintf(logfile, 255, "%sracs-started-%i", logdir, getpid());
	logs[0] = &std::cout; logs[1] = new	std::ofstream(logfile);
	printf("Start queue for %i items in %i threads, master PID=%i, logfile=\"%s\"", len_queue, copies, getpid(), logfile);
	for(int k=0; k<2; k++){
		(*logs[k])<<"# "<<getenv("USER")<<'@'<<hostname<<':'<<get_current_dir_name()<<" repo="<<repo
				  <<" tasks="<<len_queue<<" threads="<<copies<<"\n#";
		for(int i=0; i<argc; i++){ (*logs[k])<<' '<<argv[i]; } (*logs[k])<<'\n';
		for(size_t i=0; i<qkeys.size(); i++){
			(*logs[k])<<"#   "<<qkeys[i]<<": ["<<qgrid[i][0];
			for(size_t j=0; j<qgrid[i].size(); ++j) (*logs[k])<<", "<<qgrid[i][j];
			(*logs[k])<<"]\n";
		}
		logs[k]->flush();
	}
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::out_start_task(std::ostream **logs, int PID, double sprogress, const std::vector<int> &qpos,
								   const std::vector<std::string> &qkeys, const std::vector<std::vector<double> > &qgrid){
	for(int k=0; k<2; k++){
		*logs[k]<<date2string(time(nullptr))<<" +"<<PID<<' '<<sprogress*100<<'%';
		for(size_t i=0; i<qpos.size(); ++i) *logs[k]<<' '<<qkeys[i]<<'='<<qgrid[i][qpos[i]];
		*logs[k]<<'\n'; logs[k]->flush();
	}
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::out_finish_task(std::ostream **logs, int PID, double sprogress){
	double rt = omp_get_wtime()-starttime;
	for(int k=0; k<2; k++){
		*logs[k]<<date2string(time(nullptr))<<" -"<<PID<<' '<<sprogress*100<<"% "<<time2string(rt)<<" "<<time2string(rt/sprogress)<<'\n';
		logs[k]->flush();
	}
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::out_close_log(std::ostream **logs, const char *logdir){
	delete logs[1];
	char logfile1[256], logfile2[256]; int pid =  getpid();
	snprintf(logfile1, 255, "%sracs-started-%i", logdir, pid);
	snprintf(logfile2, 255, "%sracs-finished-%i", logdir, pid);
	rename(logfile1, logfile2);
}
//------------------------------------------------------------------------------
