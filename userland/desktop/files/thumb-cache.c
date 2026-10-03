/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The thumbnails of PDF documents and the thumbnails kept on disk
 * (ws127-p002, F-035).
 *
 * A PDF's thumbnail is its first page, drawn by libpdf (as PDF Viewer draws
 * it).  libpdf is opened with dlopen the first time a PDF asks: files does
 * not link it, so a system without it shows the PDF's icon as before.
 *
 * A thumbnail made once is kept in the user's cache folder
 * ($XDG_CACHE_HOME, else ~/.cache, then keiland/thumbnails), named by the
 * SHA-256 of the file's path, as a binary PPM whose comment line records the
 * file's modification time and size.  A file that changed has a stale
 * record, which is made again; the folder is the user's alone (0700).
 */

#include "files.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pdf.h>
#include <sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* The longest side a PDF's first page is drawn at before it is shrunk to a thumbnail, in pixels. */
#define CACHE_PDF_SIDE		512

/* The first bytes of a PDF. */
#define CACHE_PDF_SIGNATURE	"%PDF-"

/* The largest thumbnail read back from the cache, a side in pixels. */
#define CACHE_SIDE_MAX		1024

/* The cache's folder under the user's cache folder, and the comment that marks a record. */
#define CACHE_FOLDER		"keiland/thumbnails"
#define CACHE_MARK		"# keiland-thumbnail"

/*
 * The calls of libpdf files uses, found with dlsym.  Loaded is 0 before
 * the first attempt, 1 when they are all there and -1 when libpdf could not
 * be opened (it is not tried again); the library stays open for the life
 * of the program.
 */
struct cache_pdf_calls {
	int loaded;
	void *library;
	int (*open_memory)(const void *data, size_t size, struct pdf_document **document);
	void (*close)(struct pdf_document *document);
	size_t (*page_count)(const struct pdf_document *document);
	int (*page_box)(struct pdf_document *document, size_t index, struct pdf_page_box *box);
	int (*render)(struct pdf_document *document, size_t index, struct pdf_display_list **list);
	void (*list_destroy)(struct pdf_display_list *list);
	int (*rasterize)(const struct pdf_display_list *list, uint32_t *pixels, size_t stride, size_t width, size_t height, double scale, double offset_x, double offset_y);
};

/* libpdf's calls, opened on the first PDF (see the structure). */
static struct cache_pdf_calls cache_pdf;

static int cache_pdf_load(void);
static int cache_record_path(const char *path, char *record, size_t size);
static int cache_folder(char *folder, size_t size);
static int cache_header(FILE *file, long long *modified, unsigned long long *size, int *width, int *height);

/*
 * Tells whether an item is drawn with a thumbnail: a picture, or a PDF
 * document (its first page).
 */
int
fm_thumb_kind(
	const struct fm_entry *entry)
{
	/* Folders and items without a path have none. */
	if (entry->folder != 0 || entry->path == NULL || entry->mime == NULL)
		return 0;

	/* Pictures. */
	if (entry->mime->category == FM_CATEGORY_IMAGE)
		return 1;

	/* PDF documents. */
	if (entry->mime->category == FM_CATEGORY_PDF)
		return 1;

	/* Anything else is drawn with its kind's icon. */
	return 0;
}

/*
 * Tells whether a file's bytes are a PDF's.
 */
int
fm_thumb_is_pdf(
	const unsigned char *data,
	size_t size)
{
	int differs;

	/* The signature, and something after it. */
	if (size <= sizeof(CACHE_PDF_SIGNATURE) - 1U)
		return 0;
	differs = memcmp(data, CACHE_PDF_SIGNATURE, sizeof(CACHE_PDF_SIGNATURE) - 1U);
	if (differs != 0)
		return 0;

	/* A PDF. */
	return 1;
}

/*
 * Draws a PDF's first page into an image (on white), its longest side
 * CACHE_PDF_SIDE pixels.  Returns 0, ENOTSUP when libpdf is not there,
 * EINVAL for a document that cannot be read, or another errno value.
 */
int
fm_thumb_pdf(
	const unsigned char *data,
	size_t size,
	struct fm_image *image)
{
	struct pdf_display_list *list;
	struct pdf_document *document;
	struct pdf_page_box box;
	double width;
	double height;
	double scale;
	size_t count;
	size_t index;
	int pixels_wide;
	int pixels_high;
	int loaded;
	int error;

	/* libpdf, opened the first time. */
	memset(image, 0, sizeof(*image));
	loaded = cache_pdf_load();
	if (loaded == 0)
		return ENOTSUP;

	/* The document and its first page's size as shown (libpdf's, turned and cropped), else its crop or media box. */
	document = NULL;
	error = cache_pdf.open_memory(data, size, &document);
	if (error != 0 || document == NULL) {
		fm_log("THUMB pdf stage=open error=%d", error);
		return EINVAL;
	}
	count = cache_pdf.page_count(document);
	if (count == 0) {
		cache_pdf.close(document);
		return EINVAL;
	}
	memset(&box, 0, sizeof(box));
	error = cache_pdf.page_box(document, 0, &box);
	if (error != 0) {
		fm_log("THUMB pdf stage=box error=%d", error);
		cache_pdf.close(document);
		return EINVAL;
	}
	width = box.width;
	height = box.height;
	if (width <= 0.0 || height <= 0.0) {
		width = box.crop_right - box.crop_left;
		height = box.crop_top - box.crop_bottom;
	}
	if (width <= 0.0 || height <= 0.0) {
		width = box.media_right - box.media_left;
		height = box.media_top - box.media_bottom;
	}
	if (width <= 0.0 || height <= 0.0) {
		fm_log("THUMB pdf stage=size width=%g height=%g", width, height);
		cache_pdf.close(document);
		return EINVAL;
	}

	/* The scale that makes its longest side CACHE_PDF_SIDE pixels. */
	scale = (double)CACHE_PDF_SIDE / width;
	if (height > width)
		scale = (double)CACHE_PDF_SIDE / height;
	pixels_wide = (int)(width * scale + 0.5);
	pixels_high = (int)(height * scale + 0.5);
	if (pixels_wide < 1)
		pixels_wide = 1;
	if (pixels_high < 1)
		pixels_high = 1;

	/* The image, white like paper. */
	error = fm_image_create(image, pixels_wide, pixels_high);
	if (error != 0) {
		cache_pdf.close(document);
		return error;
	}
	for (index = 0; index < (size_t)pixels_wide * (size_t)pixels_high; index++)
		image->pixels[index] = 0xffffffffU;

	/* The page drawn on it. */
	list = NULL;
	error = cache_pdf.render(document, 0, &list);
	if (error == 0 && list != NULL) {
		error = cache_pdf.rasterize(list, image->pixels, image->stride, (size_t)pixels_wide, (size_t)pixels_high, scale, 0.0, 0.0);
		cache_pdf.list_destroy(list);
	}
	cache_pdf.close(document);

	/* A page that could not be drawn leaves no thumbnail. */
	if (error != 0) {
		fm_log("THUMB pdf stage=draw error=%d", error);
		fm_image_release(image);
		return EINVAL;
	}

	/* Succeeded: the first page. */
	return 0;
}

/*
 * Reads the kept thumbnail of a file, when the cache has one recorded for
 * the file as it is now (its modification time and size).  Returns 0, or
 * ENOENT when there is none (or a stale or damaged one).
 */
int
fm_thumb_cache_read(
	const char *path,
	struct fm_image *image)
{
	struct stat status;
	char record[FM_PATH_MAX];
	unsigned char *row;
	long long modified;
	unsigned long long size;
	FILE *file;
	size_t got;
	int width;
	int height;
	int error;
	int x;
	int y;

	/* The file as it is now, and its record's place. */
	memset(image, 0, sizeof(*image));
	error = stat(path, &status);
	if (error != 0)
		return ENOENT;
	error = cache_record_path(path, record, sizeof(record));
	if (error != 0)
		return ENOENT;

	/* The record's header, line by line: the PPM's kind, the mark with the time and size, the size, the depth. */
	file = fopen(record, "rb");
	if (file == NULL)
		return ENOENT;
	error = cache_header(file, &modified, &size, &width, &height);
	if (error != 0 ||
	    modified != (long long)status.st_mtime ||
	    size != (unsigned long long)status.st_size ||
	    width < 1 || width > CACHE_SIDE_MAX ||
	    height < 1 || height > CACHE_SIDE_MAX) {
		fclose(file);
		return ENOENT;
	}

