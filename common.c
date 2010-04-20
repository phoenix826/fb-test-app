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

void fb_open(int fb_num, struct fb_info *fb_info)
{
	char str[64];
	int fd;

	sprintf(str, "/dev/fb%d", fb_num);
	fd = open(str, O_RDWR);

	ASSERT(fd >= 0);

	fb_info->fd = fd;
	FBCTL1(OMAPFB_GET_DISPLAY_INFO, &fb_info->di);
	FBCTL1(FBIOGET_VSCREENINFO, &fb_info->var);
	FBCTL1(FBIOGET_FSCREENINFO, &fb_info->fix);

	printf("display %dx%d\n", fb_info->di.xres, fb_info->di.yres);
	printf("fb res %dx%d virtual %dx%d, line_len %d\n",
			fb_info->var.xres, fb_info->var.yres,
			fb_info->var.xres_virtual, fb_info->var.yres_virtual,
			fb_info->fix.line_length);
	printf("dim %dum x %dum\n", fb_info->var.width, fb_info->var.height);

	void* ptr = mmap(0,
			fb_info->var.yres_virtual * fb_info->fix.line_length,
			PROT_WRITE | PROT_READ,
			MAP_SHARED, fd, 0);

	ASSERT(ptr != MAP_FAILED);

	fb_info->ptr = ptr;
}

void fb_update_window(int fd, short x, short y, short w, short h)
{
	struct omapfb_update_window uw;

	uw.x = x;
	uw.y = y;
	uw.width = w;
	uw.height = h;

	//printf("update %d,%d,%d,%d\n", x, y, w, h);
	IOCTL1(fd, OMAPFB_UPDATE_WINDOW, &uw);
}

void fb_sync_gfx(int fd)
{
	IOCTL0(fd, OMAPFB_SYNC_GFX);
}

