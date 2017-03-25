/**
 * Copyright (C) 2017 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include "../include/aiwlib/configfile"
using namespace aiw;
//------------------------------------------------------------------------------
void aiw::printf_obj(std::ostream &S, bool X){ S<<(X?'Y':'N'); }
void aiw::scanf_obj(std::istream &S, bool &X){
	std::string value; S>>value;
	const char *Y[14] = {"Y", "y", "YES", "Yes", "yes", "ON", "On", "on", "TRUE", "True", "true", "V", "v", "1"};
	const char *N[14] = {"N", "n", "NO", "No", "no", "OFF", "Off", "off", "FALSE", "False", "false", "X", "x", "0"};
	for(int i=0; i<14; i++) if(value==Y[i]){ X = true; return; }
	for(int i=0; i<14; i++) if(value==N[i]){ X = false; return; }
	WRAISE("incorrect value for convert to bool, Y|y|YES|Yes|yes|ON|On|on|TRUE|True|true|V|v|1" 
			   " or N|n|NO|No|no|OFF|Off|off|FALSE|False|false|X|x|0 expected", value);
}
//------------------------------------------------------------------------------
void aiw::ConfigFile::load(std::istream &&fin){
	do {
		std::string line;
		std::getline(fin, line);
		std::stringstream buf(line); std::string key, val;
		std::getline(buf, key, '='); std::getline(buf, val, '#');
		if(key.size() and key[0]!='#') table[key] = val;
	} while(not fin.eof());
}
void aiw::ConfigFile::dump(std::ostream &&S) const {
	for(auto I=table.begin(); I!=table.end(); ++I) S<<I.first()<<'='<<I.second()<<'\n';
}
//------------------------------------------------------------------------------
