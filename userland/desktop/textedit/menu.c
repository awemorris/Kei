/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The menus of Text Editor in zdesktop's System Menu (plan/ws092/design.md
 * section 13): File, Edit, View and Help, and the context menu of the text
 * (Undo, Redo, Cut, Copy, Paste, Select All).  zdesktop draws them and
 * chooses an item for its shortcut; the choice comes back as an action
 * queued among the window's inputs.  A compositor without the System Menu
 * leaves the editor without menus, and the keys work as they do with them.
 */

#include "window.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The submenus. */
#define MENU_FILE		1U
#define MENU_EDIT		2U
#define MENU_VIEW		3U
#define MENU_HELP		4U

/* The items of File. */
#define MENU_NEW		10U
#define MENU_OPEN		11U
#define MENU_RECENT		18U
#define MENU_FILE_LINE		12U
#define MENU_SAVE		13U
#define MENU_SAVE_AS		14U
#define MENU_FILE_LINE_2	15U
#define MENU_CLOSE		16U
#define MENU_QUIT		17U

/* The items of Edit. */
#define MENU_UNDO		20U
#define MENU_REDO		21U
#define MENU_EDIT_LINE		22U
#define MENU_CUT		23U
#define MENU_COPY		24U
#define MENU_PASTE		25U
#define MENU_SELECT_ALL		26U
#define MENU_EDIT_LINE_2	27U
#define MENU_FIND		28U
#define MENU_FIND_NEXT		29U
#define MENU_FIND_PREVIOUS	30U
#define MENU_REPLACE		31U

/* The items of View. */
#define MENU_LINE_NUMBERS	40U
#define MENU_WORD_WRAP		41U
#define MENU_VIEW_LINE		42U
#define MENU_BIGGER		43U
#define MENU_SMALLER		44U
#define MENU_ACTUAL_SIZE	45U

/* The item of Help. */
#define MENU_ABOUT		50U

/* The items of the context menu. */
#define MENU_CONTEXT_UNDO	60U
#define MENU_CONTEXT_REDO	61U
#define MENU_CONTEXT_LINE	62U
#define MENU_CONTEXT_CUT	63U
#define MENU_CONTEXT_COPY	64U
#define MENU_CONTEXT_PASTE	65U
#define MENU_CONTEXT_LINE_2	66U
#define MENU_CONTEXT_ALL	67U

/*
 * The items of File > Open Recent (ws128-p003): one a file from the first,
 * and the line that says there is none.  They are made anew whenever the
 * recent files change (te_menu_recent).
 */
#define MENU_RECENT_FIRST	70U
#define MENU_RECENT_NONE	80U

/* The keysyms of the shortcuts' keys that are not letters. */
#define MENU_KEY_EQUAL		0x3dU
#define MENU_KEY_MINUS		0x2dU
#define MENU_KEY_ZERO		0x30U

/*
 * One item of the menus as the editor builds them: its ID, its parent,
 * its type, its label, its action, its role and its shortcut.
 */
struct menu_item {
	uint32_t id;
	uint32_t parent;
	unsigned type;
	const char *label;
	uint32_t action;
	unsigned role;
	unsigned modifiers;
	uint32_t keysym;
};

/* The window's menus, in the order they are shown. */
static const struct menu_item menu_items[] = {
	{ MENU_FILE, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "File", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_NEW, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "New", TE_ACTION_NEW, KEILAND_MENU_ROLE_NEW, KEILAND_MENU_CTRL, 'n' },
	{ MENU_OPEN, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Open...", TE_ACTION_OPEN, KEILAND_MENU_ROLE_OPEN, KEILAND_MENU_CTRL, 'o' },
	{ MENU_RECENT, MENU_FILE, KEILAND_MENU_ITEM_SUBMENU, "Open Recent", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_FILE_LINE, MENU_FILE, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SAVE, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Save", TE_ACTION_SAVE, KEILAND_MENU_ROLE_SAVE, KEILAND_MENU_CTRL, 's' },
	{ MENU_SAVE_AS, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Save As...", TE_ACTION_SAVE_AS, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 's' },
	{ MENU_FILE_LINE_2, MENU_FILE, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CLOSE, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Close", TE_ACTION_CLOSE, KEILAND_MENU_ROLE_CLOSE, KEILAND_MENU_CTRL, 'w' },
	{ MENU_QUIT, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Quit Text Editor", TE_ACTION_QUIT, KEILAND_MENU_ROLE_QUIT, KEILAND_MENU_CTRL, 'q' },
	{ MENU_EDIT, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Edit", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_UNDO, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Undo", TE_ACTION_UNDO, KEILAND_MENU_ROLE_UNDO, KEILAND_MENU_CTRL, 'z' },
	{ MENU_REDO, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Redo", TE_ACTION_REDO, KEILAND_MENU_ROLE_REDO, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'z' },
	{ MENU_EDIT_LINE, MENU_EDIT, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CUT, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Cut", TE_ACTION_CUT, KEILAND_MENU_ROLE_CUT, KEILAND_MENU_CTRL, 'x' },
	{ MENU_COPY, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Copy", TE_ACTION_COPY, KEILAND_MENU_ROLE_COPY, KEILAND_MENU_CTRL, 'c' },
	{ MENU_PASTE, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Paste", TE_ACTION_PASTE, KEILAND_MENU_ROLE_PASTE, KEILAND_MENU_CTRL, 'v' },
	{ MENU_SELECT_ALL, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Select All", TE_ACTION_SELECT_ALL, KEILAND_MENU_ROLE_SELECT_ALL, KEILAND_MENU_CTRL, 'a' },
	{ MENU_EDIT_LINE_2, MENU_EDIT, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_FIND, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Find...", TE_ACTION_FIND, KEILAND_MENU_ROLE_FIND, KEILAND_MENU_CTRL, 'f' },
	{ MENU_FIND_NEXT, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Find Next", TE_ACTION_FIND_NEXT, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL, 'g' },
	{ MENU_FIND_PREVIOUS, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Find Previous", TE_ACTION_FIND_PREVIOUS, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'g' },
	{ MENU_REPLACE, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Replace...", TE_ACTION_REPLACE, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL, 'h' },
	{ MENU_VIEW, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "View", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_LINE_NUMBERS, MENU_VIEW, KEILAND_MENU_ITEM_CHECKBOX, "Line Numbers", TE_ACTION_LINE_NUMBERS, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_WORD_WRAP, MENU_VIEW, KEILAND_MENU_ITEM_CHECKBOX, "Word Wrap", TE_ACTION_WORD_WRAP, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_VIEW_LINE, MENU_VIEW, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_BIGGER, MENU_VIEW, KEILAND_MENU_ITEM_NORMAL, "Bigger Text", TE_ACTION_BIGGER, KEILAND_MENU_ROLE_ZOOM_IN, KEILAND_MENU_CTRL, MENU_KEY_EQUAL },
	{ MENU_SMALLER, MENU_VIEW, KEILAND_MENU_ITEM_NORMAL, "Smaller Text", TE_ACTION_SMALLER, KEILAND_MENU_ROLE_ZOOM_OUT, KEILAND_MENU_CTRL, MENU_KEY_MINUS },
	{ MENU_ACTUAL_SIZE, MENU_VIEW, KEILAND_MENU_ITEM_NORMAL, "Actual Size", TE_ACTION_ACTUAL_SIZE, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL, MENU_KEY_ZERO },
	{ MENU_HELP, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Help", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_ABOUT, MENU_HELP, KEILAND_MENU_ITEM_NORMAL, "About Text Editor", TE_ACTION_ABOUT, KEILAND_MENU_ROLE_ABOUT, 0U, 0U }
};

/* The context menu of the text, in its order. */
static const struct menu_item menu_context_items[] = {
	{ MENU_CONTEXT_UNDO, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_NORMAL, "Undo", TE_ACTION_UNDO, KEILAND_MENU_ROLE_UNDO, 0U, 0U },
	{ MENU_CONTEXT_REDO, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_NORMAL, "Redo", TE_ACTION_REDO, KEILAND_MENU_ROLE_REDO, 0U, 0U },
	{ MENU_CONTEXT_LINE, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CONTEXT_CUT, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_NORMAL, "Cut", TE_ACTION_CUT, KEILAND_MENU_ROLE_CUT, 0U, 0U },
	{ MENU_CONTEXT_COPY, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_NORMAL, "Copy", TE_ACTION_COPY, KEILAND_MENU_ROLE_COPY, 0U, 0U },
	{ MENU_CONTEXT_PASTE, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_NORMAL, "Paste", TE_ACTION_PASTE, KEILAND_MENU_ROLE_PASTE, 0U, 0U },
	{ MENU_CONTEXT_LINE_2, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CONTEXT_ALL, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_NORMAL, "Select All", TE_ACTION_SELECT_ALL, KEILAND_MENU_ROLE_SELECT_ALL, 0U, 0U }
};

static void menu_activated(void *data, struct keiland_window_menu *window_menu, uint32_t item, uint32_t action, struct wl_seat *seat, uint32_t serial);
static void menu_context_activated(void *data, struct keiland_context_menu *context_menu, uint32_t item, uint32_t action, uint32_t serial);
static void menu_context_done(void *data, struct keiland_context_menu *context_menu);
static int menu_build(struct keiland_menu *model, const struct menu_item *items, size_t count);
static int menu_state(struct te_menu *menu, const struct te_state *state);

/* What the window menu tells the editor: only the choices. */
static const struct keiland_window_menu_listener menu_listener = {
	menu_activated, NULL, NULL
};

/* What the context menu tells the editor: the choice, and that it closed. */
static const struct keiland_context_menu_listener menu_context_listener = {
	menu_context_activated, menu_context_done
};

/*
 * Gives zdesktop the window's menus and the context menu's model, showing
 * a state.
 *
 * Returns 0, also when the compositor has no System Menu (the editor then
 * has no menus), or an errno value when the menus could not be made.
 */
int
te_menu_open(
	struct te_menu *menu,
	struct te_window *window,
	const struct te_state *state)
{
	int error;

	/* Nothing yet but the window. */
	memset(menu, 0, sizeof(*menu));
	menu->window = window;

	/* The connection's menu service; a compositor without one leaves the editor without menus. */
	menu->service = keiland_menu_service_open(kui_window_display(window->kui));
	if (menu->service == NULL) {
		te_log("MENU none errno=%d", errno);
		return 0;
	}

	/* The menu, empty until it is built. */
	menu->menu = keiland_menu_create(menu->service);
	if (menu->menu == NULL)
		return errno;

	/* The window's place for a menu, which tells the editor what is chosen. */
	menu->window_menu = keiland_window_menu_create(menu->service, kui_window_toplevel(window->kui), &menu_listener, menu);
	if (menu->window_menu == NULL)
		return errno;

	/* The items in one transaction. */
	error = menu_build(menu->menu, menu_items, sizeof(menu_items) / sizeof(menu_items[0]));
	if (error != 0)
		return error;

	/* The window shows the menu from now on. */
	error = keiland_window_menu_set(menu->window_menu, menu->menu);
	if (error != 0)
		return error;

	/* The context menu's model. */
	menu->context = keiland_menu_create(menu->service);
	if (menu->context == NULL)
		return errno;
	error = menu_build(menu->context, menu_context_items, sizeof(menu_context_items) / sizeof(menu_context_items[0]));
	if (error != 0)
		return error;

	/* The state they show. */
	error = menu_state(menu, state);
	if (error != 0)
		return error;

	/* Succeeded: the menus are zdesktop's to show. */
	te_log("MENU ready items=%u", (unsigned)(sizeof(menu_items) / sizeof(menu_items[0])));
	return 0;
}

/*
 * Tells the menus the editor's state when it differs from what they show.
 */
void
te_menu_refresh(
	struct te_menu *menu,
	const struct te_state *state)
{
	int same;
	int error;

	/* Without menus nothing is sent. */
	if (menu->menu == NULL || menu->context == NULL)
		return;

	/* Nor when the state is the one the menus show. */
	same = memcmp(state, &menu->shown, sizeof(*state));
	if (same == 0)
		return;

	/* The new state; a refusal is logged and the menus stay as they were. */
	error = menu_state(menu, state);
	if (error != 0)
		te_log("MENU update-failed errno=%d", error);
}

/*
 * Shows the recent files in File > Open Recent (ws128-p003): the items of
 * the last list go, and one comes for each file, newest first, greyed when
 * the file is no longer there; without any, one greyed line says so.
 */
void
te_menu_recent(
	struct te_menu *menu,
	const struct te_app *app)
{
	struct keiland_menu *model;
	const char *name;
	const char *slash;
	uint32_t id;
	size_t index;
	int error;

	/* Without menus nothing is sent. */
	if (menu->menu == NULL)
		return;

	/* The last list's items go (an item that is not there is no error worth reporting). */
	model = menu->menu;
	error = keiland_menu_begin(model);
	if (error != 0) {
		te_log("MENU recent-failed errno=%d", error);
		return;
	}

	/* Each item of the last list, and the line for none. */
	for (index = 0; index < menu->recent_shown; index++)
		(void)keiland_menu_remove(model, MENU_RECENT_FIRST + (uint32_t)index);
	(void)keiland_menu_remove(model, MENU_RECENT_NONE);

	/* A file an item, named by its file name; one that is gone is greyed. */
	error = 0;
	for (index = 0; index < app->recent_count && error == 0; index++) {
		name = app->recent[index];
		slash = strrchr(name, '/');
		if (slash != NULL && slash[1] != '\0')
			name = slash + 1;
		id = MENU_RECENT_FIRST + (uint32_t)index;
		error = keiland_menu_append(model, id, MENU_RECENT, KEILAND_MENU_ITEM_NORMAL, name, TE_ACTION_RECENT_FIRST + (uint32_t)index);
		if (error == 0)
			error = keiland_menu_set_enabled(model, id, app->recent_present[index]);
	}

	/* No recent file: a greyed line says so. */
	if (error == 0 && app->recent_count == 0U) {
		error = keiland_menu_append(model, MENU_RECENT_NONE, MENU_RECENT, KEILAND_MENU_ITEM_NORMAL, "No Recent Files", 0U);
		if (error == 0)
			error = keiland_menu_set_enabled(model, MENU_RECENT_NONE, 0);
	}

	/* The list is shown together (a refusal still ends the transaction). */
	menu->recent_shown = app->recent_count;
	(void)keiland_menu_commit(model);
	if (error != 0) {
		te_log("MENU recent-failed errno=%d", error);
		return;
	}

	/* The log says how many are shown. */
	te_log("MENU recent count=%lu", (unsigned long)app->recent_count);
}

/*
 * Opens the context menu of the text at a place of the window (one at a
 * time).
 */
void
te_menu_popup(
	struct te_menu *menu,
	int x,
	int y)
{
	/* Without menus, or with one open, nothing opens. */
	if (menu->context == NULL || menu->popup != NULL)
		return;

	/* The menu at the place, for the last press. */
	menu->popup = keiland_menu_popup(menu->service, menu->context, kui_window_surface(menu->window->kui), x, y, kui_window_seat(menu->window->kui), kui_window_press_serial(menu->window->kui), &menu_context_listener, menu);
	if (menu->popup == NULL) {
		te_log("MENU popup-failed errno=%d", errno);
		return;
	}

	/* Succeeded: the menu is open. */
	te_log("MENU popup x=%d y=%d", x, y);
}

/*
 * Takes the menus away from zdesktop (before the window goes).
 */
void
te_menu_close(
	struct te_menu *menu)
{
	/* The context menu, the window's place, the menus, then the service. */
	if (menu->popup != NULL)
		keiland_context_menu_destroy(menu->popup);
	if (menu->window_menu != NULL)
		keiland_window_menu_destroy(menu->window_menu);
	if (menu->context != NULL)
		keiland_menu_destroy(menu->context);
	if (menu->menu != NULL)
		keiland_menu_destroy(menu->menu);
	if (menu->service != NULL)
		keiland_menu_service_close(menu->service);
	memset(menu, 0, sizeof(*menu));
}

/* Queues a chosen action among the window's inputs. */
static void
menu_activated(
	void *data,
	struct keiland_window_menu *window_menu,
	uint32_t item,
	uint32_t action,
	struct wl_seat *seat,
	uint32_t serial)
{
	struct te_menu *menu;

	/* The menus whose item was chosen. */
	(void)window_menu;
	(void)seat;
	menu = data;

	/* The log line the tests read, and the action. */
	te_log("MENU item=%u action=%u serial=%u", item, action, serial);
	kui_window_set_serial(menu->window->kui, serial);
	te_window_action(menu->window, action);
}

/* Queues the action chosen in the context menu. */
static void
menu_context_activated(
	void *data,
	struct keiland_context_menu *context_menu,
	uint32_t item,
	uint32_t action,
	uint32_t serial)
{
	struct te_menu *menu;

	/* The action, with the choice's serial for a copy. */
	(void)context_menu;
	menu = data;
	te_log("MENU context item=%u action=%u", item, action);
	kui_window_set_serial(menu->window->kui, serial);
	te_window_action(menu->window, action);
}

/* The context menu closed: it goes. */
static void
menu_context_done(
	void *data,
	struct keiland_context_menu *context_menu)
{
	struct te_menu *menu;

	/* Destroyed, so that another may open. */
	menu = data;
	keiland_context_menu_destroy(context_menu);
	if (menu->popup == context_menu)
		menu->popup = NULL;
}

/* Gives zdesktop a menu's items, with their roles and shortcuts, in one transaction. */
static int
menu_build(
	struct keiland_menu *model,
	const struct menu_item *items,
	size_t count)
{
	const struct menu_item *item;
	size_t index;
	int error;

	/* The transaction. */
	error = keiland_menu_begin(model);
	if (error != 0)
		return error;

	/* Each item in its order under its parent. */
	for (index = 0; index < count; index++) {
		item = &items[index];
		error = keiland_menu_append(model, item->id, item->parent, item->type, item->label, item->action);
		if (error != 0)
			return error;

		/* Its role, when it has one. */
		if (item->role != KEILAND_MENU_ROLE_NONE) {
			error = keiland_menu_set_role(model, item->id, item->role);
			if (error != 0)
				return error;
		}

		/* Its shortcut, when it has one. */
		if (item->keysym != 0U) {
			error = keiland_menu_set_shortcut(model, item->id, item->modifiers, item->keysym);
			if (error != 0)
				return error;
		}
	}

	/* The items are shown together. */
	error = keiland_menu_commit(model);
	if (error != 0)
		return error;

	/* Succeeded: the menu is built. */
	return 0;
}

/*
 * Shows a state in the menus: Undo, Redo, Cut and Copy enabled when they
 * can act, and View's switches checked.
 */
static int
menu_state(
	struct te_menu *menu,
	const struct te_state *state)
{
	struct keiland_menu *model;
	int error;

	/* The window's menus, in one transaction. */
	model = menu->menu;
	error = keiland_menu_begin(model);
	if (error != 0)
		return error;
	error = keiland_menu_set_enabled(model, MENU_UNDO, state->can_undo);
	if (error == 0)
		error = keiland_menu_set_enabled(model, MENU_REDO, state->can_redo);
	if (error == 0)
		error = keiland_menu_set_enabled(model, MENU_CUT, state->selected);
	if (error == 0)
		error = keiland_menu_set_enabled(model, MENU_COPY, state->selected);
	if (error == 0)
		error = keiland_menu_set_checked(model, MENU_LINE_NUMBERS, state->line_numbers);
	if (error == 0)
		error = keiland_menu_set_checked(model, MENU_WORD_WRAP, state->wrap);

	/* A refused change still ends the transaction, and is reported. */
	if (error != 0) {
		(void)keiland_menu_commit(model);
		return error;
	}

	/* The changes are shown together. */
	error = keiland_menu_commit(model);
	if (error != 0)
		return error;

	/* The context menu, likewise. */
	model = menu->context;
	error = keiland_menu_begin(model);
	if (error != 0)
		return error;
	error = keiland_menu_set_enabled(model, MENU_CONTEXT_UNDO, state->can_undo);
	if (error == 0)
		error = keiland_menu_set_enabled(model, MENU_CONTEXT_REDO, state->can_redo);
	if (error == 0)
		error = keiland_menu_set_enabled(model, MENU_CONTEXT_CUT, state->selected);
	if (error == 0)
		error = keiland_menu_set_enabled(model, MENU_CONTEXT_COPY, state->selected);

	/* A refused change still ends the transaction, and is reported. */
	if (error != 0) {
		(void)keiland_menu_commit(model);
		return error;
	}

	/* The changes are shown together. */
	error = keiland_menu_commit(model);
	if (error != 0)
		return error;

	/* Succeeded: the menus show the state. */
	menu->shown = *state;
	return 0;
}
