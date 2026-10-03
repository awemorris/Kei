/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The pointer and the keyboard of files (spec §13, §14, §35).
 *
 * A press on an item selects it (Ctrl adds or removes it, Shift selects
 * the range from the anchor); a press on the panel's empty ground starts a
 * rubber band.  The keyboard moves a cursor through the items (Shift
 * extends the selection), opens them with Enter, goes back with Backspace,
 * and finds an item by the first letters of its name.  While a text field
 * has the focus, the keys edit it.
 */

#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* A second click this soon after the first on the same region is a double click, in milliseconds. */
#define INPUT_DOUBLE_CLICK_MS	400U

/* Letters typed within this long of each other find an item together, in milliseconds. */
#define INPUT_TYPE_AHEAD_MS	1000U

/* The evdev codes of the keys the content handles. */
#define INPUT_KEY_ESC		1U
#define INPUT_KEY_1		2U
#define INPUT_KEY_2		3U
#define INPUT_KEY_BACKSPACE	14U
#define INPUT_KEY_ENTER		28U
#define INPUT_KEY_A		30U
#define INPUT_KEY_H		35U
#define INPUT_KEY_L		38U
#define INPUT_KEY_KPENTER	96U
#define INPUT_KEY_HOME		102U
#define INPUT_KEY_UP		103U
#define INPUT_KEY_PAGEUP	104U
#define INPUT_KEY_LEFT		105U
#define INPUT_KEY_RIGHT		106U
#define INPUT_KEY_END		107U
#define INPUT_KEY_DOWN		108U
#define INPUT_KEY_PAGEDOWN	109U
#define INPUT_KEY_DELETE	111U
#define INPUT_KEY_F2		60U
#define INPUT_KEY_C		46U
#define INPUT_KEY_X		45U
#define INPUT_KEY_V		47U
#define INPUT_KEY_D		32U
#define INPUT_KEY_Z		44U
#define INPUT_KEY_N		49U
#define INPUT_KEY_F		33U
#define INPUT_KEY_T		20U
#define INPUT_KEY_P		25U
#define INPUT_KEY_I		23U
#define INPUT_KEY_O		24U
#define INPUT_KEY_W		17U
#define INPUT_KEY_S		31U
#define INPUT_KEY_R		19U
#define INPUT_KEY_M		50U
#define INPUT_KEY_TAB		15U

/*
 * A key of the menus that the other key handlers do not know, and the
 * action it asks for.  zdesktop takes these keys for the menus while their
 * items are enabled; without the System Menu they arrive here.
 */
struct input_shortcut {
	uint32_t key;
	uint32_t modifiers;
	unsigned action;
};

/* Those keys. */
static const struct input_shortcut input_shortcuts[] = {
	{ INPUT_KEY_N, FM_MOD_CTRL, FM_ACTION_NEW_WINDOW },
	{ INPUT_KEY_O, FM_MOD_CTRL, FM_ACTION_OPEN },
	{ INPUT_KEY_W, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_CLOSE_WINDOW },
	{ INPUT_KEY_S, FM_MOD_CTRL | FM_MOD_ALT, FM_ACTION_SHOW_SIDEBAR },
	{ INPUT_KEY_H, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_GO_HOME },
	{ INPUT_KEY_D, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_GO_DESKTOP },
	{ INPUT_KEY_O, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_GO_DOCUMENTS },
	{ INPUT_KEY_L, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_GO_DOWNLOADS },
	{ INPUT_KEY_R, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_GO_RECENTS },
	{ INPUT_KEY_C, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_GO_COMPUTER },
	{ INPUT_KEY_M, FM_MOD_CTRL, FM_ACTION_MINIMIZE },
	{ INPUT_KEY_T, FM_MOD_CTRL, FM_ACTION_NEW_TAB },
	{ INPUT_KEY_W, FM_MOD_CTRL, FM_ACTION_CLOSE_TAB },
	{ INPUT_KEY_TAB, FM_MOD_CTRL, FM_ACTION_NEXT_TAB },
	{ INPUT_KEY_TAB, FM_MOD_CTRL | FM_MOD_SHIFT, FM_ACTION_PREVIOUS_TAB },
	{ INPUT_KEY_PAGEDOWN, FM_MOD_CTRL, FM_ACTION_NEXT_TAB },
	{ INPUT_KEY_PAGEUP, FM_MOD_CTRL, FM_ACTION_PREVIOUS_TAB }
};
#define INPUT_KEY_SPACE		57U

/*
 * The selection the log reported last: how many items and which had the
 * cursor.  Only a change is logged; they live for the whole run and start
 * as no selection with the cursor nowhere.
 */
static size_t input_reported_count;
static int input_reported_cursor = -1;

static int input_contains(const struct fm_rect *rect, int x, int y);
static void input_press(struct fm_app *app, const struct fm_event *event);
static void input_middle(struct fm_app *app, const struct fm_event *event);
static void input_context(struct fm_app *app, struct fm_tab *tab, const struct fm_event *event, unsigned kind, int index);
static void input_click(struct fm_app *app, unsigned kind, int index, int double_click, uint32_t modifiers);
static void input_press_item(struct fm_app *app, int index, int double_click, uint32_t modifiers);
static void input_select_item(struct fm_tab *tab, int index, uint32_t modifiers);
static void input_release(struct fm_app *app);
static void input_band_start(struct fm_app *app, int x, int y, uint32_t modifiers);
static void input_band_update(struct fm_app *app, int x, int y);
static void input_sort(struct fm_app *app, int column);
static void input_location_key(struct fm_app *app, const struct fm_event *event);
static int input_command_key(struct fm_app *app, const struct fm_event *event);
static int input_move_key(struct fm_app *app, const struct fm_event *event);
static void input_move(struct fm_app *app, int target, int extend);
static void input_type_ahead(struct fm_app *app, char character);
static void input_show(struct fm_app *app, int index);
static void input_report(struct fm_app *app);
static int input_operation_key(struct fm_app *app, const struct fm_event *event);
static void input_button(struct fm_app *app, int index);
static void input_look_key(struct fm_app *app, const struct fm_event *event);
static void input_info_key(struct fm_app *app, const struct fm_event *event);
static int input_shortcut_key(struct fm_app *app, const struct fm_event *event);

