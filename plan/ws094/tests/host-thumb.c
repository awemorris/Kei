/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws094-p013: the file manager's reading of a picture (fm_image_load,
 * userland/desktop/files/thumb.c) on the host, for host-thumb.sh, which
 * compares it with Python's PIL: the error, the size, and the pixels
 * (premultiplied 0xAARRGGBB words, little-endian, row by row).
 *
 *   host-thumb PICTURE OUT
 */

#include "files.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Reads the picture and writes what was read.
 */
int
main(
	int argc,
	char **argv)
{
	struct fm_image image;
	FILE *out;
	size_t written;
	size_t row_written;
	size_t expected;
	int close_error;
	int error;
	int y;

	/* The picture and where its pixels go. */
	if (argc != 3) {
		fprintf(stderr, "usage: host-thumb PICTURE OUT\n");
		return 2;
	}

	/* The error and the size, printed whatever they are. */
	error = fm_image_load(argv[1], &image);
	printf("error=%d width=%d height=%d\n", error, image.width, image.height);
	if (error != 0)
		return 0;

	/* The pixels, row by row. */
	out = fopen(argv[2], "wb");
	if (out == NULL) {
		fm_image_release(&image);
		return 1;
	}

	/* Each row's pixels (the stride may be longer than a row). */
	written = 0;
	expected = (size_t)image.width * (size_t)image.height;
	for (y = 0; y < image.height; y++) {
		/* A short row cannot supply a complete reference image. */
		row_written = fwrite(image.pixels + (size_t)y * image.stride, sizeof(uint32_t), (size_t)image.width, out);
		if (row_written != (size_t)image.width) {
			fclose(out);
			fm_image_release(&image);
			return 1;
		}

		/* Counts complete rows toward the expected reference image size. */
		written += row_written;
	}

	/* Flushes the output before releasing the decoded pixel storage. */
	close_error = fclose(out);
	if (close_error != 0) {
		fm_image_release(&image);
		return 1;
	}

	/* The complete output no longer needs the decoded image. */
	fm_image_release(&image);

	/* Refuses an incomplete reference image. */
	if (written != expected)
		return 1;

	/* Succeeded: every decoded pixel was written. */
	return 0;
}