	/* The pixels, a row at a time. */
	error = fm_image_create(image, width, height);
	if (error != 0) {
		fclose(file);
		return ENOENT;
	}
	row = malloc((size_t)width * 3U);
	if (row == NULL) {
		fm_image_release(image);
		fclose(file);
		return ENOENT;
	}
	for (y = 0; y < height; y++) {
		got = fread(row, 3U, (size_t)width, file);
		if (got != (size_t)width)
			break;
		for (x = 0; x < width; x++) {
			image->pixels[(size_t)y * (image->stride) + (size_t)x] =
			    0xff000000U | ((uint32_t)row[x * 3] << 16) | ((uint32_t)row[x * 3 + 1] << 8) | (uint32_t)row[x * 3 + 2];
		}
	}
	free(row);
	fclose(file);

	/* A short record is no thumbnail. */
	if (y != height) {
		fm_image_release(image);
		return ENOENT;
	}

	/* Succeeded: the kept thumbnail. */
	return 0;
}

/*
 * Keeps a thumbnail of a file in the cache, recorded with the file's
 * modification time and size.  Written to a temporary name and renamed, so
 * a reader never sees half of it.  Returns 0 or an errno value (a cache that
 * cannot be written only costs the next window the time to make it).
 */
int
fm_thumb_cache_write(
	const char *path,
	const struct fm_image *image)
{
	struct stat status;
	char record[FM_PATH_MAX];
	char temporary[FM_PATH_MAX + 16];
	unsigned char *row;
	uint32_t pixel;
	FILE *file;
	int error;
	int x;
	int y;

	/* The file as it is now, and the record's place (its folder made). */
	error = stat(path, &status);
	if (error != 0)
		return errno;
	error = cache_record_path(path, record, sizeof(record));
	if (error != 0)
		return error;
	snprintf(temporary, sizeof(temporary), "%s.%ld", record, (long)getpid());

	/* The header, then the pixels a row at a time. */
	file = fopen(temporary, "wb");
	if (file == NULL)
		return errno;
	fprintf(file, "P6\n%s mtime=%lld size=%llu\n%d %d\n255\n", CACHE_MARK, (long long)status.st_mtime, (unsigned long long)status.st_size, image->width, image->height);
	row = malloc((size_t)image->width * 3U);
	if (row == NULL) {
		fclose(file);
		unlink(temporary);
		return ENOMEM;
	}
	for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {
			pixel = image->pixels[(size_t)y * (image->stride) + (size_t)x];
			row[x * 3] = (unsigned char)(pixel >> 16);
			row[x * 3 + 1] = (unsigned char)(pixel >> 8);
			row[x * 3 + 2] = (unsigned char)pixel;
		}
		(void)fwrite(row, 3U, (size_t)image->width, file);
	}
	free(row);

	/* The record takes its name once it is whole. */
	error = ferror(file);
	if (fclose(file) != 0)
		error = 1;
	if (error != 0) {
		unlink(temporary);
		return EIO;
	}
	error = rename(temporary, record);
	if (error != 0) {
		error = errno;
		unlink(temporary);
		return error;
	}

	/* Succeeded: the thumbnail is kept. */
	return 0;
}

/* Opens libpdf and finds its calls the first time; returns 1 when they are there, 0 otherwise. */
static int
cache_pdf_load(
	void)
{
	/* Tried before: the same answer. */
	if (cache_pdf.loaded != 0)
		return cache_pdf.loaded > 0;

	/* The library; without it PDFs keep their icon. */
	cache_pdf.loaded = -1;
	cache_pdf.library = dlopen("libpdf.so", RTLD_NOW | RTLD_LOCAL);
	if (cache_pdf.library == NULL)
		return 0;

	/* Each call. */
	*(void **)&cache_pdf.open_memory = dlsym(cache_pdf.library, "pdf_document_open_memory");
	*(void **)&cache_pdf.close = dlsym(cache_pdf.library, "pdf_document_close");
	*(void **)&cache_pdf.page_count = dlsym(cache_pdf.library, "pdf_document_page_count");
	*(void **)&cache_pdf.page_box = dlsym(cache_pdf.library, "pdf_document_page_box");
	*(void **)&cache_pdf.render = dlsym(cache_pdf.library, "pdf_page_render");
	*(void **)&cache_pdf.list_destroy = dlsym(cache_pdf.library, "pdf_display_list_destroy");
	*(void **)&cache_pdf.rasterize = dlsym(cache_pdf.library, "pdf_display_list_rasterize");
	if (cache_pdf.open_memory == NULL ||
	    cache_pdf.close == NULL ||
	    cache_pdf.page_count == NULL ||
	    cache_pdf.page_box == NULL ||
	    cache_pdf.render == NULL ||
	    cache_pdf.list_destroy == NULL ||
	    cache_pdf.rasterize == NULL)
		return 0;

	/* Succeeded: libpdf is there. */
	cache_pdf.loaded = 1;
	return 1;
}

