/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Notes, the handwritten notebook (plan/ws079/design-input-notes.md
 * section 5).
 *
 *   notes [--fullscreen] [--width=W] [--height=H] [--timeout-s=S] [FILE.pdf]
 *
 * One process shows one notebook of A4 pages.  The pointer (and, once the
 * compositor offers the tablet protocol, a pen) draws strokes with the
 * pen, a translucent highlighter, or erases whole strokes; Undo and Redo
 * take changes back and make them again; pages are added and turned.
 *
 * The notebook is saved as a PDF (save.c) when asked (Ctrl+S, the toolbar,
 * the menu), on its own NOTES_AUTOSAVE_IDLE_MS (5 seconds) after the last
 * change once no stroke is being drawn, and when Notes closes.  Every
 * change is also written to a journal before it is shown (journal.c), so
 * a crash or a killed process loses nothing: the next start recovers the
 * notebook from the journal -- the journal of FILE when one is given, or
 * the most recent journal when none is.  A new notebook is saved as
 * ~/Documents/Notes/note-YYYYMMDD-HHMMSS.pdf.
 *
 * Ctrl+N adds a page after the current one (one process is one notebook,
 * so a new page is what "new" makes).  FILE is opened from the edit data
 * its PDF carries (save.c).  Another program's PDF -- or a notebook whose
 * pages another program changed -- is written on: its pages are drawn under
 * the strokes (libpdf draws them into the background picture) and each
 * save adds the strokes to the file as a new revision, leaving its own
 * bytes as they were.  An encrypted or signed PDF is left as it is and a
 * new notebook starts.  Ctrl+O opens another PDF with libkeiui's file
 * chooser (the notebook shown is saved first), and Ctrl+Shift+S saves the
 * notebook as another file, which Notes goes on writing (ws128-p002).
 *
 * ws081-p013: the fingers do not write.  One finger scrolls a page zoomed
 * past the window (with inertia), two fingers zoom, a double tap zooms in
 * or back to the whole page, and a tap on the toolbar presses its button;
 * a palm near the pen is left alone (touch.c).  ws081-p015: while the
 * toolbar's Finger is on, one finger writes as the pointer does and two
 * fingers scroll and zoom.
 *
 * --timeout-s ends Notes after that many seconds as if it were closed (the
 * tests use it to bound a run).  The lines starting with "NOTES" on the
 * standard output are what the tests read.
 */

#include "app.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The window's size when the compositor leaves it to Notes. */
#define MAIN_WIDTH		1024U
#define MAIN_HEIGHT		768U

/* The font the toolbar's labels are drawn with. */
#define MAIN_FONT		KEILAND_DATADIR "/fonts/keiland.ttf"

/* The longest path Notes keeps. */
#define MAIN_PATH_MAX		4096U

/* The app_id the file chooser's window gets, so that zdesktop shows it as Notes'. */
#define MAIN_APPLICATION	"notes"

/* The eraser's radius, in points. */
#define MAIN_ERASER_RADIUS	10.0f

/* How close two samples may be before the second is dropped, in pixels. */
#define MAIN_SAMPLE_STEP	0.25f

/* How long a status stays on the toolbar, in milliseconds. */
#define MAIN_STATUS_MS		4000U

/* How long what Notes found when it opened a PDF (written on, refused) stays, in milliseconds. */
#define MAIN_NOTICE_MS		10000U

/* The evdev codes of the keys Notes handles itself. */
#define MAIN_KEY_ESC		1U
#define MAIN_KEY_W		17U
#define MAIN_KEY_E		18U
#define MAIN_KEY_Y		21U
#define MAIN_KEY_O		24U
#define MAIN_KEY_P		25U
#define MAIN_KEY_S		31U
#define MAIN_KEY_Z		44U
#define MAIN_KEY_N		49U
#define MAIN_KEY_M		50U
#define MAIN_KEY_F11		87U
#define MAIN_KEY_PAGE_UP	104U
#define MAIN_KEY_PAGE_DOWN	109U

/* The points of a circle the pen's mark is drawn with. */
#define MAIN_CIRCLE_POINTS	40U

/*
 * The desk around the page: a soft wash like Kei's blurred landscape --
 * pale sky at the top, a light haze a little below the middle, pale leaf
 * green at the bottom -- and the page's slate shadow: its
 * colour, its darkest alpha, how many pixels it fades over and how far it
 * drops below the page.
 */
#define MAIN_DESK_TOP		0xd9e6f5ffU
#define MAIN_DESK_HAZE		0xeef3f6ffU
#define MAIN_DESK_BOTTOM	0xdfecd6ffU
#define MAIN_DESK_HAZE_SHARE	0.58f
#define MAIN_SHADOW_COLOR	0x1f3a6600U
#define MAIN_SHADOW_ALPHA	44
#define MAIN_SHADOW_RINGS	12
#define MAIN_SHADOW_DROP	3.0f

/* The colours of the pen's mark: its white halo, the slate of the eraser's ring and its fill. */
#define MAIN_MARK_HALO		0xffffffd8U
#define MAIN_MARK_RING		0x33415599U
#define MAIN_MARK_FILL		0x3341551cU

/* What the current contact is doing. */
#define MAIN_CONTACT_NONE	0U
#define MAIN_CONTACT_TOOLBAR	1U
#define MAIN_CONTACT_DRAW	2U
#define MAIN_CONTACT_ERASE	3U

/*
 * Everything Notes holds for the one notebook it shows.
 */
struct notes_app {
	/* The window, its drawing, the toolbar, the frame being built and the page frame (below). */
	struct notes_window window;
	struct notes_renderer renderer;
	struct notes_ui ui;
	struct notes_frame frame;
	struct notes_frame page_frame;
	struct notes_view view;

	/*
	 * The fingers (touch.c): the gestures, the zoom and the scroll; when the
	 * page's place is due again (milliseconds, -1: not), the page it was
	 * last placed for (another page starts at its top), the place last
	 * logged for the tests, and whether the last frame was drawn while two
	 * fingers zoomed.
	 */
	struct notes_touch touch;
	int touch_due;
	const struct notes_page *touch_page;
	struct notes_view logged_view;
	int drawn_zooming;

	/*
	 * Writing with a finger (ws081-p015): whether it is on (the toolbar's
	 * Finger), and whether the contact under way is a writing finger's (its
	 * later events belong to it only while it is; the pointer or the pen
	 * taking the contact over clears it).
	 */
	int finger_write;
	int finger_contact;

	/*
	 * The page's picture (render.c) as the last frame left it: the page it
	 * shows, the renderer's serial of the picture, the document's reshaped
	 * count and the scale when it was drawn, and how many of the page's
	 * strokes it holds, from the bottom.  A frame adds the strokes put on
	 * top since, or draws the page again from the start when anything else
	 * changed.
	 */
	const struct notes_page *picture_page;
	unsigned long picture_serial;
	uint64_t picture_reshaped;
	float picture_scale;
	size_t picture_strokes;

	/*
	 * The background of a page of the PDF the notebook writes on: the
	 * drawing libpdf made of one page of the base (NULL: none yet), which
	 * page it is, and whether drawing it failed; and the page, the scale and
	 * the size the renderer's background picture was last drawn for (it is
	 * drawn again when any of them changes).
	 */
	struct pdf_display_list *background_list;
	size_t background_source;
	int background_failed;
	const struct notes_page *background_page;
	float background_scale;
	uint32_t background_width;
	uint32_t background_height;

	/* The pen over the window: whether it is there, its place (surface pixels) and its source (NOTES_SOURCE_*). */
	int hover;
	float hover_x;
	float hover_y;
	unsigned hover_source;

	/* The notebook, its file and the name shown in the title. */
	struct notes_document document;
	char path[MAIN_PATH_MAX];
	const char *name;

	/*
	 * The page shown, the tool (its NOTES_ACTION_*), the colour's and the
	 * width's index, and whether the eraser (the tool, and a pen's eraser
	 * end) cuts parts of strokes rather than removing whole ones.
	 */
	size_t page;
	unsigned tool;
	unsigned color;
	unsigned width;
	int erase_parts;

	/* The contact under way, the stroke it draws (off the page until it ends), and when it started. */
	unsigned contact;
	struct notes_stroke *live;
	uint32_t live_start;

	/* When the notebook last changed (monotonic milliseconds). */
	uint64_t changed_at;

	/* The status on the toolbar, and until when (0: none). */
	char status[160];
	uint64_t status_until;

	/* Whether the frame and the toolbar need drawing again, and the width whose buttons were logged. */
	int redraw;
	int toolbar_dirty;
	uint32_t buttons_logged;
	int drawn;

	/* When Notes ends by itself (0: never), and whether it is ending. */
	uint64_t deadline;
	int quit;

	/*
	 * File > Open and Save As (ws128-p002): libkeiui's file chooser while it
	 * is shown (NULL otherwise) and its mode, and the path it answered with,
	 * kept until the main loop carries it out (ready says it waits; an empty
	 * path is a cancel).
	 */
	struct kui_file_chooser *chooser;
	unsigned chooser_mode;
	char chosen[MAIN_PATH_MAX];
	unsigned chosen_mode;
	int chosen_ready;

	/*
	 * The frames drawn since the last NOTES FRAMES line: how many, and the
	 * sum and the longest of the time spent building their geometry and
	 * drawing them (microseconds).  The line goes out when a contact ends,
	 * so it tells what drawing a stroke costs a frame.
	 */
	unsigned long frame_count;
	uint64_t frame_build_us;
	uint64_t frame_draw_us;
	uint64_t frame_longest_us;
};

static int app_start_document(struct notes_app *app, const char *file);
static const char *app_opened_name(unsigned opened);
static int app_background(struct notes_app *app, const struct notes_page *page, float scale);
static int app_new_path(char *path, size_t size);
static int app_make_folders(const char *path);
static void app_set_title(struct notes_app *app);
static void app_status(struct notes_app *app, const char *text);
static void app_state(const struct notes_app *app, struct notes_ui_state *state);
static void app_action(struct notes_app *app, uint32_t action);
static void app_key(struct notes_app *app, const struct notes_key *key);
static void app_input(struct notes_app *app, const struct notes_input *input);
static void app_touch(struct notes_app *app);
static void app_finger(struct notes_app *app);
static void app_abort_contact(struct notes_app *app);
static void app_hover(struct notes_app *app, const struct notes_input *input);
static void app_sample(struct notes_app *app, const struct notes_input *input);
static void app_end_contact(struct notes_app *app, const struct notes_input *input);
static void app_changed(struct notes_app *app);
static int app_save(struct notes_app *app, const char *reason);
static void app_draw(struct notes_app *app);
static struct notes_frame *app_page_frame(struct notes_app *app, const struct notes_page *page, float scale, int *clear);
static void app_desk(struct notes_app *app, const struct notes_view *view, float width, float height);
static void app_mark(struct notes_app *app, const struct notes_view *view, int over_toolbar);
static void app_circle(struct notes_frame *frame, float cx, float cy, float radius, uint32_t color);
static void app_ring(struct notes_frame *frame, float cx, float cy, float radius, float thickness, uint32_t color);
static void app_frame_report(struct notes_app *app);
static int app_timeout(const struct notes_app *app, uint64_t now);
static uint64_t app_unix_ms(void);
static uint64_t app_microseconds(void);
static void app_choose(struct notes_app *app, unsigned mode);
static void app_chooser_done(void *data, struct kui_file_chooser *chooser, unsigned result, const char *path, size_t filter);
static void app_chosen(struct notes_app *app);
static void app_open_file(struct notes_app *app, const char *path);
static void app_save_as(struct notes_app *app, const char *path);
static void app_place_page(struct notes_app *app);

/*
 * Runs Notes.
 */
int
main(
	int argc,
	char **argv)
{
	static struct notes_app app;
	const char *file;
	unsigned long value;
	uint64_t now;
	uint32_t width;
	uint32_t height;
	unsigned index;
	int fullscreen;
	int is_fullscreen;
	int is_width;
	int is_height;
	int is_timeout;
	int timeout;
	int status;
	int error;
	int arg;

	/* The options and the file. */
	file = NULL;
	fullscreen = 0;
	width = MAIN_WIDTH;
	height = MAIN_HEIGHT;
	for (arg = 1; arg < argc; arg++) {
		/* Which option the argument is, when it is one. */
		is_fullscreen = strcmp(argv[arg], "--fullscreen");
		is_width = strncmp(argv[arg], "--width=", 8U);
		is_height = strncmp(argv[arg], "--height=", 9U);
		is_timeout = strncmp(argv[arg], "--timeout-s=", 12U);

		/* Each option takes its value; anything else is the file. */
		if (is_fullscreen == 0) {
			fullscreen = 1;
		} else if (is_width == 0) {
			value = strtoul(argv[arg] + 8, NULL, 10);
			if (value >= 320U && value <= 8192U)
				width = (uint32_t)value;
		} else if (is_height == 0) {
			value = strtoul(argv[arg] + 9, NULL, 10);
			if (value >= 240U && value <= 8192U)
				height = (uint32_t)value;
		} else if (is_timeout == 0) {
			value = strtoul(argv[arg] + 12, NULL, 10);
			if (value != 0U)
				app.deadline = notes_clock() + (uint64_t)value * 1000U;
		} else if (argv[arg][0] == '-') {
			fprintf(stderr, "usage: notes [--fullscreen] [--width=W] [--height=H] [--timeout-s=S] [FILE.pdf]\n");
			return 2;
		} else {
			file = argv[arg];
		}
	}

	/* The notebook: recovered from a journal, or new. */
	app.tool = NOTES_ACTION_PEN;
	app.width = 1U;
	error = app_start_document(&app, file);
	if (error != 0) {
		fprintf(stderr, "notes: cannot start a notebook: %s\n", strerror(error));
		return 1;
	}

	/* The window. */
	status = notes_window_open(&app.window, width, height, fullscreen);
	if (status != 0) {
		fprintf(stderr, "notes: cannot open a window: %s\n", strerror(errno));
		return 1;
	}

	/* Its title names the file. */
	app_set_title(&app);

	/* The menus; without the System Menu the keys still work. */
	error = notes_menu_open(&app.window);
	if (error != 0)
		printf("NOTES MENU none error=%d\n", error);

	/* The drawing. */
	error = (int)notes_renderer_open(&app.renderer, &app.window);
	if (error != (int)VK_SUCCESS) {
		fprintf(stderr, "notes: %s failed (%d)\n", app.renderer.operation, error);
		notes_renderer_close(&app.renderer);
		notes_window_close(&app.window);
		return 1;
	}

	/* The toolbar's font; the buttons work without it. */
	error = notes_ui_open(&app.ui, MAIN_FONT);
	if (error != 0)
		printf("NOTES FONT none error=%d\n", error);

	/* The fingers; without memory for them they do nothing. */
	app.touch_due = -1;
	error = notes_touch_open(&app.touch);
	if (error != 0)
		printf("NOTES TOUCH none error=%d\n", error);

	/* The page's first place, before any input needs it. */
	app_place_page(&app);

	/* The tests' first line. */
	printf("NOTES START width=%u height=%u fullscreen=%d pages=%lu strokes=%lu path=%s\n",
	       app.window.width, app.window.height, app.window.fullscreen,
	       (unsigned long)app.document.page_count, (unsigned long)notes_document_stroke_total(&app.document), app.path);
	fflush(stdout);
	app.redraw = 1;
	app.toolbar_dirty = 1;

	/* The main loop: wait, take what arrived, save when due, draw when needed. */
	while (!app.quit) {
		/* Waits for the compositor until the next thing that is due. */
		now = notes_clock();
		timeout = app_timeout(&app, now);
		status = notes_window_dispatch(&app.window, timeout);
		if (status != 0) {
			printf("NOTES DISCONNECTED\n");
			break;
		}

		/* A new size remakes the swapchain and the toolbar. */
		if (app.window.resized) {
			app.window.resized = 0;
			error = (int)notes_renderer_resize(&app.renderer, app.window.width, app.window.height);
			if (error != (int)VK_SUCCESS) {
				fprintf(stderr, "notes: %s failed (%d)\n", app.renderer.operation, error);
				break;
			}

			/* Everything is drawn again at the new size. */
			app.redraw = 1;
			app.toolbar_dirty = 1;
		}

		/* The menus' choices. */
		for (index = 0; index < app.window.action_count; index++)
			app_action(&app, app.window.actions[index]);
		app.window.action_count = 0;

		/* The keys. */
		for (index = 0; index < app.window.key_count; index++)
			app_key(&app, &app.window.keys[index]);
		app.window.key_count = 0;

		/* The pointer's and the pen's events. */
		for (index = 0; index < app.window.input_count; index++)
			app_input(&app, &app.window.inputs[index]);
		app.window.input_count = 0;

		/* What the file chooser answered (ws128-p002). */
		if (app.chosen_ready)
			app_chosen(&app);

		/* The fingers' events, and where they put the page. */
		for (index = 0; index < app.window.touch_count; index++)
			notes_touch_event(&app.touch, &app.window.touches[index]);
		app.window.touch_count = 0;
		app_touch(&app);

		/* The autosave, once the notebook has been still long enough and nothing is being drawn. */
		now = notes_clock();
		if (app.document.dirty &&
		    app.contact == MAIN_CONTACT_NONE &&
		    now >= app.changed_at + NOTES_AUTOSAVE_IDLE_MS)
			(void)app_save(&app, "autosave");

		/* A status whose time is up goes. */
		if (app.status_until != 0U && now >= app.status_until) {
			app.status[0] = '\0';
			app.status_until = 0;
			app.toolbar_dirty = 1;
			app.redraw = 1;
		}

		/* The compositor's close, or the end of the run. */
		if (app.window.closed)
			app.quit = 1;
		if (app.deadline != 0U && now >= app.deadline)
			app.quit = 1;

		/* A new frame when something changed. */
		if (app.redraw && !app.quit)
			app_draw(&app);
	}

	/* A stroke still being drawn is kept, and the notebook saved when it changed. */
	if (app.live != NULL)
		app_end_contact(&app, NULL);
	if (app.document.dirty)
		(void)app_save(&app, "close");

	/* The tests' last line. */
	printf("NOTES EXIT pages=%lu strokes=%lu dirty=%d\n", (unsigned long)app.document.page_count,
	       (unsigned long)notes_document_stroke_total(&app.document), app.document.dirty);
	fflush(stdout);

	/* Everything goes; the journal stays only when the last save failed. */
	kui_file_chooser_destroy(app.chooser);
	app.chooser = NULL;
	notes_touch_close(&app.touch);
	notes_ui_close(&app.ui);
	notes_frame_free(&app.frame);
	notes_frame_free(&app.page_frame);
	pdf_display_list_destroy(app.background_list);
	notes_renderer_close(&app.renderer);
	notes_window_close(&app.window);
	notes_journal_destroy(app.document.journal);
	app.document.journal = NULL;
	notes_document_free(&app.document);

	/* Succeeded: Notes ran to its end. */
	return 0;
}

/*
 * Starts the notebook: the given file's journal, the most recent journal
 * when no file is given, or a new notebook.
 */
