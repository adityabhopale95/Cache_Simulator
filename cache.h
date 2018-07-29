#ifndef CACHE_H_
#define CACHE_H_

#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

#define UNDEFINED 0xFFFFFFFFFFFFFFFF //constant used for initialization

typedef enum {WRITE_BACK, WRITE_THROUGH, WRITE_ALLOCATE, NO_WRITE_ALLOCATE} write_policy_t;

typedef enum {HIT, MISS} access_type_t;

typedef long long address_t; //memory address type

class cache{

	/* Add the data members required by your simulator's implementation here */

	/* number of memory accesses processed */
	unsigned number_memory_accesses;

	/* trace file input stream */
	ifstream stream;


public:
	/*
	* Instantiates the cache simulator
        */
	cache(unsigned cache_size, 		// cache size (in bytes)
              unsigned cache_associativity,     // cache associativity (fully-associative caches not considered)
	      unsigned cache_line_size,         // cache block size (in bytes)
	      write_policy_t write_hit_policy,  // write-back or write-through
	      write_policy_t write_miss_policy, // write-allocate or no-write-allocate
	      unsigned cache_hit_time,		// cache hit time (in clock cycles)
	      unsigned cache_miss_penalty,	// cache miss penalty (in clock cycles)
	      unsigned address_width            // number of bits in memory address
	);
	unsigned reads, read_misses, writes, write_misses, memory_accesses, evictions;
	unsigned type_ins;
	unsigned time_hit;
	unsigned penalty_miss;
	unsigned width_add;
	unsigned memory_writes, hits;
	unsigned time_stamp;
	unsigned size_cache;
	unsigned size_block;
	unsigned associativity_cache;
	unsigned sets;
	unsigned num_blocks;
	unsigned num_index_bits, num_blockoff_bits, num_tag_bits;
	write_policy_t hit_wr_policy;
	write_policy_t miss_wr_policy;
	unsigned tag_mask;
	unsigned get_block_off;

	long long get_tag(address_t addr) {return(addr >> (num_blockoff_bits+num_index_bits));}
	long long tag_field(address_t addr) {return(addr >> (num_blockoff_bits));}
	long long index_field(address_t addr) {return((addr >> num_blockoff_bits) & tag_mask);}
	long long block_offset_field(address_t addr) {return (addr & get_block_off);}
	// de-allocates the cache simulator
	~cache();

	// loads the trace file (with name "filename") so that it can be used by the "run" function
	void load_trace(const char *filename);

	// processes "num_memory_accesses" memory accesses (i.e., entries) from the input trace
	// if "num_memory_accesses=0" (default), then it processes the trace to completion
	void run(unsigned num_memory_accesses=0);

	// processes a read operation and returns hit/miss
	access_type_t read(address_t address);

	// processes a write operation and returns hit/miss
	access_type_t write(address_t address);

	// returns the next block to be evicted from the cache
	unsigned evict_line(unsigned index);

	void allocate_cache(address_t address);

	void set_dirty(address_t address);

	void update_LRU(unsigned block, unsigned set);

	// prints the cache configuration
	void print_configuration();

	// prints the execution statistics
	void print_statistics();

	//prints the metadata information (including "dirty" but, when applicable) for all valid cache entries
	void print_tag_array();
};


#endif /*CACHE_H_*/
