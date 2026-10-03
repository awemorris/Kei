/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The pictures of files: files read as images (binary PPM and
 * PGM), their thumbnails for the icons and the preview, and the fitting
 * of a picture into a box.
 *
 * JPEG (turned as its EXIF orientation says) and GIF (its first frame)
 * are decoded the way Image Viewer decodes them, by the code the two
 * share (userland/desktop/picture, ws094-p013).
 *
 * Thumbnails are made when an item is drawn and not yet kept: the drawing
 * asks, one thumbnail is made each round of the main loop (reading a large
 * picture takes a moment, and the window keeps answering meanwhile), and
 * the item is drawn again with it.  The last FM_THUMBS thumbnails are
 * kept, the least recently drawn going first; a file changed since it was
 * read is read again.
 */

#include "files.h"

#include "../picture/picture.h"

#include <compat/png/png.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* The largest picture file read, in bytes. */
#define THUMB_FILE_MAX		(64U * 1024U * 1024U)

/* The largest picture decoded: its pixels a side, and its pixels in all (64 MiB of them decoded). */
#define THUMB_SIDE_MAX		8192
#define THUMB_PIXELS_MAX	(16UL * 1024UL * 1024UL)

/* The first bytes of a JPEG (its start of image and a marker) and of the two kinds of GIF. */
#define THUMB_JPEG_SIGNATURE	"\xff\xd8\xff"
#define THUMB_GIF87_SIGNATURE	"GIF87a"
#define THUMB_GIF89_SIGNATURE	"GIF89a"

/* The largest number a PPM header is read up to (larger ones are refused). */
#define THUMB_NUMBER_MAX	100000

/*
 * The GIF being read from memory: its bytes and how far they were read
 * (thumb_gif_read).  One lives on thumb_gif's stack for one decoding.
 */
struct thumb_gif_source {
	const unsigned char *data;
	size_t size;
	size_t at;
};

static int thumb_read_file(const char *path, unsigned char **data, size_t *size);
static int thumb_ppm(const unsigned char *data, size_t size, struct fm_image *image);
static int thumb_png(const unsigned char *data, size_t size, struct fm_image *image);
static int thumb_signed(const unsigned char *data, size_t size, const char *signature, size_t length);
static int thumb_jpeg(const unsigned char *data, size_t size, struct fm_image *image);
static int thumb_gif(const unsigned char *data, size_t size, struct fm_image *image);
static int thumb_gif_read(GifFileType *gif, GifByteType *bytes, int count);
static void thumb_adopt(struct keiland_picture *picture, struct fm_image *image);
static int thumb_number(const unsigned char *data, size_t size, size_t *at);
static int thumb_space(unsigned char byte);
static struct fm_thumb *thumb_find(struct fm_app *app, const char *path, time_t modified);
static struct fm_thumb *thumb_slot(struct fm_app *app);

/*
 * Reads a picture file into an image of opaque pixels.
 *
 * Returns 0, ENOTSUP for a format that is not read (PPM, PGM, PNG, JPEG,
 * GIF and the first page of a PDF are), EINVAL for a damaged picture, EFBIG for one too large, or
 * another errno value.
 */
int
fm_image_load(
	const char *path,
	struct fm_image *image)
{
	unsigned char *data;
	size_t size;
	int signed_as;
	int error;

	/* The whole file. */
	memset(image, 0, sizeof(*image));
	error = thumb_read_file(path, &data, &size);
	if (error != 0)
		return error;

	/* A binary PPM or PGM, or a PNG (libpng-compat), is decoded; any other format is not read. */
	error = ENOTSUP;
	if (size > 2U && data[0] == 'P') {
		if (data[1] == '6' || data[1] == '5')
			error = thumb_ppm(data, size, image);
	}

	/* A PNG by its signature. */
	if (size > 8U && data[0] == 0x89U && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
		error = thumb_png(data, size, image);

	/* A JPEG by its start of image and first marker (ws094-p013). */
	signed_as = thumb_signed(data, size, THUMB_JPEG_SIGNATURE, sizeof(THUMB_JPEG_SIGNATURE) - 1U);
	if (signed_as)
		error = thumb_jpeg(data, size, image);

	/* A PDF, its first page (thumb-cache.c, ws127-p002). */
	signed_as = fm_thumb_is_pdf(data, size);
	if (signed_as)
		error = fm_thumb_pdf(data, size, image);

	/* A GIF by its signature, GIF87a or GIF89a. */
	signed_as = thumb_signed(data, size, THUMB_GIF87_SIGNATURE, sizeof(THUMB_GIF87_SIGNATURE) - 1U);
	if (!signed_as)
		signed_as = thumb_signed(data, size, THUMB_GIF89_SIGNATURE, sizeof(THUMB_GIF89_SIGNATURE) - 1U);
	if (signed_as)
		error = thumb_gif(data, size, image);

	/* The file's bytes are not needed any more. */
	free(data);

	/* Reports a picture that could not be read. */
	if (error != 0)
		return error;

	/* Succeeded: the image holds the picture. */
	return 0;
}

/*
 * Reads a picture and shrinks it to fit a square of a side, keeping its
 * shape; a picture smaller than the square keeps its size.
 *
 * Returns 0 or an errno value.
 */
int
fm_image_thumbnail(
	const char *path,
	int side,
	struct fm_image *thumbnail)
{
	struct fm_image full;
	int width;
	int height;
	int error;

	/* The whole picture. */
	memset(thumbnail, 0, sizeof(*thumbnail));
	error = fm_image_load(path, &full);
	if (error != 0)
		return error;

	/* A picture that fits already is the thumbnail itself. */
	if (full.width <= side && full.height <= side) {
		*thumbnail = full;
		return 0;
	}

	/* Its size in the square. */
	fm_image_fit(full.width, full.height, side, side, &width, &height);
	error = fm_image_create(thumbnail, width, height);
	if (error != 0) {
		fm_image_release(&full);
		return error;
	}

	/* The picture averaged down into it. */
	fm_image_scale(&full, thumbnail);
	fm_image_release(&full);

	/* Succeeded: the thumbnail. */
	return 0;
}

/*
 * Returns the kept thumbnail of a file as it is now, or NULL.
 *
 * A file that has no thumbnail yet is asked for, and its thumbnail is made
 * in a later round of the main loop (fm_thumb_tick); only one file is
 * asked for at a time, so the drawing asks again for the others.
 */
const struct fm_image *
fm_thumb_get(
	struct fm_app *app,
	const char *path,
	time_t modified)
{
	struct fm_thumb *thumb;

	/* A thumbnail of the same file as it is now is used, and counts as recently drawn. */
	thumb = thumb_find(app, path, modified);
	if (thumb != NULL) {
		app->thumb_clock++;
		thumb->used = app->thumb_clock;

		/* A file that could not be read has no picture. */
		if (thumb->failed != 0)
			return NULL;

		/* The picture. */
		return &thumb->image;
	}

	/* Otherwise the file is asked for, unless another one already is. */
	if (app->thumb_wanted[0] == '\0') {
		snprintf(app->thumb_wanted, sizeof(app->thumb_wanted), "%s", path);
		app->thumb_wanted_modified = modified;
	}

	/* No picture yet. */
	return NULL;
}

/*
 * Makes the thumbnail asked for, if any, in the slot least recently drawn.
 *
 * Returns nonzero when one was made, so that the window is drawn again.
 */
int
fm_thumb_tick(
	struct fm_app *app)
{
	struct fm_thumb *thumb;
	int cached;
	int error;

	/* Nothing is asked for. */
	if (app->thumb_wanted[0] == '\0')
		return 0;

	/* The slot, emptied of the thumbnail it held. */
	thumb = thumb_slot(app);
	fm_image_release(&thumb->image);

	/*
	 * The slot now stands for the file asked for.  A file that cannot be
	 * read as a picture is kept as failed, so it is not asked for again
	 * until it changes.
	 */
	snprintf(thumb->path, sizeof(thumb->path), "%s", app->thumb_wanted);
	thumb->modified = app->thumb_wanted_modified;
	app->thumb_clock++;
	thumb->used = app->thumb_clock;
	thumb->failed = 0;

	/* The thumbnail kept on disk for the file as it is now, else made and then kept (ws127-p002). */
	cached = 0;
	error = fm_thumb_cache_read(app->thumb_wanted, &thumb->image);
	if (error == 0) {
		cached = 1;
	} else {
		error = fm_image_thumbnail(app->thumb_wanted, FM_THUMB_SIDE, &thumb->image);
		if (error == 0)
			(void)fm_thumb_cache_write(app->thumb_wanted, &thumb->image);
	}

	/* A file that gave no thumbnail is not tried again until it changes. */
	if (error != 0)
		thumb->failed = 1;

	/* The log line the tests wait for, and nothing asked for any more. */
	fm_log("THUMB path=%s error=%d width=%d height=%d cached=%d", app->thumb_wanted, error, thumb->image.width, thumb->image.height, cached);
	app->thumb_wanted[0] = '\0';

