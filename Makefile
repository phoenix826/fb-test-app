
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/home/valkeine/work/linux/usr/include -O2 -Wall
LDFLAGS=-lm

all: db readback upd perf rect test offset pan

clean:
	rm -f db readback upd perf rect test offset pan