/*
 * Follows the pointer: the region under it is lit, and a rubber band being
 * dragged follows it.
 */
void
fm_input_motion(
	struct fm_app *app,
	const struct fm_event *event)
{
	unsigned kind;
	int index;
	int dragging;

	/* The pointer's place. */
	app->pointer_x = event->x;
	app->pointer_y = event->y;
	app->pointer_inside = 1;

	/* A drag of the overlay scroll bar's thumb is the bar's alone; near the edge it grows. */
	dragging = fm_scrollbar_motion(app, event->x, event->y);
	if (dragging != 0)
		return;

	/* A press on an item (or a favorite) drags the selection (or the favorite) once the pointer moves away from it. */
	if (app->pressing != 0 && (app->press_kind == FM_HIT_ITEM || app->press_kind == FM_HIT_PLACE)) {
		dragging = fm_drag_motion(app, event->x, event->y);
		if (dragging != 0)
			return;
	}

	/* A rubber band stretches to the pointer. */
	if (app->band != 0 && app->pressing != 0)
		input_band_update(app, event->x, event->y);

	/* A new region under it needs a new frame. */
	(void)fm_input_hit_at(app, event->x, event->y, &kind, &index);
	if (kind != app->hover_kind || index != app->hover_index) {
		app->hover_kind = kind;
		app->hover_index = index;
		app->dirty = 1;
	}
}

/*
 * Handles a pointer button: the left one presses and clicks, the right
 * one selects what it is on and asks for its context menu.
 */
void
fm_input_button(
	struct fm_app *app,
	const struct fm_event *event)
{
	struct fm_tab *tab;
	unsigned kind;
	int index;
	int taken;

	/* The right button selects the item under it (unless it is already selected) and asks for its context menu. */
	tab = fm_ui_tab(app);
	if (event->button == FM_BUTTON_RIGHT) {
		if (event->pressed == 0)
			return;
		(void)fm_input_hit_at(app, event->x, event->y, &kind, &index);
		input_context(app, tab, event, kind, index);
		return;
	}

	/* The middle button opens a folder or a place of the sidebar in a new tab. */
	if (event->button == FM_BUTTON_MIDDLE) {
		if (event->pressed != 0)
			input_middle(app, event);
		return;
	}

	/* Other buttons than the left one do nothing. */
	if (event->button != FM_BUTTON_LEFT)
		return;

	/* A press on the overlay scroll bar is the bar's (a drag of its thumb, or a page). */
	if (event->pressed != 0) {
		taken = fm_scrollbar_press(app, event->x, event->y);
		if (taken != 0)
			return;
	}

	/* A press, and the selection it left. */
	if (event->pressed != 0) {
		input_press(app, event);
		input_report(app);
		return;
	}

	/* The release ends a drag of the bar's thumb, or else the press. */
	taken = fm_scrollbar_release(app);
	if (taken != 0)
		return;
	input_release(app);
}

/*
 * Scrolls the content (or the sidebar, when the pointer is over it) by an
 * amount of pixels (positive is down), kept within what there is.
 */
void
fm_input_scroll(
	struct fm_app *app,
	int amount)
{
	struct fm_tab *tab;
	int limit;
	int over;

	/* Over the sidebar: the sidebar scrolls. */
	over = input_contains(&app->layout.sidebar, app->pointer_x, app->pointer_y);
	if (over != 0) {
		limit = app->layout.sidebar_height - app->layout.sidebar.height;
		if (limit < 0)
			limit = 0;
		app->sidebar_scroll += amount;
		if (app->sidebar_scroll > limit)
			app->sidebar_scroll = limit;
		if (app->sidebar_scroll < 0)
			app->sidebar_scroll = 0;
		app->dirty = 1;
		return;
	}

	/* The furthest the content scrolls. */
	tab = fm_ui_tab(app);
	limit = app->layout.content_height - app->layout.content.height;
	if (limit < 0)
		limit = 0;

	/* The new scroll, inside the range; the overlay bar comes out. */
	tab->scroll += amount;
	if (tab->scroll > limit)
		tab->scroll = limit;
	if (tab->scroll < 0)
		tab->scroll = 0;
	app->dirty = 1;
	fm_scrollbar_moved(app);

	/* The item under the pointer moved away with the content: nothing is lit until the pointer moves. */
	app->hover_kind = FM_HIT_NONE;
	app->hover_index = -1;

	/* A rubber band keeps its far corner under the pointer. */
	if (app->band != 0 && app->pressing != 0)
		input_band_update(app, app->pointer_x, app->pointer_y);
}

/*
 * Handles a key: a text field with the focus edits, the content moves,
 * selects, opens and goes.
 */
