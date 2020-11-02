#include <stdint.h>
#include <stdio.h>
//----------------------------------------------------//momery
#define MAX 100000000
unsigned char memory[MAX]={0};
//------------------------------------------------------
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&); \
    void operator=(const TypeName&)

// Storage access stats
typedef struct StorageStats_ {
    int access_counter;
    int miss_num;
    int access_time; // In nanoseconds
    int replace_num; // Evict old lines
    int fetch_num; // Fetch lower layer
    int prefetch_num; // Prefetch
} StorageStats;

// Storage basic config
typedef struct StorageLatency_ {
    int hit_latency; // In nanoseconds
    int bus_latency; // Added to each request
} StorageLatency;

class Storage {
    public:
    Storage() {}
    ~Storage() {}

    // Sets & Gets
    void SetStats(StorageStats ss) { stats_ = ss; }
    void GetStats(StorageStats &ss) { ss = stats_; }
    void SetLatency(StorageLatency sl) { latency_ = sl; }
    void GetLatency(StorageLatency &sl) { sl = latency_; }

    // Main access process
    // [in]  addr: access address
    // [in]  bytes: target number of bytes
    // [in]  read: 0|1 for write|read
    // [i|o] content: in|out data
    // [out] hit: 0|1 for miss|hit
    // [out] time: total access time
    virtual void HandleRequest(uint64_t addr, int bytes, int read, unsigned char *content, int &hit, int &time) = 0;

    protected:
    StorageStats stats_;
    StorageLatency latency_;
};
//------------------------------------------------------------
class Memory: public Storage {
 	public:
  	Memory() {}
  	~Memory() {}

  	// Main access process
  	void SetMemory(unsigned char *m){MEMORY = m;}
  	void HandleRequest(uint64_t addr, int bytes, int read, unsigned char *content, int &hit, int &time);

 	private:
  	// Memory implement
 	unsigned char *MEMORY;
  	DISALLOW_COPY_AND_ASSIGN(Memory);
};
void Memory::HandleRequest(uint64_t addr, int bytes, int read, unsigned char *content, int &hit, int &time) {
	hit = 1;
	time = latency_.hit_latency + latency_.bus_latency;
	stats_.access_time += time;
	//write
	if(read == 0){
		for(int i = 0; i < bytes; i++){
			MEMORY[addr+i] = content[i];
		}
	}
	//read
	else if (read == 1){
		for(int i = 0; i < bytes; i++){
			content[i] = MEMORY[addr+i];
		}
	}
}
//-------------------------------------------------------------
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
    unsigned char *block;
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
    void HandleRequest(uint64_t addr, int bytes, int read, unsigned char *content, int &hit, int &time);

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
//****************
int Cache::log2(int A){
	int result = -1;
	while(A!=0){
		result++;
		A >>= 1;
	}
	return result;
}
void Cache::CacheInit(){
	//check write_through&write_allocate
	/*
	if(config_.write_through==1 && config_.write_allocate==0){
	}
	else
	*/ 
	if(config_.write_through==0 && config_.write_allocate==1){
	}
	else{
		printf("write_through&write_allocate set error\n");
		return;
	}
	//set t,s,b
	if(config_.cache_size!=config_.line_size * config_.ass * config_.set_num){
		printf("config size error\n");
		return;
	}
	s = log2(config_.set_num);
	b = log2(config_.line_size);
	t = 64 - s -b;
	//malloc
	set = (Set *)malloc(sizeof(Set)*config_.set_num);
	for(int i = 0; i < config_.set_num; i++){
		set[i].line = (Line *)malloc(sizeof(Line)*config_.ass);
		for(int j = 0; j < config_.ass; j++){
			set[i].line[j].valid = 0;
			set[i].line[j].dirty = 0;
			set[i].line[j].tag = 0;
			set[i].line[j].LRU_time = 0;
			set[i].line[j].block=(unsigned char *)malloc(sizeof(unsigned char)*config_.line_size);
			for(int k = 0; k < config_.line_size; k++){
				set[i].line[j].block[k] = 0;
			}
		}
		set[i].LRU_std_time = 1;
	}
}

void Cache::CacheFree(){
	//free
	//printf("free\n");
	for(int i = 0; i < config_.set_num; i++){
		for(int j = 0; j < config_.ass; j++){
			free(set[i].line[j].block);
		}
		free(set[i].line);
	}
	free(set);
}

