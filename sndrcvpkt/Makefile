#ident  "%W% %G% %U% - FUJITSU"

CC = /usr/bin/gcc
CFLAGS =
LIBS = -L/lib
RM = rm -f

OBJS = sndrcvpkt.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c $(INCLUDES) $<

sndrcvpkt:	${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean:	FRC
	${RM} ${OBJS} sndrcvpkt

depend:	FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS:	FRC
	etags ${SRCS}

FRC:

