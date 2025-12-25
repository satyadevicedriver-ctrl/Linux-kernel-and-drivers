obj-m += simple_fb.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: module userspace

module:
	make -C $(KDIR) M=$(PWD) modules

userspace:
	gcc -Wall -O2 test_fb.c -o test_fb

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -f test_fb

install:
	sudo insmod simple_fb.ko
	@echo "Framebuffer device should be at /dev/fb0 or /dev/fb1"
	ls -l /dev/fb*

uninstall:
	sudo rmmod simple_fb

test: install
	sudo ./test_fb
	$(MAKE) uninstall