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
static unsigned char *iom_fpga_fnd_addr;//memory mapping 함수를 통해 정해질  물리 주소 공간
static unsigned short fnd;//fnd에 표시 될 fnd값
static struct timer_list timer,end_timer;// timer는 stopwatch 타이머, end_timer 끝내기 위한 시간을 재는 timer
/*                      */

static int inter_major=242, inter_minor=0;//device driver major number
static int result;
static dev_t inter_dev;
static struct cdev inter_cdev;
static int inter_open(struct inode *, struct file *);
static int inter_release(struct inode *, struct file *);
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

irqreturn_t inter_home_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_back_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_volup_handler(int irq, void* dev_id, struct pt_regs* reg);
irqreturn_t inter_voldown_handler(int irq, void* dev_id, struct pt_regs* reg);

int interruptCount=0;
static int start=0;// start flag
static int pause=0;// pause flag
static int reset=0;
static unsigned long sw_time=0;// timer time
static int gpio_val;// voldown check value
u64 float_time, e_time; // for pause remain time
/* fops table */
static struct file_operations inter_fops =
{
	.open = inter_open,
	.write = inter_write,
	.release = inter_release,
};
/**********************/
/* sw_time에 따라서 fnd에 직접값을 입력해주는 함수*/
/**********************/
static int output_fnd(void){
	fnd = 0;
    fnd |= (((sw_time / 60) % 60) / 10) << 12;
    fnd |= (((sw_time / 60) % 60) % 10) << 8;
    fnd |= ((sw_time % 60) / 10) << 4;
    fnd |= ((sw_time % 60) % 10) ;
    outw(fnd,(unsigned int)iom_fpga_fnd_addr);
    return 1;
}
/*************************/
/* kernel_timer_write 함수가 실행되고 callback 형식으로 계속 실행될 함수 */
/*************************/
static void kernel_timer_callback() {
	if(pause) return;
    sw_time++;
	printk("sw_time : %d\n",sw_time);
	output_fnd();

	timer.expires = get_jiffies_64() + ( 1 * HZ);
	timer.data = sw_time;
	timer.function = kernel_timer_callback;
	add_timer(&timer);

}
/******************************/
/* kernel_timer_callback 함수를 실행할 최초의 함수 */
/* parameter 들은 형식만 있을뿐 사용 직접적으로 사용하지는 않습니다 */
/******************************/
ssize_t kernel_timer_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) {
    
    if(!start){
		printk("set up start!\n");
        timer.expires = jiffies + (1 * HZ);// jiffies = 현재 보드의 시간
        timer.data = sw_time;
        timer.function	= kernel_timer_callback;// 이번 타이머가 끝나고 수행될 함수의 이름을 저장합니다.

        add_timer(&timer);// 타이머 추가
        start=1;
    }
	return 1;
}
/******************************/
/* pause를 눌렀을 경우 초단위로 움직이는 시간의 소수점 자리를
 * 저장하고 그 남은 시간만큼 기다렸다가 다시 timer가 작동하게
 * 하는 함수 */
/******************************/
ssize_t kernel_remain_timer_write(u64 remain_time) {
    
    
    timer.expires = jiffies + remain_time;
    timer.function	= kernel_timer_callback;

    add_timer(&timer);
	return 1;
}
/******************************/
/* end_timer_write() 함수가 실행되고 3초 뒤에 불려지는 함수
 * 그때 gpio 값을 체크해서 종료를 판단함 */
/******************************/
ssize_t kernel_end_timer_call() {
	if(get_jiffies_64() - e_time>=3*HZ){
		if(gpio_val==0 ){
			del_timer(&timer);
			outw(0,(unsigned int)iom_fpga_fnd_addr);
			__wake_up(&wait_queue,1,1,NULL);

			return 1;
		}
		else{
			return 1;
		}
	}
		end_timer.expires = jiffies + HZ/100;
    	end_timer.function	= kernel_end_timer_call;
    	add_timer(&end_timer);
	return 0;
}
/******************************/
/* 처음 voldown 버튼을 눌렀을때 실행되는 함수
 * 3초 뒤에 kernel_end_timer_call 함수가 실행되게 함 */
/******************************/
ssize_t kernel_end_timer_write(u64 remain_time) {
	e_time = get_jiffies_64();
    end_timer.expires = jiffies + 3*HZ;
    end_timer.function	= kernel_end_timer_call;

    add_timer(&end_timer);
	return 1;
}
/******************************/
/* home key에 대한 interrupt 함수
 * 처음 시작할때 눌러서 timer를 실행 시킴 */
