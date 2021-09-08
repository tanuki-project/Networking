CC = cc
CFLAGS =
LIBS = -L/lib
RM = rm -f

OBJS = scanlldp.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c -g $(INCLUDES) $<

scanlldp: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean: FRC
	${RM} ${OBJS} scanlldp

depend: FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS: FRC
	etags ${SRCS}

FRC:

