/**
 * Copyright (C) 2017 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include <mpi.h>
#include <omp.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <wait.h>
#include "../include/aiwlib/configfile"
#include "../include/aiwlib/mixt"
#include "../include/aiwlib/racs"
using namespace aiw;

const char *racs_help = " ";
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
bool next_queue_item(std::vector<int> &qpos, std::vector<double> &qvals, const std::vector<std::vector<double> >& qgrid){
	qpos[0]++; //params.resize(qpos.size());
	for(int i=0; i<int(qpos.size())-1; i++)
		if(qpos[i]<int(qgrid[i].size())) break;
		else{ qpos[i] = 0; qpos[i+1]++; }
	if(qpos.back()>=int(qgrid.back().size())) return false;
	for(int i=0; i<int(qpos.size()); i++) qvals.at(i) = qgrid[i][qpos[i]];
	return true;
}
//------------------------------------------------------------------------------
aiw::RacsCalc::RacsCalc(int argc, const char **argv){
	bool clean_path = true, use_mpi = false, schecker=true;
	int copies = 1;
	std::map<std::string, std::vector<double> > qparams;
	for(int i=0; i<argc; i++) args.push_back(argv[i]);
	for(int i=1; i<argc; i++){
		std::string a = argv[i];
		if(a=="-h" || a=="--help"){ std::cout<<racs_help; exit(0); }
		if(parse_opt(a, "-r=", repo) || parse_opt(a, "--repo=", repo) || parse_opt(a, "path=", path_) ||
		   parse_opt(a, "-p", clean_path) || parse_opt(a, "--clean-path", clean_path) ||
		   parse_opt(a, "-S", schecker) || parse_opt(a, "--statechecker", schecker) ||
		   parse_opt(a, "-c=", copies) || parse_opt(a, "--copies=", copies) ||
		   parse_opt(a, "--mpi", use_mpi)) continue;
		size_t eq_pos = 0; while(eq_pos<a.size() && a[eq_pos]!='=') ++eq_pos;
		if(eq_pos<a.size()){
			std::string key = a.substr(0, eq_pos), val = a.substr(eq_pos+1, a.size());
			if(val.front()=='[' and val.back()==']'){ // подготовка очереди
				std::vector<double> &L = qparams[key];
				std::stringstream buf(val.substr(1, val.size()-2));
				if(val.find("..")<val.size()){
					double a, b, c; char t, dd[2]; buf>>a>>t>>b>>dd[0]>>dd[1]>>c; // проверить состояние ошибки buf!!!
					if(t==','){ b -= a; t = ':'; }
                    if(t==':'){ b = int((c-a)/b+1.5); t = '#'; }
                    if(t=='#'){ double d = (c-a)/(b-1); for(int j=0; j<int(b); j++) L.push_back(a+d*j); }
                    if(t=='@'){ b = int(log(c/a)/log(b)+1.5); t = '^'; } // c=a*d**(n-1)
                    if(t=='^'){ double d = exp(log(c/a)/(b-1)); for(int j=0; j<int(b); j++){ L.push_back(a); a *= d; } }
					if(L.empty()) WRAISE("incorrect step or limits in expression: ", a, key, val, a, b, c, t);
				} else do { double x; buf>>x; L.push_back(x); } while(buf.get()==','); //???
			} else arg_params[key] = val;
		}
	}
	if(qparams.empty()){ // нет пакетного запуска
		if(!path_.empty() && clean_path && system(("rm -rf "+path_+"/*").c_str()));
		if(!path_.empty()) calc_configure();
	} else { // есть пакетный запуск
		std::vector<std::string> qkeys(qparams.size()); std::vector<std::vector<double> > qgrid(qparams.size());
		int i=0; for(auto I=qparams.begin(); I!=qparams.end(); ++I){ qkeys[i] = I->first; qgrid[i++].swap(I->second); }
		std::vector<double> qvals(qparams.size()); std::vector<int> qpos(qparams.size(), 0); // позиция в сетке значений для пакетного запуска
		if(use_mpi){
		} else {
			std::list<pid_t> pids; int wstatus;
			while(next_queue_item(qpos, qvals, qgrid)){
				if(int(pids.size())==copies){
					pid_t pid = waitpid(-1, &wstatus, 0);
					pids.remove(pid);
				}
				pid_t pid = fork();
				if(!pid){
					for(size_t k=0; k<qparams.size(); ++k) arg_params[qkeys[k]] = qvals[k];
					return; 
				}
				pids.push_back(pid);
			}
			while(pids.size()){ pid_t pid = waitpid(-1, &wstatus, 0); pids.remove(pid); }
			exit(0);
		}
	}
}
//------------------------------------------------------------------------------
const std::string& aiw::RacsCalc::path(){
	if(path_.empty()){ // создаем уникальную директорию расчета
		std::string frepo = format_string(repo.c_str(), arg_params);
		path_ = make_path(frepo.c_str());
		if(path_.empy()) WRAISE("cann't make unique path for calc: ", repo, frepo);
		calc_configure();
	}
	return path_;
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::calc_configure(){ // конфигурация расчета когда известен путь - настраиваем демона для завершения и т.д.
}
//------------------------------------------------------------------------------
void aiw::RacsCalc::set_state(const std::string& state){
	state_t st; st.state = state; st.date = omp_get_wtime();
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
	{ Pickle L = pickle_list();
		for(auto I=statelist.begin(); I!=statelist.end(); ++I) L<<pickle_tuple(I->state, getenv("USER"), getenv("HOSTNAME"),
																		  pickle_class("aiwlib.chrono", "Date", false)<<I->date, getpid());
		P("statelist", L);
	}
	for(auto I=controls.begin(); I!=controls.end(); ++I) (*I)->pickle(P);
	for(auto I=params.begin(); I!=params.end(); ++I) I->second->pickle(I->first, P);
	
	std::ofstream(path())<<P.buf.str()<<'.';
}
//------------------------------------------------------------------------------