/******************************/
irqreturn_t inter_home_handler(int irq, void* dev_id, struct pt_regs* reg) {
	printk(KERN_ALERT "interrupt home key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 11)));
    kernel_timer_write(NULL,NULL,0,0);
	return IRQ_HANDLED;
}
/******************************/
/* back key에 대한 interrupt 함수로
 * pause의 역할을 담당하며 pause시 다시 시작하는 버튼이기도 함
 * parameter는 형식을 유지했음 */
/******************************/
irqreturn_t inter_back_handler(int irq, void* dev_id, struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt back key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(1, 12)));
	
    if(pause==0){ // stop pause
		float_time = timer.expires - get_jiffies_64();

        del_timer(&timer);
        start=0;
		pause=1;
    }
	else if(pause==1){ // start pause
		pause=0;
		start=1;
		
		init_timer(&timer);
		kernel_remain_timer_write(float_time);
	}

    return IRQ_HANDLED;
}
/******************************/
/* vol+ key에 대한 interrupt 함수로
 * timer를 reset을 하는 역할을 수행함
 * fnd를 0으로 출력하고 sw_time을 0으로 초기화 한뒤
 * kernel_timer_write() 함수를 수행함 */
/******************************/
irqreturn_t inter_volup_handler(int irq, void* dev_id,struct pt_regs* reg) {
    printk(KERN_ALERT "interrupt volup key!!! = %x\n", gpio_get_value(IMX_GPIO_NR(2, 15)));

    if(start || pause){
        //reset=1;
        start=0;
        del_timer(&timer);
        sw_time=0;
    	outw(0,(unsigned int)iom_fpga_fnd_addr);
        kernel_timer_write(NULL,NULL,0,0);
    }
    return IRQ_HANDLED;
}
/******************************/
/* vol - key에 대한 interrupt 함수로
 * voldown key를 3초이상 누르고 있을 경우 timer가 종료 되게 합니다.
 * gpio_val가 누를때 0 뗄때 1의 값을 가지게 됩니다. */
/******************************/
irqreturn_t inter_voldown_handler(int irq, void* dev_id, struct pt_regs* reg) {

	gpio_val = gpio_get_value(IMX_GPIO_NR(5, 14));
    printk(KERN_ALERT "interrupt voldown key!!! = %x\n", gpio_val );
    
    if(gpio_val == 0){ // falling point
		kernel_end_timer_write(0);
    }
	else if(gpio_val == 1 && get_jiffies_64()-e_time <=3*HZ){
		del_timer(&end_timer);
	}
    return IRQ_HANDLED;
}

/******************************/
/* gpio 매크로를 통해 4개의 key의 irq 번호를 얻고
 * 그 번호를 사용해 수행될 interrupt 함수, trigger flag,이름 등을 설정합니다 */
/******************************/
static int inter_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk(KERN_ALERT "Open Module\n");
	
	// int1
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq,inter_home_handler,IRQF_TRIGGER_FALLING,"home",0);

	// int2
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, inter_back_handler,IRQF_TRIGGER_FALLING,"back",0);

	// int3
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq,inter_volup_handler,IRQF_TRIGGER_FALLING,"volup",0);

	// int4
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq,inter_voldown_handler,IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,"voldown",0);

	return 0;
}
/******************************/
/* request_irq를 통해 등록했던 irq를 해제합니다.*/
/******************************/
static int inter_release(struct inode *minode, struct file *mfile){
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	printk(KERN_ALERT "Release Module\n");
	return 0;
}
/******************************/
/* app에서 write() 함수를 부를때 호출되는 함수로
 * process를 wait_queue에 추가하여 대기시킨 상태에서
 * interrupt를 수행합니다. */
/******************************/
static int inter_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
	printk("write\n");
    interruptible_sleep_on(&wait_queue);
	start=0;
	pause=0;
	reset=0;
	sw_time=0;
	return 0;
}
/******************************/
/* charter device를 등록합니다.
 * fnd 드라이버를 iomaped 방식을 사용하여 물리적인 주소를 지정합니다.
 * timer 와 end_timer를 초기화 합니다
 * wait_queue를 초기화 시킵니다.*/
/******************************/
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
    
    iom_fpga_fnd_addr = ioremap(IOM_FND_ADDRESS, 0x04);//fnd 주소설정
    init_timer(&timer);    //init timer
    init_timer(&end_timer);    //init end_timer
	init_waitqueue_head(&wait_queue); //init wait_queue
	if(error)
	{
		printk(KERN_NOTICE "inter Register Error %d\n", error);
	}
	return 0;
}
/* init */
// inter_register_cdev() 함수 호출!
static int __init inter_init(void) {
	int result;
	if((result = inter_register_cdev()) < 0 )
		return result;
	
	printk(KERN_ALERT "Init Module Success \n");
	printk(KERN_ALERT "Device : /dev/inter, Major Num : 242 \n");
	return 0;
}
// rmmod 명령시 수행되는 함수
static void __exit inter_exit(void) {
	cdev_del(&inter_cdev);
	unregister_chrdev_region(inter_dev, 1);

    iounmap(iom_fpga_fnd_addr);

    del_timer(&end_timer);
    del_timer(&timer);
	printk(KERN_ALERT "Remove Module Success \n");
}

module_init(inter_init); //insmod
module_exit(inter_exit); //rmmod
	MODULE_LICENSE("GPL");

