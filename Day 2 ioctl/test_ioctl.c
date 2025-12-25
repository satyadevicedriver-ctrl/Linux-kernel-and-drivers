#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAGIC_NUM 'k'
#define IOCTL_GET_COUNTER _IOR(MAGIC_NUM, 1, int)
#define IOCTL_SET_COUNTER _IOW(MAGIC_NUM, 2, int)
#define IOCTL_RESET_COUNTER _IO(MAGIC_NUM, 3)
#define IOCTL_INCREMENT _IO(MAGIC_NUM, 4)

int main() {
    int fd, value;
    
    fd = open("/dev/ioctl_dev", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }
    
    // Set counter
    value = 100;
    ioctl(fd, IOCTL_SET_COUNTER, &value);
    
    // Get counter
    ioctl(fd, IOCTL_GET_COUNTER, &value);
    printf("Counter: %d\n", value);
    
    // Increment
    ioctl(fd, IOCTL_INCREMENT);
    ioctl(fd, IOCTL_GET_COUNTER, &value);
    printf("After increment: %d\n", value);
    
    // Reset
    ioctl(fd, IOCTL_RESET_COUNTER);
    ioctl(fd, IOCTL_GET_COUNTER, &value);
    printf("After reset: %d\n", value);
    
    close(fd);
    return 0;
}