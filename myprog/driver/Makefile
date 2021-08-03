export ARCH := arm64
export KERN_DIR := ../../rsrc/linux-5.10.55
export CROSS_COMPILE := aarch64-linux-gnu-

ifneq ($(KERN_DIR),)
KernelDir := $(KERN_DIR)
else
KernelDir := /lib/modules/$(shell uname -r)/build
endif

CFILES = vpci_drv.c

obj-m := vpci.o
vpci-objs := $(CFILES:.c=.o)

ccflags-y += -std=gnu99 -Wall -Wno-declaration-after-statement -D$(ARCH)
ifneq ($(LOG_LEVEL),)
  ccflags-y += -DLOG_LEVEL=$(LOG_LEVEL)
endif

all:
	make -C $(KernelDir) M=$(shell pwd) modules
clean:
	make -C $(KernelDir) M=$(shell pwd) clean
	rm -f *~

# install:
# 	sudo insmod i2c_module.ko
# 	sudo chmod 0666 /dev/i2cdrv0
# uninstall:
# 	sudo rmmod i2c_module


