/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * settings: the Settings application of the Kei desktop (WS089), in a
 * Wayland window drawn on the CPU and shown with Vulkan.
 *
 *   settings [--display=NAME] [--font=PATH] [--fallback-font=PATH]
 *            [--width=N] [--height=N] [--timeout-s=N] [PAGE]
 *
 * It opens on Home, or on the page PAGE names (network, about, ...).  Its
 * outcome is one line on standard error: ZSETTINGS DONE with the reason,
 * or ZSETTINGS FAILED naming what failed; ZSETTINGS READY says the first
 * frame is shown.
 */

#include "window.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The fonts used unless told otherwise (the fallback is optional). */
#define MAIN_FONT		KEILAND_DATADIR "/fonts/keiland.ttf"
#define MAIN_FALLBACK_FONT	KEILAND_DATADIR "/fonts/keiland-fallback.ttf"

/* How many frames in a row may find the swapchain out of date before the program gives up. */
#define MAIN_STALE_LIMIT	8U

/* A frame that takes longer than this is logged, in milliseconds. */
#define MAIN_SLOW_FRAME_MS	250U

/* The longest the loop sleeps when nothing is due, in milliseconds (the minute About shows moves on). */
#define MAIN_IDLE_MS		1000

/*
 * What the command line asked for.
 */
struct main_options {
	const char *display;
	const char *font;
	const char *fallback;
	unsigned page;
	unsigned width;
	unsigned height;
	unsigned timeout;
};

/*
 * The program's parts, for the whole run.  They are file-scope because
 * the window's input queue and the app are too large for the stack.
 */
static struct se_window main_window;
static struct se_present main_present;
static struct se_app main_app;
static struct fm_text main_text;

/*
 * The window's menus in zdesktop, opened with the window and closed before
 * it; its service is NULL when the compositor has no System Menu.
 */
static struct se_menu main_menu;

/*
 * The window's titlebar in zdesktop (its controls), opened with the window
 * and closed before it; Settings does not run without it.
 */
static struct se_titlebar main_titlebar;

/*
 * The window's glass in zdesktop (its panes on the frosted glass), opened
 * with the presenter and closed before the window; without it the window
 * keeps its opaque ground.
 */
static struct se_glass main_glass;

/* The titlebar's event being carried out (too large for the stack's taste). */
static struct se_titlebar_event main_titlebar_event;

/*
 * The frame being drawn: ordinary memory the size of the swapchain, and
 * the canvas over it.  They are remade when the window changes size.
 */
static uint32_t *main_pixels;
static struct fm_canvas main_canvas;

static int main_parse(int argc, char **argv, struct main_options *options);
static const char *main_value(const char *argument, const char *name);
static int main_number(const char *text, unsigned maximum, unsigned *value);
static int main_loop(const struct main_options *options);
static int main_frame(void);
static int main_canvas_make(void);
static int main_timeout(uint64_t now);
static void main_request(void);
static void main_state_update(void);
static void main_about_window(void);

/*
 * Runs Settings.
 */
int
main(
	int argc,
	char **argv)
{
	struct main_options options;
	struct se_menu_state menu_state;
	struct se_titlebar_state titlebar_state;
	VkResult result;
	int status;
	int error;

	/* The command line. */
	status = main_parse(argc, argv, &options);
	if (status != 0) {
		fprintf(stderr, "usage: settings [--display=NAME] [--font=PATH] [--fallback-font=PATH] [--width=N] [--height=N] [--timeout-s=N] [PAGE]\n");
		return 2;
	}

	/* The fonts. */
	error = fm_text_open(&main_text, options.font, options.fallback);
	if (error != 0) {
		fprintf(stderr, "ZSETTINGS FAILED operation=font path=%s error=%d\n", options.font, error);
		return 1;
	}

	/* The window. */
	status = se_window_open(&main_window, options.display, options.width, options.height, "Settings", "settings");
	if (status != 0) {
		fprintf(stderr, "ZSETTINGS FAILED operation=window error=%d\n", errno);
		se_window_close(&main_window);
		fm_text_close(&main_text);
		return 1;
	}

	/* The presenter. */
	result = se_present_open(&main_present, &main_window);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "ZSETTINGS FAILED operation=%s result=%d\n", main_present.operation, (int)result);
		se_present_close(&main_present);
		se_window_close(&main_window);
		fm_text_close(&main_text);
		return 1;
	}

	/* The interface, and what About shows of the machine, the graphics device and the screen among it. */
	main_app.now = se_clock();
	se_about_read(&main_app.about);
	main_about_window();
	se_ui_init(&main_app, &main_text, options.page);

	/* The network's watch (a daemon not running yet is found later), and the user's preferences. */
	se_network_open(&main_app);
	se_look_open(&main_app);
	se_sound_open(&main_app);

	/* Glass when zdesktop can show the window see-through (the frame's ground is then left clear). */
	main_app.glass = se_glass_open(&main_glass, &main_window, &main_present);

	/* The menus; a window whose menus cannot be made goes on without them. */
	se_ui_menu_state(&main_app, &menu_state);
	error = se_menu_open(&main_menu, &main_window, &menu_state);
	if (error != 0) {
		se_log("MENU failed errno=%d", error);
		se_menu_close(&main_menu);
	}

	/* The titlebar's controls; without zdesktop's titlebar Settings does not start. */
	se_ui_titlebar_state(&main_app, &titlebar_state);
	error = se_titlebar_open(&main_titlebar, &main_window, &titlebar_state);
	if (error != 0) {
		fprintf(stderr, "ZSETTINGS FAILED operation=titlebar errno=%d\n", error);
		se_titlebar_close(&main_titlebar);
		se_menu_close(&main_menu);
		se_glass_close(&main_glass);
		se_present_close(&main_present);
		se_window_close(&main_window);
		fm_text_close(&main_text);
		return 1;
	}

	/* The loop, until the window closes. */
	status = main_loop(&options);

	/* Everything goes, the network's watch, then the titlebar, the menus and the glass before the window they belong to. */
	se_network_close(&main_app);
	se_sound_close(&main_app);
	se_look_close(&main_app);
	se_titlebar_close(&main_titlebar);
	se_menu_close(&main_menu);
	se_glass_close(&main_glass);
	fm_canvas_release(&main_canvas);
	free(main_pixels);
	se_present_close(&main_present);
	se_window_close(&main_window);
	fm_text_close(&main_text);

	/* Reports how the run ended. */
	if (status != 0)
		return 1;

	/* Succeeded: the window was closed. */
	return 0;
}

/* Reads the command line into the options; returns nonzero for a malformed one. */
static int
main_parse(
	int argc,
	char **argv,
	struct main_options *options)
{
	const struct se_page *page;
	const char *value;
	int status;
	int index;

	/* The defaults. */
	memset(options, 0, sizeof(*options));
	options->font = MAIN_FONT;
	options->fallback = MAIN_FALLBACK_FONT;
	options->page = SE_PAGE_HOME;
	options->width = SE_WIDTH;
	options->height = SE_HEIGHT;

	/* Each argument. */
	for (index = 1; index < argc; index++) {
		/* The compositor's display. */
		value = main_value(argv[index], "--display=");
		if (value != NULL) {
			options->display = value;
			continue;
		}

		/* The main font. */
		value = main_value(argv[index], "--font=");
		if (value != NULL) {
			options->font = value;
			continue;
		}

		/* The fallback font. */
		value = main_value(argv[index], "--fallback-font=");
		if (value != NULL) {
			options->fallback = value;
			continue;
		}

		/* The window's width. */
		value = main_value(argv[index], "--width=");
		if (value != NULL) {
			status = main_number(value, 8192U, &options->width);
			if (status != 0)
				return status;
			continue;
		}

		/* The window's height. */
		value = main_value(argv[index], "--height=");
		if (value != NULL) {
			status = main_number(value, 8192U, &options->height);
			if (status != 0)
				return status;
			continue;
		}

		/* How long the program runs at most (0 for ever). */
		value = main_value(argv[index], "--timeout-s=");
		if (value != NULL) {
			status = main_number(value, 86400U, &options->timeout);
			if (status != 0)
				return status;
			continue;
		}

		/* An unknown option refuses the command line. */
		if (argv[index][0] == '-')
			return -1;

		/* The page to open, which must be one of the pages' words. */
		page = se_page_find(argv[index]);
		if (page == NULL)
			return -1;
		options->page = page->id;
	}

	/* A window has some size. */
	if (options->width < 480U || options->height < 360U)
		return -1;

	/* Succeeded: the options are read. */
	return 0;
}

/* Returns what follows an option's name in an argument, or NULL when the argument is another option. */
static const char *
main_value(
	const char *argument,
	const char *name)
{
	size_t length;
	int match;

	/* The name must start the argument. */
	length = strlen(name);
	match = strncmp(argument, name, length);
	if (match != 0)
		return NULL;

	/* Reports the value after it. */
	return argument + length;
}

/* Reads a decimal number no larger than a maximum; nonzero for a malformed one. */
static int
main_number(
	const char *text,
	unsigned maximum,
	unsigned *value)
{
	unsigned long number;
	char *end;

	/* The digits, all of them. */
	errno = 0;
	number = strtoul(text, &end, 10);
	if (errno != 0 ||
	    end == text ||
	    *end != '\0' ||
	    number > maximum)
		return -1;

	/* Succeeded: the number. */
	*value = (unsigned)number;
	return 0;
}