void
fm_input_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	unsigned result;
	char character;
	int handled;

	/* Releases do nothing. */
	if (event->pressed == 0)
		return;
	app->dirty = 1;

	/* Esc gives up a drag, and the other keys wait for its end. */
	if (app->drag != 0) {
		if (event->key == INPUT_KEY_ESC)
			fm_drag_cancel(app);
		return;
	}

	/* A question takes Enter (yes) and Esc (no), and nothing else. */
	if (app->dialog != FM_DIALOG_NONE) {
		if (event->key == INPUT_KEY_ENTER || event->key == INPUT_KEY_KPENTER)
			fm_action_confirm(app, 1);
		else if (event->key == INPUT_KEY_ESC)
			fm_action_confirm(app, 0);
		return;
	}

	/* A Help card closes with Esc, and takes the other keys too. */
	if (app->help != FM_HELP_NONE) {
		if (event->key == INPUT_KEY_ESC)
			fm_help_close(app);
		return;
	}

	/* The information card takes the keys while it is open. */
	if (app->info_open != 0) {
		input_info_key(app, event);
		return;
	}

	/* Quick Look takes the keys while it is open. */
	if (app->quicklook != 0) {
		input_look_key(app, event);
		return;
	}

	/* The location field edits while it has the focus. */
	if (app->focus == FM_FOCUS_LOCATION) {
		input_location_key(app, event);
		return;
	}

	/* So does the search field. */
	if (app->focus == FM_FOCUS_SEARCH) {
		fm_search_key(app, event);
		return;
	}

	/* So does the name being changed: Enter renames, Esc gives up. */
	if (app->focus == FM_FOCUS_RENAME) {
		result = fm_field_key(&app->rename, event->key, event->modifiers);
		if (result == FM_FIELD_ENTER)
			fm_action_rename_end(app, 1);
		else if (result == FM_FIELD_CANCEL)
			fm_action_rename_end(app, 0);
		return;
	}

	/* The menus' keys that nothing else handles. */
	handled = input_shortcut_key(app, event);
	if (handled != 0) {
		input_report(app);
		return;
	}

	/* The file operations' keys. */
	handled = input_operation_key(app, event);
	if (handled != 0) {
		input_report(app);
		return;
	}

	/* Commands (Ctrl, Alt, Backspace, Enter, Esc). */
	handled = input_command_key(app, event);
	if (handled != 0) {
		input_report(app);
		return;
	}

	/* The cursor keys. */
	handled = input_move_key(app, event);
	if (handled != 0) {
		input_report(app);
		return;
	}

	/* A letter finds an item by its name. */
	character = fm_key_character(event->key, event->modifiers);
	if (character > ' ')
		input_type_ahead(app, character);

	/* The selection the key left. */
	input_report(app);
}

/*
 * Finds the region of the last frame under a point (the last drawn wins);
 * zero when there is none.
 */
int
fm_input_hit_at(
	struct fm_app *app,
	int x,
	int y,
	unsigned *kind,
	int *index)
{
	int hit;
	int inside;

	/* From the last region drawn to the first. */
	for (hit = app->hit_count - 1; hit >= 0; hit--) {
		inside = input_contains(&app->hits[hit].rect, x, y);
		if (inside != 0) {
			*kind = app->hits[hit].kind;
			*index = app->hits[hit].index;
			return 1;
		}
	}

	/* Nothing clickable is there. */
	*kind = FM_HIT_NONE;
	*index = -1;
	return 0;
}

/*
 * Sorts the items by a key, in either direction, keeping the cursor on its
 * item.
 */
void
fm_input_sort_by(
	struct fm_app *app,
	unsigned sort,
	int reverse)
{
	struct fm_tab *tab;
	char cursor[FM_NAME_MAX];

	/* The window's sort from now on. */
	app->sort = sort;
	app->sort_reverse = reverse;

	/* The items in the new order, the cursor kept on its item. */
	tab = fm_ui_tab(app);
	cursor[0] = '\0';
	if (tab->cursor >= 0 && (size_t)tab->cursor < tab->listing.count)
		snprintf(cursor, sizeof(cursor), "%s", tab->listing.entries[tab->cursor].name);
	fm_dir_sort(&tab->listing, app->sort, app->sort_reverse);
	if (cursor[0] != '\0')
		tab->cursor = fm_select_find(tab, cursor);
	tab->anchor = tab->cursor;
	app->dirty = 1;
	fm_log("SORT key=%u reverse=%d", app->sort, app->sort_reverse);
}

/*
 * Opens the selected items: the first selected folder in the tab, or else
 * each selected file (at most eight).
 */
void
fm_input_open_selection(
	struct fm_app *app)
{
	struct fm_tab *tab;
	size_t index;
	int opened;

	/* A selected folder opens in the tab. */
	tab = fm_ui_tab(app);
	for (index = 0; index < tab->listing.count; index++) {
		if (tab->listing.entries[index].selected != 0 && tab->listing.entries[index].folder != 0) {
			fm_ui_open(app, (int)index);
			return;
		}
	}

	/* Otherwise each selected file, up to eight. */
	opened = 0;
	for (index = 0; index < tab->listing.count && opened < 8; index++) {
		if (tab->listing.entries[index].selected == 0)
			continue;
		fm_ui_open(app, (int)index);
		opened++;
	}
}

/*
 * Goes to the folder that holds the one shown (Ctrl+Up).
 */
void
fm_input_enclosing(
	struct fm_app *app)
{
	struct fm_location location;
	struct fm_tab *tab;
	char *slash;

	/* Only a folder below the root has one. */
	tab = fm_ui_tab(app);
	location = tab->history[tab->history_index].location;
	if (location.kind != FM_LOCATION_FOLDER)
		return;
	slash = strrchr(location.path, '/');
	if (slash == NULL || location.path[1] == '\0')
		return;

	/* The path up to the last slash (the root keeps its slash). */
	if (slash == location.path)
		slash[1] = '\0';
	else
		*slash = '\0';
	fm_ui_go(app, &location);
}

/*
 * Turns the path in the titlebar into a field to type a folder in (Ctrl+L),
 * starting from the folder shown (the home folder for another place):
 * zdesktop is asked to give its path the keyboard (fm_titlebar_state).
 */
void
fm_input_location(
	struct fm_app *app)
{
	const struct fm_location *location;
	struct fm_tab *tab;

	/* The folder shown, or the home folder. */
	tab = fm_ui_tab(app);
	location = &tab->history[tab->history_index].location;
	fm_field_set(&app->location, location->path);
	if (location->kind != FM_LOCATION_FOLDER)
		fm_field_set(&app->location, app->home);

	/* Typing goes to the field, which zdesktop is asked for. */
	app->focus = FM_FOCUS_LOCATION;
	app->control_focus = FM_CONTROL_PATH;
	app->control_focus_serial++;
	app->dirty = 1;
}

/* Tells whether a rectangle holds a point. */
static int
input_contains(
	const struct fm_rect *rect,
	int x,
	int y)
{
	/* Left of it or above it. */
	if (x < rect->x || y < rect->y)
		return 0;

	/* Right of it or below it. */
	if (x >= rect->x + rect->width || y >= rect->y + rect->height)
		return 0;

	/* Inside. */
	return 1;
}

/* Handles a press of the left button: a click on the region under it, a double click when it is the second soon on the same. */
static void
input_press(
	struct fm_app *app,
	const struct fm_event *event)
{
	unsigned kind;
	int index;
	int double_click;

	/* The region under the pointer, and the press in progress. */
	(void)fm_input_hit_at(app, event->x, event->y, &kind, &index);
	app->pressing = 1;
	app->press_kind = kind;
	app->press_index = index;
	app->press_x = event->x;
	app->press_y = event->y;
	app->press_deferred = 0;
	app->dirty = 1;

	/* A second press soon on the same region is a double click (a third starts again). */
	double_click = 0;
	if (kind == app->click_kind && index == app->click_index && event->time - app->click_time < INPUT_DOUBLE_CLICK_MS)
		double_click = 1;
	app->click_kind = kind;
	app->click_index = index;
	app->click_time = event->time;
	if (double_click != 0)
		app->click_time = 0;

	/* A press in the window ends the typing in the titlebar's location or search field. */
	if (app->focus == FM_FOCUS_LOCATION || app->focus == FM_FOCUS_SEARCH)
		app->focus = FM_FOCUS_CONTENT;

	/* A press anywhere but on the name being changed ends the change, keeping what was typed. */
	if (app->focus == FM_FOCUS_RENAME && kind != FM_HIT_OVERLAY)
		fm_action_rename_end(app, 1);

	/* A press anywhere but on the operations' list closes the list. */
	if (app->show_tasks != 0 && kind != FM_HIT_BUTTON)
		app->show_tasks = 0;

	/* What the press does. */
	input_click(app, kind, index, double_click, event->modifiers);
}

/* Handles a press of the middle button: a folder or a place of the sidebar under it opens in a new tab. */
static void
input_middle(
	struct fm_app *app,
	const struct fm_event *event)
{
	struct fm_location location;
	struct fm_entry *entry;
	struct fm_tab *tab;
	unsigned kind;
	int index;

	/* A place of the sidebar. */
	(void)fm_input_hit_at(app, event->x, event->y, &kind, &index);
	if (kind == FM_HIT_PLACE && index >= 0 && index < app->places.count) {
		fm_tabs_new(app, &app->places.items[index].location);
		return;
	}

	/* Otherwise only a folder among the items. */
	tab = fm_ui_tab(app);
	if (kind != FM_HIT_ITEM || index < 0 || (size_t)index >= tab->listing.count)
		return;
	entry = &tab->listing.entries[index];
	if (entry->folder == 0)
		return;

	/* The folder, in a tab of its own. */
	memset(&location, 0, sizeof(location));
	location.kind = FM_LOCATION_FOLDER;
	snprintf(location.path, sizeof(location.path), "%s", entry->path);
	fm_tabs_new(app, &location);
}

/*
 * Handles a press of the right button: an item under it is selected (the
 * selection stays when it is part of it), and the window asks for the
 * context menu of what it is on -- the items, the empty part of the
 * content, or a place of the sidebar.  Elsewhere nothing opens.
 */
static void
input_context(
	struct fm_app *app,
	struct fm_tab *tab,
	const struct fm_event *event,
	unsigned kind,
	int index)
{
	const struct fm_rect *content;
	int inside;

	/* The press's place, which the menu opens at. */
	app->context_x = event->x;
	app->context_y = event->y;
	app->context_place = -1;
	app->dirty = 1;
	fm_log("CONTEXT kind=%u index=%d", kind, index);

	/* A place of the sidebar. */
	if (kind == FM_HIT_PLACE && index >= 0 && index < app->places.count) {
		app->context_where = FM_CONTEXT_PLACE;
		app->context_place = index;
		app->request = FM_REQUEST_CONTEXT;
		return;
	}

	/* An item: selected unless it already is. */
	if (kind == FM_HIT_ITEM && index >= 0 && (size_t)index < tab->listing.count) {
		if (tab->listing.entries[index].selected == 0)
			fm_select_only(tab, index);
		app->context_where = FM_CONTEXT_ITEMS;
		app->request = FM_REQUEST_CONTEXT;
		return;
	}

	/* The empty part of the content: nothing stays selected. */
	content = &app->layout.content;
	inside = 0;
	if (event->x >= content->x && event->x < content->x + content->width &&
	    event->y >= content->y && event->y < content->y + content->height)
		inside = 1;
	if (inside != 0) {
		fm_select_none(tab);
		app->context_where = FM_CONTEXT_EMPTY;
		app->request = FM_REQUEST_CONTEXT;
	}
}

/* Carries out a click on a region. */
static void
input_click(
	struct fm_app *app,
	unsigned kind,
	int index,
	int double_click,
	uint32_t modifiers)
{
	/* What each region does. */
	switch (kind) {
	case FM_HIT_PLACE:
		/* A place goes there; a favorite folder at the release, as it may be dragged to another place in the list. */
		if (index < 0 || index >= app->places.count)
			break;
		if (app->places.items[index].section == FM_SECTION_FAVORITES && app->places.items[index].location.kind == FM_LOCATION_FOLDER) {
			app->press_deferred = 1;
			break;
		}

		/* Any other place at once. */
		fm_ui_go(app, &app->places.items[index].location);
		break;
	case FM_HIT_TAB:
		fm_tabs_select(app, index);
		break;
	case FM_HIT_TAB_CLOSE:
		fm_tabs_close(app, index);
		break;
	case FM_HIT_HEADER:
		input_sort(app, index);
		break;
	case FM_HIT_ITEM:
		input_press_item(app, index, double_click, modifiers);
		break;
	case FM_HIT_CONTENT:
		input_band_start(app, app->pointer_x, app->pointer_y, modifiers);
		break;
	case FM_HIT_BUTTON:
		input_button(app, index);
		break;
	case FM_HIT_SCOPE:
		fm_search_scope(app, (unsigned)index);
		break;
	case FM_HIT_CARD:
	case FM_HIT_RECENT:
	case FM_HIT_SHOW_ALL:
		fm_home_click(app, kind, index, double_click);
		break;
	case FM_HIT_OVERLAY:
		if (index == FM_OVERLAY_LOOK_GROUND)
			fm_look_close(app);
		if (index == FM_OVERLAY_INFO_GROUND)
			fm_info_close(app);
		if (index == FM_OVERLAY_HELP_GROUND)
			fm_help_close(app);
		break;
	default:
		break;
	}
}

/* Presses an item: selects it (Ctrl toggles, Shift extends), or opens it on a double click. */
static void
input_press_item(
	struct fm_app *app,
	int index,
	int double_click,
	uint32_t modifiers)
{
	struct fm_tab *tab;

	/* An item that is not there. */
	tab = fm_ui_tab(app);
	if (index < 0 || (size_t)index >= tab->listing.count)
		return;

	/* A double click opens it. */
	if (double_click != 0 && modifiers == 0U) {
		fm_select_only(tab, index);
		fm_ui_open(app, index);
		return;
	}

	/* A selected item keeps the selection, which may be dragged; the release without a drag changes it. */
	if (tab->listing.entries[index].selected != 0) {
		app->press_deferred = 1;
		app->press_deferred_modifiers = modifiers;
		return;
	}

	/* Otherwise the selection changes now. */
	input_select_item(tab, index, modifiers);
}

/* Selects an item as a press does: Ctrl adds it or takes it out, Shift selects the range from the anchor, a plain press selects it alone. */
static void
input_select_item(
	struct fm_tab *tab,
	int index,
	uint32_t modifiers)
{
	/* Each kind of press. */
	if ((modifiers & FM_MOD_CTRL) != 0U) {
		fm_select_toggle(tab, index);
	} else if ((modifiers & FM_MOD_SHIFT) != 0U) {
		fm_select_range(tab, tab->anchor, index);
	} else {
		fm_select_only(tab, index);
	}
}

/*
 * Ends a press of the left button: a drag drops its items; otherwise a
 * press on a selected item changes the selection now, and a rubber band
 * reports its selection.
 */
static void
input_release(
	struct fm_app *app)
{
	struct fm_tab *tab;
	int dropped;

	/* A drag ends with its drop. */
	dropped = fm_drag_release(app);

	/* Without a drag, the selection change the press left. */
	tab = fm_ui_tab(app);
	if (dropped == 0 && app->press_deferred != 0 && app->pressing != 0 && app->press_kind == FM_HIT_ITEM) {
		if (app->press_index >= 0 && (size_t)app->press_index < tab->listing.count) {
			input_select_item(tab, app->press_index, app->press_deferred_modifiers);
			input_report(app);
		}
	}

	/* Or the favorite the press was on is gone to. */
	if (dropped == 0 && app->press_deferred != 0 && app->pressing != 0 && app->press_kind == FM_HIT_PLACE) {
		if (app->press_index >= 0 && app->press_index < app->places.count)
			fm_ui_go(app, &app->places.items[app->press_index].location);
	}

	/* A rubber band's selection is reported. */
	if (app->band != 0)
		input_report(app);

	/* The press is over. */
	app->press_deferred = 0;
	app->pressing = 0;
	app->band = 0;
	app->dirty = 1;
}

/* Starts a rubber band at a point of the panel's empty ground; without Ctrl the selection is cleared first. */
static void
input_band_start(
	struct fm_app *app,
	int x,
	int y,
	uint32_t modifiers)
{
	struct fm_tab *tab;
	int inside;

	/* Outside the items' part (the title) nothing starts, but the selection is cleared. */
	tab = fm_ui_tab(app);
	if ((modifiers & (FM_MOD_CTRL | FM_MOD_SHIFT)) == 0U)
		fm_select_none(tab);
	inside = input_contains(&app->layout.items, x, y);
	if (inside == 0)
		return;

	/* The band's corners, in the items' coordinates (with the scroll). */
	app->band = 1;
	app->band_x0 = x;
	app->band_y0 = y + tab->scroll;
	app->band_x1 = x;
	app->band_y1 = y + tab->scroll;
}

/* Stretches the rubber band to a point and selects the items it touches (Ctrl keeps those selected before). */
static void
input_band_update(
	struct fm_app *app,
	int x,
	int y)
{
	struct fm_tab *tab;
	struct fm_rect band;
	struct fm_rect item;
	size_t index;
	int touches;

	/* The far corner. */
	tab = fm_ui_tab(app);
	app->band_x1 = x;
	app->band_y1 = y + tab->scroll;
	app->dirty = 1;

	/* The band on the screen, its corners in order. */
	band.x = app->band_x0;
	band.width = app->band_x1 - app->band_x0;
	if (band.width < 0) {
		band.x = app->band_x1;
		band.width = -band.width;
	}

	/* And its top and height. */
	band.y = app->band_y0 - tab->scroll;
	band.height = app->band_y1 - app->band_y0;
	if (band.height < 0) {
		band.y = app->band_y1 - tab->scroll;
		band.height = -band.height;
	}

	/* Each item the band touches is selected; the others are not (unless Ctrl keeps them). */
	for (index = 0; index < tab->listing.count; index++) {
		fm_view_item_rect(app, (int)index, &item);
		touches = 1;
		if (item.x + item.width < band.x || band.x + band.width < item.x)
			touches = 0;
		if (item.y + item.height < band.y || band.y + band.height < item.y)
			touches = 0;
		if (touches != 0)
			tab->listing.entries[index].selected = 1;
		else if ((app->modifiers & FM_MOD_CTRL) == 0U)
			tab->listing.entries[index].selected = 0;
	}
}

/* Sorts by a list column's key; the column sorted already turns round. */
static void
input_sort(
	struct fm_app *app,
	int column)
{
	int reverse;
	int sort;

	/* The column's sort. */
	sort = fm_list_sort_at(app, column);
	if (sort < 0 || sort >= FM_SORT_COUNT)
		return;

	/* The same sort again reverses it; another starts forward. */
	reverse = 0;
	if ((unsigned)sort == app->sort)
		reverse = !app->sort_reverse;

	/* The items in the new order. */
	fm_input_sort_by(app, (unsigned)sort, reverse);
}

/* Edits the location field: Enter goes to the path typed, Esc gives up. */
static void
input_location_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	unsigned result;

	/* The field's answer to the key. */
	result = fm_field_key(&app->location, event->key, event->modifiers);
	if (result == FM_FIELD_ENTER) {
		fm_input_location_go(app);
	} else if (result == FM_FIELD_CANCEL) {
		app->focus = FM_FOCUS_CONTENT;
	}
}

/*
 * Goes to the folder typed in the location field (~ is the home folder); a
 * path that is no folder is said so.
 */
void
fm_input_location_go(
	struct fm_app *app)
{
	struct fm_location location;
	struct stat status;
	char message[FM_PATH_MAX + 32];
	int folder;
	int error;

	/* The path, with a leading ~ for the home folder. */
	memset(&location, 0, sizeof(location));
	location.kind = FM_LOCATION_FOLDER;
	if (app->location.text[0] == '~')
		snprintf(location.path, sizeof(location.path), "%s%s", app->home, app->location.text + 1);
	else
		snprintf(location.path, sizeof(location.path), "%s", app->location.text);

	/* It must be a folder. */
	folder = 0;
	error = stat(location.path, &status);
	if (error == 0)
		folder = S_ISDIR(status.st_mode);
	if (folder == 0) {
		snprintf(message, sizeof(message), "No folder at %s", location.path);
		fm_ui_message(app, message);
		return;
	}

	/* The typing ends and the tab goes there. */
	app->focus = FM_FOCUS_CONTENT;
	fm_ui_go(app, &location);
}

/* Handles the command keys; zero when the key is not one. */
static int
input_command_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	struct fm_tab *tab;
	int handled;

	/* The keys that mean the same whatever is selected: Esc clears, Enter and Ctrl+Down open, Backspace and Alt+Left go back, Alt+Right forward, Ctrl+Up up. */
	tab = fm_ui_tab(app);
	handled = 1;
	if (event->key == INPUT_KEY_ESC && event->modifiers == 0U) {
		fm_select_none(tab);
		app->band = 0;
	} else if (event->key == INPUT_KEY_ENTER && event->modifiers == 0U) {
		fm_input_open_selection(app);
	} else if (event->key == INPUT_KEY_KPENTER && event->modifiers == 0U) {
		fm_input_open_selection(app);
	} else if (event->key == INPUT_KEY_BACKSPACE && event->modifiers == 0U) {
		fm_ui_back(app);
	} else if (event->key == INPUT_KEY_LEFT && event->modifiers == FM_MOD_ALT) {
		fm_ui_back(app);
	} else if (event->key == INPUT_KEY_RIGHT && event->modifiers == FM_MOD_ALT) {
		fm_ui_forward(app);
	} else if (event->key == INPUT_KEY_UP && event->modifiers == FM_MOD_CTRL) {
		fm_input_enclosing(app);
	} else if (event->key == INPUT_KEY_DOWN && event->modifiers == FM_MOD_CTRL) {
		fm_input_open_selection(app);
	} else if (event->key == INPUT_KEY_SPACE && event->modifiers == 0U) {
		fm_look_toggle(app);
	} else {
		handled = 0;
	}

	/* One of those was pressed. */
	if (handled != 0)
		return 1;

	/* The keys with Ctrl alone. */
	if (event->modifiers != FM_MOD_CTRL)
		return 0;

	/* Select all, the location field, hidden files, and the two views. */
	switch (event->key) {
	case INPUT_KEY_A:
		fm_select_all(tab);
		return 1;
	case INPUT_KEY_L:
		fm_input_location(app);
		return 1;
	case INPUT_KEY_H:
		app->show_hidden = !app->show_hidden;
		fm_ui_reload(app, tab);
		return 1;
	case INPUT_KEY_1:
		app->view = FM_VIEW_ICONS;
		input_show(app, tab->cursor);
		return 1;
	case INPUT_KEY_2:
		app->view = FM_VIEW_LIST;
		input_show(app, tab->cursor);
		return 1;
	default:
		break;
	}

	/* Not a command. */
	return 0;
}

/* Handles the cursor keys (Shift extends the selection); zero when the key is not one. */
static int
input_move_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	struct fm_tab *tab;
	int columns;
	int rows;
	int extend;
	int cursor;
	int last;

	/* Only plain and Shifted cursor keys move. */
	if ((event->modifiers & ~FM_MOD_SHIFT) != 0U)
		return 0;
	tab = fm_ui_tab(app);
	extend = 0;
	if ((event->modifiers & FM_MOD_SHIFT) != 0U)
		extend = 1;

	/* The grid's columns (the list has one), how many rows a page holds, and where the cursor is. */
	columns = 1;
	if (app->view == FM_VIEW_ICONS && app->layout.columns > 1)
		columns = app->layout.columns;
	rows = 1;
	if (app->layout.cell_height > 0)
		rows = app->layout.items.height / app->layout.cell_height;
	if (rows < 1)
		rows = 1;
	cursor = tab->cursor;
	last = (int)tab->listing.count - 1;

	/* With no cursor, the first key puts it on the first item. */
	if (last < 0)
		return 0;

	/* Each key's target. */
	switch (event->key) {
	case INPUT_KEY_LEFT:
		if (columns == 1)
			return 0;
		cursor--;
		break;
	case INPUT_KEY_RIGHT:
		if (columns == 1)
			return 0;
		cursor++;
		break;
	case INPUT_KEY_UP:
		cursor -= columns;
		break;
	case INPUT_KEY_DOWN:
		cursor += columns;
		if (tab->cursor < 0)
			cursor = 0;
		break;
	case INPUT_KEY_PAGEUP:
		cursor -= columns * rows;
		break;
	case INPUT_KEY_PAGEDOWN:
		cursor += columns * rows;
		break;
	case INPUT_KEY_HOME:
		cursor = 0;
		break;
	case INPUT_KEY_END:
		cursor = last;
		break;
	default:
		return 0;
	}

	/* The target kept among the items. */
	if (cursor < 0)
		cursor = 0;
	if (cursor > last)
		cursor = last;
	input_move(app, cursor, extend);

	/* The key was a move. */
	return 1;
}

/* Moves the cursor to an item, selecting it alone or extending the selection to it, and shows it. */
static void
input_move(
	struct fm_app *app,
	int target,
	int extend)
{
	struct fm_tab *tab;

	/* Shift extends from the anchor; otherwise the item is selected alone. */
	tab = fm_ui_tab(app);
	if (extend != 0 && tab->anchor >= 0) {
		fm_select_range(tab, tab->anchor, target);
	} else {
		fm_select_only(tab, target);
	}

	/* The item scrolled into sight. */
	input_show(app, target);
}

/* Finds an item whose name starts with what was typed lately (without regard to case), and selects it. */
static void
input_type_ahead(
	struct fm_app *app,
	char character)
{
	struct fm_tab *tab;
	const char *name;
	size_t index;
	size_t at;
	char left;
	char right;

	/* A pause starts a new word. */
	tab = fm_ui_tab(app);
	if (app->now - app->typed_at > INPUT_TYPE_AHEAD_MS)
		app->typed_length = 0;
	app->typed_at = app->now;
	if (app->typed_length + 1U < sizeof(app->typed)) {
		app->typed[app->typed_length] = character;
		app->typed_length++;
		app->typed[app->typed_length] = '\0';
	}

	/* The first item whose name starts with the word. */
	for (index = 0; index < tab->listing.count; index++) {
		name = tab->listing.entries[index].name;
		for (at = 0; at < app->typed_length; at++) {
			left = name[at];
			right = app->typed[at];
			if (left >= 'A' && left <= 'Z')
				left = (char)(left - 'A' + 'a');
			if (right >= 'A' && right <= 'Z')
				right = (char)(right - 'A' + 'a');
			if (left != right)
				break;
		}

		/* The whole word matched: the item is found. */
		if (at == app->typed_length) {
			input_move(app, (int)index, 0);
			return;
		}
	}
}

/* Scrolls so that an item is wholly in sight. */
static void
input_show(
	struct fm_app *app,
	int index)
{
	struct fm_tab *tab;
	struct fm_rect item;
	int top;
	int bottom;

	/* No item, nothing to show. */
	tab = fm_ui_tab(app);
	if (index < 0 || (size_t)index >= tab->listing.count)
		return;

	/* Where the item is now, and the part of the panel items show in. */
	fm_view_item_rect(app, index, &item);
	top = app->layout.items.y;
	if (app->view == FM_VIEW_LIST)
		top += FM_LIST_HEADER;
	bottom = app->layout.items.y + app->layout.items.height;

	/* Above the top: scrolled up to it; below the bottom: down to it. */
	if (item.y < top)
		tab->scroll -= top - item.y;
	else if (item.y + item.height > bottom)
		tab->scroll += item.y + item.height - bottom;
	if (tab->scroll < 0)
		tab->scroll = 0;
}

/* Logs the selection (how many items and which has the cursor) when it changed; the tests read it. */
static void
input_report(
	struct fm_app *app)
{
	struct fm_tab *tab;
	uint64_t bytes;
	size_t count;

	/* The selection now. */
	tab = fm_ui_tab(app);
	count = fm_select_count(tab, &bytes);
	if (count == input_reported_count && tab->cursor == input_reported_cursor)
		return;

	/* A change is logged and remembered. */
	input_reported_count = count;
	input_reported_cursor = tab->cursor;
	fm_log("SELECT count=%lu cursor=%d bytes=%llu", (unsigned long)count, tab->cursor, (unsigned long long)bytes);
}

/* Handles the keys of the file operations; zero when the key is not one. */
static int
input_operation_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	int handled;

	/* Delete trashes, Shift+Delete deletes for good (asking), F2 renames, Ctrl+Shift+N makes a folder, Ctrl+Shift+Z redoes. */
	handled = 1;
	if (event->key == INPUT_KEY_DELETE && event->modifiers == 0U) {
		fm_action_trash(app);
	} else if (event->key == INPUT_KEY_DELETE && event->modifiers == FM_MOD_SHIFT) {
		fm_action_delete(app);
	} else if (event->key == INPUT_KEY_F2 && event->modifiers == 0U) {
		fm_action_rename_begin(app);
	} else if (event->key == INPUT_KEY_N && event->modifiers == (FM_MOD_CTRL | FM_MOD_SHIFT)) {
		fm_action_new_folder(app);
	} else if (event->key == INPUT_KEY_Z && event->modifiers == (FM_MOD_CTRL | FM_MOD_SHIFT)) {
		fm_action_undo(app, 1);
	} else if (event->key == INPUT_KEY_T && event->modifiers == (FM_MOD_CTRL | FM_MOD_ALT)) {
		fm_action_add_favorite(app);
	} else if (event->key == INPUT_KEY_P && event->modifiers == (FM_MOD_CTRL | FM_MOD_ALT)) {
		app->show_preview = !app->show_preview;
	} else if (event->key == INPUT_KEY_I && event->modifiers == FM_MOD_CTRL) {
		fm_info_open(app);
	} else if (event->key == INPUT_KEY_F && event->modifiers == FM_MOD_CTRL) {
		fm_search_focus(app);
	} else if (event->key >= INPUT_KEY_1 && event->key <= INPUT_KEY_1 + 8U && event->modifiers == FM_MOD_ALT) {
		fm_action_toggle_tag(app, (int)(event->key - INPUT_KEY_1));
	} else {
		handled = 0;
	}

	/* One of those was pressed. */
	if (handled != 0)
		return 1;

	/* The rest take Ctrl alone. */
	if (event->modifiers != FM_MOD_CTRL)
		return 0;

	/* Copy, cut, paste, duplicate and undo. */
	switch (event->key) {
	case INPUT_KEY_C:
		fm_action_copy(app, 0);
		return 1;
	case INPUT_KEY_X:
		fm_action_copy(app, 1);
		return 1;
	case INPUT_KEY_V:
		fm_action_paste(app);
		return 1;
	case INPUT_KEY_D:
		fm_action_duplicate(app);
		return 1;
	case INPUT_KEY_Z:
		fm_action_undo(app, 0);
		return 1;
	default:
		break;
	}

	/* Not an operation's key. */
	return 0;
}

/* Carries out a button of the frame: the question's answers, the trash's buttons, a task's cancel. */
static void
input_button(
	struct fm_app *app,
	int index)
{
	/* Each button. */
	if (index == FM_BUTTON_CANCEL) {
		fm_action_confirm(app, 0);
	} else if (index == FM_BUTTON_REPLACE) {
		fm_action_collision(app, FM_COLLISION_REPLACE);
	} else if (index == FM_BUTTON_SKIP) {
		fm_action_collision(app, FM_COLLISION_SKIP);
	} else if (index == FM_BUTTON_KEEP_BOTH) {
		fm_action_collision(app, FM_COLLISION_KEEP_BOTH);
	} else if (index == FM_BUTTON_MERGE) {
		fm_action_collision(app, FM_COLLISION_MERGE);
	} else if (index == FM_BUTTON_APPLY_ALL) {
		fm_action_collision_all(app);
	} else if (index == FM_BUTTON_CONFIRM) {
		fm_action_confirm(app, 1);
	} else if (index == FM_BUTTON_PUT_BACK) {
		fm_action_put_back(app);
	} else if (index == FM_BUTTON_EMPTY_TRASH) {
		fm_action_empty_trash(app);
	} else if (index == FM_BUTTON_LOOK_CLOSE) {
		fm_look_close(app);
	} else if (index == FM_BUTTON_HELP_CLOSE) {
		fm_help_close(app);
	} else if (index == FM_BUTTON_INFO_CLOSE || index == FM_BUTTON_INFO_CHECKSUM) {
		fm_info_button(app, index);
	} else if (index >= FM_BUTTON_OPENER && index < FM_BUTTON_OPENER + FM_OPENERS) {
		fm_info_button(app, index);
	} else if (index >= FM_BUTTON_REMOVE_PLACE) {
		fm_action_remove_favorite(app, index - FM_BUTTON_REMOVE_PLACE);
	} else if (index >= FM_BUTTON_TASK_CANCEL) {
		fm_action_cancel_task(app, index - FM_BUTTON_TASK_CANCEL);
	}
}

/* Handles a key while Quick Look is open: Space and Esc close it, the arrows go to the item before or after. */
static void
input_look_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	struct fm_tab *tab;

	/* Keys with modifiers do nothing here. */
	if (event->modifiers != 0U)
		return;

	/* Each key Quick Look knows. */
	tab = fm_ui_tab(app);
	switch (event->key) {
	case INPUT_KEY_SPACE:
	case INPUT_KEY_ESC:
		fm_look_close(app);
		break;
	case INPUT_KEY_LEFT:
	case INPUT_KEY_UP:
		fm_look_step(app, -1);
		input_show(app, tab->cursor);
		break;
	case INPUT_KEY_RIGHT:
	case INPUT_KEY_DOWN:
		fm_look_step(app, 1);
		input_show(app, tab->cursor);
		break;
	default:
		break;
	}

	/* The selection the key left. */
	input_report(app);
}

/* Handles a key while the information card is open: Esc and Ctrl+I close it. */
static void
input_info_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	/* Esc alone, or Ctrl+I again. */
	if (event->key == INPUT_KEY_ESC && event->modifiers == 0U)
		fm_info_close(app);
	else if (event->key == INPUT_KEY_I && event->modifiers == FM_MOD_CTRL)
		fm_info_close(app);
}

/* Carries out a key of the menus' table; zero when the key is not one. */
static int
input_shortcut_key(
	struct fm_app *app,
	const struct fm_event *event)
{
	size_t index;

	/* The key with exactly its modifiers. */
	for (index = 0; index < sizeof(input_shortcuts) / sizeof(input_shortcuts[0]); index++) {
		if (input_shortcuts[index].key != event->key)
			continue;
		if (input_shortcuts[index].modifiers != event->modifiers)
			continue;

		/* Its action. */
		fm_ui_action(app, input_shortcuts[index].action);
		return 1;
	}

	/* Not one of them. */
	return 0;
}
