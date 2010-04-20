
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/home/valkeine/work/linux/usr/include -O2 -Wall
LDFLAGS=-lm

PROGS=db readback upd perf rect test offset pan ovl

all: $(PROGS)

.c.o: common.h

test: test.o common.o
upd: upd.o common.o
ovl: ovl.o common.o

clean:
	rm -f $(PROGS) *.o