/* Runs the window until it closes (or the timeout passes); returns nonzero when something failed. */
static int
main_loop(
	const struct main_options *options)
{
	struct se_event event;
	uint64_t started;
	uint64_t now;
	int inputs;
	int taken;
	int status;
	int timeout;

	/* The first frame's canvas. */
	status = main_canvas_make();
	if (status != 0) {
		fprintf(stderr, "ZSETTINGS FAILED operation=canvas\n");
		return -1;
	}

	/* The first frame. */
	status = main_frame();
	if (status != 0)
		return -1;

	/* The log line the tests wait for. */
	se_log("READY width=%u height=%u glass=%d page=%s", main_present.extent.width, main_present.extent.height, main_app.glass, se_pages[main_app.page].word);

	/* Each round: input, time, and a frame when something changed. */
	started = se_clock();
	for (;;) {
		/* Waits for the compositor, or until something is due. */
		now = se_clock();
		timeout = main_timeout(now);
		status = se_window_dispatch(&main_window, timeout);
		if (status != 0) {
			se_log("DONE reason=disconnected");
			return 0;
		}

		/* The held key's repeat, and every input queued (the menus' choices among them). */
		now = se_clock();
		(void)se_window_repeat(&main_window, now);
		inputs = 0;
		for (;;) {
			taken = se_window_take(&main_window, &event);
			if (taken == 0)
				break;
			se_ui_event(&main_app, &event);
			inputs++;
		}

		/* What was done with the titlebar, oldest first, at the time now. */
		main_app.now = now;
		for (;;) {
			taken = se_titlebar_take(&main_titlebar, &main_titlebar_event);
			if (taken == 0)
				break;
			se_ui_titlebar(&main_app, &main_titlebar_event);
			inputs++;
		}

		/* What the window was asked to do: minimizing, zooming, closing. */
		main_request();

		/* Time passes for the interface (the minute About shows), and the network reports. */
		se_ui_tick(&main_app, now);
		se_network_poll(&main_app, now);
		se_look_poll(&main_app, now);
		se_sound_poll(&main_app, now);
		if (main_app.dirty != 0)
			inputs++;

		/* The titlebar and the menus show the state after input. */
		if (inputs != 0)
			main_state_update();

		/* The close button ends the run. */
		if (main_window.closed != 0) {
			se_log("DONE reason=close");
			return 0;
		}

		/* So does the timeout, when one was given. */
		if (options->timeout != 0U && now - started >= (uint64_t)options->timeout * 1000U) {
			se_log("DONE reason=timeout");
			return 0;
		}

		/* A new size: a new swapchain and canvas, and a frame. */
		if (main_window.resized != 0) {
			main_window.resized = 0;
			status = se_present_resize(&main_present, main_window.width, main_window.height);
			if (status != VK_SUCCESS) {
				fprintf(stderr, "ZSETTINGS FAILED operation=%s result=%d\n", main_present.operation, status);
				return -1;
			}

			/* The canvas to match. */
			status = main_canvas_make();
			if (status != 0)
				return -1;
			main_app.dirty = 1;
		}

		/* A frame when something changed. */
		if (main_app.dirty != 0) {
			status = main_frame();
			if (status != 0)
				return -1;
		}
	}
}

/* Draws and shows a frame, remaking the swapchain when it is out of date; nonzero when it cannot be shown. */
static int
main_frame(void)
{
	VkResult result;
	uint64_t started;
	uint64_t drawn;
	uint64_t shown;
	unsigned stale;
	int status;

	/* Tries until the frame is shown, remaking a stale swapchain a few times. */
	for (stale = 0; stale < MAIN_STALE_LIMIT; stale++) {
		/* The frame on the CPU, laid out for a docked or a floating window. */
		started = se_clock();
		main_app.docked = main_window.maximized;
		se_ui_draw(&main_app, &main_canvas);
		drawn = se_clock();

		/* The frame's glass panels, sent to take effect with it. */
		se_glass_refresh(&main_glass, &main_app);

		/* Shown in the window. */
		result = se_present_frame(&main_present, main_pixels, (size_t)main_present.extent.width);
		shown = se_clock();

		/* A slow frame is logged (a diagnostic: where the time of a frame goes). */
		if (shown - started > MAIN_SLOW_FRAME_MS)
			se_log("SLOW-FRAME draw=%lu present=%lu copy=%u acquire=%u queue=%u wait=%u", (unsigned long)(drawn - started), (unsigned long)(shown - drawn), main_present.copy_ms, main_present.acquire_ms, main_present.present_ms, main_present.wait_ms);

		/* Succeeded: the frame is shown (the retries below are for a stale swapchain only). */
		if (result == VK_SUCCESS)
			return 0;

		/* Anything but a stale swapchain is a failure. */
		if (result != VK_ERROR_OUT_OF_DATE_KHR) {
			fprintf(stderr, "ZSETTINGS FAILED operation=%s result=%d\n", main_present.operation, (int)result);
			return -1;
		}

		/* A stale swapchain is remade at the window's size, with a canvas to match. */
		result = se_present_resize(&main_present, main_window.width, main_window.height);
		if (result != VK_SUCCESS) {
			fprintf(stderr, "ZSETTINGS FAILED operation=%s result=%d\n", main_present.operation, (int)result);
			return -1;
		}

		/* The canvas to match. */
		status = main_canvas_make();
		if (status != 0)
			return -1;
	}

	/* The swapchain stayed out of date. */
	fprintf(stderr, "ZSETTINGS FAILED operation=stale-swapchain\n");
	return -1;
}

