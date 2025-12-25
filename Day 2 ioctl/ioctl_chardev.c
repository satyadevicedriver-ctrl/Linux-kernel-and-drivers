// ioctl_chardev.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "ioctl_dev"
#define MAGIC_NUM 'k'

#define IOCTL_GET_COUNTER _IOR(MAGIC_NUM, 1, int)
#define IOCTL_SET_COUNTER _IOW(MAGIC_NUM, 2, int)
#define IOCTL_RESET_COUNTER _IO(MAGIC_NUM, 3)
#define IOCTL_INCREMENT _IO(MAGIC_NUM, 4)

static dev_t dev_num;
static struct cdev my_cdev;
static int counter = 0;

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int temp;
    
    switch (cmd) {
    case IOCTL_GET_COUNTER:
        if (copy_to_user((int __user *)arg, &counter, sizeof(int)))
            return -EFAULT;
        pr_info("IOCTL_GET_COUNTER: %d\n", counter);
        break;
        
    case IOCTL_SET_COUNTER:
        if (copy_from_user(&temp, (int __user *)arg, sizeof(int)))
            return -EFAULT;
        counter = temp;
        pr_info("IOCTL_SET_COUNTER: %d\n", counter);
        break;
        
    case IOCTL_RESET_COUNTER:
        counter = 0;
        pr_info("IOCTL_RESET_COUNTER\n");
        break;
        
    case IOCTL_INCREMENT:
        counter++;
        pr_info("IOCTL_INCREMENT: %d\n", counter);
        break;
        
    default:
        return -EINVAL;
    }
    
    return 0;
}

static int device_open(struct inode *inode, struct file *file) {
    pr_info("Device opened\n");
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    pr_info("Device closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static int __init ioctl_init(void) {
    int ret;
    
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;
    
    cdev_init(&my_cdev, &fops);
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    
    pr_info("%s: Registered (Major: %d)\n", DEVICE_NAME, MAJOR(dev_num));
    return 0;
}

static void __exit ioctl_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("%s: Unregistered\n", DEVICE_NAME);
}

module_init(ioctl_init);
module_exit(ioctl_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("IOCTL Character Device Driver");