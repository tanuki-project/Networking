EXTRA_CFLAGS	= -Wall -DDEBUG

CFILES		= vswitch_main.c vswitch_packet.c vswitch_fdb.c
obj-m		:= vswitch.o
vswitch-objs	:= vswitch_main.o vswitch_packet.o vswitch_fdb.o
TARGET=vswitch.ko

RM		= rm -f
INSTALL		= install
BINDIR		= ../../bin/drv

all:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
		$(INSTALL) -m 0444 vswitch.ko $(BINDIR)

uninstall:
		$(RM) $(BINDIR)/vswitch.ko

