#include "cache.h"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <string.h>

using namespace std;

#define UNDEFINED 0xFFFFFFFFFFFFFFFF
#define START 0x0

vector <vector <cache_entry_t>> cache_table;

cache::cache(unsigned size,
             unsigned associativity,
             unsigned line_size,
             write_policy_t wr_hit_policy,
             write_policy_t wr_miss_policy,
             unsigned hit_time,
             unsigned miss_penalty,
             unsigned address_width
){
    cache_table.clear();
    size_cache = size;
    way = associativity;
    block_size = line_size;
    wr_hit = wr_hit_policy;
    wr_miss = wr_miss_policy;
    hittime = hit_time;
    misspenalty = miss_penalty;
    addr_width = address_width;
    set = (size/line_size)/associativity;
    blockoffset_bits = log2(line_size);
    index_bits = log2(set);
    tag_bits = address_width - index_bits - blockoffset_bits;
    num_read = START;
    num_write = START;
    num_read_miss = START;
    num_write_miss = START;
    num_eviction = START;
    num_ins = START;
    num_hit = START;
    num_mem_write = START;
    number_memory_accesses = START;


    cache_table.resize(associativity);
    for(unsigned i = START; i < associativity; i++){
        cache_table[i].resize(set);
    }

    for(unsigned i = START; i < blockoffset_bits; i++){
        blockoffset_cv <<= 1;
        blockoffset_cv |= 1;
    }
    for(unsigned i = START; i < index_bits; i++){
        index_cv <<= 1;
        index_cv |= 1;
    }
    for(unsigned i = START; i < associativity; i++) {
        for(unsigned j = START; j < blockoffset_bits; j++){
            cache_table[i][j].valid = START;
            cache_table[i][j].dirty = START;
            cache_table[i][j].tag = START;
            cache_table[i][j].index = START;
            cache_table[i][j].access_record = START;
        }
    }
}

void cache::print_configuration() {

    cout << "CACHE CONFIGURATION" <<endl;
    cout << "size = " << (size_cache/1024) << " KB" <<endl;
    cout << "associativity = " << way << "-way" <<endl;
    cout << "cache line size = " << block_size << " B" <<endl;
    cout << "write hit policy = ";

    if(wr_hit == WRITE_BACK){
        cout << "write-back" <<endl;
    }else{
        cout << "write-through" <<endl;
    }

    cout << "write miss policy = ";

    if(wr_miss == WRITE_ALLOCATE){
        cout << "write-allocate" <<endl;
    }else{
        cout << "no-write-allocate" <<endl;
    }

    cout << "cache hit time = " << hittime << " CLK" <<endl;
    cout << "cache miss penalty = " << misspenalty << " CLK" <<endl;
    cout << "memory address width = " << addr_width << " bits" <<endl;
}

cache::~cache(){
    num_read = START;
    num_write = START;
    num_read_miss = START;
    num_write_miss = START;
    num_eviction = START;
    num_ins = START;
    num_hit = START;
    number_memory_accesses = START;
    cache_table.clear();
}

void cache::load_trace(const char *filename){
    stream.open(filename);
}

void cache::run(unsigned num_entries){
    unsigned first_access = number_memory_accesses;
    string line;
    unsigned line_nr=START;
    long long index;
    long long x;

    while (getline(stream,line)){
        line_nr++;
        char *str = const_cast<char*>(line.c_str());
        char *op = strtok (str," ");
        char *addr = strtok (NULL, " ");
        address_t address = strtoull(addr, NULL, 16);

        if (!strcmp(op, "w")) {
            num_write ++;
            if (write(address) == MISS) {
                num_write_miss ++;
                if(wr_miss == WRITE_ALLOCATE){
                    cache_way = allocate(address);
                    if(wr_hit == WRITE_THROUGH){
                        num_mem_write++;
                    }else{
                        index = (address >> blockoffset_bits) & index_cv;
                        x = index % set;
                        cache_table[cache_way][x].dirty = 1;
                    }
                }else{
                    num_mem_write ++;
                }
            }else{
                num_hit ++;
                if (wr_hit == WRITE_THROUGH){
                    num_mem_write ++;
                }
            }
        }
        else if(!strcmp(op, "r")){
            num_read ++;
            if(read(address) == MISS) {
                num_read_miss ++;
                cache_way = allocate(address);
            }else{
                num_hit ++;
            }
        }

        num_ins ++;
        number_memory_accesses++;
        if ((num_entries!=0) && (number_memory_accesses - first_access) == num_entries) break;
    }
}



