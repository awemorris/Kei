/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The single wl_seat: pointer and keyboard objects, focus and event delivery.
 *
 * Focus follows the display: the surface currently scanned out has the
 * keyboard, and only that surface's client hears it.  The pointer goes to
 * the same window, or to the sub-surface of it under the pointer (p077);
 * while a popup's grab has the pointer, to the surface of the grab's chain
 * under it (p076).  A client that never asks for a pointer or keyboard receives
 * nothing.  wl_touch objects are made here; the fingers are touch.c's.  Coordinates are surface-local integers carried as wl_fixed, the
 * surface being the whole output of --width by --height pixels.
 */

#include "zwl.h"
#include "menu.h"
#include "titlebar.h"
#include "popup.h"
#include "toplevel.h"
#include "subsurface.h"
#include "data.h"
#include "extras.h"
#include "keymap.h"
#include "ime.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Event opcodes of wl_seat, in protocol order. */
#define SEAT_CAPABILITIES		0U
#define SEAT_NAME			1U

/* Event opcodes of wl_pointer, in protocol order. */
#define POINTER_ENTER			0U
#define POINTER_LEAVE			1U
#define POINTER_MOTION			2U
#define POINTER_BUTTON			3U
#define POINTER_AXIS			4U
#define POINTER_FRAME			5U
#define POINTER_AXIS_SOURCE		6U
#define POINTER_AXIS_DISCRETE		8U

/* Event opcodes of wl_keyboard, in protocol order. */
#define KEYBOARD_KEYMAP			0U
#define KEYBOARD_ENTER			1U
#define KEYBOARD_LEAVE			2U
#define KEYBOARD_KEY			3U
#define KEYBOARD_MODIFIERS		4U
#define KEYBOARD_REPEAT_INFO		5U

/* The first pointer and seat versions that carry frame/axis_source and release. */
#define POINTER_FRAME_VERSION		5U
#define KEYBOARD_REPEAT_VERSION		4U
#define RELEASE_VERSION			3U
#define SEAT_RELEASE_VERSION		5U

/* Esc (evdev), which gives up a drag and drop. */
#define SEAT_KEY_ESC			1U

/* The L key and the Super modifier's bit (ws035-p102: Super+L locks). */
#define SEAT_KEY_L			38U
#define SEAT_MODIFIER_SUPER		0x40U
#define SEAT_NAME_VERSION		2U

/* Protocol enumeration values used on the wire. */
#define AXIS_VERTICAL			0U
#define AXIS_HORIZONTAL			1U
#define AXIS_SOURCE_WHEEL		0U
#define KEYMAP_NO_KEYMAP		0U
#define KEYMAP_XKB_V1			1U

/* One wheel notch scrolls this many surface units, as common compositors do. */
#define WHEEL_STEP			15

static int create_device(struct zwl_object *seat, enum zwl_kind kind, const unsigned char *bytes, size_t size);
static void deliver(struct zwl_client *client, uint32_t id, uint32_t opcode, const void *payload, size_t size);
static void pointer_enter(struct zwl_object *pointer, struct zwl_object *surface, uint32_t serial);
static void keyboard_enter(struct zwl_object *keyboard, struct zwl_object *surface, uint32_t serial);
static void keyboard_modifiers(struct zwl_object *keyboard, uint32_t serial);
static void send_leave(struct zwl_object *surface);
static void set_cursor(struct zwl_server *server, struct zwl_client *client, struct zwl_object *surface, const unsigned char *bytes);
static void send_enter(struct zwl_object *surface);
static void report_seat(struct zwl_client *client);
static int keyboard_keymap(struct zwl_object *keyboard);
static uint32_t pointer_button(struct zwl_object *target, uint32_t time, uint32_t button, uint32_t state, unsigned complete);
static void interactive_clear(struct zwl_server *server);

/*
 * Reserves the next nonzero event serial shared by configure and input events.
 */
uint32_t
zwl_next_serial(
	struct zwl_server *server)
{
	/* Zero is never a valid serial, so the wrap skips it. */
	server->serial++;
	if (server->serial == 0)
		server->serial++;

	/* Succeeded: the caller owns this serial for one logical event. */
	return server->serial;
}

/*
 * Sends a newly bound seat its capabilities and, from version 2, its name.
 */
int
zwl_seat_bind(
	struct zwl_object *seat)
{
	unsigned char name[12];
	uint32_t word;
	int error;

	/* The capability bits describe the devices open right now. */
	word = seat->client->server->capabilities;
	error = zwl_emit(seat->client, seat->id, SEAT_CAPABILITIES, &word, sizeof(word));
	if (error != 0)
		return error;

	/* Version 1 seats have no name event. */
	if (seat->version < SEAT_NAME_VERSION)
		return 0;

	/* The name is the string "seat0": a length word, six bytes and two of padding. */
	memset(name, 0, sizeof(name));
	word = 6;
	memcpy(name, &word, sizeof(word));
	memcpy(name + 4, "seat0", 6);
	error = zwl_emit(seat->client, seat->id, SEAT_NAME, name, sizeof(name));
	if (error != 0)
		return error;

	/* Succeeded: the seat knows what it offers. */
	return 0;
}

/*
 * Applies one wl_seat, wl_pointer or wl_keyboard request.
 */
