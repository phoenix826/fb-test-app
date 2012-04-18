
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
	CFLAGS=-O2 -Wall
else
	CFLAGS=-O2 -Wall
endif

PROGS=perf rect fb-test offset ovl

all: $(PROGS)

.c.o: common.h font.h

fb-test: fb-test.o common.o font_8x8.c
ovl: ovl.o common.o font_8x8.c

clean:
	rm -f $(PROGS) *.o
