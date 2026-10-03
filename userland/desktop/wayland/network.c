/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The network in the glass look's system bar (ws035-p013): an icon beside
 * the battery that shows how the machine is connected, and a menu under it
 * to act on the network.
 *
 * The icon is four rising bars for Wi-Fi (all of them dark when connected,
 * pale while searching or joining, pale and struck through when the Wi-Fi
 * is off) and a small tree of three boxes for a wired connection.  With no
 * connection at all it is pale bars.
 *
 * A click on the icon opens the menu: the Wi-Fi switch with the Wi-Fi's
 * state under it, the networks the radio sees (the one it is on checked,
 * a padlock on those that ask for a key, the signal as bars), the wired
 * connection, and "Disconnect" while the Wi-Fi is connected.  A click on a
 * network joins it with its saved profile; a network that asks for a key
 * and has none saved opens a key field in the menu instead, and Enter saves
 * the key in the user's store, tells the daemon and joins (ws005-p019,
 * BUG-138), the same three steps Settings takes.  A click on the switch
 * turns the Wi-Fi on or off.  A click elsewhere, or Esc, closes the menu.
 * Opening the menu asks for a scan.
 *
 * libkeiland-backend carries one request at a time.  A switch or a network clicked
 * while another request (the menu's own scan, usually) is still out waits
 * in one slot and is sent when that one is answered, rather than being
 * refused with "busy".
 *
 * All of it comes through libkeiland-backend (kl_backend_network_*): zdesktop never
 * speaks networkd's protocol.  Nothing here waits for the daemon; each tick
 * reads what has arrived.
 */

#include "glass.h"

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The evdev codes of Esc, Backspace, Enter and the keypad's Enter. */
#define NETWORK_KEY_ESC		1U
#define NETWORK_KEY_BACKSPACE	14U
#define NETWORK_KEY_ENTER	28U
#define NETWORK_KEY_KPENTER	96U

/* How many evdev codes the character tables cover (up to the space bar). */
#define NETWORK_KEYS		58U

/* The depressed Shift and the locked Caps Lock in the seat's modifier masks. */
#define NETWORK_SHIFT		0x1U
#define NETWORK_CAPS		0x2U

/* The shortest and the longest WPA passphrase, in characters. */
#define NETWORK_KEY_MIN		8U
#define NETWORK_KEY_MAX		63U

/* The menu's width, its padding, its rows' height, and its corner radius. */
#define NETWORK_MENU_WIDTH	300
#define NETWORK_MENU_PADDING	8
#define NETWORK_ROW_HEIGHT	30
#define NETWORK_NOTE_HEIGHT	22
#define NETWORK_SEPARATOR	9
#define NETWORK_MENU_RADIUS	12.0f

/* The most rows the menu has: the switch, the state, the networks, the wired line, disconnect, a message. */
#define NETWORK_ROWS_MAX	(KL_BACKEND_NETWORK_SCAN_MAX + 8)

/* The kinds of row. */
enum network_row_kind {
	NETWORK_ROW_SWITCH,
	NETWORK_ROW_NOTE,
	NETWORK_ROW_SEPARATOR,
	NETWORK_ROW_AP,
	NETWORK_ROW_WIRED,
	NETWORK_ROW_DISCONNECT,
	NETWORK_ROW_KEY
};

/*
 * One row of the open menu: what it is, its text, its place (from the
 * menu's top), and for a network the scan's entry it shows.
 */
struct network_row {
	enum network_row_kind kind;
	char text[80];
	int32_t y;
	int32_t height;
	unsigned ap;
};

/*
 * The network's side of the system bar: the watch libkeiland-backend keeps (NULL
 * until the first tick of the desktop, and while it cannot be made), the
 * state and scan last read from it, whether the menu is open and where,
 * its rows, where the icon was last drawn, the network last asked to be
 * joined, and the text of the last failed request.
 *
 * The key field: key_open while it is shown, for key_ssid, with the
 * characters typed so far in key (key_length of them, never more than
 * NETWORK_KEY_MAX, wiped whenever the field closes or the key is saved).
 * join_after_profiles is set while the daemon is being told the saved
 * networks changed, so that its answer sends the join of key_ssid.
 *
 * The waiting slot: pending_request (KL_BACKEND_NETWORK_REQUEST_NONE when
 * empty) and pending_ssid, sent when the outstanding request is answered.
 *
 * It lives as long as zdesktop; the menu's rows are laid out again each
 * time the menu is drawn, so they always show the state last read.
 */
struct network_view {
	struct kl_backend_network *watch;
	unsigned opened;
	struct kl_backend_network_state state;
	struct kl_backend_network_ap scan[KL_BACKEND_NETWORK_SCAN_MAX];
	size_t scan_count;
	unsigned open;
	int32_t menu_x;
	int32_t menu_y;
	int32_t menu_height;
	struct network_row rows[NETWORK_ROWS_MAX];
	unsigned row_count;
	uint64_t logged_layout;
	int32_t icon_x;
	int32_t icon_y;
	int32_t icon_width;
	int32_t icon_height;
	unsigned icon_logged;
	char failure[96];
	char joining[KL_BACKEND_NETWORK_SSID_MAX];
	unsigned key_open;
	char key_ssid[KL_BACKEND_NETWORK_SSID_MAX];
	char key[NETWORK_KEY_MAX + 1U];
	size_t key_length;
	unsigned join_after_profiles;
	unsigned pending_request;
	char pending_ssid[KL_BACKEND_NETWORK_SSID_MAX];
};

/*
 * The one view of the network.  Only the event loop's thread touches it
 * (the ticks, the drawing, the input).
 */
static struct network_view network_view;

/*
 * The characters each key types into the key field, without and with Shift
 * (US layout, as the greeter's password field); 0 for none.
 */
static const char network_plain[NETWORK_KEYS] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' '
};
static const char network_shifted[NETWORK_KEYS] = {
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' '
};

