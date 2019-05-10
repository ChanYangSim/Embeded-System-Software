#include<stdio.h>
#include<stdlib.h>
#include<linux/unistd.h>
#include<string.h>
#include<syscall.h>
#include<fcntl.h>

#define DEVICE_NAME "/dev/dev_driver"
int main(int argc, char *argv[])
{
    unsigned int interval, count, start_option, ret;
    int dev, retval;
    if( argc != 4){
        printf(" incorrect arguments\n");
        return -1;
    }
    interval = atoi(argv[1]);
    count = atoi(argv[2]);
    start_option = atoi(argv[3]);
    printf("hello world %d %d %d\n",interval,count,start_option);
    ret = syscall(376, interval, count, start_option);
    dev = open(DEVICE_NAME,O_RDWR);
    if( dev<0 ){
        printf("Device open error : %s\n",DEVICE_NAME);
        exit(1);
    }
    
    retval = write(dev,&ret,sizeof(ret));
    if(write<0){
        printf("Write Error!\n");
        return -1;
    }
    
    close(dev);
    
    
    
    return 0;
}
