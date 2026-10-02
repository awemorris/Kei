/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Text Editor (WS092): plain UTF-8 text in a Wayland window, drawn on the
 * CPU and shown with Vulkan (plan/ws092/design.md).
 *
 *   textedit [--display=NAME] [--font=PATH] [--fallback-font=PATH]
 *            [--ui-font=PATH] [--width=N] [--height=N] [--timeout-s=N]
 *            [FILE]
 *
 * The file is opened from the command line (a path that does not exist is
 * made by the first save), or with File > Open.  The outcome is one line
 * on standard error: TEXTEDIT DONE with the reason, or TEXTEDIT FAILED
 * naming what failed; TEXTEDIT READY says the first frame is shown, and
 * TEXTEDIT OPEN and TEXTEDIT SAVE name the files read and written.
 */

#include "window.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* The fonts used unless told otherwise: the text's (monospaced), the characters it lacks, and the interface's. */
#define MAIN_FONT		KEILAND_DATADIR "/fonts/keiland-mono.ttf"
#define MAIN_FALLBACK_FONT	KEILAND_DATADIR "/fonts/keiland-fallback.ttf"
#define MAIN_UI_FONT		KEILAND_DATADIR "/fonts/keiland.ttf"

/* How often frames are drawn while the fingers or the view's scroll move, in milliseconds. */
#define MAIN_FRAME_MS		16

/* The text view's id among the parts the fingers' input records, and the dialog's (libkeiui's kui_dialog). */
#define MAIN_TEXT_REGION	1U
#define MAIN_DIALOG		2U

/* How far above the card's bottom a message's chip stands. */
#define MAIN_CHIP_BOTTOM	42

/* How many frames in a row may find the swapchain out of date before the program gives up. */
#define MAIN_STALE_LIMIT	8U

/* The longest the loop sleeps when nothing is due, in milliseconds. */
#define MAIN_IDLE_MS		1000

/* The application's identity in the compositor and the recent files. */
#define MAIN_APPLICATION	"textedit"

/* The longest window title. */
#define MAIN_TITLE_MAX		(TE_PATH_MAX + 64)

/*
 * What the command line asked for.
 */
struct main_options {
	const char *display;
	const char *font;
	const char *fallback;
	const char *ui_font;
	const char *file;
	unsigned width;
	unsigned height;
	unsigned timeout;
};

/*
 * The program's parts, for the whole run.  They are file-scope because
 * the window's input queue and the editor are too large for the stack.
 *
 * The window: libkeiui's window (the Wayland connection and surface, the
 * Vulkan presenter, the clipboard and the primary selection) and the
 * editor's queue of inputs, from the start of the run to its end.
 */
static struct te_window main_window;

/* The size frames are drawn at (the presenter's), set when the window opens and when it changes size. */
static uint32_t main_width;
static uint32_t main_height;

/*
 * The fingers' input (libkeiui): which part of the window a finger meant,
 * the text view's touch (one finger selects, two scroll) and the scroll.
 * Made with the window, destroyed before it.
 */
static struct kui_ui *main_input;

/* Whether the fingers or the view's scroll still move (the loop draws the next frame soon). */
static int main_moving;

/* Whether the compositor asked to close the window or gave it a new size, since the loop last looked. */
static int main_closed;
static int main_resized;

/* The editor: the document and the view, made once the swapchain's size is known. */
static struct te_app main_app;

/* The text's font (monospaced) and its fallback, open for the whole run (without them the text has no words). */
static struct te_text main_body;

/* The interface's font and its fallback, open for the whole run (the chips, dialogs and messages). */
static struct te_text main_ui;

/*
 * The window's menus in the compositor, opened with the window and closed
 * before it; absent with a compositor without them.
 */
static struct te_menu main_menu;

/* The window's titlebar controls in the compositor, with the same life as the menus. */
static struct te_titlebar main_titlebar;

/* The window's glass, when zdesktop has glass and the swapchain is see-through. */
static struct te_glass main_glass;

/*
 * The frame being drawn: ordinary memory the size of the swapchain, remade
 * (and the canvas with it) when the window changes size.
 */
static uint32_t *main_pixels;

/* The canvas over main_pixels, which the editor draws each frame into. */
static struct te_canvas main_canvas;

/* libkeiui's canvas over the same pixels, which the text view's handles, a message's chip and a dialog are drawn with (made with main_canvas). */
static struct kui_canvas main_handles;
static int main_handles_made;

/* The interface's font as libkeiui's text, for the chip and the dialog (open when main_widgets_text is 1). */
static struct kui_text main_widgets;
static int main_widgets_text;

/* The title the window shows now, to set it again only when it changes. */
static char main_title[MAIN_TITLE_MAX];

