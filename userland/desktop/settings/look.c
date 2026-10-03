/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The desktop's look as Settings keeps it (ws089-p004): the user's
 * preferences (libkeiland's, ~/.config/keiland/desktop.conf, which zdesktop
 * follows within a second, ws089-p007), the pictures the Wallpaper page
 * offers with their small copies, and the file systems the Storage page
 * shows.
 *
 * Settings writes a key when the user has chosen: a picture clicked, a
 * slider let go.  The file is read again once a second, so that a change
 * made elsewhere shows too.
 *
 * The pictures' small copies are read by a thread of their own (BUG-152):
 * the Wallpaper page is shown at once with a stand-in in each tile, and
 * each tile fills as its copy is ready.
 */

#include "settings.h"

#include "userland/desktop/paths.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

/* How often the preferences are read again, in milliseconds. */
#define LOOK_CHECK_MS		1000U

/* How often the window's thread looks for finished small copies while they are being read, in milliseconds. */
#define LOOK_LOAD_POLL_MS	40

/* The windows' opacity: its range and its default, in percent. */
#define LOOK_OPACITY_MIN	85
#define LOOK_OPACITY_MAX	100

/* The input's keys: their ranges and defaults, as zdesktop takes them (userland/desktop/wayland/preferences.c). */
#define LOOK_SPEED_MIN		25
#define LOOK_SPEED_MAX		300
#define LOOK_SPEED_DEFAULT	100
#define LOOK_RATE_MIN		5
#define LOOK_RATE_MAX		60
#define LOOK_RATE_DEFAULT	25
#define LOOK_DELAY_MIN		150
#define LOOK_DELAY_MAX		1000
#define LOOK_DELAY_DEFAULT	400

/* The pictures: the default (the session's --wallpaper) and the folder of the others. */
#define LOOK_DEFAULT_PICTURE	KEILAND_DATADIR "/keiland/wallpaper.ppm"
#define LOOK_PICTURES		KEILAND_DATADIR "/keiland/wallpapers"

/* A picture's small copy, in pixels. */
#define LOOK_THUMB_WIDTH	240
#define LOOK_THUMB_HEIGHT	150

/* The bytes of a PPM's header read at most, and the widest picture read. */
#define LOOK_HEADER_MAX		64U
#define LOOK_WIDTH_MAX		8192U

/* The places the Storage page looks at (a place on the same file system as one before it is not shown again). */
static const char *const look_places[] = { "/", "/home", "/usr", "/var", "/tmp", "/boot" };

static void look_read(struct se_app *app);
static int look_write(struct se_app *app, const char *key, const char *value);
static void look_add_picture(struct se_app *app, const char *path, const char *name);
static void look_load_start(struct se_app *app);
static void *look_load_run(void *argument);
static void look_load_take(struct se_app *app);
static void look_load_stop(struct se_app *app);
static int look_thumbnail(const char *path, struct fm_image *image);
static int look_ppm_number(const unsigned char *data, size_t size, size_t *at, unsigned *number);
static int look_compare_names(const void *left, const void *right);

/*
 * Opens the user's preferences and reads what the look's pages show.  Without
 * a home nothing can be saved, and the pages say so.
 */
void
se_look_open(
	struct se_app *app)
{
	struct se_look *look;

	/* Nothing read yet. */
	look = &app->look;
	look->opacity = LOOK_OPACITY_MAX;
	look->pointer_speed = LOOK_SPEED_DEFAULT;
	look->pointer_natural = 0;
	look->repeat_rate = LOOK_RATE_DEFAULT;
	look->repeat_delay = LOOK_DELAY_DEFAULT;
	look->wallpaper[0] = '\0';

	/* The file; without a home the pages still show the defaults. */
	look->preferences = keiland_preferences_open();
	if (look->preferences == NULL) {
		look->open_error = errno;
		se_log("LOOK none errno=%d", look->open_error);
		return;
	}

	/* What it holds. */
	look_read(app);
	look->checked_at = app->now;
	se_log("LOOK open opacity=%d wallpaper=%s", look->opacity, look->wallpaper);
}

/*
 * Reads the preferences again once a second; a change made elsewhere is
 * shown.
 */