int
zwl_seat_request(
	struct zwl_object *object,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	uint32_t surface_id;
	struct zwl_object *surface;
	int error;

	/* Each interface validates its own requests; anything unmatched is refused. */
	error = EPROTO;
	switch (object->kind) {
	case ZWL_SEAT:
		/* The seat creates device objects or retires its own binding. */
		if (opcode == 0U) {
			/* get_pointer carries one new_id. */
			error = create_device(object, ZWL_POINTER, bytes, size);
		} else if (opcode == 1U) {
			/* get_keyboard carries one new_id. */
			error = create_device(object, ZWL_KEYBOARD, bytes, size);
		} else if (opcode == 2U) {
			/* get_touch carries one new_id (the fingers are delivered by touch.c). */
			error = create_device(object, ZWL_TOUCH, bytes, size);
		} else if (opcode == 3U && size == 0 && object->version >= SEAT_RELEASE_VERSION) {
			/* release exists from version 5. */
			zwl_object_destroy(object);
			error = 0;
		}

		break;
	case ZWL_POINTER:
		/* A pointer names a cursor image or retires its own binding. */
		if (opcode == 0U && size == 16U) {
			/* set_cursor names a surface of the client's own, or none (hides the cursor). */
			memcpy(&surface_id, bytes + 4, sizeof(surface_id));
			error = 0;
			surface = NULL;
			if (surface_id != 0) {
				/* A foreign object, or a surface with a window role, cannot become a cursor. */
				surface = zwl_find(object->client, surface_id);
				if (surface == NULL ||
				    surface->kind != ZWL_SURFACE ||
				    surface->role != NULL ||
				    surface->sub_role != NULL)
					error = EPROTO;
			}

			/* Only the client the pointer is over sets the cursor (design D8). */
			if (error == 0 &&
			    object->client->server->pointer_surface != NULL &&
			    object->client->server->pointer_surface->client == object->client)
				set_cursor(object->client->server, object->client, surface, bytes);
		} else if (opcode == 1U && size == 0 && object->version >= RELEASE_VERSION) {
			/* release exists from version 3. */
			zwl_object_destroy(object);
			error = 0;
		}

		break;
	case ZWL_KEYBOARD:
	case ZWL_TOUCH:
		/* release, from version 3, is the only keyboard or touch request. */
		if (opcode == 0U && size == 0 && object->version >= RELEASE_VERSION) {
			zwl_object_destroy(object);
			error = 0;
		}

		break;
	default:
		/* The dispatcher routes only seat interfaces here. */
		break;
	}

	/* Reports a malformed or unsupported request. */
	if (error != 0)
		return error;

	/* Succeeded: the request has been applied exactly once. */
	return 0;
}

/*
 * Shows zdesktop's arrow again (when the pointer goes to another client, or
 * the cursor surface goes).
 */
void
zwl_cursor_default(
	struct zwl_server *server)
{
	/* The arrow, shown (no client's surface or shape). */
	if (server->cursor_surface != NULL)
		server->cursor_surface->cursor_role = 0;
	server->cursor_surface = NULL;
	server->cursor_hidden = 0;
	server->cursor_shape = 0;
	server->cursor_client = NULL;
	server->dirty = 1;
}

/*
 * Moves input focus to the surface currently on the display.
 */
void
zwl_seat_focus(
	struct zwl_server *server)
{
	struct zwl_object *target;
	struct zwl_object *previous;

	/* A dying surface or a failed client cannot take focus. */
	target = server->front_surface;
	if (target != NULL) {
		/* Only a live surface of a healthy client is a focus candidate. */
		if (target->dead || target->client->fatal)
			target = NULL;
	}

	/* A popup holding the grab has the keyboard of its client's window (popup.c). */
	target = zwl_popup_focus(server, target);

	/* A new focus: the old surface's keyboards hear leave before the new one's hear enter; the arrow comes back. */
	if (target != server->focus) {
		previous = server->focus;
		if (server->focus != NULL)
			send_leave(server->focus);
		zwl_cursor_default(server);

		/*
		 * The focus names the surface whose client's keyboard objects have
		 * all been told enter; it is cleared before that surface is freed.
		 */
		server->focus = target;
		if (target != NULL) {
			zwl_data_focus(server, target);
			zwl_primary_focus(server, target);
			send_enter(target);
		}

		/* The text inputs and the input method follow the focus (input-method.c). */
		zwl_ime_focus(server, previous);
	}

	/* Succeeded: the pointer follows (the focused window, or the surface of it under the pointer). */
	zwl_seat_pointer_update(server);
}

/*
 * Moves the pointer to the surface that should hear it now: while a
 * popup's grab has the pointer, the surface of the grab's chain under it
 * (none outside the chain); otherwise the focused window.  Either way the
 * topmost sub-surface of that surface under the pointer takes it instead
 * (subsurface.c).  The surface it leaves hears leave, the new one enter.
 */
void
zwl_seat_pointer_update(
	struct zwl_server *server)
{
	struct zwl_object *base;
	struct zwl_object *target;
	struct zwl_object *deeper;

	/* The surface the pointer belongs to: the grab's chain, or the focus. */
	if (server->pointer_grabbed) {
		base = zwl_popup_chain_at(server);
	} else {
		base = server->focus;
	}

	/* Its sub-surface under the pointer, when there is one. */
	target = base;
	if (base != NULL) {
		deeper = zwl_subsurface_at(base, server->pointer_x, server->pointer_y);
		if (deeper != NULL)
			target = deeper;
	}

	/* An unchanged surface hears nothing. */
	if (target == server->pointer_surface)
		return;

	/* Succeeded: the last surface hears leave, the new one enter. */
	zwl_seat_pointer_move(server, server->pointer_surface, target);
	server->pointer_surface = target;
}

/*
 * Withdraws focus from a surface whose protocol identity is about to retire.
 */
