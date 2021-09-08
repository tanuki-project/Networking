#
#	Makefile of linkmonitor
#

CC = gcc
CFLAGS = 
INCLUDES = 
LIBS = 
RM = rm -f

OBJS = linkmonitor.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c -g $(INCLUDES) $<

linkmonitor: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean: FRC
	${RM} ${OBJS} linkmonitor

depend: FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS: FRC
	etags ${SRCS}

FRC:

