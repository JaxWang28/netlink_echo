# Makefile (内核模块)
obj-m += nl_echo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules


clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

