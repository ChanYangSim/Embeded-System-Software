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

//Global variable
static int fpga_dev_driver_usage = 0;
static int kernel_timer_usage = 0;
static unsigned char *iom_fpga_led_addr;
static unsigned char *iom_fpga_text_lcd_addr;
static unsigned char *iom_fpga_fnd_addr;
static unsigned char *iom_fpga_dot_addr;

struct struct_mydata mydata;

static void kernel_timer_blink(unsigned long timeout);
ssize_t kernel_timer_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);

ssize_t iom_dev_driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what);
ssize_t iom_dev_driver_ioctl(struct file *inode);
int iom_dev_driver_open(struct inode *minode, struct file *mfile);
int iom_dev_driver_release(struct inode *minode, struct file *mfile);

// define file_operations structure
struct file_operations iom_dev_driver_fops =
{
	.owner		=	THIS_MODULE,
	.open		=	iom_dev_driver_open,
	.write		=	iom_dev_driver_write,
	.release	=	iom_dev_driver_release,
};

static struct struct_mydata {
	struct timer_list timer;
    unsigned int fnd_position;
    unsigned int fnd_value;
    unsigned int interval;
    unsigned int iter_count;
};


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



ssize_t iom_dev_driver_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what){

    unsigned int value, fnd_position, fnd_value, interval, iter_count;
    const char *tmp = gdata;
    if( copy_from_user(&value, tmp, length))
        return -EFAULT;

    fnd_position =value>>24; value &= 0x00FFFFFF;
    fnd_value = value>>16; value &= 0x0000FFFF;
    interval = value>>8; value &= 0x000000FF;
    iter_count = value;

    kernel_timer_write(inode,gdata,length,off_what);

    return 0;

}



static void kernel_timer_blink(unsigned long timeout) {
	struct struct_mydata *p_data = (struct struct_mydata*)timeout;

	/*printk("kernel_timer_blink %d\n", p_data->count);

	p_data->count++;
	if( p_data->count > 15 ) {
		return;
	}*/
	
	mydata.timer.expires = get_jiffies_64() + (1 * HZ);
	mydata.timer.data = (unsigned long)&mydata;
	mydata.timer.function = kernel_timer_blink;

	add_timer(&mydata.timer);
}

ssize_t kernel_timer_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
	const char *tmp = gdata;

    unsigned int value, fnd_position, fnd_value, interval, iter_count;
	// 1 byte
	if (copy_from_user(&value, tmp, 4)) {
		return -EFAULT;
	}
	mydata.fnd_position = value>>24; value &= 0x00FFFFFF;
    mydata.fnd_value = value>>16; value &= 0x0000FFFF;
    mydata.interval = value>>8; value &= 0x000000FF;
    mydata.iter_count = value;
	//mydata.

	//printk("data  : %d \n",mydata.count);

	del_timer_sync(&mydata.timer);

	mydata.timer.expires = jiffies + (1 * HZ);
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

	printk("init module, %s major number : %d\n", IOM_DEV_DRIVER_NAME, IOM_DEV_DRIVER_MAJOR);

	return 0;
}

void __exit iom_dev_driver_exit(void)
{
	iounmap(iom_fpga_fnd_addr);
    iounmap(iom_fpga_text_lcd_addr);
    iounmap(iom_fpga_led_addr);
    iounmap(iom_fpga_dot_addr);

	unregister_chrdev(IOM_DEV_DRIVER_MAJOR, IOM_DEV_DRIVER_NAME);
}

module_init(iom_dev_driver_init);
module_exit(iom_dev_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
