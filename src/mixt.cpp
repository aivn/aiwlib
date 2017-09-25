/**
 * Copyright (C) 2017 Antov V. Ivanov  <aiv.racs@gmail.com>
 * Licensed under the Apache License, Version 2.0
 **/

#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sstream>

#include "../include/aiwlib/debug"
#include "../include/aiwlib/mixt"
using namespace aiw;
//------------------------------------------------------------------------------
std::string aiw::make_path(const char *repo){
	// prepare repo
	errno = 0;
	DIR *dp = ::opendir(repo);
	if(!dp){
		int e = repo[0]=='/'; while(repo[++e]) if(repo[e]=='/') ::mkdir(std::string(repo).substr(0, e).c_str(), 0755);
		::mkdir(repo, 0755);
	}
	errno = 0;
	dp = ::opendir(repo); if(!dp) return "";
	int counter = 0;

	// calc prefix
	time_t rawtime;  time (&rawtime); tm * timeinfo = localtime(&rawtime);
	char buf[4096]; sprintf(buf, "c%02i_%02i_%i", timeinfo->tm_year%100, timeinfo->tm_yday/7+1, (timeinfo->tm_wday-1)%7+1);
	std::string prefix(buf);
	
	// calc counter
	while(true){
		errno = 0;
		dirent *de = readdir(dp);
		if(de==NULL) break;
		std::string name(de->d_name);
		if(name.substr(0, prefix.size())==prefix){
			int c = atoi(name.substr(prefix.size(), name.size()).c_str());
			if(counter<c) counter = c;
		}
	}

	// make unique path
	for(int i=0; i<10; i++){
		sprintf(buf, "%s/%s%03i/", repo, prefix.c_str(), ++counter);
		if(!::mkdir(buf, 0755)) return buf;
	}
	return "";
}
//------------------------------------------------------------------------------
std::string aiw::format_string(const char *pattern, const std::map<std::string, std::string> &dict){
	std::stringstream buf; std::string key; bool wkey = false;
	for(int i=0; pattern[i]; i++){
		if(pattern[i]=='{'){
			if(wkey) buf<<'{'<<key;
			key.clear(); wkey = true;
		} else if(pattern[i]=='}' && wkey){
			auto I = dict.find(key);
			if(I==dict.end()) WRAISE("key error: ", pattern, buf.str(), key);
			buf<<I->second; key.clear(); wkey = false;
		} else {
			if(wkey) key += pattern[i];
			else buf<<pattern[i];
		}
	}
	return buf.str();
}
//------------------------------------------------------------------------------
std::string aiw::time2string(double t){
	char buf[64];
	snprintf(buf, 63, "%i:%02i:%06.3f", int(t/3600), int(t/60)%60, t-int(t/60)*60);
	return buf;
}
std::string aiw::date2string(time_t d){
	char buf[64];
	::strftime(buf, 63, "%Y.%m.%d-%X", ::localtime(&d));
	return buf;
}
//------------------------------------------------------------------------------
