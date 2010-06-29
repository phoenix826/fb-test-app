#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <math.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/fb.h>
#include <linux/omapfb.h>

#include "common.h"

static struct fb_info fb_info;

static void draw_pixel(struct fb_info *fb_info, int x, int y, unsigned color)
{
	void *fbmem;

	fbmem = fb_info->ptr;

	if (fb_info->var.bits_per_pixel == 16) {
		unsigned short c;
		unsigned r = (color >> 16) & 0xff;
		unsigned g = (color >> 8) & 0xff;
		unsigned b = (color >> 0) & 0xff;
		unsigned short *p;

		r = r * 32 / 256;
		g = g * 64 / 256;
		b = b * 32 / 256;

		c = (r << 11) | (g << 5) | (b << 0);

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = c;
	} else {
		unsigned int *p;

		fbmem += fb_info->fix.line_length * y;

		p = fbmem;

		p += x;

		*p = color;
	}
}

static void fill_screen(struct fb_info *fb_info)
{
	unsigned x, y;
	unsigned h = fb_info->var.yres_virtual;
	unsigned w = fb_info->var.xres_virtual;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if (x < 20 && y < 20)
				draw_pixel(fb_info, x, y, 0xffffff);
			else if (x < 20 && (y > 20 && y < h - 20))
				draw_pixel(fb_info, x, y, 0xff);
			else if (y < 20 && (x > 20 && x < w - 20))
				draw_pixel(fb_info, x, y, 0xff00);
			else if (x > w - 20 && (y > 20 && y < h - 20))
				draw_pixel(fb_info, x, y, 0xff0000);
			else if (y > h - 20 && (x > 20 && x < w - 20))
				draw_pixel(fb_info, x, y, 0xffff00);
			else if (x == 20 || x == w - 20 ||
					y == 20 || y == h - 20)
				draw_pixel(fb_info, x, y, 0xffffff);
			else if (x == y || w - x == h - y)
				draw_pixel(fb_info, x, y, 0xff00ff);
			else if (w - x == y || x == h - y)
				draw_pixel(fb_info, x, y, 0x00ffff);
			else if (x > 20 && y > 20 && x < w - 20 && y < h - 20) {
				int t = x * 3 / w;
				unsigned r = 0, g = 0, b = 0;
				unsigned c;
				if (fb_info->var.bits_per_pixel == 16) {
					if (t == 0)
						b = (y % 32) * 256 / 32;
					else if (t == 1)
						g = (y % 64) * 256 / 64;
					else if (t == 2)
						r = (y % 32) * 256 / 32;
				} else {
					if (t == 0)
						b = (y % 256);
					else if (t == 1)
						g = (y % 256);
					else if (t == 2)
						r = (y % 256);
				}
				c = (r << 16) | (g << 8) | (b << 0);
				draw_pixel(fb_info, x, y, c);
			} else {
				draw_pixel(fb_info, x, y, 0);
			}
		}

	}
}

void move_ovl(unsigned x, unsigned y, unsigned w, unsigned h,
		unsigned ox, unsigned oy)
{
	int fd = fb_info.fd;
	struct fb_var_screeninfo *var = &fb_info.var;
	struct omapfb_plane_info pi;

	//printf("%d,%d %dx%d (%d,%d)\n", x, y, w, h, ox, oy);


	IOCTL1(fd, OMAPFB_QUERY_PLANE, &pi);
	pi.enabled = 0;
	IOCTL1(fd, OMAPFB_SETUP_PLANE, &pi);

	IOCTL1(fd, FBIOGET_VSCREENINFO, var);
	var->xres = w;
	var->yres = h;
	var->xoffset = ox;
	var->yoffset = oy;
	IOCTL1(fd, FBIOPUT_VSCREENINFO, var);

	IOCTL1(fd, OMAPFB_QUERY_PLANE, &pi);
	pi.out_width = w; //*2/3;
	pi.out_height = h; //*2/3;
	pi.pos_x = x;
	pi.pos_y = y;
	pi.enabled = 1;
	IOCTL1(fd, OMAPFB_SETUP_PLANE, &pi);

}

int main(int argc, char** argv)
{
	unsigned c;

	int opt;
	int req_fb = 0;
	int req_bitspp = 32;
	int req_yuv = 0;
	int req_rot = 0;

	while ((opt = getopt(argc, argv, "f:r:m:y:")) != -1) {
		switch (opt) {
		case 'f':
			req_fb = atoi(optarg);
			break;
		case 'r':
			req_rot = atoi(optarg);
			break;
		case 'm':
			req_bitspp = atoi(optarg);
			break;
		case 'y':
			req_yuv = atoi(optarg);
			break;
		default:
			printf("usage: -f <fbnum> -r <rot> -m <bitspp> "
					"-y <yuv>\n");
			exit(EXIT_FAILURE);
		}
	}

	fb_open(req_fb, &fb_info, 0);

	fill_screen(&fb_info);

	c = 0;
	while (1) {
		unsigned x, y, w, h, ox, oy;

		w = (sin((float)c * M_PI / 180 - M_PI / 2) + 1.0) / 2.0 *
			400 + 8;

		h = (sin((float)c * M_PI / 180 *1.2 - M_PI / 2) + 1.0) / 2.0 *
			400 + 8;

		x = (sin((float)c * M_PI / 180 - M_PI / 2) + 1.0) / 2.0 *
			(fb_info.di.xres - w - 1);

		y = (sin((float)c * M_PI / 180 * 1.3 - M_PI / 2) + 1.0) / 2.0 *
			(fb_info.di.yres - h - 1);

		ox = (cos((float)c * M_PI / 180 * 1.1 - M_PI / 2) + 1.0) / 2.0 *
			(fb_info.var.xres_virtual - w);

		oy = (sin((float)c * M_PI / 180 * 1.2 - M_PI / 2) + 1.0) / 2.0 *
			(fb_info.var.yres_virtual - h);

		move_ovl(x, y, w, h, ox, oy);

		if (fb_info.update_mode == OMAPFB_MANUAL_UPDATE) {
			fb_update_window(fb_info.fd, 0, 0,
					fb_info.di.xres, fb_info.di.yres);
			fb_sync_gfx(fb_info.fd);
		}
		c++;
	}

	return 0;
}
