/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Titlebar Presentation as zdesktop draws and operates it (WS070 p010,
 * plan/ws070/titlebar-design.md sections 6, 7, 9 and 10).
 *
 * A window's titlebar shows, after its mark and title, the presentation its
 * client chose: its menu (menu-shell.c, the default), its controls or its
 * tabs.  The
 * controls are laid out in the room there is -- in the floating titlebar,
 * or in the system bar's application zone while the window is docked --
 * the navigation buttons on the left, the breadcrumb stretching after them,
 * the search field, view selector and other buttons on the right, and "..."
 * at the right end.  When the room runs short the breadcrumb loses its
 * leading parts, the search field becomes a button, and then the controls
 * give way to the "..." popup, the least important first.
 *
 * Tabs are pills in a strip (WS070 p011): the active one white with a fine
 * edge, the others a faint wash of the ink, a dot on one that wants
 * attention, a close button on the active one and on the one under the
 * pointer, and "+" after them when the client asks for it.  When the room
 * runs short the window's title gives way first, then the tabs narrow to a
 * least width (their titles cut in the middle), then the strip scrolls
 * between two arrows (and with the wheel), and "..." lists the tabs out of
 * sight.  Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+PageUp and Ctrl+PageDown go
 * between the focused window's tabs when its menu has no such shortcut.
 * Closing a tab and a new one are the application's keys (its menu's
 * shortcuts): a terminal's shell needs Ctrl+W and Ctrl+T.
 *
 * A press on a control is taken here and the control acts on the release
 * over it.  The search field, and the breadcrumb when the client asks for
 * the path to be edited, are text fields zdesktop owns: while one has the
 * keyboard, the keys edit it, and the client hears the text as it changes
 * and how the editing ended.  Hits are tested against the places the
 * controls were last drawn at, so the pointer finds what the user sees.
 */

#include "menu.h"
#include "titlebar.h"

#include <stdio.h>
#include <string.h>

/* How many control regions one frame records, and the windows whose layout is logged. */
#define SHELL_HITS		256U
#define SHELL_LOGGED		32U

/* A control's square (floating, docked), the gaps, and the icons' size. */
#define CONTROL_SIZE		30
#define CONTROL_SIZE_DOCKED	28
#define CONTROL_GAP		4
#define GROUP_PADDING		2
#define SIDE_GAP		10
#define ICON_PIXELS		16U

/* The search field's width (and the least it shrinks to), the breadcrumb's least width. */
#define SEARCH_WIDTH		220
#define SEARCH_LEAST		150
#define CRUMB_LEAST		120

/* A breadcrumb part's padding, and the room of the arrow between two. */
#define CRUMB_PADDING		8
#define CRUMB_ARROW		12

/* A text label's padding (generic and primary controls). */
#define LABEL_PADDING		12

/* A tab's widest and least width, its title's padding, its close button's room, and the gap between tabs. */
#define TAB_MOST		200
#define TAB_LEAST		96
#define TAB_PADDING		12
#define TAB_CLOSE		20
#define TAB_ATTENTION		10
#define TAB_GAP			4

/* The least a window's title gives way to its tabs, and the gap between the title and them. */
#define TAB_TITLE_LEAST		56
#define TAB_TITLE_GAP		18

/* The most tabs one window lays out. */
#define SHELL_TABS		ZWL_TITLEBAR_TABS_MAX

/* The keys a field knows (evdev codes). */
#define FIELD_KEY_ESC		1U
#define FIELD_KEY_BACKSPACE	14U
#define FIELD_KEY_TAB		15U
#define FIELD_KEY_ENTER		28U
#define FIELD_KEY_A		30U
#define FIELD_KEY_C		46U
#define FIELD_KEY_V		47U
#define FIELD_KEY_X		45U
#define FIELD_KEY_Z		44U
#define FIELD_KEY_KPENTER	96U
#define FIELD_KEY_HOME		102U
#define FIELD_KEY_LEFT		105U
#define FIELD_KEY_RIGHT		106U
#define FIELD_KEY_END		107U
#define FIELD_KEY_DELETE	111U

/* The keys of the tabs (evdev codes). */
#define TAB_KEY_TAB		15U
#define TAB_KEY_PAGEUP		104U
#define TAB_KEY_PAGEDOWN	109U

/* The modifier bits of zdesktop's wl_keyboard.modifiers. */
#define SEAT_SHIFT		0x01U
#define SEAT_CTRL		0x04U
#define SEAT_ALT		0x08U
#define SEAT_META		0x40U

/* The most controls one window lays out. */
#define SHELL_ITEMS		ZWL_TITLEBAR_CONTROLS_MAX

/*
 * The kinds of region a frame records: a button (or a text control shown
 * as a button), a face of a segmented group, a breadcrumb's part, and a
 * text field.
 */
enum shell_kind {
	KIND_BUTTON,
	KIND_FACE,
	KIND_CRUMB,
	KIND_FIELD,
	KIND_TAB,
	KIND_TAB_CLOSE,
	KIND_NEW_TAB,
	KIND_SCROLL
};

/*
 * Where a control goes: on the left (navigation), the breadcrumb between,
 * or on the right.
 */
enum shell_side {
	SIDE_LEFT,
	SIDE_CRUMB,
	SIDE_RIGHT
};

/*
 * One region of the last frame the pointer can press: the window, whether
 * in the system bar, its kind, the control and a detail (a breadcrumb's
 * part), and its rectangle on the output.
 */
struct shell_hit {
	struct zwl_object *surface;
	unsigned docked;
	unsigned kind;
	uint32_t id;
	uint32_t detail;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
};

/*
 * One control as a frame lays it out: the control, its side, its width, the
 * unit it gives way with (a segmented group is one unit), whether it is
 * shown, and where it is.
 */
struct shell_item {
	const struct zwl_titlebar_control *control;
	unsigned side;
	unsigned unit;
	unsigned shown;
	int32_t width;
	int32_t x;
};

/*
 * One tab as a frame lays it out: the tab, the width it would like, whether
 * it is shown, and where it is and how wide.
 */
struct shell_tab {
	const struct zwl_titlebar_tab *tab;
	int32_t natural;
	unsigned shown;
	int32_t x;
	int32_t width;
};

/*
 * A tab strip as a frame lays it out: whether it scrolls (with its arrows),
 * the first tab shown and how many, whether "+" and "..." are there, and
 * where the tabs start.
 */
struct shell_strip {
	unsigned scrolling;
	unsigned first;
	unsigned shown;
	unsigned new_button;
	unsigned overflow;
	int32_t start;
};

/*
 * A text field zdesktop owns: the window and the control (NULL when none
 * has the keyboard), whether it edits a breadcrumb's path, and the text
 * with its cursor and the other end of its selection (byte offsets on
 * character boundaries).
 */
struct shell_field {
	struct zwl_object *surface;
	uint32_t id;
	unsigned edit;
	char text[ZWL_TITLEBAR_TEXT_MAX + 1U];
	size_t length;
	size_t cursor;
	size_t anchor;
};

/* The last layout logged for a window and place (a checksum), so a layout is logged once. */
struct shell_logged {
	struct zwl_object *surface;
	unsigned docked;
	uint32_t checksum;
};

/*
 * The presentation's state: the regions of the last frame, the press in
 * progress (its window is NULL when none), the text field with the
 * keyboard, a key whose press the field took (so its release is taken
 * too), and the layouts logged.
 */
struct shell_titlebar {
	struct shell_hit hits[SHELL_HITS];
	unsigned hit_count;
	struct shell_hit pressed;
	unsigned pressing;
	struct shell_field field;
	uint32_t eaten_key;
	struct shell_logged logged[SHELL_LOGGED];
};

/*
 * The one compositor's titlebar presentation.  zdesktop runs one server per
 * process; the zero value is "nothing drawn, nothing pressed, no field".
 */
static struct shell_titlebar shell_titlebar;

static unsigned shell_mode(struct zwl_object *surface, struct zwl_titlebar_model **model, struct zwl_object **titlebar);
static unsigned shell_layout(struct zwl_server *server, const struct zwl_titlebar_state *state, unsigned docked, const struct zwl_menu_area *area, unsigned has_menu, struct shell_item *items, int32_t *crumb_width, unsigned *overflow, unsigned *compact_search);
static int32_t shell_item_width(struct zwl_server *server, const struct zwl_titlebar_control *control, int32_t size, unsigned compact_search);
static unsigned shell_side(const struct zwl_titlebar_control *control);
static int32_t shell_crumb_full(struct zwl_server *server, const struct zwl_titlebar_control *control);
static void shell_draw_controls(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned docked, const struct zwl_menu_area *area, const float *ink, float fade);
static void shell_draw_button(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned docked, const struct zwl_titlebar_control *control, int32_t x, int32_t y, int32_t width, int32_t size, unsigned kind, const float *ink, float fade, unsigned recording);
static void shell_draw_search(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned docked, const struct zwl_titlebar_control *control, int32_t x, int32_t y, int32_t width, int32_t size, const float *ink, float fade, unsigned recording);
static void shell_draw_crumbs(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned docked, const struct zwl_titlebar_control *control, int32_t x, int32_t y, int32_t width, int32_t size, const float *ink, float fade, unsigned recording);
static void shell_draw_field_text(struct zwl_server *server, VkCommandBuffer command, int32_t x, int32_t baseline, int32_t width, const float *ink, float fade);
static void shell_draw_progress(struct zwl_server *server, VkCommandBuffer command, const struct zwl_titlebar_control *control, int32_t x, int32_t y, int32_t size, float fade);
static int32_t shell_tab_natural(struct zwl_server *server, const struct zwl_titlebar_tab *tab);
static int32_t shell_tabs_need(struct zwl_server *server, struct zwl_object *surface, const struct zwl_titlebar_model *model);
static unsigned shell_tabs_layout(struct zwl_server *server, struct zwl_titlebar_model *model, const struct zwl_menu_area *area, int32_t size, unsigned has_menu, struct shell_tab *tabs, struct shell_strip *strip);
static void shell_draw_tabs(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned docked, const struct zwl_menu_area *area, const float *ink, float fade);
static void shell_draw_tab(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, unsigned docked, const struct shell_tab *tab, int32_t y, int32_t size, const float *ink, float fade, unsigned recording, int32_t *close_x);
static void shell_act_tabs(struct zwl_server *server, const struct shell_hit *hit, struct zwl_titlebar_model *model, struct zwl_object *titlebar);
static void shell_log_strip(struct zwl_object *surface, unsigned docked, const struct shell_tab *tabs, unsigned count, const int32_t *closes, int32_t y, int32_t size, const int32_t *buttons, uint32_t checksum);
static unsigned shell_icon(uint32_t role);
static int shell_hovered(struct zwl_server *server, int32_t x, int32_t y, int32_t width, int32_t height);
static void shell_draw_drop_part(struct zwl_server *server, VkCommandBuffer command, struct zwl_object *surface, uint32_t id, uint32_t detail, int32_t x, int32_t y, int32_t width, int32_t size, float fade);
static void shell_add_hit(struct zwl_object *surface, unsigned docked, unsigned kind, uint32_t id, uint32_t detail, int32_t x, int32_t y, int32_t width, int32_t height);
static const struct shell_hit *shell_hit_at(int32_t x, int32_t y);
static void shell_act(struct zwl_server *server, const struct shell_hit *hit);
static void shell_focus(struct zwl_server *server, struct zwl_object *surface, const struct zwl_titlebar_control *control, unsigned edit);
static void shell_field_done(struct zwl_server *server, uint32_t how);
static void shell_field_changed(struct zwl_server *server);
static void shell_field_insert(const char *text, size_t length);
static void shell_field_erase(int forward);
static size_t shell_field_step(size_t at, int step);
static void shell_log_layout(struct zwl_object *surface, unsigned docked, const struct shell_item *items, unsigned count, int32_t y, int32_t size, int32_t overflow_x, uint32_t checksum);
static uint32_t shell_mix(uint32_t checksum, uint32_t value);
static void shell_colour(float *colour, const float *ink, float alpha);

