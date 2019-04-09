#include "main.h"
int main(int argc, char *argv[])
{
	int status; 
	pid_t pid_in=0,pid_out=0;
	key_t key=0;

	/* input process fork */ 
	pid_in = fork();
    if(pid_in>0)
    {
        pid_out = fork();
        if(pid_out>0)
            main_proc();
        else
            output_proc();
    }
    else
    {
        input_proc();
    }
    return 0;

	/*if(pid_in==0){
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
	return 0;*/

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
	
	key_id_d = msgget((key_t)IN_AND_MAIN_D,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)IN_AND_MAIN_SW,IPC_CREAT|0666);
	//printf("%d %d\n",ftok("/home/embe/Desktop",'D'),ftok("/home/embe/Desktop",'S'));
	printf("%d %d\n",key_id_d,key_id_sw);
    int mode=1;
	while(1)
	{
		//printf("%d %d\n",key_id_d,key_id_sw);
		memset(&msgsend,0,sizeof(struct msgbuf));
		msgsend.msgtype=1;
        msgsend.text[1]=mode;
		int read_d = read(fd_device, event, size_ev * BUF_SIZE);
		if(read_d >= sizeof(event[0]))
		{
			if(event[0].value == KEY_PRESS){
				if(event[0].code == BACK_KEY){
					printf("in input back\n");
					msgsend.msgtype=1;
					msgsend.text[0]=TYPE_BACK;
					break;
				}
				else if(event[0].code == VOL_UP_KEY){
					printf("in input upup\n");
					msgsend.msgtype=1;
                    msgsend.text[0]=TYPE_VOL_UP;
                    mode++;
                    if(mode==6)
                        mode=1;
					msgsend.text[1]=mode;
				}
				else if(event[0].code == VOL_DOWN_KEY){
					printf("in input down\n");
					msgsend.msgtype=1;
                    msgsend.text[0]=TYPE_VOL_DOWN;
                    mode--;
                    if(mode==0) 
                        mode=5;
					msgsend.text[1]=mode;
				}
				memset(&event[0],0,sizeof(event[0]));
				msgsend.text[2]=KEY_PRESS;
				//event[0].value=KEY_RELEASE;	
			}
		}
		//printf("in input_proc msgrcv_d : %d %d %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0],msgrecv_d.text[1],msgrecv_d.text[2]);
		if(msgsnd(key_id_d,&msgsend,sizeof(struct msgbuf)-sizeof(long),IPC_NOWAIT)==-1)
			printf("in input_proc msgsnd_d error!\n");

		memset(sw,0,size_sw);
		int read_sw = read(fd_switch,&sw,MAX_SWITCH);
		if(read_sw<0){
			printf(" switch read error!\n");
			return -1;
		}
		memset(&msgsend2,0,sizeof(struct msgbuf));
		msgsend2.msgtype=TYPE_SWITCH;
		//printf("%d\n",TYPE_SWITCH);
		for(i=0;i<MAX_SWITCH;i++){
			printf("[%d] ",sw[i]);
			msgsend2.text[i] = sw[i];
		}
		printf("\n\n");
		if(msgsnd(key_id_sw,&msgsend2,sizeof(struct msgbuf)-sizeof(long),IPC_NOWAIT)==-1){
			printf("\nin input_proc msgsnd_sw error!\n");
		}
		//memset(&msgsend2,0,sizeof(struct msgbuf));
		//memset(&msgsend,0,sizeof(struct msgbuf));
		usleep(500000);
	}
	close(fd_device);
	close(fd_switch);
	usleep(10000000);

	return 0;
}
int main_proc()
{
	printf("In main_proc..\n");
	struct msgbuf msgrecv_d, msgrecv_sw;
	int back_flag=0, i;
	key_t key_id_d,key_id_sw;
	int msgtype;
	memset(&msgrecv_d,0,sizeof(struct msgbuf));
	key_id_d = msgget((key_t)IN_AND_MAIN_D,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)IN_AND_MAIN_SW,IPC_CREAT|0666);
    key_id_to_out_d = msgget((key_t)MAIN_AND_OUT_D,IPC_CREAT|0666);
    key_id_to_out_sw = msgget((key_t)MAIN_AND_OUT_SW,IPC_CREAT|0666);
    
	while(1){
		//memset(&msgrecv_d,0,sizeof(struct msgbuf));
		msgrcv(key_id_d,(void*)&msgrecv_d,sizeof(struct msgbuf),TYPE_DEVICE,IPC_NOWAIT);
		//printf("in main_proc msgrcv_d error!\n");
		printf("msgrecv_d type : %d\n",msgrecv_d.msgtype);
		printf("in main_proc msgrecv_d : %d, key press :%d\n",msgrecv_d.text[0],msgrecv_d.text[2]);

		if(msgrecv_d.text[2]==KEY_PRESS){
			/* mode change */
			if(msgrecv_d.text[0] == TYPE_VOL_UP){
				printf("in main vol_up\n");

			}
			if(msgrecv_d.text[0] == TYPE_VOL_DOWN){
				printf("in main vol_down\n");
			}
			/* terminate */
			if(msgrecv_d.text[0]==TYPE_BACK){
				break;
			}

		}
		//printf("in main_proc msgrcv_d : %d %d %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0],msgrecv_d.text[1],msgrecv_d.text[2]);
        
	    memset(&msgrecv_sw,0,sizeof(struct msgbuf));
		if((msgrcv(key_id_sw,(void*)&msgrecv_sw,sizeof(struct msgbuf),TYPE_SWITCH,IPC_NOWAIT))==-1)
			printf("in main_proc msgrcv_sw error!\n");
	
		if(msgrecv_sw.msgtype==TYPE_SWITCH){
			printf("in main_proc : ");
			for(i=0;i<MAX_SWITCH;i++){
				printf("%d ",msgrecv_sw.text[i]);
			}
			printf("\n");
            
			if(msgsnd(key_id_to_out_d,&msgrecv_d,sizeof(struct msgbuf)-sizeof(long),IPC_NOWAIT)==-1){
                printf("in main_proc msgsnd_d error!\n");
            }
			if(msgsnd(key_id_to_out_sw,&msgrecv_sw,sizeof(struct msgbuf)-sizeof(long),IPC_NOWAIT)==-1){
				printf("in main_proc msgsnd_sw error!\n");
			}
			
		}
		printf("\n");
		memset(&msgrecv_d,0,sizeof(struct msgbuf));
		memset(&msgrecv_sw,0,sizeof(struct msgbuf));
		usleep(500000);
	}
    msgctl(key_id_d,IPC_RMID,NULL);
	msgctl(key_id_sw,IPC_RMID,NULL);
	return 0;
}

