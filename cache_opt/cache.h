#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"

//#define BypassArrayBits 10
enum PreFetchOption{NONE, NEXTLINES, SPAN};
enum ReplaceOption{LRU, LIRS, LRU_K};

typedef struct CacheConfig_ {
    int cache_size;
    int ass; //associativity
    int line_size; // number of blocks in one line
    int set_num; // Number of cache sets
    int write_through; // 0|1 for back|through
    int write_allocate; // 0|1 for no-alc|alc
    //---------------------//
    int bypass_on;
    int bypass_array_bits;
    PreFetchOption prefetchopt;
    int prefetch_lines;
    ReplaceOption replaceopt;
    int lru_k;
} CacheConfig;

typedef struct Line_ {
    bool valid;
    bool dirty;
    uint64_t tag;
    int LastAccess_time;
    int SecondLastAccess_time;
    int cnt;
} Line;

typedef struct Set_ {
    Line *line;
    int STD_time;
} Set;

class Cache: public Storage {
    public:
    Cache() {}
    ~Cache() {CacheFree();}

    int log2(int A);
    void CacheInit();
    void CacheFree();
    // Sets & Gets
    void SetConfig(CacheConfig cc){config_ = cc;};
    void GetConfig(CacheConfig &cc){cc = config_;};
    void SetLower(Storage *ll) { lower_ = ll; }
    // Main access process
    void HandleRequest(uint64_t addr, int bytes, int read, char *content, int &hit, int &time);

    private:
    // Bypassing
    int BypassDecision(int ts_lowBABbit);
    // Partitioning
    void PartitionAlgorithm(uint64_t addr, uint64_t &tag, uint64_t &setindex, uint64_t &blockoffset);
    // Replacement
    uint64_t FindLineindex(uint64_t tag, uint64_t setindex);
    void ReplaceAlgorithm(uint64_t tag, uint64_t setindex, int &time);
    // Prefetching
    int PrefetchDecision();
    void PrefetchAlgorithm(uint64_t addr);

    CacheConfig config_;
    Storage *lower_;

    int prefetch_flag;
    int t,s,b;
    Set *set;

    short *bypass_array;

    DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