void
zwl_seat_surface_gone(
	struct zwl_object *surface)
{
	struct zwl_server *server;
	struct zwl_object *window;

	/* Borrowed press and interactive origins cannot outlive this surface. */
	server = surface->client->server;
	if (server->press_surface == surface) {
		server->press_surface = NULL;
		server->press_button = 0U;
	}

	/* The operation is canceled when either its window or its press origin goes. */
	if (server->interactive_surface == surface || server->interactive_window == surface) {
		window = server->interactive_window;
		if (server->drag == window)
			server->drag = NULL;

		/* An origin subsurface may retire while its window still survives. */
		if (window != NULL && server->resize == window) {
			server->resize = NULL;
			window->resize_edges = 0U;
		}

		/* Neither a later release nor a later motion reuses the retired origin. */
		interactive_clear(server);
	}

	/* The surface the pointer is on: its pointers hear leave while the surface is still named. */
	if (server->pointer_surface == surface) {
		zwl_seat_pointer_move(server, surface, NULL);
		server->pointer_surface = NULL;
	}

	/* Only the focused surface has a keyboard to withdraw. */
	if (server->focus != surface)
		return;

	/* Leave still names the surface, because delete_id has not been queued yet. */
	send_leave(surface);
	server->focus = NULL;

	/* Succeeded: no seat state refers to the retiring surface. */
	return;
}

/*
 * Moves the pointer (only the pointer, not the keyboard) from one surface
 * to another: the pointers of the first's client hear leave, those of the
 * second's hear enter at the pointer's place (for the popups, popup.c).
 */
void
zwl_seat_pointer_move(
	struct zwl_server *server,
	struct zwl_object *from,
	struct zwl_object *to)
{
	struct zwl_object *object;
	uint32_t words[2];
	uint32_t serial;

	/* The surface the pointer leaves, when it was over one. */
	if (from != NULL && !from->dead) {
		words[0] = zwl_next_serial(server);
		words[1] = from->id;
		for (object = from->client->objects; object != NULL; object = object->next) {
			/* Only live pointers. */
			if (object->kind != ZWL_POINTER || object->dead)
				continue;

			/* Leave, and from version 5 a frame closing it. */
			deliver(object->client, object->id, POINTER_LEAVE, words, sizeof(words));
			if (object->version >= POINTER_FRAME_VERSION)
				deliver(object->client, object->id, POINTER_FRAME, NULL, 0);
		}
	}

	/* The surface the pointer enters, when there is one. */
	if (to == NULL || to->dead)
		return;
	serial = zwl_next_serial(server);
	for (object = to->client->objects; object != NULL; object = object->next) {
		/* Only live pointers. */
		if (object->kind != ZWL_POINTER || object->dead)
			continue;

		/* Enter at the pointer's place on the surface. */
		pointer_enter(object, to, serial);
	}
}

/*
 * Tells every bound seat that the set of open devices changed.
 */
void
zwl_seat_capabilities(
	struct zwl_server *server)
{
	struct zwl_client *client;
	struct zwl_object *object;
	uint32_t word;

	/* Every client with a seat binding learns the new capability bits. */
	word = server->capabilities;
	for (client = server->clients; client != NULL; client = client->next) {
		/* Each live seat object of this client gets the same event. */
		for (object = client->objects; object != NULL; object = object->next) {
			/* Only seat objects carry capabilities. */
			if (object->kind != ZWL_SEAT || object->dead)
				continue;

			/* Queue the new bits for this binding. */
			deliver(client, object->id, SEAT_CAPABILITIES, &word, sizeof(word));
		}
	}

	/* Succeeded: every seat binding has been told. */
	return;
}

/*
 * Reports the current pointer position to the focused client.
 */
void
zwl_seat_motion(
	struct zwl_server *server,
	uint32_t time)
{
	int taken;

	/* zdesktop's own grabs and screens take the motion first. */
	taken = zwl_seat_motion_shell(server, time);
	if (taken)
		return;

	/* Succeeded: otherwise the surface under the pointer hears it. */
	zwl_seat_motion_deliver(server, time);
}

/*
 * Gives the pointer's motion to zdesktop's own grabs and screens (the lock
 * screen, a drag and drop, a resize, the glass look's moves and screens);
 * reports whether one of them took it.  A pen moving as the pointer passes
 * here first too (tablet.c).
 */
int
zwl_seat_motion_shell(
	struct zwl_server *server,
	uint32_t time)
{
	int fullscreen;
	int taken;

	/* The event's time, for whatever measures the pointer's speed (corner.c). */
	server->input_time = time;

	/*
	 * A window frame's resize arrow shows only where the glass look's
	 * windows take the pointer (shell.c): not before window mode, under a
	 * popup's grab, on the lock screen, or during a drag and drop.
	 */
	if (!server->glass ||
	    !server->windowed ||
	    server->popup_grab != NULL ||
	    server->locked ||
	    server->dnd_active)
		zwl_cursor_frame(server, 0U);

	/* The lock screen has the pointer: only its buttons light up (ws035-p102). */
	server->lock_input_ms = zwl_milliseconds();
	if (server->locked) {
		server->dirty = 1;
		return 1;
	}

	/* A drag and drop has the pointer (data.c). */
	if (server->dnd_active) {
		zwl_data_drag_motion(server, time);
		return 1;
	}

	/* A window being resized follows the pointer (toplevel.c). */
	taken = zwl_toplevel_motion(server);
	if (taken)
		return 1;

	/* The glass look takes the motion it follows (not while a popup holds the grab). */
	if (server->glass && server->windowed && server->popup_grab == NULL) {
		fullscreen = zwl_glass_fullscreen_input(server);
		if (fullscreen) {
			/* Over a fullscreen window only the edges' gestures are zdesktop's, and no frame shows its arrow (shell.c). */
			zwl_cursor_frame(server, 0U);
			taken = zwl_glass_edge_motion(server);
		} else {
			/* Elsewhere its windows' moves, screens, gestures and menus. */
			taken = zwl_glass_motion(server);
		}

		/* A shell operation consumed this event before ordinary client delivery. */
		if (taken)
			return 1;
	}

	/* Succeeded: nothing of zdesktop's took the motion. */
	return 0;
}

/*
 * Tells the surface under the pointer where the pointer is now.
 */
void
zwl_seat_motion_deliver(
	struct zwl_server *server,
	uint32_t time)
{
	struct zwl_object *object;
	struct zwl_object *target;
	uint32_t words[3];

	/* The surface under the pointer hears it (enter and leave as it changes); nobody without one. */
	zwl_seat_pointer_update(server);
	target = server->pointer_surface;
	if (target == NULL)
		return;

	/* Motion carries a timestamp and the surface-local position in 24.8 fixed point. */
	words[0] = time;
	words[1] = (uint32_t)((server->pointer_x - target->x) * 256);
	words[2] = (uint32_t)((server->pointer_y - target->y) * 256);
	for (object = target->client->objects; object != NULL; object = object->next) {
		/* Only live pointer objects receive motion. */
		if (object->kind != ZWL_POINTER || object->dead)
			continue;

		/* Queue the motion for this pointer. */
		deliver(object->client, object->id, POINTER_MOTION, words, sizeof(words));
	}

	/* Succeeded: the focused client knows where the pointer is. */
	return;
}

/*
 * Reports one pointer button press or release to the focused client.
 */
void
zwl_seat_button(
	struct zwl_server *server,
	uint32_t time,
	uint32_t button,
	uint32_t state)
{
	int taken;

	/* zdesktop's own grabs, screens and title bars take the button first. */
	taken = zwl_seat_button_shell(server, time, button, state);
	if (taken)
		return;

	/* Succeeded: otherwise the surface under the pointer hears it. */
	zwl_seat_button_deliver(server, time, button, state);
}

/*
 * Records a pointer button and gives it to zdesktop's own grabs, screens
 * and title bars; reports whether one of them took it.  A pen's touch
 * passes here first as BTN_LEFT (tablet.c).
 */
int
zwl_seat_button_shell(
	struct zwl_server *server,
	uint32_t time,
	uint32_t button,
	uint32_t state)
{
	struct zwl_object *origin;
	struct zwl_object *window;
	uint32_t bit;
	int fullscreen;
	int taken;

	/* The event's time, for whatever measures the pointer's speed (corner.c). */
	server->input_time = time;

	/* The buttons held now, which a move or a resize a client asks for needs (toplevel.c). */
	bit = 0;
	if (button >= ZWL_BUTTON_LEFT && button - ZWL_BUTTON_LEFT < 32U)
		bit = 1U << (button - ZWL_BUTTON_LEFT);
	if (state != 0U) {
		server->buttons_down |= bit;
	} else {
		server->buttons_down &= ~bit;
	}

	/* A matching physical release retires the authorization for future requests. */
	if (state == 0U && button == server->press_button) {
		server->press_surface = NULL;
		server->press_button = 0U;
	}

	/* Canceled operations retain no client release destination. */
	window = server->interactive_window;
	if (window != NULL &&
	    server->drag != window &&
	    server->resize != window)
		interactive_clear(server);

	/* The lock screen takes every button (ws035-p102). */
	server->lock_input_ms = zwl_milliseconds();
	if (server->locked) {
		/* The lock owns this input; retire any client operation destination. */
		interactive_clear(server);
		(void)zwl_greeter_button(server, button, state);
		return 1;
	}

	/* A drag and drop takes the buttons; the release of the last one ends it (data.c). */
	if (server->dnd_active) {
		/* Drag-and-drop ownership supersedes a previous window operation. */
		interactive_clear(server);
		if (state == 0U && server->buttons_down == 0U)
			zwl_data_drag_release(server);
		return 1;
	}

	/* Only the initiating button can end a client-requested window operation. */
	window = server->interactive_window;
	if (window != NULL) {
		if (state != 0U || button != server->interactive_button)
			return 1;

		/* Keep the live origin locally and retire ownership before any emission. */
		origin = server->interactive_surface;
		interactive_clear(server);

		/* The original resize or move finishes through its existing shell contract. */
		if (server->resize == window) {
			taken = zwl_toplevel_button(server, state);
		} else {
			zwl_glass_toplevel_move_end(server, window);
		}

		/* The matching release reaches the original client even outside its window. */
		(void)pointer_button(origin, time, button, state, 1U);

		/* Consumed: ordinary hit-based delivery must not duplicate the release. */
		return 1;
	}

	/* A window being resized takes the buttons until the release ends the resize (toplevel.c). */
	taken = zwl_toplevel_button(server, state);
	if (taken)
		return 1;

	/* While a popup holds the grab, a press outside its chain closes the popups (popup.c). */
	taken = zwl_popup_button(server, button, state);
	if (taken)
		return 1;

	/* The glass look takes the buttons it acts on (not while a popup holds the grab). */
	if (server->glass && server->windowed && server->popup_grab == NULL) {
		fullscreen = zwl_glass_fullscreen_input(server);
		if (fullscreen) {
			/* Over a fullscreen window only the edges' gestures take their buttons (shell.c). */
			taken = zwl_glass_edge_button(server, button, state);
		} else {
			/* Elsewhere the title bars, the frames and the desktop. */
			taken = zwl_glass_button(server, button, state);
		}

		/* A shell operation consumed this event before ordinary client delivery. */
		if (taken)
			return 1;
	}

	/* Succeeded: nothing of zdesktop's took the button. */
	return 0;
}

