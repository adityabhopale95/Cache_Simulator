#include "cache.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <vector>
#include <math.h>
using namespace std;

struct cache_struct{
  long long tag;
  long long tag_store;
  long long index;
  unsigned dirty;
  unsigned valid;
  unsigned LRU_time_stamp;
};

cache_struct temp_cache = {UNDEFINED,UNDEFINED,UNDEFINED,0,0};

std::vector<std::vector<cache_struct> > main_cache;
cache::cache(unsigned size,
      unsigned associativity,
      unsigned line_size,
      write_policy_t wr_hit_policy,
      write_policy_t wr_miss_policy,
      unsigned hit_time,
      unsigned miss_penalty,
      unsigned address_width
){
	hit_wr_policy = wr_hit_policy;
  miss_wr_policy = wr_miss_policy;
  reads = read_misses = time_stamp = 0;
  writes = write_misses = 0;
  evictions = memory_accesses = hits = 0;
  memory_writes = 0;
  size_cache = size;
  size_block = line_size;
  associativity_cache = associativity;
  sets = ((size/line_size)/associativity);
  num_blocks = (size/line_size);
  time_hit = hit_time;
  penalty_miss = miss_penalty;
  width_add = address_width;
  num_index_bits = log2(sets);
  num_blockoff_bits = log2(size_block);
  num_tag_bits = address_width - num_index_bits - num_blockoff_bits;
  tag_mask = 0;
  type_ins = UNDEFINED;

  for(int i = 0; i < num_blockoff_bits; i++){
    get_block_off <<= 1;
    get_block_off |= 1;
  }
  for(int i = 0; i < num_index_bits; i++){
    tag_mask <<= 1;
    tag_mask |= 1;
  }

  main_cache.resize(associativity_cache);

  for(int i = 0; i < associativity_cache; i++){
    for(int j = 0; j < sets; j++){
      main_cache[i].push_back(temp_cache);
    }
  }
}

void cache::print_configuration(){
	std::cout << "CACHE CONFIGURATION" << '\n';
  std::cout << "size = " << (size_cache/1024) << " KB" << '\n';
  std::cout << "associativity = " << associativity_cache << "-way" << '\n';
  std::cout << "cache line size = " << size_block << " B" << '\n';
  std::cout << "write hit policy = ";
  if(hit_wr_policy == WRITE_BACK) std::cout << "write-back" << '\n';
  if(hit_wr_policy == WRITE_THROUGH) std::cout << "write-through" << '\n';
  std::cout << "write miss policy = ";
  if(miss_wr_policy == WRITE_ALLOCATE) std::cout << "write-allocate" << '\n';
  if(miss_wr_policy == NO_WRITE_ALLOCATE) std::cout << "no-write-allocate" << '\n';
  std::cout << "cache hit time = " << time_hit << " CLK" << '\n';
  std::cout << "cache miss penalty = " << penalty_miss << " CLK" << '\n';
  std::cout << "memory address width = " << width_add << " bits" << '\n';
}

cache::~cache(){
  reads = read_misses = time_stamp = 0;
  writes = write_misses = 0;
  evictions = memory_accesses = hits = 0;
  memory_writes = 0;
  main_cache.clear();
}

void cache::load_trace(const char *filename){
   stream.open(filename);
}

void cache::run(unsigned num_entries){

   unsigned first_access = number_memory_accesses;
   string line;
   unsigned line_nr=0;
   access_type_t read_type;
   access_type_t write_type;
   const char *instr[2] = {"r","w"};

   while (getline(stream,line)){

	line_nr++;
        char *str = const_cast<char*>(line.c_str());

        // tokenize the instruction
        char *op = strtok (str," ");
	char *addr = strtok (NULL, " ");
	address_t address = strtoul(addr, NULL, 16);

  if(strcmp(op,instr[0]) == 0){
    type_ins = 0;
    reads++;
    read_type = read(address);
    if(read_type == HIT){
      hits++;
    }
    else if(read_type == MISS){
      read_misses++;
      allocate_cache(address);
    }
  }
  else if(strcmp(op,instr[1]) == 0){
    type_ins = 1;
    writes++;
    write_type = write(address);

    if(write_type == HIT){
      hits++;
      if(hit_wr_policy == WRITE_THROUGH){
        memory_writes++;
      }
    }
    else if(write_type == MISS){
      write_misses++;
      if(miss_wr_policy == WRITE_ALLOCATE){
        allocate_cache(address);
        if(hit_wr_policy == WRITE_BACK){
          set_dirty(address);
        }
        else if(hit_wr_policy == WRITE_THROUGH){
          memory_writes++;
        }
      }
      else if(miss_wr_policy == NO_WRITE_ALLOCATE){
        memory_writes++;
      }
    }
  }
  time_stamp++;
  memory_accesses++;
	number_memory_accesses++;
	if (num_entries!=0 && (number_memory_accesses-first_access)==num_entries)
		break;
   }
}

