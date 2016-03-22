/*
	Colin Van Oort
	CS 222: Computer Architecture
	Cache Simulation Assignment
	Simulation Driver
*/

#include <inttypes.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Cache_Block.hpp"
#include "Cache_Set.hpp"
#include "AccessReturn.hpp"
#include "Cache.hpp"


// global variables, address size is used in decoding; instructNum is used as a time keeper
 // the number of the current instruction being processed


int main(){
	std::cout << "BEGIN SIMULATION**********************************************************" << std::endl;
	// create file streams from the config and trace files
	std::string config = "Configs/base.config";
	std::cout << "Config File: " << config << std::endl;
	std::ifstream configFile( config );
	bool configOpen = false;

	std::string trace = "Traces/gcc-addrs-10M.trace";
	std::cout << "Trace File: " << trace << std::endl;
	std::ifstream traceFile( trace );
	bool traceOpen = false;

	// check to see if the config file opened correctly
	if( !configFile.is_open() ){
		std::cout<< "ERROR: Failed to open config file!" << std::endl;
	}
	else{
		std::cout<< "Config file opened successfully" << std::endl;
		configOpen = true;
	}

	// check to see if the trace file opened correctly
	if( !traceFile.is_open() ){
		std::cout<< "ERROR: Failed to open trace file!" << std::endl;
	}
	else{
		std::cout<< "Trace file opened successfully" << std::endl;
		traceOpen = true;
	}

	std::string line;
	std::vector<Cache> cache = {};

	// some variables that are used to construct our cache levels
	int size,assoc,blockSize,cacheLevel;
	char cacheType;
	std::string replace,writeBack,writeAlloc,skip;

	// parse the config file so we can build our cache
	while( std::getline(configFile, line) ){
		// read in the first line of the options for this cache object
		std::istringstream iss(line);
		// Assign the cache type, level, and size from this first line
		iss >> cacheType >> cacheLevel >> skip >> size;

		// the remaining 5 lines are repetitive so we only need to store the last token in each line
		for(int i=1;i<6;i++){
			std::getline(configFile, line);
			std::istringstream iss(line);
			if(i==1){
				iss >> skip >> skip >> skip >> blockSize;
			}
			else if(i==2){
				iss >> skip >> skip >> skip >> assoc;
			}
			else if(i==3){
				iss >> skip >> skip >> skip >> replace;
			}
			else if(i==4){
				iss >> skip >> skip >> skip >> writeBack;
			}
			else{
				iss >> skip >> skip >> skip >> writeAlloc;
			}


		}
		std::cout<< "Building Cache Object: L" << cacheLevel << " " << cacheType << " " << size << " bytes, " << blockSize << " byte blocks, " << assoc << " ways, " <<  " Replace = " << replace << ", Write Back = " << writeBack << ", Write Allocate = " << writeAlloc << std::endl;

		Cache c = Cache(size,assoc,blockSize,replace,cacheType,cacheLevel,writeBack,writeAlloc);
		cache.push_back(c);
	} // end config parsing
	// we're done with the config so we can close it up now
	configFile.close();


	// variables that will be used in parsing the trace file
	char type;
	std::string addressString;
	uint64_t address;

	// this is the main loop where we parse the trace file 
	while( std::getline(traceFile,line) ){
		// grab a line from the trace file
		std::istringstream iss(line);
		
		// type is assigned the instruction type from the trace, while address gets the address bits as a string
		iss >> type >> addressString;

		// the address as a hex value
		std::stringstream ss;
		ss<< std::hex << addressString;
		ss >> address;

		// variables used in the cache accesses
		AccessReturn tuple = AccessReturn(true, Cache_Block());
		int cacheIndex=0;

		// perform the access
		while( cacheIndex < cache.size() ){
			// I type accesses don't access d type caches
			if( type == 'I' && cache[cacheIndex].getType() != 'd' && cacheIndex <= cache.size() ){
				tuple = cache[cacheIndex].access(address, type);
			}

			// R and W type accesses don't access i type caches
			if( type != 'I' &&  cache[cacheIndex].getType() != 'i' && cacheIndex <= cache.size() ){
				tuple = cache[cacheIndex].access(address, type);
			}

			// Do write back if required
			if( type == 'W' && cache[cacheIndex].getWriteBack() == "yes" && tuple.block.dirty && tuple.block.replace){
				// find the next cache level
				int cacheOffset = 1;
				while( ((cacheOffset + cacheIndex) < cache.size()) && cache[cacheIndex + cacheOffset].getType() != cache[cacheIndex].getType() ){
					cacheOffset++;
				}

				// if we haven't gone past the lowest level of cache, do an access with the block address that needs to be written back
				if( ((cacheOffset + cacheIndex) < cache.size()) && cache[cacheIndex + cacheOffset].getType() != cache[cacheIndex].getType() ){
					tuple = cache[cacheIndex+cacheOffset].access(tuple.block.address, type);
				}
			}

			// Do write through if required
			if( type == 'W' && cache[cacheIndex].getWriteBack() == "no" && !tuple.miss ){
				// find the next cache level
				int cacheOffset = 1;
				while( ((cacheOffset + cacheIndex) < cache.size()) && cache[cacheIndex + cacheOffset].getType() != cache[cacheIndex].getType() ){
					cacheOffset++;
				}

				// if we haven't gone past the lowest level of cache, do an access with the block address that needs to be written back
				if( (cacheIndex+cacheOffset) < cache.size()){
					tuple = cache[cacheIndex+cacheOffset].access(tuple.block.address, type);
				}

			}

			// Do No Write Allocate
			if( type == 'W' && tuple.miss && cache[cacheIndex].getWriteAlloc() == "no" ){
				// find the next cache level
				int cacheOffset = 1;
				while( ((cacheOffset + cacheIndex) < cache.size()) && cache[cacheIndex + cacheOffset].getType() != cache[cacheIndex].getType() ){
					cacheOffset++;
				}

				// if we haven't gone past the lowest level of cache, do an access with the block address that needs to be written back
				if( (cacheIndex+cacheOffset) < cache.size()){
					tuple = cache[cacheIndex+cacheOffset].access(tuple.block.address, type);
				}
			}

			cacheIndex++;
		} // end while_C
		g_instructNum++;
	} // end while_B

	for( int i=0;i<cache.size();i++ ){
		cache[i].printStats();
	}
	std::cout<< "END SIMULATION************************************************************"<< std::endl;
	
} // end main