static void network_open_menu(struct zwl_server *server);
static void network_close_menu(struct zwl_server *server, const char *via);
static void network_layout(struct zwl_server *server);
static void network_add_row(enum network_row_kind kind, const char *text, int32_t height, unsigned ap);
static void network_state_text(const struct kl_backend_network_state *state, char *text, size_t size);
static void network_act(struct zwl_server *server, const struct network_row *row);
static void network_request(struct zwl_server *server, unsigned request, const char *ssid);
static const struct network_row *network_row_at(int32_t x, int32_t y, int32_t *top);
static int network_in_icon(int32_t x, int32_t y);
static void network_log_state(void);
static void network_log_layout(void);
static void network_draw_bars(struct zwl_server *server, VkCommandBuffer command, int32_t x, int32_t bottom, unsigned lit, const float *ink, float faint);
static void network_draw_wired(struct zwl_server *server, VkCommandBuffer command, int32_t x, const float *ink);
static void network_draw_row(struct zwl_server *server, VkCommandBuffer command, const struct network_row *row, int32_t top, unsigned over);
static void network_draw_switch(struct zwl_server *server, VkCommandBuffer command, int32_t right, int32_t middle, unsigned on);
static void network_draw_lock(struct zwl_server *server, VkCommandBuffer command, int32_t x, int32_t middle, const float *ink);
static unsigned network_strength(int rssi);
static const char *network_request_name(unsigned request);
static const char *network_wifi_name(unsigned wifi);
static void network_choose_ap(struct zwl_server *server, unsigned ap);
static int network_key_saved(const char *ssid);
static void network_key_open(struct zwl_server *server, const char *ssid);
static void network_key_close(void);
static void network_key_type(struct zwl_server *server, uint32_t key);
static void network_key_submit(struct zwl_server *server);
static void network_key_wipe(void);
static void network_finished(struct zwl_server *server, unsigned request, int error);

/*
 * Reads what the network watch has brought since the last tick, and makes
 * the watch on the desktop's first tick.
 */
void
zwl_network_tick(
	struct zwl_server *server)
{
	unsigned changed;
	unsigned request;
	int error;

	/* The watch, once (libkeiland-backend connects to the daemon when it can). */
	if (!network_view.opened) {
		network_view.opened = 1;
		network_view.watch = kl_backend_network_open();
		network_view.icon_x = -1;
	}

	/* No watch could be made (no memory): the icon stays pale. */
	if (network_view.watch == NULL)
		return;

	/* What arrived. */
	(void)kl_backend_network_update(network_view.watch, &changed);
	if (changed == 0)
		return;

	/* A new state redraws the icon (and the menu). */
	if ((changed & KL_BACKEND_NETWORK_CHANGED_STATE) != 0) {
		kl_backend_network_get_state(network_view.watch, &network_view.state);
		network_log_state();
	}

	/* A new scan redraws the menu's networks. */
	if ((changed & KL_BACKEND_NETWORK_CHANGED_SCAN) != 0) {
		network_view.scan_count = kl_backend_network_get_scan(network_view.watch, network_view.scan, KL_BACKEND_NETWORK_SCAN_MAX);
		if (network_view.scan_count > KL_BACKEND_NETWORK_SCAN_MAX)
			network_view.scan_count = KL_BACKEND_NETWORK_SCAN_MAX;
		printf("ZWL NETWORK scan count=%u\n", (unsigned)network_view.scan_count);
	}

	/* A request that finished: its failure said, and what waited for it sent. */
	if ((changed & KL_BACKEND_NETWORK_CHANGED_DONE) != 0) {
		request = kl_backend_network_get_request(network_view.watch, &error);
		printf("ZWL NETWORK done request=%s error=%d\n", network_request_name(request), error);
		network_finished(server, request, error);
	}

	/* Something shown has changed. */
	server->dirty = 1;
}

/*
 * Draws the network's icon in the system bar at x (its left edge), in the
 * bar's ink: Wi-Fi bars or the wired tree.
 */
void
zwl_network_draw_icon(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t x,
	const float *ink)
{
	static const float blue[4] = { 0.25f, 0.52f, 0.98f, 0.28f };
	const struct kl_backend_network_state *state;
	unsigned lit;
	unsigned index;
	int differs;

	/* Where a click opens the menu (a little larger than the drawing). */
	network_view.icon_x = x - 6;
	network_view.icon_y = 3;
	network_view.icon_width = 30;
	network_view.icon_height = ZWL_GLASS_BAR - 6;
	if (!network_view.icon_logged) {
		network_view.icon_logged = 1;
		printf("ZWL NETWORK icon x=%d y=%d width=%d height=%d\n", network_view.icon_x, network_view.icon_y, network_view.icon_width, network_view.icon_height);
	}

	/* While the menu is open its icon has a pale blue back. */
	if (network_view.open)
		glass_draw_solid(server, command, (float)network_view.icon_x, (float)network_view.icon_y, (float)network_view.icon_width, (float)network_view.icon_height, 7.0f, blue);

	/* A wired connection is the tree. */
	state = &network_view.state;
	if (state->connected && state->kind == KL_BACKEND_NETWORK_WIRED) {
		network_draw_wired(server, command, x, ink);
		return;
	}

	/* A connected Wi-Fi is as many dark bars as its signal is strong (all without a scan of it). */
	if (state->connected && state->kind == KL_BACKEND_NETWORK_WIFI) {
		lit = 4;
		for (index = 0; index < network_view.scan_count; index++) {
			differs = strcmp(network_view.scan[index].ssid, state->ssid);
			if (differs == 0)
				lit = network_strength(network_view.scan[index].rssi);
		}

		/* The bars. */
		network_draw_bars(server, command, x, 23, lit, ink, 0.25f);
		return;
	}

	/* Anything else is pale bars; Wi-Fi that is off is struck through. */
	network_draw_bars(server, command, x, 23, 0, ink, 0.30f);
	if (state->reachable && state->wifi == KL_BACKEND_WIFI_OFF)
		glass_draw_solid(server, command, (float)(x - 2), 15.0f, 22.0f, 2.0f, 1.0f, ink);
}

/*
 * Draws the open menu under the icon: its shadow, its glass and its rows,
 * the row under the pointer lit.
 */
