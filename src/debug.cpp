/**
 * Copyright (C) 2017 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include <csignal>
#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include "../include/aiwlib/debug"
using namespace aiw;
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
void segfault_hook(int signum, siginfo_t * info, void * f){
	while(exc_counter) exc_frames[--exc_counter]->out_msg(std::cerr);
	void* buf[4096]; int size = backtrace(buf, 4096);
	fprintf(stderr, "\n\n\nSEGMENTATION FAULT WHEN ACCESSING THE ADDRESS %p\nSTACK SIZE %d FRAMES:\n", info->si_addr, size);
	char** strs = backtrace_symbols(buf, size);
	if(!strs) for(int i=0; i<size; ++i) fprintf(stderr, "%p\n", buf[i]);
	else{
		for(int i=0; i<size; ++i) printf("%s\n", strs[i]);  
		free(strs);
	}
	fprintf(stderr, "\nTO VIEW DETAILS, RUN THE COMMAND:\naddr2line -Cpif");
	for(int i = 0; i<size; ++i) fprintf(stderr, " %p", buf[i]);
	fprintf(stderr, " -e YOUR-PROGRAMM\n");
	exit(1);
}
void aiw::init_segfault_hook(){
	struct sigaction act; 
	memset(&act, 0, sizeof(act)); 
	act.sa_sigaction = segfault_hook; 
	act.sa_flags = SA_SIGINFO; 
	sigaction(SIGSEGV, &act, NULL); 
}
//------------------------------------------------------------------------------
