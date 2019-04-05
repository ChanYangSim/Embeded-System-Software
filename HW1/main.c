#include "main.h"
int main(int argc, char *argv[])
{
	int status; 
	pid_t pid_in=0,pid_out=0;
	key_t key=0;

	/* input process fork */ 
	pid_in = fork();
	if(pid_in==0){
		input_proc();

	}
	else{
		pid_out=fork();
		if(pid_out==0){
			output_proc();
		}
		else{
			main_proc();
			wait(&status);
		}
		wait(&status);
	}
	return 0;

	/*if(pid_in){//main process 
		pid_out = fork();
		if(pid_out<0)
			printf("fork faulure\n");
	}
	
		pid_out=-1;
		input_proc();
	}
	if(pid_out==0){//output process
		output_proc();
		wait(&status);
	}
	if(pid_in!=0 && pid_out!=0){//main process
		main_proc();
		wait(&status);
	}*/
}
int main_proc()
{
	printf("In main_proc..\n");
	struct msgbuf msgsend,msgrecv;
	int msgtype=3;
	key_t key_id_d,key_id_sw;
	memset(&msgrecv,0,sizeof(struct msgbuf));
	key_id_d = msgget((key_t)IN_AND_MAIN,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)IN_AND_MAIN+1,IPC_CREAT|0666);
	while(1){
		if(msgrecv.msgtype==TYPE_BACK) break;
		memset(&msgrecv,0,sizeof(struct msgbuf));
		msgrcv(key_id_d,(void*)&msgrecv,sizeof(struct msgbuf),TYPE_BACK,0);
		printf("msgrcv : %d %s\n",msgrecv.msgtype,msgrecv.text);
		msgrcv(key_id_sw,(void*)&msgrecv,sizeof(struct msgbuf),TYPE_SWITCH,0);
		printf("msgrcv : %d %s\n",msgrecv.msgtype,msgrecv.text);
	}
	return 0;
}
int input_proc(){
	printf("In input_proc..\n");
	int fd_device, fd_switch;
	int val;
	struct input_event event[BUF_SIZE];
	int size_ev=sizeof(struct input_event);
	struct msgbuf msgsend, msgrecv,msgsend2;
	key_t key_id_d, key_id_sw;
	/*  open key device  */
	fd_device = open(KEY_DEVICE,O_RDONLY | O_NONBLOCK);
	if(fd_device <0){
		printf("Device open error!\n");
		close(fd_device);
	}
	/* open switch key device */
	fd_switch = open(SWITCH_DEVICE, O_RDONLY | O_NONBLOCK);
	if(fd_switch < 0){
		printf("Switch open error!\n");
		close(fd_switch);
	}
	unsigned char sw[MAX_SWITCH];
	unsigned char prev_sw[MAX_SWITCH];
	unsigned char output_sw[MAX_SWITCH];
	int size_sw = sizeof(sw);
	memset(sw,0,size_sw); memset(prev_sw,0,size_sw); memset(output_sw,0,size_sw);

	(void)signal(SIGINT,user_signal1);


	key_id_d = msgget((key_t)4321,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)1234,IPC_CREAT|0666);

	while(1)
	{
		int read_d = read(fd_device, event, size_ev * BUF_SIZE);
		if(read_d >= sizeof(event[0]))
		{
		/* message queue set */
			if(event[0].value == KEY_PRESS){
				if(event[0].code == BACK_KEY){


					memset(&msgsend,0,sizeof(struct msgbuf));
					msgsend.msgtype=TYPE_BACK;
					msgsend.text[0]=1;
					if(msgsnd(key_id_d,(void*)&msgsend,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
						printf("msgsnd error!\n");
					}
					return -1;
				}
				else if(event[0].code == VOL_UP_KEY) printf("222\n");
				else if(event[0].code == VOL_DOWN_KEY) printf("333\n");	
			}
		}
		usleep(150000);
		memset(sw,0,size_sw);
		int read_sw = read(fd_switch,&sw,MAX_SWITCH);
		if(read_sw<0){
			printf(" switch read error!\n");
			return -1;
		}
		int i;


		memset(&msgsend2,0,sizeof(struct msgbuf));
		msgsend2.msgtype=TYPE_SWITCH;
		for(i=0;i<MAX_SWITCH;i++){
			msgsend.text[i]=sw[i];
			prev_sw[i]=sw[i];
			printf("[%d] ",sw[i]);
		}
		if(msgsnd(key_id_sw,(void*)&msgsend2,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
			printf("switch send error!\n");
		}
		
		usleep(150000);
	}
	close(fd_device);
	close(fd_switch);

	return 0;
}
int output_proc()
{
	printf("output_proc..\n");
	return 0;
}
