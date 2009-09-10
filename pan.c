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

#include "common.h"

struct display_info
{
	unsigned xres;
	unsigned yres;
};

struct fb_info
{
	int fd;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned bytespp;
};

struct frame_info
{
	void *addr;
	unsigned xres, yres;
	unsigned line_len;
	struct fb_info *fb_info;
};

static void clear_frame(struct frame_info *frame)
{
	int y;

	void *p = frame->addr;

	for (y = 0; y < frame->yres; ++y) {
		memset(p, 0, frame->xres * frame->fb_info->bytespp);
		p += frame->line_len;
	}
}

static void draw_bar(struct frame_info *frame, int xpos, int width)
{
	int x, y;

	for (y = 0; y < frame->yres; ++y) {
		unsigned int *lp32 = frame->addr + y * frame->line_len;
		unsigned short *lp16 = frame->addr + y * frame->line_len;
		for (x = xpos; x < xpos+width; ++x) {
			if (frame->fb_info->bytespp == 2)
				lp16[x] = 0xffff;
			else
				lp32[x] = 0xffffffff;
		}
		//usleep(4*1000);
	}
}

static void update_window(struct fb_info *fb_info,
		unsigned width, unsigned height)
{
	struct omapfb_update_window upd;
	upd.x = 0;
	upd.y = 0;
	upd.width = width;
	upd.height = height;
	ioctl(fb_info->fd, OMAPFB_UPDATE_WINDOW, &upd);
}

int main(int argc, char **argv)
{
	int fd;
	int frame;
	struct timeval tv1, tv2, tv;
	struct timeval ftv1, ftv2;
	struct omapfb_mem_info mi;
	struct omapfb_plane_info pi;
	void *fb_base;
	unsigned long min_pan_us, max_pan_us, sum_pan_us;
	int fb_num;
	char str[64];
	enum omapfb_update_mode update_mode;
	int manual;
	struct omapfb_caps caps;

	struct fb_info fb_info;
	struct fb_var_screeninfo *var = &fb_info.var;
	struct fb_fix_screeninfo *fix = &fb_info.fix;

	struct frame_info frame1, frame2;
	struct display_info display_info;

	display_info.xres = 864;
	display_info.yres = 480;


	if (argc == 2)
		fb_num = atoi(argv[1]);
	else
		fb_num = 0;

	sprintf(str, "/dev/fb%d", fb_num);
	fd = open(str, O_RDWR);
	fb_info.fd = fd;

	FBCTL1(FBIOGET_VSCREENINFO, var);
	fb_info.bytespp = var->bits_per_pixel / 8;

	FBCTL1(OMAPFB_QUERY_PLANE, &pi);
	pi.enabled = 0;
	FBCTL1(OMAPFB_SETUP_PLANE, &pi);

	FBCTL1(OMAPFB_QUERY_MEM, &mi);
	mi.size = display_info.xres * (display_info.yres + 32) *
		fb_info.bytespp * 2;
	FBCTL1(OMAPFB_SETUP_MEM, &mi);

	FBCTL1(FBIOGET_VSCREENINFO, var);
	if (var->rotate == 0 || var->rotate == 2) {
		var->xres = display_info.xres;
		var->yres = display_info.yres;
	} else {
		var->xres = display_info.yres;
		var->yres = display_info.xres;
	}
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres * 2;
	FBCTL1(FBIOPUT_VSCREENINFO, var);

	FBCTL1(FBIOGET_FSCREENINFO, fix);

	pi.enabled = 1;
	FBCTL1(OMAPFB_SETUP_PLANE, &pi);

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


	usleep(100000);

	FBCTL1(OMAPFB_GET_UPDATE_MODE, &update_mode);
	if (update_mode == OMAPFB_MANUAL_UPDATE) {
		printf("Manual update mode\n");
		manual = 1;
	} else {
		manual = 0;
	}

	FBCTL1(OMAPFB_GET_CAPS, &caps);
	if (caps.ctrl & OMAPFB_CAPS_TEARSYNC) {
		struct omapfb_tearsync_info tear;
		printf("Enabling TE\n");
		tear.enabled = 1;
		FBCTL1(OMAPFB_SET_TEARSYNC, &tear);
	}

	printf("entering mainloop\n");
	usleep(100000);

	frame = 0;
	gettimeofday(&ftv1, NULL);
	min_pan_us = 0xffffffff;
	max_pan_us = 0;
	sum_pan_us = 0;
	while (1) {
		const int num_frames = 100;
		unsigned long us;
		struct frame_info *current_frame;

		if (frame > 0 && frame % num_frames == 0) {
			unsigned long ms;
			gettimeofday(&ftv2, NULL);
			timersub(&ftv2, &ftv1, &tv);

			ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

			printf("%d frames in %lu ms, %lu fps, "
					"pan min %lu, max %lu, avg %lu\n",
					num_frames,
					ms, num_frames * 1000 / ms,
					min_pan_us, max_pan_us,
					sum_pan_us / num_frames);

			gettimeofday(&ftv1, NULL);
			min_pan_us = 0xffffffff;
			max_pan_us = 0;
			sum_pan_us = 0;
		}

		if (frame % 2 == 0) {
			current_frame = &frame2;
			var->yoffset = 0;
		} else {
			current_frame = &frame1;
			var->yoffset = var->yres;
		}
		frame++;

		gettimeofday(&tv1, NULL);

		FBCTL1(FBIOPAN_DISPLAY, var);

		gettimeofday(&tv2, NULL);
		timersub(&tv2, &tv1, &tv);

		us = tv.tv_sec * 1000000 + tv.tv_usec;

		if (us > max_pan_us)
			max_pan_us = us;
		if (us < min_pan_us)
			min_pan_us = us;
		sum_pan_us += us;

		if (manual) {
			FBCTL0(OMAPFB_SYNC_GFX);
			update_window(&fb_info, display_info.xres,
					display_info.yres);
		} else {
			FBCTL0(OMAPFB_WAITFORGO);
			//FBCTL0(OMAPFB_WAITFORVSYNC);
		}

#if 0
		if (0)
		{
			int x, y, i;
			unsigned int *p32 = fb;
			unsigned short *p16 = fb;
			x = frame % (screen_w - 20);

			memset(fb, 0xff, screen_w*screen_h*bytespp);

			for (i = 0; i < 20; ++i) {
				for (y = 0; y < screen_h; ++y) {
					if (bytespp == 2)
						p16[y * screen_w + x] =
							random();
					else
						p32[y * screen_w + x] =
							random();
				}
				msync(fb, screen_w*screen_h*bytespp, MS_SYNC);
			}
		}
#endif

		if (1) {
			int bar_xpos;
			const int bar_width = 40;
			const int speed = 10;

			clear_frame(current_frame);

			bar_xpos = (frame * speed) % (var->xres - bar_width);

			draw_bar(current_frame, bar_xpos, bar_width);

			msync(current_frame->addr,
					current_frame->line_len * current_frame->yres,
					MS_SYNC);
		}
	}

	close(fd);

	munmap(fb_base, mi.size);

	return 0;
}

