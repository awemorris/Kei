/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The glass look's shell (ws035-p059, p062): windows, their title bars, the
 * system bar, and what the pointer does to them.
 *
 * A window's title bar floats above its body with a gap: a rounded glass
 * panel with the application's mark, the title and the minimize, maximize
 * and close buttons.  Maximizing docks the window to the system bar: the body
 * fills the output under the bar, the floating title bar goes, and the title
 * with its buttons (restore instead of maximize) moves into the bar's left
 * zone.  A window docks by a double click on its title bar, by its maximize
 * button, or by dragging its title bar into the system bar; it comes back by
 * a double click on the title in the bar, by the restore button, or by
 * pulling the title down out of the bar, which leaves the window under the
 * pointer and goes on moving it.  Docking and coming back are animated for
 * DOCK_MS: the body's rectangle and the title bar's slide between their
 * places and the title bar's glass fades.
 *
 * A triple click on a floating title bar sends the window to the back and
 * gives the focus to the window now on top (ws079-p013, "go away"; a quick
 * two-finger flick up on it on a touch screen does the same, touch.c).  So that a
 * triple click never docks first, a double click docks only when the time
 * a third press has (DOUBLE_CLICK_MS after the second) is over.
 *
 * The system bar has three zones: on the left the launcher (the Kei mark,
 * ws035-p117) and the docked window; towards the right four virtual
 * desktops; at the right edge the network, the battery and the clock.  The
 * network's icon opens its menu (network.c, ws035-p013); the battery is
 * drawn only (a mock-up).
 *
 * A fullscreen window is kept whole (ws035-p119, the 2026-09-28 user
 * decision): while it is the highest of the windows that cover the top of
 * the output (fullscreen or docked), a window opened or raised over it
 * floats above it and the system bar stays away; the bar's place is the
 * fullscreen window's, and the bar is reached from the edges' gestures,
 * with App Home and Wiseview, which show it.
 *
 * A sheet (ws090-p014, sheet.c) is a window hung under its parent's title
 * bar: it has no title bar of its own, sits with its top at the bottom of
 * the parent's floating title bar (under the system bar when the parent is
 * docked, at the top of the output when it is fullscreen), in the middle of
 * the parent's width, and slides down out of the title bar for SHEET_MS.
 * It moves, comes to the front, hides and leaves Wiseview with its parent;
 * while it is open the parent's body takes no press (its title bar still
 * moves it) and the parent passes the focus on to it.
 *
 * Wiseview (p063, plan/ws035/wiseman-design.md) is the overview of the
 * windows: dragging up from the bottom edge opens it, following the pointer
 * (how far it is open is the distance moved over WISEVIEW_DISTANCE); let go
 * past WISEVIEW_THRESHOLD it opens, otherwise it closes.  Each window moves
 * from its place to a tile of a grid over the blurred, darkened wallpaper,
 * the most recently raised first, with a glass label of its title under it.
 * A click on a tile brings that window to the top, a click elsewhere closes
 * Wiseview, and a tile's close button closes its window.  Super+Tab opens
 * it from the keyboard (p014): Tab and the arrows move the current tile,
 * Enter chooses it, Esc closes Wiseview.
 *
 * The edges' gestures (the 2026-09-28 decision at the end of
 * plan/ws079/design-input-notes.md): from the top-left corner App Home,
 * from the top-right corner Notes (corner.c, also over Home), from the
 * bottom edge Wiseview (over Home the same swipe closes Home instead,
 * home.c).  Each counts only when it starts in its corner or edge, so a
 * stroke that starts inside a window never becomes one.  They work over a
 * fullscreen window too (zwl_glass_edge_button), except that there the
 * bottom edge's swipe takes the window back to a window instead of opening
 * Wiseview (ws099-p015).  A fullscreen window is composed like any other.
 */

#include "desktop.h"
#include "extras.h"
#include "menu.h"
#include "popup.h"
#include "titlebar.h"
#include "toplevel.h"
#include "subsurface.h"
#include "panels.h"
#include "touch.h"
#include "edit.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* The title bar's buttons, counted from the right. */
#define BUTTON_CLOSE		0
#define BUTTON_MAXIMIZE		1
#define BUTTON_MINIMIZE		2
#define BUTTON_COUNT		3
#define BUTTON_SPACING		34
#define BUTTON_WIDTH		30
#define BUTTON_HEIGHT		28

/*
 * A floating window's frame (ws035-p128): the band this wide around the
 * title bar and the body resizes the window by the side it is on, and
 * within this far of a corner, along either side, by both sides of the
 * corner (a diagonal resize).
 */
#define FRAME_BAND		8
#define FRAME_CORNER		24

/*
 * Placing a new window (ws035-p092): how far each cascade step moves it
 * (a title bar and its gap, so the title bar under it shows), how many
 * steps are tried each way, and how near another window's corner is too
 * near.
 */
#define GLASS_CASCADE		48
#define GLASS_CASCADE_ROUNDS	8
#define GLASS_NEAR		32

/* How long a sheet takes to slide down out of its parent's title bar (ws090-p014). */
#define SHEET_MS		200U

/* Set while window_lower sends a sheet under the other windows, before its parent (ws090-p014). */
static unsigned sheet_lowering;

/* How much narrower than its parent's body a sheet is at least asked to be on each side, and the narrowest it is asked to be. */
#define SHEET_MARGIN		24
#define SHEET_NARROWEST		320

/* Where a title bar's first letters are, from its left edge (past the mark) and below its middle. */
#define GLASS_TITLE_START	72
#define GLASS_TITLE_LOW		8

/* The docked title's buttons in the system bar are further apart. */
#define BAR_BUTTON_SPACING	46

/*
 * The launcher at the bar's left end: the Kei mark's square, its place and
 * its side in pixels (ws035-p117; App Home's press area, home.c, is the
 * bar's first 40 pixels).
 */
#define BAR_LAUNCHER_X		10
#define BAR_LAUNCHER_Y		4
#define BAR_LAUNCHER_SIZE	26

/*
 * The dock animation, a double click, and how far a docked title is pulled
 * down to come off (ws035-p064: the window follows the pull on the way,
 * shrinking from the docked space to its own size under the pointer).
 */
#define DOCK_MS			220U

/* The kind of the animation that is not a dock or an undock: a launched window growing from its icon (ws035-p071). */
#define ANIM_LAUNCH		2U
#define DOUBLE_CLICK_MS		400U
#define PULL_DISTANCE		140

/* A docked body starts this far under the top of the output. */
#define DOCK_TOP		ZWL_GLASS_DOCK_TOP

/* The virtual desktops: how many, and the size of each picture. */
#define DESKTOPS		4
#define DESKTOP_WIDTH		40
#define DESKTOP_HEIGHT		20
#define DESKTOP_GAP		6

/* The desktops' swipe: how near the edge it starts, how far it moves before it is one, and how long a slide takes. */
#define DESKTOP_EDGE		16
#define DESKTOP_START		12
#define DESKTOP_MS		220U

/* The keys of Ctrl+Alt+Left/Right, and the modifiers' bits (Control and Mod1). */
#define SHORTCUT_LEFT		105U
#define SHORTCUT_RIGHT		106U
#define MODIFIERS_CONTROL_ALT	(4U | 8U)
#define MODIFIER_SHIFT		1U

/* The Super (Windows) key's bit, for Super+Tab (Wiseview). */
#define MODIFIER_SUPER		0x40U

/* App Home's corner, where its gesture starts over a fullscreen window (the same as home.c's). */
#define HOME_EDGE_CORNER	28

/* How far a Wiseview tile moves before it is dragged. */
#define TILE_DRAG_START		8

/* Wiseview: where the gesture starts, how far it goes, when it opens, and how long it settles. */
#define WISEVIEW_EDGE		20
#define WISEVIEW_DISTANCE	240.0f
#define WISEVIEW_THRESHOLD	0.35f
#define WISEVIEW_MS		200U

/*
 * The bottom edge's swipe over a fullscreen window (ws099-p015, the
 * 2026-09-30 user decision): a contact that starts in the bottom edge
 * (WISEVIEW_EDGE) and moves up this far takes the window back to a window.
 */
#define UNFULLSCREEN_DISTANCE	80

/* Wiseview's grid: side, top (under the header) and bottom margins, the gutter, the label under a tile. */
#define WISEVIEW_SIDE		56
#define WISEVIEW_TOP		(ZWL_GLASS_BAR + 56)
#define WISEVIEW_BOTTOM		72
#define WISEVIEW_GUTTER		24
#define WISEVIEW_LABEL		46
#define WISEVIEW_RADIUS		16.0f
#define WISEVIEW_WINDOWS	64U

/*
 * The application IDs whose window bodies keep square corners (ws035-p134,
 * p136, the user's requests of 2026-09-29): a terminal's text runs into its
 * corners, where the rounding would cut it.  Their floating title bars stay
 * rounded.  Another (an X terminal) is one more line here.
 */
static const char *const shell_square_apps[] = {
	"terminal"
};

/* Where the pointer is over a window. */
enum shell_hit {
	HIT_NONE,
	HIT_TITLE,
	HIT_BODY,
	HIT_FRAME
};

/*
 * How far the glass's blur (backdrop.c) and a window's shadow reach from
 * a change: a window above that comes this near sees it through its glass.
 */
#define DAMAGE_REACH		96

/* A rectangle in output pixels. */
struct shell_rect {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

/* Where the system bar's parts are (they follow the clock's width). */
struct shell_bar {
	char clock[48];
	int32_t clock_x;
	int32_t battery_x;
	int32_t signal_x;
	int32_t volume_x;
	int32_t status_line;
	int32_t desktops_x;
	int32_t desktops_width;
	int32_t desktops_line;
	int32_t buttons[BUTTON_COUNT];
	int32_t menu_line;
	int32_t title_x;
};

static void bar_layout(struct zwl_server *server, struct shell_bar *bar);
static void draw_window(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned focused, const struct shell_bar *bar);
static int window_shown(struct zwl_server *server, struct zwl_object *surface, float home, float position);
static void window_layer(struct zwl_server *server, struct zwl_object *surface, float home, float position);
static void draw_backdrop(struct zwl_server *server, VkCommandBuffer command, struct zwl_object **windows, unsigned below, float position);
static void draw_window_blurred(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface);
static void draw_body(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, const struct shell_rect *body, unsigned docked, unsigned focused);
static void draw_title_bar(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, const struct shell_rect *panel, float fade, float buttons, unsigned focused);
static void draw_title(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, int32_t x, int32_t middle, int32_t limit, const float *ink);
static int draw_picture_mark(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, int32_t x, int32_t middle, const float *ink);
static void draw_letter_mark(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, const char *title, int32_t x, int32_t middle);
static int32_t title_end(struct zwl_server *server, struct zwl_object *surface, int32_t limit);
static const char *mark_name(const char *app_id);
static unsigned window_square(const struct zwl_object *surface);
static int glass_crowd(struct zwl_server *server, struct zwl_object *surface, int32_t x, int32_t y, int32_t width, int32_t height);
static void glass_clear_edges(struct zwl_server *server, int32_t width, int32_t height, int32_t *x, int32_t *y);
static int glass_top(struct zwl_server *server, struct zwl_object *surface, int32_t *x, int32_t *y);
static int glass_placed(struct zwl_server *server, struct zwl_object *surface, struct zwl_object *other);
static void shown_title(const struct zwl_object *surface, char *title, size_t size);
static void draw_sign(struct zwl_server *server, VkCommandBuffer command, int button, int32_t cx, int32_t cy, unsigned restore, unsigned over, float fade, const float *ink);
static void draw_system_bar(struct zwl_server *server, VkCommandBuffer command, const struct shell_bar *bar);
static void draw_desktops(struct zwl_server *server, VkCommandBuffer command, const struct shell_bar *bar, const float *line);
static void draw_status(struct zwl_server *server, VkCommandBuffer command, const struct shell_bar *bar, const float *ink);
static void draw_dock_hint(struct zwl_server *server, VkCommandBuffer command);
static float animation_progress(struct zwl_server *server);
static void lerp_rect(const struct shell_rect *from, const struct shell_rect *to, float t, struct shell_rect *result);
static void body_rect(struct zwl_server *server, const struct zwl_object *surface, struct shell_rect *body);
static void docked_rect(struct zwl_server *server, struct shell_rect *body);
static void pulled_rect(struct zwl_server *server, const struct zwl_object *surface, struct shell_rect *body);
static void pull_back(struct zwl_server *server);
static void window_minimize(struct zwl_server *server, struct zwl_object *surface);
static void window_to_desktop(struct zwl_server *server, struct zwl_object *surface, unsigned desktop, const char *via);
static int desktop_picture_at(struct zwl_server *server, int32_t x, int32_t y);
static void floating_title(const struct shell_rect *body, struct shell_rect *panel);
static void bar_title_slot(struct zwl_server *server, const struct shell_bar *bar, struct shell_rect *slot);
static void window_size(const struct zwl_object *surface, int32_t *width, int32_t *height);
static int damage_near(struct zwl_server *server, struct zwl_object *surface, const struct shell_rect *near);
static enum shell_hit window_hit(struct zwl_server *server, const struct zwl_object *surface, int32_t x, int32_t y);
static uint32_t frame_edges(struct zwl_server *server, const struct zwl_object *surface, int32_t x, int32_t y);
static uint32_t frame_under_pointer(struct zwl_server *server);
static int glass_motion_take(struct zwl_server *server);
static int button_at(const struct zwl_object *surface, int32_t x, int32_t y);
static void button_centre(const struct zwl_object *surface, int button, int32_t *x, int32_t *y);
static int bar_button_at(const struct shell_bar *bar, int32_t x, int32_t y);
static struct zwl_object *window_at(struct zwl_server *server, int32_t x, int32_t y, enum shell_hit *hit);
static struct zwl_object *docked_window(struct zwl_server *server);
static void window_raise(struct zwl_server *server, struct zwl_object *surface);
static void sheet_place(struct zwl_server *server);
static struct zwl_object *sheet_owner(struct zwl_object *surface);
static void sheet_narrow(struct zwl_server *server, struct zwl_object *surface, struct zwl_object *parent, int32_t width, int32_t height);
static void sheet_anchor(struct zwl_server *server, struct zwl_object *parent, int32_t width, int32_t *x, int32_t *top);
static void draw_sheet(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, struct zwl_object *parent, unsigned focused);
static void window_dock(struct zwl_server *server, struct zwl_object *surface, int32_t restore_x, int32_t restore_y, const char *via);
static void window_undock(struct zwl_server *server, struct zwl_object *surface, int32_t x, int32_t y, const char *via);
static void window_configure(struct zwl_object *surface);
static unsigned double_click(struct zwl_server *server, struct zwl_object *surface);
static unsigned title_clicks(struct zwl_server *server, struct zwl_object *surface);
static void dock_when_due(struct zwl_server *server);
static void window_lower(struct zwl_server *server, struct zwl_object *surface, const char *via);
static int bar_press(struct zwl_server *server);
static struct zwl_object *bar_cover(struct zwl_server *server);
static void bar_cover_log(struct zwl_server *server, const struct zwl_object *cover);
static int home_without_bar(struct zwl_server *server, uint32_t button, uint32_t state);
static float wiseview_progress(struct zwl_server *server);
static void wiseview_settle(struct zwl_server *server, float from, float to);
static int wiseview_showing(struct zwl_server *server);
static void wiseview_open_key(struct zwl_server *server);
static void wiseview_key(struct zwl_server *server, uint32_t key, uint32_t state);
static void wiseview_close_key(struct zwl_server *server);
static unsigned wiseview_windows(struct zwl_server *server, struct zwl_object **windows, unsigned capacity);
static void wiseview_layout(struct zwl_server *server, struct zwl_object **windows, unsigned count, struct shell_rect *tiles);
static void draw_wiseview(struct zwl_server *server, VkCommandBuffer command, struct zwl_object **stacked, unsigned stacked_count, float progress);
static void draw_tile(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, const struct shell_rect *tile, float progress, unsigned current, unsigned over);
static int wiseview_button(struct zwl_server *server, uint32_t button, uint32_t state);
static void wiseview_log(struct zwl_server *server);
static int wiseview_edge_press(struct zwl_server *server, uint32_t button, uint32_t state);

/* Whether where the desktops' pictures are has been logged (once, for the tests that click them). */
static unsigned shell_desktops_logged;

/*
 * The bottom edge's swipe over a fullscreen window: whether a contact that
 * began in the bottom edge holds it (until its release, even once the
 * window has left fullscreen), where it began, and whether it has taken the
 * window back already.  Only the event loop touches it.
 */
static struct {
	int pressing;
	int done;
	int32_t start_y;
} unfullscreen_swipe;
static struct zwl_object *fullscreen_top(struct zwl_server *server);
static int fullscreen_whole(struct zwl_server *server, float home);
static int unfullscreen_press(struct zwl_server *server, uint32_t button, uint32_t state);
static int unfullscreen_motion(struct zwl_server *server);
static float desktop_position(struct zwl_server *server);
static void desktop_turn(struct zwl_server *server, int target, const char *via);
static void desktop_release(struct zwl_server *server);
static unsigned desktop_windows(struct zwl_server *server, unsigned desktop);

/*
 * Draws the wallpaper, the windows from the bottom with their shadows and
 * title bars, and the system bar over them.
 */
void
zwl_glass_draw(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object **windows,
	unsigned count)
{
	struct glass_shape shape;
	struct zwl_object *top;
	struct zwl_object *cover;
	struct shell_bar bar;
	unsigned index;
	unsigned focused;
	unsigned drawn;
	int shown;
	int whole;
	float progress;
	unsigned blur;
	int showing;
	float home;
	float position;

	/* The login screen, or a session's lock screen, is all there is to draw (greeter.c). */
	if (server->greeter || server->locked) {
		zwl_greeter_draw(server, command);
		return;
	}

	/* The fullscreen window that keeps the system bar away this frame, if any (ws035-p119). */
	cover = bar_cover(server);
	bar_cover_log(server, cover);

	/* The menus' and the controls' places are those this frame draws them at (menu-shell.c, titlebar-shell.c). */
	zwl_menu_frame(server);
	zwl_titlebar_frame(server);

	/*
	 * App Home, opening, open or closing, lies under the desktop layer,
	 * which slides aside over it with its shadow (home.c).
	 */
	home = zwl_home_progress(server);
	if (home > 0.0f) {
		zwl_home_draw(server, command, home);
		zwl_home_layer(server, home, &server->layer_x, &server->layer_y, &server->layer_scale);
		glass_shape_init(&shape, server->layer_x, server->layer_y, (float)server->width * server->layer_scale, (float)server->height * server->layer_scale);
		shape.quad[0] -= 80.0f;
		shape.quad[1] -= 80.0f;
		shape.quad[2] += 160.0f;
		shape.quad[3] += 160.0f;
		shape.mode = MODE_SHADOW;
		shape.radius = 18.0f;
		shape.soft = 36.0f;
		shape.color[0] = 0.08f;
		shape.color[1] = 0.12f;
		shape.color[2] = 0.24f;
		shape.color[3] = 0.40f * home;
		glass_shape_draw(server, command, &shape);
		server->layer_on = 1;
	}

	/*
	 * A fullscreen window on top that covers the output with an opaque
	 * image is all there is under the cursor (ws099-p015): the wallpaper,
	 * the desktop and the windows under it are not drawn.
	 */
	whole = fullscreen_whole(server, home);
	if (whole) {
		top = zwl_top_window(server);
		window_layer(server, top, home, (float)server->desktop);
		draw_window(server, command, top, 1U, NULL);
		server->layer_on = 0;
		zwl_backdrop_reset(server);
		zwl_popup_draw(server, command);
		return;
	}

	/* The wallpaper over the whole output (with round corners while it is pushed aside). */
	glass_shape_init(&shape, 0.0f, 0.0f, (float)server->width, (float)server->height);
	shape.mode = MODE_IMAGE;
	shape.opaque = 1.0f;
	shape.set = glass_wallpaper_set(server);
	if (home > 0.0f)
		shape.radius = 18.0f;
	glass_shape_draw(server, command, &shape);

	/* The system bar's layout, which a docking title bar moves to. */
	bar_layout(server, &bar);

	/* Wiseview, opening, open or closing, takes the windows' place. */
	progress = wiseview_progress(server);
	if (progress > 0.0f) {
		draw_wiseview(server, command, windows, count, progress);
		server->layer_on = 0;
		draw_system_bar(server, command, &bar);
		return;
	}

	/* The desktop's icons over the wallpaper, with the layer (desktop.c). */
	zwl_desktop_draw(server, command);

	/*
	 * The windows; the top one has the focus; where a dragged one would
	 * dock shows just under it.  The desktop shown's windows, and while
	 * the desktops slide or are swiped, the neighbour's too, a screen's
	 * width to the side (Home, when it shows, has the desktop shown only).
	 */
	top = zwl_top_window(server);
	position = desktop_position(server);
	drawn = 0;
	for (index = 0; index < count; index++) {
		shown = window_shown(server, windows[index], home, position);
		if (!shown)
			continue;

		/*
		 * The glass of a window over others that asked for it (set_blur,
		 * ws075-p029) shows them blurred (backdrop.c), not while Home has the
		 * layer; any other window's glass shows the blurred wallpaper alone,
		 * which costs nothing more (the default).
		 */
		blur = zwl_panels_blur(windows[index]);
		if (drawn > 0U && home <= 0.0f && blur) {
			draw_backdrop(server, command, windows, index, position);
		} else {
			zwl_backdrop_reset(server);
		}

		/* One more window drawn. */
		drawn++;

		/* Shifted with the layer, when Home does not have it. */
		window_layer(server, windows[index], home, position);

		/* The window. */
		focused = 0;
		if (windows[index] == top)
			focused = 1;
		if (windows[index] == server->drag)
			draw_dock_hint(server, command);
		draw_window(server, command, windows[index], focused, &bar);
	}

	/* The windows' popups over all the windows (popup.c); the glass from here is on the blurred wallpaper. */
	server->layer_on = 0;
	zwl_backdrop_reset(server);
	zwl_popup_draw(server, command);

	/*
	 * The system bar over everything but the cursor, where it always is,
	 * unless a fullscreen window is to be kept whole (ws035-p119).
	 */
	server->layer_on = 0;
	if (cover == NULL)
		draw_system_bar(server, command, &bar);

	/* An open menu's popups over the system bar (menu-shell.c). */
	zwl_menu_draw_popups(server, command);

	/* The network's menu, when open (network.c). */
	zwl_network_draw_menu(server, command);

	/* The volume's popup, when open (volume.c). */
	zwl_volume_draw_popup(server, command);

	/* The top-right corner's hint, while its swipe is followed or settles (corner.c). */
	zwl_corner_draw(server, command);

	/*
	 * The on-screen keyboard over everything (keyboard.c, ws102).  Its glass
	 * shows the scene under it blurred when zdesktop was started so
	 * (--keyboard-blur, ws075-p029), else the blurred wallpaper.
	 */
	showing = zwl_keyboard_showing();
	if (server->keyboard_blur && showing && home <= 0.0f)
		draw_backdrop(server, command, windows, count, position);
	zwl_keyboard_draw(server, command);
	zwl_backdrop_reset(server);

	/* A frame of the animation. */
	if (server->anim != NULL && server->log_frames)
		printf("ZWL GLASS anim surface=%u docking=%u t=%.2f\n", server->anim->id, server->anim_docking, (double)animation_progress(server));
}

/*
 * Handles a pointer button in the glass look.  A press raises the window
 * under the pointer; on its title bar it starts a move, presses a button,
 * (twice) docks it or (three times) sends it to the back; on the system bar
 * it acts on the docked window.  A
 * release ends a move, docking the window when it ends in the system bar.
 * Returns 1 when the button is zdesktop's, 0 when it goes to the client.
 */
int
zwl_glass_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	struct zwl_object *surface;
	struct zwl_object *cover;
	struct zwl_object *sheet;
	enum shell_hit hit;
	uint32_t edges;
	unsigned clicks;
	int pressed;
	int error;
	int open;

