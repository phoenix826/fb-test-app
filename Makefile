
CC=$(CROSS_COMPILE)gcc
CFLAGS=-I/home/valkeine/work/linux/include -O2 -Wall

pan: pan.c

clean:
	rm -f pan