void cache::print_statistics() {
    cout << "STATISTICS" << endl;
    cout << "memory accesses = " << dec << number_memory_accesses << endl;
    cout << "read = " << num_read << endl;
    cout << "read misses = " << num_read_miss << endl;
    cout << "write = " << num_write << endl;
    cout << "write misses = " << num_write_miss << endl;
    cout << "evictions = " << num_eviction << endl;
    cout << "memory writes = " << dec << num_mem_write << endl;
    cout << "average memory access time = " << float(misspenalty * ((float(num_read_miss) + float(num_write_miss)) / (number_memory_accesses)) + hittime) << endl;
}


access_type_t cache::read(address_t address){
    long long index;
    long long tag;
    long long x;

    index = (address >> blockoffset_bits) & index_cv;
    x = index % set;
    tag = address >> (blockoffset_bits + index_bits);


    for (unsigned i = START; i < way; i++) {
        if ((cache_table[i][x].valid) && (cache_table[i][x].tag == tag)) {
            cache_table[i][x].access_record = num_ins;
            return HIT;
        }
    }
    return MISS;
}

access_type_t cache::write(address_t address){
    long long index;
    long long tag;
    long long x;

    index = (address >> blockoffset_bits) & index_cv;
    x = index % set;
    tag = address >> (blockoffset_bits + index_bits);

    for (unsigned i = START; i < way; i++) {
        if ((cache_table[i][x].valid) && (cache_table[i][x].tag == tag)) {
            if(wr_hit == WRITE_BACK){
                cache_table[i][x].dirty = 1;
            }
            cache_table[i][x].access_record = num_ins;
            return HIT;
        }
    }
    return MISS;
}

void cache::print_tag_array(){

    cout << "TAG ARRAY" << endl;
    for (unsigned i = START; i < way; i++) {
        cout << "BLOCKS " << i << endl;

        if (wr_hit != WRITE_BACK) {
            cout << setfill(' ') << setw(7) << "index" << setw(6) << setw(4 + tag_bits/4) << "tag" <<endl;
            for (unsigned j = START; j < set; j++) {
                if (cache_table[i][j].valid == 1) {
                    cout << setfill(' ') << setw(7) << dec << cache_table[i][j].index << setw(4) << "0x" << hex << cache_table[i][j].tag <<endl;
                }
            }

        }else{
            cout << setfill(' ') << setw(7) << "index" << setw(6) << "dirty" << setw(4 + tag_bits/4) << "tag" <<endl;
            for (unsigned k = START; k < set; k++) {
                if (cache_table[i][k].valid == 1) {
                    cout << setfill(' ') << setw(7) << dec << cache_table[i][k].index << setw(6) << dec << cache_table[i][k].dirty << setw(4) << "0x" << hex << cache_table[i][k].tag <<endl;
                }
            }
        }
    }
}

unsigned cache::evict(long long index) {
    unsigned lru_way = START;
    unsigned record = num_ins;
    for (unsigned i = START; i < way; i++) {
        if (record >= cache_table[i][index].access_record) {
            record = cache_table[i][index].access_record;
            lru_way = i;
        }
    }
    return lru_way;
}

unsigned cache::allocate(address_t address) {
    long long index;
    long long tag;
    long long j;

    index = (address >> blockoffset_bits) & index_cv;
    j = index % set;
    tag = address >> (index_bits+blockoffset_bits);


    for (unsigned i = START; i < way; i++) {
        if (cache_table[i][j].valid == START) {
            cache_table[i][j].access_record = num_ins;
            cache_table[i][j].tag = tag;
            cache_table[i][j].valid = 1;
            cache_table[i][j].index = index;
            return i;
        }
    }
    num_eviction ++;

    unsigned evict_way = evict(j);

    if (cache_table[evict_way][j].dirty == 1) {
        cache_table[evict_way][j].dirty = START;
        num_mem_write ++;
    }

    cache_table[evict_way][j].index = index;
    cache_table[evict_way][j].access_record = num_ins;
    cache_table[evict_way][j].tag = tag;

    return evict_way;
}

