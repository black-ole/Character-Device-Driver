#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>

#define minor_devices 2
#define name "lifo_dev"

wait_queue_head_t wq;
static char msg[10000] = {0};
static int ptr = 0;
static int reads = 0;
static short size_of_msg ;

static int lifo_open(struct inode *inode, struct file *file);
static int lifo_release(struct inode *inode, struct file *file);
static ssize_t lifo_read(struct file *file, char __user *buf, size_t count , loff_t *offset);
static ssize_t lifo_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);

static const struct file_operations lifo_fops = {
	.owner = THIS_MODULE,
	.open = lifo_open,
	.release = lifo_release,
	.read = lifo_read,
	.write = lifo_write
};

static int lifo_major = 0;
static struct class *lifo_class = NULL;
static struct cdev lifo_dev[minor_devices];

static int __init lifo_init(void){
	int err , i;
	dev_t dev;

	err = alloc_chrdev_region(&dev, 0, minor_devices, name);

	lifo_major = MAJOR(dev);

	lifo_class = class_create(THIS_MODULE, name);

	for(i = 0; i < minor_devices ; i++) {
		cdev_init(&lifo_dev[i], &lifo_fops);
		lifo_dev[i].owner = THIS_MODULE;

		cdev_add(&lifo_dev[i], MKDEV(lifo_major, i), 1);

		device_create(lifo_class, NULL, MKDEV(lifo_major , i), NULL, "lifo_dev-%d", i);
	}
	
	init_waitqueue_head(&wq);

	return 0;
}

static void __exit lifo_exit(void){
	
	int i ;
	
	for(i = 0; i < minor_devices; i++){
		device_destroy(lifo_class, MKDEV(lifo_major, i));
	}

	class_unregister(lifo_class);
	class_destroy(lifo_class);

	unregister_chrdev_region(MKDEV(lifo_major, 0), MINORMASK);
}

static int lifo_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int lifo_release(struct inode *inode, struct file *file)
{		
	return 0;
}

static ssize_t lifo_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
	int ret = 0, i;
	char temp_buf[size_of_msg];
	
	int num = MINOR(file->f_path.dentry->d_inode->i_rdev);
	
	if(num == 1){
		return -EINVAL;
	}
	
	reads++;
	wait_event_interruptible(wq, strlen(msg) > 0);
	
	for( i = 0; i < size_of_msg ; i++){
		temp_buf[i] = msg[size_of_msg -i - 1];
	}

	
	ret = copy_to_user(buf, temp_buf , size_of_msg);
	reads--;
	
	if(reads == 0){

	for(i = 0; i < size_of_msg; i++){
		msg[i] = '\0';
	}
	
	}
	
	if(ret == 0){
		if(reads == 0 ){
		
		size_of_msg = 0;
		ptr = 0;
		}
		return 0;
	}else{
		return -EFAULT;
	}

}

static ssize_t lifo_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
	int num = MINOR(file->f_path.dentry->d_inode->i_rdev);
	int i ;
	char local_msg[count];
	if(num == 0){
		return -EINVAL;
	}
	int ret = copy_from_user(local_msg, buf, count);
	
	for(i = 0; i <count; i++, ptr++){
		msg[ptr] = local_msg[i];
	}
	
	if(ret == 0){
		size_of_msg = strlen(msg);
		
		wake_up_interruptible(&wq);
	
		return count;
	}else{
		return -EFAULT;
	}
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BLACKOLE");

module_init(lifo_init);
module_exit(lifo_exit);
