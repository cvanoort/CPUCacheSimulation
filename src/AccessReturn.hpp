/*
	Colin Van Oort
	CS 222: Computer Architecture
	Cache Simulation Assignment
	Access Return Struct

	Datastructure used to return access information, used to implement interactions between cache levels
*/
#ifndef ACCESS_RETURN_HPP
#define ACCESS_RETURN_HPP
	
#include "Cache_Block.hpp"

	struct AccessReturn{
		Cache_Block block;
		bool miss;

		AccessReturn(bool m, Cache_Block b): miss(m), block(b){}
	};

#endif