void cache::print_statistics(){
  float miss_rate;
  float avg_mem_access_time;
  miss_rate = (float(read_misses)+float(write_misses))/(memory_accesses);
  avg_mem_access_time = float(time_hit + miss_rate * penalty_miss);
	cout << "STATISTICS" << endl;
  std::cout << "memory accesses = " << dec << memory_accesses << '\n';
  std::cout << "read = " << reads << '\n';
  std::cout << "read misses = " << read_misses << '\n';
  std::cout << "write = " << writes << '\n';
  std::cout << "write misses = " << write_misses << '\n';
  std::cout << "evictions = " << evictions << '\n';
  std::cout << "memory writes = " << dec << memory_writes << '\n';
  std::cout << "average memory access time = " << avg_mem_access_time << '\n';
}

access_type_t cache::read(address_t address){
	ulong tag,i;
  access_type_t ret;
  tag = tag_field(address);
  i = index_field(address);
  for(unsigned iter = 0; iter < associativity_cache; iter++){
    if(main_cache[iter][i].tag == tag){
      ret = HIT;
      update_LRU(iter,i);
      break;
    }
    else{
      ret = MISS;
    }
  }
	return ret;
}

access_type_t cache::write(address_t address){
  ulong tag,i;
  access_type_t ret;
  tag = tag_field(address);
  i = index_field(address);
  for(unsigned iter = 0; iter < associativity_cache; iter++){
    if(main_cache[iter][i].tag == tag){
      if(hit_wr_policy == WRITE_BACK){
        main_cache[iter][i].dirty = 1;
      }
      ret = HIT;
      update_LRU(iter,i);
      break;
    }
    else{
      ret = MISS;
    }
  }
	return ret;
}

void cache::set_dirty(address_t address){
  long tag,i;
  tag = tag_field(address);
  i = index_field(address);

  for(unsigned iter = 0; iter < associativity_cache; iter++){
    if(main_cache[iter][i].tag == tag){
      main_cache[iter][i].dirty = 1;
      break;
    }
  }
}

void cache::allocate_cache(address_t address){
  ulong tag,i,tag_store;
  unsigned if_evict = 0;
  unsigned allocate_aft_eviction;
  access_type_t ret;
  tag = tag_field(address);
  i = index_field(address);
  tag_store = get_tag(address);
  for(unsigned iter = 0; iter < associativity_cache; iter++){
    if(main_cache[iter][i].valid == 0){
      update_LRU(iter,i);
      main_cache[iter][i].valid = 1;
      main_cache[iter][i].index = i;
      main_cache[iter][i].tag_store = tag_store;
      main_cache[iter][i].tag = tag;
      main_cache[iter][i].LRU_time_stamp = time_stamp;
      if_evict = 0;
      break;
    }
    else{
      if_evict = 1;
    }
  }

  if(if_evict == 1){
    evictions++;
    allocate_aft_eviction = evict_line(i);
    update_LRU(allocate_aft_eviction,i);
    main_cache[allocate_aft_eviction][i].valid = 1;
    main_cache[allocate_aft_eviction][i].index = i;
    main_cache[allocate_aft_eviction][i].tag_store = tag_store;
    main_cache[allocate_aft_eviction][i].tag = tag;
    main_cache[allocate_aft_eviction][i].LRU_time_stamp = time_stamp;
    if_evict = 0;
  }
}

unsigned cache::evict_line(unsigned ind){
  unsigned victim;
  unsigned min = time_stamp;
  for(unsigned assoc = 0; assoc < associativity_cache; assoc++){
    if(main_cache[assoc][ind].LRU_time_stamp <= min){
      min = main_cache[assoc][ind].LRU_time_stamp;
      victim = assoc;
    }
  }
  if(main_cache[victim][ind].dirty == 1){
    main_cache[victim][ind].dirty = 0;
    memory_writes++;
  }
  main_cache[victim][ind].valid = 0;
  main_cache[victim][ind].index = UNDEFINED;
  main_cache[victim][ind].tag_store = UNDEFINED;
  main_cache[victim][ind].tag = UNDEFINED;
  main_cache[victim][ind].LRU_time_stamp = time_stamp;

  return victim;
}

void cache::update_LRU(unsigned block, unsigned set){
  main_cache[block][set].LRU_time_stamp = time_stamp;
}

void cache::print_tag_array(){
	cout << "TAG ARRAY" << endl;
	for(int i = 0; i < associativity_cache; i++){
    std::cout << "BLOCKS " << i << '\n';
    std::cout << setfill(' ') << setw(7) << "index";
    if(hit_wr_policy == WRITE_BACK)std::cout << setw(6) << "dirty";
    std::cout << setw((4+(num_tag_bits/4))) << "tag" << '\n';
    for(int j = 0; j < sets; j++){
      if(main_cache[i][j].index != UNDEFINED){std::cout << setfill(' ') << setw(7) << dec << main_cache[i][j].index;}
      if(hit_wr_policy == WRITE_BACK && main_cache[i][j].tag_store != UNDEFINED){std::cout << setw(6) << dec << main_cache[i][j].dirty;}
      if(main_cache[i][j].tag_store != UNDEFINED){std::cout << setw(4) << "0x" << hex << main_cache[i][j].tag_store << '\n';}
    }
  }
}
