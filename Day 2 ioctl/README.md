# IOCTL Character Device Driver

## Overview
Demonstrates IOCTL interface for device control with multiple commands.

## Features
- Multiple IOCTL commands (_IOR, _IOW, _IO)
- Counter manipulation (get, set, reset, increment)
- User-space test application

## Commands
- `IOCTL_GET_COUNTER`: Read counter value
- `IOCTL_SET_COUNTER`: Set counter value
- `IOCTL_RESET_COUNTER`: Reset to 0
- `IOCTL_INCREMENT`: Increment by 1

## Build & Test
```bash
make
gcc test_ioctl.c -o test_ioctl
sudo insmod ioctl_chardev.ko
sudo mknod /dev/ioctl_dev c <major> 0
sudo chmod 666 /dev/ioctl_dev
./test_ioctl
```

## Learning Points
- IOCTL command definition macros
- unlocked_ioctl vs ioctl
- Safe user-kernel data exchange
- Command validation