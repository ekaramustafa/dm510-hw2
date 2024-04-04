#
# Module Makefile for DM510 (2024)
#

ROOT = ..
KERNELDIR = ${ROOT}/linux-6.6.9
PWD = $(shell pwd)


obj-m += dm510_dev.o

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(KERNELDIR)/include ARCH=um modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
	