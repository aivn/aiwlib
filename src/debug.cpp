/**
 * Copyright (C) 2017, 2021 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <cxxabi.h>
#ifndef AIW_WIN32
#include <csignal>
#include <execinfo.h>
#endif //AIW_WIN32
#include "../include/aiwlib/debug"
using namespace aiw;

char bUfFoRwRaIsE0[4096];
//------------------------------------------------------------------------------
SplitArgForOut::SplitArgForOut(const char *str){
	const char *br = "()[]{}"; int brc[3] = {0, 0, 0};
	for(end=0; str[end] && (brc[0] || brc[1] || brc[2] || str[end]!=','); ++end)
		for(int k=0; k<6; ++k) if(str[end]==br[k]){ brc[k/2] += 1-k%2*2; break; }
	next = str[end]?end+1:end; while(str[next] && (str[next]==' ' || str[next]=='\t')) next++;
}
//------------------------------------------------------------------------------
static BaseDebugStackTupleFrame *exc_frames[4096];
static int exc_counter = 0;
BaseDebugStackTupleFrame::BaseDebugStackTupleFrame(bool reg){ if(reg && exc_counter<4095) exc_frames[exc_counter++] = this; } 
BaseDebugStackTupleFrame::~BaseDebugStackTupleFrame(){ if(exc_counter>0 && exc_frames[exc_counter-1]==this) exc_counter--; } 
//------------------------------------------------------------------------------
#ifndef AIW_WIN32
void signal_hook(int signum, siginfo_t * info, void * f){
	while(exc_counter) exc_frames[--exc_counter]->out_msg(std::cerr);
	void* buf[4096]; int size = backtrace(buf, 4096);
	fprintf(stderr, "\n\n\nProgram terminated with signal=%d (%s), si_code=%d\n", info->si_signo, strsignal(info->si_signo), info->si_code);
	if(info->si_errno) fprintf(stderr, "[%s]\n", strerror(errno));
	if(info->si_signo==SIGSEGV || info->si_signo==SIGILL || info->si_signo==SIGFPE || info->si_signo==SIGBUS)
		fprintf(stderr, "bad address=%p\n", info->si_addr);
	//	if(info->si_signo==SIGSEGV)
	fprintf(stderr, "Stack size %d frames:\n", size);
	char** strs = backtrace_symbols(buf, size);
	if(!strs) for(int i=0; i<size; ++i) fprintf(stderr, "%p\n", buf[i]);
	else{
		for(int i=0; i<size; ++i) printf("%s\n", strs[i]);  
		free(strs);
	}
	fprintf(stderr, "\nTo view details, run the command:\naddr2line -Cpif");
	for(int i = 0; i<size; ++i) fprintf(stderr, " %p", buf[i]);
	fprintf(stderr, " -e Your-programm\n\nIf the output of the addr2line is not informative, try recompile Your code with '--static' option.\n");
	exit(1);
}
void aiw::init_signal_hook(int signal){
	struct sigaction act; 
	memset(&act, 0, sizeof(act)); 
	act.sa_sigaction = signal_hook; 
	act.sa_flags = SA_SIGINFO; 
	sigaction(signal, &act, NULL); 
}
void aiw::init_segfault_hook(){ init_signal_hook(SIGSEGV); }

void aiw::trace_out(){
	while(exc_counter) exc_frames[--exc_counter]->out_msg(std::cerr);
	void* buf[4096]; int size = backtrace(buf, 4096);
	std::cerr<<"#--------------------------------\n";
	for(int i=1; i<size; i++){
		Dl_info dl; dladdr(buf[i], &dl); int status; 
		std::cerr<<"#["<<i-1<<"] ";
		if(dl.dli_fname) std::cerr<<dl.dli_fname<<": "; else std::cerr<<"???: ";
		char *res = abi::__cxa_demangle(dl.dli_sname, 0, 0, &status);
		if(res) std::cerr<<res<<'\n';
		else if(dl.dli_sname) std::cerr<<dl.dli_sname<<'\n';
		else std::cerr<<"???\n";
	}
	std::cerr<<"#--------------------------------\n";
}
#endif //AIW_WIN32
//------------------------------------------------------------------------------
