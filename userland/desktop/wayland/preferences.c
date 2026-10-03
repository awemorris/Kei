/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The user's preferences on the desktop (ws089-p007, plan/ws089/proposed/
 * desktop-preferences.md): the wallpaper, the windows' opacity, the
 * pointer's speed and the wheel's direction, and the keyboards' repeat,
 * as Settings writes them in ~/.config/keiland/desktop.conf through
 * libkeiland.
 *
 * A desktop that is not the login screen reads them before its look draws
 * the wallpaper, then looks at the file once a second and applies the keys
 * that changed.  The command line's --wallpaper and --window-opacity stay
 * the defaults a removed key returns to.  Each key applied is logged
 * ("ZWL PREFERENCES key=... applied"), which the tests read.
 *
 * The looking is done by a thread of its own (the watcher).  A stat of the
 * file can wait on the disk for seconds, and in the event loop that wait
 * held every frame and every input event: a menu opened seconds after its
 * click (ws099-p020, BUG-125).  The watcher reads the file into a copy of
 * its own; the event loop takes that copy when it moved and applies it.
 * Without the thread the event loop looks at the file itself, as before.
 */

#include "zwl.h"

#include <keiland.h>

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* How often the file is looked at, in milliseconds. */
#define PREFERENCES_CHECK_MS		1000U

/* How long the watcher sleeps at a time between its looks, so that the end of the run is seen soon (ms). */
#define PREFERENCES_NAP_MS		100U

/* The ranges of the numeric keys, and their defaults. */
#define PREFERENCES_OPACITY_MIN		85
#define PREFERENCES_OPACITY_MAX		100
#define PREFERENCES_SPEED_MIN		25
#define PREFERENCES_SPEED_MAX		300
#define PREFERENCES_SPEED_DEFAULT	100
#define PREFERENCES_RATE_MIN		5
#define PREFERENCES_RATE_MAX		60
#define PREFERENCES_RATE_DEFAULT	25
#define PREFERENCES_DELAY_MIN		150
#define PREFERENCES_DELAY_MAX		1000
#define PREFERENCES_DELAY_DEFAULT	400

/*
 * The watcher of the preferences file and what it hands the event loop.
 *
 * watched is the watcher's own reading.  When a reload finds the file
 * moved, the watcher sets pending and leaves watched alone; the event loop
 * then swaps it with the reading it uses (the watcher reads the old one
 * again a second later) and clears pending.  error is the last reload's
 * failure for the event loop to log, stop asks the watcher to end.  lock
 * guards pending, error, stop and the swap of watched; the watcher holds
 * it only for those words, never while it waits on the disk.  running is
 * nonzero from a started thread to its join at close.
 */
struct preferences_watcher {
	pthread_t thread;
	pthread_mutex_t lock;
	struct keiland_preferences *watched;
	int pending;
	int error;
	int stop;
	int running;
};

/*
 * The one watcher of the process (one desktop runs in it).  It is started by
 * zwl_preferences_open and joined by zwl_preferences_close; zero is a
 * watcher that is not running.
 */
static struct preferences_watcher preferences_watcher;

static int preferences_watch_start(void);
static void *preferences_watch_run(void *argument);
static int preferences_watch_wait(void);
static void preferences_apply(struct zwl_server *server, int starting);
static void preferences_wallpaper(struct zwl_server *server, int starting);
static void preferences_opacity(struct zwl_server *server);
static void preferences_number(const char *key, int32_t *target, int32_t value);

/*
 * Opens the user's preferences and applies them before the look is made.
 * Without a home the desktop keeps its command line's settings.
 */
void
zwl_preferences_open(
	struct zwl_server *server)
{
	int error;

	/* What the command line gave is what a removed key returns to. */
	server->window_opacity_started = server->window_opacity;
	server->wallpaper_started = server->wallpaper_path;
	server->wallpaper_chosen[0] = '\0';

	/* The file; without a home there are no preferences. */
	server->preferences = keiland_preferences_open();
	if (server->preferences == NULL) {
		printf("ZWL PREFERENCES none errno=%d\n", errno);
		return;
	}

