INCLUDE		= -I ../../src/drv
CC		= /usr/bin/gcc
CFLAGS		= $(INCLUDE) -O -Wall
#CFLAGS		= $(INCLUDE) -g -Wall
RM		= rm -f
INSTALL		= install
BINDIR		= ../../bin/cmd

all:		vswconfig
pswctl:;	$(CC) $(CFLAGS) vswconfig.c -o vswconfig
clean:;		$(RM) vswconfig
install:;	$(INSTALL) -m 0555 vswconfig $(BINDIR)
uninstall:;	$(RM) $(BINDIR)/vswconfig

