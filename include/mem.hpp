#ifndef AIW_MEM_HPP
#define AIW_MEM_HPP

/**
 * Copyright (C) 2016 Antov V. Ivanov, KIAM RAS, Moscow.
 * This code is released under the GPL2 (GNU GENERAL PUBLIC LICENSE Version 2, June 1991)
 **/

#ifndef MINGW
#    include <sys/mman.h>
#endif //MINGW
// #include <cfcntl> ???
#include <cunistd>
#include <memory>
#include "debug.hpp"

namespace aiw{
//---------------------------------------------------------------------------------------------
	class BaseAlloc{
	protected:
		T *ptr;
		size_t size;
	public:
		void* get_addr(){ return ptr; }
		size_t get_size() const { return size; }

		virtual ~BaseAlloc() = 0;
		virtual size_t get_sizeof() const = 0;
	};
//---------------------------------------------------------------------------------------------	
	template<typename T> struct MemAlloc: public BaseAlloc{
		template<typename ... Args> MemAlloc(size_t sz, Args ... args): size(sz) { ptr = new T[sz](args...); }
		~MemAlloc(){ delete [] ptr; }
		size_t get_sizeof()  const { return sizeof(T); }				
	};
//---------------------------------------------------------------------------------------------	
#ifndef MINGW
    class MMapAlloc: public BaseAlloc{
		std::shared_ptr<FILE> pf;
		void *mmap_ptr;
		size_t mmap_sz; 
	public:
		MMapAlloc(const std::shared_ptr<FILE> &pf_, size_t size_, int flags): pf(pf_), size(size_){
			int fd = fileno(pf.get()); size_t pg_sz = sysconf(_SC_PAGE_SIZE), offset = ftell(pf.get());
			mmap_sz = size+offset%pg_sz;
			mmap_ptr = mmap(NULL, mmap_sz, flags, MAP_PRIVATE, fd, offset/pg_sz*pg_sz);
			if(mmap_ptr==MAP_FAILED) AIW_RAISE("can't mmapped file ", fd, offset, size, flags, mmap_sz, pg_sz);
			ptr = (char*)mmap_ptr + offset%pg_sz;
		}
		~MMapAlloc(){ munmap(mmap_ptr, mmap_sz); }
		size_t get_sizeof()  const { return 1; }
    };
#endif //MINGW
//---------------------------------------------------------------------------------------------
};
#endif //AIW_MEM_HPP
