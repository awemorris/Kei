/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The login screen (ws035-p095, plan/ws035/login-manager-design.md).
 *
 * zdesktop --greeter draws it in place of the desktop and opens no Wayland
 * socket.  sessiond starts it as the unprivileged _greeter account, with
 * the display and the input devices given to that account, and answers on
 * the descriptor --auth-fd names:
 *
 *   READY                   GO: the display may be taken (handoff.c)
 *   AUTH name password      OK: the user is in; FAIL
 *   POWER poweroff|reboot   OK (sent by libkeiland-backend's power, ws131-p005)
 *
 * After OK the screen says "Starting session..." and takes no input until
 * sessiond closes the descriptor, once the session is ready to take the
 * display (ws035-p101); zdesktop then ends.
 *
 * The same screen is a session's lock (ws035-p102, zwl_lock): the session's
 * user only, no power buttons, and the password goes to sessiond on the
 * session's descriptor (--control-fd) as UNLOCK password; OK unlocks, FAIL
 * (after sessiond's delay) asks again.  Its answers come through
 * handoff.c, which reads that descriptor.
 *
 * The screen is the blurred wallpaper with the time and the date at the
 * top, a frosted card in the middle with the users (the accounts with a uid
 * of 1000 or more and a login shell, or root when there are none), the
 * selected user's password field and the Log In button, and Restart and
 * Shut Down at the bottom right.  The password is shown as dots, sent to
 * sessiond and erased at once; it is never logged.
 *
 * Keys: the characters type into the password (US layout, Shift and Caps
 * Lock), Backspace erases, Esc clears, Enter logs in, Up, Down and Tab
 * choose the user.
 */

#include "glass.h"

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* The users shown, at most, and the longest name and password. */
#define GREETER_USERS		8U
#define GREETER_NAME		32U
#define GREETER_PASSWORD	128U

/* The first uid of a person's account, and the first of the system's high ones. */
#define GREETER_UID_FIRST	1000U
#define GREETER_UID_LAST	59999U

/* The card: its width, its corner, a user row's height, and the password field's height. */
#define GREETER_CARD_WIDTH	380
#define GREETER_CARD_RADIUS	22.0f
#define GREETER_ROW		44
#define GREETER_FIELD		44
#define GREETER_AVATAR		72

/* The power buttons' size, and their gap to the output's edge. */
#define GREETER_BUTTON_WIDTH	112
#define GREETER_BUTTON_HEIGHT	36
#define GREETER_MARGIN		24

/* The longest wait for the "Shutting down..." picture before the power request goes (ws099-p009). */
#define GREETER_POWER_MS	1000U

/* The dots of the power screen's spinner, and the time one turn takes. */
#define GREETER_SPINNER_DOTS	8U
#define GREETER_SPINNER_MS	1200U

/* The Kei mark's square at the bottom left, in pixels. */
#define GREETER_BRAND_MARK	48

/* The evdev codes of the keys the screen takes apart from the characters. */
#define GREETER_KEY_ESC		1U
#define GREETER_KEY_BACKSPACE	14U
#define GREETER_KEY_TAB		15U
#define GREETER_KEY_ENTER	28U
#define GREETER_KEY_KPENTER	96U
#define GREETER_KEY_UP		103U
#define GREETER_KEY_DOWN	108U

/* How many evdev codes the character tables cover (up to the space bar). */
#define GREETER_KEYS		58U

/* The depressed Shift and the locked Caps Lock in the seat's modifier masks. */
#define GREETER_SHIFT		0x1U
#define GREETER_CAPS		0x2U

/* The pointer's left button. */
#define GREETER_BUTTON_LEFT	0x110U

/*
 * What a press on the screen hit: nothing, a user's row, the password
 * field, the Log In button, Restart or Shut Down.
 */
enum greeter_hit {
	GREETER_HIT_NONE,
	GREETER_HIT_USER,
	GREETER_HIT_FIELD,
	GREETER_HIT_LOGIN,
	GREETER_HIT_RESTART,
	GREETER_HIT_POWEROFF
};

/*
 * One user the screen offers: the account's name and the name shown.
 */
struct greeter_user {
	char name[GREETER_NAME];
	char shown[GREETER_NAME];
};

/*
 * Where the parts of the screen are this frame, in output pixels
 * (x, y, width, height), laid out again on every frame and press.
 */
struct greeter_layout {
	int32_t card[4];
	int32_t avatar[4];
	int32_t rows[GREETER_USERS][4];
	int32_t field[4];
	int32_t login[4];
	int32_t restart[4];
	int32_t poweroff[4];
};

/*
 * The users read once at the start, the one selected, what has been typed
 * (erased as soon as it is sent), the line under the field, whether an
 * answer is awaited, and the partial answer read so far.  zdesktop runs one
 * greeter, so these live for the process.
 */
static struct greeter_user greeter_users[GREETER_USERS];
static unsigned greeter_user_count;
static unsigned greeter_selected;
static char greeter_password[GREETER_PASSWORD];
static unsigned greeter_password_length;
static char greeter_message[64];
static unsigned greeter_waiting;
static unsigned greeter_starting;

/*
 * Shut Down or Restart pressed (ws099-p009): the power request waiting to
 * go ("poweroff" or "reboot", empty when none), whether it has gone, and the
 * frame count and time when it was pressed.  The screen says "Shutting
 * down..." first, and the request goes to sessiond once that picture has
 * been shown (the frame count has moved on twice, or GREETER_POWER_MS has
 * passed), so the picture the display keeps at the end is that one.
 */
static char greeter_powering[16];
static unsigned greeter_power_sent;
static uint64_t greeter_power_frame;
static uint64_t greeter_power_ms;
static char greeter_answer[64];
static size_t greeter_answer_used;

/* The characters each key types, without and with Shift (US layout); 0 for none. */
static const char greeter_plain[GREETER_KEYS] = {
	0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' '
};
static const char greeter_shifted[GREETER_KEYS] = {
	0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0, 0,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' '
};

static void greeter_read_users(void);
static void greeter_add_user(const char *name, const char *gecos);
static void greeter_layout(struct zwl_server *server, struct greeter_layout *layout);
static enum greeter_hit greeter_hit(struct zwl_server *server, const struct greeter_layout *layout, unsigned *user);
static int greeter_inside(const int32_t *rect, int32_t x, int32_t y);
static void greeter_draw_card(struct zwl_server *server, VkCommandBuffer command, const struct greeter_layout *layout);
static void greeter_draw_field(struct zwl_server *server, VkCommandBuffer command, const struct greeter_layout *layout);
static void greeter_draw_button(struct zwl_server *server, VkCommandBuffer command, const int32_t *rect, const char *label, int strong);
static void greeter_draw_clock(struct zwl_server *server, VkCommandBuffer command);
static void greeter_draw_brand(struct zwl_server *server, VkCommandBuffer command);
static void greeter_draw_centered(struct zwl_server *server, VkCommandBuffer command, enum glass_size size, int32_t middle, int32_t baseline, const char *text, int32_t limit, const float *color);
static void greeter_select(struct zwl_server *server, unsigned user);
static void greeter_type(struct zwl_server *server, uint32_t key);
static void greeter_submit(struct zwl_server *server);
static void greeter_power(struct zwl_server *server, const char *what);
static void greeter_power_send(struct zwl_server *server);
static void greeter_draw_power(struct zwl_server *server, VkCommandBuffer command, const struct greeter_layout *layout);
static void greeter_send(struct zwl_server *server, const char *line);
static void greeter_answered(struct zwl_server *server, const char *answer);
static void greeter_erase(void);
static int greeter_descriptor(const struct zwl_server *server);

/*
 * Prepares the login screen: the users, and the answers' descriptor.
 */
int
zwl_greeter_open(
	struct zwl_server *server)
{
	int flags;
	int error;

	/* The users shown. */
	greeter_read_users();
	greeter_selected = 0U;
	greeter_erase();
	greeter_message[0] = '\0';

	/* sessiond's answers are read without waiting for them. */
	flags = fcntl(server->auth_fd, F_GETFL);
	if (flags < 0) {
		printf("ZWL GREETER auth-fd=%d errno=%d\n", server->auth_fd, errno);
		return EBADF;
	}

	/* The descriptor does not wait, and does not go to the programs zdesktop starts. */
	error = fcntl(server->auth_fd, F_SETFL, flags | O_NONBLOCK);
	if (error != 0)
		return errno;
	(void)fcntl(server->auth_fd, F_SETFD, FD_CLOEXEC);

	/* Succeeded: the screen can be drawn. */
	printf("ZWL GREETER open users=%u selected=%s\n", greeter_user_count, greeter_users[0].name);
	return 0;
}

/*
 * Locks a session (ws035-p102): the lock screen covers the desktop and
 * takes every key and button until the user's password unlocks it.
 * Returns 1 when locked, 0 when this zdesktop cannot be unlocked (no
 * sessiond to check the password) and so is not locked.
 */
int
zwl_lock(
	struct zwl_server *server,
	const char *reason)
{
	struct passwd *entry;

	/* Only a session sessiond started, and once. */
	if (server->greeter || server->control_fd < 0)
		return 0;
	if (server->locked)
		return 1;

	/* The session's own user, and nothing typed. */
	greeter_user_count = 0U;
	entry = getpwuid(getuid());
	if (entry != NULL)
		greeter_add_user(entry->pw_name, entry->pw_gecos);
	if (greeter_user_count == 0U)
		greeter_add_user("?", NULL);
	greeter_selected = 0U;
	greeter_erase();
	greeter_message[0] = '\0';
	greeter_waiting = 0U;
	greeter_starting = 0U;

	/* The clipboard's history goes (clipboard.c). */
	zwl_clipboard_history_clear(server, "lock");

	/* Succeeded: the lock screen shows. */
	server->locked = 1U;
	server->dirty = 1;
	printf("ZWL LOCK locked reason=%s user=%s\n", reason, greeter_users[0].name);
	return 1;
}

/*
 * Acts on an answer of sessiond's to the lock screen's UNLOCK (handoff.c
 * reads it from the session's descriptor).
 */
void
zwl_lock_answer(
	struct zwl_server *server,
	const char *answer)
{
	/* Only while the lock screen shows. */
	if (!server->locked)
		return;

	/* The same answers as the login screen's. */
	greeter_answered(server, answer);
}

/*
 * Draws the login screen over the whole output.
 */
void
zwl_greeter_draw(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	struct greeter_layout layout;
	struct glass_shape shape;

	/* Where everything goes. */
	greeter_layout(server, &layout);

	/*
	 * The wallpaper, blurred and washed towards a pale sky so the screen is
	 * as bright and airy as the boot screen; the words on it are dark slate
	 * (ws035-p109).
	 */
	glass_shape_init(&shape, 0.0f, 0.0f, (float)server->width, (float)server->height);
	shape.mode = MODE_GLASS;
	shape.color[0] = 0.96f;
	shape.color[1] = 0.98f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.30f;
	shape.opaque = 1.0f;
	glass_shape_draw(server, command, &shape);

	/* The time and the date. */
	greeter_draw_clock(server, command);

	/* The Kei mark and word at the bottom left (ws035-p108). */
	greeter_draw_brand(server, command);

	/* Shutting down or restarting: only that, with a spinner, in the card. */
	if (greeter_powering[0] != '\0') {
		greeter_draw_power(server, command, &layout);
		return;
	}

	/* The card with the users, the password and Log In. */
	greeter_draw_card(server, command, &layout);

	/* The power buttons (not on a session's lock screen). */
	if (!server->locked) {
		greeter_draw_button(server, command, layout.restart, "Restart", 0);
		greeter_draw_button(server, command, layout.poweroff, "Shut Down", 0);
	}
}

/*
 * Handles a pointer button on the login screen: every button is the screen's.
 */
int
zwl_greeter_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	struct greeter_layout layout;
	enum greeter_hit hit;
	unsigned user;

	/* Only the left button's press does anything, and nothing once the session is starting or the machine ending. */
	if (button != GREETER_BUTTON_LEFT || state == 0U || greeter_starting || greeter_powering[0] != '\0')
		return 1;

	/* What the press is on. */
	greeter_layout(server, &layout);
	user = 0U;
	hit = greeter_hit(server, &layout, &user);

	/* Acts on it. */
	switch (hit) {
	case GREETER_HIT_USER:
		greeter_select(server, user);
		break;
	case GREETER_HIT_LOGIN:
		greeter_submit(server);
		break;
	case GREETER_HIT_RESTART:
		if (!server->locked)
			greeter_power(server, "reboot");
		break;
	case GREETER_HIT_POWEROFF:
		if (!server->locked)
			greeter_power(server, "poweroff");
		break;
	default:
		break;
	}

	/* The press was the screen's. */
	server->dirty = 1;
	return 1;
}

/*
 * Handles a key on the login screen: every key is the screen's.
 */
int
zwl_greeter_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	/* Releases do nothing, and nothing does once the session is starting or the machine ending. */
	if (state == 0U || greeter_starting || greeter_powering[0] != '\0')
		return 1;

	/* Routes the key by its code. */
	server->dirty = 1;
	switch (key) {
	case GREETER_KEY_ENTER:
	case GREETER_KEY_KPENTER:
		/* Enter logs in. */
		greeter_submit(server);
		return 1;
	case GREETER_KEY_BACKSPACE:
		/* Backspace erases the last character. */
		if (greeter_password_length > 0U) {
			greeter_password_length--;
			greeter_password[greeter_password_length] = '\0';
		}

		/* The key was the screen's. */
		return 1;
	case GREETER_KEY_ESC:
		/* Esc clears the password. */
		greeter_erase();
		return 1;
	case GREETER_KEY_UP:
		/* The user above. */
		if (greeter_selected > 0U)
			greeter_select(server, greeter_selected - 1U);
		return 1;
	case GREETER_KEY_DOWN:
	case GREETER_KEY_TAB:
		/* The user below, from the last back to the first. */
		greeter_select(server, (greeter_selected + 1U) % greeter_user_count);
		return 1;
	default:
		break;
	}

	/* Any other key may type a character. */
	greeter_type(server, key);
	return 1;
}

