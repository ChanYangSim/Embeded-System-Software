#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include "./fpga_dot_font.h"

/* hw2_driver */
#define IOM_DEV_DRIVER_MAJOR 242
#define IOM_DEV_DRIVER_NAME "dev_driver"

/* dot_driver */
#define IOM_FPGA_DOT_MAJOR 262		// ioboard led device major number
#define IOM_FPGA_DOT_NAME "fpga_dot"		// ioboard led device name
#define IOM_FPGA_DOT_ADDRESS 0x08000210 // pysical address

/* lcd_driver */
#define IOM_FPGA_TEXT_LCD_MAJOR 263		// ioboard led device major number
#define IOM_FPGA_TEXT_LCD_NAME "fpga_text_lcd"		// ioboard led device name
#define IOM_FPGA_TEXT_LCD_ADDRESS 0x08000090 // pysical address - 32 Byte (16 * 2)

/* led_driver */
#define IOM_LED_MAJOR 260		// ioboard led device major number
#define IOM_LED_NAME "fpga_led"		// ioboard led device name
#define IOM_LED_ADDRESS 0x08000016 // pysical address

/* fnd_driver */
#define IOM_FND_MAJOR 261		// ioboard fpga device major number
#define IOM_FND_NAME "fpga_fnd"		// ioboard fpga device name
#define IOM_FND_ADDRESS 0x08000004 // pysical address

/* ioctl */


//Global variable
static int fpga_dev_driver_usage = 0;
static int rotate=7;
//static int kernel_timer_usage = 0;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_dot_addr;

static void kernel_timer_blink(unsigned long timeout);
ssize_t kernel_timer_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);

ssize_t iom_dev_driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
ssize_t iom_dev_driver_ioctl(struct file *inode, unsigned int cmd, const char* gdata);
int iom_dev_driver_open(struct inode *minode, struct file *mfile);
int iom_dev_driver_release(struct inode *minode, struct file *mfile);

// define file_operations structure
struct file_operations iom_dev_driver_fops =
{
	.owner		=	THIS_MODULE,
	.open		=	iom_dev_driver_open,
	.write		=	iom_dev_driver_write,
	.release	=	iom_dev_driver_release,
	.unlocked_ioctl = iom_dev_driver_ioctl
};


struct struct_mydata {
	struct timer_list timer;
    unsigned int fnd_position;
    unsigned int fnd_value;
    unsigned int interval;
    unsigned int iter_count;
};
struct struct_mydata mydata;



// when this device open, call this fucntion
int iom_dev_driver_open(struct inode *minode, struct file *mfile) 
{	
	if(fpga_dev_driver_usage != 0) return -EBUSY;
	fpga_dev_driver_usage = 1;
	return 0;
}

// when this device close ,call this function
int iom_dev_driver_release(struct inode *minode, struct file *mfile) 
{
	fpga_dev_driver_usage = 0;

	return 0;
}
ssize_t iom_dev_driver_ioctl(struct file *inode, unsigned int cmd, const char *gdata)
{
	
	switch(_IOC_NR(cmd)){
		case 0:
			kernel_timer_write(NULL,gdata,0,0);
			break;
		default:
			printk("ioctl request num error!\n");
			return -1;
	}
	return 1;
}

ssize_t iom_dev_driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what){

    unsigned int value;
	//unsigned int fnd_position, fnd_value, interval, iter_count;
    const char *tmp = gdata;
    if( copy_from_user(&value, tmp, length))
        return -EFAULT;

    kernel_timer_write(inode,gdata,length,off_what);

    return 0;

}
void move_lcd(unsigned char *lcd_h, unsigned char *lcd_r, int init){
    int i;
    static int leftward_h=0, rightward_h=1, leftward_r=0, rightward_r=1;
	if(!init){
		leftward_h=0, rightward_h=1, leftward_r=0, rightward_r=1;
		return;
	}
    if(rightward_h){
        if(lcd_h[15]==' '){
            for(i=15;i>0;i--){
                lcd_h[i]=lcd_h[i-1];
            }
            lcd_h[0]=' ';
			if(lcd_h[15]!=' '){
				leftward_h=1; rightward_h=0;
			}
        }
    }
    else if(leftward_h){
        if(lcd_h[0]==' '){
            for(i=0;i<15;i++){
                lcd_h[i]=lcd_h[i+1];
            }
            lcd_h[15]=' ';
			if(lcd_h[0]!=' '){
           		leftward_h=0; rightward_h=1;
			}
        }
    }
    if(rightward_r){
        if(lcd_r[15]==' '){
            for(i=15;i>0;i--){
                lcd_r[i]=lcd_r[i-1];
            }
            lcd_r[0]=' ';
			if(lcd_r[15]!=' '){
				leftward_r=1; rightward_r=0;
			}
        }
    }
    else if(leftward_r){
        if(lcd_r[0]==' '){
            for(i=0;i<15;i++){
                lcd_r[i]=lcd_r[i+1];
            }
            lcd_r[15]=' ';
			if(lcd_r[0]!=' '){
           		leftward_r=0; rightward_r=1;
			}
        }
    }
}
static void output_blink(unsigned long info_p){
	struct struct_mydata *p_data = (struct struct_mydata*)info_p;
	
	static unsigned char fnd[4]={0,};
	unsigned short int value_short =0;

	//static unsigned char led=0;
	static unsigned char lcd_h[16]={"20141542        "};
	static unsigned char lcd_r[16]={"simchanyang     "};
	const unsigned char str1[16]={"20141542        "};
	const unsigned char str2[16]={"simchanyang     "};
	static int lcd_flag=0;
	unsigned char value_lcd[33];
	unsigned short s_value;
	int i;
	//output led

	s_value = (unsigned short)128>>(p_data->fnd_value-1);
	outw(s_value,(unsigned int)iom_fpga_led_addr);
	
	//output fnd
	for(i=0;i<4;i++) fnd[i]=0;

	fnd[p_data->fnd_position-1] = p_data->fnd_value;
	value_short = fnd[0] << 12 | fnd[1] << 8 | fnd[2] << 4 | fnd[3];
	outw(value_short,(unsigned int)iom_fpga_fnd_addr);

	//output lcd
	if(!lcd_flag){
		for(i=0;i<16;i++){
			lcd_h[i]=str1[i];
			lcd_r[i]=str2[i];
		}
		move_lcd(lcd_h,lcd_r,lcd_flag);
		lcd_flag=1;
	}

	for(i=0;i<16;i++)
		value_lcd[i]=lcd_h[i];
	for(i=0;i<16;i++)
		value_lcd[i+16]=lcd_r[i];
	for(i=0;i<32;i++){
		s_value = ((value_lcd[i] & 0xFF) << 8) | (value_lcd[i+1] & 0xFF);
		outw(s_value,(unsigned int)iom_fpga_text_lcd_addr+i);
		i++;
	}
	move_lcd(lcd_h,lcd_r,lcd_flag);

	//output dot
	for(i=0;i<10;i++){
		s_value = fpga_number[p_data->fnd_value][i] & 0x7F;
		outw(s_value,(unsigned int)(iom_fpga_dot_addr+(i*2)));
	}
	p_data->fnd_value++;
	rotate--;

	if(rotate==0) {p_data->fnd_position++; rotate=7;}
	if(p_data->fnd_value==9) p_data->fnd_value=1;
	if(p_data->fnd_position>4) p_data->fnd_position =1;
		
}

