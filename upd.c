#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>

#include <linux/fb.h>
#include <linux/omapfb.h>

#include "common.h"

static struct fb_info fb_info;

int main(int argc, char** argv)
{
	int fd;
	int x, y, w, h;
	struct omapfb_display_info *di;

	fb_open(0, &fb_info);

	fd = fb_info.fd;
	di = &fb_info.di;

	if (argc != 5) {
		x = 0;
		y = 0;
		w = di->xres;
		h = di->yres;
	} else {
		x = atoi(argv[1]);
		y = atoi(argv[2]);
		w = atoi(argv[3]);
		h = atoi(argv[4]);
	}

	fb_update_window(fd, x, y, w, h);
	fb_sync_gfx(fd);

	close(fd);

	return 0;
}