	/* Every key, before anything is drawn. */
	server->preferences_checked_ms = zwl_milliseconds();
	preferences_apply(server, 1);
	printf("ZWL PREFERENCES open\n");

	/* From now on the file is looked at away from the event loop; without the thread the loop looks itself. */
	error = preferences_watch_start();
	if (error != 0)
		printf("ZWL PREFERENCES watcher-unavailable errno=%d\n", error);
}

/*
 * Looks at the preferences once a second and applies the keys that
 * changed.
 */
void
zwl_preferences_tick(
	struct zwl_server *server,
	uint64_t now)
{
	struct keiland_preferences *taken;
	int changed;
	int error;

	/* Nothing to look at. */
	if (server->preferences == NULL)
		return;

	/* With the watcher, only what it found is taken: the event loop does not wait on the disk. */
	if (preferences_watcher.running) {
		/* Takes what the watcher left: a failure to log, and a new reading swapped for the one in use. */
		taken = NULL;
		(void)pthread_mutex_lock(&preferences_watcher.lock);

		/* The watcher's last failure is taken once. */
		error = preferences_watcher.error;
		preferences_watcher.error = 0;

		/* A pending reading becomes the desktop's; the watcher reads the old one again from now. */
		if (preferences_watcher.pending != 0) {
			taken = preferences_watcher.watched;
			preferences_watcher.watched = server->preferences;
			preferences_watcher.pending = 0;
		}

		/* The watcher may look at the file again. */
		(void)pthread_mutex_unlock(&preferences_watcher.lock);

		/* A reload the watcher could not do is told as before. */
		if (error != 0)
			printf("ZWL PREFERENCES reload-failed errno=%d\n", error);

		/* The file moved: its keys that changed apply. */
		if (taken != NULL) {
			server->preferences = taken;
			preferences_apply(server, 0);
		}

		/* The watcher does the looking; nothing more is done in the event loop. */
		return;
	}

	/* Without the watcher the file is looked at here, once a second. */
	if (now - server->preferences_checked_ms < PREFERENCES_CHECK_MS)
		return;
	server->preferences_checked_ms = now;

	/* The file, when it moved. */
	error = keiland_preferences_reload(server->preferences, &changed);
	if (error != 0) {
		printf("ZWL PREFERENCES reload-failed errno=%d\n", error);
		return;
	}

	/* The keys that changed. */
	if (changed != 0)
		preferences_apply(server, 0);
}

/*
 * Closes the preferences (at the end of the run).
 */
void
zwl_preferences_close(
	struct zwl_server *server)
{
	/* Nothing was opened. */
	if (server->preferences == NULL)
		return;

	/* The watcher ends (after the look it may be waiting on) and its reading goes. */
	if (preferences_watcher.running) {
		(void)pthread_mutex_lock(&preferences_watcher.lock);

		/* stop tells the watcher, at its next nap, to end. */
		preferences_watcher.stop = 1;

		/* The watcher sees the request once the lock is free. */
		(void)pthread_mutex_unlock(&preferences_watcher.lock);

		/* Waits for the thread to end; from then on nothing else uses the watcher. */
		(void)pthread_join(preferences_watcher.thread, NULL);
		preferences_watcher.running = 0;

		/* The watcher's reading, when it made one, is freed. */
		if (preferences_watcher.watched != NULL)
			keiland_preferences_close(preferences_watcher.watched);
		preferences_watcher.watched = NULL;

		/* The lock is no one's any more. */
		(void)pthread_mutex_destroy(&preferences_watcher.lock);
	}

	/* The file is not looked at any more. */
	keiland_preferences_close(server->preferences);
	server->preferences = NULL;
}

/* Starts the watcher; returns 0, or an errno value when it cannot run. */
static int
preferences_watch_start(
	void)
{
	int error;

	/* The lock the watcher and the event loop share. */
	memset(&preferences_watcher, 0, sizeof(preferences_watcher));
	error = pthread_mutex_init(&preferences_watcher.lock, NULL);
	if (error != 0)
		return error;

	/* The thread, which makes its own reading of the file. */
	error = pthread_create(&preferences_watcher.thread, NULL, preferences_watch_run, NULL);
	if (error != 0) {
		(void)pthread_mutex_destroy(&preferences_watcher.lock);
		return error;
	}

	/* Succeeded: the event loop takes what the watcher finds from now on. */
	preferences_watcher.running = 1;
	return 0;
}

