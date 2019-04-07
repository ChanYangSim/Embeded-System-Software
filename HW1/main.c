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
int input_proc(){
	printf("In input_proc..\n");
	int fd_device, fd_switch;
	int val,i;
	struct input_event event[BUF_SIZE];
	int size_ev = sizeof(struct input_event);
	struct msgbuf msgsend, msgrecv, msgsend2;
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

	key_id_d = msgget((key_t)4000,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)2000,IPC_CREAT|0666);
	while(1)
	{
		int read_d = read(fd_device, event, size_ev * BUF_SIZE);
		if(read_d >= sizeof(event[0]))
		{
			memset(&msgsend,0,sizeof(struct msgbuf));
		/* message queue set */
			if(event[0].value == KEY_PRESS){
				if(event[0].code == BACK_KEY){
					printf("back\n");

					msgsend.msgtype=TYPE_BACK;
					msgsend.text[0]=1;
					if(msgsnd(key_id_d,&msgsend,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
						//printf("device_key msgsnd error!\n");
					}
					break;
				}
				else if(event[0].code == VOL_UP_KEY){
					printf("upup\n");

					msgsend.msgtype=TYPE_VOL_UP;
					msgsend.text[1]=1;
					msgsnd(key_id_d,&msgsend,sizeof(struct msgbuf),IPC_NOWAIT);
				}
				else if(event[0].code == VOL_DOWN_KEY){
					printf("down\n");

					msgsend.msgtype=TYPE_VOL_DOWN;
					msgsend.text[2]=1;
					msgsnd(key_id_d,&msgsend,sizeof(struct msgbuf),IPC_NOWAIT);
				}
			}
		}
		memset(sw,0,size_sw);
		int read_sw = read(fd_switch,&sw,MAX_SWITCH);
		if(read_sw<0){
			printf(" switch read error!\n");
			return -1;
		}
		memset(&msgsend2,0,sizeof(struct msgbuf));
		msgsend2.msgtype=TYPE_SWITCH;
		printf("dsadsa\n");
		//printf("%d\n",TYPE_SWITCH);
		for(i=0;i<MAX_SWITCH;i++){
			printf("[%d] ",sw[i]);
			msgsend2.text[i] = sw[i];
			//prev_sw[i]=sw[i];
		}
		if(msgsnd(key_id_sw,&msgsend2,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
			printf("ininput switch msgsnd error!\n");
		}
		usleep(500000);
	}
	close(fd_device);
	close(fd_switch);

	return 0;
}
int main_proc()
{
	printf("In main_proc..\n");
	struct msgbuf msgsend,msgrecv;
	int back_flag=0, i, msgtype=3;
	key_t key_id_d,key_id_sw;
	memset(&msgrecv,0,sizeof(struct msgbuf));
	key_id_d = (msgget((key_t)4000,IPC_CREAT|0666));
	key_id_sw = msgget((key_t)2000,IPC_CREAT|0666);
	while(back_flag != TYPE_BACK){

		if(msgrecv.msgtype == TYPE_VOL_UP){
			printf("vol_up\n");
		}
		if(msgrecv.msgtype == TYPE_VOL_DOWN){
			printf("vol_down\n");
		}
		if(msgrecv.msgtype==TYPE_BACK){
			back_flag=1;
		}
		memset(&msgrecv,0,sizeof(msgrecv));
		if(msgrcv(key_id_d,&msgrecv,sizeof(struct msgbuf),0,IPC_NOWAIT)==-1){
			printf("in main msgrcv_d error!\n");
		}
		//printf("msgrcv_d : %d %d %d %d\n",msgrecv.msgtype,msgrecv.text[0],msgrecv.text[1],msgrecv.text[2]);
			
		if((msgrcv(key_id_sw,&msgrecv,sizeof(struct msgbuf),TYPE_SWITCH,IPC_NOWAIT))==-1)
			printf("in main msgrcv_sw error!\n");
		
	/*	printf("msgrcv_sw : %d   ",msgrecv.msgtype);
		for(i=0;i<9;i++){
			printf("%d ",msgrecv.text[i]);
		}
			printf("\n");*/
		if(msgrecv.msgtype==TYPE_SWITCH){
			msgsend.msgtype=TYPE_SWITCH;
			printf("in main : ");
			for(i=0;i<MAX_SWITCH;i++){
				msgsend.text[i]=msgrecv.text[i];
				printf("%d ",msgsend.text[i]);
			}
			printf("\n");
			if(msgsnd(key_id_sw,&msgsend,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
				printf("in main msgsnd_sw error!\n");
			}
			
		}
		usleep(500000);
	}
	return 0;
}

int output_proc()
{
	printf("output_proc..\n");
	int fd_fnd = open(FND_DEVICE,O_RDWR);
	struct msgbuf msgrecv_sw, msgrecv_d;
	int i;
	if(fd_fnd<0) printf("FND device open error!\n");

	int key_id_d = msgget((key_t)3000,IPC_CREAT|0666);
	int key_id_sw = msgget((key_t)2000,IPC_CREAT|0666);
	

	while(1){
		
		if(msgrcv(key_id_sw,&msgrecv_d,sizeof(struct msgbuf),0,IPC_NOWAIT)==-1){
			printf("in output msgrecv_d error1\n");
		}
		if(msgrecv_d.msgtype==TYPE_BACK)
			break;
		if(msgrcv(key_id_sw,&msgrecv_sw,sizeof(struct msgbuf),0,IPC_NOWAIT)==-1){
			printf("in output msgrcv_sw error!\n");
		}
		//printf("in output %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0]);
		//printf("output : ");
		/*for(i=0;i<MAX_SWITCH;i++){
			printf(" %d",msgrecv_sw.text[i]);
		}*/
		printf("\n");
		out_clock(msgrecv_sw);
		usleep(500000);
	}
	return 0;
}

int out_clock(struct msgbuf msgrecv){
	unsigned char fnd[4]={0};
	static int clock_edit=0, init=0;

	time_t present_time = time(NULL);
	static struct tm time_info;
	if(!init){
		time_info = *localtime(&present_time);
		init =1;
	}
	unsigned char *led_addr;
	*led_addr=128;
	int hours, minutes, seconds;
	time(&present_time);
	//printf("h:%d m:%d s:%d\n",time_info.tm_hour,time_info.tm_min,time_info.tm_sec);
	//if(msgrecv.msgtype== TYPE_SWITCH)
	//{

	return 0;
}