void
se_look_poll(
	struct se_app *app,
	uint64_t now)
{
	struct se_look *look;
	int changed;
	int error;

	/* The pictures' small copies finished since the last round go to their tiles. */
	look_load_take(app);

	/* Nothing to read, or not yet. */
	look = &app->look;
	if (look->preferences == NULL)
		return;
	if (now - look->checked_at < LOOK_CHECK_MS)
		return;
	look->checked_at = now;

	/* The file, when it moved; a drag in progress keeps its own value. */
	error = keiland_preferences_reload(look->preferences, &changed);
	if (error != 0 || changed == 0)
		return;
	if (look->dragging != 0)
		return;

	/* The new values, drawn. */
	look_read(app);
	app->dirty = 1;
	se_log("LOOK changed opacity=%d wallpaper=%s", look->opacity, look->wallpaper);
}

/*
 * Reports how long the main loop may sleep for the look: a short while
 * when the pictures' small copies are being read, so that each tile fills
 * soon after its copy is ready, and -1 (no limit) otherwise.
 */
int
se_look_wait(
	const struct se_app *app)
{
	/* No thread is reading pictures: the look has nothing due. */
	if (app->look.loader.running == 0)
		return -1;

	/* Succeeded: the next look for finished copies. */
	return LOOK_LOAD_POLL_MS;
}

/*
 * Closes the preferences and frees the pictures' small copies.
 */
void
se_look_close(
	struct se_app *app)
{
	struct se_look *look;
	unsigned index;

	/* A thread still reading pictures is stopped and joined first. */
	look_load_stop(app);

	/* The small copies. */
	look = &app->look;
	for (index = 0; index < look->wallpaper_count; index++)
		fm_image_release(&look->wallpapers[index].thumbnail);
	look->wallpaper_count = 0;

	/* The file. */
	if (look->preferences != NULL)
		keiland_preferences_close(look->preferences);
	look->preferences = NULL;
}

/*
 * Saves the windows' opacity (85 to 100 percent), which zdesktop takes
 * within a second; 100 removes the key (the desktop's own).
 */
void
se_look_set_opacity(
	struct se_app *app,
	int percent)
{
	char value[16];
	int error;

	/* Within the range. */
	if (percent < LOOK_OPACITY_MIN)
		percent = LOOK_OPACITY_MIN;
	if (percent > LOOK_OPACITY_MAX)
		percent = LOOK_OPACITY_MAX;
	app->look.opacity = percent;

	/* The key, or none for fully opaque. */
	if (percent == LOOK_OPACITY_MAX) {
		error = look_write(app, "window.opacity", NULL);
	} else {
		(void)snprintf(value, sizeof(value), "%d", percent);
		error = look_write(app, "window.opacity", value);
	}

	/* The log line the tests read. */
	se_log("LOOK set key=window.opacity value=%d error=%d", percent, error);
}

/*
 * Saves a whole number under a key, which zdesktop takes within a second;
 * the default value removes the key (the desktop's own).
 */
void
se_look_set_number(
	struct se_app *app,
	const char *key,
	int value,
	int fallback)
{
	char text[16];
	int error;

	/* The key, or none for the default. */
	if (value == fallback) {
		error = look_write(app, key, NULL);
	} else {
		(void)snprintf(text, sizeof(text), "%d", value);
		error = look_write(app, key, text);
	}

	/* The log line the tests read. */
	se_log("LOOK set key=%s value=%d error=%d", key, value, error);
}

/*
 * Chooses a picture of the Wallpaper page by its index (the default's, or
 * index -1 when there is no default picture, removes the key).
 */
void
se_look_set_wallpaper(
	struct se_app *app,
	int index)
{
	struct se_look *look;
	const char *path;
	int error;

	/* The default removes the key; any other picture is written. */
	look = &app->look;
	path = NULL;
	if (index >= 0 && (unsigned)index < look->wallpaper_count) {
		if (index != 0 || look->has_default == 0)
			path = look->wallpapers[index].path;
	}

	/* The key, and what the page shows chosen. */
	error = look_write(app, "wallpaper", path);
	if (error == 0) {
		look->wallpaper[0] = '\0';
		if (path != NULL)
			(void)snprintf(look->wallpaper, sizeof(look->wallpaper), "%s", path);
	}

	/* The log line the tests read. */
	se_log("LOOK set key=wallpaper value=%s error=%d", look->wallpaper, error);
}