static int
app_start_document(
	struct notes_app *app,
	const char *file)
{
	char journal_path[MAIN_PATH_MAX];
	char recovered_path[MAIN_PATH_MAX];
	char folder[MAIN_PATH_MAX];
	struct stat status;
	const char *slash;
	const char *cwd;
	size_t records;
	unsigned opened;
	const char *kind;
	int written;
	int exists;
	int found;
	int error;

	/* The file's absolute path, when a file is given. */
	app->path[0] = '\0';
	if (file != NULL) {
		if (file[0] == '/') {
			written = snprintf(app->path, sizeof(app->path), "%s", file);
		} else {
			/* A relative path is under the current folder. */
			cwd = getcwd(folder, sizeof(folder));
			if (cwd == NULL)
				return errno;
			written = snprintf(app->path, sizeof(app->path), "%s/%s", folder, file);
		}

		/* A path that did not fit is refused. */
		if (written < 0 || (size_t)written >= sizeof(app->path))
			return ENAMETOOLONG;
	}

	/* The journal to recover: the file's, when it is there, or the most recent one. */
	found = 0;
	if (app->path[0] != '\0') {
		error = notes_journal_path(app->path, journal_path, sizeof(journal_path));
		if (error == 0) {
			exists = stat(journal_path, &status);
			if (exists == 0)
				found = 1;
		}
	} else {
		error = notes_journal_newest(journal_path, sizeof(journal_path));
		if (error == 0)
			found = 1;
	}

	/* A journal rebuilds the notebook as it was when Notes last ran; one written on a PDF gets that PDF back from the file. */
	if (found) {
		error = notes_journal_recover(journal_path, &app->document, recovered_path, sizeof(recovered_path), &records);
		if (error == 0) {
			error = notes_attach_base(recovered_path, &app->document);
			if (error != 0) {
				printf("NOTES RECOVER base error=%d path=%s\n", error, recovered_path);
				notes_document_free(&app->document);
			}
		}

		/* The recovered notebook, or the failure that leaves the file to be opened instead. */
		if (error == 0) {
			memcpy(app->path, recovered_path, strlen(recovered_path) + 1U);
			printf("NOTES RECOVER records=%lu pages=%lu strokes=%lu path=%s\n", (unsigned long)records,
			       (unsigned long)app->document.page_count, (unsigned long)notes_document_stroke_total(&app->document), app->path);
			(void)snprintf(app->status, sizeof(app->status), "Recovered %lu strokes", (unsigned long)notes_document_stroke_total(&app->document));
			app->status_until = notes_clock() + MAIN_STATUS_MS;
		} else {
			printf("NOTES RECOVER failed error=%d journal=%s\n", error, journal_path);
			found = 0;
		}
	}

	/* Whether the file is there, when no journal was recovered. */
	exists = -1;
	if (!found && app->path[0] != '\0')
		exists = stat(app->path, &status);

	/* A file that is there but has no journal is opened: a notebook from its edit data, another PDF as the background. */
	if (exists == 0) {
		error = notes_open_pdf(app->path, &app->document, &opened);
		if (error == 0) {
			found = 1;
			kind = app_opened_name(opened);
			printf("NOTES OPEN pages=%lu strokes=%lu kind=%s path=%s\n", (unsigned long)app->document.page_count,
			       (unsigned long)notes_document_stroke_total(&app->document), kind, app->path);

			/* Writing on another program's PDF is said once. */
			if (opened == NOTES_OPENED_FOREIGN)
				(void)snprintf(app->status, sizeof(app->status), "Writing on the PDF; it stays as it was under your ink");
			else if (opened == NOTES_OPENED_CHANGED)
				(void)snprintf(app->status, sizeof(app->status), "Pages changed by another program are now background");
			if (opened == NOTES_OPENED_FOREIGN || opened == NOTES_OPENED_CHANGED)
				app->status_until = notes_clock() + MAIN_NOTICE_MS;
		} else {
			/* A file Notes cannot write on is left as it is, and a new notebook starts. */
			printf("NOTES OPEN failed error=%d path=%s\n", error, app->path);
			if (error == EACCES)
				(void)snprintf(app->status, sizeof(app->status), "The PDF is encrypted; Notes cannot write on it. Started a new note");
			else if (error == EPERM)
				(void)snprintf(app->status, sizeof(app->status), "The PDF is signed; Notes does not write on it. Started a new note");
			else if (error == ENOTSUP)
				(void)snprintf(app->status, sizeof(app->status), "The PDF uses features Notes cannot read yet. Started a new note");
			else
				(void)snprintf(app->status, sizeof(app->status), "Cannot open the PDF; started a new note");
			app->status_until = notes_clock() + MAIN_NOTICE_MS;
			app->path[0] = '\0';
		}
	}

	/* Otherwise a new notebook, at the given path or a new one. */
	if (!found) {
		error = notes_document_init(&app->document, app_unix_ms());
		if (error != 0)
			return error;
		if (app->path[0] == '\0') {
			error = app_new_path(app->path, sizeof(app->path));
			if (error != 0)
				return error;
		}
	}

	/* The journal every change goes to. */
	app->document.journal = notes_journal_create(app->path);
	if (app->document.journal == NULL)
		printf("NOTES JOURNAL none error=%d\n", errno);

	/* The name shown in the title: the file's last part. */
	slash = strrchr(app->path, '/');
	app->name = app->path;
	if (slash != NULL)
		app->name = slash + 1;

	/* Succeeded: the notebook is ready. */
	app->changed_at = notes_clock();
	return 0;
}

/* Names what notes_open_pdf() found, for the tests' line. */
static const char *
app_opened_name(
	unsigned opened)
{
	/* Each kind by its word. */
	switch (opened) {
	case NOTES_OPENED_ANNOTATED:
		return "annotated";
	case NOTES_OPENED_FOREIGN:
		return "foreign";
	case NOTES_OPENED_CHANGED:
		return "changed";
	default:
		break;
	}

	/* A notebook Notes saved. */
	return "notes";
}

/*
 * Draws the background picture of a page of the PDF the notebook writes on
 * at a scale, into the renderer's background image at the page picture's
 * size, and tells whether it is there to show (0 when the page cannot be
 * drawn).
 *
 * libpdf interprets the base's page once into a display list, which is kept
 * while the page is shown, and rasterizes it on the CPU over white; the
 * picture is drawn again only for another page, scale or size.
 */
static int
app_background(
	struct notes_app *app,
	const struct notes_page *page,
	float scale)
{
	struct pdf_display_list *list;
	unsigned char *target;
	uint32_t *pixels;
	uint64_t started;
	uint64_t rendered;
	uint64_t rasterized;
	uint32_t width;
	uint32_t height;
	size_t pitch;
	size_t count;
	size_t index;
	size_t row;
	int error;

	/* Nothing to draw without the PDF. */
	if (app->document.base == NULL)
		return 0;

	/* The picture's size, the page picture's. */
	width = app->renderer.page_width;
	height = app->renderer.page_height;
	if (width == 0U || height == 0U)
		return 0;

	/* A picture drawn for the page at the scale and the size stands. */
	if (app->background_page == page &&
	    app->background_scale == scale &&
	    app->background_width == width &&
	    app->background_height == height &&
	    app->renderer.background_width == width &&
	    app->renderer.background_height == height)
		return 1;

	/* The base's page as a display list, made once while the page is shown. */
	started = app_microseconds();
	if (app->background_list == NULL || app->background_source != page->source) {
		pdf_display_list_destroy(app->background_list);
		app->background_list = NULL;
		app->background_failed = 0;
		app->background_source = page->source;
		error = pdf_page_render(app->document.base, page->source, &list);
		if (error != 0) {
			printf("NOTES BACKGROUND failed source=%lu error=%d\n", (unsigned long)page->source, error);
			fflush(stdout);
			app->background_failed = 1;
			return 0;
		}

		/* The drawing is kept while the page is shown. */
		app->background_list = list;
	}

	/* A page that could not be drawn stays white. */
	if (app->background_failed)
		return 0;
	rendered = app_microseconds();

	/* The picture on the CPU: white, then the page over it. */
	count = (size_t)width * (size_t)height;
	pixels = malloc(count * sizeof(*pixels));
	if (pixels == NULL)
		return 0;
	for (index = 0; index < count; index++)
		pixels[index] = 0xffffffffU;
	error = pdf_display_list_rasterize(app->background_list, pixels, width, width, height, (double)scale, 0.0, 0.0);
	if (error != 0) {
		printf("NOTES BACKGROUND raster failed source=%lu error=%d\n", (unsigned long)page->source, error);
		free(pixels);
		return 0;
	}

	/* When the picture was drawn, for the log. */
	rasterized = app_microseconds();

	/* The renderer's image of the size, which the picture's rows are copied into. */
	error = (int)notes_renderer_background(&app->renderer, width, height, &target, &pitch);
	if (error != (int)VK_SUCCESS || target == NULL) {
		printf("NOTES BACKGROUND image failed error=%d\n", error);
		free(pixels);
		return 0;
	}

	/* Copies each row into the image, whose rows may be longer. */
	for (row = 0; row < height; row++)
		memcpy(target + row * pitch, pixels + row * width, (size_t)width * sizeof(*pixels));
	free(pixels);

	/* The picture stands for the page at the scale (logged for the tests). */
	app->background_page = page;
	app->background_scale = scale;
	app->background_width = width;
	app->background_height = height;
	printf("NOTES BACKGROUND source=%lu items=%lu flags=%u size=%ux%u render_us=%lu raster_us=%lu\n", (unsigned long)page->source,
	       (unsigned long)app->background_list->count, app->background_list->flags, width, height,
	       (unsigned long)(rendered - started), (unsigned long)(rasterized - rendered));
	fflush(stdout);

	/* Succeeded: the background is in the image. */
	return 1;
}

/* Writes the path of a new notebook: ~/Documents/Notes/note-YYYYMMDD-HHMMSS.pdf. */
static int
app_new_path(
	char *path,
	size_t size)
{
	struct tm local;
	struct tm *converted;
	const char *home;
	time_t now;
	int written;

	/* The home directory, or /tmp without one. */
	home = getenv("HOME");
	if (home == NULL || home[0] != '/')
		home = "/tmp";

	/* The time the notebook was made names it (the epoch when the local time cannot be told). */
	now = time(NULL);
	converted = localtime_r(&now, &local);
	if (converted == NULL)
		memset(&local, 0, sizeof(local));
	written = snprintf(path, size, "%s/Documents/Notes/note-%04d%02d%02d-%02d%02d%02d.pdf", home,
			   local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec);
	if (written < 0 || (size_t)written >= size)
		return ENAMETOOLONG;

	/* Succeeded: the path (its folder is made at the first save). */
	return 0;
}

/* Makes the folders of a file's path that are missing. */
static int
app_make_folders(
	const char *path)
{
	char folder[MAIN_PATH_MAX];
	char *slash;
	size_t length;
	int status;

	/* The path, to cut at each slash. */
	length = strlen(path);
	if (length >= sizeof(folder))
		return ENAMETOOLONG;
	memcpy(folder, path, length + 1U);

	/* Each folder from the root down to the file's; one that exists is passed. */
	for (slash = folder + 1; *slash != '\0'; slash++) {
		if (*slash != '/')
			continue;
		*slash = '\0';
		status = mkdir(folder, 0755);
		*slash = '/';
		if (status != 0 && errno != EEXIST)
			return errno;
	}

	/* Succeeded: the file's folder is there. */
	return 0;
}

/* Sets the window's title: the file's name, a dash and "Notes", as PDF Viewer names its window (ws035-p122). */
static void
app_set_title(
	struct notes_app *app)
{
	char title[MAIN_PATH_MAX];

	/* The title with the file's name. */
	(void)snprintf(title, sizeof(title), "%s \xe2\x80\x94 Notes", app->name);
	notes_window_set_title(&app->window, title);
}

/* Shows a status on the toolbar for a while. */
static void
app_status(
	struct notes_app *app,
	const char *text)
{
	/* The text and its time. */
	(void)snprintf(app->status, sizeof(app->status), "%s", text);
	app->status_until = notes_clock() + MAIN_STATUS_MS;
	app->toolbar_dirty = 1;
	app->redraw = 1;
}