void
zwl_network_draw_menu(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	struct glass_shape shape;
	const struct network_row *over;
	unsigned index;
	int32_t top;

	/* Only an open menu. */
	if (!network_view.open)
		return;

	/* The rows for the state last read. */
	network_layout(server);

	/* The shadow. */
	glass_shape_init(&shape, (float)network_view.menu_x, (float)network_view.menu_y + 6.0f, (float)NETWORK_MENU_WIDTH, (float)network_view.menu_height);
	shape.quad[0] -= 40.0f;
	shape.quad[1] -= 40.0f;
	shape.quad[2] += 80.0f;
	shape.quad[3] += 80.0f;
	shape.mode = MODE_SHADOW;
	shape.radius = NETWORK_MENU_RADIUS;
	shape.soft = 18.0f;
	shape.color[0] = 0.10f;
	shape.color[1] = 0.18f;
	shape.color[2] = 0.35f;
	shape.color[3] = 0.24f;
	glass_shape_draw(server, command, &shape);

	/* The glass, as white as the System Menu's popups. */
	glass_shape_init(&shape, (float)network_view.menu_x, (float)network_view.menu_y, (float)NETWORK_MENU_WIDTH, (float)network_view.menu_height);
	shape.mode = MODE_GLASS;
	shape.radius = NETWORK_MENU_RADIUS;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.86f;
	shape.edge = 0.85f;
	glass_shape_draw(server, command, &shape);

	/* Each row, the one under the pointer lit when it can be chosen. */
	over = network_row_at(server->pointer_x, server->pointer_y, &top);
	for (index = 0; index < network_view.row_count; index++) {
		top = network_view.menu_y + network_view.rows[index].y;
		network_draw_row(server, command, &network_view.rows[index], top, over == &network_view.rows[index]);
	}
}

/*
 * Handles a pointer button for the network: a press on the icon opens or
 * closes the menu; while it is open, a press on a row acts on it and a
 * press elsewhere closes the menu.  Returns 1 when the button was the
 * network's.
 */
int
zwl_network_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	const struct network_row *row;
	int32_t top;
	int inside;

	/* With the menu closed, only a left press on the icon. */
	if (!network_view.open) {
		/* A release, or another button, goes on. */
		if (state == 0 || button != ZWL_BUTTON_LEFT)
			return 0;

		/* A press off the icon goes on. */
		inside = network_in_icon(server->pointer_x, server->pointer_y);
		if (!inside)
			return 0;

		/* The menu opens. */
		network_open_menu(server);
		return 1;
	}

	/* While it is open, releases are the menu's. */
	if (state == 0)
		return 1;

	/* A press on the icon closes it. */
	inside = network_in_icon(server->pointer_x, server->pointer_y);
	if (inside) {
		network_close_menu(server, "icon");
		return 1;
	}

	/* A press outside the menu closes it and goes no further. */
	inside = 0;
	if (server->pointer_x >= network_view.menu_x &&
	    server->pointer_x < network_view.menu_x + NETWORK_MENU_WIDTH &&
	    server->pointer_y >= network_view.menu_y &&
	    server->pointer_y < network_view.menu_y + network_view.menu_height)
		inside = 1;
	if (!inside) {
		network_close_menu(server, "outside");
		return 1;
	}

	/* A left press on a row acts on it. */
	row = network_row_at(server->pointer_x, server->pointer_y, &top);
	if (row != NULL && button == ZWL_BUTTON_LEFT)
		network_act(server, row);

	/* Succeeded: the press was the menu's. */
	return 1;
}

/*
 * Handles a key while the menu is open: Esc closes it, and the others are
 * the menu's too.  Returns 1 when the key was the network's.
 */
int
zwl_network_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	/* A closed menu takes no key. */
	if (!network_view.open)
		return 0;

	/* With the key field open, the keys type into it; Esc closes the field. */
	if (network_view.key_open) {
		if (state != 0)
			network_key_type(server, key);
		return 1;
	}

	/* Esc, pressed, closes it. */
	if (key == NETWORK_KEY_ESC && state != 0)
		network_close_menu(server, "key");

	/* Succeeded: the key was the menu's. */
	return 1;
}

/*
 * Follows the pointer while the menu is open (the row under it is lit).
 * Returns 1 when the motion was the menu's.
 */
int
zwl_network_motion(
	struct zwl_server *server)
{
	/* A closed menu does not follow the pointer. */
	if (!network_view.open)
		return 0;

	/* The lit row may have changed. */
	server->dirty = 1;

	/* Succeeded: the motion was the menu's. */
	return 1;
}

/*
 * Tells whether the menu is open (the look is not still while it is).
 */
int
zwl_network_is_open(
	void)
{
	/* Open or not. */
	return (int)network_view.open;
}

/* Opens the menu and asks for a scan when the Wi-Fi is on. */
static void
network_open_menu(
	struct zwl_server *server)
{
	/* The menu, with no failure from before. */
	network_view.open = 1;
	network_view.failure[0] = '\0';
	network_view.logged_layout = 0;
	server->dirty = 1;
	printf("ZWL NETWORK open\n");

	/* A radio that is on is asked what it sees. */
	if (network_view.state.wifi == KL_BACKEND_WIFI_ABSENT)
		return;
	if (network_view.state.wifi == KL_BACKEND_WIFI_OFF)
		return;
	network_request(server, KL_BACKEND_NETWORK_REQUEST_SCAN, NULL);
}

/* Closes the menu. */
static void
network_close_menu(
	struct zwl_server *server,
	const char *via)
{
	/* A key half typed goes with the menu. */
	network_key_close();

	/* The menu goes; the icon's back goes with it. */
	network_view.open = 0;
	server->dirty = 1;
	printf("ZWL NETWORK close via=%s\n", via);
}

/*
 * Lays out the rows from the state and scan last read: the switch and the
 * state, the networks, the wired connection, disconnect and a failure.
 */
static void
network_layout(
	struct zwl_server *server)
{
	const struct kl_backend_network_state *state;
	char text[96];
	unsigned request;
	unsigned index;
	int32_t y;
	int error;

	/* No rows yet. */
	state = &network_view.state;
	network_view.row_count = 0;

	/* The Wi-Fi's switch, with its state under it. */
	if (state->wifi != KL_BACKEND_WIFI_ABSENT && state->reachable) {
		network_add_row(NETWORK_ROW_SWITCH, "Wi-Fi", NETWORK_ROW_HEIGHT, 0);
		network_state_text(state, text, sizeof(text));
		network_add_row(NETWORK_ROW_NOTE, text, NETWORK_NOTE_HEIGHT, 0);
	} else {
		network_state_text(state, text, sizeof(text));
		network_add_row(NETWORK_ROW_NOTE, text, NETWORK_NOTE_HEIGHT, 0);
	}

