#include <iostream>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <memory.h>
#include <string>
#include <set>

int arr[1000000], arr_cpy[1000000]; // arr is read from input.txt , arr_cpy is the array to be sorted
// leave to control the master thread , finish_eight_case control the thread
bool leave = false , finish_eight_case = false; 
sem_t job_list_protect; // job_list can be accessed with only 1 thread at the same time
sem_t all_threads_done , get_job , start_allocate , new_job;
sem_t  has_job;
sem_t finish_job_cnt_lock; // to protect finish_job_cnt
pthread_t all_threads[8]; // all 8 threads
pthread_t master_thread;

int finish_job_cnt = 0; // count the finished job

struct job{
  int start, end; // the sorting range
  int thread_index; // the thread who use the information inside this job
  int job_index; // the task doing now(i.e, from 1 to 15)
  bool operator<(const job& a) const {
    return job_index < a.job_index;
  }
}job_lookup_place; // master thread remove the first job in the job_list and place here, the thread win the chance to work will look at here

std::set<job> job_list; // new job will place at here, and allocate by master thread

// bubble sort
void bubble(int left, int right){
  for (int i = left; i < right; ++i)
    for (int j = left; j < right-i+left; ++j)
      if (arr_cpy[j] > arr_cpy[j+1])
	std::swap(arr_cpy[j],arr_cpy[j+1]);
}

// paratition the range into 2 part and return the pivot place
int partition(int left, int right){
  int j = left;
  if (right > left){
    for (int i = left+1; i <= right; ++i){
      if (arr_cpy[i] <= arr_cpy[left]){
	++j;
	std::swap(arr_cpy[i] , arr_cpy[j]);
      }
    }
  }
  if (right > left) std::swap(arr_cpy[j],arr_cpy[left]);
  
  return j;
}

// working part
void* work(void* no_args){
  // untill all 8 case done will the thread exit
  while(!finish_eight_case){
    sem_wait(&has_job);  // wait the master thread to signal 
    job task = job_lookup_place; // lookup the job in the job_lookup_place
    sem_post(&get_job); // tell master that I got the job
    if (task.job_index <= 7){
      // do partition and insert 2 new job into job_list
      int pivot = partition(task.start, task.end);
      job new_job1 = {task.start,pivot-1,0,2*task.job_index};
      job new_job2 = {pivot+1,task.end,0,2*task.job_index+1};
      sem_wait(&job_list_protect);
      job_list.insert(new_job1);
      job_list.insert(new_job2);
      sem_post(&job_list_protect);

      // increase the finish job number by 1
      sem_wait(&finish_job_cnt_lock);
      finish_job_cnt++;
      sem_post(&finish_job_cnt_lock);
    }
    else{
      // bubble sort
      bubble(task.start,task.end);
      sem_wait(&finish_job_cnt_lock);
      finish_job_cnt++;
      // if all job done than told master to leave and signal the main function
      if (finish_job_cnt == 15) leave = true;
      sem_post(&finish_job_cnt_lock);
    }   
  }
  pthread_exit(NULL);
}

void* master(void* no_args){
    while(!leave){
      sem_wait(&job_list_protect);
      if (!job_list.empty()){
	// check whether the job_list is empty or not, if not , signal the thread that there is a new job
	// put the first job in the job_list to the lookup place and remove the it in the job_list
	job_lookup_place.start = job_list.begin()->start;
	job_lookup_place.end = job_list.begin()->end;
	job_lookup_place.job_index = job_list.begin()->job_index;
	job_list.erase(job_list.begin());
	sem_post(&job_list_protect); // only can be placed here , otherwise deadlock 
	sem_post(&has_job);// signal the thread there is a new job
	sem_wait(&get_job); // wait the thread to get the job
      }
      else sem_post(&job_list_protect);     
    }
    
    sem_post(&all_threads_done); // signal main function that these case is done
    pthread_exit(NULL);
}


int main(int argc, char** argv){
  int number_of_input = 0;
  timeval start, end;
  int sec, usec;
  char filename[20];
  std::fstream input, output[8];
  std::cout << "please enter the file name:\n";
  std::cin >> filename;
  input.open(filename,std::ios::in);
  if (input.fail()) {
    std::cout << "cannot open input file\n";
     exit(1);
  }
  else input >> number_of_input;
				 
  for (int i = 0; i < number_of_input; ++i)
    input >> arr[i];

  // initialize the semaphore
  sem_init(&finish_job_cnt_lock, 0, 1);
  sem_init(&has_job,0,0);
  sem_init(&all_threads_done,0,0);
  sem_init(&job_list_protect,0,1);
  sem_init(&new_job,0,0);
  sem_init(&start_allocate,0,0);   
  
  for (int i = 1; i <= 8; ++i){
    leave = false;
    memcpy(arr_cpy, arr, sizeof(arr));
    output[i-1].open(("output_" + std::to_string(i) + ".txt").c_str(), std::ios::out); // create the output file
    if (output[i-1].fail()) {
      std::cout << "cannot make output_" << i << ".txt\n";
      exit(1);
    }
    finish_job_cnt = 0;  
    pthread_create(&master_thread,NULL,master,NULL); // create the master thread
    pthread_create(all_threads+i-1, NULL, work, NULL); // add new thread to the thread pool
    job_list.clear();
    job new_job = {0,number_of_input-1,0,1};
    job_list.insert(new_job);
    
    gettimeofday(&start,0);
    sem_post(&start_allocate); // signal the master to start
    sem_wait(&all_threads_done); // wait master to report
    gettimeofday(&end, 0);
    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;

    std::cout << i<< " threads: " << sec + usec/1000000.0 << "s\n"; // output the sorting time
    
    // write to the output_i file 
    for (int k = 0; k < number_of_input; ++k)
      output[i-1] << arr_cpy[k] << " ";
  }
  
  finish_eight_case = true; // let the 8 thread to exit
  return 0;
}
  
