#ifndef AIW_DEBUG_HPP
#define AIW_DEBUG_HPP
/**
 * Copyright (C) 2016 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#include <cstring>
#include <cerrno>
#include <iostream>
#include <sstream>
#include <exception>

namespace aiw{
//------------------------------------------------------------------------------
	template <typename T> void debug_out(std::ostream& out, const char* str, T x){ out<<str<<"="<<x; }
	template <typename T, typename ... Args> void debug_out(std::ostream& out, const char* str, T x, Args ... args){
		const char *br = "()[]{}"; int i, brc[3] = {0, 0, 0};
		for(i=0; str[i] && (brc[0] || brc[1] || brc[2] || str[i]!=','); ++i)
			for(int k=0; k<6; ++k) if(str[i]==br[k]){ brc[k/2] += 1-k%2*2; break; }
		out.write(str, i); out<<"="<<x<<", ";
		int p= str[i]?i+1:i; while(str[p] && (str[p]==' ' || str[p]=='\t')) p++;
		debug_out(out, str+p, args...);
	}
//------------------------------------------------------------------------------
	struct DebugStackFrame{
		std::stringstream buf;
		DebugStackFrame(){ buf.copyfmt(std::cerr); }
		~DebugStackFrame(){ if(std::uncaught_exception()) std::cerr<<buf.str(); }
	};
//------------------------------------------------------------------------------
};

#ifdef EBUG
#   define WSTR(out, args...) { out<<"#"<<__FILE__<<" "<<__FUNCTION__<<"() "<<__LINE__<<": "; \
		aiw::debug_out(out, #args, args); out<<"\n"; }
#   define NAME_OF_STFR1(num) tmp_wrap_stack_frame##num
#   define NAME_OF_STFR2(num) NAME_OF_STFR1(num)
#   define WEXC(args...) aiw::DebugStackFrame NAME_OF_STFR2(__LINE__); WSTR(NAME_OF_STFR2(__LINE__).buf, args)
#   define WASSERT(cond, msg, args...) if(!(cond)) AIW_RAISE(msg, args)
#else //EBUG
#   define WSTR(S, args...)
#   define WEXC(args...)
#   define WASSERT(cond, msg, args...)
#endif //EBUG

#define WOUT(args...) WSTR(std::cout, args)
#define WERR(args...) WSTR(std::cerr, args)
#define AIW_WARNING(msg, args...){ std::cerr<<"#"<<__FILE__<<" "<<__FUNCTION__<<"() "<<__LINE__<<": "<<msg; \
		aiw::debug_out(std::cerr, #args, args); std::cerr<<"\n"; }
#define AIW_RAISE(msg, args...){ std::stringstream buf;					\
		buf<<"#"<<__FILE__<<" "<<__FUNCTION__<<"() "<<__LINE__<<": "<<msg; \
		if(errno) buf<<" ["<<strerror(errno)<<"] "; aiw::debug_out(buf, #args, args); buf<<"\n"; \
		std::cerr<<buf.str(); throw buf.str().c_str(); }	
//------------------------------------------------------------------------------
#endif //AIW_DEBUG_HPP
