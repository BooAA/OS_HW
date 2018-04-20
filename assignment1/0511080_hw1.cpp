#include<iostream>
#include<cstring>
#include<unistd.h>
#include <sys/wait.h>
#include<signal.h>
#include <fcntl.h> // for O_RDWR
using namespace std;


char *cmd1[10], *cmd2[10] , buf[1024];
int len_cmd1 = 0, len_cmd2 = 0;
bool whether_pipe = false , to_wait = true , ioredir = false , iogetname = false;
char * filename;
void split(char* input) // split 2 command into cmd1 and cmd2 (if has "|", mark "whether_pipe" true
{
	strtok(input,"\n");
	
	for (char* ptr = strtok(input," "); ptr != NULL; ptr = strtok(NULL," "))
	{
		if (strcmp(ptr,"|") == 0) 
		{
			whether_pipe = true;
			continue;
		}
		else if (strcmp(ptr,">") == 0)
		{
			ioredir = true;
			continue;
		}
		
		else if (ioredir && !iogetname) 
		{
			filename = ptr;
			iogetname = true;
			continue;
		}
		
		else if (strcmp(ptr,"&") == 0)
		{
			to_wait = false;
			continue;
		}
		
		
		if (whether_pipe) 
		{
			cmd2[len_cmd2] = ptr;
			len_cmd2++;
		}
		else 
		{
			cmd1[len_cmd1] = ptr;
			len_cmd1++;
		}
	}
	
	cmd1[len_cmd1] = NULL;
	cmd2[len_cmd2] = NULL;
	
}


int main(int argc, char** argv)
{
	while(1)
	{
		cout<<">";
		fgets(buf,sizeof(buf),stdin);
		split(buf);
		
		pid_t pid1 , pid2; // pid1 for children , pid2 for another children
		int pipefd[2] , status , file_fd;
		pipe(pipefd);
		pid1 = fork();
		signal(SIGCHLD, SIG_IGN);
		
		if (pid1 < 0)
		{
			cout<<"first fork error";
			exit(0);
		}
		else if (pid1 == 0) // children's part
		{
			if (whether_pipe)
			{	
				close(pipefd[0]); // children's input is useless
				dup2(pipefd[1],1); // change it's output 
				close(pipefd[1]);
			}
			
			else if (ioredir)
			{
				file_fd = open(filename,O_WRONLY|O_CREAT,0666);
				dup2(file_fd,1);
				close(file_fd);
			}
			
			execvp(cmd1[0],cmd1);
		}
		
		else  // parent's part
		{
			if (whether_pipe)
			{	

				pid2 = fork();
				
				if (pid2 < 0)
				{
					cout<<"second fork error";
					exit(0);
				}
				else if (pid2 == 0)
				{
					close(pipefd[1]);
					dup2(pipefd[0],0); // change input from stdin to pipe-read
					close(pipefd[0]);
					execvp(cmd2[0],cmd2);					
				}
				else 
				{
					close(pipefd[0]);				
					close(pipefd[1]);
					if (to_wait) waitpid(pid1,&status,0);
					if (to_wait) waitpid(pid2,&status,0);
				}
				
			}
			else 
			{	
				close(pipefd[0]);
				close(pipefd[1]);				
				if (to_wait) waitpid(pid1,&status,0);
			}
			
		}


		whether_pipe = false;
		to_wait = true;
		len_cmd1 = 0;
		len_cmd2 = 0;
		ioredir = false;
		iogetname = false;
		memset(cmd1,0,sizeof(cmd1));
		memset(cmd2,0,sizeof(cmd2));
		buf[0] = 0;
	}
	cout<<"proceess kill\n";

	
	return 0;
}