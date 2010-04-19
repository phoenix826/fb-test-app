
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/home/valkeine/work/linux/usr/include -O2 -Wall
LDFLAGS=-lm

PROGS=db readback upd perf rect test offset pan

all: $(PROGS)

$(PROGS): common.c common.h

clean:
	rm -f $(PROGS)
