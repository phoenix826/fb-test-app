#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/fb.h>
#include <linux/omapfb.h>

#include "common.h"

struct fb_var_screeninfo var;
struct fb_fix_screeninfo fix;

int open_fb(const char* dev)
{
	int fd = -1;
	fd = open(dev, O_RDWR);
	if(fd == -1)
	{
		printf("Error opening device %s : %s\n", dev, strerror(errno));
		exit(-1);
	}

	return fd;
}

static int fb_update_window(int fd, short x, short y, short w, short h)
{
	struct omapfb_update_window uw;

	uw.x = x;
	uw.y = y;
	uw.width = w;
	uw.height = h;

	//printf("update %d,%d,%d,%d\n", x, y, w, h);
	FBCTL1(OMAPFB_UPDATE_WINDOW, &uw);
	FBCTL0(OMAPFB_SYNC_GFX);

	return 0;
}

int main(int argc, char** argv)
{
	int fd;
	int x, y, w, h;

	fd = open_fb("/dev/fb0");

	FBCTL1(FBIOGET_VSCREENINFO, &var);
	FBCTL1(FBIOGET_FSCREENINFO, &fix);

	if (argc != 5) {
		x = 0;
		y = 0;
		w = var.xres;
		h = var.yres;
	} else {
		x = atoi(argv[1]);
		y = atoi(argv[2]);
		w = atoi(argv[3]);
		h = atoi(argv[4]);
	}

	fb_update_window(fd, x, y, w, h);

	close(fd);

	return 0;
}
