/**
 * Copyright (C) 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <unistd.h>
#include "../include/aiwlib/checkpoint"
using namespace aiw;

namespace aiw{ CheckPoint checkpoint; };
//------------------------------------------------------------------------------
void aiw::CheckPoint::init(const char *path, int mode){ // mode==0 - auto, 1 - read, other - write
	if(mode!=1 || (mode==0 && ::access(path, F_OK|R_OK))){ stream.name = path; cursor = table.end(); } // write
	else { stream = File(path, "rb"); stream>table;	cursor = table.begin(); }                          // read
}
//------------------------------------------------------------------------------
int aiw::CheckPoint::frame(const char* fname, int line, const char *argnames){ // 0 --- skip frame, 1 --- read frame, 2 --- write frame
	std::stringstream buf; buf<<fname<<":l"<<line<<"("<<argnames<<")";
	if(cursor==table.end()){
		for(int i=0; i<int(table.size())-1; ++i) if(table[i]==buf.str()) WRAISE("invalid checkpoint position on program trace: ", fname, line, argnames, i);
		if(table.empty() || table.back()!=buf.str()){ table.push_back(buf.str()); cursor = table.end(); }
		stream = File(stream.name.c_str(), "wb");
		stream<table;
		return 2;
	}
	if(*cursor!=buf.str()){
		WARNING("sources changed or programm trace broked: ", stream.name, buf.str(), *cursor, table.size());
		*cursor = buf.str();
	}
	return ++cursor==table.end(); 
}
//------------------------------------------------------------------------------
