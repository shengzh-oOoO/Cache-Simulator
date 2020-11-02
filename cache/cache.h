#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include "storage.h"

typedef struct CacheConfig_ {
    int cache_size;
    int ass; //associativity
    int line_size; // number of blocks in one line
    int set_num; // Number of cache sets
    int write_through; // 0|1 for back|through
    int write_allocate; // 0|1 for no-alc|alc
} CacheConfig;

typedef struct Line_ {
    bool valid;
    bool dirty;
    uint64_t tag;
    int LRU_time;
} Line;

typedef struct Set_ {
    Line *line;
    int LRU_std_time;
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
    int BypassDecision();
    // Partitioning
    void PartitionAlgorithm(uint64_t addr, uint64_t &tag, uint64_t &setindex, uint64_t &blockoffset);
    // Replacement
    uint64_t ReplaceDecision(uint64_t tag, uint64_t setindex);
    void ReplaceAlgorithm(uint64_t tag, uint64_t setindex, int &time);
    // Prefetching
    int PrefetchDecision();
    void PrefetchAlgorithm();

    CacheConfig config_;
    Storage *lower_;

    int t,s,b;
    Set *set;
    DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
