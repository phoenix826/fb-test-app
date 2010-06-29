#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

enum color_format
{
	FMT_NONE,
	FMT_RGB565,
	FMT_RGB8888,
};

struct fmt
{
	enum color_format format;
	char* name;
	int bytespp;
};

static struct fmt formats[] = {
	{ FMT_RGB565, "rgb565", 2 },
	{ FMT_RGB8888, "rgb8888", 4 },
};

void readpix(int width, int height, enum color_format format)
{
	void *buf;
	size_t size;
	size_t len;
	int x, y;
	int bytespp;
	int i;

	bytespp = 0;
	for (i = 0; i < sizeof(formats) / sizeof(*formats); ++i) {
		if (formats[i].format == format) {
			bytespp = formats[i].bytespp;
			break;
		}
	}

	if (bytespp == 0)
		exit(1);

	size = width * height * bytespp;
	buf = malloc(size);
	if (buf == NULL)
		error(1, errno, "failed to allocate buffer");

	len = fread(buf, 1, size, stdin);

	if (len != size)
		error(1, errno, "not enough raw pixels");

	fprintf(stdout, "P6 %d %d 255\n", width, height);

	for (y = 0; y < height; ++y) {
		unsigned short *p16 = buf + width * bytespp * y;
		unsigned int *p32 = buf + width * bytespp * y;

		for (x = 0; x < width; ++x) {
			unsigned int r, g, b;

			switch (format) {
			case FMT_RGB565: {
				unsigned short d16 = *p16;
				b = (d16 >> 0) & ((1 << 5) - 1);
				g = (d16 >> 5) & ((1 << 6) - 1);
				r = (d16 >> 11) & ((1 << 5) - 1);
				b = b * 256 / 32;
				g = g * 256 / 64;
				r = r * 256 / 32;
				break;
			}
			case FMT_RGB8888: {
				unsigned int d32 = *p32;
				b = (d32 >> 0) & ((1 << 8) - 1);
				g = (d32 >> 8) & ((1 << 8) - 1);
				r = (d32 >> 16) & ((1 << 8) - 1);
				break;
			default:
				exit(1);
			}
			}

			fprintf(stdout, "%c%c%c", r, g, b);

			p16++;
			p32++;
		}
	}

	fprintf(stdout, "\n");

	free(buf);
}

int main(int argc, char **argv)
{
	int i;
	int w, h;
	enum color_format fmt;

	if (argc != 4)
		error(1, 0, "usage: %s <width> <height> <rgb565|rgb8888>",
				argv[0]);

	w = atoi(argv[1]);
	h = atoi(argv[2]);

	fmt = FMT_NONE;
	for (i = 0; i < sizeof(formats) / sizeof(*formats); ++i) {
		if (strcmp(argv[3], formats[i].name) == 0) {
			fmt = formats[i].format;
		}
	}

	if (fmt == FMT_NONE)
		error(1, 0, "illegal color format");

	readpix(w, h, fmt);

	return 0;
}

