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

#define ERROR(x) printf("fbtest error in line %s:%d: %s\n", __FUNCTION__, __LINE__, strerror(errno));

#define FBCTL(cmd, arg)			\
	if(ioctl(fd, cmd, arg) == -1) {	\
		ERROR("ioctl failed");	\
		exit(1); }

#define FBCTL0(cmd)			\
	if(ioctl(fd, cmd) == -1) {	\
		ERROR("ioctl failed");	\
		exit(1); }



int main(int argc, char** argv)
{
	char str[64];
	int fd;
	struct fb_var_screeninfo var;

	int opt;
	int req_fb = 0;

	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
		case 'f':
			req_fb = atoi(optarg);
			break;
		default:
			printf("usage: %s [-f <fbnum>] <x> <y>\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (argc - optind != 2) {
		printf("usage: %s [-f <fbnum>] <x> <y>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	sprintf(str, "/dev/fb%d", req_fb);
	fd = open(str, O_RDWR);

	FBCTL(FBIOGET_VSCREENINFO, &var);
	var.xoffset = atoi(argv[optind + 0]);
	var.yoffset = atoi(argv[optind + 1]);
	FBCTL(FBIOPUT_VSCREENINFO, &var);

	return 0;
}
