#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/omapfb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <math.h>

#include "common.h"

static unsigned display_xres;
static unsigned display_yres;

static unsigned frame_xres;
static unsigned frame_yres;

static unsigned ovl_xres;
static unsigned ovl_yres;

struct frame_info
{
	void *addr;
	unsigned xres, yres;
	unsigned line_len;
	struct fb_info *fb_info;
};

static void draw_pixel(struct frame_info *frame, int x, int y, unsigned color)
{
	struct fb_info *fb_info = frame->fb_info;
	void *fbmem;

	fbmem = frame->addr;

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

static void draw_line(struct frame_info *frame,
		int x0, int y0, int x1, int y1)
{
	int dy = y1 - y0;
	int dx = x1 - x0;
	float t = (float) 0.5;

	draw_pixel(frame, x0, y0, 0xffffff);
	if (abs(dx) > abs(dy)) {
		float m = (float) dy / (float) dx;
		t += y0;
		dx = (dx < 0) ? -1 : 1;
		m *= dx;
		while (x0 != x1) {
			x0 += dx;
			t += m;
			draw_pixel(frame, x0, (int)t, 0xffffff);
		}
	} else {
		float m = (float) dx / (float) dy;
		t += x0;
		dy = (dy < 0) ? -1 : 1;
		m *= dy;
		while (y0 != y1) {
			y0 += dy;
			t += m;
			draw_pixel(frame, (int)t, y0, 0xffffff);
		}
	}
}

static void clear_frame(struct frame_info *frame)
{
	int y;

	void *p = frame->addr;

	for (y = 0; y < frame->yres; ++y) {
		if (y == 10)
			memset(p, 0xff, frame->xres * frame->fb_info->bytespp);
		//else if (y == frame->yres - 10)
		//	memset(p, 0xff, frame->xres * frame->fb_info->bytespp);
		else
			memset(p, 0, frame->xres * frame->fb_info->bytespp);
		p += frame->line_len;
	}
}

static int origin_x = 864 / 2;
static int origin_y = 480 / 2;

static void draw_linef(struct frame_info* frame, double x1, double y1,
		double x2, double y2, double rot)
{
	double fx1 = x1 * cos(rot) - y1 * sin(rot);
	double fy1 = y1 * cos(rot) + x1 * sin(rot);
	double fx2 = x2 * cos(rot) - y2 * sin(rot);
	double fy2 = y2 * cos(rot) + x2 * sin(rot);

	draw_line(frame, (int)(fx1 + origin_x), (int)(fy1 + origin_y),
			(int)(fx2 + origin_x), (int)(fy2 + origin_y));
}

static void draw_rect(struct frame_info *frame, int frame_num,
		double from, double to, int total_frames)
{
	double rot;
	double step;

	if (total_frames == 1)
		step = 0;
	else
		step = (to - from) / (total_frames - 1);

	rot = (frame_num * step) / 360 * 2 * M_PI +
		from / 360 * 2 * M_PI;

	if (frame_num == total_frames)
		rot = to / 360 * 2 * M_PI;

	draw_linef(frame, 150, -100, 150, 100, rot);
	draw_linef(frame, 150, 100, -150, 100, rot);
	draw_linef(frame, -150, 100, -150, -100, rot);
	draw_linef(frame, -150, -100, 150, -100, rot);

	draw_linef(frame, 0, -50, 0, 50, rot);
	draw_linef(frame, 0, -50, -50, -15, rot);
	draw_linef(frame, 0, -50, 50, -15, rot);
}

static void update_window(struct fb_info *fb_info,
		unsigned width, unsigned height)
{
	struct omapfb_update_window upd;

	upd.x = 0;
	upd.y = 0;
	upd.width = width;
	upd.height = height;
	IOCTL1(fb_info->fd, OMAPFB_UPDATE_WINDOW, &upd);
}

int main(int argc, char **argv)
{
	int fd;
	int frame;
	struct omapfb_mem_info mi;
	struct omapfb_plane_info pi;
	void *fb_base;
	char str[64];
	enum omapfb_update_mode update_mode;
	int manual;
	struct omapfb_caps caps;

	struct fb_info fb_info;
	struct fb_var_screeninfo *var = &fb_info.var;
	struct fb_fix_screeninfo *fix = &fb_info.fix;
	struct omapfb_display_info di;

	struct frame_info frame1, frame2;

	int req_fb = 0;
	int req_bitspp = 32;
	int req_yuv = 0;
	int req_rot = 0;
	int req_te = 1;

	int req_from, req_to, req_frames;

	if (argc != 4) {
		printf("args\n");
		return -1;
	}

	req_from = atoi(argv[1]);
	req_to = atoi(argv[2]);
	req_frames = atoi(argv[3]);




	sprintf(str, "/dev/fb%d", req_fb);
	fd = open(str, O_RDWR);
	fb_info.fd = fd;

	FBCTL1(OMAPFB_GET_DISPLAY_INFO, &di);
	display_xres = di.xres;
	display_yres = di.yres;

	origin_x = display_xres / 2;
	origin_y = display_yres / 2;

	frame_xres = display_xres;
	frame_yres = display_yres;

	ovl_xres = display_xres;
	ovl_yres = display_yres;

	/* disable overlay */
	FBCTL1(OMAPFB_QUERY_PLANE, &pi);
	pi.enabled = 0;
	FBCTL1(OMAPFB_SETUP_PLANE, &pi);

	if (req_bitspp != 0) {
		fb_info.bytespp = req_bitspp / 8;
	} else {
		FBCTL1(FBIOGET_VSCREENINFO, var);
		fb_info.bytespp = var->bits_per_pixel / 8;
	}

	/* allocate memory */
	FBCTL1(OMAPFB_QUERY_MEM, &mi);
	mi.size = frame_xres * frame_yres *
		fb_info.bytespp * 2 * 2;
	FBCTL1(OMAPFB_SETUP_MEM, &mi);

	/* setup var info */
	FBCTL1(FBIOGET_VSCREENINFO, var);
	var->rotate = req_rot;
	if (var->rotate == 0 || var->rotate == 2) {
		var->xres = frame_xres;
		var->yres = frame_yres;
	} else {
		var->xres = frame_yres;
		var->yres = frame_xres;
	}
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * 2;
	var->bits_per_pixel = fb_info.bytespp * 8;
	if (req_yuv == 1)
		var->nonstd = OMAPFB_COLOR_YUV422;
	else if (req_yuv == 2)
		var->nonstd = OMAPFB_COLOR_YUY422;
	else
		var->nonstd = 0;
	FBCTL1(FBIOPUT_VSCREENINFO, var);

	/* setup overlay */
	FBCTL1(FBIOGET_FSCREENINFO, fix);
	pi.out_width = ovl_xres;
	pi.out_height = ovl_yres;
	pi.enabled = 1;
	FBCTL1(OMAPFB_SETUP_PLANE, &pi);

	printf("mmap %u * %u = %u\n", fix->line_length, var->yres_virtual,
			fix->line_length * var->yres_virtual);
	fb_base = mmap(NULL, fix->line_length * var->yres_virtual,
			PROT_READ | PROT_WRITE, MAP_SHARED,
			fd, 0);
	if (fb_base == MAP_FAILED) {
		perror("mmap: ");
		exit(1);
	}

	frame1.addr = fb_base;
	frame1.xres = var->xres;
	frame1.yres = var->yres;
	frame1.line_len = fix->line_length;
	frame1.fb_info = &fb_info;

	frame2 = frame1;
	frame2.addr = fb_base + frame1.yres * frame1.line_len;


	FBCTL1(OMAPFB_GET_UPDATE_MODE, &update_mode);
	if (update_mode == OMAPFB_MANUAL_UPDATE) {
		printf("Manual update mode\n");
		manual = 1;
	} else {
		manual = 0;
	}

	FBCTL1(OMAPFB_GET_CAPS, &caps);
	if ((caps.ctrl & OMAPFB_CAPS_TEARSYNC)) {
		struct omapfb_tearsync_info tear;
		if (req_te)
			printf("Enabling TE\n");
		else
			printf("Disabling TE\n");
		tear.enabled = req_te;
		FBCTL1(OMAPFB_SET_TEARSYNC, &tear);
	}

	frame = 0;

	while (1) {
		struct frame_info *current_frame;
		int total_frames = req_frames;

		if (frame % 2 == 0) {
			current_frame = &frame1;
			var->yoffset = 0;
		} else {
			current_frame = &frame2;
			var->yoffset = var->yres;
		}

		clear_frame(current_frame);
		draw_rect(current_frame, frame, req_from, req_to,
				total_frames);

		FBCTL1(FBIOPAN_DISPLAY, var);

		FBCTL0(OMAPFB_SYNC_GFX);
		update_window(&fb_info, display_xres,
				display_yres);

		frame++;

		if (total_frames <= frame)
			break;

	}

	close(fd);

	munmap(fb_base, mi.size);

	return 0;
}

