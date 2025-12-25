# Simple Character Device Driver

## Overview
A basic character device driver demonstrating fundamental Linux kernel module concepts including device registration, file operations, and user-kernel data transfer.

## Features
- Character device registration using cdev interface
- Read/write operations with kernel buffer
- Proper error handling and cleanup
- Copy to/from user space

## Build & Test
```bash
make
sudo insmod simple_chardev.ko
# Check major number
dmesg | tail
# Create device node
sudo mknod /dev/simple_chardev c <major_number> 0
sudo chmod 666 /dev/simple_chardev

# Test
echo "Hello Kernel" > /dev/simple_chardev
cat /dev/simple_chardev

# Cleanup
sudo rmmod simple_chardev
sudo rm /dev/simple_chardev
```

## Learning Points
- Module initialization and cleanup
- Character device registration (alloc_chrdev_region, cdev_add)
- File operations structure
- copy_to_user() and copy_from_user()
- Kernel logging with pr_info()