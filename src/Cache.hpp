/*
	Colin Van Oort
	CS 222: Computer Architecture
	Cache Simulation Assignment
	Base Cache object header file

	The main datastructure used by the simulation, holds all relevant cache information, collects statistics during runtime.
*/
#ifndef CACHE_CACHE_HPP
#define CACHE_CACHE_HPP

#include <inttypes.h>
#include <string>
#include <vector>
#include "Cache_Block.hpp"
#include "Cache_Set.hpp"
#include "AccessReturn.hpp"

	// Address size used in instruction decoding, instruct num used as a time keeper
	extern long g_addressSize;
	extern long g_instructNum;

	class Cache{
		public:
			// Constructors ********************************************************************
			Cache(int size,int assoc,int blockSize, std::string replace, char cacheType, int cacheLevel, std::string writeBack, std::string writeAlloc);
			
			// Getters *************************************************************************
			std::string getReplace(); // returns the replacement strategy of the cache
			std::string getWriteBack(); // returns the write back setting
			std::string getWriteAlloc(); // returns the write allocate seting
			char getType(); // returns the cache type {i,d, or c}

			// Other Methods *******************************************************************
			AccessReturn access(uint64_t address, char type); // the general cache access function, uses an address and an instruction type to perform cache reads, cache writes, and instruction loads
			void printStats(); // bundle function, prints all the relevant statistics gathered by a given cache object

		private:
			// Private methods used in the internal workings of the cache
			Cache_Block replace(uint64_t address); // replaces the block at the address, behavior based on the replacement strategy; returns the block that was replaced
			AccessReturn write(uint64_t address); // the cache write function, can only be applied to d or c type caches. Uses the write strategies given at cache construction
			AccessReturn read(uint64_t address); // the cache read function, behavior is based off of the replacement strategy given at cache construction
			void decode(uint64_t address); // decodes a given address, separates out the tag,set, and block offset bits and places them into class variables for reuse
			void pBitOps(Cache_Set* set, int index); // sets the pBit of the block at the given index to true then resets the pbits of the other blocks if needed

			// Some variables to hold basic cache information
			int m_size; // cache size in bytes
			int m_assoc; // set associativity of the cache
			int m_blockSize; // size of the cache blocks in bytes
			int m_numBlocks; // number of blocks in cache
			int m_numSets; // number of sets in cache
			int m_cacheLevel; // L1,L2,L3,etc.
			char m_cacheType; // i, d, or c

			// variables used in address decoding
			int m_numSetBits; // number of set bits in a given address
			int m_numTagBits; // number of tag bits in a given address
			int m_numBlockBits; // number of block bits in a given address

			// variables that hold decoded address bits
			uint64_t m_setBits; // the decoded set bits of a given address, stored for convenience
			uint64_t m_tagBits; // the decoded tag bits
			uint64_t m_blockBits; // the decoded block offset bits

			// variables that track the write strategies and replacement strategies of the cache
			std::string m_writeBack; // use write back? {yes, no}
			std::string m_writeAlloc; // use write alloc? {yes, no}
			std::string m_replace; // the replacement strategy (oldest,Random, LRU, pseudo-LRU)

			// Here's the cache
			std::vector<Cache_Set> m_cache; // a vector of the sets in cache, each set contains assoc blocks

			// variables to hold useful statistics for the simulation
			long m_writeAccesses; // number of writes
			long m_writeMisses; // number of write misses
			long m_readAccesses; // number of reads
			long m_readMisses; // number of read misses
			long m_hits; // number of hits
			long m_misses; // number of misses
		};

#endif