/*
 * Starts a frame: the controls are hit-tested where this frame draws them.
 */
void
zwl_titlebar_frame(
	struct zwl_server *server)
{
	/* The server is the one this file's state belongs to. */
	(void)server;

	/* The last frame's places are forgotten. */
	shell_titlebar.hit_count = 0;
}

/*
 * Tells how wide a window's title may be drawn: the menu's share in menu
 * mode, a fifth of the room with controls.
 */
int32_t
zwl_titlebar_title_limit(
	struct zwl_server *server,
	struct zwl_object *surface,
	int32_t available)
{
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	int32_t limit;
	int32_t least;
	int32_t need;
	unsigned mode;

	/* In menu mode, the menu decides. */
	mode = shell_mode(surface, &model, &titlebar);
	if (mode == ZWL_TITLEBAR_MENU) {
		limit = zwl_menu_title_limit(server, surface, available);
		return limit;
	}

	/* Otherwise the controls take most of the room; the title is the window's name. */
	limit = available / 5;
	if (mode != ZWL_TITLEBAR_TABS)
		return limit;

	/* Tabs that fit as they would like leave the title its fifth. */
	need = shell_tabs_need(server, surface, model);
	if (available - TAB_TITLE_GAP - need >= limit)
		return limit;

	/* Otherwise the title gives them its room, down to its least (never more than the fifth). */
	least = TAB_TITLE_LEAST;
	if (least > limit)
		least = limit;
	limit = available - TAB_TITLE_GAP - need;
	if (limit < least)
		limit = least;

	/* Succeeded: the title's share beside the tabs. */
	return limit;
}

/*
 * Draws a window's presentation in the area after its title (in its
 * titlebar, or in the system bar when docked): its menu, or its controls.
 */
void
zwl_titlebar_draw(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct zwl_menu_area *area,
	const float *ink,
	float fade)
{
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	unsigned mode;

	/* The window's mode. */
	mode = shell_mode(surface, &model, &titlebar);

	/* A focus the client asked for is taken now that the controls are drawn. */
	if (model != NULL && model->focus_id != 0U) {
		shell_focus(server, surface, zwl_titlebar_control(&model->shown, model->focus_id), model->focus_mode);
		model->focus_id = 0;
	}

	/* A field of a window that shows no controls any more stops being edited. */
	if (mode != ZWL_TITLEBAR_CONTROLS && shell_titlebar.field.surface == surface)
		shell_field_done(server, ZWL_TEXT_LEFT);

	/* The menu mode is the menus' to draw. */
	if (mode == ZWL_TITLEBAR_MENU) {
		zwl_menu_draw_bar(server, command, surface, docked, area, ink, fade);
		return;
	}

	/* The controls, or the tabs. */
	if (mode == ZWL_TITLEBAR_CONTROLS) {
		shell_draw_controls(server, command, surface, docked, area, ink, fade);
	} else if (mode == ZWL_TITLEBAR_TABS) {
		shell_draw_tabs(server, command, surface, docked, area, ink, fade);
	}
}

/*
 * Handles a pointer button for the controls: a left press on one is taken
 * and the control acts on the release over it; a press elsewhere ends the
 * editing of a field.  Returns 1 when the button was the controls'.
 */
int
zwl_titlebar_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	const struct shell_hit *hit;
	struct shell_hit pressed;
	struct zwl_object *top;

	/* Only the left button. */
	if (button != ZWL_BUTTON_LEFT)
		return 0;

	/* A release ends the press, and acts when it is over the pressed region. */
	if (state == 0U) {
		if (shell_titlebar.pressing == 0U)
			return 0;
		shell_titlebar.pressing = 0;
		pressed = shell_titlebar.pressed;
		server->dirty = 1;

		/* The region under the release must be the one pressed. */
		hit = shell_hit_at(server->pointer_x, server->pointer_y);
		if (hit == NULL)
			return 1;
		if (hit->surface != pressed.surface || hit->id != pressed.id || hit->kind != pressed.kind || hit->detail != pressed.detail)
			return 1;

		/* The control acts. */
		shell_act(server, hit);
		return 1;
	}

	/* A press on a control is taken. */
	hit = shell_hit_at(server->pointer_x, server->pointer_y);

	/*
	 * A floating titlebar's control counts only where its window is the one
	 * on top: another window's title bar, body or frame over it takes the
	 * press (BUG-121: the frame's band of a window above, where the pointer
	 * shows the resize arrow, raised the window under it instead).
	 */
	if (hit != NULL && hit->docked == 0U) {
		top = zwl_glass_window_at(server, server->pointer_x, server->pointer_y);
		if (top != hit->surface)
			hit = NULL;
	}

	if (hit == NULL) {
		/* A press anywhere but on the field ends its editing, and goes on to what it is on. */
		if (shell_titlebar.field.surface != NULL)
			shell_field_done(server, ZWL_TEXT_LEFT);
		return 0;
	}

	/* A press outside the field (on another control) ends its editing too. */
	if (shell_titlebar.field.surface != NULL && (hit->kind != KIND_FIELD || hit->id != shell_titlebar.field.id))
		shell_field_done(server, ZWL_TEXT_LEFT);

	/* The window of a floating titlebar comes to the top; the press waits for its release. */
	if (hit->docked == 0U)
		zwl_glass_raise(server, hit->surface);
	shell_titlebar.pressed = *hit;
	shell_titlebar.pressing = 1;
	server->dirty = 1;

	/* Succeeded: the press is the controls'. */
	return 1;
}

/*
 * Handles a key for a field with the keyboard: the keys edit it, Enter and
 * Esc end it.  Keys with Alt or Super, and Ctrl keys other than the
 * editing ones, go on (to zdesktop's shortcuts and the menus).  Returns 1
 * when the key was the field's.
 */
int
zwl_titlebar_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	struct shell_field *field;
	uint32_t modifiers;
	uint32_t keysym;
	char character;
	int known;

	/* The release of a key the field took is its too. */
	if (state == 0U) {
		if (shell_titlebar.eaten_key == 0U || key != shell_titlebar.eaten_key)
			return 0;
		shell_titlebar.eaten_key = 0;
		return 1;
	}

	/* Only a field with the keyboard takes keys; a window that lost the keyboard leaves its field. */
	field = &shell_titlebar.field;
	if (field->surface == NULL)
		return 0;
	if (server->focus != field->surface) {
		shell_field_done(server, ZWL_TEXT_LEFT);
		return 0;
	}

	/* Alt and Super keys are not the field's. */
	modifiers = server->modifiers;
	if ((modifiers & (SEAT_ALT | SEAT_META)) != 0U)
		return 0;

	/* With Ctrl: select all, and the clipboard's keys (which do nothing here); others go on. */
	if ((modifiers & SEAT_CTRL) != 0U) {
		if (key == FIELD_KEY_A) {
			field->anchor = 0;
			field->cursor = field->length;
		} else if (key != FIELD_KEY_C && key != FIELD_KEY_X && key != FIELD_KEY_V && key != FIELD_KEY_Z) {
			return 0;
		}

		/* The key and its release are the field's. */
		shell_titlebar.eaten_key = key;
		server->dirty = 1;
		return 1;
	}

	/* The press and its release are the field's from here on. */
	shell_titlebar.eaten_key = key;
	server->dirty = 1;

	/* Each key that edits, moves or ends. */
	switch (key) {
	case FIELD_KEY_ESC:
		shell_field_done(server, ZWL_TEXT_CANCELLED);
		return 1;
	case FIELD_KEY_ENTER:
	case FIELD_KEY_KPENTER:
		shell_field_done(server, ZWL_TEXT_SUBMITTED);
		return 1;
	case FIELD_KEY_TAB:
		shell_field_done(server, ZWL_TEXT_LEFT);
		return 1;
	case FIELD_KEY_BACKSPACE:
		shell_field_erase(0);
		shell_field_changed(server);
		return 1;
	case FIELD_KEY_DELETE:
		shell_field_erase(1);
		shell_field_changed(server);
		return 1;
	case FIELD_KEY_LEFT:
		field->cursor = shell_field_step(field->cursor, -1);
		break;
	case FIELD_KEY_RIGHT:
		field->cursor = shell_field_step(field->cursor, 1);
		break;
	case FIELD_KEY_HOME:
		field->cursor = 0;
		break;
	case FIELD_KEY_END:
		field->cursor = field->length;
		break;
	default:
		/* A character of the US layout is typed; any other key does nothing. */
		known = zwl_menu_keysym(key, modifiers, &keysym);
		if (known == 0 || keysym < 0x20U || keysym > 0x7eU)
			return 1;
		character = (char)keysym;
		shell_field_insert(&character, 1U);
		shell_field_changed(server);
		return 1;
	}

	/* A move without Shift leaves no selection. */
	if ((modifiers & SEAT_SHIFT) == 0U)
		field->anchor = field->cursor;

	/* Succeeded: the key moved the cursor. */
	return 1;
}

/*
 * Handles the tabs' keys of the focused window when it shows tabs (after
 * its menu's shortcuts, which come first): Ctrl+Tab and Ctrl+PageDown
 * activate the next tab, Ctrl+Shift+Tab and Ctrl+PageUp the one before
 * (around the ends).  Ctrl+W and Ctrl+T stay the application's (a
 * terminal's shell uses them; its menu gives its own keys for tabs).
 * Returns 1 when the key was the tabs'.
 */
int
zwl_titlebar_tab_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	const struct zwl_titlebar_state *shown;
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	uint32_t modifiers;
	unsigned mode;
	unsigned count;
	unsigned active;
	unsigned index;
	unsigned shift;
	int step;

	/* Only a press, for a focused window. */
	if (state == 0U || server->focus == NULL)
		return 0;

	/* Only with Ctrl, and without Alt or Super. */
	modifiers = server->modifiers;
	if ((modifiers & SEAT_CTRL) == 0U || (modifiers & (SEAT_ALT | SEAT_META)) != 0U)
		return 0;

	/* Only for a window that shows tabs. */
	mode = shell_mode(server->focus, &model, &titlebar);
	if (mode != ZWL_TITLEBAR_TABS || titlebar == NULL)
		return 0;

	/* The tabs, and the active one (count when none is). */
	shown = &model->shown;
	count = shown->tab_count;
	active = count;
	for (index = 0; index < count; index++) {
		if ((shown->tabs[index].flags & ZWL_TAB_ACTIVE) != 0U && active == count)
			active = index;
	}

	/* Which way a key goes, or its request. */
	shift = 0;
	if ((modifiers & SEAT_SHIFT) != 0U)
		shift = 1;
	step = 0;
	switch (key) {
	case TAB_KEY_TAB:
		step = 1;
		if (shift != 0U)
			step = -1;
		break;
	case TAB_KEY_PAGEDOWN:
		step = 1;
		break;
	case TAB_KEY_PAGEUP:
		step = -1;
		break;
	default:
		return 0;
	}

	/* The key and its release are the tabs' even when there is no tab to go to. */
	shell_titlebar.eaten_key = key;
	server->dirty = 1;
	if (count == 0U)
		return 1;

	/* The neighbour around the ends (the first or the last when none is active). */
	if (active == count) {
		index = 0;
		if (step < 0)
			index = count - 1U;
	} else if (step > 0) {
		index = (active + 1U) % count;
	} else {
		index = (active + count - 1U) % count;
	}

	/* It is activated as its click would (the client's next commit shows it). */
	if (index != active)
		zwl_titlebar_send_tab(titlebar, shown->tabs[index].id, ZWL_TAB_EVENT_ACTIVATED);

	/* Succeeded: the key was the tabs'. */
	return 1;
}