static void kernel_timer_blink(unsigned long timeout) {
	struct struct_mydata *p_data = (struct struct_mydata*)timeout;

	printk("kernel_timer_blink %d %d %d %d\n",
		p_data->fnd_position, p_data->fnd_value, p_data->interval, p_data->iter_count);
	// end of iteration
	int i;
	if(p_data->iter_count==0){
		for(i=0;i<10;i++){
		outw(0,(unsigned int)(iom_fpga_dot_addr+(i*2)));
		}
		for(i=0;i<32;i++){
		outw(0,(unsigned int)iom_fpga_text_lcd_addr+i);i++;
		}
		outw(0,(unsigned int)iom_fpga_fnd_addr);
		outw(0,(unsigned int)iom_fpga_led_addr);
		return;
	}
	output_blink(timeout);
	--p_data->iter_count;
    	
	p_data->timer.expires = get_jiffies_64() + (p_data->interval * HZ/10);
	p_data->timer.data = (unsigned long)&mydata;
	p_data->timer.function = kernel_timer_blink;

	add_timer(&p_data->timer);
	
}

ssize_t kernel_timer_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
	const char *tmp = gdata;

    unsigned int value;
    //unsigned int fnd_position, fnd_value, interval, iter_count;
	// 1 byte
	if (copy_from_user(&value, tmp, 4)) {
		return -EFAULT;
	}
	mydata.fnd_position = value>>24; value &= 0x00FFFFFF;
	mydata.fnd_value = value>>16; value &= 0x0000FFFF;
	mydata.interval = value>>8; value &= 0x000000FF;
	mydata.iter_count = value;
	rotate=7;
	//printk("timer_write: %d %d %d %d\n",mydata.fnd_position,mydata.fnd_value,mydata.interval,mydata.iter_count);

	mydata.timer.expires = jiffies + (mydata.interval * HZ/10);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function	= kernel_timer_blink;

	add_timer(&mydata.timer);
	return 1;
}




int __init iom_dev_driver_init(void)
{
	int result;

	result = register_chrdev(IOM_DEV_DRIVER_MAJOR, IOM_DEV_DRIVER_NAME, &iom_dev_driver_fops);
	if(result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}

	iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x4);
    iom_fpga_text_lcd_addr = ioremap(IOM_FPGA_TEXT_LCD_ADDRESS, 0x32);
    iom_fpga_led_addr = ioremap(IOM_LED_ADDRESS, 0x1);
    iom_fpga_dot_addr = ioremap(IOM_FPGA_DOT_ADDRESS, 0x10);
	init_timer(&(mydata.timer));

	printk("init module, %s major number : %d\n", IOM_DEV_DRIVER_NAME, IOM_DEV_DRIVER_MAJOR);

	return 0;
}

void __exit iom_dev_driver_exit(void)
{
	iounmap(iom_fpga_fnd_addr);
    iounmap(iom_fpga_text_lcd_addr);
    iounmap(iom_fpga_led_addr);
    iounmap(iom_fpga_dot_addr);
	del_timer_sync(&mydata.timer);
	unregister_chrdev(IOM_DEV_DRIVER_MAJOR, IOM_DEV_DRIVER_NAME);
}

module_init(iom_dev_driver_init);
module_exit(iom_dev_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