/*
 * Finds the pictures the Wallpaper page offers (once, when the page is
 * first shown): the default first, then the folder's, by name.  Their
 * small copies are read by a thread of their own and fill the tiles as
 * they are ready (BUG-152).
 */
void
se_look_scan(
	struct se_app *app)
{
	struct se_look *look;
	struct dirent *entry;
	char names[SE_WALLPAPERS][64];
	char path[SE_PATH];
	DIR *folder;
	unsigned count;
	unsigned index;
	size_t length;
	int differs;

	/* Once. */
	look = &app->look;
	if (look->scanned != 0)
		return;
	look->scanned = 1;

	/* The default picture, when the image has one (tile 0; else tile 0 is the drawn landscape). */
	look_add_picture(app, LOOK_DEFAULT_PICTURE, "Kei");
	look->has_default = 1;

	/* The folder's pictures (PPM files), at most as many as fit after the default. */
	count = 0;
	folder = opendir(LOOK_PICTURES);
	while (folder != NULL && count < SE_WALLPAPERS - 1U) {
		entry = readdir(folder);
		if (entry == NULL)
			break;

		/* Only a name ending in .ppm, short enough. */
		length = strlen(entry->d_name);
		if (length <= 4U || length >= sizeof(names[0]))
			continue;
		differs = strcmp(entry->d_name + length - 4U, ".ppm");
		if (differs != 0)
			continue;

		/* The name is kept for sorting. */
		(void)snprintf(names[count], sizeof(names[count]), "%s", entry->d_name);
		count++;
	}

	/* The folder is not needed any more. */
	if (folder != NULL)
		(void)closedir(folder);

	/* By name, each with its small copy. */
	qsort(names, count, sizeof(names[0]), look_compare_names);
	for (index = 0; index < count; index++) {
		(void)snprintf(path, sizeof(path), "%s/%.63s", LOOK_PICTURES, names[index]);
		names[index][strlen(names[index]) - 4U] = '\0';
		look_add_picture(app, path, names[index]);
	}

	/* The log line the tests read. */
	se_log("LOOK pictures count=%u", look->wallpaper_count);

	/* The small copies, read while the page is already shown. */
	look_load_start(app);
}

/*
 * Reads the file systems the Storage page shows: each place's, once for a
 * file system.
 */
void
se_look_volumes(
	struct se_app *app)
{
	struct se_look *look;
	struct se_volume *volume;
	struct statvfs status;
	uint64_t seen[SE_VOLUMES];
	unsigned place;
	unsigned index;
	int result;
	int known;

	/* Each place, until the table is full. */
	look = &app->look;
	look->volume_count = 0;
	for (place = 0; place < sizeof(look_places) / sizeof(look_places[0]); place++) {
		if (look->volume_count == SE_VOLUMES)
			break;

		/* A place that is not there, or has no size, is passed over. */
		result = statvfs(look_places[place], &status);
		if (result != 0 || status.f_blocks == 0U)
			continue;

		/* A file system already shown is not shown again. */
		known = 0;
		for (index = 0; index < look->volume_count; index++) {
			if (seen[index] == (uint64_t)status.f_fsid)
				known = 1;
		}

		/* One already shown is passed over. */
		if (known != 0)
			continue;

		/* Its sizes. */
		seen[look->volume_count] = (uint64_t)status.f_fsid;
		volume = &look->volumes[look->volume_count];
		(void)snprintf(volume->path, sizeof(volume->path), "%s", look_places[place]);
		volume->total = (uint64_t)status.f_blocks * (uint64_t)status.f_frsize;
		volume->available = (uint64_t)status.f_bavail * (uint64_t)status.f_frsize;
		volume->used = volume->total - (uint64_t)status.f_bfree * (uint64_t)status.f_frsize;
		look->volume_count++;
	}
}

/*
 * Names the picture chosen for Home's tile: the default's name, or the
 * chosen file's name.
 */
const char *
se_look_wallpaper_name(
	const struct se_app *app)
{
	const char *slash;
	unsigned index;
	int differs;

	/* No key: the default. */
	if (app->look.wallpaper[0] == '\0')
		return "Kei (default)";

	/* A picture the page found has its name. */
	for (index = 0; index < app->look.wallpaper_count; index++) {
		differs = strcmp(app->look.wallpapers[index].path, app->look.wallpaper);
		if (differs == 0)
			return app->look.wallpapers[index].name;
	}

	/* Else the file's name. */
	slash = strrchr(app->look.wallpaper, '/');
	if (slash == NULL)
		return app->look.wallpaper;

	/* Succeeded: the part after the last slash. */
	return slash + 1;
}