/* Writes the place of a file's record in the cache (the cache's folder made); returns 0 or an errno value. */
static int
cache_record_path(
	const char *path,
	char *record,
	size_t size)
{
	SHA2_CTX context;
	char folder[FM_PATH_MAX];
	uint8_t digest[SHA256_DIGEST_LENGTH];
	char hex[SHA256_DIGEST_LENGTH * 2 + 1];
	size_t index;
	int error;
	int written;

	/* The cache's folder, made when it is not there. */
	error = cache_folder(folder, sizeof(folder));
	if (error != 0)
		return error;

	/* The SHA-256 of the path, in hexadecimal. */
	SHA256Init(&context);
	SHA256Update(&context, (const uint8_t *)path, strlen(path));
	SHA256Final(digest, &context);
	for (index = 0; index < SHA256_DIGEST_LENGTH; index++)
		snprintf(hex + index * 2, 3, "%02x", digest[index]);

	/* The record's path. */
	written = snprintf(record, size, "%s/%s.ppm", folder, hex);
	if (written < 0 || (size_t)written >= size)
		return ENAMETOOLONG;

	/* Succeeded: the record's place. */
	return 0;
}

/* Writes the cache's folder, made (with its parents, the user's only) when it is not there; returns 0 or an errno value. */
static int
cache_folder(
	char *folder,
	size_t size)
{
	const char *base;
	const char *home;
	char *slash;
	int written;
	int error;

	/* $XDG_CACHE_HOME, else ~/.cache. */
	base = getenv("XDG_CACHE_HOME");
	home = getenv("HOME");
	if (base != NULL && base[0] == '/') {
		written = snprintf(folder, size, "%s/%s", base, CACHE_FOLDER);
	} else if (home != NULL && home[0] == '/') {
		written = snprintf(folder, size, "%s/.cache/%s", home, CACHE_FOLDER);
	} else {
		return ENOENT;
	}
	if (written < 0 || (size_t)written >= size)
		return ENAMETOOLONG;

	/* Each part of the path made in turn (one already there is fine). */
	for (slash = strchr(folder + 1, '/'); slash != NULL; slash = strchr(slash + 1, '/')) {
		*slash = '\0';
		error = mkdir(folder, 0700);
		*slash = '/';
		if (error != 0 && errno != EEXIST)
			return errno;
	}
	error = mkdir(folder, 0700);
	if (error != 0 && errno != EEXIST)
		return errno;

	/* Succeeded: the folder is there. */
	return 0;
}

/* Reads a record's header up to its pixels; returns 0, or EINVAL for one that is not a record. */
static int
cache_header(
	FILE *file,
	long long *modified,
	unsigned long long *size,
	int *width,
	int *height)
{
	char line[160];
	char *text;
	char *end;
	long value;
	int differs;

	/* The PPM's kind. */
	text = fgets(line, sizeof(line), file);
	if (text == NULL)
		return EINVAL;
	differs = strcmp(line, "P6\n");
	if (differs != 0)
		return EINVAL;

	/* The mark, then the file's modification time and size. */
	text = fgets(line, sizeof(line), file);
	if (text == NULL)
		return EINVAL;
	differs = strncmp(line, CACHE_MARK " mtime=", sizeof(CACHE_MARK " mtime=") - 1U);
	if (differs != 0)
		return EINVAL;
	text = line + sizeof(CACHE_MARK " mtime=") - 1U;
	*modified = strtoll(text, &end, 10);
	differs = strncmp(end, " size=", 6);
	if (end == text || differs != 0)
		return EINVAL;
	text = end + 6;
	*size = strtoull(text, &end, 10);
	if (end == text || *end != '\n')
		return EINVAL;

	/* The width and the height. */
	text = fgets(line, sizeof(line), file);
	if (text == NULL)
		return EINVAL;
	value = strtol(line, &end, 10);
	if (end == line || *end != ' ' || value < 1 || value > 65535)
		return EINVAL;
	*width = (int)value;
	text = end + 1;
	value = strtol(text, &end, 10);
	if (end == text || *end != '\n' || value < 1 || value > 65535)
		return EINVAL;
	*height = (int)value;

	/* The depth, which is always 255. */
	text = fgets(line, sizeof(line), file);
	if (text == NULL)
		return EINVAL;
	differs = strcmp(line, "255\n");
	if (differs != 0)
		return EINVAL;

	/* Succeeded: the pixels follow. */
	return 0;
}
