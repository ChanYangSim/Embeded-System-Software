#include "main.h"

#ifndef INPUT_PROCESS
#define INPUT_PROCESS

int input_proc(){
	printf("input_proc\n");
	int fd_device, fd_switch;
	int val;
	struct input_event event;
	int size_ev=sizeof(struct input_event);
    struct msgbuf msgsnd, msgrcv;
    enum msg_type MSG_TYPE;
    key_t key_id;
    /*  open key device  */
	fd_device = open(KEY_DEVICE,O_RDONLY /*| O_NONBLOCK*/);
	if(fd_device <0){
		printf("Device open error!\n");
		close(fd_device);
	}
    /* opne switch key device */
	fd_switch = open(SWITCH_DEVICE, O_RDONLY/* O_RDWR | O_NONBLOCK*/);
	if(fd_switch < 0){
		printf("Switch opne error!\n");
		close(fd_switch);
	}
	unsigned char sw[MAX_SWITCH];
	unsigned char prev_sw[MAX_SWITCH];
	unsigned char output_sw[MAX_SWITCH];
	int size_sw = sizeof(sw);
	memset(sw,0,size_sw); memset(prev_sw,0,size_sw); memset(output_sw,0,size_sw);

    (void)signal(SIGINT,user_signal1);

	while(1)
	{
        int read_d = read(fd_device, &event, size_ev * BUF_SIZE);
        if(read_d < sizeof(ev))
            printf(" read form readkey error !\n");
        if(event.value == KEY_PRESS){
            if(event.code == BACK_KEY){
                key_id = msgget((key_t)IN_AND_MAIN,IPC_CREAT|0666);
                memset(&msgsnd,0,sizeof(struct msgbuf));
                msgsnd.msgtype=TYPE_BACK;
                msgsnd.text[0]=1;
                if(msgsnd(key_id,(void*)&msgsnd,sizeof(struct msgbuf),IPC_NOWAIT)==-1){
                    printf("msgsnd error!\n");
                }
                usleep(1);
                
                return -1;
            }
            if(event.code == VOL_UP_KEY)	;
            if(event.code == VOL_DOWN_KEY)	;
        }
		
	    int read_sw = read(fd_switch,sw,MAX_SWITCH);
        for(int i=0;i<MAX_SWITCH;i++){
            ;
        }
        usleep(400000);

	}
    close(fd_device);
    close(fd_switch);
		
	return 0;
}