/* Reads the look's keys from the preferences. */
static void
look_read(
	struct se_app *app)
{
	struct se_look *look;
	int error;

	/* The opacity, 100 when unset. */
	look = &app->look;
	look->opacity = keiland_preferences_get_int(look->preferences, "window.opacity", LOOK_OPACITY_MAX, LOOK_OPACITY_MIN, LOOK_OPACITY_MAX);

	/* The pointer and the keyboards. */
	look->pointer_speed = keiland_preferences_get_int(look->preferences, "pointer.speed", LOOK_SPEED_DEFAULT, LOOK_SPEED_MIN, LOOK_SPEED_MAX);
	look->pointer_natural = keiland_preferences_get_int(look->preferences, "pointer.natural", 0, 0, 1);
	look->repeat_rate = keiland_preferences_get_int(look->preferences, "keyboard.repeat.rate", LOOK_RATE_DEFAULT, LOOK_RATE_MIN, LOOK_RATE_MAX);
	look->repeat_delay = keiland_preferences_get_int(look->preferences, "keyboard.repeat.delay", LOOK_DELAY_DEFAULT, LOOK_DELAY_MIN, LOOK_DELAY_MAX);

	/* The picture; none, or one that is not an absolute path, is the default. */
	error = keiland_preferences_get(look->preferences, "wallpaper", look->wallpaper, sizeof(look->wallpaper));
	if (error != 0 || look->wallpaper[0] != '/')
		look->wallpaper[0] = '\0';
}

/* Writes a key (or removes it when value is NULL); a failure is shown on the page. Returns 0 or an errno value. */
static int
look_write(
	struct se_app *app,
	const char *key,
	const char *value)
{
	struct se_look *look;
	int error;

	/* Without a home nothing is saved. */
	look = &app->look;
	if (look->preferences == NULL) {
		(void)snprintf(look->message, sizeof(look->message), "%s", "Settings cannot be saved: this account has no home folder.");
		look->message_bad = 1;
		return ENOENT;
	}

	/* The key, or its removal. */
	if (value != NULL) {
		error = keiland_preferences_set(look->preferences, key, value);
	} else {
		error = keiland_preferences_unset(look->preferences, key);
	}

	/* A failure is shown; a success clears the last message. */
	look->message[0] = '\0';
	look->message_bad = 0;
	if (error != 0) {
		(void)snprintf(look->message, sizeof(look->message), "The setting could not be saved (error %d).", error);
		look->message_bad = 1;
		return error;
	}

	/* Succeeded: zdesktop takes it within a second. */
	app->dirty = 1;
	return 0;
}

/* Adds a picture to the page's list, its small copy still to be read (the tile shows a stand-in until then). */
static void
look_add_picture(
	struct se_app *app,
	const char *path,
	const char *name)
{
	struct se_wallpaper *wallpaper;

	/* A full list keeps the pictures it has. */
	if (app->look.wallpaper_count == SE_WALLPAPERS)
		return;

	/* The picture's names. */
	wallpaper = &app->look.wallpapers[app->look.wallpaper_count];
	memset(wallpaper, 0, sizeof(*wallpaper));
	(void)snprintf(wallpaper->path, sizeof(wallpaper->path), "%s", path);
	(void)snprintf(wallpaper->name, sizeof(wallpaper->name), "%s", name);

	/* The loader fills its small copy later; until then the tile waits for it. */
	wallpaper->pending = 1;
	app->look.wallpaper_count++;
}

/*
 * Starts the thread that reads the listed pictures' small copies.  When
 * no thread can be started, the pictures are listed without small copies
 * (their tiles show the drawn stand-in) rather than holding the page.
 */
static void
look_load_start(
	struct se_app *app)
{
	struct se_look_loader *loader;
	unsigned index;
	int error;