/* Gathers what the toolbar and the menus show. */
static void
app_state(
	const struct notes_app *app,
	struct notes_ui_state *state)
{
	/* The tool, colour, width, pages, history, changes and fullscreen. */
	memset(state, 0, sizeof(*state));
	state->tool = app->tool;
	state->color = app->color;
	state->width = app->width;
	state->page = app->page;
	state->page_count = app->document.page_count;
	state->dirty = app->document.dirty;
	state->erase_parts = app->erase_parts;

	/* Undo while a change stands, redo while one was taken back. */
	if (app->document.undo_done > 0U)
		state->can_undo = 1;
	if (app->document.undo_done < app->document.undo_count)
		state->can_redo = 1;

	/* The window's state and the status line. */
	state->fullscreen = app->window.fullscreen;
	state->finger_write = app->finger_write;
	state->status = app->status;
}

/* Carries out an action of the toolbar, a menu or a key. */
static void
app_action(
	struct notes_app *app,
	uint32_t action)
{
	size_t page;
	int error;

	/* A colour or a width chooses by its index. */
	if (action >= NOTES_ACTION_COLOR && action < NOTES_ACTION_COLOR + NOTES_COLORS) {
		app->color = action - NOTES_ACTION_COLOR;
		if (app->tool == NOTES_ACTION_ERASER)
			app->tool = NOTES_ACTION_PEN;
		app->toolbar_dirty = 1;
		app->redraw = 1;
		return;
	}

	/* A width, by its index. */
	if (action >= NOTES_ACTION_WIDTH && action < NOTES_ACTION_WIDTH + NOTES_WIDTHS) {
		app->width = action - NOTES_ACTION_WIDTH;
		app->toolbar_dirty = 1;
		app->redraw = 1;
		return;
	}

	/* The other actions. */
	switch (action) {
	case NOTES_ACTION_PEN:
	case NOTES_ACTION_HIGHLIGHTER:
		/* The tool. */
		app->tool = action;
		printf("NOTES TOOL %u\n", action);
		break;
	case NOTES_ACTION_ERASER:
		/* The eraser; chosen again, it switches between whole strokes and parts (design-input-notes.md section 5.2). */
		if (app->tool == NOTES_ACTION_ERASER)
			app->erase_parts = !app->erase_parts;
		app->tool = action;
		printf("NOTES TOOL %u parts=%d\n", action, app->erase_parts);
		break;
	case NOTES_ACTION_UNDO:
		/* The last change is taken back, and its page shown. */
		error = notes_document_undo(&app->document, &page);
		if (error != 0)
			break;
		if (page >= app->document.page_count)
			page = app->document.page_count - 1U;
		app->page = page;
		printf("NOTES UNDO page=%lu strokes=%lu pages=%lu\n", (unsigned long)app->page,
		       (unsigned long)app->document.pages[app->page]->stroke_count, (unsigned long)app->document.page_count);
		app_changed(app);
		break;
	case NOTES_ACTION_REDO:
		/* The last change taken back is made again, and its page shown. */
		error = notes_document_redo(&app->document, &page);
		if (error != 0)
			break;
		if (page >= app->document.page_count)
			page = app->document.page_count - 1U;
		app->page = page;
		printf("NOTES REDO page=%lu strokes=%lu pages=%lu\n", (unsigned long)app->page,
		       (unsigned long)app->document.pages[app->page]->stroke_count, (unsigned long)app->document.page_count);
		app_changed(app);
		break;
	case NOTES_ACTION_PREVIOUS_PAGE:
		/* The page before, when there is one. */
		if (app->page > 0U)
			app->page--;
		printf("NOTES PAGE current=%lu count=%lu\n", (unsigned long)app->page, (unsigned long)app->document.page_count);
		break;
	case NOTES_ACTION_NEXT_PAGE:
		/* The page after, when there is one. */
		if (app->page + 1U < app->document.page_count)
			app->page++;
		printf("NOTES PAGE current=%lu count=%lu\n", (unsigned long)app->page, (unsigned long)app->document.page_count);
		break;
	case NOTES_ACTION_NEW_PAGE:
		/* A blank page after the current one, shown. */
		error = notes_document_add_page(&app->document, app->page + 1U);
		if (error != 0) {
			app_status(app, "Could not add a page");
			break;
		}

		/* The new page is shown. */
		app->page++;
		printf("NOTES PAGE current=%lu count=%lu new\n", (unsigned long)app->page, (unsigned long)app->document.page_count);
		app_changed(app);
		break;
	case NOTES_ACTION_SAVE:
		/* Saved now, even when nothing changed. */
		(void)app_save(app, "request");
		break;
	case NOTES_ACTION_OPEN:
		/* Another PDF, chosen in libkeiui's file chooser (ws128-p002). */
		app_choose(app, KUI_FILE_CHOOSER_OPEN);
		break;
	case NOTES_ACTION_SAVE_AS:
		/* The notebook as another file, chosen in the file chooser (ws128-p002). */
		app_choose(app, KUI_FILE_CHOOSER_SAVE);
		break;
	case NOTES_ACTION_CLOSE:
		/* The main loop saves and ends. */
		app->quit = 1;
		break;
	case NOTES_ACTION_FULLSCREEN:
		/* Fullscreen on or off; the configure that follows redraws. */
		notes_window_set_fullscreen(&app->window, !app->window.fullscreen);
		break;
	case NOTES_ACTION_LEAVE_FULLSCREEN:
		/* Back to a window, when fullscreen. */
		if (app->window.fullscreen)
			notes_window_set_fullscreen(&app->window, 0);
		break;
	case NOTES_ACTION_FINGER:
		/* One finger writes, or scrolls again (a line under way is kept); the status says which. */
		app->finger_write = !app->finger_write;
		notes_touch_write_mode(&app->touch, app->finger_write, notes_touch_clock());
		app_finger(app);
		printf("NOTES FINGER write=%d\n", app->finger_write);
		if (app->finger_write) {
			app_status(app, "One finger writes, two fingers scroll");
		} else {
			app_status(app, "Fingers scroll and zoom");
		}

		/* Nothing else. */
		break;
	default:
		break;
	}

	/* The toolbar and the page show the result. */
	fflush(stdout);
	app->toolbar_dirty = 1;
	app->redraw = 1;
}

/* Turns a key the menus did not take into an action. */
static void
app_key(
	struct notes_app *app,
	const struct notes_key *key)
{
	int control;
	int shift;

	/* The modifiers that choose a shortcut. */
	control = 0;
	if ((key->modifiers & NOTES_MODIFIER_CONTROL) != 0U)
		control = 1;
	shift = 0;
	if ((key->modifiers & NOTES_MODIFIER_SHIFT) != 0U)
		shift = 1;

	/* The standard shortcuts with Control. */
	if (control) {
		switch (key->key) {
		case MAIN_KEY_S:
			if (shift)
				app_action(app, NOTES_ACTION_SAVE_AS);
			else
				app_action(app, NOTES_ACTION_SAVE);
			break;
		case MAIN_KEY_Z:
			if (shift)
				app_action(app, NOTES_ACTION_REDO);
			else
				app_action(app, NOTES_ACTION_UNDO);
			break;
		case MAIN_KEY_Y:
			app_action(app, NOTES_ACTION_REDO);
			break;
		case MAIN_KEY_N:
			app_action(app, NOTES_ACTION_NEW_PAGE);
			break;
		case MAIN_KEY_O:
			app_action(app, NOTES_ACTION_OPEN);
			break;
		case MAIN_KEY_W:
			app_action(app, NOTES_ACTION_CLOSE);
			break;
		default:
			break;
		}

		/* A key with Control is no tool key. */
		return;
	}

	/* The keys without Control. */
	switch (key->key) {
	case MAIN_KEY_PAGE_UP:
		app_action(app, NOTES_ACTION_PREVIOUS_PAGE);
		break;
	case MAIN_KEY_PAGE_DOWN:
		app_action(app, NOTES_ACTION_NEXT_PAGE);
		break;
	case MAIN_KEY_P:
		app_action(app, NOTES_ACTION_PEN);
		break;
	case MAIN_KEY_M:
		app_action(app, NOTES_ACTION_HIGHLIGHTER);
		break;
	case MAIN_KEY_E:
		app_action(app, NOTES_ACTION_ERASER);
		break;
	case MAIN_KEY_F11:
		app_action(app, NOTES_ACTION_FULLSCREEN);
		break;
	case MAIN_KEY_ESC:
		app_action(app, NOTES_ACTION_LEAVE_FULLSCREEN);
		break;
	default:
		break;
	}
}

/* Takes one input event: a press on the toolbar, a stroke, or an eraser drag. */
static void
app_input(
	struct notes_app *app,
	const struct notes_input *input)
{
	struct notes_stroke *stroke;
	uint32_t action;
	unsigned tool;
	uint32_t id;
	uint32_t color;
	float width;
	int near;

	/*
	 * The pen near the window or gone tells a palm from a finger
	 * (ws081-p013).  A line a palm was writing is taken back now, before
	 * the pen's own contact starts (ws081-p015).
	 */
	if (input->source != NOTES_SOURCE_POINTER) {
		near = 1;
		if (input->kind == NOTES_INPUT_LEAVE)
			near = 0;
		notes_touch_pen(&app->touch, near, notes_touch_clock());
		app_finger(app);
	}

	/* The pen's mark follows the pen, over the window or in contact. */
	app_hover(app, input);
	if (input->kind == NOTES_INPUT_HOVER || input->kind == NOTES_INPUT_LEAVE)
		return;

	/* A motion or an end belongs to the contact under way. */
	if (input->kind == NOTES_INPUT_MOTION) {
		if (app->contact == MAIN_CONTACT_DRAW || app->contact == MAIN_CONTACT_ERASE)
			app_sample(app, input);
		return;
	}

	/* An end of contact ends it. */
	if (input->kind == NOTES_INPUT_UP) {
		app_end_contact(app, input);
		return;
	}

	/* A contact that starts while one is under way (a lost release) ends the old one first. */
	if (app->contact != MAIN_CONTACT_NONE)
		app_end_contact(app, NULL);

	/* The frame times count from the contact's start. */
	app->frame_count = 0;
	app->frame_build_us = 0;
	app->frame_draw_us = 0;
	app->frame_longest_us = 0;

	/* A press on the toolbar is its button's. */
	if (input->y < (float)NOTES_TOOLBAR_HEIGHT) {
		app->contact = MAIN_CONTACT_TOOLBAR;
		action = notes_ui_hit(&app->ui, input->x, input->y);
		if (action != NOTES_ACTION_NONE)
			app_action(app, action);
		return;
	}

	/* The eraser tool, or a pen's eraser end, erases. */
	if (app->tool == NOTES_ACTION_ERASER || input->source == NOTES_SOURCE_ERASER) {
		app->contact = MAIN_CONTACT_ERASE;
		notes_document_erase_begin(&app->document);
		app_sample(app, input);
		return;
	}

	/* Otherwise a stroke starts, in the tool's colour and width. */
	tool = NOTES_TOOL_PEN;
	if (app->tool == NOTES_ACTION_HIGHLIGHTER)
		tool = NOTES_TOOL_HIGHLIGHTER;
	color = notes_ui_color(tool, app->color);
	width = notes_ui_width(tool, app->width);
	id = app->document.next_id;
	stroke = notes_stroke_create(id, tool, color, width, app_unix_ms());
	if (stroke == NULL)
		return;

	/* The stroke's number is taken, and its first sample. */
	app->document.next_id++;
	app->live = stroke;
	app->live_start = input->time_ms;
	app->contact = MAIN_CONTACT_DRAW;
	app_sample(app, input);
}

/*
 * Follows the pen for its mark: a pen over the window or in contact is
 * where its last event was; one that left, and the pointer (which has its
 * own cursor), have no mark.
 */
static void
app_hover(
	struct notes_app *app,
	const struct notes_input *input)
{
	/* The pointer and a pen that left show no mark; the frame shows it gone (logged for the tests). */
	if (input->kind == NOTES_INPUT_LEAVE || input->source == NOTES_SOURCE_POINTER) {
		if (app->hover) {
			printf("NOTES HOVER gone\n");
			fflush(stdout);
			app->redraw = 1;
		}

		/* No mark from now on. */
		app->hover = 0;
		return;
	}

	/* A pen that comes over the window is logged for the tests. */
	if (!app->hover || app->hover_source != input->source) {
		printf("NOTES HOVER source=%u x=%d y=%d\n", input->source, (int)input->x, (int)input->y);
		fflush(stdout);
	}

	/* The pen is here now; the frame shows its mark here. */
	app->hover = 1;
	app->hover_x = input->x;
	app->hover_y = input->y;
	app->hover_source = input->source;
	app->redraw = 1;
}

/*
 * Moves time on for the fingers: the toolbar's taps press its buttons, and
 * a page the fingers moved (or that glides, or is zoomed) is drawn again.
 */
static void
app_touch(
	struct notes_app *app)
{
	uint32_t action;
	float x;
	float y;
	float scale;
	int taken;

	/* A writing finger's line. */
	app_finger(app);

	/* Each tap on the toolbar presses the button under it. */
	for (;;) {
		taken = notes_touch_take_tap(&app->touch, &x, &y);
		if (!taken)
			break;
		action = notes_ui_hit(&app->ui, x, y);
		if (action != NOTES_ACTION_NONE)
			app_action(app, action);
	}

	/* The fingers' time moves on. */
	app->touch_due = notes_touch_tick(&app->touch, notes_touch_clock());

	/*
	 * A place other than the one last drawn (by the tick, or by a gesture of
	 * the events before it, such as a double tap), or the start or end of a
	 * zoom (the page is drawn at its new scale), is drawn.
	 */
	notes_touch_view(&app->touch, &x, &y, &scale);
	if (x != app->view.x ||
	    y != app->view.y ||
	    scale != app->view.scale ||
	    app->touch.zooming != app->drawn_zooming)
		app->redraw = 1;
}

/* Adds a sample to the stroke being drawn, or erases at it. */
static void
app_sample(
	struct notes_app *app,
	const struct notes_input *input)
{
	struct notes_point point;
	struct notes_point *last;
	size_t removed;
	float pressure;
	float dx;
	float dy;
	float step;
	int error;

	/* The place on the page, in points. */
	memset(&point, 0, sizeof(point));
	point.x = (input->x - app->view.x) / app->view.scale;
	point.y = (input->y - app->view.y) / app->view.scale;

	/* An eraser of parts cuts the ink under its circle out of the strokes. */
	if (app->contact == MAIN_CONTACT_ERASE && app->erase_parts) {
		error = notes_document_erase_parts_at(&app->document, app->page, point.x, point.y, MAIN_ERASER_RADIUS, &removed);
		if (error == 0 && removed != 0U) {
			printf("NOTES ERASE page=%lu cut=%lu strokes=%lu\n", (unsigned long)app->page, (unsigned long)removed,
			       (unsigned long)app->document.pages[app->page]->stroke_count);
			fflush(stdout);
			app_changed(app);
		}

		/* An eraser draws no stroke. */
		return;
	}

	/* An eraser of strokes removes the strokes its circle touches. */
	if (app->contact == MAIN_CONTACT_ERASE) {
		error = notes_document_erase_at(&app->document, app->page, point.x, point.y, MAIN_ERASER_RADIUS, &removed);
		if (error == 0 && removed != 0U) {
			printf("NOTES ERASE page=%lu removed=%lu strokes=%lu\n", (unsigned long)app->page, (unsigned long)removed,
			       (unsigned long)app->document.pages[app->page]->stroke_count);
			fflush(stdout);
			app_changed(app);
		}

		/* An eraser draws no stroke. */
		return;
	}

	/* A sample too close to the last one adds nothing. */
	if (app->live->point_count != 0U) {
		last = &app->live->points[app->live->point_count - 1U];
		dx = (point.x - last->x) * app->view.scale;
		dy = (point.y - last->y) * app->view.scale;
		step = dx * dx + dy * dy;
		if (step < MAIN_SAMPLE_STEP * MAIN_SAMPLE_STEP)
			return;
	}

	/* The pressure, the tilt (in 1/100 degree) and the time since the stroke began. */
	pressure = input->pressure;
	if (!(pressure >= 0.0f))
		pressure = 0.0f;
	if (pressure > 1.0f)
		pressure = 1.0f;
	point.pressure = (uint16_t)(pressure * (float)NOTES_PRESSURE_MAX + 0.5f);
	point.tilt_x = (int16_t)(input->tilt_x * 100.0f);
	point.tilt_y = (int16_t)(input->tilt_y * 100.0f);
	point.time_ms = input->time_ms - app->live_start;
	if (input->source != NOTES_SOURCE_POINTER)
		app->live->has_tilt = 1;

	/* Succeeded or not, the stroke is drawn again with what it has. */
	(void)notes_stroke_append(app->live, &point);
	app->redraw = 1;
}

/* Ends the contact under way: a stroke goes on the page, an eraser drag becomes one change. */
static void
app_end_contact(
	struct notes_app *app,
	const struct notes_input *input)
{
	struct notes_stroke *stroke;
	unsigned lowest;
	unsigned highest;
	size_t index;
	int error;

	/* The last sample of a stroke or an eraser drag. */
	if (input != NULL &&
	    (app->contact == MAIN_CONTACT_DRAW ||
	     app->contact == MAIN_CONTACT_ERASE))
		app_sample(app, input);

	/* A finished stroke goes on top of the page. */
	if (app->contact == MAIN_CONTACT_DRAW && app->live != NULL) {
		stroke = app->live;
		app->live = NULL;
		error = notes_document_add_stroke(&app->document, app->page, stroke);
		if (error != 0) {
			notes_stroke_free(stroke);
			app_status(app, "Could not keep the stroke");
		} else {
			/* The range of the stroke's pressure, for the tests' line. */
			lowest = NOTES_PRESSURE_MAX;
			highest = 0;
			for (index = 0; index < stroke->point_count; index++) {
				if (stroke->points[index].pressure < lowest)
					lowest = stroke->points[index].pressure;
				if (stroke->points[index].pressure > highest)
					highest = stroke->points[index].pressure;
			}

			/* The tests' line. */
			printf("NOTES STROKE page=%lu id=%u tool=%u points=%lu strokes=%lu pressure=%u..%u tilt=%d\n", (unsigned long)app->page,
			       stroke->id, stroke->tool, (unsigned long)stroke->point_count, (unsigned long)app->document.pages[app->page]->stroke_count,
			       lowest, highest, stroke->has_tilt);
			fflush(stdout);
			app_changed(app);
		}
	}

	/* An eraser drag's removals are one change from now on. */
	if (app->contact == MAIN_CONTACT_ERASE)
		notes_document_erase_end(&app->document);

	/* The frames the contact took, for the tests' line. */
	app_frame_report(app);

	/* No contact is under way, and no finger's. */
	app->contact = MAIN_CONTACT_NONE;
	app->finger_contact = 0;
	app->redraw = 1;
}

/*
 * Carries out a writing finger's events (ws081-p015): its line is drawn
 * (or erased along) as the pointer's would be, with the pointer's fixed
 * pressure, and a line taken back is dropped.  Once the pointer or the pen
 * takes the contact over, the finger's later events are left alone.
 */
static void
app_finger(
	struct notes_app *app)
{
	struct notes_touch_write write;
	struct notes_input input;
	int taken;

	/* Each event in turn. */
	for (;;) {
		taken = notes_touch_take_write(&app->touch, &write);
		if (!taken)
			break;

		/* The pointer's input at the finger's place and time. */
		memset(&input, 0, sizeof(input));
		input.source = NOTES_SOURCE_POINTER;
		input.x = write.x;
		input.y = write.y;
		input.pressure = NOTES_POINTER_PRESSURE;
		input.time_ms = write.time_ms;

		/* What the event does to the finger's contact. */
		switch (write.kind) {
		case NOTES_TOUCH_WRITE_BEGIN:
			/* A contact starts as the pointer's press starts one; the finger owns it while it draws or erases. */
			input.kind = NOTES_INPUT_DOWN;
			app_input(app, &input);
			app->finger_contact = 0;
			if (app->contact == MAIN_CONTACT_DRAW ||
			    app->contact == MAIN_CONTACT_ERASE)
				app->finger_contact = 1;
			break;
		case NOTES_TOUCH_WRITE_MOTION:
			/* A point of the finger's line. */
			input.kind = NOTES_INPUT_MOTION;
			if (app->finger_contact)
				app_input(app, &input);
			break;
		case NOTES_TOUCH_WRITE_END:
			/* The line ends and goes on the page. */
			input.kind = NOTES_INPUT_UP;
			if (app->finger_contact)
				app_input(app, &input);
			break;
		case NOTES_TOUCH_WRITE_ABORT:
			/* The line is taken back. */
			if (app->finger_contact)
				app_abort_contact(app);
			break;
		default:
			break;
		}
	}
}

/*
 * Takes back the contact under way without keeping it: a stroke being
 * drawn is dropped (its number is not used again), and an eraser drag's
 * removals so far stay as one change.
 */
static void
app_abort_contact(
	struct notes_app *app)
{
	/* The stroke never reaches the page. */
	if (app->contact == MAIN_CONTACT_DRAW && app->live != NULL) {
		notes_stroke_free(app->live);
		app->live = NULL;
		printf("NOTES ABORT stroke page=%lu strokes=%lu\n", (unsigned long)app->page,
		       (unsigned long)app->document.pages[app->page]->stroke_count);
		fflush(stdout);
	}

	/* An eraser drag's removals are one change from now on. */
	if (app->contact == MAIN_CONTACT_ERASE)
		notes_document_erase_end(&app->document);

	/* No contact is under way, and no finger's. */
	app->contact = MAIN_CONTACT_NONE;
	app->finger_contact = 0;
	app->redraw = 1;
}

/* Notes a change of the notebook: the autosave waits from now, and the toolbar shows it. */
static void
app_changed(
	struct notes_app *app)
{
	/* The time of the change. */
	app->changed_at = notes_clock();
	app->toolbar_dirty = 1;
	app->redraw = 1;
}

/*
 * Saves the notebook as its PDF, and removes the journal the save makes
 * unnecessary.  Returns 0, or an errno value.
 */
static int
app_save(
	struct notes_app *app,
	const char *reason)
{
	char text[160];
	size_t bytes;
	int error;

	/* The file's folder, then the file. */
	bytes = 0;
	error = app_make_folders(app->path);
	if (error == 0)
		error = notes_save_pdf(&app->document, app->path, &bytes);

	/* A failure is shown and logged, and the journal keeps the changes. */
	if (error != 0) {
		printf("NOTES SAVE failed reason=%s error=%d path=%s\n", reason, error, app->path);
		fflush(stdout);
		(void)snprintf(text, sizeof(text), "Could not save: %s", strerror(error));
		app_status(app, text);
		return error;
	}

	/* The journal is no longer needed; the next change starts another. */
	if (app->document.journal != NULL)
		(void)notes_journal_discard(app->document.journal);

	/* The file is among the recent ones. */
	(void)keiland_recent_add(app->path, "notes");

	/* The tests' line and the status. */
	printf("NOTES SAVE reason=%s pages=%lu strokes=%lu bytes=%lu path=%s\n", reason, (unsigned long)app->document.page_count,
	       (unsigned long)notes_document_stroke_total(&app->document), (unsigned long)bytes, app->path);
	fflush(stdout);
	app_status(app, "Saved");

	/* Succeeded: the file is the notebook as it stands. */
	return 0;
}

/*
 * Builds and draws a frame: the desk, the page's picture with the strokes
 * added since the last frame, the stroke being drawn, the pen's mark and
 * the toolbar.
 */
static void
app_draw(
	struct notes_app *app)
{
	struct notes_ui_state state;
	struct notes_view view;
	struct notes_page *page;
	struct notes_frame *page_frame;
	unsigned char *pixels;
	const char *mode;
	uint64_t started;
	uint64_t built;
	uint64_t finished;
	uint32_t picture_width;
	uint32_t picture_height;
	size_t pitch;
	size_t index;
	float stretch;
	int stretched;
	int page_clear;
	int error;

	/* When the frame starts, for the frame times. */
	started = app_microseconds();

	/* The whole page's place, and the place the fingers' zoom and scroll give it (another page starts at its top). */
	page = app->document.pages[app->page];
	notes_view_layout(&view, app->renderer.extent.width, app->renderer.extent.height, page->width, page->height);
	notes_touch_layout(&app->touch, app->renderer.extent.width, app->renderer.extent.height, (float)NOTES_TOOLBAR_HEIGHT, NOTES_PAGE_MARGIN,
			   page->width, page->height, view.scale);
	if (page != app->touch_page) {
		app->touch_page = page;
		notes_touch_top(&app->touch);
	}

	/* The place the frame is drawn at. */
	notes_touch_view(&app->touch, &view.x, &view.y, &view.scale);

	/* A new place (and the first) is logged for the tests, once the fingers let the page rest. */
	if (!app->touch.moving &&
	    !app->touch.pinching &&
	    (!app->drawn ||
	     view.x != app->logged_view.x ||
	     view.y != app->logged_view.y ||
	     view.scale != app->logged_view.scale)) {
		printf("NOTES LAYOUT window=%ux%u page=%d,%d,%d,%d scale=%.4f\n", app->renderer.extent.width, app->renderer.extent.height,
		       (int)view.x, (int)view.y, (int)(page->width * view.scale), (int)(page->height * view.scale), (double)view.scale);
		fflush(stdout);
		app->logged_view = view;
	}

	/* The place the input is measured against, and whether it is drawn while zooming. */
	app->view = view;
	app->drawn_zooming = app->touch.zooming;

	/* The toolbar, drawn again when its state changed; the menus show the same state. */
	if (app->toolbar_dirty) {
		app_state(app, &state);
		notes_renderer_toolbar(&app->renderer, &pixels, &pitch);
		notes_ui_draw(&app->ui, pixels, pitch, app->renderer.extent.width, NOTES_TOOLBAR_IMAGE_HEIGHT, &state);
		notes_menu_refresh(&app->window, &state);
		app->toolbar_dirty = 0;

		/* The buttons' places, logged for the tests once per width. */
		if (app->buttons_logged != app->renderer.extent.width) {
			app->buttons_logged = app->renderer.extent.width;
			printf("NOTES BUTTONS");
			for (index = 0; index < app->ui.button_count; index++) {
				printf(" %u:%d,%d,%d,%d", app->ui.buttons[index].action, app->ui.buttons[index].x, app->ui.buttons[index].y,
				       app->ui.buttons[index].width, app->ui.buttons[index].height);
			}

			/* The line ends. */
			printf("\n");
			fflush(stdout);
		}
	}

	/*
	 * While two fingers zoom, a picture of the page as it stands (at another
	 * scale) is stretched to the new one rather than drawn again every frame
	 * (ws081-p013); the page is drawn at its scale when they stop.
	 */
	stretch = 1.0f;
	stretched = 0;
	if (app->touch.zooming &&
	    app->picture_page == page &&
	    app->picture_serial == app->renderer.page_serial &&
	    app->picture_reshaped == app->document.reshaped &&
	    app->picture_strokes == page->stroke_count &&
	    app->picture_scale > 0.0f) {
		stretched = 1;
		stretch = view.scale / app->picture_scale;
	}

	/* The page's picture, at the page's size in whole pixels. */
	page_clear = 0;
	page_frame = NULL;
	if (!stretched) {
		picture_width = (uint32_t)ceil((double)(page->width * view.scale));
		picture_height = (uint32_t)ceil((double)(page->height * view.scale));
		error = (int)notes_renderer_page(&app->renderer, picture_width, picture_height);
		if (error != (int)VK_SUCCESS) {
			fprintf(stderr, "notes: %s failed (%d)\n", app->renderer.operation, error);
			app->quit = 1;
			return;
		}

		/* What the picture lacks, when anything: the whole page, or the strokes put on top since. */
		page_frame = app_page_frame(app, page, view.scale, &page_clear);
	}

	/* The desk and the page's picture on it. */
	notes_frame_begin(&app->frame);
	app_desk(app, &view, page->width * view.scale, page->height * view.scale);
	notes_frame_texture(&app->frame, NOTES_TEXTURE_PAGE, view.x, view.y,
			    (float)app->renderer.page_width * stretch, (float)app->renderer.page_height * stretch);

	/* The stroke being drawn, on top, clipped to the page. */
	notes_frame_clip(&app->frame, 1, view.x, view.y, page->width * view.scale, page->height * view.scale);
	if (app->live != NULL && app->live->point_count != 0U) {
		error = notes_stroke_outline(app->live);
		if (error == 0)
			notes_frame_polygon(&app->frame, app->live->outline, app->live->outline_count, &view, app->live->color);
	}

	/* The pen's mark on the page. */
	app_mark(app, &view, 0);

	/* The toolbar across the top, and the pen's mark over it. */
	notes_frame_clip(&app->frame, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	notes_frame_texture(&app->frame, NOTES_TEXTURE_TOOLBAR, 0.0f, 0.0f, (float)app->renderer.extent.width, (float)NOTES_TOOLBAR_IMAGE_HEIGHT);
	app_mark(app, &view, 1);

	/* A frame that ran out of memory is not drawn. */
	if (app->frame.error != 0 ||
	    (page_frame != NULL &&
	     page_frame->error != 0)) {
		printf("NOTES DRAW out of memory\n");
		return;
	}

	/* Draws it, after the time the geometry took; a swapchain out of date is remade and drawn next time. */
	built = app_microseconds();
	error = (int)notes_renderer_draw(&app->renderer, &app->frame, page_frame, page_clear);
	if (error == (int)VK_ERROR_OUT_OF_DATE_KHR) {
		app->window.resized = 1;
		return;
	}

	/* Any other failure ends Notes. */
	if (error != (int)VK_SUCCESS) {
		fprintf(stderr, "notes: %s failed (%d)\n", app->renderer.operation, error);
		app->quit = 1;
		return;
	}

	/* The picture holds the page as it stands now: drawn anew, or added to (logged for the tests). */
	if (page_frame != NULL) {
		mode = "add";
		if (page_clear)
			mode = "full";
		printf("NOTES PICTURE %s strokes=%lu..%lu\n", mode, (unsigned long)app->picture_strokes, (unsigned long)page->stroke_count);
		fflush(stdout);
		app->picture_page = page;
		app->picture_serial = app->renderer.page_serial;
		app->picture_reshaped = app->document.reshaped;
		app->picture_scale = view.scale;
		app->picture_strokes = page->stroke_count;
	}

	/* The frame's times join the ones the next NOTES FRAMES line reports. */
	finished = app_microseconds();
	app->frame_count++;
	app->frame_build_us += built - started;
	app->frame_draw_us += finished - built;
	if (finished - started > app->frame_longest_us)
		app->frame_longest_us = finished - started;

	/* Succeeded: the frame is shown (the first is logged for the tests). */
	if (!app->drawn) {
		printf("NOTES FRAME first draws=%lu\n", (unsigned long)app->frame.draw_count);
		fflush(stdout);
	}

	/* Nothing waits to be drawn. */
	app->drawn = 1;
	app->redraw = 0;
}

/*
 * Builds the page frame the page's picture lacks, and tells whether it
 * starts from a cleared picture: the whole page when the picture shows
 * another page, was made again, is of another scale, or lost strokes;
 * only the strokes put on top since when that is all that changed.
 * Returns NULL when the picture shows the page as it stands.
 */
static struct notes_frame *
app_page_frame(
	struct notes_app *app,
	const struct notes_page *page,
	float scale,
	int *clear)
{
	struct notes_stroke *stroke;
	struct notes_view origin;
	size_t first;
	size_t index;
	int drawn;
	int error;

	/* The picture must be drawn from the start when anything but strokes on top changed. */
	*clear = 0;
	if (app->picture_page != page) {
		*clear = 1;
	} else if (app->picture_serial != app->renderer.page_serial) {
		*clear = 1;
	} else if (app->picture_reshaped != app->document.reshaped) {
		*clear = 1;
	} else if (app->picture_scale != scale) {
		*clear = 1;
	} else if (page->stroke_count < app->picture_strokes) {
		*clear = 1;
	}

	/* A picture that has every stroke needs nothing. */
	if (!*clear && page->stroke_count == app->picture_strokes)
		return NULL;

	/* The picture's own place: the page at its top left, at the frame's scale. */
	origin.x = 0.0f;
	origin.y = 0.0f;
	origin.scale = scale;

	/*
	 * A cleared picture starts with the white page, and on a page of the
	 * PDF the notebook writes on, with that page drawn over it; one added to
	 * starts at its first missing stroke.
	 */
	notes_frame_begin(&app->page_frame);
	first = app->picture_strokes;
	if (*clear) {
		first = 0;
		app->picture_strokes = 0;
		notes_frame_rect(&app->page_frame, 0.0f, 0.0f, page->width * scale, page->height * scale, 0xffffffffU);
		drawn = 0;
		if (page->origin == NOTES_ORIGIN_OVER)
			drawn = app_background(app, page, scale);
		if (drawn) {
			notes_frame_texture(&app->page_frame, NOTES_TEXTURE_BACKGROUND, 0.0f, 0.0f,
					    (float)app->renderer.background_width, (float)app->renderer.background_height);
		}
	}

	/* The strokes, bottom first, clipped to the page. */
	notes_frame_clip(&app->page_frame, 1, 0.0f, 0.0f, page->width * scale, page->height * scale);
	for (index = first; index < page->stroke_count; index++) {
		stroke = page->strokes[index];
		error = notes_stroke_outline(stroke);
		if (error != 0)
			continue;
		notes_frame_polygon(&app->page_frame, stroke->outline, stroke->outline_count, &origin, stroke->color);
	}

	/* Reports the page frame. */
	return &app->page_frame;
}

/* Adds the desk: the soft wash over the window, and the page's shadow. */
static void
app_desk(
	struct notes_app *app,
	const struct notes_view *view,
	float width,
	float height)
{
	float window_width;
	float window_height;
	float haze;
	float left;
	float top;
	float share;
	uint32_t color;
	int ring;

	/* The wash: from sky at the top to the haze, and from the haze to leaf green at the bottom. */
	window_width = (float)app->renderer.extent.width;
	window_height = (float)app->renderer.extent.height;
	haze = (float)floor((double)(window_height * MAIN_DESK_HAZE_SHARE));
	notes_frame_gradient(&app->frame, 0.0f, 0.0f, window_width, haze, MAIN_DESK_TOP, MAIN_DESK_HAZE);
	notes_frame_gradient(&app->frame, 0.0f, haze, window_width, window_height - haze, MAIN_DESK_HAZE, MAIN_DESK_BOTTOM);

	/*
	 * The shadow, dropped a little below the page: rings a pixel wide round
	 * it, fading outwards.  Each ring is four thin strips, so only the
	 * shadow's own pixels are drawn.
	 */
	for (ring = 0; ring < MAIN_SHADOW_RINGS; ring++) {
		share = 1.0f - (float)ring / (float)MAIN_SHADOW_RINGS;
		color = MAIN_SHADOW_COLOR | (uint32_t)(share * share * (float)MAIN_SHADOW_ALPHA);
		left = view->x - (float)ring - 1.0f;
		top = view->y + MAIN_SHADOW_DROP - (float)ring - 1.0f;

		/* The ring's top and bottom strips, then its sides between them. */
		notes_frame_rect(&app->frame, left, top, width + 2.0f * (float)ring + 2.0f, 1.0f, color);
		notes_frame_rect(&app->frame, left, top + height + 2.0f * (float)ring + 1.0f, width + 2.0f * (float)ring + 2.0f, 1.0f, color);
		notes_frame_rect(&app->frame, left, top + 1.0f, 1.0f, height + 2.0f * (float)ring, color);
		notes_frame_rect(&app->frame, left + width + 2.0f * (float)ring + 1.0f, top + 1.0f, 1.0f, height + 2.0f * (float)ring, color);
	}
}

/*
 * Adds the pen's mark: where the pen is while it is over the window and
 * the compositor shows no cursor for it.  On the page it shows the tool --
 * a dot of the pen's or the highlighter's colour and width, or the eraser's
 * ring -- and over the toolbar (over_toolbar) a small slate dot.  A pen
 * drawing a stroke has no mark: the ink shows where it is.
 */
static void
app_mark(
	struct notes_app *app,
	const struct notes_view *view,
	int over_toolbar)
{
	unsigned tool;
	uint32_t color;
	float radius;
	int on_toolbar;

	/* No pen over the window, or a pen drawing. */
	if (!app->hover)
		return;
	if (app->contact == MAIN_CONTACT_DRAW)
		return;

	/* The mark is drawn with the page's part of the frame, or the toolbar's. */
	on_toolbar = 0;
	if (app->hover_y < (float)NOTES_TOOLBAR_HEIGHT)
		on_toolbar = 1;
	if (on_toolbar != over_toolbar)
		return;

	/* Over the toolbar, a small dot that points at the buttons. */
	if (on_toolbar) {
		app_circle(&app->frame, app->hover_x, app->hover_y, 5.5f, MAIN_MARK_HALO);
		app_circle(&app->frame, app->hover_x, app->hover_y, 4.0f, MAIN_MARK_RING | 0xffU);
		return;
	}

	/* The eraser, or a pen's eraser end: its ring, the size it erases. */
	if (app->tool == NOTES_ACTION_ERASER || app->hover_source == NOTES_SOURCE_ERASER) {
		radius = MAIN_ERASER_RADIUS * view->scale;
		app_circle(&app->frame, app->hover_x, app->hover_y, radius, MAIN_MARK_FILL);
		app_ring(&app->frame, app->hover_x, app->hover_y, radius, 1.5f, MAIN_MARK_RING);
		return;
	}

	/* The pen or the highlighter: a dot of its colour and about its width, on a white halo. */
	tool = NOTES_TOOL_PEN;
	if (app->tool == NOTES_ACTION_HIGHLIGHTER)
		tool = NOTES_TOOL_HIGHLIGHTER;
	color = notes_ui_color(tool, app->color);
	radius = notes_ui_width(tool, app->width) * view->scale * 0.5f;
	if (radius < 2.0f)
		radius = 2.0f;
	app_circle(&app->frame, app->hover_x, app->hover_y, radius + 1.5f, MAIN_MARK_HALO);
	app_circle(&app->frame, app->hover_x, app->hover_y, radius, color);
}

/* Adds a filled circle in window pixels, in a colour (0xRRGGBBAA). */
static void
app_circle(
	struct notes_frame *frame,
	float cx,
	float cy,
	float radius,
	uint32_t color)
{
	struct pdf_point points[MAIN_CIRCLE_POINTS];
	struct notes_view pixels;
	double angle;
	unsigned index;

	/* The circle's corners, counter-clockwise. */
	for (index = 0; index < MAIN_CIRCLE_POINTS; index++) {
		angle = 2.0 * 3.14159265358979 * (double)index / (double)MAIN_CIRCLE_POINTS;
		points[index].x = (double)cx + (double)radius * cos(angle);
		points[index].y = (double)cy + (double)radius * sin(angle);
	}

	/* The polygon, placed in pixels as they are. */
	pixels.x = 0.0f;
	pixels.y = 0.0f;
	pixels.scale = 1.0f;
	notes_frame_polygon(frame, points, MAIN_CIRCLE_POINTS, &pixels, color);
}

/*
 * Adds a ring in window pixels, in a colour (0xRRGGBBAA): one polygon that
 * goes round the outer circle and back round the inner one, which the
 * nonzero rule fills between them.
 */
static void
app_ring(
	struct notes_frame *frame,
	float cx,
	float cy,
	float radius,
	float thickness,
	uint32_t color)
{
	struct pdf_point points[2U * MAIN_CIRCLE_POINTS + 2U];
	struct notes_view pixels;
	double angle;
	double inner;
	unsigned index;
	unsigned count;

	/* The outer circle, closed back to its first corner. */
	count = 0;
	for (index = 0; index <= MAIN_CIRCLE_POINTS; index++) {
		angle = 2.0 * 3.14159265358979 * (double)index / (double)MAIN_CIRCLE_POINTS;
		points[count].x = (double)cx + (double)radius * cos(angle);
		points[count].y = (double)cy + (double)radius * sin(angle);
		count++;
	}

	/* The inner circle the other way round, back to the start. */
	inner = (double)radius - (double)thickness;
	for (index = 0; index <= MAIN_CIRCLE_POINTS; index++) {
		angle = -2.0 * 3.14159265358979 * (double)index / (double)MAIN_CIRCLE_POINTS;
		points[count].x = (double)cx + inner * cos(angle);
		points[count].y = (double)cy + inner * sin(angle);
		count++;
	}

	/* The polygon, placed in pixels as they are. */
	pixels.x = 0.0f;
	pixels.y = 0.0f;
	pixels.scale = 1.0f;
	notes_frame_polygon(frame, points, count, &pixels, color);
}

/*
 * Logs the frames drawn since the last report -- their count, the average
 * time their geometry and their drawing took, and the longest frame -- and
 * starts counting again.
 */
static void
app_frame_report(
	struct notes_app *app)
{
	unsigned long build_average;
	unsigned long draw_average;

	/* A contact that drew no frame has nothing to report. */
	if (app->frame_count == 0U)
		return;

