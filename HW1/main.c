#include "main.h"
int main(int argc, char *argv[])
{
	int status; 
	pid_t pid_in=0,pid_out=0;
	key_t key=0;
	/* input process fork */ 
	pid_in = fork();
    if(pid_in>0){
		/* output process fork */
        pid_out = fork();
        if(pid_out>0)
            main_proc();
        else
            output_proc();
    }
    else{
        input_proc();
    }
    return 0;
}
int input_proc(){
	printf("In input_proc..\n");
	int fd_device, fd_switch;
	int val,i;
	struct input_event event[BUF_SIZE]; // event structure
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
	memset(sw,0,size_sw); memset(prev_sw,0,size_sw); memset(output_sw,0,size_sw); // initialize input variable

	(void)signal(SIGINT,user_signal1);
	/* get message queue id */
	key_id_d = msgget((key_t)IN_AND_MAIN_D,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)IN_AND_MAIN_SW,IPC_CREAT|0666);
	int key_id_to_out_d = msgget((key_t)MAIN_AND_OUT_D,IPC_CREAT|0666);
    int key_id_to_out_sw = msgget((key_t)MAIN_AND_OUT_SW,IPC_CREAT|0666);
	
    int mode=1;
	while(1)
	{
		memset(&msgsend,0,sizeof(struct msgbuf));
		msgsend.msgtype=1;
        msgsend.text[1]=mode;
		// text[1] == board mode
		int read_d = read(fd_device, event, size_ev * BUF_SIZE); //read from board
		if(read_d >= sizeof(event[0]))
		{
			if(event[0].value == KEY_PRESS){
				if(event[0].code == BACK_KEY){ // BACK => terminate 
					/* message queue delete */
					msgctl(key_id_d,IPC_RMID,NULL);
					msgctl(key_id_sw,IPC_RMID,NULL);
    				msgctl(key_id_to_out_d,IPC_RMID,NULL);
    				msgctl(key_id_to_out_sw,IPC_RMID,NULL);
					printf("Bye Bye\n");
					msgsend.msgtype=1;
					msgsend.text[0]=TYPE_BACK;
					break;
				}
				else if(event[0].code == VOL_UP_KEY){ // VOL+ 
					msgsend.msgtype=1;
                    msgsend.text[0]=TYPE_VOL_UP;
					msgsend.text[2]=mode;
                    mode++;
                    if(mode==6)
                        mode=1;
					msgsend.text[1]=mode;
				}
				else if(event[0].code == VOL_DOWN_KEY){ // VOL-
					msgsend.msgtype=1;
                    msgsend.text[0]=TYPE_VOL_DOWN;
					msgsend.text[2]=mode;
                    mode--;
                    if(mode==0) 
                        mode=5;
					msgsend.text[1]=mode;
				}
				memset(&event[0],0,sizeof(event[0]));
			}
		}
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
		for(i=0;i<MAX_SWITCH;i++){ // store switch info at sw[]
			msgsend2.text[i] = sw[i];
		}
		printf("\n\n");
		if(msgsnd(key_id_sw,&msgsend2,sizeof(struct msgbuf)-sizeof(long),IPC_NOWAIT)==-1){
			printf("\nin input_proc msgsnd_sw error!\n");
		}
		usleep(500000); // slow down for read speed
	}
	// close device
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
	key_t key_id_d,key_id_sw,key_id_to_out_d,key_id_to_out_sw;
	int msgtype;
	memset(&msgrecv_d,0,sizeof(struct msgbuf));
	key_id_d = msgget((key_t)IN_AND_MAIN_D,IPC_CREAT|0666);
	key_id_sw = msgget((key_t)IN_AND_MAIN_SW,IPC_CREAT|0666);
    key_id_to_out_d = msgget((key_t)MAIN_AND_OUT_D,IPC_CREAT|0666);
    key_id_to_out_sw = msgget((key_t)MAIN_AND_OUT_SW,IPC_CREAT|0666);
	while(1){
		msgrcv(key_id_d,(void*)&msgrecv_d,sizeof(struct msgbuf),TYPE_DEVICE,IPC_NOWAIT); // recieve from message queue 
		
        if(msgrecv_d.text[0]==TYPE_BACK){ // terminate
            printf("in main_proc back!\n");
            break;
        }

        memset(&msgrecv_sw,0,sizeof(struct msgbuf));
		if((msgrcv(key_id_sw,(void*)&msgrecv_sw,sizeof(struct msgbuf),TYPE_SWITCH,IPC_NOWAIT))==-1)
			printf("in main_proc msgrcv_sw error!\n");
		msgrecv_sw.sum_sw=0;
		for(i=0;i<MAX_SWITCH;i++)
			msgrecv_sw.sum_sw += (int)msgrecv_sw.text[8-i]*(1<<i);


		if(msgrecv_sw.msgtype==TYPE_SWITCH){ // get type of message is SWITCH
            // message send to output process
			//printf("%d \n",msgrecv_sw.msgtype);
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
	return 0;
}

int output_proc()
{
	printf("output_proc..\n");

	struct msgbuf msgrecv_sw, msgrecv_d;
	int i;
	unsigned char sw[MAX_SWITCH];
	static unsigned char fnd[4]; memset(fnd,0,4);
	int key_id_from_main_d = msgget((key_t)MAIN_AND_OUT_D,IPC_CREAT|0666);
    int key_id_from_main_sw = msgget((key_t)MAIN_AND_OUT_SW,IPC_CREAT|0666);
	int fd_fnd, fd_led, fd_lcd, fd_dot, fd_mot, change_mode=0;
	unsigned char lcd[32],dot[10]={0,}; memset(lcd,' ',32);
	static int init_1=0,init_2=0,init_3=0,init_4=0,back_count=0;
    // driver open and setting 
	fd_fnd = open(FND_DEVICE,O_RDWR);
	fd_led = open("/dev/mem",O_RDWR | O_SYNC);
    fd_lcd = open(LCD_DEVICE,O_WRONLY);
    fd_dot = open(DOT_DEVICE,O_WRONLY);
    unsigned long *fpga_addr =0;
	unsigned char *led_addr =0;
	static unsigned char pre_mode=0;
    // led using mmap()
    fpga_addr = (unsigned long*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd_led, FPGA_BASE_ADDRESS); 
    led_addr = (unsigned char*)((void*)fpga_addr+LED_ADDR);	
	

	while(1){
	    memset(&msgrecv_d,0,sizeof(struct msgbuf));
        // message queue of type_device received
		if(msgrcv(key_id_from_main_d,&msgrecv_d,sizeof(struct msgbuf),TYPE_DEVICE,IPC_NOWAIT)==-1){
			printf("in output msgrecv_d error!\n");
		}
		if(msgrecv_d.text[0]==TYPE_BACK){ // terminate
			printf("out_proc back!!\n");
			break;
		}
             
	    memset(&msgrecv_sw,0,sizeof(struct msgbuf));
        // message queue of type_switch received
		if(msgrcv(key_id_from_main_sw,&msgrecv_sw,sizeof(struct msgbuf),TYPE_SWITCH,IPC_NOWAIT)==-1){
			back_count++;
			printf("in output msgrcv_sw error!\n");
			if(back_count>=3){ // if 3 more error then setting device zero and break => terminate
				*led_addr=0; 
				write(fd_fnd,fnd,4);
				write(fd_lcd,lcd,8);
				write(fd_dot,dot,10);
				break;
			}
		}
        /* FUNCTION by MODE */
		for(i=0;i<MAX_SWITCH;i++){
			sw[i]=msgrecv_sw.text[i];
		}
		//for(i=0;i<4;i++)
		//	fnd[i]=0;
		printf("now_mode : %d\n",msgrecv_d.text[1]);
		if(msgrecv_d.text[1]==CLOCK){ // MODE == 1
			if(CLOCK != pre_mode){
				*led_addr=0; write(fd_fnd,fnd,4); write(fd_lcd,lcd,32); write(fd_dot,dot,10);}// if change mode then initialize
            out_clock(sw,fd_fnd,led_addr,msgrecv_sw.sum_sw);
        }
		else if(msgrecv_d.text[1]==COUNTER){ // MODE == 2
			if(COUNTER != pre_mode){*led_addr=64; write(fd_fnd,fnd,4); write(fd_lcd,lcd,32); write(fd_dot,dot,10);change_mode=1;} // if change mode then initialize
			out_counter(sw,fd_fnd,led_addr,change_mode,msgrecv_sw.sum_sw);
			change_mode=0;
        }
        else if(msgrecv_d.text[1]==TEXT_EDITOR){ // MODE == 3
			 if(TEXT_EDITOR != pre_mode){*led_addr=0;write(fd_fnd,fnd,4);write(fd_lcd,lcd,32); write(fd_dot,dot,10);change_mode=1;} // if change mode then initialize
            out_text_editor(sw,fd_fnd,fd_lcd,fd_dot,led_addr,change_mode,msgrecv_sw.sum_sw);
			 change_mode=0;
        }
        else if(msgrecv_d.text[1]==DRAW_BOARD){ // MODE == 4
			if(DRAW_BOARD != pre_mode){*led_addr=0; write(fd_fnd,fnd,4);write(fd_lcd,lcd,32); write(fd_dot,dot,10);} // if change mode then initialize
			out_draw_board(sw,fd_fnd,fd_dot,msgrecv_sw.sum_sw);
        }
        else if(msgrecv_d.text[1]==EXTRA){ // MODE == 5
			if(DRAW_BOARD != pre_mode){*led_addr=0; write(fd_fnd,fnd,4);write(fd_lcd,lcd,32); write(fd_dot,dot,10);} // if change mode then initialize
        }
		pre_mode=msgrecv_d.text[1]; // store latest mode
		printf("pre_mode: %d\n",pre_mode);
		usleep(500000);
	}
    //close(fd_fnd);
	return 0;
}

int out_clock(unsigned char sw[],int fd_fnd, char* led_addr,int sum_sw){

	char fnd[4]={0,};
	static int change_toggle=0, init=0,led_flag=0,flag=0,min=0,hour=0,sec=0,t_hour,t_min;
    static clock_t s,t;
	time_t present_time=time(NULL); // present time stamp 
	
	static struct tm *pre_t, *lat_t;
	int i;
	if(!init){ 
		pre_t = localtime(&present_time);
		init =1;
	}
    if(sum_sw==0 && change_toggle==0){// din't push any switch
        present_time = time(NULL);
		
		pre_t = localtime(&present_time);
		if(!led_flag){
			*led_addr = 128;
			s=clock();
		}
		*led_addr = 128;
		led_flag=1;
		
    }
    if(sum_sw==256) {// switch 1
        change_toggle ^=1; // reverse toggle
		if(change_toggle==1){ // in edit mode
        	*led_addr=0;
        }
		else{ // out edit mode
			*led_addr=128;
			flag=0;
		}
    }
    if(change_toggle){// time edit mode
		t_hour = pre_t->tm_hour;
		t_min = pre_t->tm_min;
        if((clock() - s) > 200){ // compare now time and previous time stored 
			printf("clock() -s : %d\n",clock() -s );

            s = clock();
            if(flag==0){
                *led_addr^=16; // led4 setting
                flag=1;
            }
            else{
                *led_addr^=48; //  flicker exclusive OR led 3 and led 4
            }
        }

        if(sum_sw==128){// switch 2 => initialize board time 
            present_time = time(NULL);
            pre_t = localtime(&present_time);
			t_min = pre_t->tm_min; min=0;
			t_hour = pre_t->tm_hour; hour=0;
        }
        if(sum_sw==64){// switch 3
            min += 1; // miniute ++
			if(t_min+min==60) { min=-t_min; hour++;}
        }
        if(sum_sw==32){// switch 4
            hour += 1; // hour ++
			if(t_hour+hour==25) hour=-t_hour;
        }
        // not realtime 
		fnd[0]=(t_hour+hour)/10;
		fnd[1]=(t_hour+hour)%10;
		fnd[2]=(t_min+min)/10;
		fnd[3]=(t_min+min)%10;
		write(fd_fnd,&fnd,4);
		
		return 0;
	}
    // realtime
	fnd[0]=(pre_t->tm_hour+hour)/10;
	fnd[1]=(pre_t->tm_hour+hour)%10;
	fnd[2]=(pre_t->tm_min+min)/10;
	fnd[3]=(pre_t->tm_min+min)%10;
	write(fd_fnd,&fnd,4);
	return 0;
}
int out_counter(unsigned char sw[], int fd_fnd, char* led_addr,int change_mode, int sum_sw) 
{
    int i;
	static char fnd[4]={0,};
    static int init=0,t_mode=1;
    int sum;
	printf("cc %d\n",change_mode);
	if(change_mode==1){
		printf("change_mode \n");
		for(i=0;i<4;i++) fnd[i]=0;
		write(fd_fnd,&fnd,4);
	}
    if(!init){
        for(i=0;i<4;i++){
            fnd[i]=0;
        }
        write(fd_fnd,&fnd,4);
        *led_addr = 64;
        init=1;
    }

    if(sum_sw == 256){ // switch 1 => change jinsu
        t_mode++;
        sum=0;
        if(t_mode==2){ // Dec to Otc
            *led_addr = 32;
            sum+=fnd[1]*100; sum+=fnd[2]*10; sum+=fnd[3];
            fnd[0]=sum/512; sum%=512;
            fnd[1]=sum/64; sum%=64;
            fnd[2]=sum/8; sum%=8;
            fnd[3]=sum;
            fnd[0]=0;
            
        }
        else if(t_mode==3){ // Otc to Quai
            *led_addr = 16;
            sum+=fnd[1]*64; sum+=fnd[2]*8; sum+=fnd[3];
            fnd[0]=sum/64; sum%=64;
            fnd[1]=sum/16; sum%=16;
            fnd[2]=sum/4; sum%=4;
            fnd[3]=sum;
            fnd[0]=0;
        }
        else if(t_mode==4){ // Qua to Bin
            *led_addr = 128;
            sum+=fnd[1]*16; sum+=fnd[2]*4; sum+=fnd[3];
            fnd[0]=sum/8; sum%=8;
            fnd[1]=sum/4; sum%=4;
            fnd[2]=sum/2; sum%=2;
            fnd[3]=sum;
            fnd[0]=0;
        }
        else if(t_mode==5) { // Bin to Dec
            t_mode=1;
            *led_addr = 64;
            sum+=fnd[1]*4; sum+=fnd[2]*2; sum+=fnd[3];
            fnd[1]=sum/100; sum%=100;
            fnd[2]=sum/10; sum%=10;
            fnd[3]=sum;
            fnd[0]=0;
        }
    };
    if(t_mode==1){ // Decimal 
        if(sum_sw==128){ // switch 2
            fnd[1]++;
        }
        else if(sum_sw==64){ // switch 3
            fnd[2]++;
        }
        else if(sum_sw==32){ // switch 4
            fnd[3]++;
        }
        trim_number(fnd,10);
    }
    else if(t_mode==2){ // Octal
        if(sum_sw==128){ // switch 2
            fnd[1]++;
        }
        else if(sum_sw==64){ // switch 3
            fnd[2]++;
        }
        else if(sum_sw==32){ // switch 4
            fnd[3]++;
        }
        trim_number(fnd,8);
    }
    else if(t_mode==3){ // quadruple
        if(sum_sw==128){ // switch 2
            fnd[1]++;
        }
        else if(sum_sw==64){ // switch 3
            fnd[2]++;
        }
        else if(sum_sw==32){ // switch 4
            fnd[3]++;
        }
        trim_number(fnd,4);
    }
    else if(t_mode==4){ // binary
        if(sum_sw==128){ // switch 2
            fnd[1]++;
        }
        else if(sum_sw==64){ // switch 3
            fnd[2]++;
        }
        else if(sum_sw==32){ // switch 4
            fnd[3]++;
        }
        trim_number(fnd,2);
    }
    write(fd_fnd,&fnd,4);
    return 0;
}
int trim_number(char fnd[],int jinsu){
    int i;
    for(i=3;i>=1;i--){
        if(fnd[i]>=jinsu){
            fnd[i]=0;
            if(i-1>=0) fnd[i-1]++;
            if(fnd[0]>0) fnd[0]=0;
        }
    }
    return 0;
}
int out_text_editor(unsigned char sw[], int fd_fnd, int fd_lcd, int fd_dot, char* led_addr,int change_mode, int sum_sw){
    int i, count_0=0,count_num=0,flag=0,temp,temp_idx;
//	int sum_sw;
    static int sw_count[9]={0,};
    static int init=0, p_mode=0, cursor, pre_sum_sw[5000], pre_button_num=0;
    static char fnd[4]={0,};
    static unsigned char lcd[32]={0,};
     char* text[MAX_SWITCH] = {".QZ","ABC","DEF","GHI","JKL","MNO","PRS","TUV","WXY"};
    unsigned char dot[2][10]={{0x1c,0x36,0x63,0x63,0x63,0x7f,0x7f,0x63,0x63,0x63},
                    {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}};
	if(change_mode==1){
		for(i=0;i<4;i++) fnd[i]=0;
		write(fd_fnd,fnd,4);
	}
    if(!init){
		for(i=0;i<32;i++){
			lcd[i]=' ';
		}
        write(fd_fnd,&fnd,4);
        write(fd_lcd,&lcd,32);
        cursor=0;
        pre_sum_sw[0]=0;
        *led_addr=0;
		init=1;
    }

    if(sum_sw >0){
        fnd[3]++;
        if(fnd[3]>=10){ fnd[3]=0; fnd[2]++;}
        if(fnd[2]>=10){ fnd[2]=0; fnd[1]++;}
        if(fnd[1]>=10){ fnd[1]=0; fnd[0]++;}
        if(fnd[0]>=10){ fnd[0]=0;}
    }
    if(sum_sw==192){ // switch 2 and 3 -> clear up
        memset(lcd,' ',32);
        write(fd_lcd,&lcd,32);
        cursor=0;
    }
    else if(sum_sw==24){ // switch 5 and 6 -> english to number
        p_mode^=1;      // 0 = english , 1 = number
        
    }
    else if(sum_sw==3){ // switch 8 and 9 -> insert one blank
        if(cursor==8){
            for(i=0;i<8;i++){
                lcd[i]=lcd[i+1];
            }
            cursor--;
            lcd[cursor]=' ';
            cursor++;
        }
        else{
            lcd[cursor]=' ';
            cursor++;
        }
        
    }
    else if(sum_sw==256){ // switch 1
		print_lcd(0,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==128){ // switch 2
		print_lcd(1,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==64){ // switch 3
		print_lcd(2,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==32){ // switch 4		
		print_lcd(3,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==16){ // switch 5
		print_lcd(4,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==8){ // switch 6
		print_lcd(5,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==4){ // switch 7
		print_lcd(6,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==2){ // switch 8
		print_lcd(7,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
    }
    else if(sum_sw==1){ // switch 9
		print_lcd(8,p_mode,pre_sum_sw,pre_button_num,&cursor,text,sw_count,lcd,sum_sw);
	}

	pre_sum_sw[++pre_button_num] = sum_sw;

	write(fd_dot,dot[p_mode],10);
	write(fd_fnd,&fnd,4);
	write(fd_lcd,lcd,8);
	return 0;
}
int print_lcd(int key,int p_mode,int pre_sum_sw[],int pre_button_num,int *cursor, char* text[MAX_SWITCH],int sw_count[],unsigned char lcd[],int sum_sw){
	int i,temp,temp_idx,flag=0,flag2=0;
	if(p_mode==0){
		// check if cotinue to press button or not
		for(i=pre_button_num;i>=0;i--){
			if(pre_sum_sw[i]!=0){
				temp = pre_sum_sw[i];
				temp_idx = i;
				flag=1;
				break;
			}
		}
		if(flag){// if exist latest sum_sw (without 0)
			if(temp == sum_sw){ // previous input (without 0) == sum_sw
				if(pre_button_num - temp_idx>1){ // with time interval same input
					sw_count[key]++;
					if(sw_count[key]==3) sw_count[key]=0; 
					(*cursor)--;
					lcd[*cursor]=text[key][sw_count[key]];
					(*cursor)++;
				}
			}
			else{ // latest input (without 0) != sum_sw
				sw_count[key]=0;
				if((*cursor)==8){
					for(i=0;i<8;i++){
						lcd[i]=lcd[i+1];
					}
					(*cursor)--;
					lcd[*cursor]=text[key][sw_count[key]];
					(*cursor)++;
				}
				else{
					lcd[*cursor]=text[key][sw_count[key]];
					(*cursor)++;
				}
			}
		}	
		else{ // if previous inputs are all zero
				lcd[*cursor]=text[key][sw_count[key]];
				(*cursor)++;
		}
	}
	else if(p_mode==1){ // present number
	// check if cotinue to press button or not
		for(i=pre_button_num;i>=0;i--){
			if(pre_sum_sw[i]!=0){
				temp = pre_sum_sw[i];
				temp_idx = i;
				flag2=1;
				break;
			}
		}
		if(flag2){// if exist latest sum_sw (without 0)
			if(temp == sum_sw){ // previous input (without 0) == sum_sw
				if(pre_button_num - temp_idx>1){ // with time interval same input
					if((*cursor)==8){
						for(i=0;i<8;i++){
							lcd[i]=lcd[i+1];
						}
						(*cursor)--;
						lcd[*cursor]= '1'+key;
						(*cursor)++;
					}
					else{
						lcd[*cursor]='1'+key;
						(*cursor)++;
					}
				}
			}
			else{ // latest input (without 0) != sum_sw
				if((*cursor)==8){
					for(i=0;i<8;i++){
						lcd[i]=lcd[i+1];
					}
					(*cursor)--;
					lcd[*cursor]= '1'+key;
					(*cursor)++;
				}
				else{
					lcd[*cursor]='1'+key;
					(*cursor)++;
				}
			}
		}	
		else{ // if previous inputs are all zero
				lcd[*cursor]='1'+key;
				(*cursor)++;
		}
	}
	return 0;
}
int out_draw_board(unsigned char sw[], int fd_fnd, int fd_dot, int sum_sw)
{
    int i, j;
    int sw_count[9]={0,};
    static clock_t t;
    static int init=0, p_mode=0, pre_sum_sw, cursor_row=0, cursor_col=0,flicker;
    static int select[10][7]={0,};
	static char fnd[4]={0,};
	static unsigned char dot[10]={0,};
	static int on_off=0;
	const unsigned char standard=0x40;
	if(!init){
		for(i=0;i<4;i++){
			fnd[i]=0;
		}
		//dot[0]=0x40;
		t = clock();
		write(fd_fnd,&fnd,4);
		write(fd_dot,dot,10);
		pre_sum_sw=-1;
		flicker=1;
		init=1;
	}
	// cursor off
	if(flicker){
		if(clock()-t>=200){
			printf("clock() -t : %d\n",clock()-t);
			t=clock();
			on_off^=1;
			if(on_off){
				if(!select[cursor_row][cursor_col]){ // if not selected
					dot[cursor_row] |= (unsigned char)(0x40 >> cursor_col);
					write(fd_dot,dot,10);
				}
			}
			if(!on_off){
				if(!select[cursor_row][cursor_col]){ // if not selected
					if(dot[cursor_row] & (0x40>>cursor_col)){
						dot[cursor_row] -= (unsigned char)(0x40 >> cursor_col); // cursor dot off!
						write(fd_dot,dot,10);
					}
				}
			}
			//t=clock();
		}
	}

	if(sum_sw >0){ // fnd count ++
		fnd[3]++;
		if(fnd[3]>=10){ fnd[3]=0; fnd[2]++;}
		if(fnd[2]>=10){ fnd[2]=0; fnd[1]++;}
		if(fnd[1]>=10){ fnd[1]=0; fnd[0]++;}
		if(fnd[0]>=10){ fnd[0]=0; }
	}
	
	/*if(sum_sw==0){
		dot[cursor_row] |= (0x40>>cursor_col);
	}*/
	if(sum_sw==256){ // switch 1  => reset
		for(i=0;i<10;i++){
			for(j=0;j<7;j++){
				select[i][j]=0;
			}
		}
		for(i=0;i<10;i++){
			if(i==0) dot[i]= 0x40;
			else 	 dot[i]= 0x00;
		}
		cursor_row = 0;
		cursor_col = 0;
	}
	else if(sum_sw==128){ // switch 2  => up
		if( select[cursor_row][cursor_col] ){
			//cursor_row--;
		}
		else{
			if(dot[cursor_row] & (0x40>>cursor_col)){
				dot[cursor_row] -= (0x40>>cursor_col);
			}
		}
		/*if( (dot[cursor_row]&&(0x40>>cursor_col)>0) && (select[cursor_row][cursor_col]==0) )
			dot[cursor_row] -= ((unsigned char)(0x40>>cursor_col));*/
		cursor_row--;
		if(cursor_row<0) cursor_row=0;
		dot[cursor_row] |= (0x40>>cursor_col);
	}
	else if(sum_sw==64){ // switch 3  =>  cursor
		flicker =0;
		dot[cursor_row] |= (0x40>>cursor_col);
	}
	else if(sum_sw==32){ // switch 4  => left
		if( select[cursor_row][cursor_col] ){
			//cursor_col--;
		}
		else{
			if(dot[cursor_row] & (0x40>>cursor_col) ){
				dot[cursor_row] -= (0x40>>cursor_col);
			}
		}
		cursor_col--;
		if(cursor_col<0) cursor_col=0;
		dot[cursor_row] |= (0x40>>cursor_col);
	}
	else if(sum_sw==16){ // switch 5  => select
		dot[cursor_row] |= (0x40>>cursor_col);
		select[cursor_row][cursor_col]=1;
	}
	else if(sum_sw==8){ // switch 6  => right
		if( select[cursor_row][cursor_col] ){
			//cursor_col--;
		}
		else{
			if(dot[cursor_row] & (0x40>>cursor_col) ){
				dot[cursor_row] -= (0x40>>cursor_col);
			}

		}
		/*if( (dot[cursor_row]&&(0x40>>cursor_col)>0) && (select[cursor_row][cursor_col]==0) )
			dot[cursor_row] -= (0x40>>cursor_col);*/
		cursor_col++;
		if(cursor_col>6) cursor_col=6;
		dot[cursor_row] |= (0x40>>cursor_col);
		
	}
	else if(sum_sw==4){ // switch 7  => clear
		for(i=0;i<10;i++){
			dot[i]=0x00;
			for(j=0;j<7;j++){
				select[i][j]=0;
			}
		}
		dot[cursor_row] |= (0x40>>cursor_col);
	}
	else if(sum_sw==2){ // switch 8  => down
		if( select[cursor_row][cursor_col] ){
			//cursor_col--;
		}
		else{
			if(dot[cursor_row] & (0x40>>cursor_col)){
				dot[cursor_row] -= (0x40>>cursor_col);
			}
		}
		/*if( (dot[cursor_row]&&(0x40>>cursor_col)>0) && (select[cursor_row][cursor_col]==0) ) 
			dot[cursor_row] -= (unsigned char)(0x40>>cursor_col);*/
		cursor_row++;
		if(cursor_row>9) cursor_row=9;
		dot[cursor_row] |= (0x40>>cursor_col);
	}
	else if(sum_sw==1){ // switch 9  => reverse
		for(i=0;i<10;i++){
			for(j=0;j<7;j++){
				if(!select[i][j]){
					dot[i] |= (0x40>>j);
					select[i][j]=1;
				}
				else{
					dot[i] -= (0x40>>j);
					select[i][j]=0;
				}
			}
		}
		
	}
	printf("cursor : %d %d\n",cursor_row,cursor_col);
	for(i=0;i<10;i++){
		printf("%X ",dot[i]);
		for(j=0;j<7;j++){
			if(select[i][j]){
				dot[i] |= (unsigned char)(0x40>>j);
			}
		}
	}


	write(fd_fnd,fnd,4);
	write(fd_dot,dot,10);
			
	//usleep(50000);
	
	return 0;
}
//int extra_mode(unsigned char sw[], int fd_fnd, int fd_
