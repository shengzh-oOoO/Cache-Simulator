#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "stdio.h"
#include "cache.h"
#include "memory.h"

void run(int cache_size, int ass, int line_size, int write_through, int write_allocate, char *filename){
    Memory m;
    Cache l1;
    l1.SetLower(&m);
    //for(int cache_size)
    StorageStats zero;
    zero.access_counter = zero.miss_num = zero.access_time = zero.replace_num = zero.fetch_num = zero.prefetch_num = 0;
    
    StorageLatency ml,l1l;
    ml.hit_latency = 100;
    ml.bus_latency = 0;
    l1l.hit_latency = 1;
    l1l.bus_latency = 0;

    CacheConfig l1cc;
    l1cc.cache_size = cache_size;
    l1cc.ass = ass;
    l1cc.line_size = line_size;
    int set_num;
    set_num = l1cc.set_num = l1cc.cache_size / l1cc.ass / l1cc.line_size;
    l1cc.write_through = write_through;
    l1cc.write_allocate = write_allocate;

    m.SetStats(zero);
    l1.SetStats(zero);

    m.SetLatency(ml);
    l1.SetLatency(l1l);

    l1.SetConfig(l1cc);

    l1.CacheInit();
    
    FILE *fp;
    //filename = argv[1];
    
    fp = fopen(filename, "r");
    char action[2];
    unsigned long long addr;

    char _content_[8];
    memset(_content_, 0, sizeof(_content_));
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
    }
    StorageStats result;
    l1.GetStats(result);
    //printf("cache_size:%d, ass:%d, line_size:%d, set_num:%d, write_through%d, write_allocate:%d\n", cache_size, ass, line_size, set_num, write_through, write_allocate);
    //printf("%d %d\n", result.miss_num, result.access_counter);
    //printf("%f\n", (float) (result.miss_num) / result.access_counter);
    printf("%d\n", result.access_time);

    fclose(fp);
}

int main(int argc, char* argv[]) {
    if(argc < 2){
        exit(0);
    }
    
    //int cache_size = 2<<20, ass = 1,line_size = 128,write_through = 0,write_allocate = 1;
    //run(cache_size, ass, 2, write_through, write_allocate, argv[1]);
    /*
    for(int cache_size = (32<<10); cache_size <= (32<<20); cache_size = (cache_size<<1)){
        for(int line_size = 2; line_size <= 8192; line_size = line_size * 4){
            run(cache_size,1,line_size,1,0,argv[1]);
        }
        printf("------------------------------\n");
    }
    */
    /*
    for(int cache_size = (32<<10); cache_size <= (512<<10); cache_size = (cache_size<<1)){
        for(int ass = 1; ass <= 32; ass = ass * 2){
            run(cache_size,ass,64, 1,0,argv[1]);
        }
        printf("------------------------------\n");
    }
    */
    for(int cache_size = (32<<10); cache_size <= (32<<20); cache_size = (cache_size<<1)){
        for(int line_size = 2; line_size <= 512; line_size = line_size * 4){
            run(cache_size,4,line_size,0,1,argv[1]);
            run(cache_size,4,line_size,1,0,argv[1]);
            //printf("\n");
        }
        printf("------------------------------\n");
    }
    return 0;
}
