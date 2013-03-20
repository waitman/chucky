prefix=/usr/local
CC=clang
CFLAGS=-I/usr/local/include
LDFLAGS=-L/usr/local/lib -lsqlite3

all: chucky

chucky:
	${CC} -std=c99 -O3 -Wall -Werror ${CFLAGS} ${LDFLAGS} -o chucky chucky.c
clean:
	rm chucky

install:
	install -m 0755 -g wheel -o root chucky ${prefix}/bin

deinstall:
	rm -f ${prefix}/bin/chucky
