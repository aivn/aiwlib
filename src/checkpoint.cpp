/**
 * Copyright (C) 2017 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include "../include/aiwlib/checkpoint"
using namespace aiw;

namespace aiw{ CheckPoint checkpoint; };
//------------------------------------------------------------------------------
void aiw::CheckPoint::init(const char *path, bool wmode){
	if(wmode){ stream.name = path; cursor = table.end(); }
	else { stream = File(path, "rb"); stream>table;	cursor = table.begin(); }
}
//------------------------------------------------------------------------------
int aiw::CheckPoint::frame(const char* fname, int line, const char *argnames){ // 0 --- skip frame, 1 --- read frame, 2 --- write frame
	std::stringstream buf; buf<<fname<<":l"<<line<<"("<<argnames<<")";
	if(cursor==table.end()){
		for(int i=0; i<int(table.size())-1; ++i) WASSERT(table[i]!=buf.str(), "invalid checkpoint position on program trace: ", buf.str(), i);
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