	/* The networks, while the Wi-Fi is on. */
	if (state->reachable && state->wifi != KL_BACKEND_WIFI_ABSENT && state->wifi != KL_BACKEND_WIFI_OFF) {
		network_add_row(NETWORK_ROW_SEPARATOR, "", NETWORK_SEPARATOR, 0);

		/* A scan on its way, or one that found nothing, says so. */
		request = kl_backend_network_get_request(network_view.watch, &error);
		if (request == KL_BACKEND_NETWORK_REQUEST_SCAN && network_view.scan_count == 0) {
			network_add_row(NETWORK_ROW_NOTE, "Looking for networks...", NETWORK_NOTE_HEIGHT, 0);
		} else if (network_view.scan_count == 0) {
			network_add_row(NETWORK_ROW_NOTE, "No networks found", NETWORK_NOTE_HEIGHT, 0);
		}

		/* Each network the scan found. */
		for (index = 0; index < network_view.scan_count; index++)
			network_add_row(NETWORK_ROW_AP, network_view.scan[index].ssid, NETWORK_ROW_HEIGHT, index);
	}

	/* The key field: what it is for, the field, and how to finish. */
	if (network_view.key_open) {
		network_add_row(NETWORK_ROW_SEPARATOR, "", NETWORK_SEPARATOR, 0);
		(void)snprintf(text, sizeof(text), "Key for %s", network_view.key_ssid);
		network_add_row(NETWORK_ROW_NOTE, text, NETWORK_NOTE_HEIGHT, 0);
		network_add_row(NETWORK_ROW_KEY, "", NETWORK_ROW_HEIGHT, 0);
		network_add_row(NETWORK_ROW_NOTE, "Enter: join   Esc: cancel", NETWORK_NOTE_HEIGHT, 0);
	}

	/* The wired connection's line, after a separator. */
	network_add_row(NETWORK_ROW_SEPARATOR, "", NETWORK_SEPARATOR, 0);
	if (state->wired[0] != '\0') {
		(void)snprintf(text, sizeof(text), "Wired (%s): connected", state->wired);
	} else {
		(void)snprintf(text, sizeof(text), "Wired: not connected");
	}

	/* The line. */
	network_add_row(NETWORK_ROW_WIRED, text, NETWORK_ROW_HEIGHT, 0);

	/* Leaving the Wi-Fi network it is on. */
	if (state->wifi == KL_BACKEND_WIFI_CONNECTED || state->wifi == KL_BACKEND_WIFI_CONNECTING) {
		(void)snprintf(text, sizeof(text), "Disconnect from %s", state->ssid);
		network_add_row(NETWORK_ROW_DISCONNECT, text, NETWORK_ROW_HEIGHT, 0);
	}

	/* The last failure. */
	if (network_view.failure[0] != '\0')
		network_add_row(NETWORK_ROW_NOTE, network_view.failure, NETWORK_NOTE_HEIGHT, 0);

	/* The rows' places from the top, and the menu's height. */
	y = NETWORK_MENU_PADDING;
	for (index = 0; index < network_view.row_count; index++) {
		network_view.rows[index].y = y;
		y += network_view.rows[index].height;
	}

	/* The menu ends with its padding under the last row. */
	network_view.menu_height = y + NETWORK_MENU_PADDING;

	/* Under the icon, its right edge a little in from the output's. */
	network_view.menu_x = network_view.icon_x + network_view.icon_width - NETWORK_MENU_WIDTH + 60;
	if (network_view.menu_x + NETWORK_MENU_WIDTH > (int32_t)server->width - 8)
		network_view.menu_x = (int32_t)server->width - 8 - NETWORK_MENU_WIDTH;
	if (network_view.menu_x < 8)
		network_view.menu_x = 8;
	network_view.menu_y = ZWL_GLASS_BAR + 6;

	/* A new layout is logged for the tests that click the rows. */
	network_log_layout();
}

/* Adds one row (while there is room). */
static void
network_add_row(
	enum network_row_kind kind,
	const char *text,
	int32_t height,
	unsigned ap)
{
	struct network_row *row;

	/* A full menu takes no more rows. */
	if (network_view.row_count >= NETWORK_ROWS_MAX)
		return;

	/* The row. */
	row = &network_view.rows[network_view.row_count];
	memset(row, 0, sizeof(*row));
	row->kind = kind;
	(void)snprintf(row->text, sizeof(row->text), "%s", text);
	row->height = height;
	row->ap = ap;
	network_view.row_count++;
}

/* Writes the line under the switch: what the Wi-Fi is doing, or why there is none. */
static void
network_state_text(
	const struct kl_backend_network_state *state,
	char *text,
	size_t size)
{
	/* The daemon cannot be reached. */
	if (!state->reachable) {
		(void)snprintf(text, size, "Network service not available");
		return;
	}

	/* The Wi-Fi's own state. */
	switch (state->wifi) {
	case KL_BACKEND_WIFI_ABSENT:
		(void)snprintf(text, size, "No Wi-Fi hardware");
		break;
	case KL_BACKEND_WIFI_OFF:
		(void)snprintf(text, size, "Wi-Fi is off");
		break;
	case KL_BACKEND_WIFI_SEARCHING:
		(void)snprintf(text, size, "Searching for a known network");
		break;
	case KL_BACKEND_WIFI_CONNECTING:
		(void)snprintf(text, size, "Joining %s...", state->ssid);
		break;
	case KL_BACKEND_WIFI_CONNECTED:
		(void)snprintf(text, size, "Connected to %s", state->ssid);
		break;
	default:
		(void)snprintf(text, size, "Not connected");
		break;
	}
}

/* Acts on a row: the switch turns the Wi-Fi on or off, a network is joined, disconnect leaves. */
static void
network_act(
	struct zwl_server *server,
	const struct network_row *row)
{
	unsigned wanted;

	/* What the row does. */
	switch (row->kind) {
	case NETWORK_ROW_SWITCH:
		/* Off turns on, anything else turns off. */
		wanted = KL_BACKEND_NETWORK_REQUEST_WIFI_OFF;
		if (network_view.state.wifi == KL_BACKEND_WIFI_OFF)
			wanted = KL_BACKEND_NETWORK_REQUEST_WIFI_ON;
		network_request(server, wanted, NULL);
		break;
	case NETWORK_ROW_AP:
		network_choose_ap(server, row->ap);
		break;
	case NETWORK_ROW_DISCONNECT:
		network_request(server, KL_BACKEND_NETWORK_REQUEST_DISCONNECT, NULL);
		break;
	default:
		/* The notes, the separators and the wired line only show. */
		break;
	}
}

/* Sends a request through libkeiland-backend, and says in the menu when it cannot be sent. */
static void
network_request(
	struct zwl_server *server,
	unsigned request,
	const char *ssid)
{
	int error;

	/* No watch, no request. */
	if (network_view.watch == NULL)
		return;

	/* A join's network is kept for the failure line. */
	network_view.joining[0] = '\0';
	if (ssid != NULL)
		(void)snprintf(network_view.joining, sizeof(network_view.joining), "%s", ssid);

	/* The request; the answer comes through the ticks. */
	error = kl_backend_network_request(network_view.watch, request, ssid);
	printf("ZWL NETWORK request %s ssid=%s error=%d\n", network_request_name(request), network_view.joining, error);
	server->dirty = 1;

	/* A request behind another one waits for its answer (a scan is not kept). */
	if (error == EBUSY && request != KL_BACKEND_NETWORK_REQUEST_SCAN) {
		network_view.pending_request = request;
		(void)snprintf(network_view.pending_ssid, sizeof(network_view.pending_ssid), "%s", network_view.joining);
		return;
	}

	/* A request that could not even be sent is said in the menu. */
	if (error != 0 && request != KL_BACKEND_NETWORK_REQUEST_SCAN)
		(void)snprintf(network_view.failure, sizeof(network_view.failure), "Could not %s (%s)", network_request_name(request), strerror(error));
}

/* Finds the row that can be chosen under a point of the open menu, with its top. */
static const struct network_row *
network_row_at(
	int32_t x,
	int32_t y,
	int32_t *top)
{
	const struct network_row *row;
	unsigned index;

	/* Outside the menu's width, nothing. */
	if (!network_view.open)
		return NULL;
	if (x < network_view.menu_x || x >= network_view.menu_x + NETWORK_MENU_WIDTH)
		return NULL;

	/* The row the point is in, when it acts. */
	for (index = 0; index < network_view.row_count; index++) {
		row = &network_view.rows[index];
		*top = network_view.menu_y + row->y;
		if (y < *top || y >= *top + row->height)
			continue;

		/* Notes, separators and the wired line do nothing. */
		if (row->kind == NETWORK_ROW_SWITCH || row->kind == NETWORK_ROW_AP || row->kind == NETWORK_ROW_DISCONNECT)
			return row;
		return NULL;
	}

	/* The padding. */
	return NULL;
}

/* Tells whether a point is on the icon as last drawn. */
static int
network_in_icon(
	int32_t x,
	int32_t y)
{
	/* An icon never drawn has no place. */
	if (network_view.icon_x < 0)
		return 0;

	/* Its rectangle. */
	if (x < network_view.icon_x || x >= network_view.icon_x + network_view.icon_width)
		return 0;
	if (y < network_view.icon_y || y >= network_view.icon_y + network_view.icon_height)
		return 0;

	/* On it. */
	return 1;
}

/* Logs the state for the tests. */
static void
network_log_state(
	void)
{
	const struct kl_backend_network_state *state;
	const char *kind;

	/* What carries the connection. */
	state = &network_view.state;
	kind = "none";
	if (state->kind == KL_BACKEND_NETWORK_WIRED)
		kind = "wired";
	if (state->kind == KL_BACKEND_NETWORK_WIFI)
		kind = "wifi";

	/* One line. */
	printf("ZWL NETWORK state reachable=%u connected=%u kind=%s interface=%s wifi=%s ssid=%s\n",
	    state->reachable, state->connected, kind, state->interface, network_wifi_name(state->wifi), state->ssid);
}

/* Logs the rows' places when they change, for the tests that click them. */
static void
network_log_layout(
	void)
{
	const struct network_row *row;
	uint64_t checksum;
	unsigned index;
	size_t at;

	/* A checksum of the rows' kinds, places and texts. */
	checksum = 1469598103934665603ULL;
	for (index = 0; index < network_view.row_count; index++) {
		row = &network_view.rows[index];
		checksum = (checksum ^ (uint64_t)row->kind) * 1099511628211ULL;
		checksum = (checksum ^ (uint64_t)(uint32_t)row->y) * 1099511628211ULL;
		for (at = 0; row->text[at] != '\0'; at++)
			checksum = (checksum ^ (unsigned char)row->text[at]) * 1099511628211ULL;
	}

	/* And the menu's place. */
	checksum = (checksum ^ (uint64_t)(uint32_t)network_view.menu_x) * 1099511628211ULL;

	/* Only a layout not logged yet. */
	if (checksum == network_view.logged_layout)
		return;
	network_view.logged_layout = checksum;

	/* Each row that acts, with its rectangle. */
	printf("ZWL NETWORK menu x=%d y=%d width=%d height=%d rows=%u\n", network_view.menu_x, network_view.menu_y, NETWORK_MENU_WIDTH, network_view.menu_height, network_view.row_count);
	for (index = 0; index < network_view.row_count; index++) {
		row = &network_view.rows[index];
		printf("ZWL NETWORK row index=%u kind=%d x=%d y=%d width=%d height=%d text=%s\n", index, (int)row->kind,
		    network_view.menu_x, network_view.menu_y + row->y, NETWORK_MENU_WIDTH, row->height, row->text);
	}
}

/* Draws four rising bars from x, the first lit ones in the ink and the rest faint. */
static void
network_draw_bars(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t x,
	int32_t bottom,
	unsigned lit,
	const float *ink,
	float faint)
{
	float color[4];
	unsigned step;
	int32_t height;

	/* Each bar, taller to the right. */
	for (step = 0; step < 4; step++) {
		memcpy(color, ink, sizeof(color));
		if (step >= lit)
			color[3] = ink[3] * faint;
		height = 4 + (int32_t)step * 3;
		glass_draw_solid(server, command, (float)(x + (int32_t)step * 5), (float)(bottom - height), 3.0f, (float)height, 1.0f, color);
	}
}

/* Draws the wired connection's icon: a box above two, joined by lines. */
static void
network_draw_wired(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t x,
	const float *ink)
{
	/* The upper box, and the stem from it. */
	glass_draw_solid(server, command, (float)(x + 5), 8.0f, 8.0f, 6.0f, 1.5f, ink);
	glass_draw_solid(server, command, (float)(x + 8), 14.0f, 2.0f, 3.0f, 0.0f, ink);

	/* The bar across, and the legs down. */
	glass_draw_solid(server, command, (float)(x + 2), 17.0f, 14.0f, 2.0f, 0.0f, ink);
	glass_draw_solid(server, command, (float)(x + 2), 17.0f, 2.0f, 3.0f, 0.0f, ink);
	glass_draw_solid(server, command, (float)(x + 14), 17.0f, 2.0f, 3.0f, 0.0f, ink);

	/* The two lower boxes. */
	glass_draw_solid(server, command, (float)(x - 1), 20.0f, 8.0f, 6.0f, 1.5f, ink);
	glass_draw_solid(server, command, (float)(x + 11), 20.0f, 8.0f, 6.0f, 1.5f, ink);
}

/* Draws one row at top: its band when lit, and what the row shows. */
static void
network_draw_row(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct network_row *row,
	int32_t top,
	unsigned over)
{
	static const float dark[4] = { 0.12f, 0.16f, 0.24f, 1.0f };
	static const float soft[4] = { 0.40f, 0.46f, 0.56f, 1.0f };
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float blue[4] = { 0.25f, 0.52f, 0.98f, 1.0f };
	static const float line[4] = { 0.12f, 0.16f, 0.24f, 0.16f };
	static const float field[4] = { 1.0f, 1.0f, 1.0f, 0.95f };
	static const float frame[4] = { 0.25f, 0.52f, 0.98f, 0.70f };
	const struct kl_backend_network_ap *ap;
	char stars[NETWORK_KEY_MAX + 2U];
	size_t count;
	float ink[4];
	int32_t left;
	int32_t right;
	int32_t middle;
	int32_t baseline;
	int present;
	int differs;

	/* The row's edges, its middle and the text's baseline. */
	left = network_view.menu_x;
	right = network_view.menu_x + NETWORK_MENU_WIDTH - 14;
	middle = top + row->height / 2;
	baseline = middle + 5;

	/* A separator is a thin line. */
	if (row->kind == NETWORK_ROW_SEPARATOR) {
		glass_draw_solid(server, command, (float)(left + 12), (float)(top + row->height / 2), (float)(NETWORK_MENU_WIDTH - 24), 1.0f, 0.0f, line);
		return;
	}

	/* A note is soft text. */
	if (row->kind == NETWORK_ROW_NOTE) {
		glass_draw_text(server, command, SIZE_BAR, left + 14, baseline, row->text, NETWORK_MENU_WIDTH - 28, soft);
		return;
	}

	/* The lit row is a blue band with white text. */
	memcpy(ink, dark, sizeof(ink));
	if (over) {
		glass_draw_solid(server, command, (float)(left + 5), (float)(top + 1), (float)(NETWORK_MENU_WIDTH - 10), (float)(row->height - 2), 6.0f, blue);
		memcpy(ink, white, sizeof(ink));
	}

	/* The switch's row: its label and the switch at the right. */
	if (row->kind == NETWORK_ROW_SWITCH) {
		glass_draw_text(server, command, SIZE_TITLE, left + 14, baseline + 1, row->text, 160, ink);
		network_draw_switch(server, command, right, middle, network_view.state.wifi != KL_BACKEND_WIFI_OFF);
		return;
	}

	/* A network: the check of the one it is on, its SSID, a padlock and its signal. */
	if (row->kind == NETWORK_ROW_AP) {
		ap = &network_view.scan[row->ap];
		differs = strcmp(ap->ssid, network_view.state.ssid);
		if (network_view.state.wifi == KL_BACKEND_WIFI_CONNECTED && differs == 0) {
			/* The check mark, or a small square without the glyph. */
			present = glass_glyph_advance(server, SIZE_BAR, GLASS_CHECK_GLYPH);
			if (present > 0) {
				glass_draw_glyph(server, command, SIZE_BAR, GLASS_CHECK_GLYPH, left + 12, baseline, ink);
			} else {
				glass_draw_solid(server, command, (float)(left + 13), (float)(middle - 4), 8.0f, 8.0f, 2.0f, ink);
			}
		}

		/* The SSID, the padlock of a network that asks for a key, and the signal. */
		glass_draw_text(server, command, SIZE_BAR, left + 32, baseline, row->text, NETWORK_MENU_WIDTH - 32 - 64, ink);
		if (ap->secured)
			network_draw_lock(server, command, right - 42, middle, ink);
		network_draw_bars(server, command, right - 18, middle + 7, network_strength(ap->rssi), ink, 0.25f);
		return;
	}

	/* The key field: a pale box with a star for each character typed and the cursor. */
	if (row->kind == NETWORK_ROW_KEY) {
		glass_draw_solid(server, command, (float)(left + 11), (float)(top + 2), (float)(NETWORK_MENU_WIDTH - 22), (float)(row->height - 4), 6.0f, frame);
		glass_draw_solid(server, command, (float)(left + 12), (float)(top + 3), (float)(NETWORK_MENU_WIDTH - 24), (float)(row->height - 6), 5.0f, field);

		/* The key is never drawn, only its length. */
		for (count = 0U; count < network_view.key_length; count++)
			stars[count] = '*';
		stars[count] = '|';
		stars[count + 1U] = '\0';
		glass_draw_text(server, command, SIZE_BAR, left + 20, baseline, stars, NETWORK_MENU_WIDTH - 40, dark);
		return;
	}

	/* The wired line and disconnect are plain text. */
	glass_draw_text(server, command, SIZE_BAR, left + 14, baseline, row->text, NETWORK_MENU_WIDTH - 28, ink);
}

/* Draws the Wi-Fi's switch ending at right: a pill, blue when on, with its knob. */
static void
network_draw_switch(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t right,
	int32_t middle,
	unsigned on)
{
	static const float blue[4] = { 0.25f, 0.52f, 0.98f, 1.0f };
	static const float grey[4] = { 0.62f, 0.66f, 0.72f, 1.0f };
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	int32_t x;

	/* The pill. */
	x = right - 36;
	if (on) {
		glass_draw_solid(server, command, (float)x, (float)(middle - 10), 36.0f, 20.0f, 10.0f, blue);
	} else {
		glass_draw_solid(server, command, (float)x, (float)(middle - 10), 36.0f, 20.0f, 10.0f, grey);
	}

	/* The knob, right when on. */
	if (on) {
		glass_draw_solid(server, command, (float)(x + 18), (float)(middle - 8), 16.0f, 16.0f, 8.0f, white);
	} else {
		glass_draw_solid(server, command, (float)(x + 2), (float)(middle - 8), 16.0f, 16.0f, 8.0f, white);
	}
}

/* Draws a small padlock: a body and its shackle. */
static void
network_draw_lock(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t x,
	int32_t middle,
	const float *ink)
{
	struct glass_shape shape;

	/* The shackle, a ring over the body. */
	glass_shape_init(&shape, (float)(x + 2), (float)(middle - 7), 6.0f, 8.0f);
	shape.quad[0] -= 1.0f;
	shape.quad[1] -= 1.0f;
	shape.quad[2] += 2.0f;
	shape.quad[3] += 2.0f;
	shape.mode = MODE_RING;
	shape.radius = 3.0f;
	shape.soft = 1.2f;
	memcpy(shape.color, ink, sizeof(shape.color));
	glass_shape_draw(server, command, &shape);

	/* The body. */
	glass_draw_solid(server, command, (float)x, (float)(middle - 2), 10.0f, 8.0f, 1.5f, ink);
}

/* Tells how many of four bars a signal lights. */
static unsigned
network_strength(
	int rssi)
{
	/* Strong, good, fair, weak. */
	if (rssi >= -55)
		return 4;
	if (rssi >= -67)
		return 3;
	if (rssi >= -78)
		return 2;

	/* Anything weaker still shows one. */
	return 1;
}

/* Names a request, for the log and the failure line. */
static const char *
network_request_name(
	unsigned request)
{
	/* Each request's verb. */
	switch (request) {
	case KL_BACKEND_NETWORK_REQUEST_SCAN:
		return "scan";
	case KL_BACKEND_NETWORK_REQUEST_JOIN:
		return "join";
	case KL_BACKEND_NETWORK_REQUEST_DISCONNECT:
		return "disconnect";
	case KL_BACKEND_NETWORK_REQUEST_WIFI_ON:
		return "turn Wi-Fi on";
	case KL_BACKEND_NETWORK_REQUEST_WIFI_OFF:
		return "turn Wi-Fi off";
	case KL_BACKEND_NETWORK_REQUEST_PROFILES:
		return "save the key";
	default:
		break;
	}

	/* No request. */
	return "none";
}

/* Names a Wi-Fi state, for the log. */
static const char *
network_wifi_name(
	unsigned wifi)
{
	/* Each state's name. */
	switch (wifi) {
	case KL_BACKEND_WIFI_OFF:
		return "off";
	case KL_BACKEND_WIFI_SEARCHING:
		return "searching";
	case KL_BACKEND_WIFI_CONNECTING:
		return "connecting";
	case KL_BACKEND_WIFI_CONNECTED:
		return "connected";
	case KL_BACKEND_WIFI_DISCONNECTED:
		return "disconnected";
	default:
		break;
	}

	/* No radio. */
	return "absent";
}

/*
 * Acts on a network chosen in the menu: one that asks for a key and has
 * none saved opens the key field; any other is joined with its saved
 * profile.
 */
static void
network_choose_ap(
	struct zwl_server *server,
	unsigned ap)
{
	const struct kl_backend_network_ap *chosen;
	int saved;

	/* The network as the scan last reported it. */
	chosen = &network_view.scan[ap];

	/* A network with a saved key, or one that asks for none, is joined. */
	saved = network_key_saved(chosen->ssid);
	if (saved || !chosen->secured) {
		network_request(server, KL_BACKEND_NETWORK_REQUEST_JOIN, chosen->ssid);
		return;
	}

	/* A key is asked for. */
	network_key_open(server, chosen->ssid);
}

/* Tells whether the user's store has a key for a network. */
static int
network_key_saved(
	const char *ssid)
{
	char saved[KL_BACKEND_NETWORK_SCAN_MAX][KL_BACKEND_NETWORK_SSID_MAX];
	size_t count;
	size_t index;
	int differs;

	/* The networks the user has saved (read from the store, not the daemon). */
	count = kl_backend_network_get_saved(saved, KL_BACKEND_NETWORK_SCAN_MAX);
	if (count > KL_BACKEND_NETWORK_SCAN_MAX)
		count = KL_BACKEND_NETWORK_SCAN_MAX;

	/* Looks for this one among them. */
	for (index = 0; index < count; index++) {
		differs = strcmp(saved[index], ssid);
		if (differs == 0)
			return 1;
	}

	/* Not saved. */
	return 0;
}

/* Opens the key field for a network, empty. */
static void
network_key_open(
	struct zwl_server *server,
	const char *ssid)
{
	/* The field, for this network, with nothing typed and no failure from before. */
	network_key_wipe();
	(void)snprintf(network_view.key_ssid, sizeof(network_view.key_ssid), "%s", ssid);
	network_view.key_open = 1;
	network_view.failure[0] = '\0';
	server->dirty = 1;
	printf("ZWL NETWORK key open ssid=%s\n", network_view.key_ssid);
}

/* Closes the key field, wiping what was typed. */
static void
network_key_close(
	void)
{
	/* Nothing of the key is kept. */
	network_key_wipe();
	network_view.key_open = 0;
}