	/* Succeeded: a new frame shows it. */
	return 1;
}

/*
 * Frees the kept thumbnails.
 */
void
fm_thumb_release(
	struct fm_app *app)
{
	int index;

	/* Each slot's picture, and the slot emptied. */
	for (index = 0; index < FM_THUMBS; index++) {
		fm_image_release(&app->thumbs[index].image);
		memset(&app->thumbs[index], 0, sizeof(app->thumbs[index]));
	}

	/* Nothing is asked for any more. */
	app->thumb_wanted[0] = '\0';
}

/*
 * Works out the size of a picture fitted in a box, keeping its shape; the
 * fitted picture is at least a pixel each way.
 */
void
fm_image_fit(
	int width,
	int height,
	int box_width,
	int box_height,
	int *fit_width,
	int *fit_height)
{
	/* An empty picture fills nothing. */
	if (width <= 0 || height <= 0) {
		*fit_width = 1;
		*fit_height = 1;
		return;
	}

	/* The box's width decides when the picture is wider in shape than the box; its height otherwise. */
	if ((long)width * box_height >= (long)height * box_width) {
		*fit_width = box_width;
		*fit_height = (int)((long)height * box_width / width);
	} else {
		*fit_height = box_height;
		*fit_width = (int)((long)width * box_height / height);
	}

	/* At least a pixel each way. */
	if (*fit_width < 1)
		*fit_width = 1;
	if (*fit_height < 1)
		*fit_height = 1;
}

/* Reads a whole file of a sane size; returns 0 or an errno value. */
static int
thumb_read_file(
	const char *path,
	unsigned char **data,
	size_t *size)
{
	struct stat status;
	ssize_t count;
	size_t length;
	size_t done;
	int descriptor;
	int error;

	/* The file. */
	*data = NULL;
	*size = 0;
	descriptor = open(path, O_RDONLY);
	if (descriptor < 0)
		return errno;

	/* Its size, which must be of a picture worth reading. */
	error = fstat(descriptor, &status);
	if (error != 0) {
		error = errno;
		close(descriptor);
		return error;
	}

	/* An empty file, and one larger than a picture is read up to, are refused. */
	if (status.st_size <= 0 || (uint64_t)status.st_size > THUMB_FILE_MAX) {
		close(descriptor);
		return EINVAL;
	}

	/* The whole file is read. */
	length = (size_t)status.st_size;

	/* A buffer for all of it. */
	*data = malloc(length);
	if (*data == NULL) {
		close(descriptor);
		return ENOMEM;
	}

	/* The bytes, read until the end or an error. */
	done = 0;
	while (done < length) {
		count = read(descriptor, *data + done, length - done);
		if (count <= 0)
			break;
		done += (size_t)count;
	}

	/* The file is not needed any more. */
	close(descriptor);

	/* A short read leaves no picture. */
	if (done != length) {
		free(*data);
		*data = NULL;
		return EIO;
	}

	/* Succeeded: the caller owns the bytes. */
	*size = done;
	return 0;
}

/* Decodes a binary PPM (P6) or PGM (P5) of 8 bits into an opaque picture; returns 0, EINVAL or ENOMEM. */
static int
thumb_ppm(
	const unsigned char *data,
	size_t size,
	struct fm_image *image)
{
	uint32_t *row;
	size_t needed;
	size_t at;
	uint32_t red;
	uint32_t green;
	uint32_t blue;
	int channels;
	int width;
	int height;
	int maximum;
	int error;
	int x;
	int y;

	/* A PGM has one value a pixel, a PPM three. */
	channels = 3;
	if (data[1] == '5')
		channels = 1;

	/* The header: the width, the height and the largest value, then one space before the pixels. */
	at = 2;
	width = thumb_number(data, size, &at);
	height = thumb_number(data, size, &at);
	maximum = thumb_number(data, size, &at);
	at++;

	/* A size the window handles, and bytes of 8 bits. */
	if (width <= 0 ||
	    height <= 0 ||
	    width > THUMB_SIDE_MAX ||
	    height > THUMB_SIDE_MAX ||
	    maximum != 255)
		return EINVAL;

	/* Not more pixels than the window keeps in memory for a picture. */
	if ((unsigned long)width * (unsigned long)height > THUMB_PIXELS_MAX)
		return EINVAL;

	/* All the pixels are there. */
	needed = (size_t)width * (size_t)height * (size_t)channels;
	if (at > size || needed > size - at)
		return EINVAL;

	/* The picture. */
	error = fm_image_create(image, width, height);
	if (error != 0)
		return error;