/* Looks at the preferences file once a second, away from the event loop, until the run ends. */
static void *
preferences_watch_run(
	void *argument)
{
	struct keiland_preferences *watched;
	int changed;
	int pending;
	int stop;
	int error;

	/* The thread takes no argument: its state is the one watcher of the process. */
	(void)argument;

	/* The watcher's own reading, which it hands over when the file moves; without one it has nothing to do. */
	watched = keiland_preferences_open();
	if (watched == NULL)
		return NULL;

	/* Publishes the reading, where the event loop swaps it and close finds it to free it. */
	(void)pthread_mutex_lock(&preferences_watcher.lock);

	/* The reading is the watcher's until it is handed over. */
	preferences_watcher.watched = watched;

	/* The event loop may see the reading from now. */
	(void)pthread_mutex_unlock(&preferences_watcher.lock);

	/* A look each second until the run ends. */
	for (;;) {
		/* A second's wait, cut short by the end of the run. */
		stop = preferences_watch_wait();
		if (stop != 0)
			break;

		/* Samples whether the last reading was taken, and which reading is the watcher's now. */
		(void)pthread_mutex_lock(&preferences_watcher.lock);

		/* After a swap the watcher's reading is the one the event loop gave back. */
		pending = preferences_watcher.pending;
		watched = preferences_watcher.watched;

		/* The event loop may take the reading meanwhile. */
		(void)pthread_mutex_unlock(&preferences_watcher.lock);

		/* A reading the event loop has not taken yet is left alone until it has. */
		if (pending != 0)
			continue;

		/* The file is read again when it moved (this is the wait on the disk the event loop is spared). */
		error = keiland_preferences_reload(watched, &changed);

		/* Hands the outcome to the event loop: a failure to log, or a new reading to take. */
		(void)pthread_mutex_lock(&preferences_watcher.lock);

		/* pending gives the reading to the event loop, which swaps it at its next pass. */
		if (error != 0)
			preferences_watcher.error = error;
		else if (changed != 0)
			preferences_watcher.pending = 1;

		/* The event loop may take it now. */
		(void)pthread_mutex_unlock(&preferences_watcher.lock);
	}

	/* Succeeded: the run ended. */
	return NULL;
}

/* Waits one look's interval in short naps; returns nonzero as soon as the run is ending. */
static int
preferences_watch_wait(
	void)
{
	struct timespec nap;
	unsigned slept_ms;
	int stop;

	/* Each nap is short, so that close never waits long for the thread. */
	nap.tv_sec = 0;
	nap.tv_nsec = (long)PREFERENCES_NAP_MS * 1000000L;
	for (slept_ms = 0; slept_ms < PREFERENCES_CHECK_MS; slept_ms += PREFERENCES_NAP_MS) {
		(void)nanosleep(&nap, NULL);

		/* Samples whether close asked the watcher to end. */
		(void)pthread_mutex_lock(&preferences_watcher.lock);

		/* stop is set once, by close. */
		stop = preferences_watcher.stop;

		/* Close may set it meanwhile; the next nap sees it. */
		(void)pthread_mutex_unlock(&preferences_watcher.lock);

		/* The watcher stops at once. */
		if (stop != 0)
			return 1;
	}

	/* Succeeded: a whole interval passed. */
	return 0;
}

