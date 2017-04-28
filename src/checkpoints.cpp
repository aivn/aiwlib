/**
 * Copyright (C) 2017 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include "../include/aiwlib/checkpoints"
using namespace aiw;

CheckPoint checkpoint;
//------------------------------------------------------------------------------
void aiw::CheckPoint::init(const char *path, bool wmode){
	if(wmode){ stream.name = path; rcounter = -1; }
	else { stream = File(path, "rb"); stream>table;	rcounter = table.size()-1; }
}
//------------------------------------------------------------------------------
int aiw::CheckPoint::frame(const char* fname, int line, const char *argnames){ // 0 --- skip frame, 1 --- read frame, 2 --- write frame
	std::stringstream buf; buf<<fname<<":l"<<line<<"("<<argnames<<")";
	if(rcounter==-1){
		table[buf.str()] = table.size();
		stream = File(stream.name.c_str(), "wb");
		stream<table;
		return 2;
	}
	auto I = table.find(buf.str());
	if(I==table.end() || I->second!=table.size()-rcounter){
		auto J = table.begin(); while(J!=table.end() && J->second!=table.size()-rcounter) ++J;
		WARNING("sources changed or programm trace broked: ", stream.name, buf.str(), J->first, J->second, rcounter, table.size());
		int ID = J->second; table.erase(J); table[buf.str()] = ID;
	}
	return rcounter--==0; 
}
//------------------------------------------------------------------------------
