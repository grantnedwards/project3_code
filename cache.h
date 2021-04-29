#ifndef CACHE_H_
#define CACHE_H_

#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

#define UNDEFINED 0xFFFFFFFFFFFFFFFF //constant used for initialization
#define START 0x0

typedef enum {WRITE_BACK, WRITE_THROUGH, WRITE_ALLOCATE, NO_WRITE_ALLOCATE} write_policy_t;

typedef enum {HIT, MISS} access_type_t;

typedef long long address_t; //memory address type

typedef struct{
    unsigned valid; //valid bit
    unsigned dirty; //dirty bit
    long long tag; //tag
    long long index; //index
    unsigned access_record; //record access time, for LRU
} cache_entry_t;

class cache{

    /* Add the data members required by your simulator's implementation here */
    unsigned num_read, num_write;
    unsigned num_read_miss, num_write_miss;
    unsigned num_eviction;
    unsigned num_ins; // access record
    unsigned num_hit;
    unsigned num_mem_write;
    unsigned size_cache;
    unsigned way;
    unsigned set;
    unsigned block_size;
    write_policy_t wr_hit;
    write_policy_t wr_miss;
    unsigned hittime;
    unsigned misspenalty;
    unsigned addr_width;
    unsigned tag_bits;
    unsigned index_bits;
    unsigned blockoffset_bits;
    unsigned blockoffset_cv;
    unsigned index_cv;
    unsigned cache_way;


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

    // de-allocates the cache simulator
    ~cache();

    // loads the trace file (with name "filename") so that it can be used by the "run" function
    void load_trace(const char *filename);

    // processes "num_memory_accesses" memory accesses (i.e., entries) from the input trace
    // if "num_memory_accesses=0" (default), then it processes the trace to completion
    void run(unsigned num_memory_accesses=0);

    unsigned cache_allo(address_t address);

    //unsigned LRU(long long index);

    void write_back(unsigned i, address_t address);

    // processes a read operation and returns hit/miss
    access_type_t read(address_t address);

    // processes a write operation and returns hit/miss
    access_type_t write(address_t address);

    // returns the next block to be evicted from the cache
    unsigned evict(long long index);

    // prints the cache configuration
    void print_configuration();

    // prints the execution statistics
    void print_statistics();

    //prints the metadata information (including "dirty" but, when applicable) for all valid cache entries
    void print_tag_array();

    unsigned int allocate(address_t address);
};

#endif /*CACHE_H_*/