/*
 * Tells the surface under the pointer about one button press or release.
 */
void
zwl_seat_button_deliver(
	struct zwl_server *server,
	uint32_t time,
	uint32_t button,
	uint32_t state)
{
	struct zwl_object *target;
	uint32_t serial;

	/* The ordinary event goes to the current pointer surface. */
	zwl_seat_pointer_update(server);
	target = server->pointer_surface;
	if (target == NULL)
		return;

	/* Reuses the same event payload and serial ownership as interactive release. */
	serial = pointer_button(target, time, button, state, 0U);
	if (serial == 0U)
		return;

	/* A delivered press authorizes requests from this live client's actual button, which start where it was pressed. */
	if (state != 0U) {
		server->press_serial = serial;
		server->press_surface = target;
		server->press_button = button;
		server->press_x = server->pointer_x;
		server->press_y = server->pointer_y;
		zwl_ping_send(target->client);
	}

	/* Succeeded: the focused client has heard the button. */
	return;
}

/*
 * Reports wheel scrolling to the focused client.
 *
 * The counts are in Wayland direction: positive vertical scrolls down and
 * positive horizontal scrolls right.  Each notch is WHEEL_STEP units.
 */
void
zwl_seat_axis(
	struct zwl_server *server,
	uint32_t time,
	int32_t vertical,
	int32_t horizontal)
{
	struct zwl_object *object;
	struct zwl_object *target;
	uint32_t words[3];
	uint32_t word;
	int taken;

	/* The wheel does nothing during a drag and drop, nor on the lock screen. */
	server->lock_input_ms = zwl_milliseconds();
	if (server->dnd_active || server->locked)
		return;

	/* App Home, while it shows, turns its pages with the wheel. */
	taken = zwl_home_axis(server, vertical, horizontal);
	if (taken)
		return;

	/* The volume's icon (and its open popup) takes the wheel (volume.c, ws100-p004). */
	taken = zwl_volume_axis(server, vertical, horizontal);
	if (taken)
		return;

	/* A tab strip under the pointer scrolls with it (titlebar-shell.c). */
	if (server->glass) {
		taken = zwl_titlebar_axis(server, vertical, horizontal);
		if (taken)
			return;
	}

	/* Scrolling goes to the surface the pointer is on; without one it reaches nobody. */
	target = server->pointer_surface;
	if (target == NULL)
		return;

	/* Each pointer hears the source first, then per-axis discrete steps and values. */
	for (object = target->client->objects; object != NULL; object = object->next) {
		/* Only live pointer objects receive scrolling. */
		if (object->kind != ZWL_POINTER || object->dead)
			continue;

		/* Version 5 pointers learn that a wheel produced this frame's scrolling. */
		if (object->version >= POINTER_FRAME_VERSION) {
			word = AXIS_SOURCE_WHEEL;
			deliver(object->client, object->id, POINTER_AXIS_SOURCE, &word, sizeof(word));
		}

		/* Vertical notches become a discrete count and a value. */
		if (vertical != 0) {
			/* The discrete count must precede its axis value in the same frame. */
			if (object->version >= POINTER_FRAME_VERSION) {
				words[0] = AXIS_VERTICAL;
				words[1] = (uint32_t)vertical;
				deliver(object->client, object->id, POINTER_AXIS_DISCRETE, words, 2U * sizeof(uint32_t));
			}

			/* The value is the scroll distance in surface units as 24.8 fixed point. */
			words[0] = time;
			words[1] = AXIS_VERTICAL;
			words[2] = (uint32_t)(vertical * WHEEL_STEP * 256);
			deliver(object->client, object->id, POINTER_AXIS, words, sizeof(words));
		}

		/* Horizontal notches follow the same pattern on the other axis. */
		if (horizontal != 0) {
			/* The discrete count must precede its axis value in the same frame. */
			if (object->version >= POINTER_FRAME_VERSION) {
				words[0] = AXIS_HORIZONTAL;
				words[1] = (uint32_t)horizontal;
				deliver(object->client, object->id, POINTER_AXIS_DISCRETE, words, 2U * sizeof(uint32_t));
			}

			/* The value is the scroll distance in surface units as 24.8 fixed point. */
			words[0] = time;
			words[1] = AXIS_HORIZONTAL;
			words[2] = (uint32_t)(horizontal * WHEEL_STEP * 256);
			deliver(object->client, object->id, POINTER_AXIS, words, sizeof(words));
		}
	}

	/* Succeeded: the focused client has heard the scrolling. */
	return;
}

/*
 * Ends one group of pointer events for version 5 pointers of the focused client.
 */
void
zwl_seat_frame(
	struct zwl_server *server)
{
	struct zwl_object *object;
	struct zwl_object *target;

	/* A frame goes to the surface the pointer is on; without one it reaches nobody. */
	target = server->pointer_surface;
	if (target == NULL)
		return;

	/* Pointers older than version 5 have no frame event. */
	for (object = target->client->objects; object != NULL; object = object->next) {
		/* Only live version 5 pointers are told where a group ends. */
		if (object->kind != ZWL_POINTER ||
		    object->dead ||
		    object->version < POINTER_FRAME_VERSION)
			continue;

		/* Queue the frame for this pointer. */
		deliver(object->client, object->id, POINTER_FRAME, NULL, 0);
	}

