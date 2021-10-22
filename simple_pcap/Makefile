CC = /usr/bin/gcc
CFLAGS = -g
LIBS = -L/lib
RM = rm -f

OBJS = pcap.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c $(INCLUDES) $<

pcap:	${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean:	FRC
	${RM} ${OBJS} pcap

depend:	FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS:	FRC
	etags ${SRCS}

FRC:

