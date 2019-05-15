#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<linux/unistd.h>
#include<linux/ioctl.h>
#include<string.h>
#include<syscall.h>
#include<fcntl.h>
#define DEVICE_MAJOR_NUM 242
#define IOCTL_MY_WRITE _IOW(DEVICE_MAJOR_NUM,0,4)

#define DEVICE_NAME "/dev/dev_driver"
int main(int argc, char *argv[])
{
    unsigned int interval, count, start_option, ret;
    int dev_fd, retval, end=0;
    if( argc != 4){
        printf(" incorrect arguments\n");
        return -1;
    }
    interval = atoi(argv[1]);
    count = atoi(argv[2]);
    start_option = atoi(argv[3]);

    if(!(interval >=1 && interval <=100)){
        printf("invalid interval! [1-100]\n");
        end =1;
    }
    if(!(count >= 1 && count <=100)){
        printf("invalid count! [1-100]\n");
        end=1;
    }
    if(!(start_option >=1 && start_option<=8000)){
        printf("invalid start_option! [1-8000]\n");
        end=1;
    }
    if(end) return -1;


    ret = syscall(376, interval, count, start_option);
    dev_fd = open(DEVICE_NAME,O_RDWR);
    if( dev_fd<0 ){
        printf("Device open error : %s\n",DEVICE_NAME);
        exit(1);
    }
    
    //retval = write(dev,&ret,sizeof(ret));
	retval=ioctl(dev_fd,IOCTL_MY_WRITE,&ret);
    if(retval<0){
        printf("ioctl Write Error!\n");
        return -1;
    }
    
    close(dev_fd);
    
    return 0;
}
