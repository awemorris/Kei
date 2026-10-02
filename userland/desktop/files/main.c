/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * files: the file manager of the zedBSD desktop, in a Wayland
 * window drawn on the CPU and shown with Vulkan.
 *
 *   files [--display=NAME] [--font=PATH] [--fallback-font=PATH]
 *                  [--width=N] [--height=N] [--wallpaper=PATH] [--token=NAME] [--timeout-s=N] [FOLDER]
 *
 * It opens on the home dashboard, or on FOLDER.  Its outcome is one line
 * on standard error: ZFILES DONE with the reason, or ZFILES FAILED naming
 * what failed; ZFILES READY says the first frame is shown.
 */

#include "window.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* The fonts used unless told otherwise (the fallback is optional). */
#define MAIN_FONT		KEILAND_DATADIR "/fonts/keiland.ttf"
#define MAIN_FALLBACK_FONT	KEILAND_DATADIR "/fonts/keiland-fallback.ttf"

/* How many frames in a row may find the swapchain out of date before the program gives up. */
#define MAIN_STALE_LIMIT	8U

/* A frame that takes longer than this is logged, in milliseconds. */
#define MAIN_SLOW_FRAME_MS	250U

/* The program a new window runs when this one was not started by its full path. */
#define MAIN_PROGRAM		KEILAND_BINDIR "/files"

/* How often the menus' state is checked at most while no input arrives, in milliseconds. */
#define MAIN_MENU_CHECK_MS	250U

/* The longest the loop sleeps when nothing is due, in milliseconds (folders are checked for changes). */
#define MAIN_IDLE_MS		500

/*
 * What the command line asked for.
 */
struct main_options {
	const char *program;
	const char *display;
	const char *font;
	const char *fallback;
	const char *token;
	const char *start;
	const char *wallpaper;
	unsigned width;
	unsigned height;
	unsigned timeout;
	int desktop;
};

/*
 * The times of the steps of the start, in fm_clock()'s milliseconds, logged
 * once with the first frame in the desktop mode (ws094-p009).
 */
struct main_startup {
	uint64_t entered;  /* main() entered */
	uint64_t fonts;	   /* the fonts read */
	uint64_t instance; /* the desktop's Vulkan instance made (before its surface) */
	uint64_t window;   /* the window or desktop surface open */
	uint64_t present;  /* the presenter open */
	uint64_t app;	   /* the file manager (its first listing) made */
	uint64_t canvas;   /* the first canvas made */
	uint64_t draw_ms;  /* the first frame's drawing */
	uint64_t shown;	   /* the first frame shown */
	uint64_t menus;	   /* the desktop's context menus open, after the first frame */
};

/*
 * The desktop mode (files --desktop, ws094-p003): the token zdesktop gave
 * the program, copied before it leaves the environment (unsetenv frees the
 * environment's string), and the folder shown (~/Desktop).  Empty outside
 * the desktop mode.
 */
static char main_desktop_token[128];

/* The desktop folder path, retained for option storage throughout the run. */
static char main_desktop_folder[FM_PATH_MAX];

/*
 * The program's parts, for the whole run.  They are file-scope because
 * the window's input queue and the app are too large for the stack.
 */
static struct fm_window main_window;
static struct fm_present main_present;
static struct fm_app main_app;
static struct fm_text main_text;

/*
 * The touch screen (touch.c, ws081-p010): the fingers' gestures and
 * scroller, made with the window (without them fingers do nothing).
 */
static struct fm_touch main_touch;

/* In how many milliseconds the fingers want the next round (-1: none). */
static int main_touch_due = -1;

/*
 * The window's menus in zdesktop, opened with the window and closed before
 * it; its service is NULL when the compositor has no System Menu.
 */
static struct fm_menu main_menu;

/*
 * The window's titlebar in zdesktop (its controls), opened with the window
 * and closed before it; the file manager does not run without it.
 */
static struct fm_titlebar main_titlebar;

/*
 * The window's glass in zdesktop (its panels on the frosted glass), opened
 * with the presenter and closed before the window; without it the window
 * keeps its opaque ground.
 */
static struct fm_glass main_glass;

/* The context menu being opened (too large for the stack's taste). */
static struct fm_context main_context;

/* The titlebar's event being carried out, and its state being made (both too large for the stack's taste). */
static struct fm_titlebar_event main_titlebar_event;
static struct fm_titlebar_state main_titlebar_state;

/*
 * The frame being drawn: ordinary memory the size of the swapchain, and
 * the canvas over it.  They are remade when the window changes size.
 */
static uint32_t *main_pixels;
static struct fm_canvas main_canvas;

/* The current run's startup samples; zero means a step has not been recorded yet. */
static struct main_startup main_startup;

static int main_parse(int argc, char **argv, struct main_options *options);
static const char *main_value(const char *argument, const char *name);
static int main_number(const char *text, unsigned maximum, unsigned *value);
static int main_loop(const struct main_options *options);
static int main_frame(void);
static int main_canvas_make(void);
static int main_timeout(uint64_t now);
static void main_request(const struct main_options *options);
static void main_new_window(const struct main_options *options);
static void main_drag_out(void);
static void main_drop(void);
static void main_menu_update(void);
static void main_touch_round(void);
static unsigned main_touch_area(int x, int y);
static void main_touch_pointer(const struct fm_touch_pointer *made);
static int main_desktop_prepare(struct main_options *options);
static int main_open_decorations(void);
static void main_open_context_menus(void);
static void main_dispatch(const struct fm_event *event);

/*
 * Runs the file manager.
 */
int
main(
	int argc,
	char **argv)
{
	struct main_options options;
	VkResult result;
	int status;
	int error;

	/* The command line. */
	main_startup.entered = fm_clock();
	status = main_parse(argc, argv, &options);
	if (status != 0) {
		fprintf(stderr, "usage: files [--display=NAME] [--font=PATH] [--fallback-font=PATH] [--width=N] [--height=N] [--wallpaper=PATH] [--token=NAME] [--timeout-s=N] [--desktop] [FOLDER]\n");
		return 2;
	}

	/* The desktop mode: its token out of the environment, its folder. */
	if (options.desktop) {
		error = main_desktop_prepare(&options);
		if (error != 0) {
			fprintf(stderr, "ZFILES FAILED operation=desktop error=%d\n", error);
			return 1;
		}
	}

	/* The fonts. */
	error = fm_text_open(&main_text, options.font, options.fallback);
	if (error != 0) {
		fprintf(stderr, "ZFILES FAILED operation=font path=%s error=%d\n", options.font, error);
		return 1;
	}

	/* The fonts' time (the desktop's start is logged in steps). */
	main_startup.fonts = fm_clock();

	/*
	 * The desktop's Vulkan instance first: zdesktop, which started it, is
	 * still opening its output and answers the surface's requests only
	 * after that (ws094-p009).  A failure is tried again with the window.
	 */
	if (options.desktop) {
		result = fm_present_instance(&main_present);
		if (result != VK_SUCCESS)
			main_present.instance = VK_NULL_HANDLE;
	}

	/* The instance's time. */
	main_startup.instance = fm_clock();

	/* The window, or the desktop surface in the desktop mode. */
	if (options.desktop) {
		status = fm_window_open_desktop(&main_window, options.display, main_desktop_token);
	} else {
		status = fm_window_open(&main_window, options.display, options.width, options.height, "Files", "files");
	}
	if (status != 0) {
		fprintf(stderr, "ZFILES FAILED operation=window error=%d\n", errno);
		fm_text_close(&main_text);
		return 1;
	}

	/* The window's time. */
	main_startup.window = fm_clock();

	/* The fingers; without memory for them they do nothing. */
	error = fm_touch_open(&main_touch);
	if (error != 0)
		fm_log("TOUCH none error=%d", error);

	/* The presenter. */
	result = fm_present_open(&main_present, &main_window);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "ZFILES FAILED operation=%s result=%d\n", main_present.operation, (int)result);
		fm_present_close(&main_present);
		fm_window_close(&main_window);
		fm_text_close(&main_text);
		return 1;
	}

	/* The file manager itself. */
	main_startup.present = fm_clock();
	main_app.now = main_startup.present;
	error = fm_app_init(&main_app, &main_text, options.start);
	if (error != 0) {
		fprintf(stderr, "ZFILES FAILED operation=app error=%d\n", error);
		fm_present_close(&main_present);
		fm_window_close(&main_window);
		fm_text_close(&main_text);
		return 1;
	}

	/* The dashboard's picture, when another was asked for. */
	main_startup.app = fm_clock();
	if (options.wallpaper != NULL)
		snprintf(main_app.wallpaper, sizeof(main_app.wallpaper), "%s", options.wallpaper);

	/*
	 * The desktop has no glass, window menus or titlebar: its icons are
	 * drawn on the clear surface (ui-desktop.c), and its context menus open
	 * after the first frame (main_loop; the service's search waits for
	 * zdesktop, which is importing the surface's images then, ws094-p009).
	 */
	main_app.desktop = options.desktop;
	if (!options.desktop) {
		/* A window's glass, menus and titlebar; without zdesktop's titlebar the file manager does not start. */
		status = main_open_decorations();
		if (status != 0) {
			fm_app_release(&main_app);
			fm_present_close(&main_present);
			fm_window_close(&main_window);
			fm_text_close(&main_text);
			return 1;
		}
	}

	/* The loop, until the window closes. */
	status = main_loop(&options);

	/* Everything goes, the titlebar, the menus, the glass and the app before the window they belong to. */
	fm_titlebar_close(&main_titlebar);
	fm_menu_close(&main_menu);
	fm_glass_close(&main_glass);
	fm_desktop_release(&main_app.desk);
	fm_app_release(&main_app);
	fm_canvas_release(&main_canvas);
	free(main_pixels);
	fm_present_close(&main_present);
	fm_touch_close(&main_touch);
	fm_window_close(&main_window);
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
	const char *value;
	int status;
	int index;

	/* The defaults; a new window runs this program again when it was started by its full path. */
	memset(options, 0, sizeof(*options));
	options->program = MAIN_PROGRAM;
	if (argc > 0 && argv[0] != NULL && argv[0][0] == '/')
		options->program = argv[0];
	options->font = MAIN_FONT;
	options->fallback = MAIN_FALLBACK_FONT;
	options->width = FM_WIDTH;
	options->height = FM_HEIGHT;

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

		/* The picture of the dashboard's hero card (the desktop's wallpaper by default). */
		value = main_value(argv[index], "--wallpaper=");
		if (value != NULL) {
			options->wallpaper = value;
			continue;
		}

		/* A name the log lines carry (a test tells its windows apart by it). */
		value = main_value(argv[index], "--token=");
		if (value != NULL) {
			options->token = value;
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

		/* The desktop mode: the icons of ~/Desktop on zdesktop's desktop surface (ws094-p003). */
		status = strcmp(argv[index], "--desktop");
		if (status == 0) {
			options->desktop = 1;
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

		/* The folder to open, once. */
		if (options->start != NULL)
			return -1;
		options->start = argv[index];
	}

	/* A window has some size. */
	if (options->width < 320U || options->height < 240U)
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
	struct fm_event event;
	const char *token;
	uint64_t started;
	uint64_t now;
	uint64_t menu_checked_at;
	int inputs;
	int taken;
	int status;
	int timeout;

	/* The first frame's canvas. */
	status = main_canvas_make();
	if (status != 0) {
		fprintf(stderr, "ZFILES FAILED operation=canvas\n");
		return -1;
	}

	/* The first frame. */
	main_startup.canvas = fm_clock();
	status = main_frame();
	if (status != 0)
		return -1;
	main_startup.shown = fm_clock();

	/* The desktop's context menus, and the steps of its start from main() (the tests read them). */
	if (main_app.desktop) {
		main_open_context_menus();
		main_startup.menus = fm_clock();
		fm_log("DESKTOP startup entered_ms=%llu fonts=%lu instance=%lu window=%lu present=%lu app=%lu canvas=%lu draw=%lu shown=%lu menus=%lu",
		       (unsigned long long)main_startup.entered,
		       (unsigned long)(main_startup.fonts - main_startup.entered),
		       (unsigned long)(main_startup.instance - main_startup.fonts),
		       (unsigned long)(main_startup.window - main_startup.instance),
		       (unsigned long)(main_startup.present - main_startup.window),
		       (unsigned long)(main_startup.app - main_startup.present),
		       (unsigned long)(main_startup.canvas - main_startup.app),
		       (unsigned long)main_startup.draw_ms,
		       (unsigned long)(main_startup.shown - main_startup.canvas - main_startup.draw_ms),
		       (unsigned long)(main_startup.menus - main_startup.shown));
	}

	/* The log line the tests wait for. */
	token = "-";
	if (options->token != NULL)
		token = options->token;
	fm_log("READY width=%u height=%u token=%s", main_present.extent.width, main_present.extent.height, token);

	/* Each round: input, time, and a frame when something changed. */
	started = fm_clock();
	menu_checked_at = started;
	for (;;) {
		/* Waits for the compositor, or until something is due. */
		now = fm_clock();
		timeout = main_timeout(now);
		status = fm_window_dispatch(&main_window, timeout);
		if (status != 0) {
			fm_log("DONE reason=disconnected");
			return 0;
		}

		/* The held key's repeat, and every input queued (the menus' choices among them); the desktop has its own (ui-desktop.c). */
		now = fm_clock();
		(void)fm_window_repeat(&main_window, now);
		inputs = 0;
		for (;;) {
			taken = fm_window_take(&main_window, &event);
			if (taken == 0)
				break;
			main_dispatch(&event);
			inputs++;
		}

		/* The fingers: what they scroll, and the pointer's clicks and presses they make (ws081-p010). */
		main_app.now = now;
		if (main_window.touch_count != 0U)
			inputs++;
		main_touch_round();

		/* What was done with the titlebar, oldest first, at the time now. */
		main_app.now = now;
		for (;;) {
			taken = fm_titlebar_take(&main_titlebar, &main_titlebar_event);
			if (taken == 0)
				break;
			fm_ui_titlebar(&main_app, &main_titlebar_event);
			inputs++;
		}

		/* What the window was asked to do: a new window, minimizing, zooming, closing, a context menu, a drag and drop. */
		main_request(options);

		/* An "ask" whose context menu closed without a choice gives the drop up. */
		if (main_app.drop_asking != 0 && main_menu.context_done != 0) {
			main_app.drop_asking = 0;
			fm_log("DROP ask dismissed");
			fm_dnd_abort(&main_window);
		}

		/* A drag over the window whose target changed is answered: its file names taken (move preferred) or not (dnd.c). */
		if (main_app.drop_answer != 0) {
			main_app.drop_answer = 0;
			fm_dnd_answer(&main_window, fm_drop_accepts(&main_app), FM_DND_MOVE);
		}

		/* Time passes for the file manager. */
		fm_ui_tick(&main_app, now);

		/* The menus show the state after input at once, and otherwise now and then (a task's end changes it); the desktop's context menu goes when it closed. */
		if (inputs != 0 || now - menu_checked_at >= MAIN_MENU_CHECK_MS) {
			main_menu_update();
			menu_checked_at = now;
		}

		/* The close button ends the run. */
		if (main_window.closed != 0) {
			fm_log("DONE reason=close");
			return 0;
		}

		/* So does the timeout, when one was given. */
		if (options->timeout != 0U && now - started >= (uint64_t)options->timeout * 1000U) {
			fm_log("DONE reason=timeout");
			return 0;
		}

		/* A new size: a new swapchain and canvas, and a frame. */
		if (main_window.resized != 0) {
			main_window.resized = 0;
			status = fm_present_resize(&main_present, main_window.width, main_window.height);
			if (status != VK_SUCCESS) {
				fprintf(stderr, "ZFILES FAILED operation=%s result=%d\n", main_present.operation, status);
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
		started = fm_clock();
		main_app.docked = main_window.maximized;
		if (main_app.desktop) {
			fm_desktop_draw(&main_app, &main_canvas);
		} else {
			fm_ui_draw(&main_app, &main_canvas);
		}
		drawn = fm_clock();
		if (main_startup.draw_ms == 0U)
			main_startup.draw_ms = drawn - started;

		/* The frame's glass panels, sent to take effect with it (the desktop has none). */
		if (!main_app.desktop)
			fm_glass_refresh(&main_glass, &main_app);

		/* Shown in the window. */
		result = fm_present_frame(&main_present, main_pixels, (size_t)main_present.extent.width);
		shown = fm_clock();

		/* The desktop's selection shown: the time from the press that selected (ws094-p008). */
		if (main_app.desktop &&
		    main_app.desk.select_ms != 0U &&
		    result == VK_SUCCESS) {
			fm_log("DESKTOP select-frame ms=%lu before=%lu draw=%lu copy=%u acquire=%u submit=%u queue=%u wait=%u",
			       (unsigned long)(shown - main_app.desk.select_ms),
			       (unsigned long)(started - main_app.desk.select_ms),
			       (unsigned long)(drawn - started),
			       main_present.copy_ms,
			       main_present.acquire_ms,
			       main_present.submit_ms,
			       main_present.present_ms,
			       main_present.wait_ms);
			main_app.desk.select_ms = 0U;
		}

		/* A slow frame is logged (a diagnostic: where the time of a frame goes). */
		if (shown - started > MAIN_SLOW_FRAME_MS)
			fm_log("SLOW-FRAME draw=%lu present=%lu copy=%u acquire=%u queue=%u wait=%u", (unsigned long)(drawn - started), (unsigned long)(shown - drawn), main_present.copy_ms, main_present.acquire_ms, main_present.present_ms, main_present.wait_ms);
		if (result == VK_SUCCESS)
			return 0;

		/* Anything but a stale swapchain is a failure. */
		if (result != VK_ERROR_OUT_OF_DATE_KHR) {
			fprintf(stderr, "ZFILES FAILED operation=%s result=%d\n", main_present.operation, (int)result);
			return -1;
		}

		/* A stale swapchain is remade at the window's size, with a canvas to match. */
		result = fm_present_resize(&main_present, main_window.width, main_window.height);
		if (result != VK_SUCCESS) {
			fprintf(stderr, "ZFILES FAILED operation=%s result=%d\n", main_present.operation, (int)result);
			return -1;
		}

		/* The canvas to match. */
		status = main_canvas_make();
		if (status != 0)
			return -1;
	}

	/* The swapchain stayed out of date. */
	fprintf(stderr, "ZFILES FAILED operation=stale-swapchain\n");
	return -1;
}

/* Makes the frame's memory and canvas at the swapchain's size; nonzero when memory runs out. */
static int
main_canvas_make(void)
{
	size_t count;
	int error;

	/* The old canvas and memory go, and with them the desktop's kept frame. */
	fm_canvas_release(&main_canvas);
	free(main_pixels);
	fm_desktop_repaint(&main_app.desk);

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

/* Reports how long the loop may sleep: until a key repeats, the file manager's work moves on, or the idle limit. */
static int
main_timeout(
	uint64_t now)
{
	uint64_t wait;
	int limit;
	int busy;

	/* The idle limit, shortened while the file manager has work waiting, or the fingers want their next round. */
	limit = MAIN_IDLE_MS;
	busy = fm_ui_wait(&main_app);
	if (busy >= 0 && busy < limit)
		limit = busy;
	if (main_touch_due >= 0 && main_touch_due < limit)
		limit = main_touch_due;

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
main_request(
	const struct main_options *options)
{
	unsigned request;

	/* The request, taken. */
	request = main_app.request;
	main_app.request = FM_REQUEST_NONE;

	/* Each request. */
	switch (request) {
	case FM_REQUEST_NEW_WINDOW:
		main_new_window(options);
		break;
	case FM_REQUEST_MINIMIZE:
		fm_window_minimize(&main_window);
		break;
	case FM_REQUEST_ZOOM:
		fm_window_zoom(&main_window);
		break;
	case FM_REQUEST_CLOSE:
		main_window.closed = 1;
		break;
	case FM_REQUEST_CONTEXT:
		/* The context menu of the right press, at its place (ui-context.c, menu.c). */
		fm_ui_context(&main_app, &main_context);
		fm_menu_context(&main_menu, &main_context, main_app.context_x, main_app.context_y, 0U);
		break;
	case FM_REQUEST_DROP_ASK:
		/* A drop dropped with "ask": the choice at the drop's place, answering its enter (ui-context.c). */
		fm_ui_context(&main_app, &main_context);
		fm_menu_context(&main_menu, &main_context, main_app.context_x, main_app.context_y, main_window.drop_serial);
		break;
	case FM_REQUEST_DROP_CANCEL:
		/* The choice was none: the drop is given up (its source is cancelled). */
		fm_log("DROP ask cancel");
		fm_dnd_abort(&main_window);
		break;
	case FM_REQUEST_DRAG_OUT:
		/* The dragged items left the window: zdesktop carries them (dnd.c). */
		main_drag_out();
		break;
	case FM_REQUEST_DROP:
		/* A drop on the window: its names, the task, the finish (dnd.c, ui-drag.c). */
		main_drop();
		break;
	default:
		break;
	}
}

/*
 * Hands the selection dragged out of the window to zdesktop's drag and
 * drop; when it cannot be, the window's drag ends as cancelled.
 */
static void
main_drag_out(void)
{
	struct fm_event event;
	char **paths;
	size_t count;
	int error;

	/* The selection's paths, and the drag with them. */
	paths = NULL;
	count = 0;
	error = fm_selected_paths(&main_app, &paths, &count);
	if (error == 0 && count > 0)
		error = fm_dnd_start(&main_window, paths, count);
	if (error == 0 && count == 0)
		error = ENOENT;
	fm_paths_free(paths, count);

	/* Succeeded: zdesktop has it (its end comes as FM_EVENT_DRAG_DONE). */
	if (error == 0)
		return;

	/* Otherwise the window's (or the desktop's) drag ends here, as cancelled. */
	fm_log("DND failed errno=%d", error);
	memset(&event, 0, sizeof(event));
	event.type = FM_EVENT_DRAG_DONE;
	event.time = fm_clock();
	main_dispatch(&event);
}

/*
 * Carries out a drop on the window: the paths (the window's own selection
 * for its own drag, else read from zdesktop), the task into the drop's
 * folder, and the finish zdesktop tells the drag's source.
 */
static void
main_drop(void)
{
	uint32_t action;
	char **paths;
	size_t count;
	int placed;
	int error;

	/* The desktop's own items dropped on the desktop move to cells, and no file moves (ui-desktop-drag.c). */
	if (main_app.desktop) {
		placed = fm_desktop_drop_place(&main_app);
		if (placed) {
			fm_dnd_finish(&main_window, FM_DND_MOVE);
			return;
		}
	}

	/* The paths dropped. */
	paths = NULL;
	count = 0;
	if (main_app.drop_self != 0) {
		error = fm_selected_paths(&main_app, &paths, &count);
	} else {
		error = fm_dnd_receive(&main_window, &paths, &count);
	}

	/* The task into the folder; on the desktop, the new items' places from the drop's cell. */
	if (error == 0) {
		fm_drop_perform(&main_app, paths, count);
		if (main_app.desktop && main_app.drop_self == 0)
			fm_desktop_dropped(&main_app, paths, count);
	} else {
		fm_log("DROP failed errno=%d", error);
	}

	/* The paths go. */
	fm_paths_free(paths, count);

	/* Succeeded or not, the drop is done, with the operation chosen (an "ask" says it now). */
	action = FM_DND_MOVE;
	if (main_app.drop_operation != FM_TASK_MOVE)
		action = FM_DND_COPY;
	fm_dnd_finish(&main_window, action);
}

/*
 * Starts another window of the file manager (a process of its own) on the
 * folder shown, with this window's display, fonts, size and picture.
 */
static void
main_new_window(
	const struct main_options *options)
{
	char *arguments[12];
	char display[FM_PATH_MAX + 16];
	char font[FM_PATH_MAX + 16];
	char fallback[FM_PATH_MAX + 32];
	char wallpaper[FM_PATH_MAX + 16];
	char width[32];
	char height[32];
	char token[96];
	char folder[FM_PATH_MAX];
	const char *shown;
	int count;
	int error;

	/* The folder shown, or the home dashboard when none is. */
	shown = fm_current_folder(&main_app);
	folder[0] = '\0';
	if (shown != NULL)
		snprintf(folder, sizeof(folder), "%s", shown);

	/* The program, its fonts and its size. */
	count = 0;
	arguments[count] = (char *)options->program;
	count++;
	snprintf(font, sizeof(font), "--font=%s", options->font);
	arguments[count] = font;
	count++;
	snprintf(fallback, sizeof(fallback), "--fallback-font=%s", options->fallback);
	arguments[count] = fallback;
	count++;
	snprintf(width, sizeof(width), "--width=%d", main_app.width);
	arguments[count] = width;
	count++;
	snprintf(height, sizeof(height), "--height=%d", main_app.height);
	arguments[count] = height;
	count++;

	/* The dashboard's picture. */
	snprintf(wallpaper, sizeof(wallpaper), "--wallpaper=%s", main_app.wallpaper);
	arguments[count] = wallpaper;
	count++;

	/* The same display, when this one was given one. */
	if (options->display != NULL) {
		snprintf(display, sizeof(display), "--display=%s", options->display);
		arguments[count] = display;
		count++;
	}

	/* Its log lines named after this window's. */
	if (options->token != NULL) {
		snprintf(token, sizeof(token), "--token=%s-new", options->token);
		arguments[count] = token;
		count++;
	}

	/* The folder, when one is shown. */
	if (folder[0] != '\0') {
		arguments[count] = folder;
		count++;
	}

	/* The end of the arguments. */
	arguments[count] = NULL;

	/* The new window's process. */
	error = fm_apps_spawn(arguments);
	if (error != 0)
		fm_ui_message(&main_app, "A new window can't be opened.");
}

/* Tells the titlebar and the menus the window's state when it changed. */
static void
main_menu_update(void)
{
	struct fm_menu_state state;

	/* The titlebar's state now, sent when it differs from what it shows. */
	fm_ui_titlebar_state(&main_app, &main_titlebar_state);
	fm_titlebar_refresh(&main_titlebar, &main_titlebar_state);

	/* The menus' state now, sent when it differs from what they show (a closed context menu goes, also on the desktop, which has no window's menus). */
	fm_ui_menu_state(&main_app, &state);
	fm_menu_refresh(&main_menu, &state);
}

/*
 * Runs the fingers for one round (ws081-p010): gives them the scrolled
 * areas as they are, takes their events (finding what is under a new
 * finger), sets the scroll they moved to, and hands the pointer's events
 * they made to the file manager.
 */
static void
main_touch_round(void)
{
	struct fm_touch_pointer made;
	struct fm_touch_event *event;
	struct fm_touch_area area;
	struct fm_tab *tab;
	unsigned index;
	unsigned which;
	int scroll;
	int moved;
	int taken;

	/* The items' scroll: the tab's (another tab or folder is another thing shown). */
	tab = fm_ui_tab(&main_app);
	area.token = tab;
	area.scroll = tab->scroll;
	area.largest = main_app.layout.content_height - main_app.layout.content.height;
	area.height = main_app.layout.content.height;
	fm_touch_layout(&main_touch, FM_TOUCH_CONTENT, &area);

	/* The sidebar's. */
	area.token = &main_app.places;
	area.scroll = main_app.sidebar_scroll;
	area.largest = main_app.layout.sidebar_height - main_app.layout.sidebar.height;
	area.height = main_app.layout.sidebar.height;
	fm_touch_layout(&main_touch, FM_TOUCH_SIDEBAR, &area);

	/* The fingers' events, a new finger with what is under it. */
	for (index = 0U; index < main_window.touch_count; index++) {
		event = &main_window.touches[index];
		if (event->type == FM_TOUCH_DOWN)
			event->area = main_touch_area((int)event->x, (int)event->y);
		fm_touch_event(&main_touch, event);
	}

	/* The queue is empty again: the window fills it from the next read. */
	main_window.touch_count = 0U;

	/* Time moves on for them, and they say when they want the next round. */
	main_touch_due = fm_touch_tick(&main_touch, fm_touch_clock());

	/* A new scroll they set is shown in its area. */
	moved = fm_touch_scroll(&main_touch, &which, &scroll);
	if (moved && which == FM_TOUCH_CONTENT) {
		tab->scroll = scroll;
		main_app.dirty = 1;
	} else if (moved && which == FM_TOUCH_SIDEBAR) {
		main_app.sidebar_scroll = scroll;
		main_app.dirty = 1;
	}

	/* The pointer's events they made, in order. */
	for (;;) {
		taken = fm_touch_take_pointer(&main_touch, &made);
		if (!taken)
			break;
		main_touch_pointer(&made);
	}
}

/*
 * Tells what is under a new finger: the items (the content) or the sidebar,
 * which it scrolls, or something else (a button, a tab, a field, a dialog,
 * Quick Look, the information card), which it clicks.
 */
static unsigned
main_touch_area(
	int x,
	int y)
{
	unsigned kind;
	int index;

	/* A dialog, Quick Look or the information card over everything takes the finger as the pointer. */
	if (main_app.dialog != 0U)
		return FM_TOUCH_OTHER;

	/*
	 * The desktop has the gestures everywhere (it does not scroll): a tap
	 * clicks, two make a double click, a long press is the context menu
	 * and a long press that moves drags the items (ws094-p006).
	 */
	if (main_app.desktop)
		return FM_TOUCH_CONTENT;
	if (main_app.quicklook != 0)
		return FM_TOUCH_OTHER;
	if (main_app.info_open != 0)
		return FM_TOUCH_OTHER;

	/* What the last frame drew there. */
	(void)fm_input_hit_at(&main_app, x, y, &kind, &index);

	/* A button, a tab, a header or a region over the window is clicked, not scrolled. */
	switch (kind) {
	case FM_HIT_OVERLAY:
	case FM_HIT_BUTTON:
	case FM_HIT_TAB:
	case FM_HIT_TAB_CLOSE:
	case FM_HIT_SCOPE:
	case FM_HIT_HEADER:
	case FM_HIT_SECTION:
		return FM_TOUCH_OTHER;
	default:
		break;
	}

	/* The sidebar scrolls. */
	if (x >= main_app.layout.sidebar.x &&
	    x < main_app.layout.sidebar.x + main_app.layout.sidebar.width &&
	    y >= main_app.layout.sidebar.y &&
	    y < main_app.layout.sidebar.y + main_app.layout.sidebar.height)
		return FM_TOUCH_SIDEBAR;

	/* The items scroll. */
	if (x >= main_app.layout.content.x &&
	    x < main_app.layout.content.x + main_app.layout.content.width &&
	    y >= main_app.layout.content.y &&
	    y < main_app.layout.content.y + main_app.layout.content.height)
		return FM_TOUCH_CONTENT;

	/* Anything else is clicked. */
	return FM_TOUCH_OTHER;
}

/*
 * Hands one pointer event the fingers made to the file manager: a press
 * carries the touch's serial (a context menu or a drag and drop names it,
 * ws081-p014); a release while zdesktop carries a drag and drop the finger
 * started is not the file manager's.
 */
static void
main_touch_pointer(
	const struct fm_touch_pointer *made)
{
	struct fm_event event;

	/* A release after the finger went to zdesktop's drag is dropped (the drag's end comes as FM_EVENT_DRAG_DONE). */
	if (made->kind == FM_TOUCH_POINTER_RELEASE && main_window.drag_source != NULL)
		return;

	/* The event at the finger's place and time. */
	memset(&event, 0, sizeof(event));
	event.type = FM_EVENT_MOTION;
	event.x = made->x;
	event.y = made->y;
	event.modifiers = main_window.modifiers;
	event.time = made->time;

	/* A press or a release of its button. */
	if (made->kind != FM_TOUCH_POINTER_MOTION) {
		event.type = FM_EVENT_BUTTON;
		event.button = FM_BUTTON_LEFT;
		if (made->button == FM_TOUCH_RIGHT)
			event.button = FM_BUTTON_RIGHT;
		event.pressed = 0;
		if (made->kind == FM_TOUCH_POINTER_PRESS)
			event.pressed = 1;
		event.serial = made->serial;
	}

	/* A press's serial is the window's last press's (context menus and drags name it). */
	if (made->kind == FM_TOUCH_POINTER_PRESS)
		main_window.button_serial = made->serial;

	/* The file manager (or the desktop) takes it. */
	main_window.pointer_x = made->x;
	main_window.pointer_y = made->y;
	main_dispatch(&event);
}

/*
 * Prepares the desktop mode: the token zdesktop gave (KEILAND_DESKTOP_TOKEN)
 * is copied and taken out of the environment, so that no program the
 * desktop starts can take the role, and the folder shown is ~/Desktop
 * (made when it is not there).  Returns 0 or an errno value.
 */
static int
main_desktop_prepare(
	struct main_options *options)
{
	const char *token;
	const char *home;
	int written;
	int error;

	/* The token, copied before the environment's string is freed. */
	token = getenv("KEILAND_DESKTOP_TOKEN");
	if (token == NULL || token[0] == '\0')
		return EINVAL;
	written = snprintf(main_desktop_token, sizeof(main_desktop_token), "%s", token);
	if (written < 0 || (size_t)written >= sizeof(main_desktop_token))
		return ENAMETOOLONG;

	/* Nothing the desktop starts inherits it. */
	(void)unsetenv("KEILAND_DESKTOP_TOKEN");

	/* The folder: the one asked for, else ~/Desktop. */
	if (options->start != NULL)
		return 0;
	home = getenv("HOME");
	if (home == NULL || home[0] == '\0')
		return ENOENT;
	written = snprintf(main_desktop_folder, sizeof(main_desktop_folder), "%s/Desktop", home);
	if (written < 0 || (size_t)written >= sizeof(main_desktop_folder))
		return ENAMETOOLONG;

	/* ~/Desktop, made when it is not there. */
	error = mkdir(main_desktop_folder, 0755);
	if (error != 0 && errno != EEXIST)
		return errno;

	/* Succeeded: the desktop shows ~/Desktop. */
	options->start = main_desktop_folder;
	return 0;
}

/*
 * Opens a window's glass, menus and titlebar.  A window whose menus cannot
 * be made goes on without them; without zdesktop's titlebar the file
 * manager does not start.  Returns 0, or -1 with the three closed.
 */
static int
main_open_decorations(void)
{
	struct fm_menu_state state;
	int error;

	/* Glass when zdesktop can show the window see-through (the frame's ground is then left clear). */
	main_app.glass = fm_glass_open(&main_glass, &main_window, &main_present);

	/* The menus. */
	fm_ui_menu_state(&main_app, &state);
	error = fm_menu_open(&main_menu, &main_window, &state);
	if (error != 0) {
		fm_log("MENU failed errno=%d", error);
		fm_menu_close(&main_menu);
	}

	/* The titlebar's controls. */
	fm_ui_titlebar_state(&main_app, &main_titlebar_state);
	error = fm_titlebar_open(&main_titlebar, &main_window, &main_titlebar_state);
	if (error != 0) {
		fprintf(stderr, "ZFILES FAILED operation=titlebar errno=%d\n", error);
		fm_titlebar_close(&main_titlebar);
		fm_menu_close(&main_menu);
		fm_glass_close(&main_glass);
		return -1;
	}

	/* Succeeded: the window has its glass, menus and titlebar. */
	return 0;
}

/* Gives the desktop the service its context menus open with (ws094-p005); without it a right press shows none. */
static void
main_open_context_menus(void)
{
	struct fm_menu_state state;
	int error;

	/* The service, with no window's menus. */
	fm_ui_menu_state(&main_app, &state);
	error = fm_menu_open(&main_menu, &main_window, &state);
	if (error != 0) {
		fm_log("MENU failed errno=%d", error);
		fm_menu_close(&main_menu);
	}

	/* Succeeded: the desktop menu service is available or its failure logged. */
	return;
}

/* Hands an input to the desktop (files --desktop) or to the window's file manager. */
static void
main_dispatch(
	const struct fm_event *event)
{
	/* The desktop's own input (ui-desktop.c). */
	if (main_app.desktop) {
		fm_desktop_event(&main_app, event);
		return;
	}

	/* The window's. */
	fm_ui_event(&main_app, event);

	/* Succeeded: the window has received its input. */
	return;
}