	/* Each pixel, opaque; a grey one repeats its value in the three colors. */
	for (y = 0; y < height; y++) {
		row = image->pixels + (size_t)y * image->stride;
		for (x = 0; x < width; x++) {
			red = data[at];
			green = red;
			blue = red;
			if (channels == 3) {
				green = data[at + 1U];
				blue = data[at + 2U];
			}

			/* The pixel, and the next one's bytes. */
			row[x] = 0xff000000U | (red << 16) | (green << 8) | blue;
			at += (size_t)channels;
		}
	}

	/* Succeeded: the picture. */
	return 0;
}

/*
 * Decodes a PNG (libpng-compat's simplified API) into a picture: straight
 * BGRA, multiplied by its alpha into the canvas's premultiplied pixels.
 */
static int
thumb_png(
	const unsigned char *data,
	size_t size,
	struct fm_image *image)
{
	png_image png;
	unsigned char *pixels;
	const unsigned char *pixel;
	uint32_t *row;
	uint32_t alpha;
	int x;
	int y;
	int ok;
	int error;

	/* The header: a size the thumbnails take. */
	memset(&png, 0, sizeof(png));
	png.version = PNG_IMAGE_VERSION;
	ok = png_image_begin_read_from_memory(&png, data, size);
	if (!ok)
		return EINVAL;
	if (png.width == 0U || png.height == 0U || png.width > THUMB_SIDE_MAX || png.height > THUMB_SIDE_MAX ||
	    (unsigned long)png.width * png.height > THUMB_PIXELS_MAX) {
		png_image_free(&png);
		return EFBIG;
	}

	/* The pixels, 8-bit BGRA. */
	png.format = PNG_FORMAT_BGRA;
	pixels = malloc(PNG_IMAGE_SIZE(png));
	if (pixels == NULL) {
		png_image_free(&png);
		return ENOMEM;
	}

	/* Decode. */
	ok = png_image_finish_read(&png, NULL, pixels, 0, NULL);
	if (!ok) {
		free(pixels);
		return EINVAL;
	}

	/* The picture. */
	error = fm_image_create(image, (int)png.width, (int)png.height);
	if (error != 0) {
		free(pixels);
		return error;
	}

	/* Each pixel, its colour multiplied by its alpha. */
	for (y = 0; y < image->height; y++) {
		row = image->pixels + (size_t)y * image->stride;
		for (x = 0; x < image->width; x++) {
			pixel = pixels + ((size_t)y * png.width + (size_t)x) * 4U;
			alpha = pixel[3];
			row[x] = (alpha << 24) |
			    (((uint32_t)pixel[2] * alpha / 255U) << 16) |
			    (((uint32_t)pixel[1] * alpha / 255U) << 8) |
			    ((uint32_t)pixel[0] * alpha / 255U);
		}
	}

	/* The decoded bytes go. */
	free(pixels);

	/* Succeeded: the picture. */
	return 0;
}

/* Reads a decimal number of a PPM header after its spaces and comments; -1 when there is none. */
static int
thumb_number(
	const unsigned char *data,
	size_t size,
	size_t *at)
{
	int space;
	int value;

	/* The spaces and the comment lines before it. */
	while (*at < size) {
		space = thumb_space(data[*at]);
		if (data[*at] == '#') {
			while (*at < size && data[*at] != '\n')
				(*at)++;
		} else if (space != 0) {
			(*at)++;
		} else {
			break;
		}
	}

	/* A number starts with a digit. */
	if (*at >= size || data[*at] < '0' || data[*at] > '9')
		return -1;

	/* Its digits, stopping at a size no picture has. */
	value = 0;
	while (*at < size && data[*at] >= '0' && data[*at] <= '9') {
		if (value > THUMB_NUMBER_MAX)
			return -1;
		value = value * 10 + (data[*at] - '0');
		(*at)++;
	}

	/* Reports the number. */
	return value;
}

/* Tells whether a byte is a space of a PPM header. */
static int
thumb_space(
	unsigned char byte)
{
	/* The blank characters the format allows. */
	if (byte == ' ' || byte == '\t')
		return 1;
	if (byte == '\n' || byte == '\r')
		return 1;

	/* Anything else. */
	return 0;
}

/* Finds the kept thumbnail of a file as it is now; NULL when there is none. */
static struct fm_thumb *
thumb_find(
	struct fm_app *app,
	const char *path,
	time_t modified)
{
	struct fm_thumb *thumb;
	int index;
	int match;

	/* Each slot in use, for the same path and the same modification time. */
	for (index = 0; index < FM_THUMBS; index++) {
		thumb = &app->thumbs[index];
		if (thumb->path[0] == '\0')
			continue;
		if (thumb->modified != modified)
			continue;

		/* The same file. */
		match = strcmp(thumb->path, path);
		if (match == 0)
			return thumb;
	}

	/* No slot holds it. */
	return NULL;
}

/* Chooses the slot a new thumbnail goes in: a free one, else the least recently drawn. */
static struct fm_thumb *
thumb_slot(
	struct fm_app *app)
{
	int oldest;
	int index;