/*
 * The file chooser open for Open or Save As (libkeiui's), or NULL.  It
 * is destroyed when it answers, and by the main loop when the editor stops
 * waiting for it (Quit while it is open).
 */
static struct kui_file_chooser *main_chooser;

/* The interface's font, which the chooser draws its words with too. */
static const char *main_ui_font;

/*
 * The filters the chooser offers: the kinds of files that are plain text,
 * and every file.
 */
static const struct kui_file_filter main_filters[] = {
	{ "Text Files", "txt text md markdown rst c h cc cpp hpp py sh mk conf cfg ini json xml html css js log csv tsv yaml yml toml" },
	{ "All Files", NULL }
};

static int main_parse(int argc, char **argv, struct main_options *options);
static const char *main_value(const char *argument, const char *name);
static int main_number(const char *text, unsigned maximum, unsigned *value);
static int main_loop(const struct main_options *options);
static int main_frame(void);
static void main_overlay(uint64_t now_us);
static int main_canvas_make(void);
static void main_state(struct te_state *state);
static void main_title_refresh(void);
static void main_edit_state(const struct te_state *state);
static void main_opened(void);
static void main_host(struct te_app *app);
static void main_copy(void *data, const char *text, size_t length);
static size_t main_paste(void *data, char *text, size_t size);
static void main_select(void *data, const char *text, size_t length);
static size_t main_paste_primary(void *data, char *text, size_t size);
static void main_context_menu(void *data, int x, int y);
static void main_find_focus(void *data);
static int main_choose(void *data, int saving, const char *folder, const char *name);
static void main_window_event(const struct kui_window_event *event);
static void main_fingers(uint64_t now_us);
static int main_dialog_event(const struct kui_window_event *event);
static int main_resize(void);
static void main_chosen(void *data, struct kui_file_chooser *chooser, unsigned result, const char *path, size_t filter);

/*
 * Runs Text Editor.
 */
int
main(
	int argc,
	char **argv)
{
	struct main_options options;
	struct kui_window_options window_options;
	struct te_state state;
	int status;
	int error;

	/* The command line. */
	status = main_parse(argc, argv, &options);
	if (status != 0) {
		fprintf(stderr, "usage: textedit [--display=NAME] [--font=PATH] [--fallback-font=PATH] [--ui-font=PATH] [--width=N] [--height=N] [--timeout-s=N] [FILE]\n");
		return 2;
	}

	/* The fonts; without them the editor shows no words. */
	error = te_text_open(&main_body, options.font, options.fallback);
	if (error != 0)
		te_log("FONT missing path=%s error=%d", options.font, error);
	error = te_text_open(&main_ui, options.ui_font, options.fallback);
	if (error != 0)
		te_log("FONT missing path=%s error=%d", options.ui_font, error);

	/* The chip and the dialog draw with it too, as libkeiui's text. */
	error = kui_text_open(&main_widgets, options.ui_font, options.fallback);
	if (error == 0)
		main_widgets_text = 1;

	/* The chooser draws with the interface's font. */
	main_ui_font = options.ui_font;

	/* The window, its frames shown with Vulkan. */
	memset(&window_options, 0, sizeof(window_options));
	window_options.display = options.display;
	window_options.title = "Text Editor";
	window_options.application = MAIN_APPLICATION;
	window_options.width = options.width;
	window_options.height = options.height;
	window_options.present = KUI_PRESENT_VULKAN;
	main_window.kui = kui_window_open(&window_options);
	if (main_window.kui == NULL) {
		fprintf(stderr, "TEXTEDIT FAILED operation=window error=%d\n", errno);
		te_text_close(&main_ui);
		te_text_close(&main_body);
		return 1;
	}

	/* The presenter's size, which the frames are drawn at. */
	error = kui_window_present_resize(main_window.kui, &main_width, &main_height);
	if (error != 0) {
		fprintf(stderr, "TEXTEDIT FAILED operation=present error=%d\n", error);
		kui_window_close(main_window.kui);
		te_text_close(&main_ui);
		te_text_close(&main_body);
		return 1;
	}

	/* The editor at that size, with the file when one was given, and the window's services. */
	te_app_init(&main_app, &main_body, &main_ui, (int)main_width, (int)main_height);
	main_host(&main_app);
	main_app.glass = te_glass_open(&main_glass, &main_window, kui_window_see_through(main_window.kui));
	if (options.file != NULL)
		(void)te_app_open(&main_app, options.file);

	/* The fingers' input; without memory for it the fingers do nothing. */
	main_input = kui_ui_create();
	if (main_input == NULL)
		te_log("TOUCH failed errno=%d", ENOMEM);

	/* The menus and the titlebar; a window without them goes on with its keys. */
	main_state(&state);
	error = te_menu_open(&main_menu, &main_window, &state);
	if (error != 0) {
		te_log("MENU failed errno=%d", error);
		te_menu_close(&main_menu);
	}

	/* The titlebar's controls. */
	error = te_titlebar_open(&main_titlebar, &main_window, &state);
	if (error != 0) {
		te_log("TITLEBAR failed errno=%d", error);
		te_titlebar_close(&main_titlebar);
	}

	/* The loop, until the window closes. */
	status = main_loop(&options);

	/* Everything goes, the chooser, the titlebar, the menus and the editor before the window they belong to. */
	kui_file_chooser_destroy(main_chooser);
	main_chooser = NULL;
	te_titlebar_close(&main_titlebar);
	te_menu_close(&main_menu);
	kui_ui_destroy(main_input);
	te_glass_close(&main_glass);
	te_app_release(&main_app);
	if (main_handles_made)
		kui_canvas_release(&main_handles);
	free(main_pixels);
	kui_window_close(main_window.kui);
	te_text_close(&main_ui);
	te_text_close(&main_body);
	if (main_widgets_text)
		kui_text_close(&main_widgets);

	/* Reports how the run ended. */
	if (status != 0)
		return 1;

	/* Succeeded: the window was closed. */
	return 0;
}

