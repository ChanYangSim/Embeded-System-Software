#include "main.h"
#include "input_process.h"
#include "output_process.h"
int main()
{
	int status; 
	pid_t pid_in=0,pid_out=0;
	key_t key=0;

	/* input process fork */ 
	pid_in = fork();

	if(pid_in<0)
		printf("fork failure\n");
	if(pid_in){//main process 
		pid_out = fork();
		if(pid_out<0)
			printf("fork faulure\n");
	}
	if(pid_in==0){//input process
		pid_out=-1;
		input_proc();
	}
	if(pid_out==0){//output process
		output_proc();
	}
	if(pid_in!=0 && pid_out!=0){//main process
		pid_in = wait(&status);
		printf("INPUT PROCESS EXIT STATUS : %d\n",status);
		pid_out = wait(&status);
		printf("OUTPUT PROCESS EXIT STATUS : %d\n",status);
        main_process();    
		printf("main_process\n");	
	}
	return 0;
}
int main_process()
{
    struct msgbuf msgsnd,msgrcv;
    while()
}