	/* Succeeded: the group is closed. */
	return;
}

/*
 * Reports one key press or release to the focused client.
 */
void
zwl_seat_key(
	struct zwl_server *server,
	uint32_t time,
	uint32_t key,
	uint32_t state)
{
	int taken;

	/*
	 * App Home, while it shows, takes every key (home.c), and so does an
	 * open menu (menu-shell.c), then a titlebar's text field with the
	 * keyboard (titlebar-shell.c); zdesktop's shortcuts come next
	 * (shell.c), then the focused window's menu: F10 and its shortcuts,
	 * then the keys of its tabs (titlebar-shell.c).  Esc gives up a drag
	 * and drop first (data.c).  The input method (input-method.c) takes
	 * Alt+Space before all of these, and the keys of a text input it
	 * serves after zdesktop's shortcuts (before the menu's while text is
	 * being composed).
	 */
	if (server->dnd_active && key == SEAT_KEY_ESC && state != 0U) {
		zwl_data_drag_cancel(server);
		return;
	}

	/* The lock screen takes every key; Super+L locks a session (ws035-p102). */
	server->lock_input_ms = zwl_milliseconds();
	if (server->locked) {
		(void)zwl_greeter_key(server, key, state);
		return;
	}

	/* Super+L locks it. */
	if (key == SEAT_KEY_L && state != 0U && (server->modifiers & SEAT_MODIFIER_SUPER) != 0U) {
		taken = zwl_lock(server, "key");
		if (taken)
			return;
	}

	/* The login screen takes every key; it has no clients to give one to (greeter.c). */
	if (server->greeter) {
		(void)zwl_greeter_key(server, key, state);
		return;
	}

	/* The input method's keys come first: Alt+Space, and a release whose press went to it (input-method.c). */
	taken = zwl_ime_key_early(server, time, key, state);
	if (taken)
		return;

	/* The glass look's own keys, in that order. */
	if (server->glass) {
		taken = zwl_home_key(server, key, state);
		if (taken)
			return;
		taken = zwl_network_key(server, key, state);
		if (taken)
			return;
		taken = zwl_menu_grab_key(server, key, state);
		if (taken)
			return;
		taken = zwl_titlebar_key(server, key, state);
		if (taken)
			return;
		taken = zwl_glass_key(server, key, state);
		if (taken)
			return;
		taken = zwl_ime_key_grab(server, time, key, state, 1);
		if (taken)
			return;
		taken = zwl_menu_key(server, key, state);
		if (taken)
			return;
		taken = zwl_titlebar_tab_key(server, key, state);
		if (taken)
			return;
	}

	/* The input method takes the key while it serves a text input (input-method.c). */
	taken = zwl_ime_key_grab(server, time, key, state, 0);
	if (taken)
		return;

	/* The focused client hears the key. */
	zwl_seat_key_deliver(server, time, key, state);
}

/*
 * Sends one key press or release to the focused client's keyboards.
 */
void
zwl_seat_key_deliver(
	struct zwl_server *server,
	uint32_t time,
	uint32_t key,
	uint32_t state)
{
	struct zwl_object *object;
	uint32_t words[4];

	/* A key without focus reaches nobody. */
	if (server->focus == NULL)
		return;

	/* The key event carries a new serial, the time, the evdev key code and the state. */
	words[0] = zwl_next_serial(server);
	words[1] = time;
	words[2] = key;
	words[3] = state;
	for (object = server->focus->client->objects; object != NULL; object = object->next) {
		/* Only live keyboard objects receive keys. */
		if (object->kind != ZWL_KEYBOARD || object->dead)
			continue;

		/* Queue the key for this keyboard. */
		deliver(object->client, object->id, KEYBOARD_KEY, words, sizeof(words));
	}

	/* Succeeded: the focused client has heard the key. */
	return;
}

/*
 * Reports the depressed modifier mask in server->modifiers to the focused client.
 */
void
zwl_seat_modifiers(
	struct zwl_server *server)
{
	struct zwl_object *object;
	uint32_t serial;

	/* The input method's keyboard grab hears the modifiers too (input-method.c). */
	zwl_ime_modifiers(server);

	/* A modifier change without focus reaches nobody; enter will carry it later. */
	if (server->focus == NULL)
		return;

	/* One serial covers the modifier report on every keyboard of the client. */
	serial = zwl_next_serial(server);
	for (object = server->focus->client->objects; object != NULL; object = object->next) {
		/* Only live keyboard objects receive modifiers. */
		if (object->kind != ZWL_KEYBOARD || object->dead)
			continue;

		/* Queue the new modifier state for this keyboard. */
		keyboard_modifiers(object, serial);
	}

	/* Succeeded: the focused client knows the modifier state. */
	return;
}

