/*
	Colin Van Oort
	CS 222: Computer Architecture
	Cache Simulation Assignment
	Cache Set struct Header file

	A group of cache blocks, has size equalt to the number of ways in the cache
*/
#ifndef CACHE_SET_HPP
#define CACHE_SET_HPP

#include <vector>
#include "Cache_Block.hpp"

	struct Cache_Set{
		int available; // the index of the first empty block in this set
		std::vector<Cache_Block> blocks; // the blocks in the set

		Cache_Set(): available(-1){
			blocks={};
		}

		Cache_Set(int assoc): available(0){
			blocks={};
			for(int i=0;i<assoc;i++){
				blocks.push_back(Cache_Block());
			}
		}
	};

#endif