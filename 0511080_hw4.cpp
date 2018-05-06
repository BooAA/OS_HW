#include <iostream>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unordered_set>
#include <unordered_map>
#include <memory.h>
#include <string>
#include <utility>
int arr[10000], arr_cpy[10000];

sem_t semaphore[8]; // all 8 semaphore to call threads
pthread_t* all_threads[8]; // all 8 threads


std::unordered_map<int, pthread_t*> thread_pool; // threads waiting for work
std::unordered_map<int, pthread_t*> working_thread; // threads that are working

int finish_job_cnt = 0; // count the finished job
sem_t finish_job_cnt_lock; // to protect finish_job_cnt

struct job{
  int start, end; // the sorting range
  int thread_index; // the thread who use the information inside this job
  int job_index; // the task doing now(i.e, from 1 to 15)
}job_args_queue[8];

std::unordered_set<job> job_list; // new job will place at here, and allocate by funciton allocate()

// bubble sort
void bubble(int left, int right){
  for (int i = left; i < right; ++i)
    for (int j = left; j < right; ++j)
      if (arr[j] > arr[j+1])
	std::swap(arr[j],arr[j+1]);
}

// paratition the range into 2 part
int partition(int left, int right){

  int j = left;
  for (int i = left+1; i <= right; ++i){
    if (arr[i] <= arr[left]){
      ++j;
      std::swap(arr[i] , arr[j]);
    }
    std::swap(arr[j],arr[left]);
  }
  return j;
}

// working part
void* work(void* args){
  job* this_args = (job*)args;
  while(1){
    sem_wait(semaphore + this_args->thread_index);


    if (this_args->job_index <= 7){
      int x = partition(this_args->start, this_args->end);

      // create 2 new job and place it to the job_list
      job new_job1 = {this_args->start, x-1, 0, 2*this_args->job_index};
      job new_job2 = {x+1, this_args->end, 0, 2*this_args->job_index+1};
      job_list.insert(new_job1);
      job_list.insert(new_job2);
      
      
    }
    else bubble(this_args->start, this_args->end);


    // increase the finished_job_cnt by 1 , and go back to wait another job
    sem_wait(&finish_job_cnt_lock);
    finish_job_cnt++;
    sem_post(&finish_job_cnt_lock);
    if (finish_job_cnt == 15) break;
  }
  pthread_exit(NULL);
}

void allocate(){

  while (finish_job_cnt != 15){

    while(!job_list.empty() && !thread_pool.empty()){

      // always assign the first job to the first thread in the thread pool
      
      job next_do_job = *job_list.begin(); 
      int next_work_thread_index = thread_pool.begin()->first;
      job_args_queue[thread_pool.begin()->first].start = next_do_job.start;
      job_args_queue[thread_pool.begin()->first].end = next_do_job.end;
      job_args_queue[thread_pool.begin()->first].job_index = next_do_job.job_index;
      job_list.erase(job_list.begin());
      thread_pool.erase(thread_pool.begin());
      sem_post(semaphore + next_work_thread_index); // wake the thread up
    }
  }
}

void init(int num_of_threads){
  finish_job_cnt = 0;
  sem_init(&finish_job_cnt_lock,0,1);
 
  for (int i = 0; i < num_of_threads; ++i){
    job_args_queue[i].thread_index = i;
    thread_pool.insert(std::make_pair(i,all_threads[i]));
    pthread_create(thread_pool[i], NULL, work, (void*)(job_args_queue+i));
  }
}

int main(int argc, char** argv){
  char filename[20];
  std::string output_filename = "output", extension = ".txt"; 
  int number_of_input;
  std::cout << "Please enter the file name\n This file must in the same directory:\n";
  
  std::cin >> filename;
  std::fstream input, output[8];
  input.open(filename, std::ios::in);

  if (input.fail()) std::cout << "cannot open the input file!\n";
  else input >> number_of_input;

  for (int i = 1; i <= 8; ++i){
    std::string name = output_filename + std::to_string(i) + extension;
    output[i].open(name.c_str(), std::ios::out);
    if (!output[i]) std::cout << "cannot create output" << i << ".txt";
  }
  
  for (int i = 0; i < number_of_input; ++i)
    input >> arr[i];
  

  timeval start, end;
  int sec, usec;
  for (int i = 1; i <= 8; ++i){
    memcpy(arr_cpy, arr, sizeof(arr));
    init(i);
    gettimeofday(&start, 0);
    job first_job = {0,number_of_input-1,0,1};
    job_list.insert(first_job);
    allocate();
    gettimeofday(&end, 0);
    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;
    for (int j = 0; j < number_of_input; ++j){
      output[i] << arr_cpy[j];
    }
    std::cout << i << "threads: " << sec + usec/10000000.0 << "sec\n";    
  }
  
  return 0;
}
  