	/* The paths the thread reads, its own copies, so that it shares nothing else with the page. */
	loader = &app->look.loader;
	memset(loader, 0, sizeof(*loader));
	loader->count = app->look.wallpaper_count;
	for (index = 0; index < loader->count; index++)
		(void)snprintf(loader->paths[index], sizeof(loader->paths[index]), "%s", app->look.wallpapers[index].path);

	/* The lock the thread hands each finished copy over under. */
	error = pthread_mutex_init(&loader->lock, NULL);
	if (error == 0) {
		/* The thread; running says it must be joined. */
		loader->running = 1;
		error = pthread_create(&loader->thread, NULL, look_load_run, loader);
		if (error != 0) {
			loader->running = 0;
			(void)pthread_mutex_destroy(&loader->lock);
		}
	}

	/* Without a thread, no picture waits for a small copy any more. */
	if (error != 0) {
		for (index = 0; index < app->look.wallpaper_count; index++)
			app->look.wallpapers[index].pending = 0;
		app->dirty = 1;
		se_log("LOOK loader error=%d", error);
		return;
	}

	/* The log line the tests read. */
	se_log("LOOK loader started count=%u", loader->count);
}

/*
 * The loader's thread: reads each picture's small copy in the page's
 * order (so the tiles fill from the first) and hands it over under the
 * lock.  It ends early when the window closes.
 */
static void *
look_load_run(
	void *argument)
{
	struct se_look_loader *loader;
	struct fm_image image;
	struct timespec started;
	struct timespec finished;
	unsigned index;
	long milliseconds;
	int stopping;
	int error;

	/* The loader the window's thread started this thread with. */
	loader = argument;

	/* Each picture in turn, until every one is read or the window closes. */
	for (index = 0; index < loader->count; index++) {
		/* A closing window asks the thread to end between pictures. */
		(void)pthread_mutex_lock(&loader->lock);

		stopping = loader->stopping;

		(void)pthread_mutex_unlock(&loader->lock);

		/* The window is closing: the rest are not read. */
		if (stopping != 0)
			break;

		/* The small copy, timed for the log (a slow disk shows as a late tile). */
		memset(&image, 0, sizeof(image));
		(void)clock_gettime(CLOCK_MONOTONIC, &started);
		error = look_thumbnail(loader->paths[index], &image);
		(void)clock_gettime(CLOCK_MONOTONIC, &finished);
		milliseconds = (long)(finished.tv_sec - started.tv_sec) * 1000L + (finished.tv_nsec - started.tv_nsec) / 1000000L;

		/*
		 * The copy (empty when the picture could not be read) is handed
		 * over; done tells the window's thread it may take it.
		 */
		(void)pthread_mutex_lock(&loader->lock);

		loader->images[index] = image;
		loader->errors[index] = error;
		loader->milliseconds[index] = milliseconds;
		loader->done[index] = 1;

		(void)pthread_mutex_unlock(&loader->lock);
	}

	/* Succeeded: the thread ends and waits to be joined. */
	return NULL;
}

/*
 * Moves the small copies the loader has finished to their tiles and asks
 * for a frame; once every copy is taken, the thread is joined.
 */
static void
look_load_take(
	struct se_app *app)
{
	struct se_look_loader *loader;
	struct se_wallpaper *wallpaper;
	unsigned index;
	unsigned taken;

	/* No thread is reading pictures. */
	loader = &app->look.loader;
	if (loader->running == 0)
		return;

	/* Each copy finished and not yet taken goes to its tile. */
	taken = 0;
	(void)pthread_mutex_lock(&loader->lock);

	for (index = 0; index < loader->count; index++) {
		/* A copy the thread has not finished, or one already taken, stays. */
		if (loader->done[index] == 0)
			continue;
		if (loader->taken[index] != 0)
			continue;

		/* The tile owns the copy now; the loader's slot is emptied. */
		wallpaper = &app->look.wallpapers[index];
		wallpaper->thumbnail = loader->images[index];
		memset(&loader->images[index], 0, sizeof(loader->images[index]));
		wallpaper->read = 0;
		if (loader->errors[index] == 0)
			wallpaper->read = 1;

		/* The tile stops waiting, and the count says when the thread may be joined. */
		wallpaper->pending = 0;
		loader->taken[index] = 1;
		loader->taken_count++;
		taken++;
		se_log("LOOK picture path=%s error=%d ms=%ld", wallpaper->path, loader->errors[index], loader->milliseconds[index]);
	}