/* Creates a pointer, keyboard or touch object and introduces it to the current focus. */
static int
create_device(
	struct zwl_object *seat,
	enum zwl_kind kind,
	const unsigned char *bytes,
	size_t size)
{
	struct zwl_object *device;
	struct zwl_server *server;
	int32_t repeat[2];
	uint32_t id;
	uint32_t serial;
	int error;

	/* The constructor carries exactly one new_id. */
	if (size != 4U)
		return EPROTO;

	/* The child inherits the seat's negotiated version. */
	memcpy(&id, bytes, sizeof(id));
	device = zwl_create(seat->client, id, kind, seat->version);
	if (device == NULL)
		return EPROTO;

	/* A keyboard first learns that no keymap applies: keys are evdev codes. */
	if (kind == ZWL_KEYBOARD) {
		/* The keymap event must carry a descriptor even when there is no keymap. */
		error = keyboard_keymap(device);
		if (error != 0)
			return error;

		/*
		 * Version 4 keyboards are told how to repeat a held key themselves
		 * (zdesktop does not): 25 keys a second after 400 ms (ws035-p078),
		 * or the user's preferences (ws089-p007, main.c and preferences.c).
		 */
		if (device->version >= KEYBOARD_REPEAT_VERSION) {
			repeat[0] = seat->client->server->repeat_rate;
			repeat[1] = seat->client->server->repeat_delay_ms;
			deliver(device->client, device->id, KEYBOARD_REPEAT_INFO, repeat, sizeof(repeat));
		}
	}

	/* A pointer created while the pointer is on one of its client's surfaces is entered there at once. */
	server = seat->client->server;
	if (kind == ZWL_POINTER &&
	    server->pointer_surface != NULL &&
	    server->pointer_surface->client == seat->client) {
		serial = zwl_next_serial(server);
		pointer_enter(device, server->pointer_surface, serial);
	}

	/* A keyboard created while its client has the focus is entered at once (the others already were). */
	if (kind == ZWL_KEYBOARD &&
	    server->focus != NULL &&
	    server->focus->client == seat->client) {
		serial = zwl_next_serial(server);
		keyboard_enter(device, server->focus, serial);
	}

	/* The test log records which input objects each client holds. */
	report_seat(seat->client);

	/* Succeeded: the client owns the new input object. */
	return 0;
}

/* Queues one event and marks the client failed when the queue refuses it. */
static void
deliver(
	struct zwl_client *client,
	uint32_t id,
	uint32_t opcode,
	const void *payload,
	size_t size)
{
	struct zwl_server *server;
	int error;

	/* A failed client is only waiting for cleanup and hears nothing further. */
	if (client->fatal)
		return;

	/* A client that stopped reading loses its connection rather than compositor memory. */
	error = zwl_emit(client, id, opcode, payload, size);
	if (error != 0) {
		client->fatal = 1;
		client->fatal_time = zwl_milliseconds();
		return;
	}

	/* The exit summary counts every queued seat event. */
	server = client->server;
	server->seat_events++;

	/* Succeeded: the event is queued. */
	return;
}

/* Tells one pointer that it is over the focused surface, and where. */
static void
pointer_enter(
	struct zwl_object *pointer,
	struct zwl_object *surface,
	uint32_t serial)
{
	struct zwl_server *server;
	uint32_t words[4];

	/* Enter carries the serial, the surface and the surface-local position (the window's place taken off). */
	server = pointer->client->server;
	words[0] = serial;
	words[1] = surface->id;
	words[2] = (uint32_t)((server->pointer_x - surface->x) * 256);
	words[3] = (uint32_t)((server->pointer_y - surface->y) * 256);
	deliver(pointer->client, pointer->id, POINTER_ENTER, words, sizeof(words));

	/* Version 5 pointers see enter as a group of its own. */
	if (pointer->version >= POINTER_FRAME_VERSION)
		deliver(pointer->client, pointer->id, POINTER_FRAME, NULL, 0);

	/* Succeeded: the pointer is over the surface. */
	return;
}

/* Tells one keyboard that the focused surface has keyboard focus. */
static void
keyboard_enter(
	struct zwl_object *keyboard,
	struct zwl_object *surface,
	uint32_t serial)
{
	uint32_t words[3];

	/* Enter carries the serial, the surface and an empty array of held keys. */
	words[0] = serial;
	words[1] = surface->id;
	words[2] = 0;
	deliver(keyboard->client, keyboard->id, KEYBOARD_ENTER, words, sizeof(words));

	/* The modifier state that applies from now on follows enter. */
	serial = zwl_next_serial(keyboard->client->server);
	keyboard_modifiers(keyboard, serial);

	/* Succeeded: the keyboard is focused on the surface. */
	return;
}

/* Sends one keyboard the current depressed modifier mask. */
static void
keyboard_modifiers(
	struct zwl_object *keyboard,
	uint32_t serial)
{
	uint32_t words[5];

	/* The held modifiers and the locks (Caps Lock, Num Lock); nothing is latched and the group is 0. */
	words[0] = serial;
	words[1] = keyboard->client->server->modifiers;
	words[2] = 0;
	words[3] = keyboard->client->server->locked_modifiers;
	words[4] = 0;
	deliver(keyboard->client, keyboard->id, KEYBOARD_MODIFIERS, words, sizeof(words));

	/* Succeeded: the keyboard knows the modifier state. */
	return;
}

/* Tells every keyboard of a surface's client that the focus left it (the pointer is zwl_seat_pointer_update's). */
static void
send_leave(
	struct zwl_object *surface)
{
	struct zwl_object *object;
	uint32_t words[2];

	/* Leave carries one serial and the surface. */
	words[0] = zwl_next_serial(surface->client->server);
	words[1] = surface->id;
	for (object = surface->client->objects; object != NULL; object = object->next) {
		/* Only live keyboards. */
		if (object->dead || object->kind != ZWL_KEYBOARD)
			continue;

		/* The keyboard hears leave. */
		deliver(object->client, object->id, KEYBOARD_LEAVE, words, sizeof(words));
	}
}

/* Tells every keyboard of a surface's client that the focus arrived (the pointer is zwl_seat_pointer_update's). */
static void
send_enter(
	struct zwl_object *surface)
{
	struct zwl_object *object;
	uint32_t serial;

