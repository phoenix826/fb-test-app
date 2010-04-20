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
#include "font.h"

void fb_open(int fb_num, struct fb_info *fb_info)
{
	char str[64];
	int fd;

	sprintf(str, "/dev/fb%d", fb_num);
	fd = open(str, O_RDWR);

	ASSERT(fd >= 0);

	fb_info->fd = fd;
	IOCTL1(fd, OMAPFB_GET_DISPLAY_INFO, &fb_info->di);
	IOCTL1(fd, FBIOGET_VSCREENINFO, &fb_info->var);
	IOCTL1(fd, FBIOGET_FSCREENINFO, &fb_info->fix);
	IOCTL1(fd, OMAPFB_GET_UPDATE_MODE, &fb_info->update_mode);

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

static void fb_clear_area(struct fb_info *fb_info, int x, int y, int w, int h)
{
	int i = 0;
	int loc;
	char *fbuffer = (char *)fb_info->ptr;
	struct fb_var_screeninfo *var = &fb_info->var;
	struct fb_fix_screeninfo *fix = &fb_info->fix;

	for(i = 0; i < h; i++)
	{
		loc = (x + var->xoffset) * (var->bits_per_pixel / 8)
			+ (y + i + var->yoffset) * fix->line_length;
		memset(fbuffer + loc, 0, w * var->bits_per_pixel / 8);
	}
}

static void fb_put_char(struct fb_info *fb_info, int x, int y, char c,
		unsigned color)
{
	int i, j, bits, loc;
	char *fbuffer = (char *)fb_info->ptr;
	struct fb_var_screeninfo *var = &fb_info->var;
	struct fb_fix_screeninfo *fix = &fb_info->fix;

	for(i = 0; i < font_vga_8x8.height; i++) {
		bits = font_vga_8x8.data[font_vga_8x8.height * c + i];
		for(j = 0; j < 8; j++) {
			loc = (x + j + var->xoffset) * (var->bits_per_pixel / 8)
				+ (y + i + var->yoffset) * fix->line_length;
			if(loc >= 0 && loc < fix->smem_len && ((bits >> (7 - j)) & 1)) {
				switch(var->bits_per_pixel) {
					case 8:
					default:
						if(color==0)
							fbuffer[loc] = 0;
						else
							fbuffer[loc] = 1;
						break;
					case 16:
						*((unsigned short *)(fbuffer + loc)) = color;
						break;
					case 24:
					case 32:
						*((unsigned int *)(fbuffer + loc)) = color;
						break;
				}
			}
		}
	}
}

int fb_put_string(struct fb_info *fb_info, int x, int y, char *s, int maxlen,
		int color, int clear, int clearlen)
{
	int i;
	int w = 0;

	if(clear)
		fb_clear_area(fb_info, x, y, clearlen * font_vga_8x8.width,
				font_vga_8x8.height);

	for(i=0;i<strlen(s) && i < maxlen;i++) {
		fb_put_char(fb_info, (x + font_vga_8x8.width * i), y, s[i],
				color);
		w += font_vga_8x8.width;
	}

	return w;
}