/*
 * Handles the wheel over a tab strip: a strip that scrolls moves one tab
 * each way a notch goes (down or right to the tabs after), as its arrows
 * would.  Returns 1 when the wheel was over a strip.
 */
int
zwl_titlebar_axis(
	struct zwl_server *server,
	int32_t vertical,
	int32_t horizontal)
{
	const struct shell_hit *hit;
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	unsigned direction;
	unsigned index;
	unsigned mode;
	int32_t amount;

	/* Only over a tab strip's region (a tab, its close button, "+" or an arrow). */
	hit = shell_hit_at(server->pointer_x, server->pointer_y);
	if (hit == NULL)
		return 0;
	if (hit->kind != KIND_TAB && hit->kind != KIND_TAB_CLOSE && hit->kind != KIND_NEW_TAB && hit->kind != KIND_SCROLL)
		return 0;
	mode = shell_mode(hit->surface, &model, &titlebar);
	if (mode != ZWL_TITLEBAR_TABS)
		return 0;

	/* The way it goes: the vertical wheel, or else the horizontal one. */
	amount = vertical;
	if (amount == 0)
		amount = horizontal;
	if (amount == 0)
		return 1;
	direction = 0;
	if (amount > 0)
		direction = 1;

	/* The strip moves only while its arrow that way is live (drawn and recorded this frame). */
	for (index = 0; index < shell_titlebar.hit_count; index++) {
		if (shell_titlebar.hits[index].surface != hit->surface || shell_titlebar.hits[index].docked != hit->docked)
			continue;
		if (shell_titlebar.hits[index].kind != KIND_SCROLL || shell_titlebar.hits[index].detail != direction)
			continue;

		/* One tab that way (the layout keeps it within the tabs). */
		if (direction == 0U && model->tab_first > 0U)
			model->tab_first--;
		if (direction != 0U)
			model->tab_first++;
		printf("ZWL TITLEBAR strip scroll client=%llu surface=%u first=%u by=wheel\n", (unsigned long long)hit->surface->client->number, hit->surface->id, model->tab_first);
		server->dirty = 1;
		break;
	}

	/* The wheel was the strip's. */
	return 1;
}

/*
 * Finds the part of a breadcrumb at a point, where the last frame drew it,
 * for a drag and drop (data.c): its window, the window's titlebar, the
 * control and the part.  A floating titlebar counts only where its window
 * is the one on top.  Returns 1 when there is one.
 */
int
zwl_titlebar_drop_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y,
	struct zwl_object **surface,
	struct zwl_object **titlebar,
	uint32_t *id,
	uint32_t *detail)
{
	const struct shell_hit *hit;
	struct zwl_titlebar_model *model;
	struct zwl_object *top;
	unsigned mode;

	/* A breadcrumb's part under the point. */
	hit = shell_hit_at(x, y);
	if (hit == NULL || hit->kind != KIND_CRUMB)
		return 0;

	/* A floating one covered by another window is not there. */
	if (hit->docked == 0U) {
		top = zwl_glass_window_at(server, x, y);
		if (top != hit->surface)
			return 0;
	}

	/* The window's titlebar, showing its controls. */
	mode = shell_mode(hit->surface, &model, titlebar);
	if (mode != ZWL_TITLEBAR_CONTROLS || *titlebar == NULL)
		return 0;

	/* Succeeded: the window, the control and the part. */
	*surface = hit->surface;
	*id = hit->id;
	*detail = hit->detail;
	return 1;
}

/*
 * Carries out a hidden control chosen from the "..." popup (menu-shell.c).
 */
void
zwl_titlebar_overflow_chosen(
	struct zwl_server *server,
	struct zwl_object *surface,
	uint32_t id)
{
	const struct zwl_titlebar_control *control;
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	uint32_t detail;
	unsigned mode;
	unsigned index;

	/* A tab out of sight of a tab strip is chosen as its click would. */
	mode = shell_mode(surface, &model, &titlebar);
	if (mode == ZWL_TITLEBAR_TABS) {
		for (index = 0; index < model->shown.tab_count; index++) {
			if (model->shown.tabs[index].id == id)
				zwl_titlebar_send_tab(titlebar, id, ZWL_TAB_EVENT_ACTIVATED);
		}

		/* The tab is shown by the client's next commit. */
		server->dirty = 1;
		return;
	}

	/* The window's controls, and the control. */
	if (mode != ZWL_TITLEBAR_CONTROLS)
		return;
	control = zwl_titlebar_control(&model->shown, id);
	if (control == NULL || control->enabled == 0U)
		return;

	/* A search takes the keyboard; a breadcrumb goes to its last part; any other acts. */
	if (control->role == ZWL_CONTROL_SEARCH) {
		shell_focus(server, surface, control, 0U);
		return;
	}

	/* A breadcrumb goes to its last part; any other control has no detail. */
	detail = 0;
	if (control->role == ZWL_CONTROL_BREADCRUMB && control->segment_count > 0U)
		detail = control->segment_count - 1U;

	/* The event. */
	zwl_titlebar_send_activated(titlebar, id, detail, "overflow");
}

/*
 * Tells the client of a window with controls that its "..." popup opened.
 */
void
zwl_titlebar_overflow_opened(
	struct zwl_object *surface)
{
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	unsigned mode;

	/* Only a window with a titlebar presentation hears it. */
	mode = shell_mode(surface, &model, &titlebar);
	if (titlebar == NULL || mode == ZWL_TITLEBAR_MENU)
		return;

	/* The event. */
	zwl_titlebar_send_overflow(titlebar);
}

/*
 * Forgets an object that goes: its regions, a press or a field on its
 * window, and its logged layouts.  No event is sent.
 */
void
zwl_titlebar_forget(
	struct zwl_server *server,
	struct zwl_object *object)
{
	unsigned index;
	unsigned kept;

	/* The window's regions. */
	(void)server;
	kept = 0;
	for (index = 0; index < shell_titlebar.hit_count; index++) {
		if (shell_titlebar.hits[index].surface == object)
			continue;
		shell_titlebar.hits[kept] = shell_titlebar.hits[index];
		kept++;
	}

	/* The table keeps the others. */
	shell_titlebar.hit_count = kept;

	/* A press or a field on it. */
	if (shell_titlebar.pressed.surface == object)
		shell_titlebar.pressing = 0;
	if (shell_titlebar.field.surface == object)
		shell_titlebar.field.surface = NULL;

	/* Its logged layouts. */
	for (index = 0; index < SHELL_LOGGED; index++) {
		if (shell_titlebar.logged[index].surface == object)
			shell_titlebar.logged[index].surface = NULL;
	}
}

/*
 * Finds a window's presentation mode, its model and its keiland_titlebar_v1
 * (menu mode, and NULLs, for a window without one).
 */
static unsigned
shell_mode(
	struct zwl_object *surface,
	struct zwl_titlebar_model **model,
	struct zwl_object **titlebar)
{
	/* The window's model. */
	*model = zwl_titlebar_of_surface(surface, titlebar);
	if (*model == NULL)
		return ZWL_TITLEBAR_MENU;

	/* The mode it shows. */
	return (*model)->shown.mode;
}

/*
 * Lays out a state's controls in an area: each control's side, width and
 * whether it is shown, and the breadcrumb's width; reports how many items
 * there are, whether "..." is needed, and whether the search became a
 * button.
 */
static unsigned
shell_layout(
	struct zwl_server *server,
	const struct zwl_titlebar_state *state,
	unsigned docked,
	const struct zwl_menu_area *area,
	unsigned has_menu,
	struct shell_item *items,
	int32_t *crumb_width,
	unsigned *overflow,
	unsigned *compact_search)
{
	const struct zwl_titlebar_control *control;
	unsigned count;
	unsigned index;
	unsigned unit;
	unsigned hidden;
	unsigned victim;
	unsigned pass;
	unsigned crumb;
	int32_t size;
	int32_t needed;
	int32_t available;
	int32_t crumb_least;
	int32_t full;

	/* Each control, its side and its unit (a segmented group is one). */
	size = CONTROL_SIZE;
	if (docked != 0U)
		size = CONTROL_SIZE_DOCKED;
	unit = 0;
	crumb = SHELL_ITEMS;
	for (count = 0; count < state->control_count; count++) {
		control = &state->controls[count];
		items[count].control = control;
		items[count].side = shell_side(control);
		items[count].shown = 1;
		items[count].x = 0;
		if (count == 0U || control->group == 0U || control->group != state->controls[count - 1U].group)
			unit++;
		items[count].unit = unit;
		if (items[count].side == SIDE_CRUMB && crumb == SHELL_ITEMS)
			crumb = count;
	}

	/* The room, and the least the breadcrumb needs. */
	available = area->right - area->x;
	*compact_search = 0;
	crumb_least = 0;
	if (crumb != SHELL_ITEMS) {
		full = shell_crumb_full(server, items[crumb].control);
		crumb_least = full;
		if (crumb_least > CRUMB_LEAST)
			crumb_least = CRUMB_LEAST;
	}

	/* What is shown is measured again after each step of giving way. */
	for (pass = 0; pass < 2U * SHELL_ITEMS + 2U; pass++) {
		needed = 0;
		hidden = 0;
		for (index = 0; index < count; index++) {
			if (items[index].shown == 0U) {
				hidden = 1;
				continue;
			}

			/* The breadcrumb is measured on its own. */
			if (items[index].side == SIDE_CRUMB)
				continue;
			items[index].width = shell_item_width(server, items[index].control, size, *compact_search);
			needed += items[index].width + CONTROL_GAP;
		}

		/* The breadcrumb's least, the gaps between the sides, and "..." when it has something to hold. */
		if (crumb != SHELL_ITEMS && items[crumb].shown != 0U)
			needed += crumb_least + 2 * SIDE_GAP;
		*overflow = 0;
		if (has_menu != 0U || hidden != 0U) {
			*overflow = 1;
			needed += size + SIDE_GAP;
		}

		/* It fits. */
		if (needed <= available)
			break;

		/* The first step: the search field becomes a button (unless it has the keyboard). */
		if (*compact_search == 0U && shell_titlebar.field.surface == NULL) {
			*compact_search = 1;
			continue;
		}

		/* Then the last shown unit of the lowest priority there is, on the right; the breadcrumb after the normal ones. */
		victim = SHELL_ITEMS;
		for (index = count; index > 0U; index--) {
			control = items[index - 1U].control;
			if (items[index - 1U].shown == 0U || items[index - 1U].side != SIDE_RIGHT)
				continue;
			if (control->priority == ZWL_PRIORITY_SECONDARY) {
				victim = index - 1U;
				break;
			}
		}

		/* Else the last normal one. */
		for (index = count; victim == SHELL_ITEMS && index > 0U; index--) {
			control = items[index - 1U].control;
			if (items[index - 1U].shown == 0U || items[index - 1U].side != SIDE_RIGHT)
				continue;
			if (control->priority == ZWL_PRIORITY_NORMAL)
				victim = index - 1U;
		}

		/* Else the breadcrumb. */
		if (victim == SHELL_ITEMS && crumb != SHELL_ITEMS && items[crumb].shown != 0U)
			victim = crumb;

		/* Only the primary controls are left: they stay, whatever the room. */
		if (victim == SHELL_ITEMS)
			break;

		/* The victim's whole unit gives way. */
		unit = items[victim].unit;
		for (index = 0; index < count; index++) {
			if (items[index].unit == unit)
				items[index].shown = 0;
		}
	}

	/* The breadcrumb takes what is left. */
	*crumb_width = 0;
	if (crumb != SHELL_ITEMS && items[crumb].shown != 0U) {
		*crumb_width = available - needed + crumb_least;
		full = shell_crumb_full(server, items[crumb].control);
		if (*crumb_width > full)
			*crumb_width = full;
		if (*crumb_width < 0)
			*crumb_width = 0;
		items[crumb].width = *crumb_width;
	}

	/* Succeeded: the items. */
	return count;
}

/* Measures a control that is not a breadcrumb: a button's square, a segment's face, a field, a label. */
static int32_t
shell_item_width(
	struct zwl_server *server,
	const struct zwl_titlebar_control *control,
	int32_t size,
	unsigned compact_search)
{
	int32_t width;

	/* A search field, or its button. */
	if (control->role == ZWL_CONTROL_SEARCH) {
		if (compact_search != 0U)
			return size;
		return SEARCH_WIDTH;
	}

	/* A label's pill. */
	if (control->role == ZWL_CONTROL_GENERIC) {
		width = glass_text_width(server, SIZE_BAR, control->label) + 2 * LABEL_PADDING;
		return width;
	}

	/* Anything else is a square. */
	return size;
}

/* Tells which side a control goes on. */
static unsigned
shell_side(
	const struct zwl_titlebar_control *control)
{
	/* The navigation on the left, the breadcrumb between, the rest on the right. */
	switch (control->role) {
	case ZWL_CONTROL_BACK:
	case ZWL_CONTROL_FORWARD:
	case ZWL_CONTROL_UP:
	case ZWL_CONTROL_HOME:
		return SIDE_LEFT;
	case ZWL_CONTROL_BREADCRUMB:
		return SIDE_CRUMB;
	default:
		break;
	}

	/* The right. */
	return SIDE_RIGHT;
}

/* Measures a breadcrumb with all its parts. */
static int32_t
shell_crumb_full(
	struct zwl_server *server,
	const struct zwl_titlebar_control *control)
{
	unsigned part;
	int32_t width;

	/* Each part and the arrows between them. */
	width = 0;
	for (part = 0; part < control->segment_count; part++) {
		width += glass_text_width(server, SIZE_BAR, control->segments[part]) + 2 * CRUMB_PADDING;
		if (part + 1U < control->segment_count)
			width += CRUMB_ARROW;
	}

	/* Reports the width. */
	return width;
}

/* Draws a window's controls in an area, and records where they are. */
static void
shell_draw_controls(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct zwl_menu_area *area,
	const float *ink,
	float fade)
{
	static struct shell_item items[SHELL_ITEMS];
	const struct zwl_menu_item *tops[1];
	const struct zwl_titlebar_control *control;
	const struct zwl_menu_model *menu;
	const char *labels[SHELL_ITEMS];
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	struct zwl_object *place;
	uint32_t ids[SHELL_ITEMS];
	uint32_t checksum;
	unsigned has_menu;
	unsigned overflow;
	unsigned compact;
	unsigned recording;
	unsigned count;
	unsigned index;
	unsigned hidden;
	int32_t crumb_width;
	int32_t size;
	int32_t left;
	int32_t right;
	int32_t y;
	int32_t unit_start;
	int32_t unit_end;
	int32_t overflow_x;
	unsigned face;
	float pill[4];

	/* The model, and whether the window has a menu for "...". */
	(void)shell_mode(surface, &model, &titlebar);
	has_menu = 0;
	menu = zwl_menu_of_surface(surface, &place);
	if (menu != NULL)
		has_menu = zwl_menu_children(menu, ZWL_MENU_ROOT, tops, 1U);

	/*
	 * Places are recorded only where they are seen as they are: not while
	 * the desktop layer is moved or the window docks or fades.
	 */
	recording = 0;
	if (!server->layer_on && server->anim == NULL && fade >= 1.0f)
		recording = 1;

	/* The layout. */
	count = shell_layout(server, &model->shown, docked, area, has_menu, items, &crumb_width, &overflow, &compact);
	size = CONTROL_SIZE;
	if (docked != 0U)
		size = CONTROL_SIZE_DOCKED;
	y = area->top + (area->height - size) / 2;

	/* The left side, then the breadcrumb, from the area's start. */
	left = area->x;
	for (index = 0; index < count; index++) {
		if (items[index].shown == 0U || items[index].side != SIDE_LEFT)
			continue;
		items[index].x = left;
		left += items[index].width + CONTROL_GAP;
	}

	/* The breadcrumb after a wider gap. */
	left += SIDE_GAP - CONTROL_GAP;
	for (index = 0; index < count; index++) {
		if (items[index].shown == 0U || items[index].side != SIDE_CRUMB)
			continue;
		items[index].x = left;
		left += items[index].width + SIDE_GAP;
	}

	/* The right side, from the area's end ("..." last), in their order. */
	right = area->right;
	if (overflow != 0U)
		right -= size + SIDE_GAP;
	for (index = count; index > 0U; index--) {
		if (items[index - 1U].shown == 0U || items[index - 1U].side != SIDE_RIGHT)
			continue;
		right -= items[index - 1U].width;
		items[index - 1U].x = right;
		right -= CONTROL_GAP;
	}

	/* Each shown control. */
	checksum = shell_mix(2166136261U, docked);
	for (index = 0; index < count; index++) {
		control = items[index].control;
		checksum = shell_mix(checksum, control->id);
		checksum = shell_mix(checksum, items[index].shown);
		if (items[index].shown == 0U)
			continue;
		checksum = shell_mix(checksum, (uint32_t)(items[index].x - area->origin));
		checksum = shell_mix(checksum, (uint32_t)items[index].width);

		/* A segmented group's pill lies under all its faces, drawn with the first. */
		if (control->group != 0U && (index == 0U || items[index - 1U].unit != items[index].unit)) {
			unit_start = items[index].x;
			unit_end = unit_start + items[index].width;
			for (face = index + 1U; face < count && items[face].unit == items[index].unit; face++)
				unit_end = items[face].x + items[face].width;
			pill[0] = 1.0f;
			pill[1] = 1.0f;
			pill[2] = 1.0f;
			pill[3] = 0.35f * fade;
			glass_draw_solid(server, command, (float)(unit_start - GROUP_PADDING), (float)(y - GROUP_PADDING), (float)(unit_end - unit_start + 2 * GROUP_PADDING), (float)(size + 2 * GROUP_PADDING), (float)(size / 2 + GROUP_PADDING), pill);
		}

		/* Each kind of control. */
		if (control->role == ZWL_CONTROL_SEARCH && compact == 0U) {
			shell_draw_search(server, command, surface, docked, control, items[index].x, y, items[index].width, size, ink, fade, recording);
		} else if (control->role == ZWL_CONTROL_BREADCRUMB) {
			shell_draw_crumbs(server, command, surface, docked, control, items[index].x, y, items[index].width, size, ink, fade, recording);
		} else if (control->role == ZWL_CONTROL_PROGRESS) {
			shell_draw_progress(server, command, control, items[index].x, y, size, fade);
			if (recording != 0U)
				shell_add_hit(surface, docked, KIND_BUTTON, control->id, 0U, items[index].x, y, size, size);
		} else if (control->group != 0U) {
			shell_draw_button(server, command, surface, docked, control, items[index].x, y, size, size, KIND_FACE, ink, fade, recording);
		} else {
			shell_draw_button(server, command, surface, docked, control, items[index].x, y, items[index].width, size, KIND_BUTTON, ink, fade, recording);
		}
	}

	/* "..." at the right end: the menus' overflow, holding the hidden controls and the window's menu (menu-shell.c). */
	overflow_x = -1;
	hidden = 0;
	for (index = 0; index < count; index++) {
		if (items[index].shown != 0U)
			continue;
		ids[hidden] = items[index].control->id;
		labels[hidden] = items[index].control->label;
		hidden++;
	}

	/* "..." itself, when it holds something. */
	if (overflow != 0U) {
		overflow_x = area->right - size;
		shell_draw_button(server, command, surface, docked, NULL, overflow_x, y, size, size, KIND_BUTTON, ink, fade, 0U);
		if (recording != 0U)
			zwl_menu_add_overflow(surface, docked, area, area->right - size, size, ids, labels, hidden);
		checksum = shell_mix(checksum, 0xffffffffU);
	}

	/* A new layout is logged for the tests that click the controls. */
	if (recording != 0U)
		shell_log_layout(surface, docked, items, count, y, size, overflow_x, checksum);
}

/*
 * Draws a round button (or a segmented group's face, or "..." for no
 * control) with its icon or label: lit under the pointer, darker while
 * pressed, a white knob when checked, pale when disabled; records it.
 */
static void
shell_draw_button(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct zwl_titlebar_control *control,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t size,
	unsigned kind,
	const float *ink,
	float fade,
	unsigned recording)
{
	static const float accent[4] = { 0.18f, 0.49f, 0.96f, 1.0f };
	float ground[4];
	float colour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	unsigned icon;
	unsigned enabled;
	unsigned checked;
	int32_t label_width;
	int hovered;

	/* The control's state ("..." is always usable). */
	enabled = 1;
	checked = 0;
	icon = GLASS_ICON_OVERFLOW;
	if (control != NULL) {
		enabled = control->enabled;
		checked = control->checked;
		icon = shell_icon(control->role);
	}

	/* A checked control sits on a white knob. */
	if (checked != 0U) {
		ground[0] = 1.0f;
		ground[1] = 1.0f;
		ground[2] = 1.0f;
		ground[3] = 0.95f * fade;
		glass_draw_solid(server, command, (float)x + 1.0f, (float)y + 1.0f, (float)width - 2.0f, (float)size - 2.0f, (float)(size / 2 - 1), ground);
	}

	/* Under the pointer it is lit, and darker while pressed. */
	hovered = shell_hovered(server, x, y, width, size);
	if (enabled != 0U && hovered != 0 && checked == 0U) {
		ground[0] = 1.0f;
		ground[1] = 1.0f;
		ground[2] = 1.0f;
		ground[3] = 0.62f * fade;
		if (shell_titlebar.pressing != 0U && control != NULL && shell_titlebar.pressed.id == control->id)
			ground[3] = 0.9f * fade;
		glass_draw_solid(server, command, (float)x, (float)y, (float)width, (float)size, (float)(size / 2), ground);
	}

	/* The icon (the accent when checked), or a label, pale when it does nothing. */
	shell_colour(colour, ink, fade);
	if (checked != 0U)
		shell_colour(colour, accent, fade);
	if (enabled == 0U)
		colour[3] *= 0.35f;
	if (control != NULL && (control->role == ZWL_CONTROL_GENERIC || control->role == ZWL_CONTROL_PRIMARY_ACTION) && width > size) {
		label_width = glass_text_width(server, SIZE_BAR, control->label);
		glass_draw_text(server, command, SIZE_BAR, x + (width - label_width) / 2, y + size / 2 + 5, control->label, width, colour);
	} else {
		glass_draw_icon(server, command, icon, x + (width - (int32_t)ICON_PIXELS) / 2, y + (size - (int32_t)ICON_PIXELS) / 2, ICON_PIXELS, colour);
	}

	/* A control (not "...", which the menus record) can be pressed. */
	if (recording != 0U && control != NULL)
		shell_add_hit(surface, docked, kind, control->id, 0U, x, y, width, size);
}

/*
 * Draws a search field: a rounded field with a magnifier and its text (the
 * placeholder when empty), with the accent edge and the cursor while it has
 * the keyboard; records it.
 */
static void
shell_draw_search(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct zwl_titlebar_control *control,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t size,
	const float *ink,
	float fade,
	unsigned recording)
{
	static const float accent[4] = { 0.18f, 0.49f, 0.96f, 1.0f };
	float ground[4];
	float colour[4];
	const char *text;
	int32_t baseline;
	int32_t height;
	int32_t top;
	unsigned focused;

	/* The field is a little lower than a button, in the middle. */
	height = size - 4;
	top = y + 2;
	baseline = top + height / 2 + 5;

	/* Whether it has the keyboard. */
	focused = 0;
	if (shell_titlebar.field.surface == surface && shell_titlebar.field.id == control->id)
		focused = 1;

	/* The accent edge while it has the keyboard, and the field's ground. */
	if (focused != 0U) {
		shell_colour(colour, accent, fade);
		colour[3] *= 0.8f;
		glass_draw_solid(server, command, (float)x - 1.5f, (float)top - 1.5f, (float)width + 3.0f, (float)height + 3.0f, (float)(height / 2) + 1.5f, colour);
	}

	/* The ground, whiter while it has the keyboard. */
	ground[0] = 1.0f;
	ground[1] = 1.0f;
	ground[2] = 1.0f;
	ground[3] = 0.55f * fade;
	if (focused != 0U)
		ground[3] = 0.95f * fade;
	glass_draw_solid(server, command, (float)x, (float)top, (float)width, (float)height, (float)(height / 2), ground);

	/* The magnifier. */
	shell_colour(colour, ink, fade);
	colour[3] *= 0.7f;
	glass_draw_icon(server, command, GLASS_ICON_SEARCH, x + 8, top + (height - (int32_t)ICON_PIXELS) / 2, ICON_PIXELS, colour);

	/* The text being typed, with its cursor. */
	if (focused != 0U) {
		shell_draw_field_text(server, command, x + 30, baseline, width - 40, ink, fade);
	} else {
		/* The client's text, or the placeholder, paler. */
		text = control->text;
		shell_colour(colour, ink, fade);
		if (text == NULL || text[0] == '\0') {
			text = control->placeholder;
			colour[3] *= 0.45f;
		}

		/* The text, when there is one. */
		if (text != NULL)
			glass_draw_text(server, command, SIZE_BAR, x + 30, baseline, text, width - 40, colour);
	}

	/* It can be pressed. */
	if (recording != 0U)
		shell_add_hit(surface, docked, KIND_FIELD, control->id, 0U, x, y, width, size);
}

/*
 * Draws a breadcrumb in its width: as many of its last parts as fit (the
 * leading ones as "..."), arrows between them, the last one darker, each
 * lit under the pointer; or, while its path is edited, a field.  Records the
 * parts.
 */
static void
shell_draw_crumbs(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct zwl_titlebar_control *control,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t size,
	const float *ink,
	float fade,
	unsigned recording)
{
	static const char ellipsis[] = "\xe2\x80\xa6";
	int32_t widths[ZWL_TITLEBAR_SEGMENTS_MAX];
	float ground[4];
	float colour[4];
	unsigned first;
	unsigned part;
	int32_t total;
	int32_t more;
	int32_t pen;
	int32_t baseline;
	int hovered;

	/* A breadcrumb being edited is a field in its place. */
	baseline = y + size / 2 + 5;
	if (shell_titlebar.field.surface == surface && shell_titlebar.field.id == control->id) {
		ground[0] = 1.0f;
		ground[1] = 1.0f;
		ground[2] = 1.0f;
		ground[3] = 0.95f * fade;
		glass_draw_solid(server, command, (float)x, (float)y + 2.0f, (float)width, (float)size - 4.0f, 8.0f, ground);
		shell_draw_field_text(server, command, x + 10, baseline, width - 20, ink, fade);
		if (recording != 0U)
			shell_add_hit(surface, docked, KIND_FIELD, control->id, 0U, x, y, width, size);
		return;
	}

	/* Nothing to show. */
	if (control->segment_count == 0U || width <= 0)
		return;

	/* The parts' widths, and as many from the last as fit (with "..." before them when not all do). */
	for (part = 0; part < control->segment_count; part++)
		widths[part] = glass_text_width(server, SIZE_BAR, control->segments[part]) + 2 * CRUMB_PADDING;
	more = glass_text_width(server, SIZE_BAR, ellipsis) + CRUMB_PADDING + CRUMB_ARROW;
	first = control->segment_count - 1U;
	total = widths[first];
	while (first > 0U) {
		if (total + CRUMB_ARROW + widths[first - 1U] + more > width && first > 1U)
			break;
		if (first == 1U && total + CRUMB_ARROW + widths[0] > width)
			break;
		first--;
		total += CRUMB_ARROW + widths[first];
	}

	/* "..." for the parts left out; it goes to the nearest of them (lit when a drag and drop is over it). */
	pen = x;
	if (first > 0U) {
		shell_draw_drop_part(server, command, surface, control->id, first - 1U, pen, y, more - CRUMB_ARROW, size, fade);
		shell_colour(colour, ink, fade);
		colour[3] *= 0.6f;
		glass_draw_text(server, command, SIZE_BAR, pen + CRUMB_PADDING / 2, baseline, ellipsis, more, colour);
		if (recording != 0U)
			shell_add_hit(surface, docked, KIND_CRUMB, control->id, first - 1U, pen, y, more - CRUMB_ARROW, size);
		pen += more - CRUMB_ARROW;
		glass_draw_glyph(server, command, SIZE_BAR, GLASS_ARROW_GLYPH, pen + 2, baseline, colour);
		pen += CRUMB_ARROW;
	}

	/* Each part: lit under the pointer, the last one dark, the others quieter, an arrow after each but the last. */
	for (part = first; part < control->segment_count; part++) {
		hovered = shell_hovered(server, pen, y, widths[part], size);
		if (hovered != 0 && control->enabled != 0U) {
			ground[0] = 1.0f;
			ground[1] = 1.0f;
			ground[2] = 1.0f;
			ground[3] = 0.62f * fade;
			glass_draw_solid(server, command, (float)pen, (float)y + 2.0f, (float)widths[part], (float)size - 4.0f, 8.0f, ground);
		}

		/* The part a drag and drop is over, lit as a target. */
		shell_draw_drop_part(server, command, surface, control->id, part, pen, y, widths[part], size, fade);

		/* The label. */
		shell_colour(colour, ink, fade);
		if (part + 1U < control->segment_count)
			colour[3] *= 0.65f;
		glass_draw_text(server, command, SIZE_BAR, pen + CRUMB_PADDING, baseline, control->segments[part], x + width - pen - CRUMB_PADDING, colour);
		if (recording != 0U)
			shell_add_hit(surface, docked, KIND_CRUMB, control->id, part, pen, y, widths[part], size);
		pen += widths[part];

		/* The arrow before the next part. */
		if (part + 1U < control->segment_count) {
			shell_colour(colour, ink, fade);
			colour[3] *= 0.4f;
			glass_draw_glyph(server, command, SIZE_BAR, GLASS_ARROW_GLYPH, pen + 2, baseline, colour);
			pen += CRUMB_ARROW;
		}
	}
}

/* Draws the field's text from x on a baseline within a width: the selection's tint, the text and the cursor. */
static void
shell_draw_field_text(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t x,
	int32_t baseline,
	int32_t width,
	const float *ink,
	float fade)
{
	static const float selection[4] = { 0.18f, 0.49f, 0.96f, 0.25f };
	struct shell_field *field;
	char before[ZWL_TITLEBAR_TEXT_MAX + 1U];
	float colour[4];
	size_t start;
	size_t end;
	int32_t start_x;
	int32_t end_x;
	int32_t cursor_x;

	/* The selection's ends in order. */
	field = &shell_titlebar.field;
	start = field->anchor;
	end = field->cursor;
	if (start > end) {
		start = field->cursor;
		end = field->anchor;
	}

	/* Where the selection's ends and the cursor are: the widths of the text before them. */
	memcpy(before, field->text, start);
	before[start] = '\0';
	start_x = glass_text_width(server, SIZE_BAR, before);
	memcpy(before, field->text, end);
	before[end] = '\0';
	end_x = glass_text_width(server, SIZE_BAR, before);
	memcpy(before, field->text, field->cursor);
	before[field->cursor] = '\0';
	cursor_x = glass_text_width(server, SIZE_BAR, before);

	/* The selection's tint. */
	if (end > start) {
		memcpy(colour, selection, sizeof(colour));
		colour[3] *= fade;
		glass_draw_solid(server, command, (float)(x + start_x), (float)(baseline - 14), (float)(end_x - start_x), 18.0f, 2.0f, colour);
	}

	/* The text, then the cursor. */
	shell_colour(colour, ink, fade);
	glass_draw_text(server, command, SIZE_BAR, x, baseline, field->text, width, colour);
	if (cursor_x <= width)
		glass_draw_solid(server, command, (float)(x + cursor_x), (float)(baseline - 13), 1.5f, 16.0f, 0.0f, colour);
}

/* Draws a progress control: a short bar with the share done in the accent (a third of it when the share is not known). */
static void
shell_draw_progress(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct zwl_titlebar_control *control,
	int32_t x,
	int32_t y,
	int32_t size,
	float fade)
{
	static const float track[4] = { 0.12f, 0.16f, 0.24f, 0.14f };
	static const float accent[4] = { 0.18f, 0.49f, 0.96f, 1.0f };
	float colour[4];
	float share;

	/* The share done. */
	share = (float)control->value / 1000.0f;
	if (control->value > 1000U)
		share = 0.33f;

	/* The track and the share over it. */
	memcpy(colour, track, sizeof(colour));
	colour[3] *= fade;
	glass_draw_solid(server, command, (float)x + 4.0f, (float)(y + size / 2 - 3), (float)size - 8.0f, 6.0f, 3.0f, colour);
	memcpy(colour, accent, sizeof(colour));
	colour[3] *= fade;
	glass_draw_solid(server, command, (float)x + 4.0f, (float)(y + size / 2 - 3), ((float)size - 8.0f) * share, 6.0f, 3.0f, colour);
}

/*
 * Lays out a window's tabs in an area: each tab's width and whether it is
 * shown, and the strip's arrows, "+" and "...".  The tabs get the width
 * they would like when all fit, the same narrower width (not below the
 * least) when they do not, and otherwise the strip scrolls: as many least
 * tabs as fit, from the first the arrows chose, brought to the active tab
 * once after each commit.  Returns how many tabs there are.
 */
static unsigned
shell_tabs_layout(
	struct zwl_server *server,
	struct zwl_titlebar_model *model,
	const struct zwl_menu_area *area,
	int32_t size,
	unsigned has_menu,
	struct shell_tab *tabs,
	struct shell_strip *strip)
{
	const struct zwl_titlebar_state *state;
	const struct zwl_titlebar_tab *tab;
	unsigned count;
	unsigned index;
	unsigned active;
	int32_t room;
	int32_t total;
	int32_t width;
	int32_t x;

	/* Each tab and the width it would like. */
	state = &model->shown;
	count = state->tab_count;
	active = count;
	total = 0;
	for (index = 0; index < count; index++) {
		tab = &state->tabs[index];
		width = shell_tab_natural(server, tab);
		tabs[index].tab = tab;
		tabs[index].natural = width;
		tabs[index].shown = 1;
		tabs[index].width = width;
		tabs[index].x = 0;
		total += width + TAB_GAP;
		if ((tab->flags & ZWL_TAB_ACTIVE) != 0U && active == count)
			active = index;
	}

	/* The room for the tabs: the area without "+" and "..." (when the window has a menu). */
	memset(strip, 0, sizeof(*strip));
	strip->shown = count;
	room = area->right - area->x;
	if ((state->options & ZWL_TABS_NEW_BUTTON) != 0U) {
		strip->new_button = 1;
		room -= size + TAB_GAP;
	}

	/* "..." for the window's menu. */
	if (has_menu != 0U) {
		strip->overflow = 1;
		room -= size + SIDE_GAP;
	}

	/* All fit as they would like. */
	strip->start = area->x;
	if (total <= room + TAB_GAP)
		return count;

	/* All fit narrower, the same width each (none wider than it would like). */
	if (count > 0U && (int32_t)count * (TAB_LEAST + TAB_GAP) <= room + TAB_GAP) {
		width = (room + TAB_GAP) / (int32_t)count - TAB_GAP;
		for (index = 0; index < count; index++) {
			if (tabs[index].width > width)
				tabs[index].width = width;
		}

		/* The narrowed tabs. */
		return count;
	}

	/* Otherwise the strip scrolls between its arrows, and "..." holds the tabs out of sight. */
	strip->scrolling = 1;
	if (strip->overflow == 0U) {
		strip->overflow = 1;
		room -= size + SIDE_GAP;
	}

	/* The arrows at both ends, and as many least tabs as fit between them. */
	room -= 2 * (size + TAB_GAP);
	strip->start = area->x + size + TAB_GAP;
	strip->shown = 1;
	if (room > TAB_LEAST)
		strip->shown = (unsigned)((room + TAB_GAP) / (TAB_LEAST + TAB_GAP));
	if (strip->shown > count)
		strip->shown = count;

	/* The active tab comes into sight once after each commit; the arrows choose otherwise. */
	if (model->tab_seen != model->generation && active < count) {
		if (active < model->tab_first)
			model->tab_first = active;
		if (active >= model->tab_first + strip->shown)
			model->tab_first = active + 1U - strip->shown;
	}

	/* The commit is seen, and the strip does not scroll past its last tab. */
	model->tab_seen = model->generation;
	if (model->tab_first + strip->shown > count)
		model->tab_first = count - strip->shown;
	strip->first = model->tab_first;

	/* The tabs in sight share the room; the others are hidden. */
	width = TAB_LEAST;
	if (strip->shown > 0U)
		width = (room + TAB_GAP) / (int32_t)strip->shown - TAB_GAP;
	x = strip->start;
	for (index = 0; index < count; index++) {
		tabs[index].shown = 0;
		if (index < strip->first || index >= strip->first + strip->shown)
			continue;
		tabs[index].shown = 1;
		tabs[index].width = width;
		if (tabs[index].natural < width)
			tabs[index].width = tabs[index].natural;
		tabs[index].x = x;
		x += tabs[index].width + TAB_GAP;
	}

	/* The tabs of the scrolled strip. */
	return count;
}

/*
 * Tells the width a tab would like: its title, its padding, its close
 * button and its dot, within the widest and the least a tab is.
 */
static int32_t
shell_tab_natural(
	struct zwl_server *server,
	const struct zwl_titlebar_tab *tab)
{
	int32_t width;

	/* The title and its padding. */
	width = glass_text_width(server, SIZE_BAR, tab->title) + 2 * TAB_PADDING;

	/* The close button's room, and the dot's. */
	if ((tab->flags & ZWL_TAB_CLOSABLE) != 0U)
		width += TAB_CLOSE;
	if ((tab->flags & ZWL_TAB_ATTENTION) != 0U)
		width += TAB_ATTENTION;

	/* Within the widest and the least. */
	if (width > TAB_MOST)
		width = TAB_MOST;
	if (width < TAB_LEAST)
		width = TAB_LEAST;
	return width;
}

/*
 * Tells the room a window's tab strip would like: each tab as wide as it
 * would like, "+" when the client asks for it, and "..." for a window
 * with a menu (a floating titlebar's buttons, the larger of the two).
 */
static int32_t
shell_tabs_need(
	struct zwl_server *server,
	struct zwl_object *surface,
	const struct zwl_titlebar_model *model)
{
	const struct zwl_menu_item *tops[1];
	const struct zwl_menu_model *menu;
	struct zwl_object *place;
	unsigned has_menu;
	unsigned index;
	int32_t need;

	/* Each tab and the gap after it. */
	need = 0;
	for (index = 0; index < model->shown.tab_count; index++)
		need += shell_tab_natural(server, &model->shown.tabs[index]) + TAB_GAP;

	/* "+". */
	if ((model->shown.options & ZWL_TABS_NEW_BUTTON) != 0U)
		need += CONTROL_SIZE + TAB_GAP;

	/* "..." for the window's menu. */
	has_menu = 0;
	menu = zwl_menu_of_surface(surface, &place);
	if (menu != NULL)
		has_menu = zwl_menu_children(menu, ZWL_MENU_ROOT, tops, 1U);
	if (has_menu != 0U)
		need += CONTROL_SIZE + SIDE_GAP;
	return need;
}

/* Draws a window's tabs in an area, with the strip's arrows, "+" and "...", and records where they are. */
static void
shell_draw_tabs(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct zwl_menu_area *area,
	const float *ink,
	float fade)
{
	static struct shell_tab tabs[SHELL_TABS];
	static int32_t closes[SHELL_TABS];
	static uint32_t ids[SHELL_TABS];
	static const char *labels[SHELL_TABS];
	struct zwl_titlebar_control button;
	const struct zwl_menu_item *tops[1];
	const struct zwl_menu_model *menu;
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	struct zwl_object *place;
	struct shell_strip strip;
	uint32_t checksum;
	unsigned has_menu;
	unsigned recording;
	unsigned count;
	unsigned index;
	unsigned hidden;
	int32_t buttons[4];
	int32_t size;
	int32_t right;
	int32_t x;
	int32_t y;

	/* The model, and whether the window has a menu for "...". */
	(void)shell_mode(surface, &model, &titlebar);
	has_menu = 0;
	menu = zwl_menu_of_surface(surface, &place);
	if (menu != NULL)
		has_menu = zwl_menu_children(menu, ZWL_MENU_ROOT, tops, 1U);

	/* Places are recorded only where they are seen as they are (as for the controls). */
	recording = 0;
	if (!server->layer_on && server->anim == NULL && fade >= 1.0f)
		recording = 1;

	/* The layout. */
	size = CONTROL_SIZE;
	if (docked != 0U)
		size = CONTROL_SIZE_DOCKED;
	y = area->top + (area->height - size) / 2;
	count = shell_tabs_layout(server, model, area, size, has_menu, tabs, &strip);

	/* The tabs one after another from the strip's start (a scrolled strip placed them already). */
	x = strip.start;
	for (index = 0; index < count && strip.scrolling == 0U; index++) {
		tabs[index].x = x;
		x += tabs[index].width + TAB_GAP;
	}

	/* Each tab in sight. */
	checksum = shell_mix(2166136261U, docked);
	checksum = shell_mix(checksum, 0x7ab5U);
	for (index = 0; index < count; index++) {
		closes[index] = -1;
		checksum = shell_mix(checksum, tabs[index].tab->id);
		checksum = shell_mix(checksum, tabs[index].shown);
		if (tabs[index].shown == 0U)
			continue;
		shell_draw_tab(server, command, surface, docked, &tabs[index], y, size, ink, fade, recording, &closes[index]);
		checksum = shell_mix(checksum, (uint32_t)(tabs[index].x - area->origin));
		checksum = shell_mix(checksum, (uint32_t)tabs[index].width);
		checksum = shell_mix(checksum, tabs[index].tab->flags);
		checksum = shell_mix(checksum, (uint32_t)(closes[index] + 1));
	}

	/* The buttons drawn as controls without events of their own: an arrow, "+". */
	memset(&button, 0, sizeof(button));
	button.enabled = 1;
	buttons[0] = -1;
	buttons[1] = -1;
	buttons[2] = -1;
	buttons[3] = -1;

	/* The arrows of a scrolled strip, pale at its ends. */
	if (strip.scrolling != 0U) {
		button.role = ZWL_CONTROL_BACK;
		button.enabled = 0;
		if (strip.first > 0U)
			button.enabled = 1;
		buttons[0] = area->x;
		shell_draw_button(server, command, surface, docked, &button, buttons[0], y, size, size, KIND_SCROLL, ink, fade, 0U);
		if (recording != 0U && button.enabled != 0U)
			shell_add_hit(surface, docked, KIND_SCROLL, 0U, 0U, buttons[0], y, size, size);

		/* The right arrow after the last tab's room. */
		button.role = ZWL_CONTROL_FORWARD;
		button.enabled = 0;
		if (strip.first + strip.shown < count)
			button.enabled = 1;
		right = area->right;
		if (strip.overflow != 0U)
			right -= size + SIDE_GAP;
		if (strip.new_button != 0U)
			right -= size + TAB_GAP;
		buttons[1] = right - size;
		shell_draw_button(server, command, surface, docked, &button, buttons[1], y, size, size, KIND_SCROLL, ink, fade, 0U);
		if (recording != 0U && button.enabled != 0U)
			shell_add_hit(surface, docked, KIND_SCROLL, 0U, 1U, buttons[1], y, size, size);
		checksum = shell_mix(checksum, strip.first);
	}

	/* "+" after the tabs (after the right arrow of a scrolled strip). */
	if (strip.new_button != 0U) {
		button.role = ZWL_CONTROL_PRIMARY_ACTION;
		button.enabled = 1;
		buttons[2] = x;
		if (strip.scrolling != 0U)
			buttons[2] = buttons[1] + size + TAB_GAP;
		shell_draw_button(server, command, surface, docked, &button, buttons[2], y, size, size, KIND_NEW_TAB, ink, fade, 0U);
		if (recording != 0U)
			shell_add_hit(surface, docked, KIND_NEW_TAB, 0U, 0U, buttons[2], y, size, size);
		checksum = shell_mix(checksum, (uint32_t)(buttons[2] - area->origin));
	}

	/* The tabs out of sight, for "...". */
	hidden = 0;
	for (index = 0; index < count; index++) {
		if (tabs[index].shown != 0U)
			continue;
		ids[hidden] = tabs[index].tab->id;
		labels[hidden] = tabs[index].tab->title;
		hidden++;
	}

	/* "..." at the right end: the tabs out of sight, then the window's menu (menu-shell.c). */
	if (strip.overflow != 0U) {
		buttons[3] = area->right - size;
		shell_draw_button(server, command, surface, docked, NULL, buttons[3], y, size, size, KIND_BUTTON, ink, fade, 0U);
		if (recording != 0U)
			zwl_menu_add_overflow(surface, docked, area, buttons[3], size, ids, labels, hidden);
		checksum = shell_mix(checksum, 0xffffffffU);
	}

	/* A new layout is logged for the tests that click the tabs. */
	if (recording != 0U)
		shell_log_strip(surface, docked, tabs, count, closes, y, size, buttons, checksum);
}

/*
 * Draws one tab: a white face with a fine edge when active, otherwise a
 * faint wash of the ink (seen on the white system bar as on the glass)
 * that deepens under the pointer, a dot when it wants attention, its title
 * (cut in the middle), and its close button when it is closable and active
 * or under the pointer; records it and its close button (where that is
 * drawn).
 */
static void
shell_draw_tab(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	unsigned docked,
	const struct shell_tab *tab,
	int32_t y,
	int32_t size,
	const float *ink,
	float fade,
	unsigned recording,
	int32_t *close_x)
{
	static const float accent[4] = { 0.18f, 0.49f, 0.96f, 1.0f };
	float ground[4];
	float colour[4];
	unsigned active;
	unsigned closable;
	unsigned shows_close;
	int32_t text_x;
	int32_t text_room;
	int32_t close;
	int hovered;
	int over_close;

	/* The tab's state, and whether the pointer is on it. */
	active = 0;
	if ((tab->tab->flags & ZWL_TAB_ACTIVE) != 0U)
		active = 1;
	closable = 0;
	if ((tab->tab->flags & ZWL_TAB_CLOSABLE) != 0U)
		closable = 1;
	hovered = shell_hovered(server, tab->x, y, tab->width, size);
	shows_close = 0;
	if (closable != 0U && (active != 0U || hovered != 0))
		shows_close = 1;

	/* An inactive tab's face: a faint wash of the ink, deeper under the pointer. */
	if (active == 0U) {
		shell_colour(ground, ink, 0.07f * fade);
		if (hovered != 0)
			shell_colour(ground, ink, 0.13f * fade);
		glass_draw_solid(server, command, (float)tab->x, (float)y, (float)tab->width, (float)size, (float)(size / 2), ground);
	}

	/* The active tab's: a fine edge of the ink, and white within it. */
	if (active != 0U) {
		shell_colour(ground, ink, 0.16f * fade);
		glass_draw_solid(server, command, (float)(tab->x - 1), (float)(y - 1), (float)(tab->width + 2), (float)(size + 2), (float)(size / 2 + 1), ground);
		ground[0] = 1.0f;
		ground[1] = 1.0f;
		ground[2] = 1.0f;
		ground[3] = 0.97f * fade;
		glass_draw_solid(server, command, (float)tab->x, (float)y, (float)tab->width, (float)size, (float)(size / 2), ground);
	}

	/* A dot before the title of a tab that wants attention. */
	text_x = tab->x + TAB_PADDING;
	if ((tab->tab->flags & ZWL_TAB_ATTENTION) != 0U) {
		memcpy(colour, accent, sizeof(colour));
		colour[3] *= fade;
		glass_draw_solid(server, command, (float)text_x, (float)(y + size / 2 - 3), 6.0f, 6.0f, 3.0f, colour);
		text_x += TAB_ATTENTION;
	}

	/* The title, cut in the middle (names often differ only at their end), before the close button when it is drawn. */
	text_room = tab->x + tab->width - TAB_PADDING - text_x;
	if (shows_close != 0U)
		text_room -= TAB_CLOSE - TAB_PADDING / 2;
	shell_colour(colour, ink, fade);
	if (active == 0U)
		colour[3] *= 0.75f;
	glass_draw_text_middle(server, command, SIZE_BAR, text_x, y + size / 2 + 5, tab->tab->title, text_room, colour);

	/* The tab can be pressed. */
	if (recording != 0U)
		shell_add_hit(surface, docked, KIND_TAB, tab->tab->id, 0U, tab->x, y, tab->width, size);

	/* The close button of a closable tab that is active or under the pointer, recorded over the tab. */
	*close_x = -1;
	if (shows_close == 0U)
		return;
	close = tab->x + tab->width - TAB_CLOSE - 2;
	over_close = shell_hovered(server, close, y + (size - TAB_CLOSE) / 2, TAB_CLOSE, TAB_CLOSE);
	if (over_close != 0) {
		ground[3] = 0.9f * fade;
		ground[0] = 0.85f;
		ground[1] = 0.87f;
		ground[2] = 0.9f;
		glass_draw_solid(server, command, (float)close, (float)(y + (size - TAB_CLOSE) / 2), (float)TAB_CLOSE, (float)TAB_CLOSE, (float)(TAB_CLOSE / 2), ground);
	}

	/* The cross, recorded over the tab. */
	shell_colour(colour, ink, fade);
	glass_draw_icon(server, command, GLASS_ICON_CLOSE, close + (TAB_CLOSE - 12) / 2, y + (size - 12) / 2, 12U, colour);
	if (recording != 0U)
		shell_add_hit(surface, docked, KIND_TAB_CLOSE, tab->tab->id, 0U, close, y + (size - TAB_CLOSE) / 2, TAB_CLOSE, TAB_CLOSE);
	*close_x = close;
}

/* Carries out a released region of a tab strip: a tab, its close button, "+", or an arrow. */
static void
shell_act_tabs(
	struct zwl_server *server,
	const struct shell_hit *hit,
	struct zwl_titlebar_model *model,
	struct zwl_object *titlebar)
{
	/* Each region's event, or the arrows' scroll. */
	switch (hit->kind) {
	case KIND_TAB:
		zwl_titlebar_send_tab(titlebar, hit->id, ZWL_TAB_EVENT_ACTIVATED);
		break;
	case KIND_TAB_CLOSE:
		zwl_titlebar_send_tab(titlebar, hit->id, ZWL_TAB_EVENT_CLOSE);
		break;
	case KIND_NEW_TAB:
		zwl_titlebar_send_tab(titlebar, 0U, ZWL_TAB_EVENT_NEW);
		break;
	case KIND_SCROLL:
		/* The strip moves one tab (the layout keeps it within the tabs). */
		if (hit->detail == 0U && model->tab_first > 0U)
			model->tab_first--;
		if (hit->detail != 0U)
			model->tab_first++;
		printf("ZWL TITLEBAR strip scroll client=%llu surface=%u first=%u\n", (unsigned long long)hit->surface->client->number, hit->surface->id, model->tab_first);
		break;
	default:
		break;
	}

	/* A new frame shows it. */
	server->dirty = 1;
}

/* Returns the icon of a role. */
static unsigned
shell_icon(
	uint32_t role)
{
	/* Each role's icon. */
	switch (role) {
	case ZWL_CONTROL_BACK:
		return GLASS_ICON_BACK;
	case ZWL_CONTROL_FORWARD:
		return GLASS_ICON_FORWARD;
	case ZWL_CONTROL_UP:
		return GLASS_ICON_UP;
	case ZWL_CONTROL_HOME:
		return GLASS_ICON_HOME;
	case ZWL_CONTROL_SEARCH:
		return GLASS_ICON_SEARCH;
	case ZWL_CONTROL_VIEW_GRID:
		return GLASS_ICON_GRID;
	case ZWL_CONTROL_VIEW_LIST:
		return GLASS_ICON_LIST;
	case ZWL_CONTROL_VIEW_COLUMNS:
		return GLASS_ICON_COLUMNS;
	case ZWL_CONTROL_SORT:
		return GLASS_ICON_SORT;
	case ZWL_CONTROL_FILTER:
		return GLASS_ICON_FILTER;
	case ZWL_CONTROL_SIDEBAR:
		return GLASS_ICON_SIDEBAR;
	case ZWL_CONTROL_PREVIEW:
		return GLASS_ICON_PREVIEW;
	case ZWL_CONTROL_PRIMARY_ACTION:
		return GLASS_ICON_PLUS;
	default:
		break;
	}

	/* Any other role. */
	return GLASS_ICON_OVERFLOW;
}

/* Tells whether the pointer is over a rectangle (and no menu is open). */
static int
shell_hovered(
	struct zwl_server *server,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height)
{
	/* Outside it. */
	if (server->pointer_x < x || server->pointer_x >= x + width)
		return 0;
	if (server->pointer_y < y || server->pointer_y >= y + height)
		return 0;

	/* Inside. */
	return 1;
}

/*
 * Lights the part of a breadcrumb a drag and drop is over (data.c) as a
 * target: a pale blue face with a blue edge.  Other parts are left alone.
 */
static void
shell_draw_drop_part(
	struct zwl_server *server,
	VkCommandBuffer command,
	struct zwl_object *surface,
	uint32_t id,
	uint32_t detail,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t size,
	float fade)
{
	static const float edge[4] = { 0.18f, 0.49f, 0.96f, 0.9f };
	static const float face[4] = { 0.86f, 0.92f, 1.0f, 0.95f };
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	float colour[4];

	/* Only the part the drag is over, of this window's titlebar. */
	if (!server->dnd_active || server->dnd_titlebar == NULL)
		return;
	if (server->dnd_part_id != id || server->dnd_part_detail != detail)
		return;
	(void)shell_mode(surface, &model, &titlebar);
	if (titlebar != server->dnd_titlebar)
		return;

	/* The edge, then the face within it. */
	memcpy(colour, edge, sizeof(colour));
	colour[3] *= fade;
	glass_draw_solid(server, command, (float)x - 1.0f, (float)y + 1.0f, (float)width + 2.0f, (float)size - 2.0f, 9.0f, colour);
	memcpy(colour, face, sizeof(colour));
	colour[3] *= fade;
	glass_draw_solid(server, command, (float)x + 1.0f, (float)y + 3.0f, (float)width - 2.0f, (float)size - 6.0f, 7.0f, colour);
}

/* Records a region of the frame, when there is room. */
static void
shell_add_hit(
	struct zwl_object *surface,
	unsigned docked,
	unsigned kind,
	uint32_t id,
	uint32_t detail,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height)
{
	struct shell_hit *hit;

	/* The table is full (it is not in practice). */
	if (shell_titlebar.hit_count == SHELL_HITS)
		return;

	/* The region. */
	hit = &shell_titlebar.hits[shell_titlebar.hit_count];
	hit->surface = surface;
	hit->docked = docked;
	hit->kind = kind;
	hit->id = id;
	hit->detail = detail;
	hit->x = x;
	hit->y = y;
	hit->width = width;
	hit->height = height;
	shell_titlebar.hit_count++;
}

/* Finds the region of the last frame under a point; the latest drawn wins. */
static const struct shell_hit *
shell_hit_at(
	int32_t x,
	int32_t y)
{
	const struct shell_hit *hit;
	unsigned index;

	/* From the last recorded. */
	for (index = shell_titlebar.hit_count; index > 0U; index--) {
		hit = &shell_titlebar.hits[index - 1U];
		if (x < hit->x || x >= hit->x + hit->width)
			continue;
		if (y < hit->y || y >= hit->y + hit->height)
			continue;
		return hit;
	}

	/* None. */
	return NULL;
}

/* Carries out a released control: a field takes the keyboard, any other enabled control sends its event. */
static void
shell_act(
	struct zwl_server *server,
	const struct shell_hit *hit)
{
	const struct zwl_titlebar_control *control;
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	unsigned mode;

	/* The tabs have their own regions. */
	mode = shell_mode(hit->surface, &model, &titlebar);
	if (mode == ZWL_TITLEBAR_TABS) {
		shell_act_tabs(server, hit, model, titlebar);
		return;
	}

	/* The control, as the window shows it now. */
	if (mode != ZWL_TITLEBAR_CONTROLS)
		return;
	control = zwl_titlebar_control(&model->shown, hit->id);
	if (control == NULL || control->enabled == 0U)
		return;

	/* A field takes the keyboard (a search field shown as a button too). */
	if (control->role == ZWL_CONTROL_SEARCH) {
		if (shell_titlebar.field.surface != hit->surface || shell_titlebar.field.id != hit->id)
			shell_focus(server, hit->surface, control, 0U);
		return;
	}

	/* The field being edited stays as it is. */
	if (hit->kind == KIND_FIELD)
		return;

	/* Any other control, or a breadcrumb's part, sends its event. */
	zwl_titlebar_send_activated(titlebar, hit->id, hit->detail, "pointer");
}

/*
 * Gives the keyboard to a text control of a window: the field starts with
 * the control's text, all of it selected.
 */
static void
shell_focus(
	struct zwl_server *server,
	struct zwl_object *surface,
	const struct zwl_titlebar_control *control,
	unsigned edit)
{
	struct shell_field *field;
	size_t length;

	/* No control, nothing to focus; another field's editing ends first. */
	if (control == NULL)
		return;
	if (shell_titlebar.field.surface != NULL)
		shell_field_done(server, ZWL_TEXT_LEFT);

	/* The field, with the control's text. */
	field = &shell_titlebar.field;
	field->surface = surface;
	field->id = control->id;
	field->edit = edit;
	length = 0;
	if (control->text != NULL)
		length = strlen(control->text);
	if (length > ZWL_TITLEBAR_TEXT_MAX)
		length = ZWL_TITLEBAR_TEXT_MAX;
	if (control->text != NULL)
		memcpy(field->text, control->text, length);
	field->text[length] = '\0';
	field->length = length;
	field->anchor = 0;
	field->cursor = length;

	/* The window has the keyboard; the log line the tests read. */
	zwl_glass_raise(server, surface);
	server->dirty = 1;
	printf("ZWL TITLEBAR focus client=%llu surface=%u id=%u edit=%u\n", (unsigned long long)surface->client->number, surface->id, control->id, edit);
}

/* Ends the field's editing and tells the client how. */
static void
shell_field_done(
	struct zwl_server *server,
	uint32_t how)
{
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;
	struct zwl_object *surface;

	/* The field goes first (the event may make the client change its controls). */
	surface = shell_titlebar.field.surface;
	shell_titlebar.field.surface = NULL;
	server->dirty = 1;
	if (surface == NULL)
		return;

	/* The client hears how it ended, with the text. */
	(void)shell_mode(surface, &model, &titlebar);
	if (titlebar != NULL)
		zwl_titlebar_send_text(titlebar, shell_titlebar.field.id, shell_titlebar.field.text, 1, how);
}

/* Tells the client the field's text changed. */
static void
shell_field_changed(
	struct zwl_server *server)
{
	struct zwl_titlebar_model *model;
	struct zwl_object *titlebar;

	/* The window's titlebar hears the text. */
	(void)server;
	(void)shell_mode(shell_titlebar.field.surface, &model, &titlebar);
	if (titlebar != NULL)
		zwl_titlebar_send_text(titlebar, shell_titlebar.field.id, shell_titlebar.field.text, 0, 0U);
}

/* Replaces the field's selection with some bytes, when there is room. */
static void
shell_field_insert(
	const char *text,
	size_t length)
{
	struct shell_field *field;
	size_t start;
	size_t end;

	/* The selection goes first. */
	field = &shell_titlebar.field;
	start = field->anchor;
	end = field->cursor;
	if (start > end) {
		start = field->cursor;
		end = field->anchor;
	}

	/* The bytes after the selection close up over it. */
	memmove(field->text + start, field->text + end, field->length - end + 1U);
	field->length -= end - start;

	/* The bytes, when they fit. */
	if (field->length + length <= ZWL_TITLEBAR_TEXT_MAX) {
		memmove(field->text + start + length, field->text + start, field->length - start + 1U);
		memcpy(field->text + start, text, length);
		field->length += length;
		start += length;
	}

	/* The cursor after them. */
	field->cursor = start;
	field->anchor = start;
}

/* Erases the selection, or the character before (forward zero) or after the cursor. */
static void
shell_field_erase(
	int forward)
{
	struct shell_field *field;

	/* Without a selection, the character next to the cursor is selected. */
	field = &shell_titlebar.field;
	if (field->anchor == field->cursor) {
		if (forward != 0)
			field->anchor = shell_field_step(field->cursor, 1);
		else
			field->anchor = shell_field_step(field->cursor, -1);
	}

	/* The selection goes. */
	shell_field_insert("", 0U);
}

/* Moves a byte offset of the field one character forward or back (over UTF-8's continuation bytes). */
static size_t
shell_field_step(
	size_t at,
	int step)
{
	struct shell_field *field;

	/* Back: to the start of the character before. */
	field = &shell_titlebar.field;
	if (step < 0) {
		if (at == 0U)
			return 0;
		at--;
		while (at > 0U && ((unsigned char)field->text[at] & 0xc0U) == 0x80U)
			at--;
		return at;
	}

	/* Forward: past the character's continuation bytes. */
	if (at >= field->length)
		return field->length;
	at++;
	while (at < field->length && ((unsigned char)field->text[at] & 0xc0U) == 0x80U)
		at++;

	/* Reports the new offset. */
	return at;
}

/* Logs a window's control layout when it changed: each control's place (or that it is hidden). */
static void
shell_log_layout(
	struct zwl_object *surface,
	unsigned docked,
	const struct shell_item *items,
	unsigned count,
	int32_t y,
	int32_t size,
	int32_t overflow_x,
	uint32_t checksum)
{
	static const char *const where[] = { "floating", "docked" };
	struct shell_logged *logged;
	unsigned index;
	unsigned slot;
	unsigned free_slot;

	/* The window's entry, or a free one. */
	logged = NULL;
	free_slot = SHELL_LOGGED;
	for (slot = 0; slot < SHELL_LOGGED; slot++) {
		if (shell_titlebar.logged[slot].surface == surface && shell_titlebar.logged[slot].docked == docked) {
			logged = &shell_titlebar.logged[slot];
			break;
		}

		/* The first free entry, for a window logged for the first time. */
		if (shell_titlebar.logged[slot].surface == NULL && free_slot == SHELL_LOGGED)
			free_slot = slot;
	}

	/* A window new to the table takes the free entry. */
	if (logged == NULL && free_slot != SHELL_LOGGED) {
		logged = &shell_titlebar.logged[free_slot];
		logged->surface = surface;
		logged->docked = docked;
		logged->checksum = 0;
	}

	/* The same layout is not logged again. */
	if (logged == NULL || logged->checksum == checksum)
		return;
	logged->checksum = checksum;

	/* Each control. */
	for (index = 0; index < count; index++) {
		printf("ZWL TITLEBAR control client=%llu surface=%u where=%s id=%u x=%d y=%d width=%d height=%d shown=%u\n",
		       (unsigned long long)surface->client->number, surface->id, where[docked], items[index].control->id,
		       items[index].x, y, items[index].width, size, items[index].shown);
	}

	/* "..." as the control 0, when there is one. */
	if (overflow_x >= 0) {
		printf("ZWL TITLEBAR control client=%llu surface=%u where=%s id=0 x=%d y=%d width=%d height=%d shown=1\n",
		       (unsigned long long)surface->client->number, surface->id, where[docked], overflow_x, y, size, size);
	}
}

/* Logs a window's tab strip when it changed: each tab's place (or that it is hidden) and the strip's buttons. */
static void
shell_log_strip(
	struct zwl_object *surface,
	unsigned docked,
	const struct shell_tab *tabs,
	unsigned count,
	const int32_t *closes,
	int32_t y,
	int32_t size,
	const int32_t *buttons,
	uint32_t checksum)
{
	static const char *const where[] = { "floating", "docked" };
	static const char *const names[] = { "left", "right", "new", "overflow" };
	struct shell_logged *logged;
	unsigned index;
	unsigned slot;
	unsigned free_slot;

	/* The window's entry, or a free one (shared with the controls' layouts). */
	logged = NULL;
	free_slot = SHELL_LOGGED;
	for (slot = 0; slot < SHELL_LOGGED; slot++) {
		if (shell_titlebar.logged[slot].surface == surface && shell_titlebar.logged[slot].docked == docked) {
			logged = &shell_titlebar.logged[slot];
			break;
		}

		/* The first free entry, for a window logged for the first time. */
		if (shell_titlebar.logged[slot].surface == NULL && free_slot == SHELL_LOGGED)
			free_slot = slot;
	}

	/* A window new to the table takes the free entry. */
	if (logged == NULL && free_slot != SHELL_LOGGED) {
		logged = &shell_titlebar.logged[free_slot];
		logged->surface = surface;
		logged->docked = docked;
		logged->checksum = 0;
	}

	/* The same layout is not logged again. */
	if (logged == NULL || logged->checksum == checksum)
		return;
	logged->checksum = checksum;

	/* Each tab. */
	for (index = 0; index < count; index++) {
		printf("ZWL TITLEBAR strip client=%llu surface=%u where=%s id=%u x=%d y=%d width=%d height=%d shown=%u flags=%u close=%d\n",
		       (unsigned long long)surface->client->number, surface->id, where[docked], tabs[index].tab->id,
		       tabs[index].x, y, tabs[index].width, size, tabs[index].shown, tabs[index].tab->flags, closes[index]);
	}

	/* The arrows, "+" and "...", when they are there. */
	for (index = 0; index < 4U; index++) {
		if (buttons[index] < 0)
			continue;
		printf("ZWL TITLEBAR strip client=%llu surface=%u where=%s button=%s x=%d y=%d width=%d height=%d\n",
		       (unsigned long long)surface->client->number, surface->id, where[docked], names[index], buttons[index], y, size, size);
	}
}

/* Mixes a value into a layout's checksum (FNV-1a over the value's bytes). */
static uint32_t
shell_mix(
	uint32_t checksum,
	uint32_t value)
{
	unsigned index;

	/* Each byte of the value. */
	for (index = 0; index < 4U; index++) {
		checksum ^= (value >> (index * 8U)) & 0xffU;
		checksum *= 16777619U;
	}

	/* Reports the checksum. */
	return checksum;
}

/* Makes a colour: an ink faded by an amount; with no ink, nothing (a group's pill uses its own). */
static void
shell_colour(
	float *colour,
	const float *ink,
	float alpha)
{
	/* No ink, nothing to make. */
	if (colour == NULL || ink == NULL)
		return;

	/* The ink with its opacity faded. */
	colour[0] = ink[0];
	colour[1] = ink[1];
	colour[2] = ink[2];
	colour[3] = ink[3] * alpha;
}
