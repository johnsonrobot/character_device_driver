#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

struct fake_device {
	char data[100];
	struct semaphore sem;
} virtual_device;

struct cdev *mycdev;
int major_number;
int ret;

dev_t dev_num;

#define DEVICE_NAME  "usesr_device"


int device_open(struct inode *inode, struct file *filep){
	if(down_interruptible(&virtual_device.sem) != 0){
		printk(KERN_ALERT "could not lock device during open");
		return -1;
	}
	
	printk(KERN_INFO "opened device");
	return 0;
}

ssize_t device_read(struct file* filep, char* buff_des_data, size_t bufCount, loff_t* curoffest){
	printk(KERN_INFO "reading from device");
	ret = copy_to_user(buff_des_data, virtual_device.data, bufCount);
	return ret;
}

ssize_t device_write(struct file* filep, const char* buff_sou_data, size_t bufCount, loff_t* curoffest){
	printk(KERN_INFO "writing to device");
	ret = copy_from_user(virtual_device.data, buff_sou_data, bufCount);
	return ret;
}

int device_close(struct inode *inode, struct file *filep){
	up(&virtual_device.sem);
	printk(KERN_INFO "closed device");
	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
	.read = device_read
};

static int driver_entry(void){
	ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
	if(ret < 0){
		printk(KERN_ALERT "failed to a allocate a major number");
		return ret;
	}
	major_number = MAJOR(dev_num);
	printk(KERN_INFO "major number is %d", major_number);
	printk(KERN_INFO "\tuse \"mknod /dev/%sc %d 0\" for device file", DEVICE_NAME, major_number);
	
	mycdev = cdev_alloc();
	mycdev->ops = &fops;
	mycdev->owner = THIS_MODULE;
	
	ret = cdev_add(mycdev, dev_num, 1);
	if(ret < 0){
		printk(KERN_ALERT "unable to add cdev to kernel");
		return ret;
	}
	
	sema_init(&virtual_device.sem,1);

	return 0;
}

static void driver_exit(void){
	cdev_del(mycdev);

	unregister_chrdev_region(dev_num, 1);
	printk(KERN_ALERT "unloaded module");
}

module_init(driver_entry);
module_exit(driver_exit);