/* Types one key into the field: a character, Backspace, Enter or Esc. */
static void
network_key_type(
	struct zwl_server *server,
	uint32_t key)
{
	char character;
	int shift;

	/* Esc closes the field and leaves the menu open. */
	if (key == NETWORK_KEY_ESC) {
		network_key_close();
		server->dirty = 1;
		printf("ZWL NETWORK key cancel\n");
		return;
	}

	/* Enter saves the key and joins. */
	if (key == NETWORK_KEY_ENTER || key == NETWORK_KEY_KPENTER) {
		network_key_submit(server);
		return;
	}

	/* Backspace takes back the last character. */
	if (key == NETWORK_KEY_BACKSPACE) {
		/* An empty field has nothing to take back. */
		if (network_view.key_length != 0U) {
			network_view.key_length--;
			network_view.key[network_view.key_length] = '\0';
		}
		server->dirty = 1;
		return;
	}

	/* Keys beyond the table type nothing. */
	if (key >= NETWORK_KEYS)
		return;

	/* Shift, and Caps Lock for the letters. */
	shift = 0;
	if ((server->modifiers & NETWORK_SHIFT) != 0U)
		shift = 1;
	character = network_plain[key];
	if ((server->locked_modifiers & NETWORK_CAPS) != 0U && character >= 'a' && character <= 'z')
		shift = !shift;
	if (shift)
		character = network_shifted[key];
	if (character == 0)
		return;

	/* A full field takes no more. */
	if (network_view.key_length >= NETWORK_KEY_MAX)
		return;

	/* The character. */
	network_view.key[network_view.key_length] = character;
	network_view.key_length++;
	network_view.key[network_view.key_length] = '\0';
	server->dirty = 1;
}

/*
 * Saves the key typed in the user's store, wipes it, and tells the daemon
 * the saved networks changed; the join follows that answer.
 */
static void
network_key_submit(
	struct zwl_server *server)
{
	int error;

	/* A WPA key is 8 to 63 characters; a shorter one is said and kept to finish. */
	if (network_view.key_length < NETWORK_KEY_MIN) {
		(void)snprintf(network_view.failure, sizeof(network_view.failure), "The key must be 8 to 63 characters");
		server->dirty = 1;
		return;
	}

	/* The key, saved in the user's own store. */
	error = kl_backend_network_save_key(network_view.key_ssid, network_view.key);
	network_key_wipe();
	if (error != 0) {
		(void)snprintf(network_view.failure, sizeof(network_view.failure), "Could not save the key (%s)", strerror(error));
		printf("ZWL NETWORK key save error=%d\n", error);
		server->dirty = 1;
		return;
	}
	printf("ZWL NETWORK key saved ssid=%s\n", network_view.key_ssid);

	/* The field closes; the daemon is told, and its answer sends the join. */
	network_view.key_open = 0;
	network_view.join_after_profiles = 1;
	(void)snprintf(network_view.joining, sizeof(network_view.joining), "%s", network_view.key_ssid);
	network_request(server, KL_BACKEND_NETWORK_REQUEST_PROFILES, NULL);
}

/* Wipes the key field's characters, every byte of it. */
static void
network_key_wipe(
	void)
{
	volatile char *byte;
	size_t index;

	/* Through a volatile pointer, so the stores are not left out as dead. */
	byte = network_view.key;
	for (index = 0; index < sizeof(network_view.key); index++)
		byte[index] = '\0';
	network_view.key_length = 0U;
}

/*
 * Follows a request that finished: says its failure in the menu, sends the
 * join that waited for a saved key to reach the daemon, opens the key field
 * for a network the daemon has no key for, and sends the request that
 * waited in the slot.
 */
static void
network_finished(
	struct zwl_server *server,
	unsigned request,
	int error)
{
	unsigned waiting;
	unsigned index;
	int differs;

	/* No failure from before. */
	network_view.failure[0] = '\0';

	/* The daemon has the new key: the join follows (the slot waits for it). */
	if (request == KL_BACKEND_NETWORK_REQUEST_PROFILES && network_view.join_after_profiles) {
		network_view.join_after_profiles = 0;
		network_request(server, KL_BACKEND_NETWORK_REQUEST_JOIN, network_view.key_ssid);
		return;
	}

	/* A join of a network that asks for a key, without one, asks for it. */
	if (error == ENOENT && request == KL_BACKEND_NETWORK_REQUEST_JOIN) {
		for (index = 0; index < network_view.scan_count; index++) {
			/* The scan's entry of the network, when it asks for a key. */
			differs = strcmp(network_view.scan[index].ssid, network_view.joining);
			if (differs == 0 && network_view.scan[index].secured)
				network_key_open(server, network_view.joining);
		}

		/* A network that asks for no key, or is no longer seen, says so. */
		if (!network_view.key_open)
			(void)snprintf(network_view.failure, sizeof(network_view.failure), "Could not join %s: no saved key", network_view.joining);
	} else if (error == EPERM &&
	    request == KL_BACKEND_NETWORK_REQUEST_JOIN &&
	    network_view.state.wifi == KL_BACKEND_WIFI_OFF) {
		/* networkd refuses a join while Wi-Fi is off. */
		(void)snprintf(network_view.failure, sizeof(network_view.failure), "Wi-Fi is off; turn it on to join");
	} else if (error == EPERM) {
		/* Only root and the network group may control Wi-Fi (2026-10-02, ws005-p019). */
		(void)snprintf(network_view.failure, sizeof(network_view.failure), "This account may not control Wi-Fi");
	} else if (error != 0 && request != KL_BACKEND_NETWORK_REQUEST_SCAN) {
		/* Anything else that failed says the errno's text. */
		(void)snprintf(network_view.failure, sizeof(network_view.failure), "Could not %s (%s)", network_request_name(request), strerror(error));
	}

	/* What waited in the slot is sent now. */
	waiting = network_view.pending_request;
	if (waiting != KL_BACKEND_NETWORK_REQUEST_NONE) {
		network_view.pending_request = KL_BACKEND_NETWORK_REQUEST_NONE;

		/* A join names its network; the other requests take none. */
		if (network_view.pending_ssid[0] != '\0') {
			network_request(server, waiting, network_view.pending_ssid);
		} else {
			network_request(server, waiting, NULL);
		}
	}
}
