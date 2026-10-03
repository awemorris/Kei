/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The desktop's shared widgets and controls (WS090, plan/ws090/design.md):
 * one library the desktop's applications draw their parts with, so that a
 * button, a list or a scroll looks and feels the same in every one of them.
 *
 * The first layer (KUI_VERSION 1) is the drawing: a CPU canvas of
 * premultiplied BGRA pixels, the text drawn on it from TrueType fonts, the
 * icons made of its shapes, and the theme -- the colours and sizes of the
 * Kei look.  It began as the file manager's drawing surface (Files'
 * canvas.c, text.c and icons.c) and Settings' line pictures, moved here
 * unchanged so that an application moved onto the library draws the same
 * pixels as before.  The second (KUI_VERSION 2) is the scroll, the input
 * that finds which part of a frame a pointer, a wheel or a finger meant,
 * and the touch of a view of editable text.  The third (KUI_VERSION 3) is
 * the window: a Wayland toplevel whose input arrives as a queue of
 * events, whose CPU-drawn frames are shown through Vulkan or shared
 * memory, and which holds the clipboard and the primary selection.  The
 * fourth (KUI_VERSION 4) is the widgets: buttons, switches, sliders, text
 * fields, lists, sidebars, cards and their rows, dialogs, chips and
 * progress bars, drawn in the Kei look of Files and Settings, and the
 * keyboard's focus among them.
 *
 * Times are CLOCK_MONOTONIC microseconds throughout (the clock of
 * libkeiland's touch motion, scroller and gestures).
 *
 * Nothing in this layer knows about Wayland or Vulkan.  A window's frame
 * is drawn into a canvas and handed to its presenter, and host tests draw
 * into a canvas of their own and write it out as a picture.  Every call
 * is made from one thread.  The library's name is internal and never
 * appears in what a user reads.
 */

#ifndef KEIUI_H
#define KEIUI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The interface version this header describes (1: the drawing -- canvas, text, icons and the theme; 2: the scroll, the input and the text view's touch; 3: the window, the clipboard and the primary selection; 4: the widgets, the keyboard's focus and the theme's controls; 5: the file chooser, moved from libkeiland, and a list's touched rows; 6: the window's text input, text an input method or the on-screen keyboard sends; 7: the on-screen keyboard's inset and the caret kept in sight; 8: the editing operations of the on-screen keyboard's buttons; 9: colour emoji from the emoji font, a third face; 10: the window's full screen, asked for and as configured; 11: waiting for other descriptors with the compositor, the pointer's input at the compositor's time, and a window made fullscreen; 12: the overlay scroll bar). */
#define KUI_VERSION	12U

/*
 * Reports the interface version of the library that was loaded.
 *
 * A program built against this header may compare the result with
 * KUI_VERSION to learn whether the library it runs with is older.
 */
unsigned kui_version(void);

/* How deep the clip rectangles nest. */
#define KUI_CANVAS_CLIPS		16

/* The most corners a polygon may have. */
#define KUI_POLYGON_POINTS	96

/*
 * How many fonts the text draws from: the main one, a fallback, and the
 * colour emoji font (KUI_TEXT_EMOJI, opened the first time a character
 * neither of the others has is drawn; version 9).
 */
#define KUI_TEXT_FACES		3
#define KUI_TEXT_EMOJI		"/usr/share/fonts/keiland-emoji.ttf"

/* A color as 0xAARRGGBB, not premultiplied. */
typedef uint32_t kui_color;

/* An opaque color from 0xRRGGBB. */
#define KUI_RGB(value)		((kui_color)(0xff000000U | (uint32_t)(value)))

/* A color from 0xRRGGBB and an alpha from 0 to 255. */
#define KUI_RGBA(value, alpha)	((kui_color)(((uint32_t)(alpha) << 24) | ((uint32_t)(value) & 0xffffffU)))

/*
 * A rectangle of whole pixels.
 *
 * It is a plain value: a layout computes one, the drawing and the hit test
 * both read it.
 */
struct kui_rect {
	int x;
	int y;
	int width;
	int height;
};

/*
 * A picture in memory, premultiplied BGRA (0xAARRGGBB words).
 *
 * The pixels belong to whoever made the image: kui_image_create allocates
 * them and kui_image_release frees them.
 */
struct kui_image {
	uint32_t *pixels;
	int width;
	int height;
	size_t stride;
};

/*
 * A surface to draw on.
 *
 * The pixels are the caller's; the canvas adds the clip rectangles and the
 * scratch row the polygon filler accumulates coverage in, which live as
 * long as the canvas.
 */
struct kui_canvas {
	/* The pixels, the words in a row and the size. */
	uint32_t *pixels;
	size_t stride;
	int width;
	int height;

	/* The clip in force, and the ones it replaced (innermost last). */
	struct kui_rect clip;
	struct kui_rect clips[KUI_CANVAS_CLIPS];
	int clip_depth;

	/* One row of polygon coverage, a float per pixel and one more. */
	float *coverage;
};

/*
 * One glyph drawn at one size, kept for the next time.
 *
 * key is zero for an empty slot; the bitmap is the glyph's coverage, or
 * pixels its colour (premultiplied 0xAARRGGBB, a colour emoji, version 9)
 * with bitmap NULL.
 */
struct kui_glyph {
	uint32_t key;
	int width;
	int height;
	int left;
	int top;
	int advance;
	uint8_t *bitmap;
	uint32_t *pixels;
};

/*
 * One font file: its bytes (kept for the face) and the face.
 */
struct kui_text_face {
	void *data;
	size_t size;
	struct truetype_face *face;
	unsigned pixels;
};

/*
 * The text of the window: the fonts and every glyph drawn so far.
 *
 * One lives for the whole program.  The cache is emptied when it fills up,
 * which only costs drawing the glyphs again.  emoji_tried says the emoji
 * font (faces[2]) was looked for (version 9).
 */
struct kui_text {
	struct kui_text_face faces[KUI_TEXT_FACES];
	int face_count;
	int emoji_tried;
	struct kui_glyph *cache;
	unsigned cache_size;
	unsigned cache_used;
	uint8_t *scratch;
	size_t scratch_size;
};

/*
 * The vertical measurements of text at one size, in pixels.
 */
struct kui_text_line {
	int ascent;
	int descent;
	int height;
};

/*
 * The icons drawn with lines (sidebar, toolbar) or filled shapes (items).
 */
enum kui_icon {
	KUI_ICON_HOME,
	KUI_ICON_DESKTOP,
	KUI_ICON_DOCUMENTS,
	KUI_ICON_DOWNLOADS,
	KUI_ICON_PICTURES,
	KUI_ICON_MUSIC,
	KUI_ICON_MOVIES,
	KUI_ICON_FOLDER_LINE,
	KUI_ICON_RECENTS,
	KUI_ICON_TRASH,
	KUI_ICON_COMPUTER,
	KUI_ICON_VOLUME,
	KUI_ICON_BACK,
	KUI_ICON_FORWARD,
	KUI_ICON_SEARCH,
	KUI_ICON_GRID,
	KUI_ICON_LIST,
	KUI_ICON_PREVIEW,
	KUI_ICON_CHEVRON,
	KUI_ICON_CLOSE,
	KUI_ICON_PLUS,
	KUI_ICON_UP,
	KUI_ICON_DOWN,

	/* The line pictures (Settings' pages), from KUI_ICON_TILES on, in the order Settings numbered them. */
	KUI_ICON_TILES,
	KUI_ICON_WIFI,
	KUI_ICON_ETHERNET,
	KUI_ICON_BLUETOOTH,
	KUI_ICON_SHIELD,
	KUI_ICON_GLOBE,
	KUI_ICON_PALETTE,
	KUI_ICON_PICTURE,
	KUI_ICON_BELL,
	KUI_ICON_SPEAKER,
	KUI_ICON_MONITOR,
	KUI_ICON_DISK,
	KUI_ICON_BATTERY,
	KUI_ICON_KEYBOARD,
	KUI_ICON_MOUSE,
	KUI_ICON_TOUCHPAD,
	KUI_ICON_PRINTER,
	KUI_ICON_SHARE,
	KUI_ICON_PEOPLE,
	KUI_ICON_EYE,
	KUI_ICON_LOCK,
	KUI_ICON_PERSON,
	KUI_ICON_REFRESH,
	KUI_ICON_INFO,
	KUI_ICON_DISCLOSURE
};

/* The canvas (canvas.c). */
int kui_canvas_init(struct kui_canvas *canvas, uint32_t *pixels, size_t stride, int width, int height);
void kui_canvas_release(struct kui_canvas *canvas);
void kui_canvas_clip_push(struct kui_canvas *canvas, const struct kui_rect *rect);
void kui_canvas_clip_pop(struct kui_canvas *canvas);
void kui_canvas_clear(struct kui_canvas *canvas);
void kui_canvas_fill(struct kui_canvas *canvas, const struct kui_rect *rect, kui_color color);
void kui_canvas_gradient(struct kui_canvas *canvas, const struct kui_rect *rect, kui_color top, kui_color bottom);
void kui_canvas_round(struct kui_canvas *canvas, float x, float y, float width, float height, float radius, kui_color color);
void kui_canvas_round_gradient(struct kui_canvas *canvas, float x, float y, float width, float height, float radius, kui_color top, kui_color bottom);
void kui_canvas_round_border(struct kui_canvas *canvas, float x, float y, float width, float height, float radius, float thickness, kui_color color);
void kui_canvas_shadow(struct kui_canvas *canvas, float x, float y, float width, float height, float radius, float softness, kui_color color);
void kui_canvas_circle(struct kui_canvas *canvas, float cx, float cy, float radius, kui_color color);
void kui_canvas_ring(struct kui_canvas *canvas, float cx, float cy, float radius, float thickness, float fraction, kui_color color);
void kui_canvas_polygon(struct kui_canvas *canvas, const float *points, int count, kui_color color);
void kui_canvas_line(struct kui_canvas *canvas, float x0, float y0, float x1, float y1, float thickness, kui_color color);
void kui_canvas_mask(struct kui_canvas *canvas, int x, int y, const uint8_t *mask, int width, int height, size_t stride, kui_color color);
void kui_canvas_image(struct kui_canvas *canvas, const struct kui_image *image, float x, float y, float width, float height, float radius, float opacity);
int kui_image_create(struct kui_image *image, int width, int height);
void kui_image_release(struct kui_image *image);
void kui_image_scale(const struct kui_image *source, struct kui_image *target);
kui_color kui_color_mix(kui_color from, kui_color to, float amount);

/* The text (text.c). */
int kui_text_open(struct kui_text *text, const char *primary, const char *fallback);
void kui_text_close(struct kui_text *text);
void kui_text_metrics(struct kui_text *text, unsigned pixels, struct kui_text_line *line);
int kui_text_center(unsigned pixels, int top, int height);
int kui_text_width(struct kui_text *text, const char *string, size_t length, unsigned pixels, int bold);
int kui_text_draw(struct kui_text *text, struct kui_canvas *canvas, int x, int baseline, const char *string, size_t length, unsigned pixels, int bold, kui_color color);
int kui_text_draw_fit(struct kui_text *text, struct kui_canvas *canvas, int x, int baseline, const char *string, unsigned pixels, int bold, int width, kui_color color);
size_t kui_text_fit(struct kui_text *text, const char *string, unsigned pixels, int bold, int width, char *out, size_t size);
size_t kui_text_break(struct kui_text *text, const char *string, unsigned pixels, int bold, int width);
uint32_t kui_utf8_next(const char *string, size_t length, size_t *index);

/*
 * The theme: the colours and sizes of the Kei look, which every widget
 * draws with (the file manager's values, plan/ws071/spec.md).  One theme
 * exists so far, the light one; an application reads it and does not
 * change it.
 */
struct kui_theme {
	/* The window's ground (a vertical gradient) and the cards on it. */
	kui_color ground_top;
	kui_color ground_bottom;
	kui_color panel;
	kui_color panel_edge;
	kui_color shadow;

	/* A sidebar and a content card standing on glass, and on the plain ground. */
	kui_color glass_sidebar;
	kui_color glass_content;
	kui_color sidebar;

	/* Text: the main ink, the secondary and the faint, and an icon's ink. */
	kui_color text;
	kui_color text_secondary;
	kui_color text_faint;
	kui_color icon;

	/* The accent, a selection with and without the keyboard, the pointer's hover, a separator, a folder and danger. */
	kui_color accent;
	kui_color selection;
	kui_color selection_inactive;
	kui_color hover;
	kui_color separator;
	kui_color folder;
	kui_color danger;

	/* A card's and a control's corner radius, a list row's height, and the text sizes of body, secondary and title text. */
	float card_radius;
	float control_radius;
	int row_height;
	unsigned text_body;
	unsigned text_small;
	unsigned text_title;

	/*
	 * KUI_VERSION 4 (Settings' values, plan/ws089): a card within a page
	 * and its edge, the line between a card's rows, a control's ground and
	 * edge, a switch's track when off, good and bad news, a control's
	 * height, and a switch's size.
	 */
	kui_color card;
	kui_color card_edge;
	kui_color row_separator;
	kui_color control;
	kui_color control_edge;
	kui_color track;
	kui_color good;
	kui_color bad;
	int control_height;
	int switch_width;
	int switch_height;
};

/* The theme (theme.c). */
const struct kui_theme *kui_theme_default(void);

/* The icons (icons.c and icons-line.c). */
void kui_icon_draw(struct kui_canvas *canvas, enum kui_icon icon, float x, float y, float size, kui_color color);
void kui_icon_folder(struct kui_canvas *canvas, float x, float y, float size, kui_color tint);
void kui_icon_file(struct kui_canvas *canvas, struct kui_text *text, float x, float y, float size, kui_color band, const char *label);
void kui_icon_tag(struct kui_canvas *canvas, float cx, float cy, float radius, kui_color color);

/*
 * The scroll (scroll.c, KUI_VERSION 2): the state of one part of a window
 * whose content is larger than the part, which an application keeps and
 * draws its content at (x, y) of.
 *
 * The wheel and the keys glide the content to where they send it (the
 * distance left shrinks by e every KUI_SCROLL_GLIDE_US); a finger drags it
 * and lets it fly on with libkeiland's scroller (the inertia and the
 * rubber band past an end are the scroller's).  Outside a finger's hold
 * the position stays within 0..content-viewport on each axis that
 * scrolls.  The scroll bars show while the content moves and fade out
 * over KUI_SCROLL_FADE_US after it stops.
 *
 * The fields are read by the application (x and y above all); they are
 * written only through the calls.  Nothing here draws but
 * kui_scroll_draw_bars, so a program that draws its own content with
 * Vulkan (Terminal, Notes) uses the same scroll without the canvas.
 */
struct keiland_scroller;

/* The axes a scroll moves along. */
#define KUI_SCROLL_X		1U
#define KUI_SCROLL_Y		2U

/* How quickly a glide closes on its target (the time constant), and how long the bars take to fade. */
#define KUI_SCROLL_GLIDE_US	70000U
#define KUI_SCROLL_FADE_US	1000000U

struct kui_scroll {
	/* The axes it moves along, the position drawn, and the sizes of the content and of the part that shows it. */
	unsigned axes;
	double x;
	double y;
	double content_width;
	double content_height;
	double viewport_width;
	double viewport_height;

	/* A glide of the wheel or the keys: where it started, where it goes, and when it started. */
	int gliding;
	double from_x;
	double from_y;
	double to_x;
	double to_y;
	uint64_t glide_us;

	/* The finger's scroller, whether it owns the content (a finger holds it or it flies on), and whether the finger has lifted. */
	struct keiland_scroller *scroller;
	int touched;
	int released;

	/* When the content last moved (for the bars), 0 before it ever moved. */
	uint64_t moved_us;
};

int kui_scroll_init(struct kui_scroll *scroll, unsigned axes);
void kui_scroll_release(struct kui_scroll *scroll);
void kui_scroll_set_size(struct kui_scroll *scroll, double content_width, double content_height, double viewport_width, double viewport_height);
void kui_scroll_wheel(struct kui_scroll *scroll, double dx, double dy, uint64_t now_us);
void kui_scroll_move_to(struct kui_scroll *scroll, double x, double y, int glide, uint64_t now_us);
void kui_scroll_reveal(struct kui_scroll *scroll, const struct kui_rect *rect, uint64_t now_us);
int kui_scroll_key(struct kui_scroll *scroll, uint32_t key, unsigned modifiers, double line, uint64_t now_us);
int kui_scroll_press(struct kui_scroll *scroll, uint64_t now_us);
void kui_scroll_drag(struct kui_scroll *scroll, double dx, double dy);
void kui_scroll_fling(struct kui_scroll *scroll, double vx, double vy, uint64_t now_us);
void kui_scroll_cancel(struct kui_scroll *scroll, uint64_t now_us);
int kui_scroll_step(struct kui_scroll *scroll, uint64_t now_us);
double kui_scroll_limit_x(const struct kui_scroll *scroll);
double kui_scroll_limit_y(const struct kui_scroll *scroll);
int kui_scroll_draw_bars(const struct kui_scroll *scroll, struct kui_canvas *canvas, const struct kui_rect *viewport, const struct kui_theme *theme, uint64_t now_us);

/*
 * The overlay scroll bar (scroll-bar.c, KUI_VERSION 12, ws127-p002): the
 * vertical bar of a view, drawn over its content's right edge the way
 * macOS draws one (the user's choice of 2026-10-02).  It comes out thin
 * while the content moves, grows thick (with a faint track) while the
 * pointer is near the edge or drags it, and fades a while after the last
 * of these.  A press on the thumb drags the content; a press on the track
 * moves it a page towards the press.
 *
 * The state knows nothing of the window or of drawing: the application
 * tells it what happened (its sizes are in the content's pixels, offset
 * is how far the content is scrolled), asks for the shape to draw with its
 * own canvas (kui_scroll_bar_draw draws it on a kui_canvas), and draws
 * again while kui_scroll_bar_busy says the bar still changes.  The times
 * are microseconds of one clock.
 */
#define KUI_SCROLL_BAR_THIN	6
#define KUI_SCROLL_BAR_THICK	11
#define KUI_SCROLL_BAR_REACH	16
#define KUI_SCROLL_BAR_GAP	2
#define KUI_SCROLL_BAR_MIN	28
#define KUI_SCROLL_BAR_SHOW_US	1000000U
#define KUI_SCROLL_BAR_FADE_US	400000U

/*
 * The state of one overlay bar.  active_us is when the content last moved
 * or the pointer last came near or dragged (0 before any); near says the
 * pointer is over the bar's band; dragging and grab (where in the thumb the
 * drag holds it) belong to a press on the thumb.  Zeroed, it is a bar that
 * has not shown yet.
 */
struct kui_scroll_bar {
	uint64_t active_us;
	int near;
	int dragging;
	double grab;
};

/*
 * What to draw now: the track (shown while the bar is thick) and the
 * thumb, in the window's pixels, and the strength of the ink (0 to 1).
 */
struct kui_scroll_bar_shape {
	double track_x;
	double track_y;
	double track_width;
	double track_height;
	double thumb_x;
	double thumb_y;
	double thumb_width;
	double thumb_height;
	double alpha;
	int thick;
};

void kui_scroll_bar_moved(struct kui_scroll_bar *bar, uint64_t now_us);
int kui_scroll_bar_hover(struct kui_scroll_bar *bar, const struct kui_rect *viewport, double content, double x, double y, uint64_t now_us);
int kui_scroll_bar_leave(struct kui_scroll_bar *bar, uint64_t now_us);
int kui_scroll_bar_shape(const struct kui_scroll_bar *bar, const struct kui_rect *viewport, double content, double offset, uint64_t now_us, struct kui_scroll_bar_shape *shape);
int kui_scroll_bar_press(struct kui_scroll_bar *bar, const struct kui_rect *viewport, double content, double offset, double x, double y, uint64_t now_us, double *new_offset);
int kui_scroll_bar_drag(struct kui_scroll_bar *bar, const struct kui_rect *viewport, double content, double y, uint64_t now_us, double *new_offset);
int kui_scroll_bar_release(struct kui_scroll_bar *bar, uint64_t now_us);
int kui_scroll_bar_busy(const struct kui_scroll_bar *bar, uint64_t now_us);
int kui_scroll_bar_draw(const struct kui_scroll_bar *bar, struct kui_canvas *canvas, const struct kui_rect *viewport, double content, double offset, uint64_t now_us);

/*
 * The keys (input.c, KUI_VERSION 2).  zdesktop forwards evdev key codes
 * with no keymap; the library carries the US layout, as the desktop's
 * programs do, until an input method arrives (WS095).
 */
#define KUI_KEY_ESC		1U
#define KUI_KEY_BACKSPACE	14U
#define KUI_KEY_TAB		15U
#define KUI_KEY_ENTER		28U
#define KUI_KEY_SPACE		57U
#define KUI_KEY_KPENTER	96U
#define KUI_KEY_HOME		102U
#define KUI_KEY_UP		103U
#define KUI_KEY_PAGEUP		104U
#define KUI_KEY_LEFT		105U
#define KUI_KEY_RIGHT		106U
#define KUI_KEY_END		107U
#define KUI_KEY_DOWN		108U
#define KUI_KEY_PAGEDOWN	109U
#define KUI_KEY_DELETE		111U

/* The modifiers held. */
#define KUI_MOD_SHIFT		1U
#define KUI_MOD_CTRL		2U
#define KUI_MOD_ALT		4U
#define KUI_MOD_SUPER		8U

uint32_t kui_key_character(uint32_t key, unsigned modifiers);

/*
 * The touch of a view of editable text (text-touch.c, KUI_VERSION 2,
 * plan/ws090/design.md section 6.1): in a text editor's body and a text
 * field, one finger's drag selects and two fingers scroll.
 *
 * A tap puts the caret, a double tap selects a word, one finger's drag
 * selects from where it touched (the content scrolls by itself while the
 * finger is near the view's edge), and a long press asks for the context
 * menu.  A selection made by touch shows a handle at each end; dragging a
 * handle moves that end.  Two fingers are the scroll's (kui_ui gives them
 * to it).
 *
 * The view gives three answers in its content's coordinates (the scroll
 * already undone): the text position nearest a point, the caret's
 * rectangle at a position, and the word around a position.  Positions are
 * whatever the view counts in (byte offsets in Text Editor).
 */
struct kui_text_view {
	size_t (*position_at)(void *data, double x, double y);
	void (*caret_rect)(void *data, size_t position, struct kui_rect *rect);
	void (*word_at)(void *data, size_t position, size_t *start, size_t *end);
};

/* What the fingers changed, for kui_text_touch_take. */
#define KUI_TEXT_TOUCH_SELECTION	1U
#define KUI_TEXT_TOUCH_MENU		2U

/* The handles: their drawn diameter and the diameter a finger finds them within. */
#define KUI_TEXT_HANDLE		12
#define KUI_TEXT_HANDLE_REACH	44

/* How near the view's edge a selecting finger scrolls the content, and how fast at the edge (pixels a second). */
#define KUI_TEXT_EDGE		24
#define KUI_TEXT_EDGE_SPEED	1200.0

/* Which end of the selection a handle drag moves. */
#define KUI_TEXT_HANDLE_NONE	0
#define KUI_TEXT_HANDLE_ANCHOR	1
#define KUI_TEXT_HANDLE_CARET	2

struct kui_text_touch {
	/* The view's answers and their data. */
	const struct kui_text_view *view;
	void *data;

	/* The selection: from the anchor to the caret (equal: only a caret). */
	size_t anchor;
	size_t caret;

	/*
	 * Whether a finger is selecting, which handle it holds, whether the
	 * handles show, the finger (content coordinates), and how far the finger
	 * holding a handle is from the middle of its end's caret (subtracted, so
	 * that the end follows the caret's line, not the knob's below it).
	 */
	int selecting;
	int handle;
	int handles;
	double finger_x;
	double finger_y;
	double grip_x;
	double grip_y;

	/* What changed since kui_text_touch_take, and where the context menu was asked for (window coordinates). */
	unsigned changes;
	double menu_x;
	double menu_y;
};

void kui_text_touch_init(struct kui_text_touch *touch, const struct kui_text_view *view, void *data);
void kui_text_touch_set_selection(struct kui_text_touch *touch, size_t anchor, size_t caret);
void kui_text_touch_tap(struct kui_text_touch *touch, double x, double y, int twice);
void kui_text_touch_long_press(struct kui_text_touch *touch, double window_x, double window_y);
void kui_text_touch_drag_begin(struct kui_text_touch *touch, double x, double y);
void kui_text_touch_drag(struct kui_text_touch *touch, double x, double y);
void kui_text_touch_drag_end(struct kui_text_touch *touch);
int kui_text_touch_edge(const struct kui_text_touch *touch, const struct kui_scroll *scroll, double *vx, double *vy);
unsigned kui_text_touch_take(struct kui_text_touch *touch);
void kui_text_touch_draw_handles(const struct kui_text_touch *touch, struct kui_canvas *canvas, double origin_x, double origin_y, const struct kui_theme *theme);

/*
 * The input of a window (ui.c, KUI_VERSION 2, design section 4): which
 * part of a frame a pointer, the wheel or a finger meant.
 *
 * While a frame is drawn, each part that takes input is recorded: a
 * widget (kui_ui_hit, by an id the application chooses and an index), a
 * scroll's viewport (kui_ui_scroll_region) and a view of editable text
 * (kui_ui_text_region).  Input that arrives before the next frame is
 * resolved against the parts of the frame last drawn, the latest recorded
 * first (on top): a press and a release on the same widget click it (twice
 * within 400 ms: a double click), the wheel goes to the scroll under the
 * pointer, and a finger goes by libkeiland's gestures: a tap to the widget
 * it touched (else to the text view there), a drag to the scroll or text
 * view it touched (a widget such as a list's row inside a scroll does not
 * take a drag; the scroll does).  In a text view one finger's drag selects
 * and two fingers' drag scrolls (kui_text_touch).  Input that meets no
 * part is kept for the application (kui_ui_take; a drag's distance from
 * kui_ui_drag_offset).  Each input call returns 1 when the window must
 * draw again.
 *
 * A scroll or a text touch given to kui_ui_scroll_region or
 * kui_ui_text_region must live until the next frame is drawn (the input
 * in between reaches it).
 */
struct kui_ui;

/* What a widget's record reports of the input (bits). */
#define KUI_HIT_HOT		1U	/* the pointer is over it */
#define KUI_HIT_ACTIVE		2U	/* a press on it is held */
#define KUI_HIT_CLICKED	4U	/* pressed and released on it since the last frame */
#define KUI_HIT_DOUBLE		8U	/* the click was the second of a double click or tap */
#define KUI_HIT_FOCUSED	16U	/* it has the keyboard's focus (a widget that takes the keyboard) */
#define KUI_HIT_TOUCHED	32U	/* KUI_VERSION 5: the click was a finger's tap */

/* The input no part took. */
#define KUI_EVENT_PRESS		1U
#define KUI_EVENT_RELEASE	2U
#define KUI_EVENT_WHEEL		3U
#define KUI_EVENT_TAP		4U
#define KUI_EVENT_DOUBLE_TAP	5U
#define KUI_EVENT_LONG_PRESS	6U
#define KUI_EVENT_DRAG_BEGIN	7U
#define KUI_EVENT_DRAG_END	8U

/*
 * One input no part took: its kind, where (window coordinates), the
 * wheel's distance or a drag's velocity, the fingers down, and the id of
 * the region it happened over (a long press over a text view: that view's
 * id, 0 over none).
 */
struct kui_event {
	unsigned kind;
	double x;
	double y;
	double dx;
	double dy;
	unsigned fingers;
	uint32_t region;
	uint32_t code;
	unsigned modifiers;
};

struct kui_ui *kui_ui_create(void);
void kui_ui_destroy(struct kui_ui *ui);
int kui_ui_pointer_motion(struct kui_ui *ui, double x, double y);
int kui_ui_pointer_leave(struct kui_ui *ui);
int kui_ui_pointer_button(struct kui_ui *ui, int pressed, uint64_t now_us);
int kui_ui_wheel(struct kui_ui *ui, double dx, double dy, uint64_t now_us);
int kui_ui_touch_down(struct kui_ui *ui, int32_t id, uint64_t time_us, uint64_t now_us, double x, double y);
int kui_ui_touch_motion(struct kui_ui *ui, int32_t id, uint64_t time_us, uint64_t now_us, double x, double y);
int kui_ui_touch_up(struct kui_ui *ui, int32_t id, uint64_t time_us, uint64_t now_us);
int kui_ui_touch_cancel(struct kui_ui *ui, uint64_t now_us);
void kui_ui_begin(struct kui_ui *ui, uint64_t now_us);
unsigned kui_ui_hit(struct kui_ui *ui, uint32_t id, uint32_t index, const struct kui_rect *rect);
void kui_ui_scroll_region(struct kui_ui *ui, uint32_t id, const struct kui_rect *rect, struct kui_scroll *scroll);
void kui_ui_text_region(struct kui_ui *ui, uint32_t id, const struct kui_rect *rect, struct kui_scroll *scroll, struct kui_text_touch *touch);
int kui_ui_end(struct kui_ui *ui, uint64_t now_us);
int kui_ui_take(struct kui_ui *ui, struct kui_event *event);
int kui_ui_drag_offset(struct kui_ui *ui, uint64_t now_us, double *dx, double *dy);

/*
 * The window (window.c, present.c, present-shm.c, clipboard.c,
 * primary.c; KUI_VERSION 3, plan/ws090/design.md section 5): an
 * xdg-shell toplevel of its own connection, its seat's input, the frames
 * the application draws on the CPU, and the clipboard and the primary
 * selection.
 *
 * The input arrives as events in a queue the application takes after
 * each kui_window_dispatch, in the order they came: the pointer, the
 * wheel, the keys (a held key repeats: the application calls
 * kui_window_repeat after the dispatch, so that a release read in the
 * same dispatch stops it first, BUG-111), the keyboard's focus, the
 * fingers (their times turned into CLOCK_MONOTONIC microseconds), a new
 * size and the request to close.  An application posts its own inputs
 * heard through other objects during a dispatch (a System Menu's shortcut,
 * a titlebar's control) with kui_window_post, so that they keep their
 * place among the keys (typed text, then Ctrl+S).  Input on the program's other surfaces
 * (a file chooser's window) is not the window's and never queued.
 *
 * A frame is ordinary memory of premultiplied 0xAARRGGBB words the size
 * kui_window_present_resize reported.  KUI_PRESENT_VULKAN shows it through
 * a Vulkan swapchain (see-through when the compositor offers it, the way
 * zdesktop's glass needs), KUI_PRESENT_SHM through wl_shm buffers (for a
 * small window of a library, or where Vulkan is missing), and
 * KUI_PRESENT_NONE leaves the surface to the application's own Vulkan.
 * The menus, the titlebar's controls and the glass panels stay the
 * application's (libkeiland), on the objects the accessors give.
 */
struct kui_window;
struct wl_display;
struct wl_surface;
struct wl_seat;
struct xdg_toplevel;

/* How the frames are shown. */
#define KUI_PRESENT_VULKAN	0U
#define KUI_PRESENT_SHM		1U
#define KUI_PRESENT_NONE	2U

/* The kinds of input. */
#define KUI_WINDOW_MOTION	1U
#define KUI_WINDOW_LEAVE	2U
#define KUI_WINDOW_BUTTON	3U
#define KUI_WINDOW_AXIS		4U
#define KUI_WINDOW_KEY		5U
#define KUI_WINDOW_FOCUS	6U
#define KUI_WINDOW_TOUCH_DOWN	7U
#define KUI_WINDOW_TOUCH_MOTION	8U
#define KUI_WINDOW_TOUCH_UP	9U
#define KUI_WINDOW_TOUCH_CANCEL	10U
#define KUI_WINDOW_RESIZE	11U
#define KUI_WINDOW_CLOSE	12U
#define KUI_WINDOW_POST		13U

/*
 * KUI_VERSION 6: the text an input method or zdesktop's on-screen keyboard
 * sends through the text input (text-input-unstable-v3), while the window
 * asks for it (kui_window_text_input): text to insert at the caret in place
 * of the selection (text), the text being composed to show at the caret
 * until it is committed or replaced (text, empty when it goes; begin and end
 * are its cursor's byte offsets, -1 when hidden), and bytes to delete
 * before and after the caret first (before, after).  They come in the order
 * of the protocol's done: delete, commit, preedit.
 */
#define KUI_WINDOW_TEXT_COMMIT	14U
#define KUI_WINDOW_TEXT_PREEDIT	15U
#define KUI_WINDOW_TEXT_DELETE	16U

/* The longest text one input carries, with its NUL (a longer one is cut at a character's start). */
#define KUI_WINDOW_TEXT_MAX	256U

/* The evdev codes of the pointer's buttons. */
#define KUI_BUTTON_LEFT		0x110U
#define KUI_BUTTON_RIGHT	0x111U
#define KUI_BUTTON_MIDDLE	0x112U

/*
 * What a window is made with.  Any pointer may be NULL: display (the
 * WAYLAND_DISPLAY one), title and application (the app_id).  width and
 * height are the size asked for until the compositor gives one.
 * fullscreen (KUI_VERSION 11) asks for the full screen before the window
 * is first configured, so that its first configure is the full screen's.
 */
struct kui_window_options {
	const char *display;
	const char *title;
	const char *application;
	uint32_t width;
	uint32_t height;
	unsigned present;
	int fullscreen;
};

/*
 * One input: its kind (KUI_WINDOW_*), where the pointer or the finger is
 * (surface pixels), a button's or a key's code and whether it is pressed
 * (a focus: 1 when it came), whether a key is a repeat, the modifiers held
 * (KUI_MOD_*), the wheel's distance in pixels, a finger's id and the time
 * it happened (a finger's, and since KUI_VERSION 11 the pointer's motions and
 * buttons, from the compositor's time; otherwise when it was read), when it
 * was read, and its serial (a press's, for a popup or
 * a selection); for the text input's (KUI_VERSION 6), its text, the
 * composed text's cursor, and the bytes to delete around the caret.
 */
struct kui_window_event {
	unsigned kind;
	double x;
	double y;
	uint32_t code;
	int pressed;
	int repeated;
	unsigned modifiers;
	double dx;
	double dy;
	int32_t id;
	uint64_t time_us;
	uint64_t arrival_us;
	uint32_t serial;
	char text[KUI_WINDOW_TEXT_MAX];
	int32_t begin;
	int32_t end;
	uint32_t before;
	uint32_t after;
};

struct kui_window *kui_window_open(const struct kui_window_options *options);
void kui_window_close(struct kui_window *window);
int kui_window_dispatch(struct kui_window *window, int timeout_ms);
int kui_window_take(struct kui_window *window, struct kui_window_event *event);
void kui_window_post(struct kui_window *window, uint32_t code);
int kui_window_repeat(struct kui_window *window, uint64_t now_us);
int kui_window_repeat_wait(const struct kui_window *window, uint64_t now_us);
void kui_window_set_title(struct kui_window *window, const char *title);
void kui_window_size(const struct kui_window *window, uint32_t *width, uint32_t *height);
int kui_window_present_resize(struct kui_window *window, uint32_t *width, uint32_t *height);
int kui_window_present(struct kui_window *window, const uint32_t *pixels, size_t stride);
int kui_window_see_through(const struct kui_window *window);
struct wl_display *kui_window_display(const struct kui_window *window);
struct wl_surface *kui_window_surface(const struct kui_window *window);
struct xdg_toplevel *kui_window_toplevel(const struct kui_window *window);
uint32_t kui_window_serial(const struct kui_window *window);
uint32_t kui_window_press_serial(const struct kui_window *window);
void kui_window_set_serial(struct kui_window *window, uint32_t serial);
struct wl_seat *kui_window_seat(const struct kui_window *window);
void kui_window_copy(struct kui_window *window, const char *text, size_t length);
size_t kui_window_paste(struct kui_window *window, char *text, size_t size);
int kui_window_can_paste(const struct kui_window *window);
void kui_window_text_input(struct kui_window *window, int enabled);
void kui_window_text_cursor(struct kui_window *window, int x, int y, int width, int height);
void kui_window_select(struct kui_window *window, const char *text, size_t length);

/*
 * KUI_VERSION 7 (ws102-p015, plan/ws102/design.md section 2.8): the
 * on-screen keyboard's inset.  zdesktop tells a window how much of it the
 * keyboard covers, in the window's pixels from its right edge (the flick
 * panel's column) and from its bottom edge (the QWERTY row), when the
 * keyboard opens, closes or changes the window (before that configure);
 * both are 0 when it has closed or does not cover the window.  With a
 * compositor that does not tell, nothing happens.
 *
 * By default the caret is kept in sight: when the keyboard comes or
 * changes, the next kui_ui_end moves the scroll of the frame's text view
 * (kui_ui_text_region; the one with the keyboard's focus, else the last
 * recorded) so that the caret's line is in the middle of the part of the
 * view the keyboard leaves, as far as the scroll goes (not past the text's
 * start or end; a view that does not scroll stays).  The application may
 * hear the inset first: its callback returns 1 when it took care of it
 * itself (the default is skipped), 0 to keep the default.
 */
#define KUI_KEYBOARD_INSET_NONE		0U
#define KUI_KEYBOARD_INSET_RIGHT	1U
#define KUI_KEYBOARD_INSET_BOTTOM	2U
typedef int (*kui_keyboard_inset_fn)(void *data, int right, int bottom, unsigned reason);
void kui_window_on_keyboard_inset(struct kui_window *window, kui_keyboard_inset_fn callback, void *data);
void kui_window_keyboard_inset(const struct kui_window *window, int *right, int *bottom);

/*
 * KUI_VERSION 8 (ws102-p017, plan/ws102/design.md section 2.10): the
 * editing operations the on-screen keyboard's buttons ask for.  A window
 * tells zdesktop it carries all of them out and its state, and hears them.
 * By default each becomes the keys it stands for, queued as the window's
 * own key inputs (copy Ctrl+C, cut Ctrl+X, paste Ctrl+V, undo Ctrl+Z, redo
 * Ctrl+Shift+Z, select all Ctrl+A), and select_begin and select_end start
 * and end a selection: while it is made, the keys that move the caret
 * (the arrows, Home, End, Page Up and Page Down) come with Shift, and a
 * copy or a cut ends it.  An application that knows its state tells it
 * (kui_window_edit_state: KUI_EDIT_HAS_SELECTION ...); otherwise a
 * selection, undo and redo are taken to be there and paste follows the
 * clipboard.  The application may hear an operation first: its callback
 * returns 1 when it carried it out itself (the default is skipped).
 */
#define KUI_EDIT_COPY		0U
#define KUI_EDIT_CUT		1U
#define KUI_EDIT_PASTE		2U
#define KUI_EDIT_UNDO		3U
#define KUI_EDIT_REDO		4U
#define KUI_EDIT_SELECT_ALL	5U
#define KUI_EDIT_SELECT_BEGIN	6U
#define KUI_EDIT_SELECT_END	7U
#define KUI_EDIT_HAS_SELECTION	1U
#define KUI_EDIT_CAN_PASTE	2U
#define KUI_EDIT_CAN_UNDO	4U
#define KUI_EDIT_CAN_REDO	8U
typedef int (*kui_edit_fn)(void *data, unsigned operation);
void kui_window_on_edit(struct kui_window *window, kui_edit_fn callback, void *data);
void kui_window_edit_state(struct kui_window *window, unsigned state);
int kui_window_selecting(const struct kui_window *window);
size_t kui_window_paste_primary(struct kui_window *window, char *text, size_t size);

/*
 * KUI_VERSION 10 (ws090-p008, Image Viewer): the full screen.  An
 * application asks the compositor for it (or out of it), and learns from
 * the configure whether the window is fullscreen.
 */
void kui_window_set_fullscreen(struct kui_window *window, int fullscreen);
int kui_window_fullscreen(const struct kui_window *window);

/*
 * KUI_VERSION 11 (ws090-p011, Terminal): an application that waits for
 * other descriptors too (a terminal's shells) waits for them with the
 * compositor, at most KUI_WINDOW_FDS_MAX of them; ready[i] says fds[i] has
 * something to read or has hung up.
 */
#define KUI_WINDOW_FDS_MAX	16U
int kui_window_dispatch_fds(struct kui_window *window, const int *fds, unsigned count, int timeout_ms, int *ready);
uint64_t kui_clock_us(void);

/*
 * The widgets (widgets.c, field.c, list.c, cards.c; KUI_VERSION 4,
 * plan/ws090/design.md section 3): each is drawn by one call during a
 * frame, which also records where it is for the input and reports what
 * the input did to it since the last frame (the immediate way of section
 * 2).  What a widget remembers between frames -- a field's text, a list's
 * selection and scroll -- is the application's, in a small struct it
 * keeps.  A widget draws with a style: the canvas of the frame, the text,
 * the theme, and whether the window stands on glass.
 *
 * The keyboard's focus is on one widget at a time (by id and index).  A
 * click or a tap on a widget that takes the keyboard gives it the focus;
 * Tab and Shift+Tab move it through those widgets in the order they were
 * drawn.  The keys a focused widget does not take, and every key while no
 * widget has the focus, are the application's (KUI_EVENT_KEY).
 */
struct kui_style {
	struct kui_canvas *canvas;
	struct kui_text *text;
	const struct kui_theme *theme;
	int glass;
};

/* A key no widget took (an event's kind; kui_event's code and modifiers name it). */
#define KUI_EVENT_KEY		9U

/* A button's look and state (bits). */
#define KUI_BUTTON_PRIMARY	1U
#define KUI_BUTTON_DANGER	2U
#define KUI_BUTTON_DISABLED	4U

/* What a text field reports (bits). */
#define KUI_FIELD_CHANGED	1U
#define KUI_FIELD_SUBMITTED	2U
#define KUI_FIELD_CANCELLED	4U

/* What a list reports (bits). */
#define KUI_LIST_SELECTED	1U
#define KUI_LIST_ACTIVATED	2U
#define KUI_LIST_TOUCHED	4U	/* KUI_VERSION 5: the row was chosen by a finger's tap */

/* The longest text a field holds, with its NUL. */
#define KUI_FIELD_MAX		512U

/*
 * A one-line text field's state: its UTF-8 text, the caret and the other
 * end of the selection (byte offsets on character boundaries), how far the
 * text is scrolled across, and whether its characters are shown as dots.
 */
struct kui_field {
	char text[KUI_FIELD_MAX];
	size_t length;
	size_t caret;
	size_t anchor;
	int scroll;
	int secret;
};

/*
 * A list's state: how many items it has, the one selected (-1 for none),
 * and its scroll.
 */
struct kui_list {
	size_t count;
	long selected;
	struct kui_scroll scroll;
};

/* The keyboard's focus, and where the pointer is for a widget that follows it. */
int kui_ui_key(struct kui_ui *ui, uint32_t key, int pressed, unsigned modifiers);
void kui_ui_set_focus(struct kui_ui *ui, uint32_t id, uint32_t index);
void kui_ui_clear_focus(struct kui_ui *ui);
int kui_ui_has_focus(const struct kui_ui *ui, uint32_t id, uint32_t index);
void kui_ui_pointer(const struct kui_ui *ui, double *x, double *y);

/* The widgets, each drawn and asked by one call during a frame. */
int kui_button(struct kui_ui *ui, const struct kui_style *style, uint32_t id, const struct kui_rect *rect, const char *label, unsigned flags);
int kui_button_width(const struct kui_style *style, const char *label);
int kui_switch(struct kui_ui *ui, const struct kui_style *style, uint32_t id, int x, int y, int *on, unsigned flags);
int kui_slider(struct kui_ui *ui, const struct kui_style *style, uint32_t id, const struct kui_rect *rect, double minimum, double maximum, double step, double *value);
void kui_field_set(struct kui_field *field, const char *text);
unsigned kui_field(struct kui_ui *ui, const struct kui_style *style, uint32_t id, const struct kui_rect *rect, struct kui_field *field, const char *placeholder);
int kui_list_init(struct kui_list *list);
void kui_list_release(struct kui_list *list);
unsigned kui_list_begin(struct kui_ui *ui, const struct kui_style *style, uint32_t id, const struct kui_rect *rect, struct kui_list *list, size_t count, size_t *first, size_t *last);
unsigned kui_list_row(struct kui_ui *ui, const struct kui_style *style, uint32_t id, const struct kui_rect *rect, struct kui_list *list, size_t index, struct kui_rect *row, kui_color *ink);
void kui_list_end(struct kui_ui *ui, const struct kui_style *style, const struct kui_rect *rect, struct kui_list *list);
int kui_sidebar_section(const struct kui_style *style, int x, int y, int width, const char *title);
int kui_sidebar_item(struct kui_ui *ui, const struct kui_style *style, uint32_t id, uint32_t index, const struct kui_rect *rect, enum kui_icon icon, const char *label, int current);
void kui_panel(const struct kui_style *style, const struct kui_rect *rect, int sidebar);
int kui_card(const struct kui_style *style, const struct kui_rect *rect, const char *title, const char *subtitle);
int kui_row(const struct kui_style *style, int x, int y, int width, const char *label, const char *value, int last);
int kui_header(const struct kui_style *style, int x, int y, int width, const char *title, const char *summary);
int kui_dialog(struct kui_ui *ui, const struct kui_style *style, uint32_t id, const struct kui_rect *area, const char *title, const char *body, const char *const *labels, int count);
void kui_chip(const struct kui_style *style, int centre_x, int bottom, const char *message);
void kui_progress(const struct kui_style *style, const struct kui_rect *rect, double fraction, uint64_t now_us);

/*
 * The file chooser (KUI_VERSION 5; libkeiland's keiland_file_chooser of
 * KEILAND_VERSION 12, moved here by ws090-p006 and made of the widgets):
 * the Open and Save As window every application shares.  It shows the
 * folders and files of a folder, the sidebar's places (Recent, Home and
 * its usual folders, Computer), the filter chosen, and in Save mode takes
 * a name and asks before a file is replaced.  The answer comes once,
 * through the listener, while the application dispatches its default
 * Wayland queue; the application then destroys the chooser.  The
 * application keeps running meanwhile, and should take no input of its
 * own until the answer comes.
 *
 * The chooser is a window of its own on the application's connection, with
 * its own wl_seat objects.  Wayland sends a client's pointer, keyboard and
 * touch events to all of its objects of a seat, so an application ignores
 * the enter, key and touch events of surfaces that are not its own (as
 * kui_window does).
 */
struct kui_file_chooser;

/* What the chooser asks for: an existing file to open, or a folder and a name to save as. */
#define KUI_FILE_CHOOSER_OPEN		0U
#define KUI_FILE_CHOOSER_SAVE		1U

/* How it ended: a path was chosen, or the user cancelled. */
#define KUI_FILE_CHOOSER_CHOSEN		0U
#define KUI_FILE_CHOOSER_CANCELLED	1U

/* The most filters one chooser offers. */
#define KUI_FILE_CHOOSER_FILTERS_MAX	16U

/*
 * One filter: the label it is shown by, and the file name extensions it
 * shows, separated by spaces and without their dots ("txt md c h"),
 * compared without regard to case.  NULL or empty extensions show every
 * file.  Folders are always shown.
 */
struct kui_file_filter {
	const char *label;
	const char *extensions;
};

/*
 * What a chooser starts with.  Any pointer may be NULL.
 *
 * mode: KUI_FILE_CHOOSER_OPEN or _SAVE.  title: the window's title ("Open"
 * or "Save As" when NULL).  application: the app_id the window gets, so
 * that zdesktop shows it as the application's.  folder: where it starts
 * (the home folder when NULL or not a folder).  name: the name Save starts
 * with, selected up to its extension.  filters, filter_count and filter:
 * the filters offered (at most KUI_FILE_CHOOSER_FILTERS_MAX) and the one
 * chosen first; without filters every file is shown.  font and
 * fallback_font: the interface's font and the one for characters it lacks
 * (the system's when NULL).
 */
struct kui_file_chooser_options {
	unsigned mode;
	const char *title;
	const char *application;
	const char *folder;
	const char *name;
	const struct kui_file_filter *filters;
	size_t filter_count;
	size_t filter;
	const char *font;
	const char *fallback_font;
};

/*
 * What a chooser tells the application, once and last: how it ended
 * (KUI_FILE_CHOOSER_*), the absolute path chosen (empty when cancelled),
 * and the filter chosen last.  In Save mode the user has already agreed to
 * replace a file that exists.  The chooser's window is closed by then; the
 * application destroys the chooser, from the callback or later.
 */
struct kui_file_chooser_listener {
	void (*done)(void *data, struct kui_file_chooser *chooser, unsigned result, const char *path, size_t filter);
};

/*
 * Opens a file chooser over an application's window (parent may be NULL)
 * on the application's connection.
 *
 * Returns NULL with errno set: EINVAL (an unknown mode, too many filters,
 * a filter number past them, no listener), ENOTSUP (a compositor without
 * wl_shm or xdg_wm_base), an errno value of opening the font, ENOMEM.
 */
struct kui_file_chooser *kui_file_chooser_open(struct wl_display *display, struct xdg_toplevel *parent, const struct kui_file_chooser_options *options, const struct kui_file_chooser_listener *listener, void *data);

/*
 * Closes a chooser; one still open closes without telling.
 */
void kui_file_chooser_destroy(struct kui_file_chooser *chooser);

#ifdef __cplusplus
}
#endif

#endif
