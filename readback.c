#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <linux/fb.h>
#include <linux/omapfb.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "common.h"

static struct fb_var_screeninfo var;
static struct fb_fix_screeninfo fix;

static void readmem(int fd, int x, int y, int w, int h)
{
	struct omapfb_memory_read mr;
	int len;
	int bytespp = 3;

	mr.buffer_size = w * h * bytespp;
	mr.buffer = malloc(mr.buffer_size);
	mr.x = x;
	mr.y = y;
	mr.w = w;
	mr.h = h;

	fprintf(stderr, "reading %d bytes\n", mr.buffer_size);

	len = ioctl(fd, OMAPFB_MEMORY_READ, &mr);

	fprintf(stderr, "read returned %d bytes\n", len);

	if (len > 0) {
		int ffd;
		fprintf(stderr, "writing to buf-24.raw\n");
		ffd = open("buf-24.raw", O_WRONLY | O_CREAT | O_TRUNC);
		if (ffd < 0)
			printf("open failed\n");
		write(ffd, mr.buffer, len);
		close(ffd);
	}

	if (len > 0) {
		int ffd;
		int i;
		unsigned char *buf, *p1, *p2;
		fprintf(stderr, "writing to buf-32.raw\n");
		ffd = open("buf-32.raw", O_WRONLY | O_CREAT | O_TRUNC);
		if (ffd < 0)
			printf("open failed\n");
		buf = malloc(len * 4 / 3);
		p1 = mr.buffer;
		p2 = buf;
		for (i = 0; i < len; i += 3)
		{
			p2[0] = p1[2];
			p2[1] = p1[1];
			p2[2] = p1[0];
			p2[3] = 0;
			p1 += 3;
			p2 += 4;
		}

		write(ffd, buf, len * 4 / 3);

		close(ffd);
		free(buf);
	}

	free(mr.buffer);
}

int main(int argc, char **argv)
{
	int fd;
	int x, y, w, h;

	fd = open("/dev/fb0", O_RDWR);

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

	readmem(fd, x, y, w, h);

	close(fd);

	return 0;
}

