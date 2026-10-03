/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The terminal's menus: Shell, Edit, View, Session and Help.
 *
 * zdesktop draws them (in the window's title bar, or in the system bar
 * while the window is docked) from the model given through libkeiland
 * (the System Menu, WS070).  A choice arrives as an action number while the
 * window's events are dispatched; it is queued here and carried out by the
 * main loop, which then tells the menus the terminal's state (Copy and
 * Paste enabled, the font's size checked, Fullscreen and Treat
 * Ambiguous-Width Characters as Wide checked) in one transaction.  Without
 * the System Menu the terminal simply has no menus.
 */

#include "terminal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The top-level items. */
#define MENU_SHELL		1U
#define MENU_EDIT		2U
#define MENU_VIEW		3U
#define MENU_SESSION		4U
#define MENU_HELP		5U

/* Shell. */
#define MENU_NEW_WINDOW		10U
#define MENU_SHELL_LINE		11U
#define MENU_CLOSE		12U
#define MENU_NEW_TAB		13U
#define MENU_CLOSE_TAB		14U

/* Edit. */
#define MENU_COPY		20U
#define MENU_PASTE		21U
#define MENU_EDIT_LINE		22U
#define MENU_SELECT_ALL		23U

/* View, and its Text Size submenu. */
#define MENU_ZOOM_IN		30U
#define MENU_ZOOM_OUT		31U
#define MENU_ZOOM_NORMAL	32U
#define MENU_TEXT_SIZE		33U
#define MENU_VIEW_LINE		34U
#define MENU_FULLSCREEN		35U
#define MENU_AMBIGUOUS_WIDE	36U
#define MENU_SIZE_SMALL		40U
#define MENU_SIZE_MEDIUM	41U
#define MENU_SIZE_LARGE		42U
#define MENU_SIZE_HUGE		43U

/* Session. */
#define MENU_INTERRUPT		50U
#define MENU_END_OF_FILE	51U
#define MENU_SESSION_LINE	52U
#define MENU_CLEAR		53U
#define MENU_RESET		54U

/* Help. */
#define MENU_ABOUT		60U

/* The keysyms of the shortcuts' keys that are not letters. */
#define MENU_KEY_PLUS		0x2bU
#define MENU_KEY_MINUS		0x2dU
#define MENU_KEY_ZERO		0x30U
#define MENU_KEY_F11		0xffc8U

/*
 * One item of the menus as the terminal builds them: its ID, its parent,
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

/* The menus, in the order they are shown. */
static const struct menu_item menu_items[] = {
	{ MENU_SHELL, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Shell", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_NEW_WINDOW, MENU_SHELL, KEILAND_MENU_ITEM_NORMAL, "New Window", TERMINAL_ACTION_NEW_WINDOW, KEILAND_MENU_ROLE_NEW, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'n' },
	{ MENU_NEW_TAB, MENU_SHELL, KEILAND_MENU_ITEM_NORMAL, "New Tab", TERMINAL_ACTION_NEW_TAB, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 't' },
	{ MENU_SHELL_LINE, MENU_SHELL, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CLOSE_TAB, MENU_SHELL, KEILAND_MENU_ITEM_NORMAL, "Close Tab", TERMINAL_ACTION_CLOSE_TAB, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'w' },
	{ MENU_CLOSE, MENU_SHELL, KEILAND_MENU_ITEM_NORMAL, "Close Window", TERMINAL_ACTION_CLOSE, KEILAND_MENU_ROLE_CLOSE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'q' },
	{ MENU_EDIT, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Edit", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_COPY, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Copy", TERMINAL_ACTION_COPY, KEILAND_MENU_ROLE_COPY, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'c' },
	{ MENU_PASTE, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Paste", TERMINAL_ACTION_PASTE, KEILAND_MENU_ROLE_PASTE, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'v' },
	{ MENU_EDIT_LINE, MENU_EDIT, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SELECT_ALL, MENU_EDIT, KEILAND_MENU_ITEM_NORMAL, "Select All", TERMINAL_ACTION_SELECT_ALL, KEILAND_MENU_ROLE_SELECT_ALL, KEILAND_MENU_CTRL | KEILAND_MENU_SHIFT, 'a' },
	{ MENU_VIEW, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "View", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_ZOOM_IN, MENU_VIEW, KEILAND_MENU_ITEM_NORMAL, "Zoom In", TERMINAL_ACTION_ZOOM_IN, KEILAND_MENU_ROLE_ZOOM_IN, KEILAND_MENU_CTRL, MENU_KEY_PLUS },
	{ MENU_ZOOM_OUT, MENU_VIEW, KEILAND_MENU_ITEM_NORMAL, "Zoom Out", TERMINAL_ACTION_ZOOM_OUT, KEILAND_MENU_ROLE_ZOOM_OUT, KEILAND_MENU_CTRL, MENU_KEY_MINUS },
	{ MENU_ZOOM_NORMAL, MENU_VIEW, KEILAND_MENU_ITEM_NORMAL, "Normal Size", TERMINAL_ACTION_ZOOM_NORMAL, KEILAND_MENU_ROLE_NONE, KEILAND_MENU_CTRL, MENU_KEY_ZERO },
	{ MENU_TEXT_SIZE, MENU_VIEW, KEILAND_MENU_ITEM_SUBMENU, "Text Size", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SIZE_SMALL, MENU_TEXT_SIZE, KEILAND_MENU_ITEM_RADIO, "Small", TERMINAL_ACTION_SIZE_SMALL, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SIZE_MEDIUM, MENU_TEXT_SIZE, KEILAND_MENU_ITEM_RADIO, "Medium", TERMINAL_ACTION_SIZE_MEDIUM, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SIZE_LARGE, MENU_TEXT_SIZE, KEILAND_MENU_ITEM_RADIO, "Large", TERMINAL_ACTION_SIZE_LARGE, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SIZE_HUGE, MENU_TEXT_SIZE, KEILAND_MENU_ITEM_RADIO, "Huge", TERMINAL_ACTION_SIZE_HUGE, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_VIEW_LINE, MENU_VIEW, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_FULLSCREEN, MENU_VIEW, KEILAND_MENU_ITEM_CHECKBOX, "Fullscreen", TERMINAL_ACTION_FULLSCREEN, KEILAND_MENU_ROLE_FULLSCREEN, 0U, MENU_KEY_F11 },
	{ MENU_AMBIGUOUS_WIDE, MENU_VIEW, KEILAND_MENU_ITEM_CHECKBOX, "Treat Ambiguous-Width Characters as Wide", TERMINAL_ACTION_AMBIGUOUS_WIDE, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SESSION, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Session", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_INTERRUPT, MENU_SESSION, KEILAND_MENU_ITEM_NORMAL, "Send Interrupt", TERMINAL_ACTION_INTERRUPT, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_END_OF_FILE, MENU_SESSION, KEILAND_MENU_ITEM_NORMAL, "Send End of File", TERMINAL_ACTION_END_OF_FILE, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_SESSION_LINE, MENU_SESSION, KEILAND_MENU_ITEM_SEPARATOR, "", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_CLEAR, MENU_SESSION, KEILAND_MENU_ITEM_NORMAL, "Clear Screen", TERMINAL_ACTION_CLEAR, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_RESET, MENU_SESSION, KEILAND_MENU_ITEM_NORMAL, "Reset Terminal", TERMINAL_ACTION_RESET, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_HELP, KEILAND_MENU_ROOT, KEILAND_MENU_ITEM_SUBMENU, "Help", 0U, KEILAND_MENU_ROLE_NONE, 0U, 0U },
	{ MENU_ABOUT, MENU_HELP, KEILAND_MENU_ITEM_NORMAL, "About Terminal", TERMINAL_ACTION_ABOUT, KEILAND_MENU_ROLE_ABOUT, 0U, 0U }
};

static void menu_activated(void *data, struct keiland_window_menu *window_menu, uint32_t item, uint32_t action, struct wl_seat *seat, uint32_t serial);
static int menu_build(struct terminal_window *window);
static int menu_state(struct terminal_window *window, const struct terminal_menu_state *state);

/* What the window menu tells the terminal: only the choices. */
static const struct keiland_window_menu_listener menu_listener = {
	menu_activated, NULL, NULL
};

/*
 * Gives zdesktop the window's menus, showing a state.
 *
 * Returns 0, also when the compositor has no System Menu (the terminal then
 * has no menus), or -1 with errno set when the menus could not be made.
 */
int
terminal_menu_open(
	struct terminal_window *window,
	const struct terminal_menu_state *state)
{
	int error;

	/* The connection's menu service; a compositor without one leaves the terminal without menus. */
	window->menu_service = keiland_menu_service_open(window->display);
	if (window->menu_service == NULL) {
		printf("ZTERM MENU none errno=%d\n", errno);
		return 0;
	}

	/* The menu, empty until it is built. */
	window->menu = keiland_menu_create(window->menu_service);
	if (window->menu == NULL)
		return -1;

	/* The window's place for a menu, which tells the terminal what is chosen. */
	window->window_menu = keiland_window_menu_create(window->menu_service, window->toplevel, &menu_listener, window);
	if (window->window_menu == NULL)
		return -1;

	/* The items, and the state they start with, in one transaction. */
	error = menu_build(window);
	if (error != 0) {
		errno = error;
		return -1;
	}

	/* The window shows the menu from now on. */
	error = keiland_window_menu_set(window->window_menu, window->menu);
	if (error != 0) {
		errno = error;
		return -1;
	}

	/* The state it shows. */
	error = menu_state(window, state);
	if (error != 0) {
		errno = error;
		return -1;
	}

	/* Succeeded: the menus are zdesktop's to show. */
	printf("ZTERM MENU ready items=%u\n", (unsigned)(sizeof(menu_items) / sizeof(menu_items[0])));
	return 0;
}

/*
 * Tells the menus the terminal's state when it differs from what they show.
 */
void
terminal_menu_refresh(
	struct terminal_window *window,
	const struct terminal_menu_state *state)
{
	int same;
	int error;

	/* Without menus nothing is sent. */
	if (window->menu == NULL)
		return;

	/* Nor when the state is the one the menus show. */
	same = memcmp(state, &window->menu_state, sizeof(*state));
	if (same == 0)
		return;

	/* The new state in one transaction; a refusal is reported and the menus stay as they were. */
	error = menu_state(window, state);
	if (error != 0)
		printf("ZTERM MENU update-failed errno=%d\n", error);
}

/*
 * Takes the oldest action chosen and not yet carried out
 * (TERMINAL_ACTION_NONE when there is none).
 */
uint32_t
terminal_menu_take(
	struct terminal_window *window)
{
	uint32_t action;

	/* Nothing waits. */
	if (window->action_count == 0U)
		return TERMINAL_ACTION_NONE;

	/* The oldest leaves the queue. */
	action = window->actions[0];
	window->action_count--;
	memmove(window->actions, window->actions + 1, window->action_count * sizeof(window->actions[0]));

	/* Succeeded: the action to carry out. */
	return action;
}

/*
 * Takes the menus away from zdesktop (before the window goes).
 */
void
terminal_menu_close(
	struct terminal_window *window)
{
	/* The window's place, the menu, then the service. */
	keiland_window_menu_destroy(window->window_menu);
	keiland_menu_destroy(window->menu);
	keiland_menu_service_close(window->menu_service);
	window->window_menu = NULL;
	window->menu = NULL;
	window->menu_service = NULL;
}

/* Queues a chosen action for the main loop. */
static void
menu_activated(
	void *data,
	struct keiland_window_menu *window_menu,
	uint32_t item,
	uint32_t action,
	struct wl_seat *seat,
	uint32_t serial)
{
	struct terminal_window *window;

	/* The window whose menu was chosen from. */
	(void)window_menu;
	(void)seat;
	window = data;

	/* The log line the tests read. */
	printf("ZTERM MENU item=%u action=%u serial=%u\n", item, action, serial);
	fflush(stdout);

	/* A full queue drops the choice (the user has chosen sixteen things in one round). */
	if (window->action_count == TERMINAL_ACTIONS)
		return;

	/* The count is how many choices wait for the main loop (terminal_menu_take). */
	window->actions[window->action_count] = action;
	window->action_count++;
}

/* Gives zdesktop every item, with its role and shortcut, in one transaction. */
static int
menu_build(
	struct terminal_window *window)
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

/*
 * Shows a state in the menus in one transaction: Copy and Paste enabled,
 * Zoom In and Out within the sizes, the size's radio item, Fullscreen and
 * Treat Ambiguous-Width Characters as Wide checked.
 */
static int
menu_state(
	struct terminal_window *window,
	const struct terminal_menu_state *state)
{
	struct keiland_menu *menu;
	uint32_t checked_size;
	int larger;
	int smaller;
	int error;

	/* Zooming stops at the largest and the smallest size. */
	larger = 0;
	if (state->pixels < TERMINAL_PIXELS_MAX)
		larger = 1;
	smaller = 0;
	if (state->pixels > TERMINAL_PIXELS_MIN)
		smaller = 1;

	/* The radio item of the size, if the size is one the menu names. */
	checked_size = 0;
	if (state->pixels == TERMINAL_PIXELS_SMALL)
		checked_size = MENU_SIZE_SMALL;
	else if (state->pixels == TERMINAL_PIXELS_MEDIUM)
		checked_size = MENU_SIZE_MEDIUM;
	else if (state->pixels == TERMINAL_PIXELS_LARGE)
		checked_size = MENU_SIZE_LARGE;
	else if (state->pixels == TERMINAL_PIXELS_HUGE)
		checked_size = MENU_SIZE_HUGE;

	/* The transaction. */
	menu = window->menu;
	error = keiland_menu_begin(menu);
	if (error != 0)
		return error;

	/* Copy needs a selection, Paste the clipboard's text. */
	error = keiland_menu_set_enabled(menu, MENU_COPY, state->selection);
	if (error == 0)
		error = keiland_menu_set_enabled(menu, MENU_PASTE, state->clipboard);

	/* Zoom In and Zoom Out while there is room. */
	if (error == 0)
		error = keiland_menu_set_enabled(menu, MENU_ZOOM_IN, larger);
	if (error == 0)
		error = keiland_menu_set_enabled(menu, MENU_ZOOM_OUT, smaller);

	/* One radio item of the four is checked, or none for a size between them. */
	if (error == 0)
		error = keiland_menu_set_checked(menu, MENU_SIZE_SMALL, checked_size == MENU_SIZE_SMALL);
	if (error == 0)
		error = keiland_menu_set_checked(menu, MENU_SIZE_MEDIUM, checked_size == MENU_SIZE_MEDIUM);
	if (error == 0)
		error = keiland_menu_set_checked(menu, MENU_SIZE_LARGE, checked_size == MENU_SIZE_LARGE);
	if (error == 0)
		error = keiland_menu_set_checked(menu, MENU_SIZE_HUGE, checked_size == MENU_SIZE_HUGE);

	/* Fullscreen is checked while the window is. */
	if (error == 0)
		error = keiland_menu_set_checked(menu, MENU_FULLSCREEN, state->fullscreen);

	/* Treat Ambiguous-Width Characters as Wide is checked while the setting is on (ws128-p009). */
	if (error == 0)
		error = keiland_menu_set_checked(menu, MENU_AMBIGUOUS_WIDE, state->ambiguous_wide);

	/*
	 * A refused change still ends the transaction, so that the menu is not
	 * left open for changes; the refusal is reported.
	 */
	if (error != 0) {
		(void)keiland_menu_commit(menu);
		return error;
	}

	/* The state is shown together. */
	error = keiland_menu_commit(menu);
	if (error != 0)
		return error;

	/* Succeeded: the menus show the state. */
	window->menu_state = *state;
	printf("ZTERM MENU state selection=%d clipboard=%d pixels=%u fullscreen=%d ambiguous_wide=%d\n", state->selection, state->clipboard, state->pixels, state->fullscreen, state->ambiguous_wide);
	fflush(stdout);
	return 0;
}
