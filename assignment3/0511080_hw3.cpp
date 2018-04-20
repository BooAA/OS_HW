#include <iostream>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <memory.h>
using namespace std;

// st stands for single thread, mt stands for multiple threads

sem_t semaphore[15]; // 15 semaphore
pthread_t threads[15] , single_thread; // 15 threads for mt and 1 thread for st
int finish_cnt = 0, finish_cnt_st = 0; // count the finish thread of mt and st
sem_t finish_cnt_lock; // protect each time only 1 thread change the counter
sem_t done,report,report_st; //report after finish mt and st 

// argument for partition and bubble_sort, used for both st and mt
struct sort_args
{
  int* array; // the array to be sorted
  int start,end; // indicate the first and last element inside a range of array to be sorted
  int thread_index; // the index of the thread
  bool st; // whether it's single thread case now
}args_array[15];
 
// swap 2 numbers
void swap(int* array, int j, int i)
{
  int tmp = *(array+i);
  *(array+i) = *(array+j);
  *(array+j) = tmp;
}

// bubble sort for a range of array(t8-t15)
void* bubble_sort(void* args)
{
  sort_args* this_args = (sort_args*)(args);
  if (!this_args->st) sem_wait(semaphore + this_args->thread_index-1);
  
  for (int i = this_args->start; i < this_args->end; ++i)
    for (int j = this_args->start; j < this_args->end - i + this_args->start; ++j)
      if (this_args->array[j] > this_args->array[j+1])
	swap(this_args->array,j,j+1);
   
  if (!this_args->st)
    {
      sem_wait(&finish_cnt_lock);
      finish_cnt++;
      if (finish_cnt == 8) sem_post(&done);
      sem_post(&finish_cnt_lock);
      pthread_exit(NULL);
    }
  else
    {
      // case for st: if counter == 8, report main
      if (this_args->thread_index < 15) bubble_sort(args_array+(this_args->thread_index)); // the last time do nothing
      finish_cnt_st++;
      if (finish_cnt_st == 8) sem_post(&report_st);
    }
}
// partition a range of array(t1-t7)
void* partition(void* args)
{
  sort_args* this_args = (sort_args*)(args); // type conversion to sort_args*
  
  // wait for the ith semaphore to wake it if it's the mt thread case
  if (!this_args->st) sem_wait(semaphore+ this_args->thread_index-1);
  
  int pivot_place = this_args->start;
  int pivot = *(this_args->array+pivot_place);
  int j = this_args->start;; // record the position of number <= pivot
  for (int i = this_args->start+1; i <= this_args->end; ++i )
    {
      if (*(this_args->array+i) <= pivot)
	{
	  j++;
	  swap(this_args->array,j,i);
	}
    }
  swap(this_args->array,j,pivot_place);

  // assign the arguments of next level thread
  args_array[(this_args->thread_index)*2 -1].start = this_args->start;
  args_array[(this_args->thread_index)*2 -1].end = j-1;
  args_array[(this_args->thread_index)*2].start = j+1;
  args_array[(this_args->thread_index)*2].end = this_args->end;
  
  if (!this_args->st)
    {
      // mt thread case: wake the next level threads up
      sem_post(semaphore + (this_args->thread_index)*2 -1);
      sem_post(semaphore + (this_args->thread_index)*2);
      if (this_args->thread_index == 1)
	{
	  // for t1, wait t8-t15 
	  sem_wait(&done);
	  sem_post(&report); // report the result to main
	}
      pthread_exit(NULL);
    }
  else
    {
      // st thread case, the only thread so the sorting recursively by itself
      if (this_args->thread_index < 7)
	partition(args_array+(this_args->thread_index));
      else 
	bubble_sort(args_array+(this_args->thread_index)); // case : thread index == 7, do bubble sort next time
    }
}

void init_args(const int array_size, int* array_of_mt)
{
  for (int i = 0; i < 15; ++i)
    {
      sem_init(semaphore+i,0,0);
      args_array[i].thread_index = i+1;
      args_array[i].array =array_of_mt;
      args_array[i].st = false;
    }
  args_array[0].start = 0;
  args_array[0].end = array_size-1;
  sem_init(&finish_cnt_lock,0,1);
  sem_init(&done,0,0);
  sem_init(&report,0,0);
  sem_init(&report_st,0,0);  
}

int main(int argc, char** argv)
{
  // read in the input file and init the args
  cout <<"please enter the file name?\nThe file must in the same directory:\n";
  char filename[20];
  cin >> filename;
  fstream input, output1,output2;
  input.open(filename,ios::in);
  if (input.fail()) cout<<"cannot open the input file!\n";
  int number_of_input = 0 , tmp = 0;
  input >> number_of_input;
  const int array_size = number_of_input;
  int input_array[array_size] = {0};
  int input_array_cpy[array_size] = {0};
  for (int i = 0; i < array_size; ++i)
    {
      input >> tmp;
      input_array[i] = tmp;
    }
  memcpy(input_array_cpy,input_array,sizeof(input_array));
  init_args(array_size,input_array);
  
  // create 15 thread, first 7 => partition, last 8 => bubble_sort
  for (int i = 0; i < 15; ++i)
    {
      if (i < 7)
	pthread_create(threads+i,NULL,partition,(void*)(args_array+i));
      else
	pthread_create(threads+i, NULL, bubble_sort, (void*)(args_array+i));
    }

  // start doing mt quicksort
  struct timeval start, end;
  int sec, usec;
  gettimeofday(&start,0);
  sem_post(semaphore);
  sem_wait(&report);
  gettimeofday(&end,0);

  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  cout << "MT: "<< sec+usec/1000000.0 << "sec\n";

  for (int i = 0; i < 15; ++i)
    sem_destroy(semaphore+i);
  sem_destroy(&report);
  sem_destroy(&done);
  sem_destroy(&finish_cnt_lock);
  
  // write the result of mt to output1.txt
  output1.open("output1.txt",ios::out);
  if (output1.fail()) cout<<"cannot make a output1.txt!\n";
  else
    {
      for (int i = 0; i < array_size; ++i)
	output1 << input_array[i] << " ";
      cout << "output1.txt write successfully\n";
    }
  // switch to st quicksort
  for (int i = 0; i < 15; ++i)
    {
      args_array[i].array = input_array_cpy; // change to another unsorted array
      args_array[i].st = true; // set st true, then in partition and bubble sort , it will do the st part
    }
  gettimeofday(&start,0);
  pthread_create(&single_thread,NULL,partition,(void*)(args_array));
  sem_wait(&report_st);
  gettimeofday(&end,0);
  sec = end.tv_sec - start.tv_sec;
  usec = end.tv_usec - start.tv_usec;
  cout << "ST: " <<sec+usec/1000000.0 << "sec\n";
  sem_destroy(&report_st);
  
  // write the result of st to output2.txt
  output2.open("output2.txt",ios::out);
  if (output2.fail()) cout << "cannot make output2.txt!!\n";
  else
    {
      for (int i = 0; i < array_size; ++i)
	output2 << input_array_cpy[i] << " ";
      cout <<"output2.txt write successfully\n";
    }
  return 0;
}
