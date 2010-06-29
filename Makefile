
ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
	CFLAGS=-I/home/valkeine/work/linux/usr/include -O2 -Wall
else
	CFLAGS=-O2 -Wall
endif
LDFLAGS=-lm

PROGS=db readback upd perf rect test offset pan ovl dbrot

all: $(PROGS)

.c.o: common.h font.h

test: test.o common.o font_8x8.c
upd: upd.o common.o font_8x8.c
ovl: ovl.o common.o font_8x8.c

clean:
	rm -f $(PROGS) *.o