void Cache::HandleRequest(uint64_t addr, int bytes, int read, unsigned char *content, int &hit, int &time) {
	hit = 0;
	time = 0;
	//判断是不是在同一个line
	uint64_t block_offest = (addr << (t+s)) >> (t+s);
	if(t+s == 64){
		block_offest = 0;
	}
	if(block_offest + bytes > config_.line_size){
        //bytes too long
        int tmp_hit, tmp_time;
        HandleRequest(addr, (int)(config_.line_size - block_offest), read, content, tmp_hit, tmp_time);
        hit += tmp_hit;
        time += tmp_time;
        HandleRequest(addr+config_.line_size, bytes-(int)(config_.line_size - block_offest), read,
        	content+(int)(config_.line_size - block_offest), tmp_hit, tmp_time);
        hit += tmp_hit;
        time += tmp_time;
        if(hit == 2){
        	hit = 1;
        }
        else{
        	hit = 0;
        }
        return;
    }
    /*
	//写穿透 写不分配
	if(config_.write_through==1 && config_.write_allocate==0){
		//Bypass?
		if(!BypassDecision()){
			stats_.access_counter ++;

			uint64_t tag;
			uint64_t setindex;
			uint64_t blockoffest;
			PartitionAlgorithm(addr, tag, setindex, blockoffest);

			uint64_t lineindex = ReplaceDecision(tag, setindex);
			//write
			if(read == 0){
				//hit
				if(lineindex != -1){
					//write this layer
					hit = 1;
					time += latency_.bus_latency + latency_.hit_latency;
					//更新访问时间
					set[setindex].line[lineindex].LRU_time = set[setindex].LRU_std_time ++;
					//write through
					int lower_hit=0, lower_time=0;
					lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
					time += lower_time;

					stats_.access_time += time;
				}
				//miss
				else if(lineindex == -1){
					hit = 0;
					time += latency_.bus_latency + latency_.hit_latency;
					stats_.miss_num++;
					//写不分配, 访问下一层
					int lower_hit=0, lower_time=0;
					lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
					time += lower_time;

					stats_.access_time += time;
				}
			}
			//read
			else if(read == 1){
				//hit
				if(lineindex != -1){
					hit = 1;
					time += latency_.bus_latency + latency_.hit_latency;
					//更新访问时间
					set[setindex].line[lineindex].LRU_time = set[setindex].LRU_std_time ++;

					stats_.access_time += time;
				}
				//miss
				else if(lineindex == -1){
					hit = 0;
					time += latency_.bus_latency + latency_.hit_latency;
					stats_.miss_num++;
					//访问下一层
					stats_.fetch_num++;
					int lower_hit=0, lower_time=0;
					lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
					time += lower_time;
					//更新cache
					int tmp_time;
					ReplaceAlgorithm(tag, setindex, tmp_time);
					time += tmp_time;

					stats_.access_time += time;
				}
			}
		}
		// Prefetch?
	  	if (PrefetchDecision()) {
	    	PrefetchAlgorithm();
	  	}
	}
	//HandleRequest(uint64_t addr, int bytes, int read, unsigned char *content, int &hit, int &time)//
	//写回 写分配
	else 
	*/
	if(config_.write_through==0 && config_.write_allocate==1){
		//Bypass?
		if(!BypassDecision()){
			stats_.access_counter ++;

			uint64_t tag;
			uint64_t setindex;
			uint64_t blockoffest;
			PartitionAlgorithm(addr, tag, setindex, blockoffest);

			uint64_t lineindex = ReplaceDecision(tag, setindex);

			//write
			if(read == 0){
				//hit
				if(lineindex != -1){
					//write this layer, write back
					hit = 1;
					time += latency_.bus_latency + latency_.hit_latency;					
					//
					for(int i = 0; i < bytes; i++){
						set[setindex].line[lineindex].block[blockoffest+i] = content[i];
					}

					//更新访问时间
					set[setindex].line[lineindex].LRU_time = set[setindex].LRU_std_time ++;
					set[setindex].line[lineindex].dirty = 1;
					
					stats_.access_time += time;
				}
				//miss
				else if(lineindex == -1){
					hit = 0;
					time += latency_.bus_latency + latency_.hit_latency;

					stats_.miss_num++;
					//写分配, 访问下一层,write
					int lower_hit=0, lower_time=0;
					lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
					time += lower_time;
					//写分配
					stats_.fetch_num++;
					int tmp_time;
					ReplaceAlgorithm(tag, setindex, tmp_time);
					time += tmp_time;
					stats_.access_time += time;
				}
			}
			//read
			else if(read == 1){
				//hit
				if(lineindex != -1){
					hit = 1;
					time += latency_.bus_latency + latency_.hit_latency;
					//更新访问时间
					set[setindex].line[lineindex].LRU_time = set[setindex].LRU_std_time ++;

					for(int i = 0; i < bytes; i++){
						content[i] = set[setindex].line[lineindex].block[blockoffest+i];
					}

					stats_.access_time += time;
				}
				//miss
				else if(lineindex == -1){
					hit = 0;
					time += latency_.bus_latency + latency_.hit_latency;
					stats_.miss_num++;
					//访问下一层
					stats_.fetch_num++;
					int lower_hit=0, lower_time=0;
					lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
					time += lower_time;
					//更新cache
					int tmp_time;
					ReplaceAlgorithm(tag, setindex, tmp_time);
					time += tmp_time;

					stats_.access_time += time;
				}
			}
		}
		// Prefetch?
	  	if (PrefetchDecision()) {
	    	PrefetchAlgorithm();
	  	}
	}
	/*
	// Bypass?
	if (!BypassDecision()) {
    	PartitionAlgorithm();
    	// Miss?
    	if (ReplaceDecision()) {
      	// Choose victim
      		ReplaceAlgorithm();
    	}
    	else {
      		// return hit & time
      		hit = 1;
      		time += latency_.bus_latency + latency_.hit_latency;
      		stats_.access_time += time;
      		return;
    	}
  	}
  	// Prefetch?
  	if (PrefetchDecision()) {
    	PrefetchAlgorithm();
  	}
  	else {
    	// Fetch from lower layer
    	int lower_hit, lower_time;
    	lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
    	hit = 0;
    	time += latency_.bus_latency + lower_time;
    	stats_.access_time += latency_.bus_latency;
  	}
  	*/
}

