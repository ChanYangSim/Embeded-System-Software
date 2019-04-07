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

	key_id_d = msgget((key_t)ftok("/tmp",'D'),IPC_CREAT|0666);
	key_id_sw = msgget((key_t)ftok("/tmp",'S'),IPC_CREAT|0666);
    int mode=1;
	while(1)
	{
		int read_d = read(fd_device, event, size_ev * BUF_SIZE);
		if(read_d >= sizeof(event[0]))
		{
			msgsend.msgtype=0;
            msgsend.text[0]=0;
            msgsend.text[1]=mode;
		    /*  */
			if(event[0].value == KEY_PRESS){
				if(event[0].code == BACK_KEY){
					printf("back\n");
					msgsend.msgtype=TYPE_BACK;
					msgsend.text[0]=1;
					break;
				}
				else if(event[0].code == VOL_UP_KEY){
					printf("upup\n");

					msgsend.msgtype=TYPE_VOL_UP;
                    msgsend.text[0]=0;
                    mode++;
                    if(mode==6)
                        mode=1;
					msgsend.text[1]=mode;
				}
				else if(event[0].code == VOL_DOWN_KEY){
					printf("down\n");
					msgsend.msgtype=TYPE_VOL_DOWN;
                    msgsend.text[0]=0;
                    mode--;
                    if(mode==0) 
                        mode=5;
					msgsend.text[1]=mode;
				}
				if(msgsnd(key_id_d,&msgsend,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
                    printf("in input_proc msgsnd_d error!\n");
                }
			}
		}

		//printf("in input_proc msgrcv_d : %d %d %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0],msgrecv_d.text[1],msgrecv_d.text[2]);

		memset(sw,0,size_sw);
		int read_sw = read(fd_switch,&sw,MAX_SWITCH);
		if(read_sw<0){
			printf(" switch read error!\n");
			return -1;
		}
		memset(&msgsend2,0,sizeof(struct msgbuf));
		msgsend2.msgtype=TYPE_SWITCH;
		printf("in input_proc ");
		//printf("%d\n",TYPE_SWITCH);
		for(i=0;i<MAX_SWITCH;i++){
			printf("[%d] ",sw[i]);
			msgsend2.text[i] = sw[i];
			//prev_sw[i]=sw[i];
		}
		if(msgsnd(key_id_sw,&msgsend2,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
			printf("\nin input_proc msgsnd_sw error!\n");
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
	struct msgbuf msgsend, msgrecv_d, msgrecv_sw;
	int back_flag=0, i, msgtype=3;
	key_t key_id_d,key_id_sw;
	memset(&msgrecv_d,0,sizeof(struct msgbuf));
	key_id_d = msgget((key_t)ftok("/tmp",'D'),IPC_CREAT|0666);
	key_id_sw = msgget((key_t)ftok("/tmp",'S'),IPC_CREAT|0666);
	while(back_flag != TYPE_BACK){

        /* mode change */
		if(msgrecv_d.msgtype == TYPE_VOL_UP){
			printf("vol_up\n");
            if(
		}
		if(msgrecv_d.msgtype == TYPE_VOL_DOWN){
			printf("vol_down\n");
		}
        /* terminate */
		if(msgrecv_d.msgtype==TYPE_BACK){
			back_flag=1;
		}
		memset(&msgrecv_d,0,sizeof(struct msgbuf));
		if(msgrcv(key_id_d,&msgrecv_d,sizeof(struct msgbuf),0,IPC_NOWAIT)==-1){
			printf("in main_proc msgrcv_d error!\n");
		}
        
		//printf("in main_proc msgrcv_d : %d %d %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0],msgrecv_d.text[1],msgrecv_d.text[2]);
        
	    memset(&msgrecv_sw,0,sizeof(struct msgbuf));
		if((msgrcv(key_id_sw,&msgrecv_sw,sizeof(struct msgbuf),0,IPC_NOWAIT))==-1)
			printf("in main_proc msgrcv_sw error!\n");
	

		if(msgrecv_sw.msgtype==TYPE_SWITCH){
			msgsend.msgtype=TYPE_SWITCH;
			printf("in main_proc : ");
			for(i=0;i<MAX_SWITCH;i++){
				printf("%d ",msgrecv_sw.text[i]);
			}
			printf("\n");
            
			if(msgsnd(key_id_d,&msgrecv_d,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
                printf("in main_proc msgsnd_d error!\n");
            }
			if(msgsnd(key_id_sw,&msgrecv_sw,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
				printf("in main_proc msgsnd_sw error!\n");
			}
			
		}
		usleep(500000);
	}
	return 0;
}

int output_proc()
{
	printf("output_proc..\n");

    

	struct msgbuf msgrecv_sw, msgrecv_d;
	int i;
	key_t key_id_d = msgget((key_t)ftok("/tmp",'D'),IPC_CREAT|0666);
	key_t key_id_sw = msgget((key_t)ftok("/tmp",'S'),IPC_CREAT|0666);
	

	while(1){
	    memset(&msgrecv_d,0,sizeof(struct msgbuf));
		if(msgrcv(key_id_sw,&msgrecv_d,sizeof(struct msgbuf),0,IPC_NOWAIT)==-1){
			printf("in output msgrecv_d error!\n");
		}
		if(msgrecv_d.msgtype==TYPE_BACK)
			break;
             
	    memset(&msgrecv_sw,0,sizeof(struct msgbuf));
		if(msgrcv(key_id_sw,&msgrecv_sw,sizeof(struct msgbuf),0,IPC_NOWAIT)==-1){
			printf("in output msgrcv_sw error!\n");
		}
		//printf("in output_proc msgrcv_d : %d %d %d %d\n",msgrecv_d.msgtype,msgrecv_d.text[0],msgrecv_d.text[1],msgrecv_d.text[2]);
		//printf("in output_proc msgrcv_sw : ");
		/*for(i=0;i<MAX_SWITCH;i++){
			printf(" %d",msgrecv_sw.text[i]);
		}*/
		printf("\n");
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
	return 0;
}

int out_clock(struct msgbuf msgrecv_sw){
	char fnd[4]={0,};
	static int clock_edit=0, init=0;
    int i;
	time_t present_time = time(NULL);
	static struct tm *pre_t;
	if(!init){
		pre_t = localtime(&present_time);
		init =1;
	}
    unsigned long *fpga_addr =0;
	unsigned char *led_addr =0;

    int fd_fnd = open(FND_DEVICE,O_RDWR);
    int fd_led = open(LED_DEVICE,O_RDWR);
	if(fd_fnd<0) printf("FND device open error!\n");
	if(fd_led<0) printf("LED device open error!\n");

    fpga_addr = (unsigned long*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_led, FPGA_BASE_ADDRESS); 
    led_addr = (unsigned char*)((void*)fpga_addr+LED_ADDR);	
	*led_addr=128;

    
    bool sw_flag =false;
    for(i=0;i<MAX_SWITCH;i++){
        if(msgrecv_sw.text[i]!=0){
            sw_flag=true;
            break;
        }
    }
    //no sw input
    if(sw_flag==false){
        fnd[0]=pre_t->tm_hour/10;
        fnd[1]=pre_t->tm_hour%10;
        fnd[2]=pre_t->tm_min/10;
        fnd[3]=pre_t->tm_min%10;
        write(fd_fnd,&fnd,4);
    }
    
    //handle sw input
    if(sw_flag==true){
        if(msgrecv_sw.text[0]==1){

    }
        


	return 0;
}