	/* A free slot, or the one drawn longest ago (a free slot was never drawn, so it is the oldest). */
	oldest = 0;
	for (index = 1; index < FM_THUMBS; index++) {
		if (app->thumbs[index].used < app->thumbs[oldest].used)
			oldest = index;
	}

	/* Reports the slot. */
	return &app->thumbs[oldest];
}

/* Tells whether a file's bytes start with a signature (and have more after it). */
static int
thumb_signed(
	const unsigned char *data,
	size_t size,
	const char *signature,
	size_t length)
{
	int differs;

	/* A file no longer than the signature is not a picture. */
	if (size <= length)
		return 0;

	/* The first bytes must be the signature. */
	differs = memcmp(data, signature, length);
	if (differs != 0)
		return 0;

	/* Succeeded: the file starts with the signature. */
	return 1;
}

/*
 * Decodes a JPEG, turned upright as its EXIF orientation says; returns 0,
 * EINVAL (damaged, or a kind not supported), EFBIG or ENOMEM.
 */
static int
thumb_jpeg(
	const unsigned char *data,
	size_t size,
	struct fm_image *image)
{
	struct keiland_picture picture;
	int orientation;
	int error;

	/* The picture as stored, within the sizes the thumbnails take. */
	error = keiland_picture_jpeg(NULL, data, size, THUMB_SIDE_MAX, THUMB_PIXELS_MAX, &picture, &orientation);
	if (error == E2BIG)
		return EFBIG;
	if (error != 0)
		return error;

	/* Turned upright. */
	error = keiland_picture_orient(&picture, orientation);
	if (error != 0) {
		free(picture.pixels);
		return error;
	}

	/* Succeeded: the picture is the image's. */
	thumb_adopt(&picture, image);
	return 0;
}

/* Decodes a GIF's first frame; returns 0, EINVAL (damaged), EFBIG or ENOMEM. */
static int
thumb_gif(
	const unsigned char *data,
	size_t size,
	struct fm_image *image)
{
	struct thumb_gif_source source;
	struct keiland_picture picture;
	GifFileType *gif;
	int status;
	int error;

	/* The file's records, read from the bytes. */
	source.data = data;
	source.size = size;
	source.at = 0;
	error = 0;
	gif = DGifOpen(&source, thumb_gif_read, &error);
	if (gif == NULL)
		return EINVAL;

	/* Every frame's record at once (the first is drawn). */
	status = DGifSlurp(gif);
	if (status != GIF_OK) {
		(void)DGifCloseFile(gif, &error);
		return EINVAL;
	}

	/* The first frame on its clear screen, within the sizes the thumbnails take. */
	error = keiland_picture_gif_first(gif, THUMB_SIDE_MAX, THUMB_PIXELS_MAX, &picture);
	if (error != 0) {
		(void)DGifCloseFile(gif, &status);
		if (error == E2BIG)
			return EFBIG;
		return error;
	}

	/* The records are not needed any more. */
	(void)DGifCloseFile(gif, &status);

	/* Succeeded: the picture is the image's. */
	thumb_adopt(&picture, image);
	return 0;
}

/* Gives libgif-compat up to a count of the GIF's bytes; returns how many were given. */
static int
thumb_gif_read(
	GifFileType *gif,
	GifByteType *bytes,
	int count)
{
	struct thumb_gif_source *source;
	size_t left;

	/* The bytes not read yet, no more than asked for. */
	source = gif->UserData;
	left = source->size - source->at;
	if (count < 0)
		return 0;
	if ((size_t)count < left)
		left = (size_t)count;

	/* Copied out, and the place moves on. */
	memcpy(bytes, source->data + source->at, left);
	source->at += left;
	return (int)left;
}

/* Makes a decoded picture the image (the same pixels, freed by fm_image_release). */
static void
thumb_adopt(
	struct keiland_picture *picture,
	struct fm_image *image)
{
	/* The image takes the pixels; the picture no longer owns them. */
	image->pixels = picture->pixels;
	image->width = picture->width;
	image->height = picture->height;
	image->stride = (size_t)picture->width;
	picture->pixels = NULL;

	/* Succeeded: the image holds the picture. */
	return;
}
