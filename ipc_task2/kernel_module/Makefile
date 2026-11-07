obj-m += kbd_driver.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

.PHONY: all clean load unload reload

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f modules.order Module.symvers *.ko *.mod.c *.o .*.cmd

load:
	sudo insmod kbd_driver.ko
	sudo chmod 666 /dev/keyboard_dev 2>/dev/null || true
	@echo "Module kbd_driver loaded. Check dmesg | tail -n 20"

unload:
	sudo rmmod kbd_driver
	@echo "Module kbd_driver unloaded."

reload: unload load

