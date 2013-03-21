prefix=/usr/local
man7dir=/usr/local/share/man/man7
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
	-install -m 0644 -g wheel -o root chucky.7 ${man7dir}

deinstall:
	rm -f ${prefix}/bin/chucky
	rm -f ${man7dir}/chucky.7
