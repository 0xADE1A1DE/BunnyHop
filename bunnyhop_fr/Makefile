PROJ=bh
SRCS=main.c ../src/bunnyhop.c
OBJS=${SRCS:.c=.o}

CFLAGS=-g -O0 
LDFLAGS=-g -O0
LDLIBS=-static -lassemblyline -lmastik

all: ${PROJ}

${PROJ}:${OBJS}
	cc ${LDFLAGS} -o ${PROJ} ${OBJS}  ${LDLIBS}

bunnyhop.o: bunnyhop.h

clean:
	rm -f ${PROJ} ${OBJS}