	/* The login screen takes every button (greeter.c). */
	if (server->greeter) {
		pressed = zwl_greeter_button(server, button, state);
		return pressed;
	}

	/* Wiseview, open or being opened, takes every button. */
	if (server->wiseview_gesture || server->wiseview > 0.0f || server->wiseview_moving) {
		pressed = wiseview_button(server, button, state);
		return pressed;
	}

	/*
	 * The top-right corner's swipe to Notes (corner.c) takes a press that
	 * starts in the corner, and that contact's release; before Home, so
	 * that it works over Home too (its corner is not Home's).
	 */
	pressed = zwl_corner_button(server, button, state);
	if (pressed)
		return 1;

	/*
	 * The on-screen keyboard (keyboard.c, ws102) takes a press on its
	 * panel and one in a bottom corner (its swipe), before Wiseview's
	 * bottom edge and the desktops' side edges, which start outside the
	 * corners.
	 */
	pressed = zwl_keyboard_button(server, button, state);
	if (pressed)
		return 1;

	/*
	 * While a fullscreen window keeps the system bar away (ws035-p119),
	 * the bar's place is that window's: there is no launcher and no
	 * network icon to press, and App Home is reached from the top-left
	 * corner as over a fullscreen window.
	 */
	cover = bar_cover(server);
	pressed = 0;
	if (cover == NULL) {
		/* App Home takes the launcher, the top-left corner, and every button while it shows. */
		pressed = zwl_home_button(server, button, state);
	} else {
		/* Only the corner's press, and the rest of one of Home's own presses. */
		open = home_without_bar(server, button, state);
		if (open)
			pressed = zwl_home_button(server, button, state);
	}

	/* A button Home took goes no further. */
	if (pressed)
		return 1;

	/* The volume takes a press on its icon, and every button while its popup is open (volume.c). */
	open = zwl_volume_is_open();
	if (cover == NULL || open) {
		pressed = zwl_volume_button(server, button, state);
		if (pressed)
			return 1;
	}

	/* The network takes a press on its icon, and every button while its menu is open (network.c). */
	open = zwl_network_is_open();
	if (cover == NULL || open) {
		pressed = zwl_network_button(server, button, state);
		if (pressed)
			return 1;
	}

	/* The menus take a press on a window's menu, and every button while one is open (menu-shell.c). */
	pressed = zwl_menu_button(server, button, state);
	if (pressed)
		return 1;

	/* A titlebar's controls take a press on one, and its release (titlebar-shell.c). */
	pressed = zwl_titlebar_button(server, button, state);
	if (pressed)
		return 1;

	/* The end of a press at the left or right edge: the swipe switches the desktop, or goes back. */
	if (state == 0 && server->desktop_press) {
		desktop_release(server);
		return 1;
	}

	/* A left press at the left or right edge (under the system bar) may become the desktops' swipe. */
	if (state != 0 &&
	    button == ZWL_BUTTON_LEFT &&
	    server->pointer_y >= ZWL_GLASS_BAR &&
	    (server->pointer_x < DESKTOP_EDGE || server->pointer_x >= (int32_t)server->width - DESKTOP_EDGE)) {
		server->desktop_press = 1;
		server->desktop_dragging = 0;
		server->desktop_start_x = server->pointer_x;
		server->desktop_offset = 0;
		return 1;
	}

	/* A left press at the bottom edge starts opening Wiseview. */
	pressed = wiseview_edge_press(server, button, state);
	if (pressed)
		return 1;

	/* A release ends a move (docking in the system bar) or a pull. */
	if (state == 0) {
		if (server->pull != NULL) {
			pull_back(server);
			return 1;
		}

		/* Without a move the client has the release. */
		if (server->drag == NULL)
			return 0;

		/* Let go in the system bar, the window docks; it comes back to where the move started. */
		surface = server->drag;
		server->drag = NULL;
		if (server->pointer_y < ZWL_GLASS_BAR) {
			window_dock(server, surface, server->drag_start_x, server->drag_start_y, "drag");
			return 1;
		}

		/* Otherwise it stays where it was moved. */
		printf("ZWL GLASS moved surface=%u x=%d y=%d\n", surface->id, surface->x, surface->y);
		return 1;
	}

	/* The system bar is zdesktop's, where it is drawn. */
	if (server->pointer_y < ZWL_GLASS_BAR && cover == NULL) {
		pressed = bar_press(server);
		return pressed;
	}

	/* Only the left button acts on windows. */
	surface = window_at(server, server->pointer_x, server->pointer_y, &hit);

	/* A press where no window is goes to the desktop's icons when there are any (desktop.c); a window's press takes the keyboard back from them. */
	if (surface == NULL) {
		open = zwl_desktop_press(server);
		if (open)
			return 0;
	} else {
		zwl_desktop_unfocus(server);
	}

	/* A window with a sheet open takes no press but its title bar's, which still moves it (ws090-p014). */
	sheet = NULL;
	if (surface != NULL && hit != HIT_TITLE)
		sheet = zwl_sheet_of(surface);
	if (sheet != NULL) {
		window_raise(server, surface);
		printf("ZWL GLASS sheet holds surface=%u\n", surface->id);
		return 1;
	}

	/* Another button is the client's on a body, zdesktop's elsewhere. */
	if (button != ZWL_BUTTON_LEFT)
		return hit != HIT_BODY;

	/* A press on the desktop is zdesktop's. */
	if (surface == NULL)
		return 1;

	/* The window comes to the top and takes the focus. */
	window_raise(server, surface);

	/* On the body the client has the press. */
	if (hit == HIT_BODY)
		return 0;

	/* On the frame a resize by its side or corner starts, until the button is let go (toplevel.c). */
	if (hit == HIT_FRAME) {
		edges = frame_edges(server, surface, server->pointer_x, server->pointer_y);
		error = zwl_toplevel_resize_start(server, surface, edges);
		if (error != 0)
			printf("ZWL GLASS frame refused surface=%u edges=%u\n", surface->id, edges);
		return 1;
	}

	/* On a button, its action. */
	pressed = button_at(surface, server->pointer_x, server->pointer_y);
	if (pressed == BUTTON_CLOSE) {
		(void)zwl_emit(surface->client, surface->role->top->id, 1U, NULL, 0U);
		printf("ZWL GLASS close surface=%u\n", surface->id);
		return 1;
	}

	/* Maximize docks the window. */
	if (pressed == BUTTON_MAXIMIZE) {
		window_dock(server, surface, surface->x, surface->y, "button");
		return 1;
	}

	/* Minimize hides it. */
	if (pressed == BUTTON_MINIMIZE) {
		window_minimize(server, surface);
		return 1;
	}

	/*
	 * A second quick press on the title bar is a double click, which docks
	 * the window once a third press can no longer come (dock_when_due).
	 */
	clicks = title_clicks(server, surface);
	if (clicks == 2U) {
		server->dock_waiting = surface;
		server->dock_due_ms = server->click_ms + DOUBLE_CLICK_MS;
		printf("ZWL GLASS dock waiting surface=%u\n", surface->id);
		return 1;
	}

	/* A third quick press sends the window to the back instead. */
	if (clicks >= 3U) {
		server->dock_waiting = NULL;
		server->click_surface = NULL;
		server->click_count = 0;
		window_lower(server, surface, "triple-click");
		return 1;
	}

	/* Otherwise a move starts. */
	server->drag = surface;
	server->drag_dx = server->pointer_x - surface->x;
	server->drag_dy = server->pointer_y - surface->y;
	server->drag_start_x = surface->x;
	server->drag_start_y = surface->y;

	/* Succeeded: the press was zdesktop's. */
	return 1;
}

/*
 * Moves the window being moved, or pulls a docked window out of the system
 * bar, and shows a window frame's resize arrow where the pointer is on one.
 * Returns 1 when the motion is zdesktop's.
 */
int
zwl_glass_motion(
	struct zwl_server *server)
{
	uint32_t edges;
	int taken;

	/* The glass look's screens, gestures, menus and moves. */
	taken = glass_motion_take(server);

	/* Where the motion goes on to the client, a window's frame under the pointer shows its resize arrow. */
	edges = 0U;
	if (!taken)
		edges = frame_under_pointer(server);
	zwl_cursor_frame(server, edges);

	/* Succeeded: whether the motion was zdesktop's. */
	return taken;
}

/*
 * Handles a pointer button over a fullscreen window (zwl_glass_fullscreen_input),
 * which the rest of the glass look does not see: only the edges' gestures
 * (the top-left corner's App Home, the top-right corner's Notes, the bottom
 * corners' keyboard, the bottom edge's swipe back to a window) and what
 * they opened.  Returns 1 when the button is zdesktop's, 0 when it goes to
 * the fullscreen window.
 */
int
zwl_glass_edge_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	float home;
	int corner_press;
	int pressed;

	/* The login screen is never over a fullscreen window. */
	if (server->greeter)
		return 0;

	/* The bottom edge's swipe holds its contact until the release, also once the window has left fullscreen. */
	if (unfullscreen_swipe.pressing) {
		pressed = unfullscreen_press(server, button, state);
		return pressed;
	}

	/* Wiseview, opened from the keyboard (Super+Tab), takes every button. */
	if (server->wiseview_gesture || server->wiseview > 0.0f || server->wiseview_moving) {
		pressed = wiseview_button(server, button, state);
		return pressed;
	}

	/* The top-right corner's swipe to Notes (corner.c). */
	pressed = zwl_corner_button(server, button, state);
	if (pressed)
		return 1;

	/* The on-screen keyboard's panel and its bottom corners' swipe (keyboard.c), before the bottom edge's swipe. */
	pressed = zwl_keyboard_button(server, button, state);
	if (pressed)
		return 1;

	/* App Home, when it shows or follows a press of its own, has the button as in window mode. */
	home = zwl_home_progress(server);
	if (home > 0.0f ||
	    server->home_to > 0.0f ||
	    server->home_press ||
	    server->home_page_press ||
	    server->home_bottom_press) {
		pressed = zwl_home_button(server, button, state);
		return pressed;
	}

	/* A left press in the top-left corner may open App Home (the launcher is under the window). */
	corner_press = 0;
	if (state != 0 &&
	    button == ZWL_BUTTON_LEFT &&
	    server->pointer_x < HOME_EDGE_CORNER &&
	    server->pointer_y < HOME_EDGE_CORNER)
		corner_press = 1;
	if (corner_press) {
		pressed = zwl_home_button(server, button, state);
		return pressed;
	}

	/* A left press at the bottom edge starts the swipe that takes the window back (not Wiseview, ws099-p015). */
	pressed = unfullscreen_press(server, button, state);
	if (pressed)
		return 1;

	/* Succeeded: anything else is the fullscreen window's. */
	return 0;
}

/*
 * Follows the edges' gestures over a fullscreen window.  Returns 1 when the
 * motion is zdesktop's, 0 when it goes to the fullscreen window.
 */
int
zwl_glass_edge_motion(
	struct zwl_server *server)
{
	int taken;

	/* The login screen is never over a fullscreen window. */
	if (server->greeter)
		return 0;

	/* The bottom edge's swipe follows its contact. */
	taken = unfullscreen_motion(server);
	if (taken)
		return 1;

	/* Wiseview follows its gesture. */
	if (server->wiseview_gesture || server->wiseview > 0.0f || server->wiseview_moving) {
		server->dirty = 1;
		return 1;
	}

	/* The top-right corner's swipe (corner.c). */
	taken = zwl_corner_motion(server);
	if (taken)
		return 1;

	/* The on-screen keyboard's swipe and a press on its panel (keyboard.c). */
	taken = zwl_keyboard_motion(server);
	if (taken)
		return 1;

	/* App Home's gesture from the top-left corner (home.c). */
	taken = zwl_home_motion(server);
	if (taken)
		return 1;

	/* Succeeded: the motion is the fullscreen window's. */
	return 0;
}

/*
 * Tells whether the pointer and the fingers go to the fullscreen window
 * with only the edges' gestures for zdesktop (zwl_glass_edge_button): the
 * top window is fullscreen and nothing shows over it, or the bottom edge's
 * swipe holds its contact.  seat.c chooses by it (until ws099-p015 it
 * chose by fullscreen mode, the direct scanout, which is gone).
 */
int
zwl_glass_fullscreen_input(
	struct zwl_server *server)
{
	struct zwl_object *top;

	/* The swipe's contact stays the edges' until its release. */
	if (unfullscreen_swipe.pressing)
		return 1;

	/* A fullscreen window on top. */
	top = fullscreen_top(server);
	if (top == NULL)
		return 0;

	/* Succeeded: the input is the fullscreen window's. */
	return 1;
}

/* Finds the top window when it is fullscreen with nothing shown over it; NULL otherwise. */
static struct zwl_object *
fullscreen_top(
	struct zwl_server *server)
{
	struct zwl_object *top;
	int overlay;

	/* Only the glass look's composed output, not the login or the lock screen. */
	if (!server->glass || !server->windowed)
		return NULL;
	if (server->greeter || server->locked)
		return NULL;

	/* An edge's gesture showing something (App Home, Wiseview, the hints, the keyboard) has the input as in window mode. */
	overlay = zwl_glass_overlay(server);
	if (overlay)
		return NULL;

	/* The top window, fullscreen with an image. */
	top = zwl_top_window(server);
	if (top == NULL || !top->fullscreen || top->current == NULL)
		return NULL;

	/* Succeeded: that window. */
	return top;
}

/*
 * Tells whether a frame draws only the fullscreen window on top
 * (zwl_glass_draw): with nothing shown over it or moving, it covers the
 * output with an opaque image, so what is under it is not seen.
 */
static int
fullscreen_whole(
	struct zwl_server *server,
	float home)
{
	struct zwl_object *top;
	const struct zwl_import *image;
	struct shell_rect body;
	unsigned panels;
	float progress;
	int still;

	/* The fullscreen window on top with nothing over it (fullscreen_top), App Home and Wiseview gone. */
	top = fullscreen_top(server);
	if (top == NULL || home > 0.0f || server->home_to > 0.0f)
		return 0;
	progress = wiseview_progress(server);
	if (progress > 0.0f)
		return 0;

	/* Nothing moves or is open over it: no animation, drag, sliding desktops, popup or menu (zwl_glass_still). */
	still = zwl_glass_still(server);
	if (!still)
		return 0;

	/* Its image is opaque, with no sub-surfaces under it and no glass panels. */
	image = zwl_compose_surface_image(top);
	if (image == NULL || image->draw != ZWL_DRAW_OPAQUE)
		return 0;
	panels = zwl_panels_count(top);
	if (top->sub_children != NULL || panels > 0U)
		return 0;

	/* It covers the output. */
	body_rect(server, top, &body);
	if (body.x > 0 || body.y > 0 ||
	    body.x + body.width < (int32_t)server->width ||
	    body.y + body.height < (int32_t)server->height)
		return 0;

	/* Succeeded: only that window is drawn. */
	return 1;
}

/*
 * Starts and ends the bottom edge's swipe over a fullscreen window: a left
 * press in the bottom edge starts it, the release ends it.  Returns 1 when
 * the button was the swipe's.
 */
static int
unfullscreen_press(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	/* Another button while the swipe holds its contact is the swipe's too. */
	if (button != ZWL_BUTTON_LEFT)
		return unfullscreen_swipe.pressing;

	/* The release ends it. */
	if (state == 0U) {
		if (!unfullscreen_swipe.pressing)
			return 0;
		unfullscreen_swipe.pressing = 0;
		printf("ZWL GLASS unfullscreen-swipe end done=%d\n", unfullscreen_swipe.done);
		return 1;
	}

	/* Only a press in the bottom edge starts it: a stroke that starts above it is the window's. */
	if (server->pointer_y < (int32_t)server->height - WISEVIEW_EDGE)
		return 0;

	/* The contact is the swipe's from now on. */
	unfullscreen_swipe.pressing = 1;
	unfullscreen_swipe.done = 0;
	unfullscreen_swipe.start_y = server->pointer_y;
	printf("ZWL GLASS unfullscreen-swipe start y=%d source=%d\n", server->pointer_y, (int)server->shell_source);

	/* Succeeded: the press is the swipe's. */
	return 1;
}

/*
 * Follows the bottom edge's swipe: once its contact has moved up
 * UNFULLSCREEN_DISTANCE, the fullscreen window goes back to a window, where
 * zwl_window_leave_fullscreen puts it (BUG-114's rule).  Returns 1 while the
 * swipe holds the contact.
 */
static int
unfullscreen_motion(
	struct zwl_server *server)
{
	struct zwl_object *top;
	int error;

	/* No swipe, or one that has done its work already. */
	if (!unfullscreen_swipe.pressing)
		return 0;
	if (unfullscreen_swipe.done)
		return 1;

	/* Not far enough up yet. */
	if (unfullscreen_swipe.start_y - server->pointer_y < UNFULLSCREEN_DISTANCE)
		return 1;

	/* The window that is fullscreen on top now; one that went away meanwhile ends the swipe's work. */
	unfullscreen_swipe.done = 1;
	top = fullscreen_top(server);
	if (top == NULL)
		return 1;

	/* It becomes a window again. */
	error = zwl_window_leave_fullscreen(top);
	printf("ZWL GLASS unfullscreen surface=%u via=swipe errno=%d at_ms=%llu\n", top->id, error, (unsigned long long)zwl_milliseconds());

	/* Succeeded: the contact stays the swipe's until its release. */
	return 1;
}

/*
 * Tells whether an edge's gesture shows something over the windows (the
 * top-right corner's hint, App Home, Wiseview, the keyboard), so that the
 * input goes to it as in window mode even while the top window is
 * fullscreen (zwl_glass_fullscreen_input).
 */
int
zwl_glass_overlay(
	struct zwl_server *server)
{
	float progress;
	int showing;

	/* Only the glass look has the gestures. */
	if (!server->glass)
		return 0;

	/* The top-right corner's hint (corner.c). */
	showing = zwl_corner_showing();
	if (showing)
		return 1;

	/* The on-screen keyboard's panel or its swipe's hint (keyboard.c). */
	showing = zwl_keyboard_showing();
	if (showing)
		return 1;

	/* App Home, opening, open or closing. */
	progress = zwl_home_progress(server);
	if (progress > 0.0f || server->home_to > 0.0f)
		return 1;

	/* Wiseview's gesture once it has moved (a click at the bottom edge shows nothing). */
	if (server->wiseview_gesture) {
		progress = wiseview_progress(server);
		if (progress > 0.0f)
			return 1;
	}

	/* Wiseview, open or settling. */
	if (server->wiseview > 0.0f || server->wiseview_moving)
		return 1;

	/* Nothing shows. */
	return 0;
}

/*
 * Brings a window to the top and gives it the focus (for the menus, whose
 * press on a title bar does what a press on it does).
 */
void
zwl_glass_raise(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	/* The same as a press on the window. */
	window_raise(server, surface);
}

/*
 * Finds the topmost window whose title bar or body is at a point (for the
 * menus, whose items in a title bar another window may cover); NULL when
 * there is none.
 */
struct zwl_object *
zwl_glass_window_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y)
{
	struct zwl_object *surface;
	enum shell_hit hit;

	/* The same search a press makes. */
	surface = window_at(server, x, y, &hit);

	/* Succeeded: the window, or NULL. */
	return surface;
}

/*
 * Finds the window whose body (its image, not its titlebar) is on top at a
 * point in the glass look; NULL for none (a drag and drop's target,
 * data.c).
 */
struct zwl_object *
zwl_glass_body_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y)
{
	struct zwl_object *surface;
	enum shell_hit hit;

	/* The same search a press makes; only a body counts. */
	surface = window_at(server, x, y, &hit);
	if (hit != HIT_BODY)
		return NULL;

	/* Succeeded: the window. */
	return surface;
}

/*
 * Finds the window whose floating title bar a press at a point would reach
 * (touch.c, which holds a finger on a title bar back a moment to see
 * whether a second one comes); NULL when the press would go anywhere else:
 * to a screen or a menu over the windows, to an edge's gesture, to the
 * system bar, to a body, or to the desktop.
 */
struct zwl_object *
zwl_glass_title_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y)
{
	struct zwl_object *surface;
	enum shell_hit hit;
	float home;
	int fullscreen;
	int open;

	/* Only the glass look's window mode has floating title bars. */
	if (!server->glass || !server->windowed)
		return NULL;

	/* The login and lock screens, a drag and drop and a popup's grab take every press. */
	if (server->greeter || server->locked || server->dnd_active)
		return NULL;
	if (server->popup_grab != NULL)
		return NULL;

	/* A fullscreen window has no title bar; only the edges' gestures are over it. */
	fullscreen = zwl_glass_fullscreen_input(server);
	if (fullscreen)
		return NULL;

	/* Wiseview, open or being opened, takes every press. */
	if (server->wiseview_gesture || server->wiseview > 0.0f || server->wiseview_moving)
		return NULL;

	/* App Home takes every press while it shows or follows one. */
	home = zwl_home_progress(server);
	if (home > 0.0f || server->home_to > 0.0f)
		return NULL;

	/* The on-screen keyboard's panel and its bottom corners are the keyboard's (keyboard.c). */
	open = zwl_keyboard_at(x, y);
	if (open)
		return NULL;
	if (y >= (int32_t)server->height - ZWL_KEYBOARD_ZONE &&
	    (x < ZWL_KEYBOARD_ZONE || x >= (int32_t)server->width - ZWL_KEYBOARD_ZONE))
		return NULL;

	/* An open menu closes on a press anywhere. */
	open = zwl_network_is_open();
	if (open)
		return NULL;
	open = zwl_volume_is_open();
	if (open)
		return NULL;
	open = zwl_menu_is_open();
	if (open)
		return NULL;

	/* The system bar, the desktops' swipe at the side edges and Wiseview's bottom edge come before the windows. */
	if (y < ZWL_GLASS_BAR)
		return NULL;
	if (x < DESKTOP_EDGE || x >= (int32_t)server->width - DESKTOP_EDGE)
		return NULL;
	if (y >= (int32_t)server->height - WISEVIEW_EDGE)
		return NULL;

	/* The topmost window at the point, when the point is on its title bar. */
	surface = window_at(server, x, y, &hit);
	if (surface == NULL || hit != HIT_TITLE)
		return NULL;

	/* Succeeded: the window whose title bar it is. */
	return surface;
}

