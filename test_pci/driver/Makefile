obj-m := test_pci_driver.o

all:
	make -C ../../rsrc/linux-5.10.55 M=$(PWD) modules

clean:
	make -C ../../rsrc/linux-5.10.55 M=$(PWD) clean
