#include<linux/kernel.h>
#include<linux/uaccess.h>

asmlinkage unsigned int sys_hw2call(unsigned int interval, unsigned int count, unsigned int start_option)
{
    unsigned int fnd_position, fnd_value, ret;
    int i;
    for(i=0;i<4;i++){
        if(start_option%10 != 0){
            fnd_value = start_option%10;
            fnd_position = 4-i;
            break;
        }
        start_option /= 10;
    }
	ret = fnd_position<<24 | fnd_value<<16 | interval<<8 | count;
    return ret;
}