int output_proc()
{
	printf("output_proc..\n");

	struct msgbuf msgrecv_sw, msgrecv_d;
	int i;
	key_id_from_main_d = msgget((key_t)MAIN_AND_OUT_D,IPC_CREAT|0666);
    key_id_from_main_sw = msgget((key_t)MAIN_AND_OUT_SW,IPC_CREAT|0666);

	while(1){
	    memset(&msgrecv_d,0,sizeof(struct msgbuf));
		if(msgrcv(key_id_from_main_d,&msgrecv_d,sizeof(struct msgbuf),TYPE_DEVICE,IPC_NOWAIT)==-1){
			printf("in output msgrecv_d error!\n");
		}
		if(msgrecv_d.msgtype==TYPE_BACK)
			break;
             
	    memset(&msgrecv_sw,0,sizeof(struct msgbuf));
		if(msgrcv(key_id_from_main_sw,&msgrecv_sw,sizeof(struct msgbuf),TYPE_SWITCH,IPC_NOWAIT)==-1){
			printf("in output msgrcv_sw error!\n");
		}
		//printf("in output_proc msgrcv_d : %d %d %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0],msgrecv_d.text[1],msgrecv_d.text[2]);
		//printf("in output_proc msgrcv_sw : ");
		/*for(i=0;i<MAX_SWITCH;i++){
			printf(" %d",msgrecv_sw.text[i]);
		}*/
		printf("\n");
        /* MODE 4 */
        if(msgrecv_d.text[1]==CLOCK){
            out_clock(msgrecv_sw);
        }
        else if(msgrecv_d.text[1]==COUNTER){
        }
        else if(msgrecv_d.text[1]==TEXT_EDITOR){
        }
        else if(msgrecv_d.text[1]==DRAW_BOARD){
        }
        else if(msgrecv_d.text[1]==EXTRA){
        }

		usleep(500000);
	}
    close(fd_fnd);
    msgctl(key_id_from_main_d,IPC_RMID,NULL);
	msgctl(key_id_from_main_sw,IPC_RMID,NULL);
	return 0;
}

int out_clock(struct msgbuf msgrecv_sw){
	char fnd[4]={0,};
    unsigned char sum_sw;
	static int change_toggle=0, init=0;
    static clock_t c,t;
	time_t present_time = time(NULL);
	static struct tm *pre_t, *lat_t;
	if(!init){
        int fd_fnd = open(FND_DEVICE,O_RDWR);
		pre_t = localtime(&present_time);
		init =1;
	}
	if(fd_fnd<0) printf("FND device open error!\n");
    unsigned long *fpga_addr =0;
	unsigned char *led_addr =0;

    fpga_addr = (unsigned long*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_led, FPGA_BASE_ADDRESS); 
    led_addr = (unsigned char*)((void*)fpga_addr+LED_ADDR);	


    sum_sw = check_sw(msgrecv_sw.text);
    if(sum_sw==0 && change_toggle==0){// not switch 
        present_time = time(NULL);
		lat_t = localtime(&present_time);
        if(lat_t->tm_sec - pre_t->tm_sec >= 1 || lat_t->tm_sec  - pre_t->tm_sec <=-1){ // later 1 sec
            pre_t = lat_t;
        }
    }
    if(sum_sw==256) {// switch 1 
        change_toggle ^=1;
        *led_addr=0;
    }

    if(change_toggle){// edit mode
        if(clock()-s>SEC){
            if(flag==0){
                *led_addr^=16;
                flag=1;
            }
            else{
                *led_addr^=48;
            }
            s =clock();
        }

        if(sum_sw==128){// switch 2
            present_time = time(NULL);
            pre_t = localtime(&present_time);
        }
        if(sum_sw==64){// switch 3
            pre_t->tm_min += 1;
        }
        if(sum_sw==32){// switch 4
            pre_t->tm_hour += 1;
        }
    }
    fnd[0]=pre_t->tm_hour/10;
    fnd[1]=pre_t->tm_hour%10;
    fnd[2]=pre_t->tm_min/10;
    fnd[3]=pre_t->tm_min%10;
    
    write(fd_fnd,&fnd,4);
    
	return 0;
}
unsigned char check_sw(unsigned char sw[]){
    int i;
    unsigned char sum=0;
    for(i=8;i>=0;i--){
        if(sw[i]==1){
            sum+=sw[i]*(1<<(8-i));
        }
    }
    return sum;
}
int out_counter(struct msgbuf msgrecv_sw)
{
    return 0;
}
