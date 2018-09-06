#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/time.h>
#include <list>
#include <set>
#include <unordered_map>
#include <utility>
#include <iomanip>

std::list<unsigned int> memory;
std::set<unsigned int> record_table;
std::unordered_map<unsigned int, std::list<unsigned int>::iterator>  LRU_hash_table;
unsigned int hit_cnt = 0;
unsigned int miss_cnt = 0;
unsigned int mem_size = 0;

void FIFO(unsigned int element){
  if(record_table.find(element) != record_table.end())
    hit_cnt++;
  else if (memory.size() < mem_size){
    miss_cnt++;
    memory.push_back(element);
    record_table.insert(element);
  }
  else{
    miss_cnt++;
    record_table.erase(record_table.find(memory.front()));
    memory.pop_front();
    memory.push_back(element);
    record_table.insert(element);
  }
}

void LRU(unsigned element){
  std::unordered_map<unsigned int, std::list<unsigned int>::iterator >::iterator it = LRU_hash_table.find(element);
  if (it != LRU_hash_table.end()){
    hit_cnt++;
    memory.erase(it->second);
    memory.push_front(element);
    it->second = memory.begin();
  }
  else if (LRU_hash_table.size() < mem_size){
    miss_cnt++;
    memory.push_front(element);
    LRU_hash_table.insert(std::make_pair(element, memory.begin()));
  }
  else {
    miss_cnt++;
    LRU_hash_table.erase(LRU_hash_table.find(memory.back()));
    memory.pop_back();
    memory.push_front(element);
    LRU_hash_table.insert(std::make_pair(element, memory.begin()));
  }
}


int main(int argc, char** argv){
  std::fstream trace;
  trace.open("trace.txt", std::ios::in);
  unsigned int next = 0;
  timeval start, end;
  
  if (trace.fail()){
    std::cout << "cannot open trace.txt";
    exit(1);
  }	
  else {
    gettimeofday(&start,0);
    std::cout << "FIFO---\n";
    std::cout << "size      " << "miss      " << "hit       " << "page fault ratio\n";
    for (unsigned int i = 128; i <= 1024; i *= 2){
      hit_cnt = 0;
      miss_cnt = 0;
      memory.clear();
      record_table.clear();
      mem_size = i;
      while(trace >> next){
	FIFO(next);
      }
      std::cout << std::left << std::setw(10) << i << std::setw(10) << miss_cnt << std::setw(10) << hit_cnt << std::fixed << std::setprecision(9) << (double)miss_cnt/(miss_cnt+hit_cnt) << "\n";
      trace.clear();
      trace.seekg(0, std::ios::beg);
      
    }
    std::cout << "LRU---\n";
    std::cout << "size      " << "miss      " << "hit       " << "page fault ratio\n";
    for (unsigned int i = 128; i <= 1024; i *= 2){
      hit_cnt = 0;
      miss_cnt = 0;
      memory.clear();
      LRU_hash_table.clear();
      mem_size = i;
      while(trace >> next){
	LRU(next);
      }
      std::cout << std::left << std::setw(10) << i << std::setw(10) << miss_cnt << std::setw(10) << hit_cnt << std::fixed << std::setprecision(9) << (double)miss_cnt/(miss_cnt+hit_cnt) << "\n";
      trace.clear();
      trace.seekg(0,std::ios::beg);
    }
    gettimeofday(&end,0);
    int sec = end.tv_sec - start.tv_sec;
    int usec = end.tv_usec - start.tv_usec;
    std::cout << "Use " << sec+usec/1000000.0 << " s\n";
  }
  
  return 0;
}