	/* The averages, in microseconds. */
	build_average = (unsigned long)(app->frame_build_us / app->frame_count);
	draw_average = (unsigned long)(app->frame_draw_us / app->frame_count);

	/* The tests' line. */
	printf("NOTES FRAMES count=%lu strokes=%lu build_us=%lu draw_us=%lu frame_us=%lu longest_us=%lu\n", app->frame_count,
	       (unsigned long)app->document.pages[app->page]->stroke_count, build_average, draw_average,
	       build_average + draw_average, (unsigned long)app->frame_longest_us);
	fflush(stdout);

	/* The next report counts from here. */
	app->frame_count = 0;
	app->frame_build_us = 0;
	app->frame_draw_us = 0;
	app->frame_longest_us = 0;
}

/* Tells how long the main loop may wait, in milliseconds (-1: until the compositor speaks). */
static int
app_timeout(
	const struct notes_app *app,
	uint64_t now)
{
	uint64_t due;
	uint64_t wait;

	/* The next thing due: the autosave, the status's end, the end of the run. */
	due = 0;
	if (app->document.dirty && app->contact == MAIN_CONTACT_NONE)
		due = app->changed_at + NOTES_AUTOSAVE_IDLE_MS;
	if (app->status_until != 0U &&
	    (due == 0U ||
	     app->status_until < due))
		due = app->status_until;
	if (app->deadline != 0U &&
	    (due == 0U ||
	     app->deadline < due))
		due = app->deadline;

	/* A frame waiting to be drawn is due now. */
	if (app->redraw)
		return 0;

	/* The fingers' next tick, when it comes before anything else. */
	if (app->touch_due >= 0 &&
	    (due == 0U ||
	     now + (uint64_t)app->touch_due < due))
		return app->touch_due;

	/* Nothing is due: wait for the compositor. */
	if (due == 0U)
		return -1;

	/* Something past due runs now. */
	if (due <= now)
		return 0;

	/* Reports the wait, within an int. */
	wait = due - now;
	if (wait > 1000000U)
		wait = 1000000U;
	return (int)wait;
}

/* Returns the time of day in UNIX milliseconds. */
static uint64_t
app_unix_ms(void)
{
	struct timespec now;
	int status;

	/* The real-time clock. */
	status = clock_gettime(CLOCK_REALTIME, &now);
	if (status != 0)
		return 0U;

	/* Reports it in milliseconds. */
	return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
}

/* Returns the monotonic time in microseconds. */
static uint64_t
app_microseconds(void)
{
	struct timespec now;
	int status;

	/* The monotonic clock. */
	status = clock_gettime(CLOCK_MONOTONIC, &now);
	if (status != 0)
		return 0U;

	/* Reports it in microseconds. */
	return (uint64_t)now.tv_sec * 1000000U + (uint64_t)now.tv_nsec / 1000U;
}

/*
 * Shows libkeiui's file chooser for File > Open (KUI_FILE_CHOOSER_OPEN) or
 * Save As (KUI_FILE_CHOOSER_SAVE), at the notebook's folder, the PDFs
 * shown first (ws128-p002).  One already shown answers in its time.
 */
static void
app_choose(
	struct notes_app *app,
	unsigned mode)
{
	static const struct kui_file_filter filters[] = {
		{ "PDF documents", "pdf" },
		{ "All files", NULL }
	};
	static const struct kui_file_chooser_listener listener = {
		app_chooser_done
	};
	struct kui_file_chooser_options options;
	char folder[MAIN_PATH_MAX];
	const char *word;
	char *slash;

	/* One chooser at a time. */
	if (app->chooser != NULL)
		return;

	/* The notebook's folder (the home folder when the path has none). */
	(void)snprintf(folder, sizeof(folder), "%s", app->path);
	slash = strrchr(folder, '/');
	if (slash != NULL && slash != folder)
		*slash = '\0';

	/* Open shows the PDFs; Save As starts with the notebook's name. */
	memset(&options, 0, sizeof(options));
	options.mode = mode;
	options.application = MAIN_APPLICATION;
	options.folder = folder;
	options.filters = filters;
	options.filter_count = sizeof(filters) / sizeof(filters[0]);
	options.filter = 0;
	options.font = MAIN_FONT;
	if (mode == KUI_FILE_CHOOSER_SAVE) {
		options.title = "Save As";
		options.name = app->name;
	}

	/* The chooser's window over Notes'; without it the status says why. */
	app->chooser = kui_file_chooser_open(app->window.display, app->window.toplevel, &options, &listener, app);
	if (app->chooser == NULL) {
		printf("NOTES CHOOSER failed errno=%d\n", errno);
		app_status(app, "The file chooser could not be shown");
		return;
	}

	/* The tests' line. */
	app->chooser_mode = mode;
	word = "open";
	if (mode == KUI_FILE_CHOOSER_SAVE)
		word = "save";
	printf("NOTES CHOOSER open mode=%s folder=%s\n", word, folder);
}

/* The chooser answered: the path (empty when cancelled) waits for the main loop, and the chooser goes. */
static void
app_chooser_done(
	void *data,
	struct kui_file_chooser *chooser,
	unsigned result,
	const char *path,
	size_t filter)
{
	struct notes_app *app;

	/* The answer, kept for the main loop (which carries it out after the dispatch). */
	(void)filter;
	app = data;
	app->chosen[0] = '\0';
	if (result == KUI_FILE_CHOOSER_CHOSEN && path != NULL)
		(void)snprintf(app->chosen, sizeof(app->chosen), "%s", path);
	app->chosen_mode = app->chooser_mode;
	app->chosen_ready = 1;

	/* The chooser is spent. */
	kui_file_chooser_destroy(chooser);
	if (chooser == app->chooser)
		app->chooser = NULL;
}

/* Carries out what the chooser answered: nothing for a cancel, else the file opened or saved as. */
static void
app_chosen(
	struct notes_app *app)
{
	char path[MAIN_PATH_MAX];

	/* Once. */
	app->chosen_ready = 0;
	(void)snprintf(path, sizeof(path), "%s", app->chosen);

	/* Cancelled: the notebook stays as it is. */
	if (path[0] == '\0') {
		printf("NOTES CHOOSER cancelled\n");
		fflush(stdout);
		return;
	}

	/* Save As, or Open. */
	if (app->chosen_mode == KUI_FILE_CHOOSER_SAVE) {
		app_save_as(app, path);
	} else {
		app_open_file(app, path);
	}
}

/*
 * Opens another PDF in place of the notebook shown (File > Open,
 * ws128-p002): the notebook is saved first (a stroke being drawn is kept),
 * then the new one starts as at Notes' start -- its journal recovered,
 * its edit data read, or another program's PDF written on.
 */
static void
app_open_file(
	struct notes_app *app,
	const char *path)
{
	int same;
	int error;

	/* The notebook shown is opened already. */
	same = strcmp(path, app->path);
	if (same == 0) {
		app_status(app, "That notebook is open");
		return;
	}

	/* The notebook shown is kept: a stroke being drawn ends, and changes are saved. */
	if (app->live != NULL)
		app_end_contact(app, NULL);
	if (app->document.dirty) {
		error = app_save(app, "open");
		if (error != 0)
			return;
	}

	/* The notebook shown goes, with the pictures drawn of it. */
	notes_journal_destroy(app->document.journal);
	app->document.journal = NULL;
	notes_document_free(&app->document);
	memset(&app->document, 0, sizeof(app->document));
	pdf_display_list_destroy(app->background_list);
	app->background_list = NULL;
	app->background_failed = 0;
	app->background_page = NULL;
	app->picture_page = NULL;
	app->touch_page = NULL;
	app->page = 0;
	app->status[0] = '\0';
	app->status_until = 0;

	/* The new one; a notebook that cannot start at all leaves a new one at a new path. */
	error = app_start_document(app, path);
	if (error != 0) {
		printf("NOTES OPEN failed error=%d path=%s\n", error, path);
		error = app_start_document(app, NULL);
		if (error != 0) {
			fprintf(stderr, "notes: cannot start a notebook: %s\n", strerror(error));
			app->quit = 1;
			return;
		}
	}

	/* The new notebook shown from its first page, named in the title, and among the recent files. */
	app_place_page(app);
	app_set_title(app);
	(void)keiland_recent_add(app->path, MAIN_APPLICATION);
	if (app->status[0] == '\0')
		app_status(app, "Opened");
	printf("NOTES OPENED pages=%lu strokes=%lu path=%s\n", (unsigned long)app->document.page_count,
	       (unsigned long)notes_document_stroke_total(&app->document), app->path);
	fflush(stdout);
	app->toolbar_dirty = 1;
	app->redraw = 1;
}

/*
 * Saves the notebook as another file and goes on writing that one (File >
 * Save As, ws128-p002): the journal follows the new path, and the file the
 * notebook was saved as before stays as it was saved.
 */
static void
app_save_as(
	struct notes_app *app,
	const char *path)
{
	const char *slash;
	int same;

	/* The same file is a plain save. */
	same = strcmp(path, app->path);
	if (same == 0) {
		(void)app_save(app, "request");
		return;
	}

	/* The old path's journal goes (a save follows at once, so nothing is lost). */
	if (app->document.journal != NULL) {
		(void)notes_journal_discard(app->document.journal);
		notes_journal_destroy(app->document.journal);
		app->document.journal = NULL;
	}

	/* The new path, its name and its journal. */
	(void)snprintf(app->path, sizeof(app->path), "%s", path);
	slash = strrchr(app->path, '/');
	app->name = app->path;
	if (slash != NULL)
		app->name = slash + 1;
	app->document.journal = notes_journal_create(app->path);
	if (app->document.journal == NULL)
		printf("NOTES JOURNAL none error=%d\n", errno);

	/* The title, and the notebook saved there now. */
	app_set_title(app);
	(void)app_save(app, "save-as");
	app->toolbar_dirty = 1;
	app->redraw = 1;
}

/* Places the notebook's first page in the window, for the pointer and the fingers (at the start, and after Open). */
static void
app_place_page(
	struct notes_app *app)
{
	/* The page's place, and the fingers' with it. */
	notes_view_layout(&app->view, app->renderer.extent.width, app->renderer.extent.height,
			  app->document.pages[0]->width, app->document.pages[0]->height);
	notes_touch_layout(&app->touch, app->renderer.extent.width, app->renderer.extent.height, (float)NOTES_TOOLBAR_HEIGHT, NOTES_PAGE_MARGIN,
			   app->document.pages[0]->width, app->document.pages[0]->height, app->view.scale);
}
