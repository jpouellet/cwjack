CC=clang
SOURCES=main.c jack.c key.c
BIN=cwjack
CFLAGS=-Wall -pedantic
LIBS=-l jack -framework ApplicationServices
MANPAGE=${BIN}.1
BINPATH=/usr/local/bin
MANPATH=/usr/local/share/man/man1/

all:
	${CC} ${SOURCES} -o ${BIN} ${CFLAGS} ${LIBS}

debug:
	${CC} -g ${SOURCES} -o ${BIN} ${CFLAGS} ${LIBS}

.PHONY: clean
clean:
	rm -f ${BIN}
	rm -rf ${BIN}.dSYM

.PHONY: install
install:
	cp ${BIN} ${BINPATH}${BIN}
	cp ${MANPAGE} ${MANPATH}${MANPAGE}

.PHONY: uninstall
uninstall:
	rm -f ${BINPATH}${BIN}
	rm -f ${MANPATH}${MANPAGE}
