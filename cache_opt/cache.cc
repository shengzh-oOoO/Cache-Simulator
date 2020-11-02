#include "cache.h"
#include "def.h"
#include<malloc.h>
#include<cstring>

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

	if(config_.bypass_on == 1){
		bypass_array = (short *)malloc(sizeof(short) * (1 << config_.bypass_array_bits));
		memset(bypass_array, 0, sizeof(bypass_array));
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
			set[i].line[j].LastAccess_time = 0;
			set[i].line[j].SecondLastAccess_time = 0;
			set[i].line[j].cnt = 0;
		}
		set[i].STD_time = 1;
	}

}

void Cache::CacheFree(){
	//free
	if(config_.bypass_on == 1){
		free(bypass_array);
	}
	for(int i = 0; i < config_.set_num; i++){
		free(set[i].line);
	}
	free(set);
}

void Cache::HandleRequest(uint64_t addr, int bytes, int read, char *content, int &hit, int &time) {
	prefetch_flag = 0;
	hit = 0;
	time = 0;
	//判断是不是在同一个line
	uint64_t block_offest = (addr << (t+s)) >> (t+s);
	if(t+s == 64){
		block_offest = 0;
	}
	if(block_offest + bytes > config_.line_size){
		printf("too long\n");
        //bytes too long
        int tmp_hit, tmp_time;
        HandleRequest(addr, (int)(config_.line_size - block_offest), read, content, tmp_hit, tmp_time);
        hit += tmp_hit;
        time += tmp_time;
        HandleRequest(addr+config_.line_size, bytes-(int)(config_.line_size - block_offest), read, content+(int)(config_.line_size - block_offest), tmp_hit, tmp_time);
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
    
	//写回 写分配
	if(config_.write_through==0 && config_.write_allocate==1){
		//Bypass?
		int ts_lowBABbit = int((addr<<(t+s-config_.bypass_array_bits)) >> (t+s-config_.bypass_array_bits+b) );

		uint64_t tag;
		uint64_t setindex;
		uint64_t blockoffest;
		PartitionAlgorithm(addr, tag, setindex, blockoffest);

		if(!BypassDecision(ts_lowBABbit)){
			stats_.access_counter ++;

			uint64_t lineindex = FindLineindex(tag, setindex);
			//write
			if(read == 0){
				//hit
				if(lineindex != -1){
					//write this layer, write back
					hit = 1;
					time += latency_.bus_latency + latency_.hit_latency;
					//更新访问时间
					set[setindex].line[lineindex].SecondLastAccess_time = set[setindex].line[lineindex].LastAccess_time;
					set[setindex].line[lineindex].LastAccess_time = set[setindex].STD_time ++;
					set[setindex].line[lineindex].cnt++;
					set[setindex].line[lineindex].dirty = 1;

					stats_.access_time += time;
				}
				//miss
				else if(lineindex == -1){
					prefetch_flag = 1;
					hit = 0;
					time += latency_.bus_latency + latency_.hit_latency;

					stats_.miss_num++;
					//写分配, 访问下一层,write&read
					stats_.fetch_num++;
					int lower_hit=0, lower_time=0;
					lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
					time += lower_time;
					//写分配
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
					set[setindex].line[lineindex].SecondLastAccess_time = set[setindex].line[lineindex].LastAccess_time;
					set[setindex].line[lineindex].LastAccess_time = set[setindex].STD_time ++;
					set[setindex].line[lineindex].cnt++;
					stats_.access_time += time;
				}
				//miss
				else if(lineindex == -1){
					prefetch_flag = 1;
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
		//bypass
		else{
			prefetch_flag = 1;
			stats_.bypass_num++;
			stats_.fetch_num ++;
			int lower_hit=0, lower_time=0;
			lower_->HandleRequest(addr, bytes, read, content, lower_hit, lower_time);
			time += lower_time;
			//
			int tmp_time;
			ReplaceAlgorithm(tag, setindex, tmp_time);
			time += tmp_time;
		}
		
		// Prefetch?
	  	if (PrefetchDecision()) {
	    	PrefetchAlgorithm(addr);
	  	}
	}
	else{
		printf("ERROR\n");
	}
}

int Cache::BypassDecision(int ts_lowBABbit) {
	//never Bypass
  	//return FALSE;
  	if(config_.bypass_on == 0){
  		return 0;
  	}
  	else if(config_.bypass_on == 1){
  		if(bypass_array[ts_lowBABbit] == 0){
  			return 1;
  		}
  		else if(bypass_array[ts_lowBABbit] > 0){
  			return 0;
  		}
  		else{
  			printf("Bypass Error\n");
  			return 0;
  		}
  	}
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
}

uint64_t Cache::FindLineindex(uint64_t tag, uint64_t setindex) {
	for(uint64_t i = 0; i < config_.ass; i++){
		if(set[setindex].line[i].tag == tag && set[setindex].line[i].valid == 1){
			return i;
		}
	}
	return -1;
}

void Cache::ReplaceAlgorithm(uint64_t tag, uint64_t setindex, int &time){
	time = 0;
	int victim = -1;
	//寻找无效line
	for(int i = 0; i < config_.ass; i++){
		if(set[setindex].line[i].valid == 0){
			victim = i;
		}
	}
	//寻找LRU
	if(victim == -1){
		
		if(config_.replaceopt == LRU){
			int min_time = set[setindex].STD_time;
			for(int i = 0; i< config_.ass; i++){
				if(min_time > set[setindex].line[i].LastAccess_time){
					min_time = set[setindex].line[i].LastAccess_time;
					victim = i;
				}
			}
		}

		else if(config_.replaceopt == LIRS){
			int *IRR;
			int *R;
			IRR = (int *)malloc(sizeof(int)*config_.ass);
			R = (int *)malloc(sizeof(int)*config_.ass);
			for(int i = 0; i < config_.ass; i++){
				IRR[i] = set[setindex].line[i].LastAccess_time - set[setindex].line[i].SecondLastAccess_time;
				if(set[setindex].line[i].SecondLastAccess_time == 0){
					IRR[i] = 0x7FFFFFFF;
				}
				R[i] = set[setindex].STD_time - set[setindex].line[i].LastAccess_time;
			}
			int maxIRR = 0;
			for(int i = 0; i < config_.ass; i++){
				if(maxIRR < IRR[i]){
					maxIRR = IRR[i];
				}
			}
			int maxR = 0;
			for(int i = 0; i < config_.ass; i++){
				if(IRR[i] != maxIRR) continue;
				if(maxR < R[i]){
					maxR = R[i];
					victim = i;
				}
			}
			free(IRR);
			free(R);
		}
		
		else if(config_.replaceopt == LRU_K){
			int min_time = set[setindex].STD_time;
			for(int k = 1; k <= config_.lru_k - 1; k++){
				for(int i = 0; i< config_.ass; i++){
					if(set[setindex].line[i].cnt != k) continue;
					if(min_time > set[setindex].line[i].LastAccess_time){
						min_time = set[setindex].line[i].LastAccess_time;
						victim = i;
					}
				}
				if(victim != -1) break;
			}

			if(victim == -1){
				for(int i = 0; i< config_.ass; i++){
					if(set[setindex].line[i].cnt < config_.lru_k) continue;
					if(min_time > set[setindex].line[i].LastAccess_time){
						min_time = set[setindex].line[i].LastAccess_time;
						victim = i;
					}
				}
			}
		}
	}
	if(victim == -1){
		printf("REPACE ERROR\n");
	}

	if (config_.bypass_on == 1){
		if(set[setindex].line[victim].valid == 1){
			uint64_t tmp_addr = (set[setindex].line[victim].tag << (s+b)) | (setindex << b);
			int ts_lowBABbit = int((tmp_addr<<(t+s-config_.bypass_array_bits)) >> (t+s-config_.bypass_array_bits+b));
			bypass_array[ts_lowBABbit] --;
		}
		uint64_t tmp_addr = (tag << (s+b)) | (setindex << b);
		int ts_lowBABbit = int((tmp_addr<<(t+s-config_.bypass_array_bits)) >> (t+s-config_.bypass_array_bits+b));
		bypass_array[ts_lowBABbit] ++;
	}

	if(set[setindex].line[victim].dirty == 1){
		int lower_hit, lower_time;
		//write dirty line to memmory
		char content[8];
		//char *content = (char *)malloc(sizeof(char)*config_.line_size);
		uint64_t addr = (tag << (s+b)) | (setindex << b);
		int bytes = config_.line_size;
		lower_->HandleRequest(addr, bytes, 0, content, lower_hit, lower_time);
		//free(content);
		time += lower_time;
	}
	stats_.replace_num++;
	set[setindex].line[victim].tag = tag;
	set[setindex].line[victim].valid = 1;
	set[setindex].line[victim].dirty = 0;
	set[setindex].line[victim].SecondLastAccess_time = 0;
	set[setindex].line[victim].LastAccess_time = set[setindex].STD_time++;
	set[setindex].line[victim].cnt = 1;
}

int Cache::PrefetchDecision() {
	//Never prefetch
  	//return FALSE;
  	if(config_.prefetchopt == NONE){
  		return 0;
  	}
  	else if (config_.prefetchopt == NEXTLINES){
  		return 1;
  	}
  	else{
  		printf("prefetch ERROR\n");
  		return 0;
  	}
}

void Cache::PrefetchAlgorithm(uint64_t addr) {
	stats_.prefetch_num++;
	if(config_.prefetchopt == NONE){
		printf("PrefetchAlgorithm ERROR\n");
	}
	else if(config_.prefetchopt == NEXTLINES){
		addr = addr & 0xFFFFFFC0;
		
		for(int i = 1; i <= config_.prefetch_lines; i++){
			uint64_t tag;
			uint64_t setindex;
			uint64_t blockoffest;
			PartitionAlgorithm(addr+(i<<b), tag, setindex, blockoffest);
			
			int lineindex = FindLineindex(tag, setindex);

			if(lineindex == -1){
				char content[64];
				int lower_hit=0, lower_time=0;
				lower_->HandleRequest(addr+(i<<b), config_.line_size, 1, content, lower_hit, lower_time);

				int tmp_time;
				ReplaceAlgorithm(tag, setindex, tmp_time);
			}
		}
	}
}