/*
 * Reads sessiond's answers, and redraws when the minute changes.
 */
void
zwl_greeter_tick(
	struct zwl_server *server)
{
	char *end;
	ssize_t count;
	int64_t minute;

	/* The clock shows a new minute. */
	minute = (int64_t)(time(NULL) / 60);
	if (minute != server->clock_minute) {
		server->clock_minute = minute;
		server->dirty = 1;
	}

	/* The lock screen's answers come through handoff.c. */
	if (!server->greeter)
		return;

	/* Shutting down: the spinner turns, and the request goes once its picture is shown. */
	if (greeter_powering[0] != '\0') {
		server->dirty = 1;
		greeter_power_send(server);
	}

	/* What sessiond has answered. */
	count = read(server->auth_fd, greeter_answer + greeter_answer_used, sizeof(greeter_answer) - 1U - greeter_answer_used);
	if (count < 0)
		return;

	/* sessiond gone: the screen ends. */
	if (count == 0) {
		printf("ZWL GREETER closed at_ms=%llu\n", (unsigned long long)zwl_milliseconds());
		zwl_handoff_release(server);
		zwl_request_stop();
		return;
	}

	/* Each whole answer. */
	greeter_answer_used += (size_t)count;
	greeter_answer[greeter_answer_used] = '\0';
	for (;;) {
		end = strchr(greeter_answer, '\n');
		if (end == NULL)
			break;
		*end = '\0';
		greeter_answered(server, greeter_answer);
		greeter_answer_used -= (size_t)(end - greeter_answer) + 1U;
		memmove(greeter_answer, end + 1, greeter_answer_used + 1U);
	}

	/* An answer that never ends is thrown away. */
	if (greeter_answer_used + 1U >= sizeof(greeter_answer))
		greeter_answer_used = 0U;
}

/* Reads the users offered: the people's accounts, or root when there are none. */
static void
greeter_read_users(
	void)
{
	struct passwd *entry;
	const char *shell;
	size_t length;
	int nologin;
	int match;

	/* Each account with a person's uid and a shell to log in to. */
	greeter_user_count = 0U;
	setpwent();
	for (;;) {
		entry = getpwent();
		if (entry == NULL)
			break;
		if (entry->pw_uid < GREETER_UID_FIRST || entry->pw_uid > GREETER_UID_LAST)
			continue;

		/* An account whose shell is nologin or false cannot log in. */
		shell = entry->pw_shell;
		length = strlen(shell);
		nologin = 0;
		if (length >= 7U) {
			match = strcmp(shell + length - 7U, "nologin");
			if (match == 0)
				nologin = 1;
		}

		/* The same for false. */
		if (length >= 5U) {
			match = strcmp(shell + length - 5U, "false");
			if (match == 0)
				nologin = 1;
		}

		/* Such an account is not offered. */
		if (nologin)
			continue;

		/* A person who can log in. */
		greeter_add_user(entry->pw_name, entry->pw_gecos);
	}

	/* The accounts are read. */
	endpwent();

	/* A machine without a person's account offers root. */
	if (greeter_user_count == 0U)
		greeter_add_user("root", "System Administrator");
}