/* Makes the frame's memory and canvas at the swapchain's size; nonzero when memory runs out. */
static int
main_canvas_make(void)
{
	size_t count;
	int error;

	/* The old canvas and memory go. */
	fm_canvas_release(&main_canvas);
	free(main_pixels);

	/* Memory for the swapchain's size. */
	count = (size_t)main_present.extent.width * (size_t)main_present.extent.height;
	main_pixels = calloc(count, sizeof(uint32_t));
	if (main_pixels == NULL)
		return -1;

	/* The canvas over it. */
	error = fm_canvas_init(&main_canvas, main_pixels, (size_t)main_present.extent.width, (int)main_present.extent.width, (int)main_present.extent.height);
	if (error != 0)
		return -1;

	/* Succeeded: frames can be drawn. */
	return 0;
}

/* Reports how long the loop may sleep: not at all while a frame is due, else until a key repeats, or the idle limit. */
static int
main_timeout(
	uint64_t now)
{
	uint64_t wait;
	int network;
	int sound;
	int look;
	int limit;

	/* A frame the last one asked for (a scroll it corrected) is drawn at once. */
	if (main_app.dirty != 0)
		return 0;

	/* The idle limit, shortened while the network wants polls. */
	limit = MAIN_IDLE_MS;
	network = se_network_wait(&main_app);
	if (network >= 0 && network < limit)
		limit = network;

	/* And while the sound holds something back, or its page follows audiod. */
	sound = se_sound_wait(&main_app);
	if (sound >= 0 && sound < limit)
		limit = sound;

	/* And while the Wallpaper page's small copies are being read, so each tile fills soon after its copy. */
	look = se_look_wait(&main_app);
	if (look >= 0 && look < limit)
		limit = look;

	/* No key is held: the limit. */
	if (main_window.repeat_key == 0U)
		return limit;

	/* A repeat already due is due now. */
	if (main_window.repeat_at <= now)
		return 0;

	/* A held key repeats soon. */
	wait = main_window.repeat_at - now;
	if (wait < (uint64_t)limit)
		return (int)wait;

	/* Otherwise the limit. */
	return limit;
}

/* Carries out what the window was asked to do by an action, once. */
static void
main_request(void)
{
	unsigned request;

	/* The request, taken. */
	request = main_app.request;
	main_app.request = SE_REQUEST_NONE;

	/* Each request. */
	switch (request) {
	case SE_REQUEST_MINIMIZE:
		se_window_minimize(&main_window);
		break;
	case SE_REQUEST_ZOOM:
		se_window_zoom(&main_window);
		break;
	case SE_REQUEST_CLOSE:
		main_window.closed = 1;
		break;
	default:
		break;
	}
}

/* Tells the titlebar and the menus the window's state (each sends only what changed). */
static void
main_state_update(void)
{
	struct se_titlebar_state titlebar;
	struct se_menu_state menu;

	/* The titlebar. */
	se_ui_titlebar_state(&main_app, &titlebar);
	se_titlebar_refresh(&main_titlebar, &titlebar);

	/* The menus. */
	se_ui_menu_state(&main_app, &menu);
	se_menu_refresh(&main_menu, &menu);
}

/* Puts what the window learned into About: the graphics device's name and the screen's mode. */
static void
main_about_window(void)
{
	unsigned hertz;

	/* The graphics device, as Vulkan names it. */
	(void)snprintf(main_app.about.graphics, sizeof(main_app.about.graphics), "%s", main_present.device_name);

	/* The screen's mode, when the compositor told it (the refresh in millihertz, shown in whole hertz). */
	if (main_window.output_width <= 0 || main_window.output_height <= 0)
		return;
	hertz = (unsigned)((main_window.output_refresh + 500) / 1000);
	if (hertz > 0U) {
		(void)snprintf(main_app.about.display, sizeof(main_app.about.display), "%d x %d, %u Hz", (int)main_window.output_width, (int)main_window.output_height, hertz);
	} else {
		(void)snprintf(main_app.about.display, sizeof(main_app.about.display), "%d x %d", (int)main_window.output_width, (int)main_window.output_height);
	}
}
