
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/home/valkeine/work/linux/include -O2 -Wall

all: pan readback upd

clean:
	rm -f pan readback