int Cache::BypassDecision() {
	//never Bypass
  	return 0;
}

void Cache::PartitionAlgorithm(uint64_t addr, uint64_t &tag, uint64_t &setindex, uint64_t &blockoffest) {
	tag = addr >> (s+b);
	setindex = (addr << t) >> (t+b);
	blockoffest = (addr <<(t+s)) >>(t+s);
	if(t == 0){
		tag = 0;
	}
	if(s == 0){
		setindex = 0;
	}
	if(b == 0){
		blockoffest = 0;
	}
	//printf("%d %d %d\n", t,s,b);
	//printf("%llx\n", addr);
	//printf("%llx %llx %llx \n", tag, setindex, blockoffest);
}

uint64_t Cache::ReplaceDecision(uint64_t tag, uint64_t setindex) {
	for(uint64_t i = 0; i < config_.ass; i++){
		if(set[setindex].line[i].tag == tag && set[setindex].line[i].valid == 1){
			//printf("%lld %lld\n",set[setindex].line[i].tag,tag);
			return i;
		}
	}
	return -1;
}

void Cache::ReplaceAlgorithm(uint64_t tag, uint64_t setindex, int &time){
	time = 0;
	int max_time = set[setindex].LRU_std_time;
	int victim = -1;
	//寻找无效line
	for(int i = 0; i < config_.ass; i++){
		if(set[setindex].line[i].valid == 0){
			victim = i;
		}
	}
	//寻找LRU
	if(victim == -1){
		for(int i = 0; i < config_.ass; i++){
			if(max_time > set[setindex].line[i].LRU_time){
				max_time = set[setindex].line[i].LRU_time;
				victim = i;
			}
		}
	}
	if(victim == -1){
		printf("REPACE ERROR\n");
	}
	if(set[setindex].line[victim].dirty == 1){
		int lower_hit, lower_time;
		//write dirty line to memmory
		uint64_t addr = (set[setindex].line[victim].tag << (s+b)) | (setindex << b);
		int bytes = config_.line_size;
		lower_->HandleRequest(addr, bytes, 0, set[setindex].line[victim].block, lower_hit, lower_time);
		time += lower_time;
	}
	//read new line
	set[setindex].line[victim].tag = tag;
	set[setindex].line[victim].valid = 1;
	set[setindex].line[victim].dirty = 0;
	set[setindex].line[victim].LRU_time = set[setindex].LRU_std_time++;
	int lower_hit, lower_time;
	uint64_t addr = (tag << (s+b)) | (setindex << b);
	int bytes = config_.line_size;
	lower_->HandleRequest(addr, bytes, 1, set[setindex].line[victim].block,lower_hit, lower_time);
	time += lower_time;
}

int Cache::PrefetchDecision() {
	//Never prefetch
  	return 0;
}

void Cache::PrefetchAlgorithm() {
	//donothing
}