	/* One serial covers the enter on every keyboard of the client. */
	serial = zwl_next_serial(surface->client->server);
	for (object = surface->client->objects; object != NULL; object = object->next) {
		/* Only live keyboards. */
		if (object->dead || object->kind != ZWL_KEYBOARD)
			continue;

		/* The keyboard hears enter, with the keys and modifiers held. */
		keyboard_enter(object, surface, serial);
	}
}

/* Prints which input objects one client currently holds. */
static void
report_seat(
	struct zwl_client *client)
{
	struct zwl_object *object;
	unsigned pointer;
	unsigned keyboard;
	unsigned touch;

	/* The client's live objects say whether it holds a pointer, a keyboard and a touch. */
	pointer = 0;
	keyboard = 0;
	touch = 0;
	for (object = client->objects; object != NULL; object = object->next) {
		/* Retired objects no longer count. */
		if (object->dead)
			continue;

		/* Record the three device kinds the log line reports. */
		if (object->kind == ZWL_POINTER)
			pointer = 1;
		if (object->kind == ZWL_KEYBOARD)
			keyboard = 1;
		if (object->kind == ZWL_TOUCH)
			touch = 1;
	}

	/* One line per constructor keeps the test log short. */
	printf("ZWL SEAT client=%llu pointer=%u keyboard=%u touch=%u\n", (unsigned long long)client->number, pointer, keyboard, touch);

	/* Succeeded: the line is printed. */
	return;
}

/* Sends a keyboard the no_keymap format with a descriptor of an empty file. */
static int
keyboard_keymap(
	struct zwl_object *keyboard)
{
	uint32_t words[2];
	uint32_t size;
	int descriptor;
	int error;

	/* The US XKB keymap (keymap.c, ws035-p078): a read-only descriptor of its file and its size. */
	words[0] = KEYMAP_XKB_V1;
	descriptor = zwl_keymap_descriptor(&size);
	words[1] = size;

	/* Without it the protocol still requires a descriptor: /dev/null is an empty file, and no keymap applies. */
	if (descriptor < 0) {
		descriptor = open("/dev/null", O_RDONLY | O_CLOEXEC);
		if (descriptor < 0)
			return errno;
		words[0] = KEYMAP_NO_KEYMAP;
		words[1] = 0;
	}

	/* The payload words are the format and the size; the descriptor travels beside them. */
	error = zwl_emit_fd(keyboard->client, keyboard->id, KEYBOARD_KEYMAP, words, sizeof(words), descriptor);
	if (error != 0)
		return error;

	/* The exit summary counts every queued seat event. */
	keyboard->client->server->seat_events++;

	/* Succeeded: the keyboard knows keys arrive as evdev codes. */
	return 0;
}

/*
 * Makes a surface the cursor, with its hotspot, or hides the cursor when
 * there is none.
 */
static void
set_cursor(
	struct zwl_server *server,
	struct zwl_client *client,
	struct zwl_object *surface,
	const unsigned char *bytes)
{
	int32_t hotspot[2];

	/* The previous cursor surface is an ordinary surface again. */
	if (server->cursor_surface != NULL && server->cursor_surface != surface)
		server->cursor_surface->cursor_role = 0;

	/* A cursor surface (or none) replaces a shape the client asked for; no surface hides the cursor. */
	server->dirty = 1;
	server->cursor_shape = 0;
	server->cursor_client = client;
	if (surface == NULL) {
		server->cursor_surface = NULL;
		server->cursor_hidden = 1;
		return;
	}

	/* The surface and the point of it that is the pointer's position. */
	memcpy(hotspot, bytes + 8, sizeof(hotspot));
	surface->cursor_role = 1;
	server->cursor_surface = surface;
	server->cursor_hotspot_x = hotspot[0];
	server->cursor_hotspot_y = hotspot[1];
	server->cursor_hidden = 0;
}

/* Sends one button directly to a live origin and returns its nonzero serial. */
static uint32_t
pointer_button(
    struct zwl_object *target,
    uint32_t time,
    uint32_t button,
    uint32_t state,
    unsigned complete)
{
	struct zwl_object *object;
	struct zwl_client *client;
	struct zwl_server *server;
	uint32_t words[4];

	/* Teardown or a failed connection has no live pointer destination. */
	if (target == NULL || target->dead)
		return 0U;

	/* The origin's client owns its pointer objects throughout this dispatch. */
	client = target->client;
	if (client->fatal)
		return 0U;

	/* The existing pointer protocol carries serial, time, physical button and state. */
	server = client->server;
	words[0] = zwl_next_serial(server);
	words[1] = time;
	words[2] = button;
	words[3] = state;
	for (object = client->objects; object != NULL; object = object->next) {
		/* Dead objects and interfaces other than wl_pointer receive no event. */
		if (object->kind != ZWL_POINTER || object->dead)
			continue;

		/* Reuses the established buffered delivery and failure contract. */
		deliver(client, object->id, POINTER_BUTTON, words, sizeof(words));

		/* A direct release closes its origin's group even if ordinary focus moved. */
		if (complete && object->version >= POINTER_FRAME_VERSION)
			deliver(client, object->id, POINTER_FRAME, NULL, 0);
	}

	/* Succeeded: this event owns exactly one serial. */
	return words[0];
}

/* Retires borrowed client-operation identities before any later input delivery. */
static void
interactive_clear(
    struct zwl_server *server)
{
	/* The compositor owns these links, never the objects they borrow. */
	server->interactive_window = NULL;
	server->interactive_surface = NULL;
	server->interactive_button = 0U;

	/* No later release can reuse the retired destination. */
	return;
}
