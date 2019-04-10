#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#define CLOCK 1
#define COUNTER 2
#define TEXT_EDITOR 3
#define DRAW_BOARD 4
#define EXTRA 5


#define BUF_SIZE 32

#define BACK_KEY 158
#define PROG_KEY 116
#define VOL_UP_KEY 115
#define VOL_DOWN_KEY 114


#define TYPE_BACK 1
#define TYPE_PROG 2
#define TYPE_VOL_UP 3
#define TYPE_VOL_DOWN 4
#define TYPE_SWITCH 5
#define TYPE_DEVICE 1 

#define KEY_RELEASE 0
#define KEY_PRESS 1

#define MSGTYPE_ 1

#define KEY_DEVICE "/dev/input/event0"
#define SWITCH_DEVICE "/dev/fpga_push_switch"

#define FND_DEVICE "/dev/fpga_fnd"
#define LCD_DEVICE "/dev/fpga_text_lcd"
#define LED_DEVICE "/dev/fpga_led"
#define DOT_DEVICE "/dev/fpga_dot"
#define TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define BUZZER_DEVICE "/dev/fpga_buzzer"
#define MOT_DEVICE "/dev/fpga_step_motor"
#define FPGA_BASE_ADDRESS 0x08000000
#define LED_ADDR 0x16
#define MAX_SWITCH 9
#define MAX_LCD 32
#define DELAY 30000
#define SEC 400

#define IN_AND_MAIN_D 8000
#define IN_AND_MAIN_SW 2000
#define MAIN_AND_OUT_D 3000
#define MAIN_AND_OUT_SW 4000

struct msgbuf{
	long msgtype;
	unsigned char text[BUF_SIZE];
};
unsigned char quit =0;
void user_signal1(int sig){quit=1;}
int input_proc();
int output_proc();
int main_proc();
int check_sw(unsigned char sw[]);
int out_clock(unsigned char sw[],int fd_fnd,char* led_addr);
int out_counter(unsigned char sw[],int fd_fnd, char* led_addr);
int trim_number(char fnd[],int jinsu);
int out_text_editor(unsigned char sw[], int fd_fnd, int fd_lcd, int fd_dot, char*);
int out_draw_board(unsigned char sw[], int fd_fnd, int fd_dot);
