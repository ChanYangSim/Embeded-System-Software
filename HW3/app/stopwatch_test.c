#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<linux/unistd.h>
#include<linux/ioctl.h>


#define DEVICE_NAME "/dev/stopwatch"


int main()
{
    int fd;
    char data;
    fd=open(DEVICE_NAME,O_RDWR);
    if( fd < 0 ){
        printf("device open error!\n");
        return -1;
    }
    data = write(fd, ,0);
    close(fd);
    return 0;
}
