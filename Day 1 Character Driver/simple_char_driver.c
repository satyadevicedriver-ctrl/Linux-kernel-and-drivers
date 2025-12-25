// simple_chardev.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "simple_chardev"
#define BUF_SIZE 1024

static dev_t dev_num;
static struct cdev my_cdev;
static char kernel_buffer[BUF_SIZE];
static int buffer_pointer = 0;

static int device_open(struct inode *inode, struct file *file) {
    pr_info("%s: Device opened\n", DEVICE_NAME);
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    pr_info("%s: Device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t device_read(struct file *filp, char __user *buffer, 
                          size_t len, loff_t *offset) {
    int bytes_read = 0;
    
    if (*offset >= buffer_pointer)
        return 0;
    
    if (*offset + len > buffer_pointer)
        len = buffer_pointer - *offset;
    
    if (copy_to_user(buffer, kernel_buffer + *offset, len))
        return -EFAULT;
    
    *offset += len;
    bytes_read = len;
    
    pr_info("%s: Read %d bytes\n", DEVICE_NAME, bytes_read);
    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char __user *buffer,
                           size_t len, loff_t *offset) {
    if (len > BUF_SIZE - 1)
        len = BUF_SIZE - 1;
    
    if (copy_from_user(kernel_buffer, buffer, len))
        return -EFAULT;
    
    kernel_buffer[len] = '\0';
    buffer_pointer = len;
    
    pr_info("%s: Wrote %zu bytes\n", DEVICE_NAME, len);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int __init chardev_init(void) {
    int ret;
    
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("%s: Failed to allocate device number\n", DEVICE_NAME);
        return ret;
    }
    
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        pr_err("%s: Failed to add cdev\n", DEVICE_NAME);
        return ret;
    }
    
    pr_info("%s: Registered with major number %d\n", 
            DEVICE_NAME, MAJOR(dev_num));
    return 0;
}

static void __exit chardev_exit(void) {
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("%s: Unregistered\n", DEVICE_NAME);
}

module_init(chardev_init);
module_exit(chardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Simple Character Device Driver");
MODULE_VERSION("1.0");