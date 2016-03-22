/*
	Colin Van Oort
	CS 222: Computer Architecture
	Cache Simulation Assignment
	Base Cache object
*/

#include <stdlib.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>

#include "Cache_Block.hpp"
#include "Cache_Set.hpp"
#include "AccessReturn.hpp"

long g_addressSize = 64; // the size of the system addresses in number of bits
long g_instructNum = 0; // the current instruction being performed; used as a time keeper

class Cache{
	public:
		Cache(int size,int assoc,int blockSize, std::string replace, char cacheType, int cacheLevel, std::string writeBack, std::string writeAlloc);
		std::string getReplace();
		std::string getWriteAlloc();
		std::string getWriteBack();
		char getType();
		AccessReturn access(uint64_t address, char type);
		void printStats();
		
	private:
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

Cache::Cache(int size,int assoc,int blockSize, std::string replace, char cacheType, int cacheLevel, std::string writeBack, std::string writeAlloc): m_size(size), m_assoc(assoc), m_blockSize(blockSize), m_replace(replace), m_cacheType(cacheType), m_cacheLevel(cacheLevel), m_writeAlloc(writeAlloc), m_writeBack(writeBack){	
	m_readAccesses = 0;
	m_readMisses = 0;
	m_writeAccesses = 0;
	m_writeMisses = 0;
	m_hits = 0;
	m_misses = 0;
	m_setBits = 0;
	m_tagBits = 0;
	m_blockBits = 0;

	m_numBlocks = m_size/m_blockSize;
	m_numSets = m_size/(m_assoc*m_blockSize);

	m_numBlockBits = std::log2(m_blockSize);
	m_numSetBits = std::log2(m_numSets);
	m_numTagBits = (g_addressSize - (m_numSetBits + m_numBlockBits));

	m_cache = {};
	//initializes our cache object
	for(int i=0;i<m_numSets;i++){
		m_cache.push_back(Cache_Set(assoc));
	}
	
}

void Cache::pBitOps(Cache_Set* set, int index){
	// check to see if all of the pBits have been set, if so reset them
	int pcount=1;
	for(int j=0;j<m_assoc;j++){
		if( set->blocks.at(j).pBit ){
			pcount++;
		}
	}

	if( pcount == m_assoc ){
		for(int j=0;j<m_assoc;j++){
			set->blocks.at(j).pBit = false;
		}
	}

	// the most recently used block keeps its pBit
	set->blocks.at(index).pBit = true;
}

void Cache::decode(uint64_t address){
	// get the set bits using bit shifting; shift off the tag bits then the block offset bits, leaving the set bits
	m_setBits = address << (m_numTagBits);
	m_setBits = m_setBits >> (m_numTagBits + m_numBlockBits);

	// shift off the set bits and block offset bits, leaving the tag bits
	m_tagBits = address >> (m_numSetBits + m_numBlockBits);

	// shift off the tag and set bits, leaving the offset bits. Shift the offset bits back into the correct position
	m_blockBits = address << (m_numTagBits + m_numSetBits);
	m_blockBits = m_blockBits >> (m_numTagBits + m_numSetBits);

	// no need to return anything since they are stored in class variables

}

AccessReturn Cache::read(uint64_t address){
	decode(address);
	
	// create a pointer to the set that we're going to be working in
	Cache_Set* set = &(m_cache[m_setBits]);
	
	// linear search the set for a block with the same tag as our address
	bool found=false;
	for(int i=0;i<m_assoc;i++){

		// if the block is valid and the tag matches the tag we're looking for
		if( (set->blocks.at(i).assignTime != -1) && (set->blocks.at(i).tag == m_tagBits) ){
			found = true;
			m_hits++;

			// update the block to show that we just used it
			set->blocks.at(i).lastUsed = g_instructNum;
			
			// do pBit ops
			pBitOps(set, i);

			// we didn't miss, return that information and the block we just read 
			return AccessReturn(false, set->blocks.at(i));
		}
	}

	// if we failed to find the desired block, get it and put it in this level
	if(found == false){
		m_misses++;
		m_readMisses++;
		Cache_Block oldBlock = replace(address);

		// we missed, so return that information and the block that we replaced (used for write strategy and now we need to check the next level)
		return AccessReturn(true, oldBlock);

	}
}

AccessReturn Cache::write(uint64_t address){
	decode(address);
	
	// create a pointer to the set that we're going to be working in
	Cache_Set* set = &(m_cache[m_setBits]);
	
	bool found=false;
	
	// linear search the set for a block with the same tag as our address
	for(int i=0;i<m_assoc;i++){

		// if a block is valid and its tag matches the tag we're looking for
		if( (set->blocks.at(i).assignTime != -1) && (set->blocks.at(i).tag == m_tagBits) ){
			found = true;
			m_hits++;
			
			// if we're using the writeback write strategy then we need to set the dirty bit when we modify data
			if( (m_writeBack == "yes") && (m_cacheLevel == 1) ){
				set->blocks.at(i).dirty = true;
			}

			// if we're using pseudo-LRU then update the pBits
			if( m_replace == "pseudo-LRU"){
				pBitOps(set, i);
			}

			// write through is handled by the driver

			// update the block to show that we used it and the set to show the new most recently used block
			set->blocks.at(i).lastUsed = g_instructNum;
			
			// if write alloc is enabled then we will update the data
			if( m_writeAlloc == "yes" ){
				set->blocks.at(i).address = address;
			}
			
			
			return AccessReturn(false, set->blocks.at(i));
		}
	}

	// if we failed to find it, the behavior is determined by the write strategy
	if(found == 0){
		m_misses++;
		m_writeMisses++;
		Cache_Block oldBlock = Cache_Block();

		if(m_writeAlloc == "no"){
			// no write alloc means we go to the next level and modify the value if we find it there, handled in the simulation driver
			oldBlock = Cache_Block();
		}

		else{
			// write alloc enabled means we modify the value in this cache level even if it is a miss, then depending on write strategy the lower levels of cache may be modified
			oldBlock = replace(address);
		}

		return AccessReturn(true, oldBlock);

	}
}




Cache_Block Cache::replace(uint64_t address){
	decode(address);

	// create a pointer to the set that we're going to be working in
	Cache_Set* set = &(m_cache[m_setBits]);

	// if there is an empty block in the set then we don't need to replace anything
	if( (set->available != -1) && (set->available<m_assoc) ){
		// drop the new block into the empty space
		Cache_Block newBlock = Cache_Block(address, m_tagBits, g_instructNum);
		set->blocks.at(set->available) = newBlock;
		
		// look for a new empty space
		bool emptySpace=false;
		for(int i=0; i<m_assoc; i++){
			if( set->blocks.at(i).assignTime == -1 ){
				set->available = i;
				emptySpace == true;
			}

		}
		// if we don't find any empty spaces then set the available to -1
		if( !emptySpace ){
			set->available = -1;
		}

		return newBlock;
	}

	else if( m_replace == "pseudo-LRU" ){
		// find a block whose pBit is 0
		int replaceIndex=0;
		while( set->blocks.at(replaceIndex).pBit ){
			replaceIndex++;
		}

		// replace that block 
		Cache_Block oldBlock = set->blocks.at(replaceIndex);
		oldBlock.replace = true;
		set->blocks.at(replaceIndex) =  Cache_Block(address, m_tagBits, g_instructNum);

		// perform pBit operations
		pBitOps(set, replaceIndex);

		// return the old block incase we want to do writeback
		return oldBlock;
	}
	
	else if( m_replace == "random" ){
		// Choose a block at random
		srand(time(NULL));
		int randIndex = rand() % m_assoc;

		// Perform the replacement
		Cache_Block oldBlock = set->blocks.at(randIndex);
		oldBlock.replace = true;
		set->blocks.at(randIndex) =  Cache_Block(address, m_tagBits, g_instructNum);

		// return the old block incase we want to do writeback
		return oldBlock;
	}
	
	else if( m_replace == "LRU" ){
		// Find the least recently used block
		int lru = set->blocks.at(0).lastUsed;
		int lruIndex = 0;
		for(int i=1;i<m_assoc;i++){
			if( set->blocks.at(i).lastUsed < lru ){
				lru = set->blocks.at(0).lastUsed;
				lruIndex = i;
			}
		}

		// Perform the replacement
		Cache_Block oldBlock = set->blocks.at(lruIndex);
		oldBlock.replace = true;
		set->blocks.at(lruIndex) =  Cache_Block(address, m_tagBits, g_instructNum);

		// return the old block incase we want to do writeback
		return oldBlock;
	}
	
	else if(m_replace == "oldest"){
		// Find the oldest block in the set
		int oldest = set->blocks.at(0).assignTime;
		int oldestIndex = 0;
		for(int i=1;i<m_assoc;i++){
			if( set->blocks.at(i).assignTime < oldest ){
				oldest = set->blocks.at(i).assignTime;
				oldestIndex = i;
			}
		}

		// Perform the replacement
		Cache_Block oldBlock = set->blocks.at(oldestIndex);
		oldBlock.replace = true;
		set->blocks.at(oldestIndex) =  Cache_Block(address, m_tagBits, g_instructNum);

		// return the old block incase we want to do writeback
		return oldBlock;
	}

	else{
		// error: invalid replacement strategy
		std::cout<<"Error: (Cache::replace) Invalid replacement strategy";
	}
}

AccessReturn Cache::access(uint64_t address, char type){
	AccessReturn tuple = AccessReturn(true, Cache_Block());
	if( type == 'R' ){
		// instruction type is r, make sure the cache type is c or d
		if( m_cacheType != 'i' ){
			tuple = read(address);
			m_readAccesses++;
		}
	}

	else if( type == 'W' ){
		// instruction type is w, make sure the cache type is c or d
		if( m_cacheType != 'i' ){
			tuple = write(address);
			m_writeAccesses++;
		}
	}

	else if( type == 'I' ){
		// instruction type is i, make sure the cache type is i
		if( m_cacheType == 'i' ){
			// instruction loads are functionally the same as reads, so we don't need any new code here
			tuple = read(address);
			m_readAccesses++;
		}
	} 

	else{
		// error: Invalid access type
		std::cout<<"Error: (Cache::access) Invalid access type";
	}

	return tuple;
}

void Cache::printStats(){
	std::cout<< "L" << m_cacheLevel << " " << m_cacheType << ": " << std::endl;
	std::cout<< "	Reads: " << m_readAccesses << std::endl;
	std::cout<< "	Writes: " << m_writeAccesses << std::endl;
	std::cout<< "	Accesses: " << (m_hits + m_misses) << std::endl;
	std::cout<< "	Misses: " << m_misses << "(Write: " << m_writeMisses << " Read: " << m_readMisses << " )"  << std::endl;
	std::cout<< "	Miss Rate: " << (static_cast<double>(m_misses) / (m_hits + m_misses))*100 << "% "  << "(Write: " << ((static_cast<double>(m_writeMisses)/m_writeAccesses)*100) << "% Read: "<< (static_cast<double>(m_readMisses)/m_readAccesses)*100 << "% )\n" << std::endl;
}

char Cache::getType(){
	return m_cacheType;
}

std::string Cache::getReplace(){
	return m_replace;
}

std::string Cache::getWriteBack(){
	return m_writeBack;
}

std::string Cache::getWriteAlloc(){
	return m_writeAlloc;
}