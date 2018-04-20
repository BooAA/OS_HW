#include <iostream>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <cmath>
using namespace std;

int shmid, shmid2,size;
unsigned int checksum = 0;

void initmatrix(const int size)
{
  unsigned int(*ptr)[size];
  unsigned int (*ptr2)[size];
  shmid  = shmget(IPC_PRIVATE, sizeof(unsigned int[size][size]),IPC_CREAT | 0666);
  shmid2 = shmget(IPC_PRIVATE, sizeof(unsigned int[size][size]),IPC_CREAT | 0666);
  ptr = (unsigned int(*)[size])shmat(shmid,NULL,0);
  ptr2 = (unsigned int(*)[size])shmat(shmid2,NULL,0);
  for (int i = 0; i < size; ++i)
    {
      for (int j = 0; j < size; ++j)
	{
	  ptr[i][j] = i*size+j;
	  ptr2[i][j] = 0;
	}
    }
  shmdt(ptr);
  shmdt(ptr2);
}


void multiply(int start , int end, const int size)
{
  unsigned int(*ptr)[size] =(unsigned int(*)[size])shmat(shmid,NULL,0);
  unsigned int (*ptr2)[size] =(unsigned int(*)[size])shmat(shmid2,NULL,0);
  for (int i = start; i <= end; ++i)
    {
      for (int j = 0; j < size; ++j)
	{
	  ptr2[i][j] = 0;
	  for (int k = 0; k < size; ++k)
	      ptr2[i][j] += ptr[k][j] * ptr[i][k];
	}
    }
  shmdt(ptr);
  shmdt(ptr2);

}


void create_process(const int size)
{
  struct timeval start,end;
  for (int i = 1; i <= 16; ++i)
    {
      gettimeofday(&start, 0);
      int range = size/i;
      for (int j = 0; j < i; ++j)
	{
	  pid_t pid = fork();
	  
	  if (pid < 0)
	    {
	      cout<<"error when create a fork\n";
	      exit(1);
	    }

	  else if (pid == 0) // child process
	    {
	      if (j == i-1)
		{
		  multiply(j*range, size-1, size);
		  exit(0);
		}
	      
	      multiply(j*range,(j+1)*range-1,size);
	      exit(0);
	    }
	}

      while(true)
	{
	  int status;
	  pid_t done = wait(&status);
	  if (done == -1)
	    {
	      if (errno == ECHILD) break;
	    }
	  else
	    {
	      if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
		{
		  cerr<<"pid"<<done<<" failed \n";
		}
	    }
	}
      
      gettimeofday(&end,0);

      
      int second = end.tv_sec - start.tv_sec;
      int usecond = end.tv_usec - start.tv_usec;

      if (i == 1) cout<<"Multiplying matrices using 1 process\n";
      else cout<<"Multiplying matrices using "<<i<<" processes\n";

      unsigned int (*ptr)[size] =(unsigned int(*)[size])shmat(shmid2,NULL,0);

      for (int x = 0; x < size; ++x)
	for (int y = 0; y < size; ++y)
	  checksum += ptr[x][y];
      
      cout<<"Elapsed time:"<<second+usecond/1000000.0<<"sec, Checksum :"<<checksum<<"\n";
      checksum = 0;
      shmdt(ptr);
 }
}

int main(int argc, char *argv[])
{
  cout<<"please input the size of array:";
  cin>>size;
  initmatrix(size);
  create_process(size);
  return 0;
}