	(void)pthread_mutex_unlock(&loader->lock);

	/* A tile that filled is drawn. */
	if (taken != 0)
		app->dirty = 1;

	/* Some copies are still being read. */
	if (loader->taken_count < loader->count)
		return;

	/* Every copy is taken: the thread has ended, or is about to, and is joined. */
	(void)pthread_join(loader->thread, NULL);
	(void)pthread_mutex_destroy(&loader->lock);
	loader->running = 0;

	/* The log line the tests read. */
	se_log("LOOK pictures ready count=%u", loader->count);
}

/*
 * Stops a thread still reading pictures (the window is closing): asks it
 * to end between pictures, joins it, and frees the copies it finished
 * that no tile took.
 */
static void
look_load_stop(
	struct se_app *app)
{
	struct se_look_loader *loader;
	unsigned index;

	/* No thread is reading pictures. */
	loader = &app->look.loader;
	if (loader->running == 0)
		return;

	/* stopping tells the thread to read no further picture. */
	(void)pthread_mutex_lock(&loader->lock);

	loader->stopping = 1;

	(void)pthread_mutex_unlock(&loader->lock);

	/* The thread ends after the picture it is reading. */
	(void)pthread_join(loader->thread, NULL);
	(void)pthread_mutex_destroy(&loader->lock);
	loader->running = 0;

	/* The copies no tile took are freed. */
	for (index = 0; index < loader->count; index++) {
		if (loader->done[index] != 0 && loader->taken[index] == 0)
			fm_image_release(&loader->images[index]);
	}
}

/*
 * Reads a binary PPM into a small copy (LOOK_THUMB_WIDTH by
 * LOOK_THUMB_HEIGHT, the middle of the picture in the tile's proportions,
 * each small pixel the average of 3 by 3 samples).  Only the rows the
 * samples fall on are read, one at a time into a buffer a row long: a
 * large picture is not held whole.  Returns 0 or an errno value.
 */
static int
look_thumbnail(
	const char *path,
	struct fm_image *image)
{
	unsigned char header[LOOK_HEADER_MAX];
	unsigned char *row;
	const unsigned char *pixel;
	struct stat status;
	ssize_t count;
	size_t at;
	size_t row_bytes;
	off_t offset;
	unsigned width;
	unsigned height;
	unsigned maximum;
	unsigned x;
	unsigned y;
	unsigned source_x;
	unsigned source_y;
	unsigned sums[LOOK_THUMB_WIDTH][3];
	unsigned dx;
	unsigned dy;
	unsigned channel;
	unsigned crop_width;
	unsigned crop_height;
	unsigned crop_left;
	unsigned crop_top;
	int descriptor;
	int result;
	int error;

	/* The file, and its header's bytes. */
	descriptor = open(path, O_RDONLY);
	if (descriptor < 0)
		return errno;
	count = read(descriptor, header, sizeof(header));
	if (count < 0) {
		error = errno;
		(void)close(descriptor);
		return error;
	}

	/* The header: P6, the width, the height and the largest value (255), one space before the pixels. */
	at = 2;
	width = 0;
	height = 0;
	maximum = 0;
	error = EINVAL;
	if (count > 2 &&
	    header[0] == 'P' &&
	    header[1] == '6')
		error = 0;
	if (error == 0)
		error = look_ppm_number(header, (size_t)count, &at, &width);
	if (error == 0)
		error = look_ppm_number(header, (size_t)count, &at, &height);
	if (error == 0)
		error = look_ppm_number(header, (size_t)count, &at, &maximum);
	if (error == 0 &&
	    (maximum != 255U ||
	     width == 0U ||
	     height == 0U ||
	     width > LOOK_WIDTH_MAX))
		error = EINVAL;
	at++;

	/* The file must hold every row the header promises. */
	row_bytes = (size_t)width * 3U;
	result = fstat(descriptor, &status);
	if (error == 0 && result != 0)
		error = errno;
	if (error == 0 &&
	    (uint64_t)status.st_size < (uint64_t)at + (uint64_t)row_bytes * height)
		error = EINVAL;
	if (error != 0) {
		(void)close(descriptor);
		return error;
	}