/* Applies every key whose value differs from what the desktop uses (starting: before the look is made). */
static void
preferences_apply(
	struct zwl_server *server,
	int starting)
{
	struct keiland_preferences *preferences;
	int32_t value;

	/* The wallpaper and the windows' opacity. */
	preferences = server->preferences;
	preferences_wallpaper(server, starting);
	preferences_opacity(server);

	/* The pointer's speed and the wheel's direction. */
	value = keiland_preferences_get_int(preferences, "pointer.speed", PREFERENCES_SPEED_DEFAULT, PREFERENCES_SPEED_MIN, PREFERENCES_SPEED_MAX);
	preferences_number("pointer.speed", &server->pointer_speed, value);
	value = keiland_preferences_get_int(preferences, "pointer.natural", 0, 0, 1);
	preferences_number("pointer.natural", &server->pointer_natural, value);

	/* The keyboards' repeat, for keyboards bound from now on. */
	value = keiland_preferences_get_int(preferences, "keyboard.repeat.rate", PREFERENCES_RATE_DEFAULT, PREFERENCES_RATE_MIN, PREFERENCES_RATE_MAX);
	preferences_number("keyboard.repeat.rate", &server->repeat_rate, value);
	value = keiland_preferences_get_int(preferences, "keyboard.repeat.delay", PREFERENCES_DELAY_DEFAULT, PREFERENCES_DELAY_MIN, PREFERENCES_DELAY_MAX);
	preferences_number("keyboard.repeat.delay", &server->repeat_delay_ms, value);

	/*
	 * The sound's volume is not taken from here: audiod holds it during the
	 * session, and volume.c reads the file once when audiod is first reached
	 * and writes it once at the session's end (BUG-161).
	 */
}

/* Shows the wallpaper the preferences choose (an absolute path), or the command line's when they choose none. */
static void
preferences_wallpaper(
	struct zwl_server *server,
	int starting)
{
	char chosen[KEILAND_PREFERENCES_VALUE_MAX];
	const char *path;
	int differs;
	int error;

	/* The key's value; a key not set, or not an absolute path, chooses the command line's. */
	error = keiland_preferences_get(server->preferences, "wallpaper", chosen, sizeof(chosen));
	if (error != 0 || chosen[0] != '/')
		chosen[0] = '\0';

	/* The same choice as shown changes nothing. */
	differs = strcmp(chosen, server->wallpaper_chosen);
	if (differs == 0)
		return;

	/* The choice, and the picture it means. */
	(void)snprintf(server->wallpaper_chosen, sizeof(server->wallpaper_chosen), "%s", chosen);
	path = server->wallpaper_started;
	if (server->wallpaper_chosen[0] != '\0')
		path = server->wallpaper_chosen;
	server->wallpaper_path = path;

	/* Before the look is made, it draws the picture itself. */
	if (starting != 0) {
		printf("ZWL PREFERENCES key=wallpaper applied\n");
		return;
	}

	/* Afterwards the look draws it now. */
	error = zwl_glass_wallpaper(server, path);
	if (error != 0) {
		printf("ZWL PREFERENCES key=wallpaper failed errno=%d\n", error);
		return;
	}

	/* The new picture is shown. */
	printf("ZWL PREFERENCES key=wallpaper applied\n");
}

/* Sets the windows' opacity the preferences choose (85 to 100 percent), or the command line's. */
static void
preferences_opacity(
	struct zwl_server *server)
{
	float opacity;
	int percent;
	int started;

	/* The command line's opacity in whole percent is the default. */
	started = (int)(server->window_opacity_started * 100.0f + 0.5f);
	percent = keiland_preferences_get_int(server->preferences, "window.opacity", started, PREFERENCES_OPACITY_MIN, PREFERENCES_OPACITY_MAX);

	/* A key not set keeps the command line's exactly (it may lie outside the range). */
	opacity = (float)percent / 100.0f;
	if (percent == started)
		opacity = server->window_opacity_started;

	/* The same opacity changes nothing. */
	if (opacity == server->window_opacity)
		return;

	/* Every window is drawn again at the new opacity. */
	server->window_opacity = opacity;
	server->dirty = 1;
	printf("ZWL PREFERENCES key=window.opacity applied value=%d\n", percent);
}

/* Sets a number the desktop uses when the preferences' value differs, and logs it. */
static void
preferences_number(
	const char *key,
	int32_t *target,
	int32_t value)
{
	/* The same value changes nothing. */
	if (*target == value)
		return;

	/* The new value, from the next input on. */
	*target = value;
	printf("ZWL PREFERENCES key=%s applied value=%d\n", key, (int)value);
}
