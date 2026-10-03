/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The parts of Text Editor that speak to zdesktop through libkeiui's
 * window (the window, its presenter, the clipboard and the primary
 * selection, WS090) and libkeiland's extensions: the editor's queue of
 * inputs (queue.c), the menus and the context menu (menu.c), the
 * titlebar's controls and find field (titlebar.c), and the glass
 * (glass.c).  The host tests build the rest of the program without them.
 */

#ifndef TEXTEDIT_WINDOW_H
#define TEXTEDIT_WINDOW_H

#include "textedit.h"

#include <keiland.h>
#include <keiui.h>

/* How many inputs wait for the editor at most. */
#define TE_WINDOW_EVENTS	256U

/*
 * The editor's window: libkeiui's window, and the queue of the editor's
 * inputs -- the window's pointer, keys and focus turned into te_event
 * values by the main loop, and the actions of the menus, the titlebar and
 * the file chooser -- in the order they came.  It lives for the whole run.
 */
struct te_window {
	struct kui_window *kui;

	/* The pointer's place and the modifiers held (TE_MOD_*), which every queued input carries. */
	int pointer_x;
	int pointer_y;
	uint32_t modifiers;

	/* The inputs waiting, a ring: the oldest's slot and how many. */
	struct te_event events[TE_WINDOW_EVENTS];
	unsigned event_first;
	unsigned event_count;
};

/*
 * What the menus and the titlebar show of the editor: whether a change can
 * be undone and redone, whether text is selected, whether the document
 * has unsaved changes, and the two View switches.
 */
struct te_state {
	int can_undo;
	int can_redo;
	int selected;
	int modified;
	int line_numbers;
	int wrap;
};

/*
 * The window's menus as given to zdesktop (menu.c): the connection's menu
 * service (NULL when the compositor has none, and the window then has no
 * menus), the window's menu and its place, the context menu's model and
 * the context menu open (NULL for none), the state the menus last
 * showed, and how many files File > Open Recent shows (ws128-p003).
 */
struct te_menu {
	struct keiland_menu_service *service;
	struct keiland_menu *menu;
	struct keiland_window_menu *window_menu;
	struct keiland_menu *context;
	struct keiland_context_menu *popup;
	struct te_state shown;
	struct te_window *window;
	size_t recent_shown;
};

/*
 * The window's titlebar in zdesktop (titlebar.c): zdesktop's titlebar
 * object (NULL without one), the state it last showed, whether it was
 * ever sent, and whether the find field should take the keyboard.
 */
struct te_titlebar {
	struct te_window *window;
	struct keiland_titlebar *titlebar;
	struct te_state shown;
	int sent;
	int want_focus;
};

/*
 * The window's glass (glass.c): zdesktop's glass object (NULL when the
 * window keeps its own ground), and the card it was last told of.
 */
struct te_glass {
	struct keiland_glass *glass;
	struct te_rect shown;
	int sent;
};

/* The editor's queue of inputs (queue.c). */
int te_window_take(struct te_window *window, struct te_event *event);
struct te_event *te_window_push(struct te_window *window, enum te_event_type type);
void te_window_action(struct te_window *window, uint32_t action);
void te_window_act(struct te_window *window, uint32_t action);

/* The menus and the context menu (menu.c). */
int te_menu_open(struct te_menu *menu, struct te_window *window, const struct te_state *state);
void te_menu_refresh(struct te_menu *menu, const struct te_state *state);
void te_menu_popup(struct te_menu *menu, int x, int y);
void te_menu_recent(struct te_menu *menu, const struct te_app *app);
void te_menu_close(struct te_menu *menu);

/* The titlebar's controls (titlebar.c). */
int te_titlebar_open(struct te_titlebar *titlebar, struct te_window *window, const struct te_state *state);
void te_titlebar_refresh(struct te_titlebar *titlebar, const struct te_state *state);
void te_titlebar_close(struct te_titlebar *titlebar);

/* The glass (glass.c). */
int te_glass_open(struct te_glass *glass, struct te_window *window, int see_through);
void te_glass_refresh(struct te_glass *glass, const struct te_app *app);
void te_glass_close(struct te_glass *glass);


#endif
