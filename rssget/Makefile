CC = cc -g
CFLAGS = 
INCLUDES = -I/usr/include/libxml2
LIBS = -lcurl -lxml2
RM = rm -f

OBJS = rssget.o
SRCS = ${OBJS:.o=.c}

.c.o:
	$(RM) $@
	$(CC) -c -g $(INCLUDES) $<

rssget: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean: FRC
	${RM} ${OBJS} rssget

depend: FRC
	mkdep ${INCLUDES} ${SRCS}

TAGS: FRC
	etags ${SRCS}

FRC:

