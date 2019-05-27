#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

#define DEV_MAJOR 242

#define IOM_FND_ADDRESS 0x08000004
/*  wait queue variable */
wait_queue_head_t wait_queue;

/*  user define variable    */
static unsigned char *iom_fpga_fnd_addr;
static unsigned short fnd;
static struct timer_list timer;
/*                      */

static int inter_major=0, inter_minor=0;
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;
static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_home__handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_back_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_volup_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_voldown_handler(int irq, void* dev_id, struct pt_regs* reg);

static inter_usage=0;
int interruptCount=0;
static int start=0;
static int pause=0;
static int reset=0;
static unsigned long sw_time=0;
u64 int4_start, int4_end;

static struct file_operations inter_fops =
{
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};

static int output_fnd(){
    fnd |= (((sw_time / 60) % 60) / 10) << 12;
    fnd |= (((sw_time / 60) % 60) % 10) << 8;
    fnd |= ((sw_time % 60) / 10) << 4;
    fnd |= ((sw_time % 60) % 10) ;
    outw(fnd,(unsigned int)iom_fpga_fnd_addr);
    return 1;
}
    
static void kernel_timer_callback(unsigned long sw_time) {
    sw_time++;
	output_fnd();
	if(pause) return;

	timer.expires = get_jiffies_64() + ( 1 * HZ);
	timer.data = sw_time;
	timer.function = kernel_timer_callback;
	add_timer(&timer);

}

ssize_t kernel_timer_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
    
    if(reset){
        sw_time=0;
        reset=0;
    }
    if(!start){
        timer.expires = jiffies + (HZ/100);
        timer.data = sw_time;
        timer.function	= kernel_timer_callback;

        add_timer(&timer);
        start=1;
    }
	return 1;
}

irqreturn_t inter_home_handler(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt home key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));

    kernel_timer_write(NULL,NULL,0,0);
	return IRQ_HANDLED;
}

irqreturn_t inter_back_handler(int irq, void* dev_id, struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt back key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));

    if(!pause){
        del_timer(&timer);
        pause=1;
        start=0;
    }

    return IRQ_HANDLED;
}

irqreturn_t inter_volup_handler(int irq, void* dev_id,struct pt_regs* reg) {
    priintk(KERN_ALERT "interrupt volup key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

    if(start){
        reset=1;
        start=0;
        del_timer(&timer);
        //sw_time=0;
        kernel_timer_write(NULL,NULL,0,0);
    }
    return IRQ_HANDLED;
}

irqreturn_t inter_voldown_handler(int irq, void* dev_id, struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt voldown key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
    int gpio_val = gpio_get_value(IMX_GPIO_NR(5,14));

    static int falling=0;
    
    if(gpio_val == 0){ //
        int4_start = get_jiffies_64();
    }
    else{
        if(get_jiffies_64()-int4_start>=3*HZ){
            del_timer(&timer);
            outw(0,(unsigned int)iom_fpga_fnd_addr);
            wake_up_interruptible(&wq_queue);
            return IRQ_HANDLED;
        }
    }
        
    return IRQ_HANDLED;
}


static int inter_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");

	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, &interrupt_home_handler,IRQF_TRIGGER_FALLING,"home",0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, &interrupt_back_handler,IRQF_TRIGGER_FALLING,"back",0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq,&interrupt_volup_handler,IRQF_TRIGGER_FALLING,"volup",0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq,&interrupt_voldown,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,"voldown",0);

	return 0;
}

static int inter_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	printk(KERN_ALERT "Release Module\n");
	return 0;
}

static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	printk("write\n");
    interruptible_sleep_on(&wait_queue);
	return 0;
}

static int inter_register_cdev(void)
{
	int error;
	if(inter_major) {
		inter_dev = MKDEV(inter_major, inter_minor);
		error = register_chrdev_region(inter_dev,1,"inter");
	}else{
		error = alloc_chrdev_region(&inter_dev,inter_minor,1,"inter");
		inter_major = MAJOR(inter_dev);
	}
	if(error<0) {
		printk(KERN_WARNING "inter: can't get major %d\n", inter_major);
		return result;
	}
	printk(KERN_ALERT "major number = %d\n", inter_major);
	cdev_init(&inter_cdev, &inter_fops);
	inter_cdev.owner = THIS_MODULE;
	inter_cdev.ops = &inter_fops;
	error = cdev_add(&inter_cdev, inter_dev, 1);
    
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x04);
    init_timer(&timer);    

	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}
/* init */
static int __init inter_init(void) {
	int result;
	if((result = inter_register_cdev()) < 0 )
		return result;
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/inter, Major Num : 242 \n");
	return 0;
}

static void __exit inter_exit(void) {
	cdev_del(&inter_cdev);
	unregister_chrdev_region(inter_dev, 1);

    iounmap(iom_fpga_fnd_addr);
    del_timer(&timer);
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init);
module_exit(inter_exit);
	MODULE_LICENSE("GPL");