/*
 * Writes one line of the log: "TEXTEDIT " and the words, on standard error.
 */
void
te_log(
	const char *format,
	...)
{
	va_list arguments;

	/* The line. */
	va_start(arguments, format);
	fputs("TEXTEDIT ", stderr);
	vfprintf(stderr, format, arguments);
	fputc('\n', stderr);
	va_end(arguments);
}

/*
 * Reports the monotonic clock in milliseconds.
 */
uint64_t
te_clock(void)
{
	struct timespec now;

	/* The monotonic clock. */
	(void)clock_gettime(CLOCK_MONOTONIC, &now);

	/* Reports it in milliseconds. */
	return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
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

	/* The defaults. */
	memset(options, 0, sizeof(*options));
	options->font = MAIN_FONT;
	options->fallback = MAIN_FALLBACK_FONT;
	options->ui_font = MAIN_UI_FONT;
	options->width = TE_WIDTH;
	options->height = TE_HEIGHT;

	/* Each argument. */
	for (index = 1; index < argc; index++) {
		/* The compositor's display. */
		value = main_value(argv[index], "--display=");
		if (value != NULL) {
			options->display = value;
			continue;
		}

		/* The text's font. */
		value = main_value(argv[index], "--font=");
		if (value != NULL) {
			options->font = value;
			continue;
		}

		/* The font of the characters the others lack. */
		value = main_value(argv[index], "--fallback-font=");
		if (value != NULL) {
			options->fallback = value;
			continue;
		}

		/* The interface's font. */
		value = main_value(argv[index], "--ui-font=");
		if (value != NULL) {
			options->ui_font = value;
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

		/* The file to open, once. */
		if (options->file != NULL)
			return -1;
		options->file = argv[index];
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
	struct kui_window_event window_event;
	struct te_event event;
	struct te_state state;
	uint64_t started;
	uint64_t now;
	int taken;
	int status;
	int timeout;
	int due;

	/* The first frame's canvas and the first frame. */
	status = main_canvas_make();
	if (status != 0) {
		fprintf(stderr, "TEXTEDIT FAILED operation=canvas\n");
		return -1;
	}

	/* The file given on the command line, the title, and the first frame. */
	main_app.now = te_clock();
	main_opened();
	main_title_refresh();
	status = main_frame();
	if (status != 0)
		return -1;
	te_log("READY width=%u height=%u lines=%lu", main_width, main_height, (unsigned long)main_app.buffer.line_count);

	/* Each round: input, time, and a frame when something changed. */
	started = te_clock();
	for (;;) {
		/* Waits for the compositor, or until something is due (the fingers and the view's scroll soon while they move). */
		now = te_clock();
		timeout = MAIN_IDLE_MS;
		due = te_app_tick(&main_app, now);
		if (due >= 0 && due < timeout)
			timeout = due;
		due = kui_window_repeat_wait(main_window.kui, kui_clock_us());
		if (due >= 0 && due < timeout)
			timeout = due;
		if (main_moving && timeout > MAIN_FRAME_MS)
			timeout = MAIN_FRAME_MS;
		if (main_app.dirty)
			timeout = 0;

		/* Waits; a lost connection ends the run. */
		status = kui_window_dispatch(main_window.kui, timeout);
		if (status != 0) {
			te_log("DONE reason=disconnected");
			return 0;
		}

		/* A key held repeats once the compositor's input is in, so that its release is seen first (BUG-111). */
		now = te_clock();
		main_app.now = now;
		(void)kui_window_repeat(main_window.kui, kui_clock_us());

		/* The window's input: the pointer, the keys and the focus become the editor's, the fingers go to libkeiui. */
		for (;;) {
			taken = kui_window_take(main_window.kui, &window_event);
			if (taken == 0)
				break;
			main_window_event(&window_event);
		}

		/* Every input queued (the menus' and the titlebar's among them). */
		for (;;) {
			taken = te_window_take(&main_window, &event);
			if (taken == 0)
				break;
			te_app_event(&main_app, &event);
		}

		/* The text input is asked for where the text is edited: not under a dialog or the chooser. */
		kui_window_text_input(main_window.kui, main_app.dialog == TE_DIALOG_NONE && !main_app.choosing);

		/* The fingers at this time: their selection, taps and menus, and the view's scroll. */
		main_fingers(kui_clock_us());

		/* A chooser the editor no longer waits for (Quit came meanwhile) closes. */
		if (main_chooser != NULL && !main_app.choosing) {
			kui_file_chooser_destroy(main_chooser);
			main_chooser = NULL;
		}

		/* The close button asks like File > Close (unsaved changes are asked about). */
		if (main_closed != 0) {
			main_closed = 0;
			te_app_action(&main_app, TE_ACTION_CLOSE);
		}

		/* A selection made becomes the primary one; a file opened joins the recent files; the title follows. */
		te_app_publish_primary(&main_app);
		main_opened();
		main_title_refresh();

		/* Time passes for the editor; the menus and the titlebar show its state. */
		(void)te_app_tick(&main_app, now);
		main_state(&state);
		main_edit_state(&state);
		te_menu_refresh(&main_menu, &state);
		te_titlebar_refresh(&main_titlebar, &state);

		/* Close (with nothing unsaved, or dropped) ends the run. */
		if (main_app.want_close != 0) {
			te_log("DONE reason=close");
			return 0;
		}

		/* So does the timeout, when one was given. */
		if (options->timeout != 0U && now - started >= (uint64_t)options->timeout * 1000U) {
			te_log("DONE reason=timeout");
			return 0;
		}

		/* A new size: a new swapchain and canvas, and a frame. */
		if (main_resized != 0) {
			main_resized = 0;
			status = main_resize();
			if (status != 0)
				return -1;
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
	struct kui_rect clip;
	struct te_rect caret;
	struct te_rect text;
	unsigned stale;
	int status;

	/* Tries until the frame is shown, remaking a stale swapchain a few times. */
	for (stale = 0; stale < MAIN_STALE_LIMIT; stale++) {
		/* The frame on the CPU, and the fingers' handles over the text (within it, and a knob's size around it). */
		te_draw(&main_app, &main_canvas);
		te_app_text_rect(&main_app, &text);
		clip.x = text.x - KUI_TEXT_HANDLE;
		clip.y = text.y;
		clip.width = text.width + 2 * KUI_TEXT_HANDLE;
		clip.height = text.height + KUI_TEXT_HANDLE;
		kui_canvas_clip_push(&main_handles, &clip);
		kui_text_touch_draw_handles(&main_app.touch, &main_handles, (double)text.x - main_app.scroll_x, (double)text.y - main_app.scroll_y, kui_theme_default());
		kui_canvas_clip_pop(&main_handles);

		/* A message's chip and a dialog over it all. */
		main_overlay(kui_clock_us());

		/* Its glass card, and the frame shown in the window. */
		te_glass_refresh(&main_glass, &main_app);
		status = kui_window_present(main_window.kui, main_pixels, (size_t)main_width);
		if (status == 0) {
			te_app_caret_rect(&main_app, &caret);
			kui_window_text_cursor(main_window.kui, caret.x, caret.y, caret.width, caret.height);
			return 0;
		}

		/* Anything but a stale swapchain is a failure. */
		if (status != EAGAIN) {
			fprintf(stderr, "TEXTEDIT FAILED operation=present error=%d\n", status);
			return -1;
		}

		/* A stale swapchain is remade at the window's size, with a canvas to match. */
		status = main_resize();
		if (status != 0)
			return -1;
	}

	/* The swapchain stayed out of date. */
	fprintf(stderr, "TEXTEDIT FAILED operation=stale-swapchain\n");
	return -1;
}

/* Draws a message's chip and the dialog shown (libkeiui's), and carries out the dialog's answer. */
static void
main_overlay(
	uint64_t now_us)
{
	struct kui_style style;
	struct kui_event event;
	struct kui_rect area;
	struct te_rect card;
	const char *const *labels;
	const char *words;
	char title[TE_PATH_MAX + 64];
	int answer;
	int count;
	int taken;

	/* Without the interface's text, the words cannot be drawn. */
	if (!main_widgets_text)
		return;

	/* The widgets draw over the frame, on the window's card. */
	style.canvas = &main_handles;
	style.text = &main_widgets;
	style.theme = kui_theme_default();
	style.glass = main_app.glass;
	te_app_card(&main_app, &card);
	area.x = card.x;
	area.y = card.y;
	area.width = card.width;
	area.height = card.height;

	/* A message, at the bottom middle of the card. */
	if (main_app.message[0] != '\0')
		kui_chip(&style, card.x + card.width / 2, card.y + card.height - MAIN_CHIP_BOTTOM, main_app.message);

	/* The dialog, a frame of the fingers' and the pointer's input of its own. */
	if (main_app.dialog == TE_DIALOG_NONE || main_input == NULL)
		return;
	te_app_dialog_words(&main_app, title, sizeof(title), &words, &labels, &count);
	kui_ui_begin(main_input, now_us);
	answer = kui_dialog(main_input, &style, MAIN_DIALOG, &area, title, words, labels, count);
	main_moving = kui_ui_end(main_input, now_us);

	/* What no part took under a dialog is nothing. */
	for (;;) {
		taken = kui_ui_take(main_input, &event);
		if (taken == 0)
			break;
	}

	/* The answer: the dialog closes and its button is carried out (the next frame shows it). */
	if (answer >= 0)
		te_app_dialog_choose(&main_app, answer);
}

/* Remakes the presenter at the window's size, with a canvas to match; nonzero when it cannot. */
static int
main_resize(void)
{
	int status;

	/* The presenter at the window's size. */
	status = kui_window_present_resize(main_window.kui, &main_width, &main_height);
	if (status != 0) {
		fprintf(stderr, "TEXTEDIT FAILED operation=present error=%d\n", status);
		return -1;
	}

	/* A canvas of the presenter's size. */
	status = main_canvas_make();
	if (status != 0)
		return -1;
	te_app_resize(&main_app, (int)main_width, (int)main_height);

	/* Succeeded: frames are drawn at the new size. */
	return 0;
}

/* Makes the frame's memory and canvas at the swapchain's size; nonzero when memory runs out. */
static int
main_canvas_make(void)
{
	uint32_t *pixels;
	size_t count;
	int status;

	/* The frame's memory. */
	count = (size_t)main_width * (size_t)main_height;
	pixels = malloc(count * sizeof(pixels[0]));
	if (pixels == NULL)
		return -1;
	free(main_pixels);
	main_pixels = pixels;

	/* The canvas over it. */
	main_canvas.pixels = main_pixels;
	main_canvas.stride = main_width;
	main_canvas.width = (int)main_width;
	main_canvas.height = (int)main_height;
	te_canvas_unclip(&main_canvas);
	main_app.dirty = 1;

	/* libkeiui's canvas over the same pixels, for the handles. */
	if (main_handles_made)
		kui_canvas_release(&main_handles);
	main_handles_made = 0;
	status = kui_canvas_init(&main_handles, main_pixels, (size_t)main_width, (int)main_width, (int)main_height);
	if (status != 0)
		return -1;
	main_handles_made = 1;

	/* Succeeded: frames can be drawn. */
	return 0;
}

/* Gathers what the menus and the titlebar show. */
static void
main_state(
	struct te_state *state)
{
	size_t start;
	size_t end;

	/* A clean state, so that states compare by their bytes. */
	memset(state, 0, sizeof(*state));
	state->can_undo = te_undo_can_undo(&main_app.undo);
	state->can_redo = te_undo_can_redo(&main_app.undo);
	te_edit_selection(&main_app, &start, &end);
	state->selected = 0;
	if (end > start)
		state->selected = 1;
	state->modified = te_app_modified(&main_app);
	state->line_numbers = main_app.line_numbers;
	state->wrap = main_app.wrap;
}

/*
 * Tells the window's editing state -- a selection, something to paste,
 * something to undo or redo -- which the on-screen keyboard's editing
 * buttons follow (KUI_VERSION 8, ws102-p023); the library sends it only
 * when it changed.
 */
static void
main_edit_state(
	const struct te_state *state)
{
	unsigned flags;
	int paste;

	/* The state's bits. */
	flags = 0U;
	if (state->selected)
		flags |= KUI_EDIT_HAS_SELECTION;
	if (state->can_undo)
		flags |= KUI_EDIT_CAN_UNDO;
	if (state->can_redo)
		flags |= KUI_EDIT_CAN_REDO;
	paste = kui_window_can_paste(main_window.kui);
	if (paste)
		flags |= KUI_EDIT_CAN_PASTE;

	/* Succeeded: the window tells it before its next wait. */
	kui_window_edit_state(main_window.kui, flags);
}

/* Sets the window's title when it changed: "• " for unsaved changes, the document's name and the application's. */
static void
main_title_refresh(void)
{
	char title[MAIN_TITLE_MAX];
	const char *mark;
	int modified;
	int same;

	/* The title as it should be. */
	modified = te_app_modified(&main_app);
	mark = "";
	if (modified)
		mark = "\xe2\x80\xa2 ";
	snprintf(title, sizeof(title), "%s%s \xe2\x80\x94 Text Editor", mark, te_app_name(&main_app));

	/* Sent only when it differs. */
	same = strcmp(title, main_title);
	if (same == 0)
		return;
	snprintf(main_title, sizeof(main_title), "%s", title);
	kui_window_set_title(main_window.kui, main_title);
	te_log("TITLE %s", main_title);
}

/* After a file was opened or saved: it joins the recent files. */
static void
main_opened(void)
{
	char resolved[PATH_MAX];
	char *absolute;
	int error;

	/* Only once for each file. */
	if (!main_app.opened)
		return;
	main_app.opened = 0;

	/* The recent files, by the absolute path. */
	absolute = realpath(main_app.path, resolved);
	if (absolute == NULL)
		return;
	error = keiland_recent_add(resolved, MAIN_APPLICATION);
	if (error != 0)
		te_log("RECENT failed errno=%d", error);
}

/* Gives the editor the window's services. */
static void
main_host(
	struct te_app *app)
{
	/* The clipboard, the primary selection, the context menu and the find field. */
	memset(&app->host, 0, sizeof(app->host));
	app->host.data = &main_window;
	app->host.copy = main_copy;
	app->host.paste = main_paste;
	app->host.select = main_select;
	app->host.paste_primary = main_paste_primary;
	app->host.context_menu = main_context_menu;
	app->host.find_focus = main_find_focus;
	app->host.choose = main_choose;
}

/* Copies text to the clipboard. */
static void
main_copy(
	void *data,
	const char *text,
	size_t length)
{
	struct te_window *window;

	/* The window's clipboard. */
	window = data;
	kui_window_copy(window->kui, text, length);
	te_log("CLIPBOARD set bytes=%lu", (unsigned long)length);
}

/* Pastes the clipboard's text into a buffer; reports its length. */
static size_t
main_paste(
	void *data,
	char *text,
	size_t size)
{
	struct te_window *window;
	size_t length;

	/* The window's clipboard. */
	window = data;
	length = kui_window_paste(window->kui, text, size);
	te_log("CLIPBOARD paste received bytes=%lu", (unsigned long)length);

	/* Succeeded: the length received. */
	return length;
}

/* Makes text the primary selection. */
static void
main_select(
	void *data,
	const char *text,
	size_t length)
{
	struct te_window *window;

	/* The window's primary selection. */
	window = data;
	kui_window_select(window->kui, text, length);
	te_log("PRIMARY set bytes=%lu", (unsigned long)length);
}

/* Pastes the primary selection's text into a buffer; reports its length. */
static size_t
main_paste_primary(
	void *data,
	char *text,
	size_t size)
{
	struct te_window *window;
	size_t length;

	/* The window's primary selection. */
	window = data;
	length = kui_window_paste_primary(window->kui, text, size);
	te_log("PRIMARY paste bytes=%lu", (unsigned long)length);

	/* Succeeded: the length received. */
	return length;
}

/* Opens the context menu at a place. */
static void
main_context_menu(
	void *data,
	int x,
	int y)
{
	/* The menus' context menu (the window is the only one). */
	(void)data;
	te_menu_popup(&main_menu, x, y);
}

/* Gives the titlebar's find field the keyboard (with the next refresh). */
static void
main_find_focus(
	void *data)
{
	/* Asked of the titlebar. */
	(void)data;
	main_titlebar.want_focus = 1;
}

/*
 * Opens the file chooser to open a file or to save as a name in a folder;
 * its answer comes back as a TE_EVENT_CHOSEN.  Returns 0 or an errno value.
 */
static int
main_choose(
	void *data,
	int saving,
	const char *folder,
	const char *name)
{
	struct kui_file_chooser_options options;
	static const struct kui_file_chooser_listener listener = {
		main_chosen
	};
	struct te_window *window;

	/* A chooser left open goes first (one at a time). */
	window = data;
	kui_file_chooser_destroy(main_chooser);
	main_chooser = NULL;

	/* Open or Save As, at the document's folder, with the text files shown first. */
	memset(&options, 0, sizeof(options));
	options.mode = KUI_FILE_CHOOSER_OPEN;
	if (saving) {
		options.mode = KUI_FILE_CHOOSER_SAVE;
		options.name = name;
	}

	/* The editor's mark on the chooser, its folder, its filters and the interface's font. */
	options.application = MAIN_APPLICATION;
	options.folder = folder;
	options.filters = main_filters;
	options.filter_count = sizeof(main_filters) / sizeof(main_filters[0]);
	options.filter = 0;
	options.font = main_ui_font;

	/* The chooser's window over the editor's. */
	main_chooser = kui_file_chooser_open(kui_window_display(window->kui), kui_window_toplevel(window->kui), &options, &listener, window);
	if (main_chooser == NULL)
		return errno;

	/* Succeeded: the answer comes while the loop dispatches. */
	return 0;
}

/* The chooser answered: the path (empty when cancelled) goes to the editor, and the chooser goes. */
static void
main_chosen(
	void *data,
	struct kui_file_chooser *chooser,
	unsigned result,
	const char *path,
	size_t filter)
{
	struct te_event *event;

	/* The answer as an input of the editor. */
	(void)filter;
	event = te_window_push(data, TE_EVENT_CHOSEN);
	if (event != NULL) {
		event->text[0] = '\0';
		if (result == KUI_FILE_CHOOSER_CHOSEN)
			snprintf(event->text, sizeof(event->text), "%s", path);
	}

	/* The log names the answer. */
	te_log("CHOSEN result=%u path=%s", result, path);

	/* The chooser is spent. */
	kui_file_chooser_destroy(chooser);
	if (chooser == main_chooser)
		main_chooser = NULL;
}

/*
 * Takes one input of the window: the pointer, the keys and the focus
 * become the editor's inputs, the fingers go to the fingers' input, and a
 * new size or the close button is noted for the loop.
 */
static void
main_window_event(
	const struct kui_window_event *event)
{
	struct te_event *input;
	int main_dialog_input;

	/* Every input carries the pointer's place and the modifiers (the same bits as the editor's). */
	main_window.pointer_x = (int)event->x;
	main_window.pointer_y = (int)event->y;
	main_window.modifiers = event->modifiers;

	/* A dialog takes the pointer and the keys (libkeiui's). */
	main_dialog_input = 0;
	if (main_app.dialog != TE_DIALOG_NONE && main_input != NULL)
		main_dialog_input = main_dialog_event(event);
	if (main_dialog_input)
		return;

	/* What it is. */
	switch (event->kind) {
	case KUI_WINDOW_MOTION:
		(void)te_window_push(&main_window, TE_EVENT_MOTION);
		break;
	case KUI_WINDOW_LEAVE:
		(void)te_window_push(&main_window, TE_EVENT_LEAVE);
		break;
	case KUI_WINDOW_BUTTON:
		/* The button and whether it went down. */
		input = te_window_push(&main_window, TE_EVENT_BUTTON);
		if (input == NULL)
			break;
		input->button = event->code;
		input->pressed = event->pressed;
		break;
	case KUI_WINDOW_AXIS:
		/* The wheel's distance down and across. */
		input = te_window_push(&main_window, TE_EVENT_AXIS);
		if (input == NULL)
			break;
		input->scroll = (int)event->dy;
		input->scroll_x = (int)event->dx;
		break;
	case KUI_WINDOW_KEY:
		/* The key and whether it went down. */
		input = te_window_push(&main_window, TE_EVENT_KEY);
		if (input == NULL)
			break;
		input->key = event->code;
		input->pressed = event->pressed;
		break;
	case KUI_WINDOW_TEXT_COMMIT:
		/* Text from an input method or the on-screen keyboard. */
		te_log("TEXT input commit=%s", event->text);
		input = te_window_push(&main_window, TE_EVENT_TEXT);
		if (input != NULL)
			snprintf(input->text, sizeof(input->text), "%s", event->text);
		break;
	case KUI_WINDOW_TEXT_DELETE:
		/* Bytes around the caret it replaces. */
		input = te_window_push(&main_window, TE_EVENT_TEXT_DELETE);
		if (input == NULL)
			break;
		input->key = event->before;
		input->button = event->after;
		break;
	case KUI_WINDOW_TEXT_PREEDIT:
		/* The text being composed, drawn in the body at the cursor (draw.c), and its segment or caret. */
		te_log("TEXT input preedit=%s begin=%d end=%d", event->text, (int)event->begin, (int)event->end);
		snprintf(main_app.preedit, sizeof(main_app.preedit), "%s", event->text);
		main_app.preedit_begin = event->begin;
		main_app.preedit_end = event->end;
		main_app.dirty = 1;
		break;
	case KUI_WINDOW_FOCUS:
		input = te_window_push(&main_window, TE_EVENT_FOCUS);
		if (input != NULL)
			input->pressed = event->pressed;
		break;
	case KUI_WINDOW_TOUCH_DOWN:
		te_log("TOUCH down id=%d x=%.0f y=%.0f", (int)event->id, event->x, event->y);
		if (main_input != NULL)
			(void)kui_ui_touch_down(main_input, event->id, event->time_us, event->arrival_us, event->x, event->y);
		break;
	case KUI_WINDOW_TOUCH_MOTION:
		if (main_input != NULL)
			(void)kui_ui_touch_motion(main_input, event->id, event->time_us, event->arrival_us, event->x, event->y);
		break;
	case KUI_WINDOW_TOUCH_UP:
		te_log("TOUCH up id=%d", (int)event->id);
		if (main_input != NULL)
			(void)kui_ui_touch_up(main_input, event->id, event->time_us, event->arrival_us);
		break;
	case KUI_WINDOW_TOUCH_CANCEL:
		if (main_input != NULL)
			(void)kui_ui_touch_cancel(main_input, event->arrival_us);
		break;
	case KUI_WINDOW_RESIZE:
		main_resized = 1;
		break;
	case KUI_WINDOW_CLOSE:
		main_closed = 1;
		break;
	case KUI_WINDOW_POST:
		/* An action of the menus or the titlebar, in its place among the keys. */
		te_window_act(&main_window, event->code);
		break;
	default:
		break;
	}
}

/* Gives the pointer's and the keys' input to a dialog shown; 1 when it took it. */
static int
main_dialog_event(
	const struct kui_window_event *event)
{
	/* What it is. */
	switch (event->kind) {
	case KUI_WINDOW_MOTION:
		(void)kui_ui_pointer_motion(main_input, event->x, event->y);
		break;
	case KUI_WINDOW_LEAVE:
		(void)kui_ui_pointer_leave(main_input);
		break;
	case KUI_WINDOW_BUTTON:
		/* The main button only. */
		(void)kui_ui_pointer_motion(main_input, event->x, event->y);
		if (event->code == KUI_BUTTON_LEFT)
			(void)kui_ui_pointer_button(main_input, event->pressed, event->arrival_us);
		break;
	case KUI_WINDOW_KEY:
		(void)kui_ui_key(main_input, event->code, event->pressed, event->modifiers);
		break;
	case KUI_WINDOW_AXIS:
		break;
	default:
		return 0;
	}

	/* Succeeded: the dialog is drawn again with it. */
	main_app.dirty = 1;
	return 1;
}

/*
 * Moves the fingers' input on to a time: the text view is recorded (unless
 * a dialog or the chooser covers it), the fingers' selection and context
 * menu reach the editor, a tap elsewhere (a dialog's button) is a click,
 * and the view is drawn where its scroll has it.
 */
static void
main_fingers(
	uint64_t now_us)
{
	struct kui_event event;
	struct kui_rect region;
	struct te_rect text;
	int taken;

	/* Without the fingers' input, only the view's scroll moves. */
	main_moving = 0;
	if (main_input == NULL) {
		main_moving = te_app_sync_scroll(&main_app, now_us);
		return;
	}

	/* A dialog records its own frame of the input (main_overlay). */
	if (main_app.dialog != TE_DIALOG_NONE)
		return;

	/* The frame of the fingers' input: the text view, when nothing covers it. */
	kui_ui_begin(main_input, now_us);
	if (main_app.dialog == TE_DIALOG_NONE && !main_app.choosing) {
		te_app_text_rect(&main_app, &text);
		region.x = text.x;
		region.y = text.y;
		region.width = text.width;
		region.height = text.height;
		kui_ui_text_region(main_input, MAIN_TEXT_REGION, &region, &main_app.scroll, &main_app.touch);
	}

	/* The frame is recorded; whether something still moves. */
	main_moving = kui_ui_end(main_input, now_us);

	/* The fingers' selection and context menu. */
	te_app_touch(&main_app);

	/* What no part took: a tap is a click (a dialog's button), a long press elsewhere asks for the menu. */
	for (;;) {
		taken = kui_ui_take(main_input, &event);
		if (taken == 0)
			break;
		if (event.kind == KUI_EVENT_TAP) {
			te_log("TOUCH tap x=%.0f y=%.0f", event.x, event.y);
			te_app_tap(&main_app, (int)event.x, (int)event.y, 1);
		} else if (event.kind == KUI_EVENT_DOUBLE_TAP) {
			te_app_tap(&main_app, (int)event.x, (int)event.y, 2);
		}
	}

	/* The view where its scroll has it (a new place is drawn, the handles with it). */
	(void)te_app_sync_scroll(&main_app, now_us);
}
