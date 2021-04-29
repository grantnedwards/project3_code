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

    cache_size = size;
    coeval = associativity;
    lsize = line_size;
    hit = wr_hit_policy;
    miss = wr_miss_policy;
    hitT = hit_time;
    penalty = miss_penalty;
    width = address_width;

    sized = (size/line_size)/associativity;
    offset = log2(line_size);
    indexes = log2(sized);
    tags = address_width - indexes - offset;

    reads = START;
    rd_miss = START;
    writes = START;
    wr_miss = START;

    eviction = START;
    access = START;
    hits = START;
    memory = START;

    number_memory_accesses = START;
    cache_table.clear();


    cache_table.resize(associativity);
    for(unsigned i = START; i < associativity; i++){
        cache_table[i].resize(sized);
    }

    for(unsigned i = START; i < indexes; i++){
        index_shift <<= 1;
        index_shift |= 1;
    }
    for(unsigned i = START; i < associativity; i++) {
        for(unsigned j = START; j < offset; j++){
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
    cout << "size = " << (cache_size/1024) << " KB" <<endl;
    cout << "associativity = " << coeval << "-way" <<endl;
    cout << "cache line size = " << lsize << " B" <<endl;
    cout << "write hit policy = ";

    if(hit == WRITE_BACK){
        cout << "write-back" <<endl;
    }else{
        cout << "write-through" <<endl;
    }

    cout << "write miss policy = ";

    if(miss == WRITE_ALLOCATE){
        cout << "write-allocate" <<endl;
    }else{
        cout << "no-write-allocate" <<endl;
    }

    cout << "cache hit time = " << hitT << " CLK" <<endl;
    cout << "cache miss penalty = " << penalty << " CLK" <<endl;
    cout << "memory address width = " << width << " bits" <<endl;
}

cache::~cache(){
    reads = START;
    writes = START;
    rd_miss = START;
    wr_miss = START;
    eviction = START;
    access = START;
    hits = START;
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
            writes++;
            if (write(address) == MISS) {
                wr_miss++;
                if(miss == WRITE_ALLOCATE){
                    path = allocate(address);
                    if(hit == WRITE_THROUGH){
                        memory++;
                    }else{
                        index = (address >> offset) & index_shift;
                        x = index % sized;
                        cache_table[path][x].dirty = 1;
                    }
                }else{
                    memory++;
                }
            }else{
                hits++;
                if (hit == WRITE_THROUGH){
                    memory++;
                }
            }
        }
        else if(!strcmp(op, "r")){
            reads++;
            if(read(address) == MISS) {
                rd_miss++;
                path = allocate(address);
            }else{
                hits++;
            }
        }

        access++;
        number_memory_accesses++;
        if ((num_entries!=0) && (number_memory_accesses - first_access) == num_entries) break;
    }
}



void cache::print_statistics() {
    cout << "STATISTICS" << endl;
    cout << "memory accesses = " << dec << number_memory_accesses << endl;
    cout << "read = " << reads << endl;
    cout << "read misses = " << rd_miss << endl;
    cout << "write = " << writes << endl;
    cout << "write misses = " << wr_miss << endl;
    cout << "evictions = " << eviction << endl;
    cout << "memory writes = " << dec << memory << endl;
    cout << "average memory access time = " << float(penalty * ((float(rd_miss) + float(wr_miss)) / (number_memory_accesses)) + hitT) << endl;
}


access_type_t cache::read(address_t address){
    long long index;
    long long tag;
    long long x;

    index = (address >> offset) & index_shift;
    x = index % sized;
    tag = address >> (offset + indexes);


    for (unsigned i = START; i < coeval; i++) {
        if ((cache_table[i][x].valid) && (cache_table[i][x].tag == tag)) {
            cache_table[i][x].access_record = access;
            return HIT;
        }
    }
    return MISS;
}

access_type_t cache::write(address_t address){
    long long index;
    long long tag;
    long long x;

    index = (address >> offset) & index_shift;
    x = index % sized;
    tag = address >> (offset + indexes);

    for (unsigned i = START; i < coeval; i++) {
        if ((cache_table[i][x].valid) && (cache_table[i][x].tag == tag)) {
            if(hit == WRITE_BACK){
                cache_table[i][x].dirty = 1;
            }
            cache_table[i][x].access_record = access;
            return HIT;
        }
    }
    return MISS;
}

void cache::print_tag_array(){

    cout << "TAG ARRAY" << endl;
    for (unsigned i = START; i < coeval; i++) {
        cout << "BLOCKS " << i << endl;

        if (hit != WRITE_BACK) {
            cout << setfill(' ') << setw(7) << "index" << setw(6) << setw(4 + tags/4) << "tag" <<endl;
            for (unsigned j = START; j < sized; j++) {
                if (cache_table[i][j].valid == 1) {
                    cout << setfill(' ') << setw(7) << dec << cache_table[i][j].index << setw(4) << "0x" << hex << cache_table[i][j].tag <<endl;
                }
            }

        }else{
            cout << setfill(' ') << setw(7) << "index" << setw(6) << "dirty" << setw(4 + tags/4) << "tag" <<endl;
            for (unsigned k = START; k < sized; k++) {
                if (cache_table[i][k].valid == 1) {
                    cout << setfill(' ') << setw(7) << dec << cache_table[i][k].index << setw(6) << dec << cache_table[i][k].dirty << setw(4) << "0x" << hex << cache_table[i][k].tag <<endl;
                }
            }
        }
    }
}

unsigned cache::evict(long long index) {
    unsigned least = START;
    unsigned record = access;

    for (unsigned i = START; i < coeval; i++) {
        if (record >= cache_table[i][index].access_record) {
            least = i;
            record = cache_table[i][index].access_record;
        }
    }
    return least;
}

unsigned cache::allocate(address_t address) {
    long long index;
    long long tag;
    long long j;

    index = ((address >> offset) & index_shift);
    j = (index % sized);

    unsigned evicter = evict(j);
    tag = address >> (indexes+offset);


    for (unsigned i = START; i < coeval; i++) {
        if (cache_table[i][j].valid == START) {
            cache_table[i][j].access_record = access;
            cache_table[i][j].tag = tag;

            cache_table[i][j].valid = 1;
            cache_table[i][j].index = index;

            return i;
        }
    }
    eviction++;
    if (cache_table[evicter][j].dirty == 1) {
        cache_table[evicter][j].dirty = START;
        memory++;
    }

    cache_table[evicter][j].index = index;
    cache_table[evicter][j].access_record = access;

    cache_table[evicter][j].tag = tag;

    return evicter;
}

