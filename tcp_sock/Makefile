CC = cc
CFLAGS =
LIBS = 
RM = rm -f

OBJS = tcp_sock.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c $(INCLUDES) $<

tcptps: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}
	cp tcptps tcptpc

clean: FRC
	${RM} ${OBJS} tcptps tcptpc core

depend: FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS: FRC
	etags ${SRCS}

FRC:

