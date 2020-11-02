#include <stdint.h>
#include <stdio.h>
#include "storage.h"

Memory m;
Cache l1,l2,llc;
/*
unsigned char read_mem_1(unsigned long long address);
unsigned short read_mem_2(unsigned long long address);
unsigned int read_mem_4(unsigned long long address);
unsigned long long read_mem_8(unsigned long long address);
void write_mem_1(unsigned long long address, unsigned char val);
void write_mem_2(unsigned long long address, unsigned short val);
void write_mem_4(unsigned long long address, unsigned int val);
void write_mem_8(unsigned long long address, unsigned long long val);
*/
unsigned char read_mem_1(unsigned long long address, int &time){
    //return memory[address];
    int hit;
    unsigned char result;
    l1.HandleRequest(address, 1, 1, (unsigned char*)&result, hit, time);
    return result;
}
unsigned short read_mem_2(unsigned long long address, int &time){
    //return memory[address] | (memory[address+1]<<8);
    int hit;
    unsigned short result;
    l1.HandleRequest(address, 2, 1, (unsigned char*)&result, hit, time);
    return result;
}
unsigned int read_mem_4(unsigned long long address, int &time){
    //return memory[address] | (memory[address+1]<<8) | (memory[address+2]<<16) | (memory[address+3]<<24);
    int hit;
    unsigned int result;
    l1.HandleRequest(address, 4, 1, (unsigned char*)&result, hit, time);
    return result;
}
unsigned long long read_mem_8(unsigned long long address, int &time){
    /*
    return memory[address] | (memory[address+1]<<8) | (memory[address+2]<<16) | (memory[address+3]<<24)
           | ((unsigned long long)(memory[address+4])<<32) | ((unsigned long long)(memory[address+5])<<40) 
           | ((unsigned long long)(memory[address+6])<<48) | ((unsigned long long)(memory[address+7])<<56);
    */
    int hit;
    unsigned long long result;
    l1.HandleRequest(address, 8, 1, (unsigned char*)&result, hit, time);
    return result;
}

void write_mem_1(unsigned long long address, unsigned char val, int &time){
    //memory[address] = (unsigned char)val;
    int hit;
    l1.HandleRequest(address, 1, 0, (unsigned char*)&val, hit, time);
}
void write_mem_2(unsigned long long address, unsigned short val, int &time){
    //memory[address] = (unsigned char)val;
    //memory[address+1] = (unsigned char)(val>>8);
    int hit;
    l1.HandleRequest(address, 2, 0, (unsigned char*)&val, hit, time);
}
void write_mem_4(unsigned long long address, unsigned int val, int &time){
    //memory[address] = (unsigned char)val;
    //memory[address+1] = (unsigned char)(val>>8);
    //memory[address+2] = (unsigned char)(val>>16);
    //memory[address+3] = (unsigned char)(val>>24);
    int hit;
    l1.HandleRequest(address, 4, 0, (unsigned char*)&val, hit, time);
}
void write_mem_8(unsigned long long address, unsigned long long val, int &time){
    //memory[address] = (unsigned char)val;
    //memory[address+1] = (unsigned char)(val>>8);
    //memory[address+2] = (unsigned char)(val>>16);
    //memory[address+3] = (unsigned char)(val>>24);
    //memory[address+4] = (unsigned char)(val>>32);
    //memory[address+5] = (unsigned char)(val>>40);
    //memory[address+6] = (unsigned char)(val>>48);
    //memory[address+7] = (unsigned char)(val>>56);
    int hit;
    l1.HandleRequest(address, 8, 0, (unsigned char*)&val, hit, time);
}

void Cache_Memory_Init(){
    StorageStats zero;
    zero.access_counter = zero.miss_num = zero.access_time = zero.replace_num = zero.fetch_num = zero.prefetch_num = 0;

    StorageLatency ml;
    ml.hit_latency = 100;
    ml.bus_latency = 0;

    m.SetStats(zero);
    m.SetLatency(ml);
    m.SetMemory(memory);

    StorageLatency l1l;
    l1l.hit_latency = 1;
    l1l.bus_latency = 0;

    StorageLatency l2l;
    l2l.hit_latency = 8;
    l2l.bus_latency = 6;

    StorageLatency llcl;
    llcl.hit_latency = 20;
    llcl.bus_latency = 20;

    CacheConfig l1c;
    l1c.cache_size = 32<<10;
    l1c.ass = 8;
    l1c.line_size = 64;
    l1c.set_num = l1c.cache_size / l1c.ass / l1c.line_size;
    l1c.write_through = 0;
    l1c.write_allocate = 1;

    CacheConfig l2c;
    l2c.cache_size = 256<<10;
    l2c.ass = 8;
    l2c.line_size = 64;
    l2c.set_num = l2c.cache_size / l2c.ass / l2c.line_size;
    l2c.write_through = 0;
    l2c.write_allocate = 1;

    CacheConfig llcc;
    llcc.cache_size = 8<<20;
    llcc.ass = 8;
    llcc.line_size = 64;
    llcc.set_num = llcc.cache_size / llcc.ass / llcc.line_size;
    llcc.write_through = 0;
    llcc.write_allocate = 1;

    l1.SetStats(zero);
    l1.SetLatency(l1l);
    l1.SetLower(&l2);
    l1.SetConfig(l1c);
    l1.CacheInit();

    l2.SetStats(zero);
    l2.SetLatency(l2l);
    l2.SetLower(&llc);
    l2.SetConfig(l2c);
    l2.CacheInit();

    llc.SetStats(zero);
    llc.SetLatency(llcl);
    llc.SetLower(&m);
    llc.SetConfig(llcc);
    llc.CacheInit();
}