/* Adds one user, shown by the first part of its GECOS field when it has one. */
static void
greeter_add_user(
	const char *name,
	const char *gecos)
{
	struct greeter_user *user;
	char *comma;

	/* The list holds what fits. */
	if (greeter_user_count >= GREETER_USERS)
		return;

	/* The name, and the name shown. */
	user = &greeter_users[greeter_user_count];
	snprintf(user->name, sizeof(user->name), "%s", name);
	snprintf(user->shown, sizeof(user->shown), "%s", name);
	if (gecos != NULL && gecos[0] != '\0' && gecos[0] != ',') {
		snprintf(user->shown, sizeof(user->shown), "%s", gecos);
		comma = strchr(user->shown, ',');
		if (comma != NULL)
			*comma = '\0';
	}

	/* One more in the list. */
	greeter_user_count++;
}

/* Lays the screen out for the output's size and the number of users. */
static void
greeter_layout(
	struct zwl_server *server,
	struct greeter_layout *layout)
{
	int32_t width;
	int32_t height;
	int32_t card_height;
	int32_t x;
	int32_t y;
	unsigned index;
	unsigned rows;

	/* The card: the avatar, the name, the other users' rows, the field and a line for messages. */
	width = (int32_t)server->width;
	height = (int32_t)server->height;
	rows = 0U;
	if (greeter_user_count > 1U)
		rows = greeter_user_count;
	card_height = 28 + GREETER_AVATAR + 48 + (int32_t)rows * GREETER_ROW + 12 + GREETER_FIELD + 44;
	layout->card[2] = GREETER_CARD_WIDTH;
	layout->card[3] = card_height;
	layout->card[0] = (width - GREETER_CARD_WIDTH) / 2;
	layout->card[1] = height / 2 - card_height / 2 + height / 16;
	x = layout->card[0];
	y = layout->card[1] + 28;

	/* The selected user's avatar, centred. */
	layout->avatar[0] = x + (GREETER_CARD_WIDTH - GREETER_AVATAR) / 2;
	layout->avatar[1] = y;
	layout->avatar[2] = GREETER_AVATAR;
	layout->avatar[3] = GREETER_AVATAR;
	y += GREETER_AVATAR + 48;

	/* One row a user when there is more than one. */
	for (index = 0U; index < GREETER_USERS; index++) {
		layout->rows[index][0] = x + 24;
		layout->rows[index][1] = y + (int32_t)index * GREETER_ROW;
		layout->rows[index][2] = GREETER_CARD_WIDTH - 48;
		layout->rows[index][3] = GREETER_ROW - 4;
	}

	/* Below the rows. */
	y += (int32_t)rows * GREETER_ROW + 12;

	/* The password field, with Log In at its right. */
	layout->field[0] = x + 24;
	layout->field[1] = y;
	layout->field[2] = GREETER_CARD_WIDTH - 48 - GREETER_FIELD - 8;
	layout->field[3] = GREETER_FIELD;
	layout->login[0] = layout->field[0] + layout->field[2] + 8;
	layout->login[1] = y;
	layout->login[2] = GREETER_FIELD;
	layout->login[3] = GREETER_FIELD;

	/* Restart and Shut Down at the bottom right. */
	layout->poweroff[0] = width - GREETER_MARGIN - GREETER_BUTTON_WIDTH;
	layout->poweroff[1] = height - GREETER_MARGIN - GREETER_BUTTON_HEIGHT;
	layout->poweroff[2] = GREETER_BUTTON_WIDTH;
	layout->poweroff[3] = GREETER_BUTTON_HEIGHT;
	layout->restart[0] = layout->poweroff[0] - 12 - GREETER_BUTTON_WIDTH;
	layout->restart[1] = layout->poweroff[1];
	layout->restart[2] = GREETER_BUTTON_WIDTH;
	layout->restart[3] = GREETER_BUTTON_HEIGHT;
}

/* Says what the pointer is on, and which user's row when it is one. */
static enum greeter_hit
greeter_hit(
	struct zwl_server *server,
	const struct greeter_layout *layout,
	unsigned *user)
{
	unsigned index;
	int inside;

	/* A user's row, when rows are shown. */
	for (index = 0U; greeter_user_count > 1U && index < greeter_user_count; index++) {
		inside = greeter_inside(layout->rows[index], server->pointer_x, server->pointer_y);
		if (inside) {
			*user = index;
			return GREETER_HIT_USER;
		}
	}

	/* The field, Log In and the power buttons. */
	inside = greeter_inside(layout->field, server->pointer_x, server->pointer_y);
	if (inside)
		return GREETER_HIT_FIELD;
	inside = greeter_inside(layout->login, server->pointer_x, server->pointer_y);
	if (inside)
		return GREETER_HIT_LOGIN;
	inside = greeter_inside(layout->restart, server->pointer_x, server->pointer_y);
	if (inside)
		return GREETER_HIT_RESTART;
	inside = greeter_inside(layout->poweroff, server->pointer_x, server->pointer_y);
	if (inside)
		return GREETER_HIT_POWEROFF;

	/* Nothing. */
	return GREETER_HIT_NONE;
}

/* Reports whether a point is inside a rectangle (x, y, width, height). */
static int
greeter_inside(
	const int32_t *rect,
	int32_t x,
	int32_t y)
{
	/* Left of it or above it. */
	if (x < rect[0] || y < rect[1])
		return 0;

	/* Right of it or below it. */
	if (x >= rect[0] + rect[2] || y >= rect[1] + rect[3])
		return 0;

	/* Succeeded: the point is inside. */
	return 1;
}

/* Draws the frosted card: the avatar and name, the users' rows, the field and the message. */
static void
greeter_draw_card(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct greeter_layout *layout)
{
	static const float ink[4] = { 0.10f, 0.14f, 0.22f, 1.0f };
	static const float faint[4] = { 0.10f, 0.14f, 0.22f, 0.62f };
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float avatar[4] = { 0.26f, 0.48f, 0.86f, 1.0f };
	static const float chosen[4] = { 1.0f, 1.0f, 1.0f, 0.55f };
	static const float hover[4] = { 1.0f, 1.0f, 1.0f, 0.28f };
	static const float warning[4] = { 0.72f, 0.12f, 0.10f, 1.0f };
	struct glass_shape shape;
	const struct greeter_user *user;
	char letter[2];
	int32_t middle;
	int32_t baseline;
	unsigned index;
	int inside;

	/* The card's shadow. */
	glass_shape_init(&shape, (float)layout->card[0], (float)layout->card[1] + 10.0f, (float)layout->card[2], (float)layout->card[3]);
	shape.quad[0] -= 60.0f;
	shape.quad[1] -= 60.0f;
	shape.quad[2] += 120.0f;
	shape.quad[3] += 120.0f;
	shape.mode = MODE_SHADOW;
	shape.radius = GREETER_CARD_RADIUS;
	shape.soft = 30.0f;
	shape.color[0] = 0.12f;
	shape.color[1] = 0.20f;
	shape.color[2] = 0.34f;
	shape.color[3] = 0.16f;
	glass_shape_draw(server, command, &shape);

	/* The frosted glass, whiter than the windows' so the dark text reads on it. */
	glass_shape_init(&shape, (float)layout->card[0], (float)layout->card[1], (float)layout->card[2], (float)layout->card[3]);
	shape.mode = MODE_GLASS;
	shape.radius = GREETER_CARD_RADIUS;
	shape.soft = 1.0f;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.62f;
	shape.edge = 0.70f;
	glass_shape_draw(server, command, &shape);

	/* The selected user's avatar: a circle with the first letter of the name. */
	user = &greeter_users[greeter_selected];
	glass_draw_solid(server, command, (float)layout->avatar[0], (float)layout->avatar[1], (float)GREETER_AVATAR, (float)GREETER_AVATAR, (float)GREETER_AVATAR / 2.0f, avatar);
	letter[0] = user->shown[0];
	if (letter[0] >= 'a' && letter[0] <= 'z')
		letter[0] = (char)(letter[0] - 'a' + 'A');
	letter[1] = '\0';
	middle = layout->avatar[0] + GREETER_AVATAR / 2;
	greeter_draw_centered(server, command, SIZE_ICON, middle, layout->avatar[1] + GREETER_AVATAR / 2 + 13, letter, GREETER_AVATAR, white);

	/* The name under it. */
	baseline = layout->avatar[1] + GREETER_AVATAR + 32;
	greeter_draw_centered(server, command, SIZE_SEARCH, middle, baseline, user->shown, GREETER_CARD_WIDTH - 32, ink);

	/* The users' rows, the selected one lit, when there is more than one. */
	for (index = 0U; greeter_user_count > 1U && index < greeter_user_count; index++) {
		inside = greeter_inside(layout->rows[index], server->pointer_x, server->pointer_y);
		if (index == greeter_selected) {
			glass_draw_solid(server, command, (float)layout->rows[index][0], (float)layout->rows[index][1], (float)layout->rows[index][2], (float)layout->rows[index][3], 10.0f, chosen);
		} else if (inside) {
			glass_draw_solid(server, command, (float)layout->rows[index][0], (float)layout->rows[index][1], (float)layout->rows[index][2], (float)layout->rows[index][3], 10.0f, hover);
		}

		/* The name shown, and the account's name at the right. */
		glass_draw_text(server, command, SIZE_TITLE, layout->rows[index][0] + 14, layout->rows[index][1] + 26, greeter_users[index].shown, layout->rows[index][2] - 28, ink);
		glass_draw_text(server, command, SIZE_BAR, layout->rows[index][0] + layout->rows[index][2] - 14 - glass_text_width(server, SIZE_BAR, greeter_users[index].name), layout->rows[index][1] + 26, greeter_users[index].name, 120, faint);
	}

	/* The password field and Log In. */
	greeter_draw_field(server, command, layout);

	/* The line under the field: a wrong password, or the wait for the answer. */
	baseline = layout->field[1] + GREETER_FIELD + 28;
	if (greeter_starting) {
		greeter_draw_centered(server, command, SIZE_TITLE, middle, baseline, "Starting session...", GREETER_CARD_WIDTH - 32, faint);
	} else if (greeter_waiting) {
		greeter_draw_centered(server, command, SIZE_TITLE, middle, baseline, "Checking...", GREETER_CARD_WIDTH - 32, faint);
	} else if (greeter_message[0] != '\0') {
		greeter_draw_centered(server, command, SIZE_TITLE, middle, baseline, greeter_message, GREETER_CARD_WIDTH - 32, warning);
	}
}

/* Draws the password field (dots for the characters, or its hint) and the Log In button. */
static void
greeter_draw_field(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct greeter_layout *layout)
{
	static const float field[4] = { 1.0f, 1.0f, 1.0f, 0.86f };
	static const float rim[4] = { 0.26f, 0.48f, 0.86f, 0.90f };
	static const float dot[4] = { 0.10f, 0.14f, 0.22f, 1.0f };
	static const float hint[4] = { 0.10f, 0.14f, 0.22f, 0.45f };
	float x;
	float y;
	unsigned index;
	unsigned shown;

	/* The field: a blue rim around a white box. */
	glass_draw_solid(server, command, (float)layout->field[0] - 2.0f, (float)layout->field[1] - 2.0f, (float)layout->field[2] + 4.0f, (float)layout->field[3] + 4.0f, 12.0f, rim);
	glass_draw_solid(server, command, (float)layout->field[0], (float)layout->field[1], (float)layout->field[2], (float)layout->field[3], 10.0f, field);

	/* The hint while it is empty. */
	if (greeter_password_length == 0U) {
		glass_draw_text(server, command, SIZE_TITLE, layout->field[0] + 16, layout->field[1] + 28, "Password", layout->field[2] - 32, hint);
	}

	/* A dot a character, as many as fit. */
	shown = greeter_password_length;
	if (shown > (unsigned)((layout->field[2] - 32) / 16))
		shown = (unsigned)((layout->field[2] - 32) / 16);
	y = (float)layout->field[1] + (float)GREETER_FIELD / 2.0f - 5.0f;
	for (index = 0U; index < shown; index++) {
		x = (float)layout->field[0] + 16.0f + (float)index * 16.0f;
		glass_draw_solid(server, command, x, y, 10.0f, 10.0f, 5.0f, dot);
	}

	/* Log In: a right arrow (U+2192, from the glyph cache and its fallback font) on the blue button (ws035-p116). */
	greeter_draw_button(server, command, layout->login, "\xe2\x86\x92", 1);
}

/* Draws one button: frosted, or blue when it is the strong one; lit under the pointer. */
static void
greeter_draw_button(
	struct zwl_server *server,
	VkCommandBuffer command,
	const int32_t *rect,
	const char *label,
	int strong)
{
	static const float blue[4] = { 0.20f, 0.44f, 0.86f, 1.0f };
	static const float blue_lit[4] = { 0.28f, 0.54f, 0.95f, 1.0f };
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float slate[4] = { 0.17f, 0.23f, 0.31f, 1.0f };
	struct glass_shape shape;
	int inside;

	/* Whether the pointer is on it. */
	inside = greeter_inside(rect, server->pointer_x, server->pointer_y);

	/* The strong button is solid blue. */
	if (strong) {
		if (inside) {
			glass_draw_solid(server, command, (float)rect[0], (float)rect[1], (float)rect[2], (float)rect[3], 10.0f, blue_lit);
		} else {
			glass_draw_solid(server, command, (float)rect[0], (float)rect[1], (float)rect[2], (float)rect[3], 10.0f, blue);
		}

		/* Its label, white. */
		greeter_draw_centered(server, command, SIZE_SEARCH, rect[0] + rect[2] / 2, rect[1] + rect[3] / 2 + 8, label, rect[2], white);
		return;
	}

	/* The others are light frosted glass with a slate label, whiter under the pointer. */
	glass_shape_init(&shape, (float)rect[0], (float)rect[1], (float)rect[2], (float)rect[3]);
	shape.mode = MODE_GLASS;
	shape.radius = (float)rect[3] / 2.0f;
	shape.soft = 1.0f;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.55f;
	if (inside)
		shape.color[3] = 0.78f;
	shape.edge = 0.80f;
	glass_shape_draw(server, command, &shape);
	greeter_draw_centered(server, command, SIZE_TITLE, rect[0] + rect[2] / 2, rect[1] + rect[3] / 2 + 5, label, rect[2] - 8, slate);
}

/*
 * Draws the Kei mark and the word Kei at the bottom left of the output, as
 * on the boot screen (the word in three letters, never a lone K).
 */
static void
greeter_draw_brand(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	static const float slate[4] = { 0.17f, 0.23f, 0.31f, 0.95f };
	int32_t x;
	int32_t y;

	/* The mark's square, level with the power buttons' foot. */
	x = GREETER_MARGIN;
	y = (int32_t)server->height - GREETER_MARGIN - GREETER_BRAND_MARK;
	glass_draw_mark(server, command, x, y, GREETER_BRAND_MARK, GLASS_MARK_SPLASH, 1.0f);

	/* The word beside it, on the mark's lower part, in the boot screen's slate. */
	glass_draw_text(server, command, SIZE_ICON, x + GREETER_BRAND_MARK + 8, y + GREETER_BRAND_MARK - 12, "Kei", 200, slate);
}