/*
 * Sends a window to the back and gives the focus to the window now on top,
 * as a triple click on its title bar does (touch.c's two-finger flick up,
 * "go away").
 */
void
zwl_glass_lower(
	struct zwl_server *server,
	struct zwl_object *surface,
	const char *via)
{
	/* A run of clicks or a double click's dock waiting on the window is over. */
	if (server->dock_waiting == surface)
		server->dock_waiting = NULL;
	if (server->click_surface == surface) {
		server->click_surface = NULL;
		server->click_count = 0;
	}

	/* Succeeded: the same as the triple click. */
	window_lower(server, surface, via);
}

/*
 * Draws zdesktop's badge of a drag and drop without an icon of its own
 * (data.c): a small white page below and right of the pointer, with an
 * outline and two lines of text, so the user sees something being carried.
 */
void
zwl_glass_draw_drag_badge(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	static const float edge[4] = { 0.12f, 0.16f, 0.24f, 0.35f };
	static const float page[4] = { 1.0f, 1.0f, 1.0f, 0.97f };
	static const float line[4] = { 0.25f, 0.52f, 0.98f, 0.75f };
	float x;
	float y;

	/* Below and right of the pointer, clear of the arrow. */
	x = (float)server->pointer_x + 14.0f;
	y = (float)server->pointer_y + 16.0f;

	/* The page: its outline, then its face. */
	glass_draw_solid(server, command, x - 1.0f, y - 1.0f, 24.0f, 30.0f, 5.0f, edge);
	glass_draw_solid(server, command, x, y, 22.0f, 28.0f, 4.0f, page);

	/* Two lines of text on it. */
	glass_draw_solid(server, command, x + 5.0f, y + 8.0f, 12.0f, 2.0f, 1.0f, line);
	glass_draw_solid(server, command, x + 5.0f, y + 14.0f, 9.0f, 2.0f, 1.0f, line);
}

/*
 * Tells whether the glass look is still (ws035-p055): nothing moves or
 * fades by itself -- no window animation, move, pull or drag, no Home or
 * Wiseview, no desktop sliding, no see-through bodies -- so that a change
 * can be drawn in a rectangle of its own.
 */
int
zwl_glass_still(
	struct zwl_server *server)
{
	struct zwl_object *popups[1];
	float home;
	unsigned open;

	/* Something moves (a drag and drop's icon or badge too). */
	if (server->anim != NULL || server->drag != NULL || server->pull != NULL || server->dnd_active)
		return 0;
	if (server->desktop_moving || server->desktop_dragging)
		return 0;
	if (server->wiseview > 0.0f || server->wiseview_gesture || server->wiseview_moving)
		return 0;

	/* Home, even closing. */
	home = zwl_home_progress(server);
	if (home > 0.0f)
		return 0;

	/* The top-right corner's hint (corner.c). */
	open = (unsigned)zwl_corner_showing();
	if (open)
		return 0;

	/* The on-screen keyboard (keyboard.c). */
	open = (unsigned)zwl_keyboard_showing();
	if (open)
		return 0;

	/* A see-through body shows what is under it through its glass. */
	if (server->window_opacity < 1.0f)
		return 0;

	/* A popup or a menu open over the windows. */
	open = zwl_popup_collect(server, popups, 1U);
	if (open > 0U)
		return 0;
	open = zwl_menu_is_open();
	if (open)
		return 0;
	open = (unsigned)zwl_network_is_open();
	if (open)
		return 0;
	open = (unsigned)zwl_volume_is_open();
	if (open)
		return 0;

	/* Still. */
	return 1;
}

/*
 * Finds the rectangle (left, top, right, bottom) a window's body alone
 * covers, when a change of its image can be drawn there alone: the look is
 * still, the window is shown, and no window above it comes near enough
 * for its glass to blur the change in (ws035-p055).  Returns 1 with the
 * rectangle, 0 when the whole output must be drawn.
 */
int
zwl_glass_body_damage(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t *rect)
{
	struct zwl_client *client;
	struct zwl_object *other;
	struct shell_rect body;
	struct shell_rect near;
	int still;
	int close;

	/* A still look, and a window on the desktop shown. */
	still = zwl_glass_still(server);
	if (!still || !surface->mapped || surface->minimized || surface->desktop != server->desktop)
		return 0;

	/* The body, and the reach of the glass's blur around it. */
	body_rect(server, surface, &body);
	near.x = body.x - DAMAGE_REACH;
	near.y = body.y - DAMAGE_REACH;
	near.width = body.width + 2 * DAMAGE_REACH;
	near.height = body.height + 2 * DAMAGE_REACH;

	/* No window above it within the reach (its body and its title bar). */
	for (client = server->clients; client != NULL; client = client->next) {
		for (other = client->objects; other != NULL; other = other->next) {
			if (other == surface || other->kind != ZWL_SURFACE || !other->mapped)
				continue;
			if (other->map_order < surface->map_order || other->minimized || other->desktop != server->desktop)
				continue;
			close = damage_near(server, other, &near);
			if (close)
				return 0;
		}
	}

	/* Succeeded: the body alone. */
	rect[0] = body.x;
	rect[1] = body.y;
	rect[2] = body.x + body.width;
	rect[3] = body.y + body.height;
	return 1;
}

/*
 * Tells whether the pointer at a point is over a window's body (its
 * client's own area) in a still look, where zdesktop draws nothing that
 * follows the pointer but the cursor (ws035-p055).
 */
int
zwl_glass_pointer_calm(
	struct zwl_server *server,
	int32_t x,
	int32_t y)
{
	struct zwl_object *surface;
	enum shell_hit hit;
	int still;

	/* A still look. */
	still = zwl_glass_still(server);
	if (!still)
		return 0;

	/* Over a body. */
	surface = window_at(server, x, y, &hit);
	if (surface == NULL || hit != HIT_BODY)
		return 0;

	/* Calm. */
	return 1;
}

/*
 * Finds where a window's body (its image) is drawn now: at its place, in
 * the docked space, or on its way while animated (for its popups, popup.c).
 * Returns 0.
 */
int
zwl_glass_body_origin(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t *x,
	int32_t *y)
{
	struct shell_rect body;

	/* The rectangle the body is drawn in. */
	body_rect(server, surface, &body);

	/* Succeeded: its top-left corner. */
	*x = body.x;
	*y = body.y;
	return 0;
}

/*
 * Carries out what a toplevel asks of the shell (ws035-p076): a move the
 * client started from its own title bar, maximize (dock) and unmaximize,
 * and minimize.
 */
void
zwl_glass_toplevel_request(
	struct zwl_server *server,
	struct zwl_object *surface,
	int request)
{
	/* Only a shown window of the desktop shown. */
	if (surface->dead || !surface->mapped || surface->desktop != server->desktop)
		return;

	/* What was asked. */
	switch (request) {
	case ZWL_TOPLEVEL_MOVE:
		/* A move as a press on the title bar starts one, until the button is let go (a docked window stays). */
		if (surface->maximized || server->drag != NULL)
			break;
		window_raise(server, surface);
		server->drag = surface;
		server->drag_dx = server->pointer_x - surface->x;
		server->drag_dy = server->pointer_y - surface->y;
		server->drag_start_x = surface->x;
		server->drag_start_y = surface->y;
		printf("ZWL GLASS request move surface=%u\n", surface->id);
		break;
	case ZWL_TOPLEVEL_MAXIMIZE:
		/* Docked where it is. */
		window_dock(server, surface, surface->x, surface->y, "request");
		break;
	case ZWL_TOPLEVEL_UNMAXIMIZE:
		/* Back to its place before it docked. */
		window_undock(server, surface, surface->restore_x, surface->restore_y, "request");
		break;
	case ZWL_TOPLEVEL_MINIMIZE:
		/* Hidden until Wiseview brings it back. */
		window_minimize(server, surface);
		break;
	default:
		break;
	}
}

/*
 * Places a new window in the glass look (ws035-p092).  The places tried, in
 * order: the centre of the space below the system bar, then down and right
 * of the top window a title bar's height at a time, then from the space's
 * top-left corner the same way, each kept inside the space.  The first
 * place that hides no other window's title (the start of its title bar)
 * and is near no other window's corner is taken; when every place hides
 * some, the one that hides the fewest.
 */
void
zwl_glass_place(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t width,
	int32_t height,
	int32_t step)
{
	int32_t places[1 + 2 * GLASS_CASCADE_ROUNDS][2];
	int32_t space_width;
	int32_t space_height;
	int32_t top_x;
	int32_t top_y;
	unsigned count;
	unsigned best;
	unsigned index;
	int crowd;
	int least;
	int found;
	int round;

	/* The centre of the space for bodies under the system bar and a title bar (the plain look's cascade step is not used). */
	(void)step;
	zwl_glass_space(server, &space_width, &space_height);
	places[0][0] = ((int32_t)server->width - width) / 2;
	places[0][1] = ZWL_GLASS_TOP + (space_height - height) / 2;
	count = 1U;

	/* Down and right of the top window. */
	found = glass_top(server, surface, &top_x, &top_y);
	for (round = 1; found && round <= GLASS_CASCADE_ROUNDS; round++) {
		places[count][0] = top_x + round * GLASS_CASCADE;
		places[count][1] = top_y + round * GLASS_CASCADE;
		count++;
	}

	/* Down and right of the space's top-left corner. */
	for (round = 0; round < GLASS_CASCADE_ROUNDS; round++) {
		places[count][0] = ZWL_GLASS_MARGIN + round * GLASS_CASCADE;
		places[count][1] = ZWL_GLASS_TOP + round * GLASS_CASCADE;
		count++;
	}

	/* The first place that hides nothing, else the one that hides least. */
	best = 0U;
	least = -1;
	for (index = 0U; index < count; index++) {
		zwl_glass_fit(server, width, height, &places[index][0], &places[index][1]);
		glass_clear_edges(server, width, height, &places[index][0], &places[index][1]);
		crowd = glass_crowd(server, surface, places[index][0], places[index][1], width, height);
		if (least < 0 || crowd < least) {
			least = crowd;
			best = index;
		}

		/* Nothing hidden: taken. */
		if (crowd == 0)
			break;
	}

	/* Succeeded: the place. */
	surface->x = places[best][0];
	surface->y = places[best][1];
}

/*
 * Moves a new window's place so that its frame is clear of the screen's
 * edge gestures, which take a press before any frame (frame_under_pointer):
 * its bottom band above Wiseview's edge and its side bands inside the
 * desktops' edges, so each side and corner can be dragged (BUG-121: a
 * window placed down to the bottom margin could not be resized from its
 * bottom corners, and dragging one opened Wiseview).  A window too large
 * for that keeps the place it has.
 */
static void
glass_clear_edges(
	struct zwl_server *server,
	int32_t width,
	int32_t height,
	int32_t *x,
	int32_t *y)
{
	int32_t lowest;
	int32_t leftmost;
	int32_t rightmost;

	/* The bottom band above Wiseview's edge, unless the title bar would go under the system bar. */
	lowest = (int32_t)server->height - WISEVIEW_EDGE - FRAME_BAND - height;
	if (*y > lowest && lowest >= ZWL_GLASS_TOP)
		*y = lowest;

	/* The side bands inside the desktops' edges. */
	leftmost = DESKTOP_EDGE + FRAME_BAND;
	rightmost = (int32_t)server->width - DESKTOP_EDGE - FRAME_BAND - width;

	/* A window too wide for both keeps its place across. */
	if (rightmost < leftmost)
		return;

	/* Otherwise it comes in from whichever side it was over. */
	if (*x < leftmost)
		*x = leftmost;
	if (*x > rightmost)
		*x = rightmost;

	/* Succeeded: the place clears the edges it can. */
	return;
}


/*
 * Counts what a new window at a place would hide of the other windows of
 * the desktop shown: each title whose first letters (right of its mark)
 * it covers counts one, each corner within GLASS_NEAR of its own counts
 * four.
 */
static int
glass_crowd(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height)
{
	struct zwl_client *client;
	struct zwl_object *other;
	struct shell_rect body;
	int32_t title_x;
	int32_t title_y;
	int32_t dx;
	int32_t dy;
	int placed;
	int crowd;

	/* Each shown window. */
	crowd = 0;
	for (client = server->clients; client != NULL; client = client->next) {
		for (other = client->objects; other != NULL; other = other->next) {
			placed = glass_placed(server, surface, other);
			if (!placed)
				continue;
			body_rect(server, other, &body);

			/* Its title's first letters (their lower half), under the new window and its title bar. */
			title_x = body.x + GLASS_TITLE_START;
			title_y = body.y - ZWL_GLASS_GAP - ZWL_GLASS_TITLE / 2 + GLASS_TITLE_LOW;
			if (title_x >= x && title_x < x + width && title_y >= y - ZWL_GLASS_GAP - ZWL_GLASS_TITLE && title_y < y + height)
				crowd++;

			/* Its corner, near the new one's. */
			dx = body.x - x;
			dy = body.y - y;
			if (dx > -GLASS_NEAR && dx < GLASS_NEAR && dy > -GLASS_NEAR && dy < GLASS_NEAR)
				crowd += 4;
		}
	}

	/* Succeeded: how much is hidden. */
	return crowd;
}

/* Gives the corner of the top window of the desktop shown other than surface; 0 when there is none. */
static int
glass_top(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t *x,
	int32_t *y)
{
	struct zwl_client *client;
	struct zwl_object *other;
	struct zwl_object *top;
	struct shell_rect body;
	int placed;

	/* The shown window mapped or raised last. */
	top = NULL;
	for (client = server->clients; client != NULL; client = client->next) {
		for (other = client->objects; other != NULL; other = other->next) {
			placed = glass_placed(server, surface, other);
			if (!placed)
				continue;
			if (top == NULL || other->map_order > top->map_order)
				top = other;
		}
	}

	/* Its body's corner. */
	if (top == NULL)
		return 0;
	body_rect(server, top, &body);
	*x = body.x;
	*y = body.y;
	return 1;
}

/* Tells whether an object is a window placement looks at: shown on the desktop shown, not docked, and not the new one. */
static int
glass_placed(
	struct zwl_server *server,
	struct zwl_object *surface,
	struct zwl_object *other)
{
	/* A mapped window of the desktop shown. */
	if (other == surface || other->kind != ZWL_SURFACE || other->dead || !other->mapped)
		return 0;
	if (other->role == NULL || other->cursor_role || other->minimized || other->maximized || other->fullscreen)
		return 0;

	/* Succeeded: when on the desktop shown. */
	return other->desktop == server->desktop;
}

/*
 * Gives the largest body a window can have and still be seen whole in the
 * glass look: the output less the margins at its sides and bottom, the
 * system bar, and a floating title bar above the body.  xdg-shell's
 * configure_bounds tells windows this size (protocol.c).
 */
void
zwl_glass_space(
	struct zwl_server *server,
	int32_t *width,
	int32_t *height)
{
	int32_t space_width;
	int32_t space_height;
	int32_t right;
	int32_t bottom;

	/* What the on-screen keyboard's panel takes at the right or the bottom (keyboard.c, ws102-p007). */
	zwl_keyboard_reserved(&right, &bottom);

	/* The output less a margin at each side, and the keyboard's column. */
	space_width = (int32_t)server->width - 2 * ZWL_GLASS_MARGIN - right;
	if (space_width < 0)
		space_width = 0;

	/* The output under the system bar and a title bar, less the bottom margin and the keyboard's row. */
	space_height = (int32_t)server->height - ZWL_GLASS_TOP - ZWL_GLASS_MARGIN - bottom;
	if (space_height < 0)
		space_height = 0;

	/* Both at once. */
	*width = space_width;
	*height = space_height;
}

/*
 * Moves a place so that a body of a size ends inside the glass look's
 * space; a body too large for it starts at the space's top-left corner and
 * overhangs right and down.  The space starts under the system bar and a
 * floating title bar, so the title bar is never under the system bar.
 */
void
zwl_glass_fit(
	struct zwl_server *server,
	int32_t width,
	int32_t height,
	int32_t *x,
	int32_t *y)
{
	int32_t space_width;
	int32_t space_height;
	int32_t right;
	int32_t bottom;

	/* Its right edge inside the space. */
	zwl_glass_space(server, &space_width, &space_height);
	right = ZWL_GLASS_MARGIN + space_width;
	if (*x + width > right)
		*x = right - width;

	/* And its bottom edge. */
	bottom = ZWL_GLASS_TOP + space_height;
	if (*y + height > bottom)
		*y = bottom - height;

	/* Never above the space, nor left of it. */
	if (*x < ZWL_GLASS_MARGIN)
		*x = ZWL_GLASS_MARGIN;
	if (*y < ZWL_GLASS_TOP)
		*y = ZWL_GLASS_TOP;
}

/*
 * Starts the growth of a newly mapped window out of App Home's icon, when
 * it is the window of an application Home just started (ws035-p071).
 */
void
zwl_glass_mapped(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	struct shell_rect to;
	int32_t from[4];
	int launched;

	/* Only the glass look animates, and only the window a launch waits for. */
	if (!server->glass)
		return;
	launched = zwl_home_launched(server, from);
	if (!launched)
		return;

	/* From the icon's rectangle to the window's own. */
	body_rect(server, surface, &to);
	memcpy(server->anim_from, from, sizeof(server->anim_from));
	memcpy(server->anim_to, &to, sizeof(server->anim_to));
	server->anim = surface;
	server->anim_docking = ANIM_LAUNCH;
	server->anim_start_ms = zwl_milliseconds();
	server->dirty = 1;
	printf("ZWL GLASS launch surface=%u from=%d,%d to=%d,%d size=%dx%d\n", surface->id, from[0], from[1], to.x, to.y, to.width, to.height);
}

/*
 * Handles zdesktop's shortcuts: Ctrl+Alt+Left and Right switch to the
 * desktop before and after, Super+Tab opens Wiseview (and while it is open
 * every key is Wiseview's).  Returns 1 when the key is zdesktop's.
 */
int
zwl_glass_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	struct zwl_object *surface;
	int showing;
	int target;
	int step;
	int taken;

	/* The volume's open popup takes every key (volume.c). */
	taken = zwl_volume_key(server, key, state);
	if (taken)
		return 1;

	/* Wiseview, open or opening, takes every key (ws035-p014). */
	showing = wiseview_showing(server);
	if (showing) {
		wiseview_key(server, key, state);
		return 1;
	}

	/* Super+Tab opens Wiseview from the keyboard, as Windows+Tab does. */
	if (key == KEY_TAB && (server->modifiers & MODIFIER_SUPER) != 0U) {
		if (state != 0U)
			wiseview_open_key(server);
		return 1;
	}

	/* Super+Alt with a letter: the editing operations and the previous application (edit.c, ws102-p017). */
	taken = zwl_edit_key(server, key, state);
	if (taken)
		return 1;

	/* Only with Control and Alt held, and only the two arrows. */
	if ((server->modifiers & MODIFIERS_CONTROL_ALT) != MODIFIERS_CONTROL_ALT)
		return 0;
	if (key != SHORTCUT_LEFT && key != SHORTCUT_RIGHT)
		return 0;

	/* With Shift, the window on top goes along to the neighbour. */
	step = 1;
	if (key == SHORTCUT_LEFT)
		step = -1;
	if (state != 0U && (server->modifiers & MODIFIER_SHIFT) != 0U) {
		surface = sheet_owner(zwl_top_window(server));
		target = (int)server->desktop + step;
		if (surface != NULL && target >= 0 && target < DESKTOPS)
			window_to_desktop(server, surface, (unsigned)target, "key");
	}

	/* A press switches; the release is the shortcut's too. */
	if (state != 0U)
		desktop_turn(server, (int)server->desktop + step, "key");
	return 1;
}

/*
 * Keeps the output being redrawn: every frame while the dock animation runs
 * (ending it after DOCK_MS), and when the clock shows a new minute.
 */
void
zwl_glass_tick(
	struct zwl_server *server)
{
	uint64_t elapsed;
	uint64_t idle;
	float progress;
	time_t now;

	/* The login screen has only its clock and sessiond's answers (greeter.c). */
	if (server->greeter) {
		zwl_greeter_tick(server);
		return;
	}

	/* What sessiond sent the session (handoff.c). */
	zwl_handoff_tick(server);

	/* The lock screen has only its clock (its answers came above). */
	if (server->locked) {
		zwl_greeter_tick(server);
		return;
	}

	/* A session left without input for long enough locks (ws035-p102). */
	idle = zwl_milliseconds() - server->lock_input_ms;
	if (server->lock_idle_ms != 0U && idle >= server->lock_idle_ms)
		(void)zwl_lock(server, "idle");

	/* The sheets where their parents are (ws090-p014). */
	sheet_place(server);

	/* App Home's animation, and the applications it started that have ended. */
	zwl_home_tick(server);

	/* The top-right corner's swipe: its time limit, its hint settling, and Notes being waited for (corner.c). */
	zwl_corner_tick(server);

	/* The on-screen keyboard's swipe and its panel's place (keyboard.c). */
	zwl_keyboard_tick(server);

	/* A finger on a title bar that has waited long enough for a second one, or two that did not flick in time (touch.c). */
	zwl_touch_tick(server);

	/* A double click on a title bar docks its window once a third press can no longer come. */
	dock_when_due(server);

	/* An open menu closes when what it belongs to changed (menu-shell.c). */
	zwl_menu_tick(server);

	/* What the network watch brought (network.c). */
	zwl_network_tick(server);

	/* What audiod reported, and the volume's sends held back (volume.c). */
	zwl_volume_tick(server);

	/* The desktops' slide draws every frame until it is done. */
	if (server->desktop_moving) {
		server->dirty = 1;
		elapsed = zwl_milliseconds() - server->desktop_start_ms;
		if (elapsed >= DESKTOP_MS) {
			server->desktop_moving = 0;
			printf("ZWL GLASS desktop settled desktop=%u windows=%u\n", server->desktop + 1U, desktop_windows(server, server->desktop));
		}
	}

	/* The animation draws every frame until it is done. */
	if (server->anim != NULL) {
		progress = animation_progress(server);
		server->dirty = 1;
		if (progress >= 1.0f)
			server->anim = NULL;
	}

	/* Wiseview draws every frame while it settles; at the end its value is where it went. */
	if (server->wiseview_moving) {
		server->dirty = 1;
		elapsed = zwl_milliseconds() - server->wiseview_start_ms;
		if (elapsed >= WISEVIEW_MS) {
			server->wiseview_moving = 0;
			server->wiseview = server->wiseview_to;
			if (server->wiseview > 0.0f)
				wiseview_log(server);
			else
				printf("ZWL WISEVIEW closed at_ms=%llu\n", (unsigned long long)zwl_milliseconds());
		}
	}

	/* The minute of the clock. */
	now = time(NULL);
	if ((int64_t)now / 60 == server->clock_minute)
		return;

	/* A new minute. */
	server->clock_minute = (int64_t)now / 60;
	server->dirty = 1;
}

/*
 * Lays out the system bar from the right: the clock, the battery, the
 * signal, a line, the desktops, a line, the docked window's buttons; and
 * from the left the launcher, a line and the docked title.
 */
static void
bar_layout(
	struct zwl_server *server,
	struct shell_bar *bar)
{
	struct tm local;
	time_t now;
	int button;

	/* The date and time at the right edge. */
	now = time(NULL);
	memset(&local, 0, sizeof(local));
	(void)localtime_r(&now, &local);
	bar->clock[0] = '\0';
	(void)strftime(bar->clock, sizeof(bar->clock), "%a %b %e  %H:%M", &local);
	bar->clock_x = (int32_t)server->width - 16 - glass_text_width(server, SIZE_BAR, bar->clock);

	/* The battery and the signal left of it, and a line. */
	bar->battery_x = bar->clock_x - 44;
	bar->signal_x = bar->battery_x - 36;
	bar->volume_x = bar->signal_x - 34;
	bar->status_line = bar->volume_x - 16;

	/* The desktops, and a line. */
	bar->desktops_width = DESKTOPS * DESKTOP_WIDTH + (DESKTOPS - 1) * DESKTOP_GAP + 12;
	bar->desktops_x = bar->status_line - 16 - bar->desktops_width;
	bar->desktops_line = bar->desktops_x - 16;

	/* The docked window's buttons, close nearest the line. */
	for (button = 0; button < BUTTON_COUNT; button++)
		bar->buttons[button] = bar->desktops_line - 30 - button * BAR_BUTTON_SPACING;

	/* On the left, after the launcher, a line and the docked title (ws035-p117: no word after the mark). */
	bar->menu_line = BAR_LAUNCHER_X + BAR_LAUNCHER_SIZE + 12;
	bar->title_x = bar->menu_line + 17;
}

/*
 * Tells whether a window is drawn in this frame: not minimized, on the
 * desktop shown or on one sliding in beside it (only the desktop shown
 * while Home is open).
 */
static int
window_shown(
	struct zwl_server *server,
	struct zwl_object *surface,
	float home,
	float position)
{
	float shift;

	/* A minimized window, or one a screen or more to the side. */
	shift = ((float)surface->desktop - position) * (float)server->width;
	if (shift <= -(float)server->width || shift >= (float)server->width || surface->minimized)
		return 0;

	/* With Home open, only the desktop shown. */
	if (home > 0.0f && surface->desktop != server->desktop)
		return 0;

	/* The window is drawn. */
	return 1;
}

/* Moves the layer with a window's desktop while the desktops slide (Home, when open, has the layer instead). */
static void
window_layer(
	struct zwl_server *server,
	struct zwl_object *surface,
	float home,
	float position)
{
	float shift;

	/* Home keeps its own layer. */
	if (home > 0.0f)
		return;

	/* The window's desktop's place beside the one shown. */
	shift = ((float)surface->desktop - position) * (float)server->width;
	server->layer_on = 0;
	if (shift != 0.0f)
		server->layer_on = 1;
	server->layer_x = shift;
	server->layer_y = 0.0f;
	server->layer_scale = 1.0f;
}

/*
 * Draws the scene under a window into the backdrop (backdrop.c): the
 * wallpaper and the windows below it, whose own glass is on the blurred
 * wallpaper.  The glass drawn after it is on this scene, blurred.
 */
static void
draw_backdrop(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object **windows,
	unsigned below,
	float position)
{
	struct glass_shape shape;
	unsigned index;
	int started;
	int shown;

	/* The backdrop's pass; a device without it keeps the blurred wallpaper. */
	started = zwl_backdrop_begin(server, command);
	if (!started)
		return;

	/* The wallpaper, not moved. */
	server->layer_on = 0;
	glass_shape_init(&shape, 0.0f, 0.0f, (float)server->width, (float)server->height);
	shape.mode = MODE_IMAGE;
	shape.opaque = 1.0f;
	shape.set = glass_wallpaper_set(server);
	glass_shape_draw(server, command, &shape);

	/* The desktop's icons on it (desktop.c). */
	zwl_desktop_draw(server, command);

	/* The windows below, as the blur will show them. */
	for (index = 0; index < below; index++) {
		shown = window_shown(server, windows[index], 0.0f, position);
		if (!shown)
			continue;
		window_layer(server, windows[index], 0.0f, position);
		draw_window_blurred(server, command, windows[index]);
	}

	/* The output's pass again, and the scene blurred for the glass. */
	zwl_backdrop_end(server, command);
}

/*
 * Draws a window for the backdrop, where it is only seen blurred: its body
 * and a floating title bar's glass, without the title, the menu, the
 * controls or the buttons (which would also record their places twice).
 */
static void
draw_window_blurred(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface)
{
	struct shell_rect body;
	struct shell_rect panel;
	struct glass_shape shape;
	int decorated;

	/* The body where it is now (docked, its lower corners below the output). */
	body_rect(server, surface, &body);
	draw_body(server, command, surface, &body, surface->maximized, 0U);
	if (surface->maximized)
		return;

	/* Blurred scenes must not recreate the titlebar removed from CSD windows. */
	decorated = zwl_decoration_server(surface);
	if (!decorated)
		return;

	/* A floating title bar's glass (rounded even on a window whose body keeps square corners). */
	floating_title(&body, &panel);
	glass_shape_init(&shape, (float)panel.x, (float)panel.y, (float)panel.width, (float)panel.height);
	shape.mode = MODE_GLASS;
	shape.radius = GLASS_RADIUS;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.38f;
	shape.edge = 0.85f;
	glass_shape_draw(server, command, &shape);
}

/*
 * Draws a window: its body, and its title bar floating above it, docked in
 * the system bar (drawn with the bar), or on its way between the two.
 */
static void
draw_window(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned focused,
	const struct shell_bar *bar)
{
	struct shell_rect body;
	struct shell_rect from;
	struct shell_rect to;
	struct shell_rect panel;
	struct shell_rect slot;
	struct zwl_object *parent;
	float t;
	int decorated;

	/* A sheet has only its body, under its parent's title bar (ws090-p014). */
	parent = zwl_sheet_parent(surface);
	if (parent != NULL) {
		draw_sheet(server, command, surface, parent, focused);
		return;
	}

	/* The body, where it is now. */
	body_rect(server, surface, &body);

	/* Client-decorated windows supply their own frame, controls and animation content. */
	decorated = zwl_decoration_server(surface);
	if (!decorated) {
		draw_body(server, command, surface, &body, 0, focused);

		/* The client image is the entire decorated window. */
		return;
	}

	/* A window a launch from Home started grows out of the icon; its title bar fades in on it. */
	if (server->anim == surface && server->anim_docking == ANIM_LAUNCH) {
		t = animation_progress(server);
		draw_body(server, command, surface, &body, 0, focused);
		floating_title(&body, &panel);
		draw_title_bar(server, command, surface, &panel, t, t, focused);
		return;
	}

	/* While docking or coming back, the title bar slides between its two places and its glass fades. */
	if (server->anim == surface) {
		t = animation_progress(server);
		draw_body(server, command, surface, &body, 0, focused);
		bar_title_slot(server, bar, &slot);
		memcpy(&from, server->anim_from, sizeof(from));
		memcpy(&to, server->anim_to, sizeof(to));
		if (server->anim_docking) {
			floating_title(&from, &panel);
			lerp_rect(&panel, &slot, t, &panel);
			draw_title_bar(server, command, surface, &panel, 1.0f - t, 1.0f - t, focused);
		} else {
			floating_title(&to, &panel);
			lerp_rect(&slot, &panel, t, &panel);
			draw_title_bar(server, command, surface, &panel, t, t, focused);
		}

		/* Nothing more is drawn for it. */
		return;
	}

	/* A docked window being pulled has round corners and its floating title bar, fading in with the pull. */
	if (surface->maximized && surface == server->pull && server->pull_distance > 0) {
		t = (float)server->pull_distance / (float)PULL_DISTANCE;
		draw_body(server, command, surface, &body, 0, focused);
		floating_title(&body, &panel);
		draw_title_bar(server, command, surface, &panel, t, t, focused);
		return;
	}

	/* A docked window has only its body; its title is in the system bar. */
	if (surface->maximized) {
		draw_body(server, command, surface, &body, 1, focused);
		return;
	}

	/* A fullscreen window has only its body, whole (draw_body). */
	if (surface->fullscreen) {
		draw_body(server, command, surface, &body, 0, focused);
		return;
	}

	/* A floating window. */
	draw_body(server, command, surface, &body, 0, focused);
	floating_title(&body, &panel);
	draw_title_bar(server, command, surface, &panel, 1.0f, 1.0f, focused);
}

/*
 * Draws a window's body in a rectangle: its shadow, the frosted glass under
 * a see-through body, and its image with rounded corners (a docked body's
 * lower corners are below the output).  A window with glass panels
 * (panels.c) is not one slab: its panels cast the shadows and stand on the
 * glass, and its image is blended over them by its alpha.  A sheet's upper
 * corners are square, flush with its parent's title bar (ws090-p014).
 */
static void
draw_body(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	const struct shell_rect *body,
	unsigned docked,
	unsigned focused)
{
	const struct zwl_import *image;
	struct zwl_object *parent;
	struct glass_shape shape;
	float place[4];
	unsigned panels;
	int32_t width;
	int32_t height;
	float soft;
	float radius;
	unsigned square;
	int whole;
	int decorated;
	float scale_x;
	float scale_y;
	float raise;

	/* The window's size (its viewport's, else its image's), whose scale to the rectangle stretches it while it changes size. */
	image = zwl_compose_surface_image(surface);
	window_size(surface, &width, &height);
	if (width <= 0 || height <= 0) {
		width = (int32_t)image->width;
		height = (int32_t)image->height;
	}

	/* The body's scale from that size, for the panels and the sub-surfaces (in surface coordinates). */
	scale_x = (float)body->width / (float)width;
	scale_y = (float)body->height / (float)height;

	/* A window with glass panels: their shadows and glass instead of the body's (panels.c). */
	panels = zwl_panels_count(surface);
	if (panels > 0U) {
		place[0] = (float)body->x;
		place[1] = (float)body->y;
		place[2] = scale_x;
		place[3] = scale_y;
		zwl_panels_draw(server, command, surface, place, server->window_opacity, 1U);
	}

	/* The corners: rounded, or square for a window that keeps them (window_square). */
	radius = GLASS_RADIUS;
	square = window_square(surface);
	if (square)
		radius = 0.0f;

	/*
	 * A fullscreen window (not while it animates) is its image alone, as
	 * the output's (ws099-p015, as its direct scanout showed it before):
	 * square corners, no shadow, no frosted glass, not see-through.
	 */
	whole = 0;
	if (surface->fullscreen && server->anim != surface && panels == 0U) {
		whole = 1;
		radius = 0.0f;
	}

	/* CSD owns its alpha, shadows and corners; server embellishments must not clip them. */
	decorated = zwl_decoration_server(surface);
	if (!decorated) {
		whole = 1;
		radius = 0.0f;
	}

	/* A sheet's rounding reaches above its top, so that its upper corners are square. */
	raise = 0.0f;
	parent = zwl_sheet_parent(surface);
	if (parent != NULL)
		raise = 2.0f * radius;

	/* The shadow, deeper for the focused window. */
	soft = 22.0f;
	if (focused)
		soft = 30.0f;
	glass_shape_init(&shape, (float)body->x, (float)body->y + 8.0f, (float)body->width, (float)body->height);
	shape.quad[0] -= 2.0f * soft;
	shape.quad[1] -= 2.0f * soft;
	shape.quad[2] += 4.0f * soft;
	shape.quad[3] += 4.0f * soft;
	shape.mode = MODE_SHADOW;
	shape.radius = radius;
	shape.soft = soft;
	shape.color[0] = 0.10f;
	shape.color[1] = 0.18f;
	shape.color[2] = 0.35f;
	shape.color[3] = 0.20f;
	if (panels == 0U && !whole)
		glass_shape_draw(server, command, &shape);

	/* A see-through body lies on frosted glass (a window with panels has its own). */
	if (server->window_opacity < 1.0f && panels == 0U && !whole) {
		glass_shape_init(&shape, (float)body->x, (float)body->y, (float)body->width, (float)body->height);
		if (docked)
			shape.box[3] += 2.0f * radius;
		shape.box[1] -= raise;
		shape.box[3] += raise;
		shape.mode = MODE_GLASS;
		shape.radius = radius;
		shape.color[0] = 1.0f;
		shape.color[1] = 1.0f;
		shape.color[2] = 1.0f;
		shape.color[3] = 0.30f;
		shape.edge = 0.75f;
		glass_shape_draw(server, command, &shape);
	}

	/* The sub-surfaces below the image, scaled with it from the window's size (subsurface.c). */
	zwl_subsurface_draw(server, command, surface, (float)body->x, (float)body->y, scale_x, scale_y, 0U);

	/* The image (its viewport's source), stretched to the rectangle while it changes, as opaque as asked. */
	glass_shape_init(&shape, (float)body->x, (float)body->y, (float)body->width, (float)body->height);
	if (docked)
		shape.box[3] += 2.0f * radius;
	shape.box[1] -= raise;
	shape.box[3] += raise;
	zwl_viewport_source(surface, shape.uv);
	shape.opacity = server->window_opacity;
	if (whole)
		shape.opacity = 1.0f;
	shape.mode = MODE_IMAGE;
	shape.radius = radius;
	shape.set = image->set;
	if (image->draw == ZWL_DRAW_OPAQUE)
		shape.opaque = 1.0f;
	glass_shape_draw(server, command, &shape);

	/* The sub-surfaces above the image. */
	zwl_subsurface_draw(server, command, surface, (float)body->x, (float)body->y, scale_x, scale_y, 1U);
}

/*
 * Draws a title bar in a rectangle: its shadow and glass faded by fade, the
 * mark and title, and its buttons faded by buttons (a docking title bar
 * keeps its title while its glass and buttons fade).
 */
static void
draw_title_bar(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	const struct shell_rect *panel,
	float fade,
	float buttons,
	unsigned focused)
{
	static const float dark[4] = { 0.12f, 0.16f, 0.24f, 1.0f };
	static const float faint[4] = { 0.30f, 0.35f, 0.44f, 1.0f };
	struct zwl_menu_area area;
	struct glass_shape shape;
	const float *ink;
	int32_t available;
	int32_t limit;
	int32_t end;
	int32_t cx;
	int32_t cy;
	int button;
	int over;

	/* Its shadow. */
	glass_shape_init(&shape, (float)panel->x, (float)panel->y + 4.0f, (float)panel->width, (float)panel->height);
	shape.quad[0] -= 36.0f;
	shape.quad[1] -= 36.0f;
	shape.quad[2] += 72.0f;
	shape.quad[3] += 72.0f;
	shape.mode = MODE_SHADOW;
	shape.radius = GLASS_RADIUS;
	shape.soft = 18.0f;
	shape.color[0] = 0.10f;
	shape.color[1] = 0.18f;
	shape.color[2] = 0.35f;
	shape.color[3] = 0.12f;
	shape.opacity = fade;
	glass_shape_draw(server, command, &shape);

	/*
	 * The glass, whiter for the focused window; white enough for the
	 * others that their titles read over a dark window under them
	 * (ws035-p092).
	 */
	glass_shape_init(&shape, (float)panel->x, (float)panel->y, (float)panel->width, (float)panel->height);
	shape.mode = MODE_GLASS;
	shape.radius = GLASS_RADIUS;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.54f;
	if (focused)
		shape.color[3] = 0.64f;
	shape.edge = 0.85f;
	shape.opacity = fade;
	glass_shape_draw(server, command, &shape);

	/* The mark and the title, darker for the focused window, cut short to share the room before the buttons with the menu. */
	ink = faint;
	if (focused)
		ink = dark;
	available = panel->width - 44 - BUTTON_SPACING * BUTTON_COUNT - 12;
	limit = zwl_titlebar_title_limit(server, surface, available);
	draw_title(server, command, surface, panel->x + 14, panel->y + panel->height / 2, limit, ink);

	/* The window's presentation after the title (its menu or its controls), faded with the buttons (titlebar-shell.c). */
	end = title_end(server, surface, limit);
	area.x = panel->x + 44 + end + 18;
	area.top = panel->y;
	area.right = panel->x + 44 + available;
	area.height = panel->height;
	area.origin = panel->x;
	zwl_titlebar_draw(server, command, surface, 0, &area, ink, buttons);

	/* The buttons, as they fade. */
	if (buttons <= 0.0f)
		return;
	for (button = 0; button < BUTTON_COUNT; button++) {
		cx = panel->x + panel->width - 26 - button * BUTTON_SPACING;
		cy = panel->y + panel->height / 2;
		over = button_at(surface, server->pointer_x, server->pointer_y);
		draw_sign(server, command, button, cx, cy, 0, over == button && server->anim == NULL, buttons, ink);
	}
}

/*
 * Draws the application's mark at x and the title after it, centred on
 * middle: a known application's App Home picture on its colour
 * (ws035-p124), else a blue rounded square with a letter.
 */
static void
draw_title(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	int32_t x,
	int32_t middle,
	int32_t limit,
	const float *ink)
{
	char title[ZWL_TITLE_MAX + 24];
	int drawn;

	/* The title as the title bar shows it. */
	shown_title(surface, title, sizeof(title));

	/* The mark: the application's picture when it has one, else its letter. */
	drawn = draw_picture_mark(server, command, surface, x, middle, ink);
	if (!drawn)
		draw_letter_mark(server, command, surface, title, x, middle);

	/* The title. */
	glass_draw_text(server, command, SIZE_TITLE, x + 30, middle + 6, title, limit, ink);
}

/*
 * Draws a known application's mark: its App Home picture, white, on a
 * rounded square of its App Home colour, as faded as the title's ink.
 * Returns 1, or 0 (drawing nothing) for an application ID without one.
 */
static int
draw_picture_mark(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	int32_t x,
	int32_t middle,
	const float *ink)
{
	float square[4];
	float white[4];
	uint32_t rgb;
	int picture;

	/* The picture and the colour that belong to the window's application ID. */
	rgb = 0U;
	picture = zwl_icon_for_app_id(surface->app_id, &rgb);
	if (picture < 0)
		return 0;

	/* The square in the application's colour. */
	square[0] = (float)((rgb >> 16) & 0xffU) / 255.0f;
	square[1] = (float)((rgb >> 8) & 0xffU) / 255.0f;
	square[2] = (float)(rgb & 0xffU) / 255.0f;
	square[3] = ink[3];
	glass_draw_solid(server, command, (float)x, (float)(middle - 10), 20.0f, 20.0f, 6.0f, square);

	/* The picture in its middle, white. */
	white[0] = 1.0f;
	white[1] = 1.0f;
	white[2] = 1.0f;
	white[3] = ink[3];
	glass_draw_icon(server, command, (unsigned)picture, x + 3, middle - 7, 14U, white);

	/* Succeeded: the mark is drawn. */
	return 1;
}

/*
 * Draws the mark of an application without a picture: a blue rounded
 * square with a letter, the application ID's last word's, else the
 * title's first.
 */
static void
draw_letter_mark(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	const char *title,
	int32_t x,
	int32_t middle)
{
	static const float mark[4] = { 0.29f, 0.55f, 1.0f, 1.0f };
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const char *source;
	char letter[5];
	size_t length;
	size_t index;
	int32_t width;

	/* The mark. */
	glass_draw_solid(server, command, (float)x, (float)(middle - 10), 20.0f, 20.0f, 6.0f, mark);

	/* Its letter: the application ID's (it stays when the title changes), else the title's. */
	source = mark_name(surface->app_id);
	if (source == NULL)
		source = title;

	/* The first character, all its UTF-8 bytes (a lead byte says how many). */
	length = 1;
	if (((unsigned char)source[0] & 0xe0U) == 0xc0U)
		length = 2;
	else if (((unsigned char)source[0] & 0xf0U) == 0xe0U)
		length = 3;
	else if (((unsigned char)source[0] & 0xf8U) == 0xf0U)
		length = 4;
	for (index = 0; index < length && source[index] != '\0'; index++)
		letter[index] = source[index];
	letter[index] = '\0';

	/* A letter in capitals. */
	if (letter[0] >= 'a' && letter[0] <= 'z')
		letter[0] = (char)(letter[0] - 'a' + 'A');
	width = glass_text_width(server, SIZE_BAR, letter);
	glass_draw_text(server, command, SIZE_BAR, x + 10 - width / 2, middle + 5, letter, 20, white);
}

/* Tells whether a window's body keeps square corners (its application ID is in shell_square_apps). */
static unsigned
window_square(
	const struct zwl_object *surface)
{
	unsigned index;
	int same;

	/* Each application ID of the list. */
	for (index = 0; index < sizeof(shell_square_apps) / sizeof(shell_square_apps[0]); index++) {
		same = strcmp(surface->app_id, shell_square_apps[index]);
		if (same == 0)
			return 1U;
	}

	/* Succeeded: the window's corners are rounded. */
	return 0U;
}

/*
 * Gives the word of an application ID the mark's letter comes from: the
 * last one after a dot or a hyphen ("files" is "files",
 * "org.example.Viewer" is "Viewer"), or NULL when there is none that starts
 * with a letter or a digit.
 */
static const char *
mark_name(
	const char *app_id)
{
	const char *word;
	const char *at;
	char first;

	/* The text after the last dot or hyphen. */
	word = app_id;
	for (at = app_id; *at != '\0'; at++) {
		if (*at == '.' || *at == '-')
			word = at + 1;
	}

	/* A word that starts with a letter or a digit. */
	first = word[0];
	if ((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z') || (first >= '0' && first <= '9'))
		return word;

	/* Succeeded: none (the title gives the letter). */
	return NULL;
}

/* Tells how far a window's title, drawn by draw_title within a limit, reaches after its start. */
static int32_t
title_end(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t limit)
{
	char title[ZWL_TITLE_MAX + 24];
	int32_t width;

	/* The title as draw_title shows it. */
	shown_title(surface, title, sizeof(title));

	/* Its width, or the limit it is cut at. */
	width = glass_text_width(server, SIZE_TITLE, title);
	if (width > limit)
		width = limit;

	/* Succeeded: where the title ends. */
	return width;
}

/*
 * Makes the title a title bar shows: the client's ("Window" without one),
 * saying so when the client is not responding to pings (toplevel.c).
 */
static void
shown_title(
	const struct zwl_object *surface,
	char *title,
	size_t size)
{
	const char *name;

	/* A window without a title is called "Window". */
	name = surface->title;
	if (name[0] == '\0')
		name = "Window";

	/* A client that does not answer its pings. */
	if (surface->client->unresponsive) {
		(void)snprintf(title, size, "%s (not responding)", name);
		return;
	}

	/* Otherwise the name alone. */
	(void)snprintf(title, size, "%s", name);
}

/*
 * Draws one button's sign centred on (cx, cy): a line (minimize), a square
 * (maximize) or two squares (restore), or the multiplication sign (close);
 * with its background when the pointer is over it (red for close).
 */
static void
draw_sign(
	struct zwl_server *server,
	VkCommandBuffer command,
	int button,
	int32_t cx,
	int32_t cy,
	unsigned restore,
	unsigned over,
	float fade,
	const float *ink)
{
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	struct glass_shape shape;
	const float *sign;
	float hover[4];
	float colour[4];

	/* The background under the pointer. */
	sign = ink;
	if (over && button == BUTTON_CLOSE) {
		hover[0] = 0.91f;
		hover[1] = 0.30f;
		hover[2] = 0.28f;
		hover[3] = 0.95f * fade;
		glass_draw_solid(server, command, (float)(cx - BUTTON_WIDTH / 2), (float)(cy - BUTTON_HEIGHT / 2), (float)BUTTON_WIDTH, (float)BUTTON_HEIGHT, 8.0f, hover);
		sign = white;
	} else if (over) {
		hover[0] = 1.0f;
		hover[1] = 1.0f;
		hover[2] = 1.0f;
		hover[3] = 0.70f * fade;
		glass_draw_solid(server, command, (float)(cx - BUTTON_WIDTH / 2), (float)(cy - BUTTON_HEIGHT / 2), (float)BUTTON_WIDTH, (float)BUTTON_HEIGHT, 8.0f, hover);
	}

	/* The sign's color, faded. */
	memcpy(colour, sign, sizeof(colour));
	colour[3] *= fade;

	/* Minimize: a short line. */
	if (button == BUTTON_MINIMIZE) {
		glass_draw_solid(server, command, (float)(cx - 6), (float)cy - 0.75f, 12.0f, 1.5f, 0.75f, colour);
		return;
	}

	/* Maximize: a small rounded square; restore: a second one behind it. */
	if (button == BUTTON_MAXIMIZE) {
		glass_shape_init(&shape, (float)(cx - 5), (float)(cy - 5), 10.0f, 10.0f);
		if (restore) {
			shape.box[0] -= 2.0f;
			shape.box[1] += 2.0f;
			shape.box[2] = 9.0f;
			shape.box[3] = 9.0f;
		}

		/* The quad leaves room for the outline's edge. */
		shape.quad[0] -= 3.0f;
		shape.quad[1] -= 3.0f;
		shape.quad[2] += 6.0f;
		shape.quad[3] += 6.0f;
		shape.mode = MODE_RING;
		shape.radius = 2.5f;
		shape.soft = 1.4f;
		memcpy(shape.color, colour, sizeof(shape.color));
		glass_shape_draw(server, command, &shape);

		/* The square behind: its top and right edges show above and beside the front one. */
		if (restore) {
			glass_draw_solid(server, command, (float)(cx - 4), (float)(cy - 5), 9.0f, 1.4f, 0.7f, colour);
			glass_draw_solid(server, command, (float)(cx + 4) - 0.4f, (float)(cy - 5), 1.4f, 9.0f, 0.7f, colour);
		}

		/* Done. */
		return;
	}

	/* Close: the multiplication sign, centred. */
	glass_draw_glyph(server, command, SIZE_SIGN, GLASS_CLOSE_GLYPH, cx - glass_glyph_advance(server, SIZE_SIGN, GLASS_CLOSE_GLYPH) / 2, cy + 7, colour);
}

/*
 * Draws the system bar: a glass strip along the top (a little whiter while a
 * window is docked), the launcher, the docked window's title and
 * buttons, the desktops, and the status at the right.
 */
static void
draw_system_bar(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct shell_bar *bar)
{
	static const float dark[4] = { 0.12f, 0.16f, 0.24f, 1.0f };
	static const float edge[4] = { 1.0f, 1.0f, 1.0f, 0.55f };
	static const float line[4] = { 0.12f, 0.16f, 0.24f, 0.18f };
	struct zwl_menu_area area;
	struct glass_shape shape;
	struct zwl_object *docked;
	int32_t available;
	int32_t limit;
	int32_t end;
	float label[4];
	float progress;
	float home;
	int button;
	int over;

	/* The docked window, if one is on top and not moving (not while App Home shows). */
	docked = docked_window(server);
	home = zwl_home_progress(server);
	if (home > 0.0f)
		docked = NULL;

	/* The strip, with a light line under it; whiter while a window is docked, or would dock. */
	glass_shape_init(&shape, 0.0f, 0.0f, (float)server->width, (float)ZWL_GLASS_BAR);
	shape.mode = MODE_GLASS;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.55f;
	if (docked != NULL)
		shape.color[3] = 0.63f;
	if (server->drag != NULL && server->pointer_y < ZWL_GLASS_BAR)
		shape.color[3] = 0.75f;
	glass_shape_draw(server, command, &shape);
	glass_draw_solid(server, command, 0.0f, (float)(ZWL_GLASS_BAR - 1), (float)server->width, 1.0f, 0.0f, edge);

	/* The launcher: the Kei mark (ws035-p117) in the bar's deeper colours (ws035-p118). */
	glass_draw_mark(server, command, BAR_LAUNCHER_X, BAR_LAUNCHER_Y, BAR_LAUNCHER_SIZE, GLASS_MARK_BAR, 1.0f);

	/* In App Home the launcher is marked by a ring around the mark. */
	if (home > 0.0f) {
		glass_shape_init(&shape, (float)(BAR_LAUNCHER_X - 2), (float)(BAR_LAUNCHER_Y - 2), (float)(BAR_LAUNCHER_SIZE + 4), (float)(BAR_LAUNCHER_SIZE + 4));
		shape.mode = MODE_RING;
		shape.radius = 9.0f;
		shape.soft = 2.0f;
		shape.color[0] = 0.25f;
		shape.color[1] = 0.52f;
		shape.color[2] = 0.98f;
		shape.color[3] = home;
		glass_shape_draw(server, command, &shape);
	}

	/* The docked window: a line, its mark, title and menu, and its buttons with restore for maximize. */
	if (docked != NULL) {
		glass_draw_solid(server, command, (float)bar->menu_line, 9.0f, 1.0f, 16.0f, 0.0f, line);

		/* The title shares the room before the buttons with the window's menu. */
		available = bar->buttons[BUTTON_MINIMIZE] - 24 - bar->title_x - 30;
		limit = zwl_titlebar_title_limit(server, docked, available);
		draw_title(server, command, docked, bar->title_x, ZWL_GLASS_BAR / 2, limit, dark);

		/* The presentation after the title: its menu or its controls (titlebar-shell.c). */
		end = title_end(server, docked, limit);
		area.x = bar->title_x + 30 + end + 18;
		area.top = 0;
		area.right = bar->title_x + 30 + available;
		area.height = ZWL_GLASS_BAR;
		area.origin = 0;
		zwl_titlebar_draw(server, command, docked, 1, &area, dark, 1.0f);

		/* The buttons, the one under the pointer lit. */
		over = bar_button_at(bar, server->pointer_x, server->pointer_y);
		for (button = 0; button < BUTTON_COUNT; button++)
			draw_sign(server, command, button, bar->buttons[button], ZWL_GLASS_BAR / 2, 1, over == button, 1.0f, dark);
	}

	/* Wiseview's name where a docked title would be. */
	progress = wiseview_progress(server);
	if (progress > 0.0f) {
		memcpy(label, dark, sizeof(label));
		label[3] = progress;
		glass_draw_solid(server, command, (float)bar->menu_line, 9.0f, 1.0f, 16.0f, 0.0f, line);
		glass_draw_text(server, command, SIZE_TITLE, bar->title_x, 23, "Wiseview", 200, label);
	}

	/* The desktops, then the status. */
	draw_desktops(server, command, bar, line);
	draw_status(server, command, bar, dark);
}

/*
 * Draws the virtual desktops: a light pill with a small picture of the
 * wallpaper for each (the others paler, a dot under those with windows),
 * the one shown outlined, and lines on both sides.
 */
static void
draw_desktops(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct shell_bar *bar,
	const float *line)
{
	static const float pill[4] = { 1.0f, 1.0f, 1.0f, 0.45f };
	static const float current[4] = { 0.25f, 0.52f, 0.98f, 1.0f };
	struct glass_shape shape;
	float progress;
	unsigned windows;
	int32_t x;
	int desktop;

	/* The lines, and the pill. */
	glass_draw_solid(server, command, (float)bar->desktops_line, 9.0f, 1.0f, 16.0f, 0.0f, line);
	glass_draw_solid(server, command, (float)bar->status_line, 9.0f, 1.0f, 16.0f, 0.0f, line);
	glass_draw_solid(server, command, (float)bar->desktops_x, 4.0f, (float)bar->desktops_width, (float)(ZWL_GLASS_BAR - 8), 10.0f, pill);

	/* Wiseview brings the desktops forward with a blue edge. */
	progress = wiseview_progress(server);
	if (progress > 0.0f) {
		glass_shape_init(&shape, (float)bar->desktops_x, 4.0f, (float)bar->desktops_width, (float)(ZWL_GLASS_BAR - 8));
		shape.quad[0] -= 1.0f;
		shape.quad[1] -= 1.0f;
		shape.quad[2] += 2.0f;
		shape.quad[3] += 2.0f;
		shape.mode = MODE_RING;
		shape.radius = 10.0f;
		shape.soft = 1.5f;
		memcpy(shape.color, current, sizeof(shape.color));
		shape.opacity = progress * 0.6f;
		glass_shape_draw(server, command, &shape);
	}

	/* Where the pictures are, once. */
	if (!shell_desktops_logged) {
		shell_desktops_logged = 1U;
		printf("ZWL GLASS desktops x=%d step=%d width=%d\n", bar->desktops_x + 6, DESKTOP_WIDTH + DESKTOP_GAP, DESKTOP_WIDTH);
	}

	/* Each desktop's picture; the others are paler. */
	for (desktop = 0; desktop < DESKTOPS; desktop++) {
		x = bar->desktops_x + 6 + desktop * (DESKTOP_WIDTH + DESKTOP_GAP);
		glass_shape_init(&shape, (float)x, (float)((ZWL_GLASS_BAR - DESKTOP_HEIGHT) / 2), (float)DESKTOP_WIDTH, (float)DESKTOP_HEIGHT);
		shape.mode = MODE_IMAGE;
		shape.opaque = 1.0f;
		shape.radius = 4.0f;
		shape.set = glass_wallpaper_set(server);
		if (desktop != (int)server->desktop)
			shape.opacity = 0.45f;
		glass_shape_draw(server, command, &shape);

		/* A desktop with windows has a small dot under its picture. */
		windows = desktop_windows(server, (unsigned)desktop);
		if (windows != 0U)
			glass_draw_solid(server, command, (float)(x + DESKTOP_WIDTH / 2 - 2), (float)(ZWL_GLASS_BAR - 5), 4.0f, 3.0f, 1.5f, current);

		/* The current one is outlined in blue. */
		if (desktop == (int)server->desktop) {
			glass_shape_init(&shape, (float)(x - 2), (float)((ZWL_GLASS_BAR - DESKTOP_HEIGHT) / 2 - 2), (float)(DESKTOP_WIDTH + 4), (float)(DESKTOP_HEIGHT + 4));
			shape.quad[0] -= 1.0f;
			shape.quad[1] -= 1.0f;
			shape.quad[2] += 2.0f;
			shape.quad[3] += 2.0f;
			shape.mode = MODE_RING;
			shape.radius = 6.0f;
			shape.soft = 1.6f;
			memcpy(shape.color, current, sizeof(shape.color));
			glass_shape_draw(server, command, &shape);
		}
	}
}

/*
 * Draws the status at the right edge: the network (network.c), the battery
 * (a mock-up) and the date and time.
 */
static void
draw_status(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct shell_bar *bar,
	const float *ink)
{
	struct glass_shape shape;

	/* The date and time. */
	glass_draw_text(server, command, SIZE_BAR, bar->clock_x, 22, bar->clock, 400, ink);

	/* The battery: an outline, its charge and its terminal. */
	glass_shape_init(&shape, (float)bar->battery_x, 11.0f, 22.0f, 12.0f);
	shape.quad[0] -= 1.0f;
	shape.quad[1] -= 1.0f;
	shape.quad[2] += 2.0f;
	shape.quad[3] += 2.0f;
	shape.mode = MODE_RING;
	shape.radius = 3.5f;
	shape.soft = 1.3f;
	memcpy(shape.color, ink, sizeof(shape.color));
	glass_shape_draw(server, command, &shape);
	glass_draw_solid(server, command, (float)(bar->battery_x + 3), 14.0f, 14.0f, 6.0f, 1.5f, ink);
	glass_draw_solid(server, command, (float)(bar->battery_x + 23), 15.0f, 2.0f, 4.0f, 1.0f, ink);

	/* The network: Wi-Fi's bars or the wired tree, which opens its menu (network.c). */
	zwl_network_draw_icon(server, command, bar->signal_x, ink);

	/* The volume's speaker, which opens its popup (volume.c, ws100-p004). */
	zwl_volume_draw_icon(server, command, bar->volume_x, ink);
}

/*
 * Shows where a dragged window would dock while the pointer is in the
 * system bar: a pale rounded rectangle with a blue edge over the space the
 * docked body would take.
 */
static void
draw_dock_hint(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	static const float fill[4] = { 1.0f, 1.0f, 1.0f, 0.22f };
	static const float edge[4] = { 0.25f, 0.52f, 0.98f, 0.9f };
	struct glass_shape shape;
	struct shell_rect body;

	/* Only while a move is in the system bar. */
	if (server->drag == NULL || server->pointer_y >= ZWL_GLASS_BAR)
		return;

	/* The space, filled and outlined. */
	docked_rect(server, &body);
	glass_draw_solid(server, command, (float)(body.x + 6), (float)(body.y + 6), (float)(body.width - 12), (float)(body.height - 12), GLASS_RADIUS, fill);
	glass_shape_init(&shape, (float)(body.x + 6), (float)(body.y + 6), (float)(body.width - 12), (float)(body.height - 12));
	shape.quad[0] -= 1.0f;
	shape.quad[1] -= 1.0f;
	shape.quad[2] += 2.0f;
	shape.quad[3] += 2.0f;
	shape.mode = MODE_RING;
	shape.radius = GLASS_RADIUS;
	shape.soft = 2.0f;
	memcpy(shape.color, edge, sizeof(shape.color));
	glass_shape_draw(server, command, &shape);
}

/* How far the dock animation is, from 0 to 1, eased (1 without an animation). */
static float
animation_progress(
	struct zwl_server *server)
{
	uint64_t elapsed;
	float t;

	/* No animation is finished. */
	if (server->anim == NULL)
		return 1.0f;

	/* The time since it started, as a fraction of DOCK_MS. */
	elapsed = zwl_milliseconds() - server->anim_start_ms;
	if (elapsed >= DOCK_MS)
		return 1.0f;
	t = (float)elapsed / (float)DOCK_MS;

	/* Eased out: quick at first, slow at the end. */
	return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

/* The rectangle a fraction t of the way from one to another. */
static void
lerp_rect(
	const struct shell_rect *from,
	const struct shell_rect *to,
	float t,
	struct shell_rect *result)
{
	/* Each side moves in a straight line. */
	result->x = from->x + (int32_t)((float)(to->x - from->x) * t);
	result->y = from->y + (int32_t)((float)(to->y - from->y) * t);
	result->width = from->width + (int32_t)((float)(to->width - from->width) * t);
	result->height = from->height + (int32_t)((float)(to->height - from->height) * t);
}

/* Tells whether a window (its body and floating title bar) comes into a rectangle. */
static int
damage_near(
	struct zwl_server *server,
	struct zwl_object *surface,
	const struct shell_rect *near)
{
	struct shell_rect body;
	int32_t top;

	/* The body, with its floating title bar above it. */
	body_rect(server, surface, &body);
	top = body.y;
	if (!surface->maximized)
		top = body.y - ZWL_GLASS_TITLE - ZWL_GLASS_GAP;

	/* Apart across or down. */
	if (body.x + body.width <= near->x || near->x + near->width <= body.x)
		return 0;
	if (body.y + body.height <= near->y || near->y + near->height <= top)
		return 0;

	/* It comes in. */
	return 1;
}

/* Where a window's body is drawn: on its way while animated, the docked space, or its own place and size. */
static void
body_rect(
	struct zwl_server *server,
	const struct zwl_object *surface,
	struct shell_rect *body)
{
	struct shell_rect from;
	struct shell_rect to;
	float t;

	/* Between the two while animated. */
	if (server->anim == surface) {
		t = animation_progress(server);
		memcpy(&from, server->anim_from, sizeof(from));
		memcpy(&to, server->anim_to, sizeof(to));
		lerp_rect(&from, &to, t, body);
		return;
	}

	/* Docked and being pulled: on its way from the docked space to its own size under the pointer. */
	if (surface->maximized && surface == server->pull && server->pull_distance > 0) {
		pulled_rect(server, surface, body);
		return;
	}

	/* Docked. */
	if (surface->maximized) {
		docked_rect(server, body);
		return;
	}

	/* Its place, its image's size. */
	body->x = surface->x;
	body->y = surface->y;
	window_size(surface, &body->width, &body->height);
}

/* The space a docked body takes: the output under the system bar. */
static void
docked_rect(
	struct zwl_server *server,
	struct shell_rect *body)
{
	int32_t right;
	int32_t bottom;

	/* What the on-screen keyboard's panel takes now, as it slides (keyboard.c, ws102-p007). */
	zwl_keyboard_reserved_now(&right, &bottom);

	/* Edge to edge, from just under the bar to the bottom, less the keyboard's column or row. */
	body->x = 0;
	body->y = DOCK_TOP;
	body->width = (int32_t)server->width - right;
	body->height = (int32_t)server->height - DOCK_TOP - bottom;
}

/* The floating title bar of a body: as wide as it, a gap above it. */
static void
floating_title(
	const struct shell_rect *body,
	struct shell_rect *panel)
{
	/* Above the body. */
	panel->x = body->x;
	panel->y = body->y - ZWL_GLASS_GAP - ZWL_GLASS_TITLE;
	panel->width = body->width;
	panel->height = ZWL_GLASS_TITLE;
}

/*
 * The place in the system bar a docking title bar slides to: its mark lands
 * where the docked title's mark is drawn, and it ends at the docked buttons.
 */
static void
bar_title_slot(
	struct zwl_server *server,
	const struct shell_bar *bar,
	struct shell_rect *slot)
{
	/* The bar's height, from the mark's place to past the buttons. */
	(void)server;
	slot->x = bar->title_x - 14;
	slot->y = 0;
	slot->width = bar->buttons[BUTTON_CLOSE] + 26 - slot->x;
	slot->height = ZWL_GLASS_BAR;
}

/* The size of a window's image: its viewport's size, else its buffer's (viewport.c); 0 by 0 without an image. */
static void
window_size(
	const struct zwl_object *surface,
	int32_t *width,
	int32_t *height)
{
	uint32_t surface_width;
	uint32_t surface_height;

	/* No image, no size. */
	*width = 0;
	*height = 0;
	if (surface->current == NULL)
		return;

	/* The surface's size (ws035-p081: a viewport's destination or source). */
	zwl_surface_size(surface, &surface_width, &surface_height);
	*width = (int32_t)surface_width;
	*height = (int32_t)surface_height;
}

/* Tells whether a point is on a window's title bar, its body, its frame, or none of them (the gap is none). */
static enum shell_hit
window_hit(
	struct zwl_server *server,
	const struct zwl_object *surface,
	int32_t x,
	int32_t y)
{
	struct shell_rect body;
	struct zwl_object *parent;
	uint32_t edges;
	int32_t top;
	int decorated;

	/* A sheet has only its body: no title bar, no frame (ws090-p014). */
	body_rect(server, surface, &body);
	parent = zwl_sheet_parent(surface);
	if (parent != NULL) {
		if (x >= body.x && x < body.x + body.width && y >= body.y && y < body.y + body.height)
			return HIT_BODY;
		return HIT_NONE;
	}

	/* A CSD surface has no hidden titlebar or server resize band to consume input. */
	decorated = zwl_decoration_server(surface);
	if (!decorated) {
		if (x >= body.x && x < body.x + body.width && y >= body.y && y < body.y + body.height)
			return HIT_BODY;

		/* The client owns only its surface extent. */
		return HIT_NONE;
	}

	/* Within the window's columns: its body, or its floating title bar (a docked window's title is in the system bar). */
	if (x >= body.x && x < body.x + body.width) {
		if (y >= body.y && y < body.y + body.height)
			return HIT_BODY;
		top = body.y - ZWL_GLASS_GAP - ZWL_GLASS_TITLE;
		if (!surface->maximized && y >= top && y < top + ZWL_GLASS_TITLE)
			return HIT_TITLE;
	}

	/* The frame around a floating window resizes it. */
	edges = frame_edges(server, surface, x, y);
	if (edges != 0U)
		return HIT_FRAME;

	/* Neither (the gap between the title bar and the body is neither too). */
	return HIT_NONE;
}

/*
 * Tells which edges of a floating window's frame are at a point (ZWL_EDGE_*
 * bits of toplevel.h: one side, or a corner's two), or 0 off its frame.
 *
 * The frame is the band FRAME_BAND wide around the title bar and the body.
 * On the band above or below them, within FRAME_CORNER of the left or right
 * end, the side's edge joins the corner's; on the band left or right of
 * them, within FRAME_CORNER of the top or bottom, likewise.
 */
static uint32_t
frame_edges(
	struct zwl_server *server,
	const struct zwl_object *surface,
	int32_t x,
	int32_t y)
{
	struct shell_rect body;
	struct zwl_object *parent;
	uint32_t edges;
	int32_t left;
	int32_t right;
	int32_t top;
	int32_t bottom;
	int decorated;

	/* CSD clients request interactive resize from their own visible edges. */
	decorated = zwl_decoration_server(surface);
	if (!decorated)
		return 0U;

	/* Only a floating toplevel window has a frame: not a docked one, nor one moving, animated or without an image. */
	if (surface->maximized ||
	    surface->fullscreen ||
	    surface->role == NULL ||
	    surface->role->top == NULL ||
	    surface->current == NULL ||
	    server->anim == surface)
		return 0U;

	/* Nor a sheet (ws090-p014). */
	parent = zwl_sheet_parent(surface);
	if (parent != NULL)
		return 0U;

	/* The window's outline: from the title bar's top to the body's bottom. */
	body_rect(server, surface, &body);
	left = body.x;
	right = body.x + body.width;
	top = body.y - ZWL_GLASS_GAP - ZWL_GLASS_TITLE;
	bottom = body.y + body.height;

	/* Nothing beyond the band around the outline. */
	if (x < left - FRAME_BAND || x >= right + FRAME_BAND)
		return 0U;
	if (y < top - FRAME_BAND || y >= bottom + FRAME_BAND)
		return 0U;

	/* Nothing within the outline (the title bar, the body, the gap between them). */
	if (x >= left && x < right && y >= top && y < bottom)
		return 0U;

	/* The side the point is beyond, or the two sides at a corner. */
	edges = 0U;
	if (x < left)
		edges |= ZWL_EDGE_LEFT;
	if (x >= right)
		edges |= ZWL_EDGE_RIGHT;
	if (y < top)
		edges |= ZWL_EDGE_TOP;
	if (y >= bottom)
		edges |= ZWL_EDGE_BOTTOM;

	/* Above or below the outline, near its left or right end: that corner. */
	if ((edges & (ZWL_EDGE_TOP | ZWL_EDGE_BOTTOM)) != 0U) {
		if (x < left + FRAME_CORNER)
			edges |= ZWL_EDGE_LEFT;
		if (x >= right - FRAME_CORNER)
			edges |= ZWL_EDGE_RIGHT;
	}

	/* Left or right of the outline, near its top or bottom: that corner. */
	if ((edges & (ZWL_EDGE_LEFT | ZWL_EDGE_RIGHT)) != 0U) {
		if (y < top + FRAME_CORNER)
			edges |= ZWL_EDGE_TOP;
		if (y >= bottom - FRAME_CORNER)
			edges |= ZWL_EDGE_BOTTOM;
	}

	/* A window narrower or shorter than two corners keeps one side of each pair: the right, the bottom. */
	if ((edges & (ZWL_EDGE_LEFT | ZWL_EDGE_RIGHT)) == (ZWL_EDGE_LEFT | ZWL_EDGE_RIGHT))
		edges &= ~ZWL_EDGE_LEFT;
	if ((edges & (ZWL_EDGE_TOP | ZWL_EDGE_BOTTOM)) == (ZWL_EDGE_TOP | ZWL_EDGE_BOTTOM))
		edges &= ~ZWL_EDGE_TOP;

	/* Succeeded: the frame's edges at the point. */
	return edges;
}

/* Tells which frame edges of the top window at the pointer the pointer is on, or 0. */
static uint32_t
frame_under_pointer(
	struct zwl_server *server)
{
	struct zwl_object *surface;
	enum shell_hit hit;
	uint32_t edges;

	/*
	 * The screen's own strips take a press before any frame (zwl_glass_button):
	 * the system bar, the desktops' swipe at the left and right edges, and
	 * Wiseview's at the bottom.  A frame there shows no resize arrow.
	 */
	if (server->pointer_y < ZWL_GLASS_BAR)
		return 0U;
	if (server->pointer_y >= (int32_t)server->height - WISEVIEW_EDGE)
		return 0U;
	if (server->pointer_x < DESKTOP_EDGE)
		return 0U;
	if (server->pointer_x >= (int32_t)server->width - DESKTOP_EDGE)
		return 0U;

	/* The top window at the pointer, when it is its frame that is there. */
	surface = window_at(server, server->pointer_x, server->pointer_y, &hit);
	if (surface == NULL || hit != HIT_FRAME)
		return 0U;

	/* Succeeded: that frame's edges. */
	edges = frame_edges(server, surface, server->pointer_x, server->pointer_y);
	return edges;
}

/* Returns the floating title bar button at a point, or -1. */
static int
button_at(
	const struct zwl_object *surface,
	int32_t x,
	int32_t y)
{
	int32_t cx;
	int32_t cy;
	int button;

	/* A docked window's buttons are in the bar. */
	if (surface->maximized)
		return -1;

	/* Each button's box around its centre. */
	for (button = 0; button < BUTTON_COUNT; button++) {
		button_centre(surface, button, &cx, &cy);
		if (x >= cx - BUTTON_WIDTH / 2 && x < cx + BUTTON_WIDTH / 2 &&
		    y >= cy - BUTTON_HEIGHT / 2 && y < cy + BUTTON_HEIGHT / 2)
			return button;
	}

	/* None. */
	return -1;
}

/* The centre of a floating title bar button (counted from the right edge). */
static void
button_centre(
	const struct zwl_object *surface,
	int button,
	int32_t *x,
	int32_t *y)
{
	int32_t width;
	int32_t height;

	/* From the bar's right edge, in the middle of its height. */
	window_size(surface, &width, &height);
	*x = surface->x + width - 26 - button * BUTTON_SPACING;
	*y = surface->y - ZWL_GLASS_GAP - ZWL_GLASS_TITLE / 2;
}

/* Returns the docked window's button in the system bar at a point, or -1. */
static int
bar_button_at(
	const struct shell_bar *bar,
	int32_t x,
	int32_t y)
{
	int button;

	/* Each button's box in the bar. */
	if (y < 0 || y >= ZWL_GLASS_BAR)
		return -1;
	for (button = 0; button < BUTTON_COUNT; button++) {
		if (x >= bar->buttons[button] - BUTTON_WIDTH / 2 && x < bar->buttons[button] + BUTTON_WIDTH / 2)
			return button;
	}

	/* None. */
	return -1;
}

/* Finds the topmost window whose title bar or body is at a point. */
static struct zwl_object *
window_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y,
	enum shell_hit *hit)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *found;
	enum shell_hit place;

	/* The hit with the highest map order. */
	found = NULL;
	*hit = HIT_NONE;
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			/* Only mapped windows of the desktop shown. */
			if (surface->kind != ZWL_SURFACE ||
			    surface->dead ||
			    !surface->mapped ||
			    surface->role == NULL ||
			    surface->cursor_role ||
			    surface->desktop != server->desktop ||
			    surface->minimized)
				continue;

			/* Above what was found so far. */
			place = window_hit(server, surface, x, y);
			if (place == HIT_NONE)
				continue;
			if (found != NULL && surface->map_order < found->map_order)
				continue;
			found = surface;
			*hit = place;
		}
	}

	/* Succeeded: the window, or NULL. */
	return found;
}

/* The docked window whose title the system bar shows: the top window, docked and not moving. */
static struct zwl_object *
docked_window(
	struct zwl_server *server)
{
	struct zwl_object *parent;
	struct zwl_object *top;
	int decorated;

	/* None while Wiseview shows. */
	if (server->wiseview_gesture || server->wiseview > 0.0f || server->wiseview_moving)
		return NULL;

	/* The top window (a sheet's parent for a sheet, ws090-p014). */
	top = zwl_top_window(server);
	parent = zwl_sheet_parent(top);
	if (parent != NULL)
		top = parent;
	if (top == NULL || !top->maximized || server->anim == top)
		return NULL;
	if (server->pull == top && server->pull_distance > 0)
		return NULL;

	/* CSD windows retain their own controls when maximized; the system bar adds none. */
	decorated = zwl_decoration_server(top);
	if (!decorated)
		return NULL;

	/* Succeeded: this maximized SSD window supplies the system bar's titlebar. */
	return top;
}

/* Brings a window to the top and gives it the focus. */
static void
window_raise(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	struct zwl_object *top;
	struct zwl_object *parent;
	struct zwl_object *sheet;

	/* A sheet comes with its parent, and a parent with its sheet, which has the focus (ws090-p014). */
	parent = zwl_sheet_parent(surface);
	if (parent != NULL) {
		sheet = surface;
		surface = parent;
	} else {
		sheet = zwl_sheet_of(surface);
	}

	/* Already on top. */
	top = zwl_top_window(server);
	if (surface == top || (sheet != NULL && sheet == top))
		return;

	/* The highest map order (the sheet's above its parent's), and the focus follows. */
	server->map_order++;
	surface->map_order = server->map_order;
	server->front_surface = surface;
	if (sheet != NULL) {
		server->map_order++;
		sheet->map_order = server->map_order;
		server->front_surface = sheet;
	}

	/* The keyboard goes to the front window. */
	zwl_seat_focus(server);
	server->dirty = 1;
}

/*
 * Keeps each sheet where its parent is (ws090-p014): its top at the bottom
 * of the parent's title bar (sliding down out of it for SHEET_MS after it
 * began to show), in the middle of the parent's width, on the parent's
 * desktop and hidden with it.
 */
static void
sheet_place(
	struct zwl_server *server)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *parent;
	uint64_t now;
	int32_t width;
	int32_t height;
	int32_t x;
	int32_t y;
	int32_t top;
	int moved;
	float t;

	/* Every window that is a sheet now. */
	now = zwl_milliseconds();
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			if (surface->kind != ZWL_SURFACE)
				continue;
			parent = zwl_sheet_parent(surface);
			if (parent == NULL)
				continue;

			/* Its time from when it began to show, eased out; one wider than its parent is asked to be narrower. */
			window_size(surface, &width, &height);
			if (surface->sheet_ms == 0U) {
				surface->sheet_ms = now;
				sheet_narrow(server, surface, parent, width, height);
				printf("ZWL GLASS sheet surface=%u parent=%u width=%d height=%d\n", surface->id, parent->id, width, height);
			}

			/* How far it has slid out, eased out, and frames asked for until it has. */
			t = (float)(now - surface->sheet_ms) / (float)SHEET_MS;
			if (t < 1.0f)
				server->dirty = 1;
			if (t > 1.0f)
				t = 1.0f;
			t = 1.0f - (1.0f - t) * (1.0f - t);

			/* Its place: under the parent's title bar, risen by what has not slid out yet. */
			sheet_anchor(server, parent, width, &x, &top);
			y = top - (int32_t)((1.0f - t) * (float)height);
			moved = surface->x != x || surface->y != y;
			if (moved)
				server->dirty = 1;
			surface->x = x;
			surface->y = y;

			/* Its place once it has slid out, whenever it changes (the tests read it). */
			if (moved && t >= 1.0f)
				printf("ZWL GLASS sheet at x=%d y=%d parent=%u\n", x, y, parent->id);

			/* The parent's desktop, shown or hidden with it. */
			surface->desktop = parent->desktop;
			surface->minimized = parent->minimized;
		}
	}
}

/* The window a surface is shown with: a sheet's parent, or the surface itself (ws090-p014). */
static struct zwl_object *
sheet_owner(
	struct zwl_object *surface)
{
	struct zwl_object *parent;

	/* A sheet is its parent's. */
	parent = zwl_sheet_parent(surface);
	if (parent != NULL)
		return parent;
	return surface;
}

/*
 * Asks a sheet as wide as its parent or wider to be SHEET_MARGIN narrower
 * on each side than the parent's body, when that is not too narrow for it
 * (its smallest width, or SHEET_NARROWEST).
 */
static void
sheet_narrow(
	struct zwl_server *server,
	struct zwl_object *surface,
	struct zwl_object *parent,
	int32_t width,
	int32_t height)
{
	struct shell_rect body;
	int32_t wanted;

	/* The width the parent leaves it. */
	body_rect(server, parent, &body);
	wanted = body.width - 2 * SHEET_MARGIN;
	if (width <= wanted || wanted < SHEET_NARROWEST || wanted < surface->min_width)
		return;

	/* Its new size, told to it. */
	surface->window_width = (uint32_t)wanted;
	surface->window_height = (uint32_t)height;
	window_configure(surface);
	printf("ZWL GLASS sheet narrower surface=%u width=%d\n", surface->id, wanted);
}

/*
 * The place of a sheet of a width under its parent: the left of its middle
 * over the parent's body, and its top at the bottom of the parent's title
 * bar (the top of the body of a docked or fullscreen parent).
 */
static void
sheet_anchor(
	struct zwl_server *server,
	struct zwl_object *parent,
	int32_t width,
	int32_t *x,
	int32_t *top)
{
	struct shell_rect body;

	/* The parent's body where it is now. */
	body_rect(server, parent, &body);
	*x = body.x + (body.width - width) / 2;

	/* A floating parent's title bar ends a gap above its body; a docked or fullscreen one has none there. */
	*top = body.y - ZWL_GLASS_GAP;
	if (parent->maximized || parent->fullscreen)
		*top = body.y;
}

/*
 * Draws a sheet (ws090-p014): its body only, cut at the bottom of its
 * parent's title bar (its shadow does not fall on the title bar, and it
 * slides down out of it), with a hairline along that seam.
 */
static void
draw_sheet(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	struct zwl_object *parent,
	unsigned focused)
{
	static const float seam[4] = { 0.12f, 0.16f, 0.24f, 0.14f };
	struct shell_rect body;
	VkRect2D saved;
	VkRect2D cut;
	int32_t x;
	int32_t top;
	int32_t bottom;

	/* The body where it is, and where the parent's title bar ends. */
	body_rect(server, surface, &body);
	sheet_anchor(server, parent, body.width, &x, &top);

	/* Only below the title bar (its shadow and, while it slides, itself): the frame's scissor cut there. */
	saved = server->compose->scissor_now;
	cut = saved;
	bottom = saved.offset.y + (int32_t)saved.extent.height;
	if (cut.offset.y < top)
		cut.offset.y = top;
	if (bottom < cut.offset.y)
		bottom = cut.offset.y;
	cut.extent.height = (uint32_t)(bottom - cut.offset.y);
	server->compose->scissor_now = cut;
	vkCmdSetScissor(command, 0U, 1U, &cut);

	/* The body, and a hairline along the seam with the title bar. */
	draw_body(server, command, surface, &body, 0, focused);
	glass_draw_solid(server, command, (float)body.x, (float)top, (float)body.width, 1.0f, 0.0f, seam);

	/* The frame's scissor back. */
	server->compose->scissor_now = saved;
	vkCmdSetScissor(command, 0U, 1U, &saved);
}

/*
 * Docks a window to the system bar: its body takes the space under the bar
 * and it is told that size; it comes back to (restore_x, restore_y) at its
 * present size.  The change is animated.
 */
static void
window_dock(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t restore_x,
	int32_t restore_y,
	const char *via)
{
	struct shell_rect from;
	struct shell_rect to;
	struct shell_bar bar;
	uint32_t geometry_width;
	uint32_t geometry_height;

	/* A docked window stays docked. */
	if (surface->maximized)
		return;

	/* The place and size to come back to, and where the body is now. */
	surface->restore_x = restore_x;
	surface->restore_y = restore_y;
	zwl_decoration_geometry(surface, &geometry_width, &geometry_height);
	surface->restore_width = geometry_width;
	surface->restore_height = geometry_height;
	body_rect(server, surface, &from);

	/* Docked. */
	surface->maximized = 1;
	docked_rect(server, &to);
	surface->x = to.x;
	surface->y = to.y;
	surface->window_width = (uint32_t)to.width;
	surface->window_height = (uint32_t)to.height;

	/* The animation from the floating body to the docked space. */
	memcpy(server->anim_from, &from, sizeof(server->anim_from));
	memcpy(server->anim_to, &to, sizeof(server->anim_to));
	server->anim = surface;
	server->anim_docking = 1;
	server->anim_start_ms = zwl_milliseconds();
	server->dirty = 1;

	/* The client draws the new size; the log gives where the bar's buttons are (close, restore, minimize) and the docked body. */
	bar_layout(server, &bar);
	printf("ZWL GLASS dock surface=%u via=%s buttons=%d,%d,%d title=%d x=%d y=%d w=%d h=%d\n", surface->id, via,
	       bar.buttons[BUTTON_CLOSE], bar.buttons[BUTTON_MAXIMIZE], bar.buttons[BUTTON_MINIMIZE], bar.title_x,
	       (int)to.x, (int)to.y, (int)to.width, (int)to.height);
	window_configure(surface);
}

/*
 * Brings a docked window back: its body at (x, y) at the size it had, and
 * it is told that size.  The change is animated.
 */
static void
window_undock(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t x,
	int32_t y,
	const char *via)
{
	struct shell_rect from;
	struct shell_rect to;

	/* Only a docked window comes back. */
	if (!surface->maximized)
		return;

	/* The place asked for, inside the space: its title bar never under the system bar (ws035-p138). */
	zwl_glass_fit(server, (int32_t)surface->restore_width, (int32_t)surface->restore_height, &x, &y);

	/* From the docked space to that place, at the size it had. */
	docked_rect(server, &from);
	surface->maximized = 0;
	surface->x = x;
	surface->y = y;
	surface->window_width = surface->restore_width;
	surface->window_height = surface->restore_height;
	to.x = x;
	to.y = y;
	to.width = (int32_t)surface->restore_width;
	to.height = (int32_t)surface->restore_height;

	/* The animation back. */
	memcpy(server->anim_from, &from, sizeof(server->anim_from));
	memcpy(server->anim_to, &to, sizeof(server->anim_to));
	server->anim = surface;
	server->anim_docking = 0;
	server->anim_start_ms = zwl_milliseconds();
	server->dirty = 1;

	/* The client draws the size it had. */
	printf("ZWL GLASS undock surface=%u via=%s x=%d y=%d\n", surface->id, via, x, y);
	window_configure(surface);
}

/* Tells a window its new size. */
static void
window_configure(
	struct zwl_object *surface)
{
	int error;

	/* A failure is reported; the window keeps drawing its old size. */
	error = zwl_window_send_configure(surface);
	if (error != 0)
		printf("ZWL GLASS configure errno=%d\n", error);
}

/*
 * Tells whether this press on a window's title is the second of a double
 * click (within DOUBLE_CLICK_MS of one on the same window); otherwise it
 * remembers this press as a first one.
 */
static unsigned
double_click(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	uint64_t now;

	/* The second press of a pair. */
	now = zwl_milliseconds();
	if (server->click_surface == surface && now - server->click_ms < DOUBLE_CLICK_MS) {
		server->click_surface = NULL;
		server->click_count = 0;
		return 1;
	}

	/* A first press. */
	server->click_surface = surface;
	server->click_ms = now;
	server->click_count = 1;
	return 0;
}

/*
 * Counts this press on a window's floating title bar into the run of quick
 * presses on it: 1 for a press that starts a run, 2 for the second of a
 * double click, 3 for the third of a triple click.  A press more than
 * DOUBLE_CLICK_MS after the one before, or on another window, starts a new
 * run.
 */
static unsigned
title_clicks(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	uint64_t now;

	/* A quick press after the one before on the same window goes on the run. */
	now = zwl_milliseconds();
	if (server->click_surface == surface && now - server->click_ms < DOUBLE_CLICK_MS) {
		server->click_count++;
	} else {
		server->click_surface = surface;
		server->click_count = 1;
	}

	/*
	 * The time of this press is where the next one is measured from, so
	 * the third press has DOUBLE_CLICK_MS after the second.
	 */
	server->click_ms = now;

	/* Succeeded: how many presses the run has. */
	return server->click_count;
}

/*
 * Docks the window whose double click has waited out the time a third
 * press had (DOUBLE_CLICK_MS after the second).
 */
static void
dock_when_due(
	struct zwl_server *server)
{
	struct zwl_object *surface;
	uint64_t now;

	/* Nothing waits to dock. */
	surface = server->dock_waiting;
	if (surface == NULL)
		return;

	/* A third press may still come. */
	now = zwl_milliseconds();
	if (now < server->dock_due_ms)
		return;
	server->dock_waiting = NULL;

	/* A window that went away, docked, was hidden or left the desktop meanwhile stays as it is. */
	if (surface->dead ||
	    !surface->mapped ||
	    surface->maximized ||
	    surface->minimized ||
	    surface->desktop != server->desktop) {
		printf("ZWL GLASS dock dropped surface=%u\n", surface->id);
		return;
	}

	/* The double click docks it; the log says how long after the second press. */
	printf("ZWL GLASS double-click surface=%u waited_ms=%llu\n", surface->id, (unsigned long long)(now - (server->dock_due_ms - DOUBLE_CLICK_MS)));
	window_dock(server, surface, surface->x, surface->y, "double-click");
}

/*
 * Sends a window to the back of the stacking order and gives the focus to
 * the window now on top.  Every other window keeps its place relative to
 * the others.
 */
static void
window_lower(
	struct zwl_server *server,
	struct zwl_object *surface,
	const char *via)
{
	struct zwl_client *client;
	struct zwl_object *other;
	struct zwl_object *top;
	uint64_t lowest;
	unsigned found;
	uint64_t next_client;
	uint32_t next;
	uint64_t focus_client;
	uint32_t focus;
	struct zwl_object *parent;
	struct zwl_object *sheet;

	/*
	 * A sheet goes with its parent, and a parent with its sheet (ws090-p014):
	 * the sheet goes under every window first, then the parent under it.
	 */
	if (!sheet_lowering) {
		parent = zwl_sheet_parent(surface);
		if (parent != NULL)
			surface = parent;
		sheet = zwl_sheet_of(surface);
		if (sheet != NULL) {
			sheet_lowering = 1;
			window_lower(server, sheet, via);
			sheet_lowering = 0;
		}
	}

	/* The lowest place any other mapped surface holds. */
	found = 0;
	lowest = 0;
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (other = client->objects; other != NULL; other = other->next) {
			/* Only the other live mapped surfaces. */
			if (other == surface ||
			    other->kind != ZWL_SURFACE ||
			    other->dead ||
			    !other->mapped)
				continue;

			/* Below what was found so far. */
			if (found && other->map_order >= lowest)
				continue;
			found = 1;
			lowest = other->map_order;
		}
	}

	/* A window alone stays where it is. */
	if (!found) {
		printf("ZWL GLASS lower client=%llu surface=%u via=%s next=none\n", (unsigned long long)surface->client->number, surface->id, via);
		return;
	}

	/*
	 * With no place free under the lowest (map orders start at 1), every
	 * other mapped surface moves up one place, which keeps their order.
	 */
	if (lowest <= 1U) {
		for (client = server->clients; client != NULL; client = client->next) {
			for (other = client->objects; other != NULL; other = other->next) {
				/* Only the other mapped surfaces hold a place. */
				if (other == surface ||
				    other->kind != ZWL_SURFACE ||
				    !other->mapped)
					continue;
				other->map_order++;
			}
		}

		/* The next window mapped or raised still comes above them all. */
		server->map_order++;
		lowest++;
	}

	/* Under every other window. */
	surface->map_order = lowest - 1U;

	/* The window now on top takes the focus. */
	top = zwl_top_window(server);
	server->front_surface = top;
	zwl_seat_focus(server);
	server->dirty = 1;

	/*
	 * The log names the window that came forward and the surface that has
	 * the keyboard now, each as client:surface (surface numbers are each
	 * client's own).
	 */
	next_client = 0;
	next = 0;
	if (top != NULL) {
		next_client = top->client->number;
		next = top->id;
	}

	/* The keyboard's surface, when there is one. */
	focus_client = 0;
	focus = 0;
	if (server->focus != NULL) {
		focus_client = server->focus->client->number;
		focus = server->focus->id;
	}

	/* Written as one line for the tests. */
	printf("ZWL GLASS lower client=%llu surface=%u via=%s next=%llu:%u focus=%llu:%u\n",
	       (unsigned long long)surface->client->number,
	       surface->id,
	       via,
	       (unsigned long long)next_client,
	       next,
	       (unsigned long long)focus_client,
	       focus);
}

/*
 * Handles a press in the system bar: on the docked window's buttons their
 * action, on its title a double click (back) or the start of a pull.  The
 * launcher, the desktops and the status do nothing yet.  The press is
 * always zdesktop's.
 */
static int
bar_press(
	struct zwl_server *server)
{
	struct zwl_object *surface;
	struct shell_bar bar;
	unsigned second;
	int32_t picture;
	int pressed;

	/* A desktop's picture switches to it. */
	bar_layout(server, &bar);
	picture = server->pointer_x - (bar.desktops_x + 6);
	if (picture >= 0 && picture < DESKTOPS * (DESKTOP_WIDTH + DESKTOP_GAP)) {
		desktop_turn(server, picture / (DESKTOP_WIDTH + DESKTOP_GAP), "bar");
		return 1;
	}

	/* Otherwise only a docked window acts. */
	surface = docked_window(server);
	if (surface == NULL)
		return 1;

	/* Its buttons. */
	pressed = bar_button_at(&bar, server->pointer_x, server->pointer_y);
	if (pressed == BUTTON_CLOSE) {
		(void)zwl_emit(surface->client, surface->role->top->id, 1U, NULL, 0U);
		printf("ZWL GLASS close surface=%u\n", surface->id);
		return 1;
	}

	/* Restore brings it back where it was. */
	if (pressed == BUTTON_MAXIMIZE) {
		window_undock(server, surface, surface->restore_x, surface->restore_y, "button");
		return 1;
	}

	/* Minimize hides it. */
	if (pressed == BUTTON_MINIMIZE) {
		window_minimize(server, surface);
		return 1;
	}

	/* Its title: between the line after the launcher and the buttons. */
	if (server->pointer_x < bar.menu_line || server->pointer_x >= bar.buttons[BUTTON_MINIMIZE] - BUTTON_WIDTH / 2)
		return 1;

	/* A double click brings it back where it was. */
	second = double_click(server, surface);
	if (second) {
		window_undock(server, surface, surface->restore_x, surface->restore_y, "double-click");
		return 1;
	}

	/* A single press may become a pull. */
	server->pull = surface;
	server->pull_start_y = server->pointer_y;
	server->pull_distance = 0;
	return 1;
}

/*
 * Finds the fullscreen window that keeps the system bar away (ws035-p119,
 * the 2026-09-28 user decision): of the windows on the desktop shown that
 * cover the output's top, fullscreen or docked, the highest, when it is
 * fullscreen.  A window opened or raised over it floats above it and the
 * bar stays away, so that the fullscreen window (Notes) is kept whole; the
 * bar is reached through App Home and Wiseview, which show it.  A docked
 * window above it has the bar, which holds its title.  Returns NULL when
 * the bar is drawn.
 */
static struct zwl_object *
bar_cover(
	struct zwl_server *server)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *cover;
	float progress;

	/* App Home shows the bar, opening, open or closing. */
	progress = zwl_home_progress(server);
	if (progress > 0.0f || server->home_to > 0.0f)
		return NULL;

	/* So does Wiseview. */
	progress = wiseview_progress(server);
	if (progress > 0.0f || server->wiseview_moving)
		return NULL;

	/* The highest fullscreen or docked window of the desktop shown. */
	cover = NULL;
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			/* Only mapped windows of the desktop shown. */
			if (surface->kind != ZWL_SURFACE ||
			    surface->dead ||
			    !surface->mapped ||
			    surface->role == NULL ||
			    surface->cursor_role ||
			    surface->desktop != server->desktop ||
			    surface->minimized)
				continue;

			/* A floating window leaves the top of the output to what is under it. */
			if (!surface->fullscreen && !surface->maximized)
				continue;

			/* Above what was found so far. */
			if (cover != NULL && surface->map_order < cover->map_order)
				continue;
			cover = surface;
		}
	}

	/* No such window, or a docked one on top: the bar is drawn. */
	if (cover == NULL || !cover->fullscreen)
		return NULL;

	/* Succeeded: the fullscreen window the bar keeps away from. */
	return cover;
}

/* Logs the system bar leaving or coming back for a fullscreen window, once per change (for the tests). */
static void
bar_cover_log(
	struct zwl_server *server,
	const struct zwl_object *cover)
{
	/* The bar went away. */
	if (cover != NULL && !server->bar_hidden) {
		server->bar_hidden = 1U;
		printf("ZWL GLASS bar hidden fullscreen=%u\n", cover->id);
		return;
	}

	/* The bar came back. */
	if (cover == NULL && server->bar_hidden) {
		server->bar_hidden = 0U;
		printf("ZWL GLASS bar shown\n");
	}
}

/*
 * Tells whether a button is App Home's while the system bar is kept away
 * (ws035-p119): the rest of a press Home started, or a left press in the
 * top-left corner, as over a fullscreen window.
 */
static int
home_without_bar(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	/* A press Home follows goes on being Home's. */
	if (server->home_press || server->home_page_press || server->home_bottom_press)
		return 1;

	/* Only a left press starts one. */
	if (state == 0 || button != ZWL_BUTTON_LEFT)
		return 0;

	/* In the corner, where Home's gesture starts. */
	if (server->pointer_x < HOME_EDGE_CORNER && server->pointer_y < HOME_EDGE_CORNER)
		return 1;

	/* Anything else is the windows'. */
	return 0;
}

/*
 * How far Wiseview is open, from 0 to 1: following the gesture, on its way
 * to where it settles (eased), or settled.
 */
static float
wiseview_progress(
	struct zwl_server *server)
{
	uint64_t elapsed;
	float t;
	float value;

	/* The gesture: the distance moved up from where it started. */
	if (server->wiseview_gesture) {
		value = (float)(server->wiseview_start_y - server->pointer_y) / WISEVIEW_DISTANCE;
		if (value < 0.0f)
			value = 0.0f;
		if (value > 1.0f)
			value = 1.0f;
		return value;
	}

	/* Settled. */
	if (!server->wiseview_moving)
		return server->wiseview;

	/* Settling, eased out. */
	elapsed = zwl_milliseconds() - server->wiseview_start_ms;
	t = 1.0f;
	if (elapsed < WISEVIEW_MS)
		t = (float)elapsed / (float)WISEVIEW_MS;
	t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	return server->wiseview_from + (server->wiseview_to - server->wiseview_from) * t;
}

/* Starts the swipe up from the bottom edge that opens Wiseview, for a left press in that edge.  Returns 1 when it started. */
static int
wiseview_edge_press(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	/* Only a left press. */
	if (state == 0 || button != ZWL_BUTTON_LEFT)
		return 0;

	/* Only in the bottom edge: a stroke that starts above it is not the gesture. */
	if (server->pointer_y < (int32_t)server->height - WISEVIEW_EDGE)
		return 0;

	/* The gesture starts; the window on top is the current tile. */
	server->wiseview_gesture = 1;
	server->wiseview_start_y = server->pointer_y;
	server->wiseview_current = sheet_owner(zwl_top_window(server));
	server->dirty = 1;

	/* Succeeded: the press is the gesture's. */
	return 1;
}

/* Starts Wiseview settling from one value to another (0 closed, 1 open). */
static void
wiseview_settle(
	struct zwl_server *server,
	float from,
	float to)
{
	/* The animation, drawn every frame by zwl_glass_tick. */
	server->wiseview_from = from;
	server->wiseview_to = to;
	server->wiseview_start_ms = zwl_milliseconds();
	server->wiseview_moving = 1;
	server->wiseview = from;
	server->dirty = 1;
}

/* Tells whether Wiseview is open or on its way open (while it closes, keys go to the windows again). */
static int
wiseview_showing(
	struct zwl_server *server)
{
	/* The gesture from the bottom edge holds it open. */
	if (server->wiseview_gesture)
		return 1;

	/* On its way, it shows only when it is going open. */
	if (server->wiseview_moving) {
		if (server->wiseview_to > 0.0f)
			return 1;
		return 0;
	}

	/* Settled open. */
	if (server->wiseview > 0.0f)
		return 1;

	/* Settled closed. */
	return 0;
}

/* Opens Wiseview from the keyboard (Super+Tab), with the window on top as the current tile. */
static void
wiseview_open_key(
	struct zwl_server *server)
{
	/* The window on top is the one Enter comes back to (a sheet's parent for a sheet). */
	server->wiseview_current = sheet_owner(zwl_top_window(server));

	/* Wiseview opens as it does at the end of the gesture. */
	printf("ZWL WISEVIEW opening key at_ms=%llu\n", (unsigned long long)zwl_milliseconds());
	zwl_transition_request(server, "wiseview-open");
	wiseview_settle(server, 0.0f, 1.0f);
}

/*
 * Carries out a key while Wiseview shows: Tab (Shift+Tab back), the arrows
 * move the current tile, Enter and Space choose it, Esc and Super+Tab close
 * Wiseview.  A release and any other key do nothing.
 */
static void
wiseview_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	struct zwl_object *windows[WISEVIEW_WINDOWS];
	struct zwl_object *surface;
	unsigned count;
	unsigned index;
	int position;
	int step;

	/* Only a press acts. */
	if (state == 0U)
		return;

	/* Esc closes Wiseview where it is. */
	if (key == KEY_ESC) {
		wiseview_close_key(server);
		return;
	}

	/* So does Super+Tab again. */
	if (key == KEY_TAB && (server->modifiers & MODIFIER_SUPER) != 0U) {
		wiseview_close_key(server);
		return;
	}

	/* Enter and Space choose the current tile. */
	if (key == KEY_ENTER || key == KEY_SPACE) {
		surface = server->wiseview_current;

		/* Without a current window there is nothing to come back to: Wiseview only closes. */
		if (surface == NULL || surface->dead || !surface->mapped) {
			wiseview_close_key(server);
			return;
		}

		/* A minimized window comes back; it comes to the top and Wiseview closes. */
		surface->minimized = 0;
		window_raise(server, surface);
		printf("ZWL WISEVIEW select surface=%u via=key\n", surface->id);
		wiseview_settle(server, wiseview_progress(server), 0.0f);
		return;
	}

	/* The direction: on for Tab, Right and Down (back for Shift+Tab), back for Left and Up. */
	step = 0;
	if (key == KEY_TAB) {
		step = 1;
		if ((server->modifiers & MODIFIER_SHIFT) != 0U)
			step = -1;
	} else if (key == KEY_RIGHT || key == KEY_DOWN) {
		step = 1;
	} else if (key == KEY_LEFT || key == KEY_UP) {
		step = -1;
	}

	/* Any other key does nothing. */
	if (step == 0)
		return;

	/* The tiles, in the order Wiseview lays them out; none, nothing to move. */
	count = wiseview_windows(server, windows, WISEVIEW_WINDOWS);
	if (count == 0U)
		return;

	/* The current tile's place (count when the current window is not among them). */
	for (index = 0; index < count; index++) {
		/* The current window's tile. */
		if (windows[index] == server->wiseview_current)
			break;
	}

	/* The next tile, round the ends; without a current tile, the first. */
	position = 0;
	if (index < count)
		position = (int)index + step;
	if (position < 0)
		position = (int)count - 1;
	if (position >= (int)count)
		position = 0;

	/* It becomes the current tile, which Wiseview draws marked. */
	server->wiseview_current = windows[position];
	server->dirty = 1;
	printf("ZWL WISEVIEW current surface=%u\n", windows[position]->id);
}

/* Closes Wiseview from the keyboard, from wherever it is on its way. */
static void
wiseview_close_key(
	struct zwl_server *server)
{
	float progress;

	/* The gesture, if one was under way, ends with it. */
	progress = wiseview_progress(server);
	server->wiseview_gesture = 0;

	/* Wiseview settles closed. */
	printf("ZWL WISEVIEW close key at_ms=%llu\n", (unsigned long long)zwl_milliseconds());
	zwl_transition_request(server, "wiseview-close");
	wiseview_settle(server, progress, 0.0f);
}

/*
 * Collects the windows Wiseview shows, the most recently raised first:
 * mapped toplevel windows with an image.
 */
static unsigned
wiseview_windows(
	struct zwl_server *server,
	struct zwl_object **windows,
	unsigned capacity)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *parent;
	unsigned count;
	unsigned index;
	unsigned at;

	/* Every window, by falling map order. */
	count = 0;
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			/* Only a window with an image, of the desktop shown. */
			if (surface->kind != ZWL_SURFACE ||
			    surface->dead ||
			    !surface->mapped ||
			    surface->role == NULL ||
			    surface->cursor_role ||
			    surface->current == NULL ||
			    surface->desktop != server->desktop)
				continue;

			/* A sheet is not a window of its own there: it comes back with its parent (ws090-p014). */
			parent = zwl_sheet_parent(surface);
			if (parent != NULL)
				continue;

			/* Inserted after those raised later. */
			if (count == capacity)
				break;
			at = count;
			while (at > 0 && windows[at - 1]->map_order < surface->map_order)
				at--;
			for (index = count; index > at; index--)
				windows[index] = windows[index - 1];
			windows[at] = surface;
			count++;
		}
	}

	/* Succeeded. */
	return count;
}

/*
 * Lays the windows out as Wiseview's grid: 1 column for one window, 2 for up
 * to 4, 3 for up to 9, 4 beyond; each tile keeps its window's shape, fills
 * its cell at most (and at most 42 % of the output's width and 36 % of its
 * height), and sits in the middle of its cell; a short last row is centred.
 * The window that was on top is 4 % larger.
 */
static void
wiseview_layout(
	struct zwl_server *server,
	struct zwl_object **windows,
	unsigned count,
	struct shell_rect *tiles)
{
	int32_t columns;
	int32_t rows;
	int32_t width;
	int32_t height;
	int32_t cell_width;
	int32_t cell_height;
	int32_t row;
	int32_t column;
	int32_t in_row;
	int32_t row_x;
	float scale;
	float limit;
	unsigned index;

	/* The columns and rows. */
	if (count == 0)
		return;
	columns = 4;
	if (count <= 9U)
		columns = 3;
	if (count <= 4U)
		columns = 2;
	if (count == 1U)
		columns = 1;
	rows = ((int32_t)count + columns - 1) / columns;

	/* The cells, with room for the label under each tile. */
	cell_width = ((int32_t)server->width - 2 * WISEVIEW_SIDE - (columns - 1) * WISEVIEW_GUTTER) / columns;
	cell_height = ((int32_t)server->height - WISEVIEW_TOP - WISEVIEW_BOTTOM - (rows - 1) * WISEVIEW_GUTTER) / rows - WISEVIEW_LABEL;

	/* Each tile. */
	for (index = 0; index < count; index++) {
		window_size(windows[index], &width, &height);
		if (width <= 0 || height <= 0) {
			width = 1;
			height = 1;
		}

		/* The largest scale that fits the cell and the limits. */
		scale = (float)cell_width / (float)width;
		limit = (float)cell_height / (float)height;
		if (limit < scale)
			scale = limit;
		limit = 0.42f * (float)server->width / (float)width;
		if (limit < scale)
			scale = limit;
		limit = 0.36f * (float)server->height / (float)height;
		if (limit < scale)
			scale = limit;
		if (windows[index] == server->wiseview_current)
			scale *= 1.04f;

		/* Its cell; a short last row is centred. */
		row = (int32_t)index / columns;
		column = (int32_t)index % columns;
		in_row = columns;
		if (row == rows - 1)
			in_row = (int32_t)count - row * columns;
		row_x = ((int32_t)server->width - in_row * cell_width - (in_row - 1) * WISEVIEW_GUTTER) / 2;

		/* In the middle of the cell. */
		tiles[index].width = (int32_t)((float)width * scale);
		tiles[index].height = (int32_t)((float)height * scale);
		tiles[index].x = row_x + column * (cell_width + WISEVIEW_GUTTER) + (cell_width - tiles[index].width) / 2;
		tiles[index].y = WISEVIEW_TOP + row * (cell_height + WISEVIEW_LABEL + WISEVIEW_GUTTER) + (cell_height - tiles[index].height) / 2;
	}
}

/*
 * Draws Wiseview as far as it is open: the wallpaper blurred and darkened,
 * each window on its way from its place to its tile (the stacking order
 * kept, so the top window stays in front while they move), the labels, the
 * header and the footer.
 */
static void
draw_wiseview(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object **stacked,
	unsigned stacked_count,
	float progress)
{
	static const float shade[4] = { 0.0f, 0.02f, 0.06f, 0.08f };
	static const float dark[4] = { 0.10f, 0.14f, 0.22f, 1.0f };
	struct zwl_object *windows[WISEVIEW_WINDOWS];
	struct shell_rect tiles[WISEVIEW_WINDOWS];
	struct glass_shape shape;
	struct shell_rect body;
	struct shell_rect rect;
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	char header[48];
	float ink[4];
	float wash[4];
	unsigned count;
	unsigned index;
	unsigned slot;
	unsigned over;
	int32_t width;

	/* The blurred wallpaper over the sharp one, and a little darker. */
	glass_shape_init(&shape, 0.0f, 0.0f, (float)server->width, (float)server->height);
	shape.mode = MODE_GLASS;
	shape.opacity = progress;
	glass_shape_draw(server, command, &shape);
	glass_draw_solid(server, command, 0.0f, 0.0f, (float)server->width, (float)server->height, 0.0f, shade);

	/* The windows and their tiles. */
	count = wiseview_windows(server, windows, WISEVIEW_WINDOWS);
	wiseview_layout(server, windows, count, tiles);

	/* In stacking order, each window between its place and its tile. */
	for (index = 0; index < stacked_count; index++) {
		/* Its tile. */
		for (slot = 0; slot < count; slot++) {
			if (windows[slot] == stacked[index])
				break;
		}

		/* A window without a tile is not shown. */
		if (slot == count)
			continue;

		/* The dragged tile is drawn last, over the others. */
		if (server->wiseview_dragging && stacked[index] == server->wiseview_press)
			continue;

		/* On its way; a minimized window's tile is washed paler. */
		body_rect(server, stacked[index], &body);
		lerp_rect(&body, &tiles[slot], progress, &rect);
		over = 0;
		if (progress >= 1.0f &&
		    server->pointer_x >= tiles[slot].x && server->pointer_x < tiles[slot].x + tiles[slot].width &&
		    server->pointer_y >= tiles[slot].y && server->pointer_y < tiles[slot].y + tiles[slot].height)
			over = 1;
		draw_tile(server, command, stacked[index], &rect, progress, stacked[index] == server->wiseview_current, over);
		if (stacked[index]->minimized) {
			memcpy(wash, white, sizeof(wash));
			wash[3] = 0.5f * progress;
			glass_draw_solid(server, command, (float)rect.x, (float)rect.y, (float)rect.width, (float)rect.height, 16.0f, wash);
		}
	}

	/* The dragged tile follows the pointer. */
	for (slot = 0; server->wiseview_dragging && slot < count; slot++) {
		if (windows[slot] != server->wiseview_press)
			continue;
		rect = tiles[slot];
		rect.x += server->pointer_x - server->wiseview_press_x;
		rect.y += server->pointer_y - server->wiseview_press_y;
		draw_tile(server, command, windows[slot], &rect, progress, 0, 1);
	}

	/* The header: what is shown, and how many. */
	memcpy(ink, dark, sizeof(ink));
	ink[3] = progress;
	if (count == 1U)
		(void)snprintf(header, sizeof(header), "Wiseview  -  1 window");
	else
		(void)snprintf(header, sizeof(header), "Wiseview  -  %u windows", count);
	glass_draw_text(server, command, SIZE_TITLE, WISEVIEW_SIDE, ZWL_GLASS_BAR + 34, header, 400, ink);

	/* One window alone: say there are no others. */
	if (count == 1U)
		glass_draw_text(server, command, SIZE_BAR, WISEVIEW_SIDE, ZWL_GLASS_BAR + 54, "No other windows", 400, ink);

	/* The footer: a handle and how to go back. */
	ink[3] = progress * 0.35f;
	glass_draw_solid(server, command, (float)((int32_t)server->width / 2 - 24), (float)((int32_t)server->height - 44), 48.0f, 5.0f, 2.5f, ink);
	ink[3] = progress * 0.7f;
	width = glass_text_width(server, SIZE_BAR, "Swipe down to return to your window");
	glass_draw_text(server, command, SIZE_BAR, ((int32_t)server->width - width) / 2, (int32_t)server->height - 18, "Swipe down to return to your window", 400, ink);

	/* A frame of the way. */
	if (server->log_frames)
		printf("ZWL WISEVIEW frame progress=%.2f windows=%u\n", (double)progress, count);
}

/*
 * Draws one window in Wiseview: its shadow, its image with rounded corners
 * (sampled linearly, as it is smaller), a blue glow for the window that was
 * on top or the one under the pointer, its floating title bar fading as it
 * goes, and its label and (under the pointer) close button fading in.
 */
static void
draw_tile(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	const struct shell_rect *tile,
	float progress,
	unsigned current,
	unsigned over)
{
	static const float glow[4] = { 0.25f, 0.52f, 0.98f, 0.45f };
	static const float dark[4] = { 0.12f, 0.16f, 0.24f, 1.0f };
	static const float button[4] = { 1.0f, 1.0f, 1.0f, 0.92f };
	const struct zwl_import *image;
	struct glass_shape shape;
	struct shell_rect panel;
	float place[4];
	unsigned panels;
	int32_t width;
	int32_t height;
	int32_t label_width;
	int32_t label_x;
	int32_t label_y;
	int32_t cx;
	int32_t cy;
	float colour[4];
	float appear;
	float radius;
	unsigned square;
	int decorated;

	/* The tile's corners: rounded, or square for a window that keeps them (window_square). */
	radius = WISEVIEW_RADIUS;
	square = window_square(surface);
	if (square)
		radius = 0.0f;

	/* The shadow, or a blue glow for the window that was on top or is under the pointer. */
	glass_shape_init(&shape, (float)tile->x, (float)tile->y + 6.0f, (float)tile->width, (float)tile->height);
	shape.quad[0] -= 48.0f;
	shape.quad[1] -= 48.0f;
	shape.quad[2] += 96.0f;
	shape.quad[3] += 96.0f;
	shape.mode = MODE_SHADOW;
	shape.radius = radius;
	shape.soft = 24.0f;
	shape.color[0] = 0.10f;
	shape.color[1] = 0.18f;
	shape.color[2] = 0.35f;
	shape.color[3] = 0.22f;
	if (current || over) {
		memcpy(shape.color, glow, sizeof(shape.color));
		shape.opacity = progress;
		if (over)
			shape.color[3] = 0.70f;
	}

	/* Drawn under the tile, unless the window is glass panels, whose own glass shows under the image. */
	image = zwl_compose_surface_image(surface);
	panels = zwl_panels_count(surface);
	if (panels == 0U || current || over)
		glass_shape_draw(server, command, &shape);

	/* The window's size (its viewport's, else its image's), for its glass panels' scale. */
	window_size(surface, &width, &height);
	if (width <= 0 || height <= 0) {
		width = (int32_t)image->width;
		height = (int32_t)image->height;
	}

	/* A window's glass panels, small with the tile. */
	place[0] = (float)tile->x;
	place[1] = (float)tile->y;
	place[2] = (float)tile->width / (float)width;
	place[3] = (float)tile->height / (float)height;
	zwl_panels_draw(server, command, surface, place, 1.0f, 0U);

	/* The image (its viewport's source), sampled linearly. */
	glass_shape_init(&shape, (float)tile->x, (float)tile->y, (float)tile->width, (float)tile->height);
	zwl_viewport_source(surface, shape.uv);
	shape.mode = MODE_IMAGE;
	shape.radius = radius;
	shape.set = image->linear_set;
	if (shape.set == VK_NULL_HANDLE)
		shape.set = image->set;
	if (image->draw == ZWL_DRAW_OPAQUE)
		shape.opaque = 1.0f;
	glass_shape_draw(server, command, &shape);

	/* A blue edge on the window that was on top, or under the pointer. */
	if (current || over) {
		glass_shape_init(&shape, (float)(tile->x - 3), (float)(tile->y - 3), (float)(tile->width + 6), (float)(tile->height + 6));
		shape.quad[0] -= 1.0f;
		shape.quad[1] -= 1.0f;
		shape.quad[2] += 2.0f;
		shape.quad[3] += 2.0f;
		shape.mode = MODE_RING;
		shape.radius = radius + 3.0f;
		shape.soft = 2.0f;
		memcpy(shape.color, glow, sizeof(shape.color));
		shape.color[3] = 0.9f;
		shape.opacity = progress;
		glass_shape_draw(server, command, &shape);
	}

	/* Only an SSD title bar fades as its window goes to a tile. */
	decorated = zwl_decoration_server(surface);
	if (decorated && !surface->maximized && progress < 1.0f) {
		floating_title(tile, &panel);
		draw_title_bar(server, command, surface, &panel, 1.0f - progress, 1.0f - progress, 0);
	}

	/* The label comes in over the last part of the way, when the tile is nearly in place. */
	appear = (progress - 0.6f) / 0.4f;
	if (appear <= 0.0f)
		return;

	/* The label under the tile: a glass pill with the mark and the title. */
	label_width = glass_text_width(server, SIZE_TITLE, surface->title) + 64;
	if (label_width > tile->width)
		label_width = tile->width;
	if (label_width < 120)
		label_width = 120;
	label_x = tile->x + (tile->width - label_width) / 2;
	label_y = tile->y + tile->height + 10;
	glass_shape_init(&shape, (float)label_x, (float)label_y, (float)label_width, 32.0f);
	shape.mode = MODE_GLASS;
	shape.radius = 16.0f;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.60f;
	shape.edge = 0.8f;
	shape.opacity = appear;
	glass_shape_draw(server, command, &shape);
	memcpy(colour, dark, sizeof(colour));
	colour[3] = appear;
	draw_title(server, command, surface, label_x + 10, label_y + 16, label_width - 50, colour);

	/* Under the pointer, the close button at the top right. */
	if (!over)
		return;
	cx = tile->x + tile->width - 14;
	cy = tile->y + 14;
	glass_draw_solid(server, command, (float)(cx - 12), (float)(cy - 12), 24.0f, 24.0f, 12.0f, button);
	glass_draw_glyph(server, command, SIZE_SIGN, GLASS_CLOSE_GLYPH, cx - glass_glyph_advance(server, SIZE_SIGN, GLASS_CLOSE_GLYPH) / 2, cy + 7, dark);
}

/*
 * Handles a button while Wiseview is open or opening: the release of the
 * gesture opens or closes it; a press on a tile's close button closes that
 * window, on a tile selects it (to the top, and Wiseview closes), elsewhere
 * closes Wiseview.  Every button is zdesktop's.
 */
static int
wiseview_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	struct zwl_object *windows[WISEVIEW_WINDOWS];
	struct shell_rect tiles[WISEVIEW_WINDOWS];
	struct zwl_object *surface;
	unsigned count;
	unsigned index;
	float progress;
	int target;

	/* The end of the gesture: past the threshold it opens, otherwise it closes. */
	if (server->wiseview_gesture) {
		if (state != 0)
			return 1;
		progress = wiseview_progress(server);
		server->wiseview_gesture = 0;
		if (progress > WISEVIEW_THRESHOLD) {
			printf("ZWL WISEVIEW opening from=%.2f\n", (double)progress);
			wiseview_settle(server, progress, 1.0f);
		} else {
			printf("ZWL WISEVIEW cancel from=%.2f\n", (double)progress);
			wiseview_settle(server, progress, 0.0f);
		}

		/* The release was zdesktop's. */
		return 1;
	}

	/* The release of a press on a tile: a drag let go on a desktop's picture moves the window there, a click selects it. */
	if (state == 0 && server->wiseview_press != NULL) {
		surface = server->wiseview_press;
		server->wiseview_press = NULL;
		if (server->wiseview_dragging) {
			server->wiseview_dragging = 0;
			server->dirty = 1;
			target = desktop_picture_at(server, server->pointer_x, server->pointer_y);
			if (target >= 0 && (unsigned)target != surface->desktop && !surface->dead)
				window_to_desktop(server, surface, (unsigned)target, "wiseview");
			return 1;
		}

		/* A click: a minimized window comes back; it comes to the top and Wiseview closes. */
		if (surface->dead || !surface->mapped)
			return 1;
		surface->minimized = 0;
		window_raise(server, surface);
		printf("ZWL WISEVIEW select surface=%u\n", surface->id);
		wiseview_settle(server, 1.0f, 0.0f);
		return 1;
	}

	/* Only a left press on the settled Wiseview acts. */
	if (state == 0 || button != ZWL_BUTTON_LEFT || server->wiseview_moving)
		return 1;

	/* The tile under the pointer. */
	count = wiseview_windows(server, windows, WISEVIEW_WINDOWS);
	wiseview_layout(server, windows, count, tiles);
	surface = NULL;
	for (index = 0; index < count; index++) {
		if (server->pointer_x >= tiles[index].x && server->pointer_x < tiles[index].x + tiles[index].width &&
		    server->pointer_y >= tiles[index].y && server->pointer_y < tiles[index].y + tiles[index].height) {
			surface = windows[index];
			break;
		}
	}

	/* A desktop's picture in the bar switches Wiseview's desktop (the bar stays zdesktop's). */
	target = desktop_picture_at(server, server->pointer_x, server->pointer_y);
	if (surface == NULL && target >= 0) {
		desktop_turn(server, target, "wiseview");
		return 1;
	}

	/* Elsewhere, Wiseview closes. */
	if (surface == NULL) {
		printf("ZWL WISEVIEW close\n");
		wiseview_settle(server, 1.0f, 0.0f);
		return 1;
	}

	/* Its close button closes the window. */
	if (server->pointer_x >= tiles[index].x + tiles[index].width - 26 && server->pointer_y < tiles[index].y + 26) {
		(void)zwl_emit(surface->client, surface->role->top->id, 1U, NULL, 0U);
		printf("ZWL WISEVIEW close-window surface=%u\n", surface->id);
		return 1;
	}

	/* Otherwise the press may be a click or the tile's drag: the release decides. */
	server->wiseview_press = surface;
	server->wiseview_press_x = server->pointer_x;
	server->wiseview_press_y = server->pointer_y;
	server->wiseview_dragging = 0;
	return 1;
}

/* Reports that Wiseview is open, and where each window's tile is. */
static void
wiseview_log(
	struct zwl_server *server)
{
	struct zwl_object *windows[WISEVIEW_WINDOWS];
	struct shell_rect tiles[WISEVIEW_WINDOWS];
	unsigned count;
	unsigned index;

	/* The tiles as they are laid out now. */
	count = wiseview_windows(server, windows, WISEVIEW_WINDOWS);
	wiseview_layout(server, windows, count, tiles);
	printf("ZWL WISEVIEW open windows=%u at_ms=%llu\n", count, (unsigned long long)zwl_milliseconds());
	for (index = 0; index < count; index++)
		printf("ZWL WISEVIEW tile client=%llu surface=%u x=%d y=%d width=%d height=%d\n", (unsigned long long)windows[index]->client->number, windows[index]->id, tiles[index].x, tiles[index].y, tiles[index].width, tiles[index].height);
}

/* Returns where the desktops are, as a desktop number: the one shown, swiped by the pointer, or sliding. */
static float
desktop_position(
	struct zwl_server *server)
{
	uint64_t elapsed;
	float t;

	/* Swiped: the desktop shown, moved by the swipe (a swipe to the left brings the next one). */
	if (server->desktop_dragging && server->desktop_offset != 0)
		return (float)server->desktop - (float)server->desktop_offset / (float)server->width;

	/* Settled. */
	if (!server->desktop_moving)
		return (float)server->desktop;

	/* Sliding: eased (cubic ease-out) from where the desktops were to the desktop. */
	elapsed = zwl_milliseconds() - server->desktop_start_ms;
	t = (float)elapsed / (float)DESKTOP_MS;
	if (t > 1.0f)
		t = 1.0f;
	t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
	return server->desktop_from + (server->desktop_to - server->desktop_from) * t;
}

/* Switches to a desktop (clamped to those there are), sliding from where the desktops are; its top window takes the focus. */
static void
desktop_turn(
	struct zwl_server *server,
	int target,
	const char *via)
{
	float from;

	/* One of the desktops. */
	if (target < 0)
		target = 0;
	if (target >= DESKTOPS)
		target = DESKTOPS - 1;

	/* From where they are to it. */
	from = desktop_position(server);
	server->desktop = (unsigned)target;
	server->desktop_from = from;
	server->desktop_to = (float)target;
	server->desktop_start_ms = zwl_milliseconds();
	server->desktop_moving = 1;
	server->desktop_dragging = 0;
	server->desktop_offset = 0;
	server->drag = NULL;
	server->pull = NULL;
	server->dirty = 1;

	/* The focus goes to the desktop's top window (or nobody). */
	server->front_surface = zwl_top_window(server);
	zwl_seat_focus(server);
	printf("ZWL GLASS desktop=%u via=%s\n", server->desktop + 1U, via);
}

/* Ends a press at the edge: a swipe of a quarter of the output switches to the neighbour, a shorter one goes back. */
static void
desktop_release(
	struct zwl_server *server)
{
	int32_t dx;

	/* The press is over; without a swipe nothing happens. */
	server->desktop_press = 0;
	if (!server->desktop_dragging)
		return;

	/* Far enough: the neighbour on that side; else back. */
	dx = server->pointer_x - server->desktop_start_x;
	if (dx >= (int32_t)server->width / 4) {
		desktop_turn(server, (int)server->desktop - 1, "swipe");
	} else if (dx <= -(int32_t)server->width / 4) {
		desktop_turn(server, (int)server->desktop + 1, "swipe");
	} else {
		desktop_turn(server, (int)server->desktop, "swipe");
	}
}

/* Counts the mapped windows of a desktop. */
static unsigned
desktop_windows(
	struct zwl_server *server,
	unsigned desktop)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	unsigned count;

	/* Every live, mapped window with a role on that desktop. */
	count = 0U;
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			if (surface->kind != ZWL_SURFACE || surface->dead || !surface->mapped || surface->role == NULL || surface->cursor_role)
				continue;
			if (surface->desktop == desktop)
				count++;
		}
	}

	/* The count. */
	return count;
}

/*
 * Where a docked window being pulled is: a fraction of the way (the pull
 * over PULL_DISTANCE, eased) from the docked space to its own size under
 * the pointer, where it comes off at PULL_DISTANCE.
 */
static void
pulled_rect(
	struct zwl_server *server,
	const struct zwl_object *surface,
	struct shell_rect *body)
{
	struct shell_rect docked;
	struct shell_rect own;
	float t;

	/* The docked space. */
	docked_rect(server, &docked);

	/* Its own size, placed as the pull leaves it (the same part of the title under the pointer). */
	own.width = (int32_t)surface->restore_width;
	own.height = (int32_t)surface->restore_height;
	own.x = server->pointer_x - (int32_t)((int64_t)surface->restore_width * server->pointer_x / (int32_t)server->width);
	own.y = server->pointer_y + ZWL_GLASS_GAP + ZWL_GLASS_TITLE / 2;

	/* Eased out along the pull. */
	t = (float)server->pull_distance / (float)PULL_DISTANCE;
	if (t > 1.0f)
		t = 1.0f;
	t = 1.0f - (1.0f - t) * (1.0f - t);
	lerp_rect(&docked, &own, t, body);
}

/*
 * Ends a pull short of coming off: the window springs back from where it
 * was pulled to the docked space (the dock animation), or, not pulled at
 * all, stays.
 */
static void
pull_back(
	struct zwl_server *server)
{
	struct zwl_object *surface;
	struct shell_rect from;
	struct shell_rect to;

	/* The pull ends. */
	surface = server->pull;
	server->pull = NULL;
	if (surface == NULL || surface->dead || !surface->mapped || server->pull_distance <= 0) {
		server->pull_distance = 0;
		return;
	}

	/* From where it was pulled back to the docked space. */
	server->pull = surface;
	body_rect(server, surface, &from);
	server->pull = NULL;
	server->pull_distance = 0;
	docked_rect(server, &to);
	memcpy(server->anim_from, &from, sizeof(server->anim_from));
	memcpy(server->anim_to, &to, sizeof(server->anim_to));
	server->anim = surface;
	server->anim_docking = 1;
	server->anim_start_ms = zwl_milliseconds();
	server->dirty = 1;
	printf("ZWL GLASS pull back surface=%u\n", surface->id);
}

/* Hides a window (it keeps its place, and comes back from Wiseview); the next window takes the focus. */
static void
window_minimize(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	/* Hidden, not moved or pulled any more. */
	surface->minimized = 1;
	if (server->drag == surface)
		server->drag = NULL;
	if (server->pull == surface)
		server->pull = NULL;

	/* The focus goes to the window under it. */
	server->front_surface = zwl_top_window(server);
	zwl_seat_focus(server);
	server->dirty = 1;
	printf("ZWL GLASS minimize surface=%u\n", surface->id);
}

/* Moves a window to another desktop (shown when that desktop is), and gives the focus to the top window of the desktop shown. */
static void
window_to_desktop(
	struct zwl_server *server,
	struct zwl_object *surface,
	unsigned desktop,
	const char *via)
{
	/* The window's desktop; on top of it there. */
	surface->desktop = desktop;
	server->map_order++;
	surface->map_order = server->map_order;

	/* The focus on the desktop shown. */
	server->front_surface = zwl_top_window(server);
	zwl_seat_focus(server);
	server->dirty = 1;
	printf("ZWL GLASS move-desktop surface=%u desktop=%u via=%s\n", surface->id, desktop + 1U, via);
}

/* Returns the desktop whose picture in the system bar is under a point, or -1. */
static int
desktop_picture_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y)
{
	struct shell_bar bar;
	int32_t offset;

	/* In the bar, over the pictures. */
	if (y < 0 || y >= ZWL_GLASS_BAR)
		return -1;
	bar_layout(server, &bar);
	offset = x - (bar.desktops_x + 6);
	if (offset < 0 || offset >= DESKTOPS * (DESKTOP_WIDTH + DESKTOP_GAP))
		return -1;

	/* The picture (its gap counts as its own). */
	return offset / (DESKTOP_WIDTH + DESKTOP_GAP);
}

/* Follows the pointer for the glass look's screens, gestures, menus and moves; returns 1 when the motion is theirs. */
static int
glass_motion_take(
	struct zwl_server *server)
{
	struct zwl_object *surface;
	int32_t lowest;
	int32_t x;
	int32_t y;
	int32_t dx;
	int taken;
	int calm;

	/* The hover of buttons, the dock hint and the moves are redrawn (over a window's own area only the cursor is, damage.c). */
	calm = zwl_glass_pointer_calm(server, server->pointer_x, server->pointer_y);
	if (!calm)
		server->dirty = 1;

	/* The login screen lights its buttons under the pointer, and takes the motion. */
	if (server->greeter) {
		server->dirty = 1;
		return 1;
	}

	/* Wiseview follows the gesture, and hears the pointer while it is open; a pressed tile that moves is dragged. */
	if (server->wiseview_gesture || server->wiseview > 0.0f || server->wiseview_moving) {
		if (server->wiseview_press != NULL && !server->wiseview_dragging) {
			x = server->pointer_x - server->wiseview_press_x;
			y = server->pointer_y - server->wiseview_press_y;
			if (x * x + y * y >= TILE_DRAG_START * TILE_DRAG_START) {
				server->wiseview_dragging = 1;
				printf("ZWL WISEVIEW drag surface=%u\n", server->wiseview_press->id);
			}
		}

		/* The motion is Wiseview's. */
		return 1;
	}

	/* The top-right corner's swipe follows the pointer (corner.c). */
	taken = zwl_corner_motion(server);
	if (taken)
		return 1;

	/* The on-screen keyboard's swipe and a press on its panel (keyboard.c). */
	taken = zwl_keyboard_motion(server);
	if (taken)
		return 1;

	/* App Home follows its gesture, and hears the pointer while it shows. */
	taken = zwl_home_motion(server);
	if (taken)
		return 1;

	/* The network's open menu lights the row under the pointer (network.c). */
	taken = zwl_network_motion(server);
	if (taken)
		return 1;

	/* The volume's open popup follows a drag of its slider (volume.c). */
	taken = zwl_volume_motion(server);
	if (taken)
		return 1;

	/* An open menu follows the pointer (menu-shell.c). */
	taken = zwl_menu_motion(server);
	if (taken)
		return 1;

	/* The desktops' swipe: past DESKTOP_START the windows follow the pointer (with resistance where there is no neighbour). */
	if (server->desktop_press) {
		dx = server->pointer_x - server->desktop_start_x;
		if (!server->desktop_dragging && (dx >= DESKTOP_START || dx <= -DESKTOP_START)) {
			server->desktop_dragging = 1;
			server->desktop_moving = 0;
			printf("ZWL GLASS desktop swipe\n");
		}

		/* The offset follows the pointer. */
		if (server->desktop_dragging) {
			server->desktop_offset = dx;
			if ((dx > 0 && server->desktop == 0U) || (dx < 0 && server->desktop + 1U >= (unsigned)DESKTOPS))
				server->desktop_offset = dx / 4;
		}

		/* The motion was the swipe's. */
		return 1;
	}

	/* A docked title pulled far enough down comes off under the pointer, and the move goes on. */
	surface = server->pull;
	if (surface != NULL) {
		if (surface->dead || !surface->mapped) {
			server->pull = NULL;
			return 1;
		}

		/* Not far enough yet: the window follows the pull. */
		server->pull_distance = server->pointer_y - server->pull_start_y;
		if (server->pull_distance < 0)
			server->pull_distance = 0;
		if (server->pull_distance < PULL_DISTANCE)
			return 1;
		server->pull_distance = 0;

		/* The same part of the title bar stays under the pointer. */
		x = server->pointer_x - (int32_t)((int64_t)surface->restore_width * server->pointer_x / (int32_t)server->width);
		y = server->pointer_y + ZWL_GLASS_GAP + ZWL_GLASS_TITLE / 2;
		window_undock(server, surface, x, y, "pull");
		server->pull = NULL;
		server->drag = surface;
		server->drag_dx = server->pointer_x - x;
		server->drag_dy = server->pointer_y - y;
		server->drag_start_x = surface->restore_x;
		server->drag_start_y = surface->restore_y;
		return 1;
	}

	/* Without a move the client hears the motion. */
	surface = server->drag;
	if (surface == NULL)
		return 0;

	/* A window that went away ends the move. */
	if (surface->dead || !surface->mapped) {
		server->drag = NULL;
		return 1;
	}

	/* The body follows the pointer; the title bar stays below the system bar. */
	surface->x = server->pointer_x - server->drag_dx;
	surface->y = server->pointer_y - server->drag_dy;
	lowest = ZWL_GLASS_BAR + ZWL_GLASS_GAP + ZWL_GLASS_TITLE;
	if (surface->y < lowest)
		surface->y = lowest;

	/* Succeeded: the motion was zdesktop's. */
	return 1;
}
