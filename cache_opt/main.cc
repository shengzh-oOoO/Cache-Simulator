#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "stdio.h"
#include "cache.h"
#include "memory.h"

int main(int argc, char* argv[]) {
    if(argc < 2){
        exit(0);
    }
    Memory m;
    Cache l1,l2;
    l1.SetLower(&l2);
    l2.SetLower(&m);

    StorageStats zero;
    zero.access_counter = zero.miss_num = zero.access_time = zero.replace_num = zero.fetch_num = zero.prefetch_num = zero.bypass_num = 0;
    
    StorageLatency ml,l1l,l2l;
    ml.hit_latency = 100;
    ml.bus_latency = 0;
    l1l.hit_latency = 3;
    l1l.bus_latency = 0;
    l2l.hit_latency = 4;
    l2l.bus_latency = 6;
    
    //enum PreFetchOption{NONE, NEXTLINES};
    //enum ReplaceOption{LRU, LIRS, LRU_K};
    CacheConfig l1c,l2c;
    l1c.cache_size = 32<<10;
    l1c.ass = 8;
    l1c.line_size = 64;
    l1c.set_num = l1c.cache_size / l1c.ass / l1c.line_size;
    l1c.write_through = 0;
    l1c.write_allocate = 1;
    l1c.bypass_on = 1;
    l1c.bypass_array_bits = 13;
    l1c.prefetchopt = NEXTLINES;
    l1c.prefetch_lines = 1;
    l1c.replaceopt = LIRS;
    l1c.lru_k = 2;

    l2c.cache_size = 256<<10;
    l2c.ass = 8;
    l2c.line_size = 64;
    l2c.set_num = l2c.cache_size / l2c.ass / l2c.line_size;
    l2c.write_through = 0;
    l2c.write_allocate = 1;
    l2c.bypass_on = 1;
    l2c.bypass_array_bits = 13;
    l2c.prefetchopt = NEXTLINES;
    l2c.prefetch_lines = 1;
    l2c.replaceopt = LIRS;
    l2c.lru_k = 2;

    m.SetStats(zero);
    l1.SetStats(zero);
    l2.SetStats(zero);

    m.SetLatency(ml);
    l1.SetLatency(l1l);
    l2.SetLatency(l2l);

    l1.SetConfig(l1c);
    l2.SetConfig(l2c);

    l1.CacheInit();
    l2.CacheInit();

    
    FILE *fp;
    
    fp = fopen(argv[1], "r");

    char action[2];
    unsigned long long addr;

    char _content_[8];
    memset(_content_, 0, sizeof(_content_));
    long long int total_time=0, rw_cnt=0;
    int hit, time;
    while (fscanf(fp, "%s %llx", action, &addr) != EOF){
        if (action[0] == 'r'){
            l1.HandleRequest(addr, 1, 1, _content_, hit, time);
        }
        else if (action[0] == 'w'){
            l1.HandleRequest(addr, 1, 0, _content_, hit, time);
        }
        else {
            printf("action is %s, addr is %lld\n", action, addr);
            printf("Illegal action. EXIT!\n");
            exit(0);
        }
        rw_cnt++;
        total_time+=time;
    }
    printf("%lld %lld\n", total_time, rw_cnt);
    printf("**AMAT:\t\t\t%f\n", (float) (total_time) / rw_cnt);

    StorageStats result;
    l1.GetStats(result);
    printf("l1 miss num:\t\t%d\nl1 access counter:\t%d\n", result.miss_num, result.access_counter);
    printf("**l1 local miss rate:\t%f\n", (float) (result.miss_num) / result.access_counter);
    printf("l1 bypass num:\t\t%d\n", result.bypass_num);
    printf("**l1 global miss rate:\t%f\n", (float) (result.miss_num+result.bypass_num) / rw_cnt);
    l2.GetStats(result);
    printf("l2 miss num:\t\t%d\nl2 access counter:\t%d\n", result.miss_num, result.access_counter);
    printf("**l2 local miss rate:\t%f\n", (float) (result.miss_num) / result.access_counter);
    printf("l1 bypass num:\t\t%d\n", result.bypass_num);
    printf("**l2 global miss rate:\t%f\n", (float) (result.miss_num+result.bypass_num) / rw_cnt);
    printf("----------\n");
    fclose(fp);
    return 0;
}