/* Draws the time, large, and the date under it, at the top of the output. */
static void
greeter_draw_clock(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	static const float slate[4] = { 0.15f, 0.21f, 0.29f, 1.0f };
	static const float soft[4] = { 0.20f, 0.27f, 0.36f, 0.88f };
	struct glass_shape shape;
	char text[64];
	struct tm local;
	time_t now;
	int32_t middle;
	int32_t top;

	/* The time now. */
	now = time(NULL);
	memset(&local, 0, sizeof(local));
	(void)localtime_r(&now, &local);
	middle = (int32_t)server->width / 2;
	top = (int32_t)server->height / 7;

	/*
	 * A soft pale glow behind the time and the date: a local scrim that
	 * keeps the slate words readable on a bright or busy wallpaper without
	 * darkening the screen (ws035-p109).
	 */
	glass_shape_init(&shape, (float)(middle - 190), (float)top - 10.0f, 380.0f, 104.0f);
	shape.quad[0] -= 70.0f;
	shape.quad[1] -= 70.0f;
	shape.quad[2] += 140.0f;
	shape.quad[3] += 140.0f;
	shape.mode = MODE_SHADOW;
	shape.radius = 52.0f;
	shape.soft = 60.0f;
	shape.color[0] = 0.97f;
	shape.color[1] = 0.99f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.42f;
	glass_shape_draw(server, command, &shape);

	/* The time. */
	(void)strftime(text, sizeof(text), "%H:%M", &local);
	greeter_draw_centered(server, command, SIZE_ICON, middle, top + 36, text, (int32_t)server->width, slate);

	/* The date. */
	(void)strftime(text, sizeof(text), "%A, %B %e", &local);
	greeter_draw_centered(server, command, SIZE_SEARCH, middle, top + 76, text, (int32_t)server->width, soft);
}

/* Draws a line of text centred on a point of its baseline, no wider than limit. */
static void
greeter_draw_centered(
	struct zwl_server *server,
	VkCommandBuffer command,
	enum glass_size size,
	int32_t middle,
	int32_t baseline,
	const char *text,
	int32_t limit,
	const float *color)
{
	int32_t width;

	/* How wide it is, at most the limit (a longer text is cut in its middle). */
	width = glass_text_width(server, size, text);
	if (width > limit)
		width = limit;

	/* Half of it to the left of the middle. */
	glass_draw_text_middle(server, command, size, middle - width / 2, baseline, text, limit, color);
}

/* Selects a user, clearing what was typed for the one before. */
static void
greeter_select(
	struct zwl_server *server,
	unsigned user)
{
	/* A user that is not there. */
	if (user >= greeter_user_count)
		return;

	/* The new user starts with an empty password. */
	greeter_selected = user;
	greeter_erase();
	greeter_message[0] = '\0';
	server->dirty = 1;
	printf("ZWL GREETER select user=%s\n", greeter_users[user].name);
}

/* Types a key's character into the password. */
static void
greeter_type(
	struct zwl_server *server,
	uint32_t key)
{
	char character;
	int shift;

	/* Keys beyond the table type nothing. */
	if (key >= GREETER_KEYS)
		return;

	/* Shift, and Caps Lock for the letters. */
	shift = 0;
	if ((server->modifiers & GREETER_SHIFT) != 0U)
		shift = 1;
	character = greeter_plain[key];
	if ((server->locked_modifiers & GREETER_CAPS) != 0U && character >= 'a' && character <= 'z')
		shift = !shift;
	if (shift)
		character = greeter_shifted[key];
	if (character == 0)
		return;

	/* While an answer is awaited, or the password is full, nothing is typed. */
	if (greeter_waiting || greeter_password_length + 1U >= sizeof(greeter_password))
		return;

	/* The character. */
	greeter_password[greeter_password_length] = character;
	greeter_password_length++;
	greeter_password[greeter_password_length] = '\0';
	greeter_message[0] = '\0';
}

/* Sends the selected user's password to sessiond and erases it. */
static void
greeter_submit(
	struct zwl_server *server)
{
	char line[GREETER_NAME + GREETER_PASSWORD + 8];

	/* One question at a time. */
	if (greeter_waiting)
		return;

	/* The request (UNLOCK on a session's lock screen), sent; then nothing of the password is kept. */
	if (server->locked) {
		snprintf(line, sizeof(line), "UNLOCK %s\n", greeter_password);
	} else {
		snprintf(line, sizeof(line), "AUTH %s %s\n", greeter_users[greeter_selected].name, greeter_password);
	}

	/* Nothing typed is kept once it is in the request. */
	greeter_erase();
	greeter_waiting = 1;
	greeter_message[0] = '\0';
	greeter_send(server, line);
	memset(line, 0, sizeof(line));
	server->dirty = 1;
	printf("ZWL GREETER auth user=%s\n", greeter_users[greeter_selected].name);
}

/*
 * Starts ending the machine (Shut Down or Restart): the screen says so from
 * the next frame, and greeter_power_send sends the request once that
 * picture has been shown.
 */
static void
greeter_power(
	struct zwl_server *server,
	const char *what)
{
	/* The request to send, and when it was asked for. */
	(void)snprintf(greeter_powering, sizeof(greeter_powering), "%s", what);
	greeter_power_sent = 0U;
	greeter_power_frame = server->frame;
	greeter_power_ms = zwl_milliseconds();
	server->dirty = 1;
	printf("ZWL GREETER powering=%s frame=%llu\n", what, (unsigned long long)server->frame);
}

/*
 * Asks for the power action once the "Shutting down..." picture has been
 * shown (two frames on, or GREETER_POWER_MS).  The backend sends it to
 * sessiond (ws131-p005); sessiond's answer is read with the others.
 */
static void
greeter_power_send(
	struct zwl_server *server)
{
	uint64_t elapsed;
	unsigned action;
	int reboot;
	int error;

	/* Sent already. */
	if (greeter_power_sent)
		return;

	/* The picture is shown once two frames have gone since the press, or after the longest wait. */
	elapsed = zwl_milliseconds() - greeter_power_ms;
	if (server->frame < greeter_power_frame + 2U && elapsed < GREETER_POWER_MS)
		return;

	/* The action the button asked for. */
	action = KL_BACKEND_POWER_POWEROFF;
	reboot = strcmp(greeter_powering, "reboot");
	if (reboot == 0)
		action = KL_BACKEND_POWER_REBOOT;

	/* The request, through the backend. */
	greeter_power_sent = 1U;
	error = kl_backend_power_action(server->backend, action);
	if (error != 0)
		printf("ZWL GREETER send errno=%d\n", error);
	printf("ZWL GREETER power=%s frames=%llu ms=%llu\n", greeter_powering, (unsigned long long)(server->frame - greeter_power_frame), (unsigned long long)elapsed);
}

/* Draws the card of a machine that is ending: "Shutting down..." or "Restarting...", and a turning ring of dots. */
static void
greeter_draw_power(
	struct zwl_server *server,
	VkCommandBuffer command,
	const struct greeter_layout *layout)
{
	static const float ink[4] = { 0.10f, 0.14f, 0.22f, 1.0f };
	struct glass_shape shape;
	const char *words;
	float dot[4];
	float angle;
	float phase;
	float cx;
	float cy;
	int32_t middle;
	unsigned index;
	int reboot;

	/* The card's frosted glass, as the login card's. */
	glass_shape_init(&shape, (float)layout->card[0], (float)layout->card[1], (float)layout->card[2], (float)layout->card[3]);
	shape.mode = MODE_GLASS;
	shape.radius = GREETER_CARD_RADIUS;
	shape.soft = 1.0f;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.62f;
	shape.edge = 0.70f;
	glass_shape_draw(server, command, &shape);

	/* The words, in the card's upper half. */
	words = "Shutting down...";
	reboot = strcmp(greeter_powering, "reboot");
	if (reboot == 0)
		words = "Restarting...";
	middle = layout->card[0] + layout->card[2] / 2;
	greeter_draw_centered(server, command, SIZE_SEARCH, middle, layout->card[1] + layout->card[3] * 2 / 5, words, GREETER_CARD_WIDTH - 32, ink);

	/* The spinner under them: dots on a ring, the brightest turning with the time. */
	cx = (float)middle;
	cy = (float)(layout->card[1] + layout->card[3] * 2 / 3);
	phase = (float)(zwl_milliseconds() % GREETER_SPINNER_MS) / (float)GREETER_SPINNER_MS;
	for (index = 0U; index < GREETER_SPINNER_DOTS; index++) {
		angle = 6.2831853f * (float)index / (float)GREETER_SPINNER_DOTS;
		dot[0] = 0.26f;
		dot[1] = 0.48f;
		dot[2] = 0.86f;
		dot[3] = 0.25f + 0.75f * fmodf(1.0f + (float)index / (float)GREETER_SPINNER_DOTS - phase, 1.0f);
		glass_draw_solid(server, command, cx + 22.0f * sinf(angle) - 5.0f, cy - 22.0f * cosf(angle) - 5.0f, 10.0f, 10.0f, 5.0f, dot);
	}
}

/* Writes one request line to sessiond. */
static void
greeter_send(
	struct zwl_server *server,
	const char *line)
{
	size_t length;
	ssize_t written;

	/* The whole line in one write (it is short). */
	length = strlen(line);
	written = write(greeter_descriptor(server), line, length);
	if (written != (ssize_t)length) {
		printf("ZWL GREETER send errno=%d\n", errno);
		greeter_waiting = 0;
	}
}

/* Acts on one answer of sessiond's. */
static void
greeter_answered(
	struct zwl_server *server,
	const char *answer)
{
	int match;
	int failed;
	int refused;

	/* The screen is redrawn with the result. */
	server->dirty = 1;
	printf("ZWL GREETER answer=%s\n", answer);

	/* Unlocked: the desktop shows again. */
	match = strcmp(answer, "OK");
	if (match == 0 && greeter_waiting && server->locked) {
		greeter_waiting = 0;
		server->locked = 0U;
		server->lock_input_ms = zwl_milliseconds();
		printf("ZWL LOCK unlocked\n");
		return;
	}

	/* Logged in: the screen stays until sessiond closes the descriptor (the session is then ready). */
	if (match == 0 && greeter_waiting) {
		greeter_waiting = 0;
		greeter_starting = 1;
		printf("ZWL GREETER starting\n");
		return;
	}

	/* A wrong password, or a refused request. */
	failed = strcmp(answer, "FAIL");
	refused = strcmp(answer, "ERROR");
	if (failed == 0) {
		snprintf(greeter_message, sizeof(greeter_message), "Wrong password. Try again.");
	} else if (refused == 0) {
		snprintf(greeter_message, sizeof(greeter_message), "The login failed.");
	}

	/* The next password can be typed. */
	greeter_waiting = 0;
}

/* Returns the descriptor to sessiond: the login screen's, or a session's for its lock screen. */
static int
greeter_descriptor(
	const struct zwl_server *server)
{
	/* A session's lock screen asks on the session's descriptor. */
	if (server->locked)
		return server->control_fd;

	/* The login screen on its own. */
	return server->auth_fd;
}

/* Erases what has been typed. */
static void
greeter_erase(
	void)
{
	/* Every byte, not only the characters typed. */
	memset(greeter_password, 0, sizeof(greeter_password));
	greeter_password_length = 0U;
}
