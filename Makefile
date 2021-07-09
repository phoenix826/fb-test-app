VERSION = 1
PATCHLEVEL = 1
SUBLEVEL = 1
EXTRAVERSION = .git
NAME = rosetta

ifdef CROSS_COMPILE
	CC=$(CROSS_COMPILE)gcc
	AR=$(CROSS_COMPILE)ar
	CFLAGS=-O2 -Wall
else
	CFLAGS=-O2 -Wall
endif

override CFLAGS += -DVERSION=$(VERSION)
override CFLAGS += -DPATCHLEVEL=$(PATCHLEVEL)
override CFLAGS += -DSUBLEVEL=$(SUBLEVEL)
override CFLAGS += -DVERSION_NAME=\"$(NAME)\"

PROGS = perf rect fb-test offset fb-string
LIBFBTEST = libfbtest.a

all: $(LIBFBTEST) $(PROGS)

$(LIBFBTEST): common.o font_8x8.o
	$(AR) rcs $@ $^

LDLIBS += $(LIBFBTEST)

clean:
	rm -f $(PROGS) $(LIBFBTEST) *.o
