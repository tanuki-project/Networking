#
#	Makefile of linkmonitor
#

CC = gcc
CFLAGS = 
INCLUDES = 
LIBS = -ludev
RM = rm -f

OBJS = udev_monitor.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c -g $(INCLUDES) $<

udev_monitor: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean: FRC
	${RM} ${OBJS} udev_monitor

depend: FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS: FRC
	etags ${SRCS}

FRC:

