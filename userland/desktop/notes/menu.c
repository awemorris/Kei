/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The menus of Notes: File, Edit, Page, Tool and View.
 *
 * The compositor draws them from the model given through libkeiland (the
 * System Menu, WS070) and runs their shortcuts: a shortcut's key is the
 * menu's and does not reach the window, so the keys Notes handles itself
 * (main.c) are the ones without a menu item and every key when there is no
 * System Menu.  A choice arrives as an action while the window's events
 * are dispatched and waits in the window's queue for the main loop.
 *
 * The shortcuts are the standard ones (design-input-notes.md D8): Ctrl+N a
 * new page, Ctrl+O open, Ctrl+S save, Ctrl+Shift+S save as, Ctrl+W close, Ctrl+Z undo,
 * Ctrl+Shift+Z redo (Ctrl+Y too, as a key of Notes'), Page Up and Page Down
 * the previous and next page, F11 fullscreen.
 */

#include "app.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The top-level items. */
#define MENU_FILE		1U
#define MENU_EDIT		2U
#define MENU_PAGE		3U
#define MENU_TOOL		4U
#define MENU_VIEW		5U

/* File. */
#define MENU_NEW_PAGE		10U
#define MENU_OPEN		11U
#define MENU_SAVE		12U
#define MENU_FILE_LINE		13U
#define MENU_CLOSE		14U
#define MENU_SAVE_AS		15U

/* Edit. */
#define MENU_UNDO		20U
#define MENU_REDO		21U

/* Page. */
#define MENU_PREVIOUS_PAGE	30U
#define MENU_NEXT_PAGE		31U

/* Tool. */
#define MENU_PEN		40U
#define MENU_HIGHLIGHTER	41U
#define MENU_ERASER		42U

/* View. */
#define MENU_FULLSCREEN		50U

/* The keysyms of the shortcuts' keys that are not letters. */
#define MENU_KEY_PAGE_UP	0xff55U
#define MENU_KEY_PAGE_DOWN	0xff56U
#define MENU_KEY_F11		0xffc8U

/*
 * One item of the menus as Notes builds them: its ID, its parent, its
 * type, its label, its action, its role and its shortcut.
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

/* The menus, in the order they are shown. */
static const struct menu_item menu_items[] = {
	{ MENU_FILE, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "File", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_NEW_PAGE, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "New Page", NOTES_ACTION_NEW_PAGE, KEILAND_MENU_ROLE_NEW, KEILAND_MENU_CTRL, 'n' },
	{ MENU_OPEN, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Open...", NOTES_ACTION_OPEN, KEILAND_MENU_ROLE_OPEN, KEILAND_MENU_CTRL, 'o' },
	{ MENU_SAVE, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Save", NOTES_ACTION_SAVE, KEILAND_MENU_ROLE_SAVE, KEILAND_MENU_CTRL, 's' },
	{ MENU_SAVE_AS, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Save As...", NOTES_ACTION_SAVE_AS, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 's' },
	{ MENU_FILE_LINE, MENU_FILE, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CLOSE, MENU_FILE, KEILAND_MENU_ITEM_NORMAL, "Close", NOTES_ACTION_CLOSE, KEILAND_MENU_ROLE_CLOSE, KEILAND_MENU_CTRL, 'w' },
	{ MENU_EDIT, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Edit", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_UNDO, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Undo", NOTES_ACTION_UNDO, KEILAND_MENU_ROLE_UNDO, KEILAND_MENU_CTRL, 'z' },
	{ MENU_REDO, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Redo", NOTES_ACTION_REDO, KEILAND_MENU_ROLE_REDO, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'z' },
	{ MENU_PAGE, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Page", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_PREVIOUS_PAGE, MENU_PAGE, KEILAND_MENU_ITEM_NORMAL, "Previous Page", NOTES_ACTION_PREVIOUS_PAGE, KEILAND_MENU_ROLE_NONE, 0U, MENU_KEY_PAGE_UP },
	{ MENU_NEXT_PAGE, MENU_PAGE, KEILAND_MENU_ITEM_NORMAL, "Next Page", NOTES_ACTION_NEXT_PAGE, KEILAND_MENU_ROLE_NONE, 0U, MENU_KEY_PAGE_DOWN },
	{ MENU_TOOL, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Tool", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_PEN, MENU_TOOL, KEILAND_MENU_ITEM_RADIO, "Pen", NOTES_ACTION_PEN, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_HIGHLIGHTER, MENU_TOOL, KEILAND_MENU_ITEM_RADIO, "Marker", NOTES_ACTION_HIGHLIGHTER, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_ERASER, MENU_TOOL, KEILAND_MENU_ITEM_RADIO, "Eraser", NOTES_ACTION_ERASER, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_VIEW, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "View", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_FULLSCREEN, MENU_VIEW, KEILAND_MENU_ITEM_CHECKBOX, "Fullscreen", NOTES_ACTION_FULLSCREEN, KEILAND_MENU_ROLE_FULLSCREEN, 0U, MENU_KEY_F11 }
};

static void menu_activated(void *data, struct keiland_window_menu *window_menu, uint32_t item, uint32_t action, struct wl_seat *seat, uint32_t serial);
static int menu_build(struct notes_window *window);
static int menu_is(uint32_t tool, uint32_t action);

/* What the window menu tells Notes: only the choices. */
static const struct keiland_window_menu_listener menu_listener = {
	menu_activated, NULL, NULL
};

/*
 * Gives the compositor the window's menus.
 *
 * Returns 0, or an errno value (ENOTSUP without the System Menu, when the
 * keys of main.c are the only shortcuts).
 */
int
notes_menu_open(
	struct notes_window *window)
{
	int error;

	/* The connection's menu service. */
	window->menu_service = keiland_menu_service_open(window->display);
	if (window->menu_service == NULL)
		return errno;

	/* The menu. */
	window->menu = keiland_menu_create(window->menu_service);
	if (window->menu == NULL) {
		error = errno;
		notes_menu_close(window);
		return error;
	}

	/* Its items. */
	error = menu_build(window);
	if (error != 0) {
		notes_menu_close(window);
		return error;
	}

	/* Its place on the window. */
	window->window_menu = keiland_window_menu_create(window->menu_service, window->toplevel, &menu_listener, window);
	if (window->window_menu == NULL) {
		error = errno;
		notes_menu_close(window);
		return error;
	}

	/* Shows it. */
	error = keiland_window_menu_set(window->window_menu, window->menu);
	if (error != 0) {
		notes_menu_close(window);
		return error;
	}

	/* Succeeded: the compositor shows the menus. */
	return 0;
}

/*
 * Shows a state in the menus in one transaction: Undo and Redo enabled,
 * the pages' items, the tool's radio item and Fullscreen checked.
 */
void
notes_menu_refresh(
	struct notes_window *window,
	const struct notes_ui_state *state)
{
	struct keiland_menu *menu;
	int earlier;
	int later;
	int error;

	/* Nothing to show without a menu. */
	menu = window->menu;
	if (menu == NULL)
		return;

	/* The transaction. */
	error = keiland_menu_begin(menu);
	if (error != 0)
		return;

	/* Undo and Redo while the history allows. */
	(void)keiland_menu_set_enabled(menu, MENU_UNDO, state->can_undo);
	(void)keiland_menu_set_enabled(menu, MENU_REDO, state->can_redo);

	/* The page before and the page after, when there are such pages. */
	earlier = 0;
	if (state->page > 0U)
		earlier = 1;
	later = 0;
	if (state->page + 1U < state->page_count)
		later = 1;
	(void)keiland_menu_set_enabled(menu, MENU_PREVIOUS_PAGE, earlier);
	(void)keiland_menu_set_enabled(menu, MENU_NEXT_PAGE, later);

	/* The tool's radio item, and Fullscreen. */
	(void)keiland_menu_set_checked(menu, MENU_PEN, menu_is(state->tool, NOTES_ACTION_PEN));
	(void)keiland_menu_set_checked(menu, MENU_HIGHLIGHTER, menu_is(state->tool, NOTES_ACTION_HIGHLIGHTER));
	(void)keiland_menu_set_checked(menu, MENU_ERASER, menu_is(state->tool, NOTES_ACTION_ERASER));
	(void)keiland_menu_set_checked(menu, MENU_FULLSCREEN, state->fullscreen);

	/* The state is shown together. */
	(void)keiland_menu_commit(menu);
}

/*
 * Takes the menus away.
 */
void
notes_menu_close(
	struct notes_window *window)
{
	/* The window's place, the menu and the service, where made. */
	if (window->window_menu != NULL)
		keiland_window_menu_destroy(window->window_menu);
	if (window->menu != NULL)
		keiland_menu_destroy(window->menu);
	if (window->menu_service != NULL)
		keiland_menu_service_close(window->menu_service);
	window->window_menu = NULL;
	window->menu = NULL;
	window->menu_service = NULL;
}

/* Queues a choice of the menus for the main loop. */
static void
menu_activated(
	void *data,
	struct keiland_window_menu *window_menu,
	uint32_t item,
	uint32_t action,
	struct wl_seat *seat,
	uint32_t serial)
{
	struct notes_window *window;

	/* The log line the tests read. */
	(void)window_menu;
	(void)seat;
	window = data;
	printf("NOTES MENU item=%u action=%u serial=%u\n", item, action, serial);
	fflush(stdout);

	/* A full queue drops the choice. */
	if (window->action_count >= NOTES_ACTIONS)
		return;

	/* Succeeded: the choice waits for the main loop. */
	window->actions[window->action_count] = action;
	window->action_count++;
}

/* Gives the compositor every item, with its role and shortcut, in one transaction. */
static int
menu_build(
	struct notes_window *window)
{
	const struct menu_item *item;
	unsigned index;
	int error;

	/* The transaction. */
	error = keiland_menu_begin(window->menu);
	if (error != 0)
		return error;

	/* Each item in its order under its parent. */
	for (index = 0; index < sizeof(menu_items) / sizeof(menu_items[0]); index++) {
		item = &menu_items[index];
		error = keiland_menu_append(window->menu, item->id, item->parent, item->type, item->label, item->action);
		if (error != 0)
			return error;

		/* Its role, when it has one. */
		if (item->role != KEILAND_MENU_ROLE_NONE) {
			error = keiland_menu_set_role(window->menu, item->id, item->role);
			if (error != 0)
				return error;
		}

		/* Its shortcut, when it has one. */
		if (item->keysym != 0U) {
			error = keiland_menu_set_shortcut(window->menu, item->id, item->modifiers, item->keysym);
			if (error != 0)
				return error;
		}
	}

	/* The items are shown together. */
	error = keiland_menu_commit(window->menu);
	if (error != 0)
		return error;

	/* Succeeded: the menus are built. */
	return 0;
}

/* Tells whether the tool chosen is the one an item stands for (1) or not (0). */
static int
menu_is(
	uint32_t tool,
	uint32_t action)
{
	/* The item's tool. */
	if (tool == action)
		return 1;

	/* Another tool. */
	return 0;
}