	/*
	 * The middle of the picture in the tile's proportions (16:10): a wider
	 * picture loses its sides, a taller one its top and bottom.
	 */
	crop_width = width;
	crop_height = height;
	if ((uint64_t)width * LOOK_THUMB_HEIGHT > (uint64_t)height * LOOK_THUMB_WIDTH) {
		crop_width = (unsigned)((uint64_t)height * LOOK_THUMB_WIDTH / LOOK_THUMB_HEIGHT);
	} else {
		crop_height = (unsigned)((uint64_t)width * LOOK_THUMB_HEIGHT / LOOK_THUMB_WIDTH);
	}

	/* The crop's corner, in the middle. */
	crop_left = (width - crop_width) / 2U;
	crop_top = (height - crop_height) / 2U;

	/* A buffer one row long. */
	row = malloc(row_bytes);
	if (row == NULL) {
		(void)close(descriptor);
		return ENOMEM;
	}

	/* The small image. */
	error = fm_image_create(image, LOOK_THUMB_WIDTH, LOOK_THUMB_HEIGHT);
	if (error != 0) {
		free(row);
		(void)close(descriptor);
		return error;
	}

	/* Each small row averages the samples of three source rows, each row read once. */
	for (y = 0; error == 0 && y < (unsigned)LOOK_THUMB_HEIGHT; y++) {
		memset(sums, 0, sizeof(sums));
		for (dy = 0; dy < 3U; dy++) {
			/* The source row this sample row falls on. */
			source_y = crop_top + (unsigned)(((uint64_t)y * 3U + dy) * crop_height / (LOOK_THUMB_HEIGHT * 3U));
			offset = (off_t)at + (off_t)source_y * (off_t)row_bytes;
			count = pread(descriptor, row, row_bytes, offset);
			if (count != (ssize_t)row_bytes) {
				error = EIO;
				break;
			}

			/* Three samples across for each small pixel. */
			for (x = 0; x < (unsigned)LOOK_THUMB_WIDTH; x++) {
				for (dx = 0; dx < 3U; dx++) {
					source_x = crop_left + (unsigned)(((uint64_t)x * 3U + dx) * crop_width / (LOOK_THUMB_WIDTH * 3U));
					pixel = row + (size_t)source_x * 3U;
					for (channel = 0; channel < 3U; channel++)
						sums[x][channel] += pixel[channel];
				}
			}
		}

		/* The row's pixels, opaque, in the canvas's order (0xAARRGGBB). */
		for (x = 0; error == 0 && x < (unsigned)LOOK_THUMB_WIDTH; x++) {
			image->pixels[(size_t)y * image->stride + x] = 0xff000000U |
			    ((sums[x][0] / 9U) << 16) |
			    ((sums[x][1] / 9U) << 8) |
			    (sums[x][2] / 9U);
		}
	}

	/* The buffer and the file are not needed any more. */
	free(row);
	(void)close(descriptor);

	/* A row that could not be read spoils the copy. */
	if (error != 0) {
		fm_image_release(image);
		return error;
	}

	/* Succeeded: the small copy is made. */
	return 0;
}

/* Reads a PPM header's number after white space and comments; returns 0 or EINVAL. */
static int
look_ppm_number(
	const unsigned char *data,
	size_t size,
	size_t *at,
	unsigned *number)
{
	unsigned digits;

	/* White space and comments first. */
	while (*at < size) {
		if (data[*at] == '#') {
			while (*at < size && data[*at] != '\n')
				(*at)++;
		} else if (data[*at] == ' ' ||
			   data[*at] == '\t' ||
			   data[*at] == '\n' ||
			   data[*at] == '\r') {
			(*at)++;
		} else {
			break;
		}
	}

	/* The digits, at most seven. */
	*number = 0;
	digits = 0;
	while (*at < size &&
	       data[*at] >= '0' &&
	       data[*at] <= '9' &&
	       digits < 7U) {
		*number = *number * 10U + (unsigned)(data[*at] - '0');
		(*at)++;
		digits++;
	}

	/* No digit is no number. */
	if (digits == 0U)
		return EINVAL;

	/* Succeeded: the number is read. */
	return 0;
}

/* Orders two names as strcmp does (for qsort). */
static int
look_compare_names(
	const void *left,
	const void *right)
{
	int order;

	/* The names' order. */
	order = strcmp((const char *)left, (const char *)right);

	/* Succeeded: the order. */
	return order;
}
