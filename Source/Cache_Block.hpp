/*
	Colin Van Oort
	CS 222: Computer Architecture
	Cache Simulation Assignment
	Cache Block struct Header file

	The most basic cache structure
*/


#ifndef CACHE_BLOCK_HPP
#define CACHE_BLOCK_HPP

#include <inttypes.h>

	struct Cache_Block{
		bool dirty; // has the data in this block been modified
		bool pBit; // used in pseudo-LRU
		bool replace; // has this block been replaced, used in write back and write through
		uint64_t address; // the address held by this cache block
		uint tag; // the tag of the address held by this cache block
		long lastUsed; // what instruction number last used this block, used in LRU replacement strategy
		long assignTime; // when was this block assigned, used in oldest replacement strategy

		Cache_Block(): dirty(false), pBit(false), replace(false), address(0), tag(0) , assignTime(-1), lastUsed(-1){}

		Cache_Block(uint64_t a, uint t, long l): dirty(false), pBit(false), replace(false), address(a), tag(t), assignTime(l), lastUsed(l){}
	};

#endif