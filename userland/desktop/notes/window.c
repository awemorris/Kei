/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Wayland window of Notes: libkeiui's window (ws090-p011), whose
 * surface Notes draws on with its own Vulkan (KUI_PRESENT_NONE), and the
 * queue of input events the main loop draws from.
 *
 * The pointer's left button draws: its press, the motions while it is
 * held, and its release become NOTES_INPUT_DOWN, _MOTION and _UP events
 * from NOTES_SOURCE_POINTER with a fixed pressure, at the compositor's
 * time.  Every motion is kept, not only the last of a frame, because each
 * one is a sample of the stroke.  A pen tablet's tools (tablet.c, the
 * tablet protocol, bound from a registry of Notes' own on the window's
 * seat) feed the same queue through notes_window_input() with the pen's
 * own pressure and tilt; a pen without the tablet protocol arrives as the
 * pointer.  Keys are queued as pressed (Notes does not repeat them).
 * ws081-p013: the touch screen's events queue for touch.c.
 */

#include "app.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* The evdev code of the left button. */
#define WINDOW_BUTTON_LEFT	0x110U

static void window_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void window_global_remove(void *data, struct wl_registry *registry, uint32_t name);
static void window_take(struct notes_window *window);
static void window_event(struct notes_window *window, const struct kui_window_event *event);
static void window_button(struct notes_window *window, const struct kui_window_event *event);
static void window_key(struct notes_window *window, const struct kui_window_event *event);
static void window_pointer_event(struct notes_window *window, unsigned kind, const struct kui_window_event *event);
static void window_touch_push(struct notes_window *window, unsigned type, const struct kui_window_event *event);
static uint32_t window_modifiers(unsigned modifiers);

/* The titlebar's events: Notes' titlebar shows only its menus (menu.c), so it hears none. */
static const struct keiland_titlebar_listener titlebar_listener = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

/* Notes' registry: the tablet manager. */
static const struct wl_registry_listener registry_listener = {
	window_global, window_global_remove
};

/*
 * Connects to the compositor and makes a toplevel window of a size,
 * mapped fullscreen when asked.  Returns 0 once the first configure is
 * acknowledged, or -1 with errno set.
 */
int
notes_window_open(
	struct notes_window *window,
	uint32_t width,
	uint32_t height,
	int fullscreen)
{
	struct kui_window_options options;
	int status;

	/* Nothing held yet. */
	memset(window, 0, sizeof(*window));

	/* libkeiui's window: the title, the application's identity (the gesture finds Notes by it), the size and the full screen asked for. */
	memset(&options, 0, sizeof(options));
	options.title = "Notes";
	options.application = "notes";
	options.width = width;
	options.height = height;
	options.present = KUI_PRESENT_NONE;
	options.fullscreen = fullscreen;
	window->kui = kui_window_open(&options);
	if (window->kui == NULL)
		return -1;

	/* The window's objects Notes' parts use, and the size and the full screen it was given. */
	window->display = kui_window_display(window->kui);
	window->seat = kui_window_seat(window->kui);
	window->surface = kui_window_surface(window->kui);
	window->toplevel = kui_window_toplevel(window->kui);
	kui_window_size(window->kui, &window->width, &window->height);
	window->fullscreen = kui_window_fullscreen(window->kui);

	/*
	 * zdesktop's titlebar, asked for before anything is drawn: the
	 * roundtrip below acknowledges the configure it brings, so the first
	 * image is shown with it.
	 */
	window->titlebar = keiland_titlebar_create(window->display, window->toplevel, &titlebar_listener, window);
	if (window->titlebar == NULL)
		printf("NOTES TITLEBAR none errno=%d\n", errno);

	/* Notes' registry, for the tablet manager. */
	window->registry = wl_display_get_registry(window->display);
	if (window->registry == NULL)
		return -1;

	/* Listens for the globals the compositor announces. */
	status = wl_registry_add_listener(window->registry, &registry_listener, window);
	if (status != 0)
		return -1;

	/* Waits until every global has been announced. */
	status = wl_display_roundtrip(window->display);
	if (status < 0)
		return -1;

	/* The seat's tablets, when the compositor has the tablet protocol. */
	notes_tablet_start(window);

	/* What the first configure left in the window's queue (its size is already known). */
	window_take(window);
	window->resized = 0;

	/* Succeeded: the window can be drawn into. */
	return 0;
}

/*
 * Waits for the compositor's events, at most a timeout in milliseconds
 * (-1: for ever), runs them and takes the window's input.
 *
 * Returns 0, or -1 when the connection is broken.
 */
int
notes_window_dispatch(
	struct notes_window *window,
	int timeout)
{
	int status;

	/* Nothing to wait for when events are already queued for the main loop. */
	if (window->input_count != 0U ||
	    window->key_count != 0U ||
	    window->action_count != 0U)
		timeout = 0;

	/* The compositor's events. */
	status = kui_window_dispatch(window->kui, timeout);
	if (status != 0)
		return -1;

	/* The input they queued. */
	window_take(window);

	/* Succeeded: the events so far have run. */
	return 0;
}

/*
 * Destroys the window's objects and disconnects.
 */
void
notes_window_close(
	struct notes_window *window)
{
	/* The menus, before the window they are shown on, and the pen before the seat. */
	notes_menu_close(window);
	notes_tablet_close(window);

	/* The titlebar, before the toplevel it is tied to. */
	if (window->titlebar != NULL)
		keiland_titlebar_destroy(window->titlebar);

	/* Notes' registry, then the window and its connection. */
	if (window->registry != NULL)
		wl_registry_destroy(window->registry);
	if (window->kui != NULL)
		kui_window_close(window->kui);
	memset(window, 0, sizeof(*window));
}

/*
 * Queues an input event for the main loop.
 *
 * The pointer's events come through here, and so do a pen tablet's.  A
 * full queue drops a motion but keeps room for the contact's end, so that
 * a stroke is always finished.
 */
void
notes_window_input(
	struct notes_window *window,
	const struct notes_input *input)
{
	/* The last slot is kept for an end of contact. */
	if (window->input_count + 1U >= NOTES_INPUTS && input->kind != NOTES_INPUT_UP)
		return;
	if (window->input_count >= NOTES_INPUTS)
		return;

	/* Succeeded: queued after the ones before it. */
	window->inputs[window->input_count] = *input;
	window->input_count++;
}

/*
 * Sets the title the compositor shows.
 */
void
notes_window_set_title(
	struct notes_window *window,
	const char *title)
{
	/* The toplevel's title. */
	kui_window_set_title(window->kui, title);
}

/*
 * Asks the compositor to make the window fullscreen, or to end it; the
 * configure that follows says what it did.
 */
void
notes_window_set_fullscreen(
	struct notes_window *window,
	int fullscreen)
{
	/* On the default output, or back to a window. */
	kui_window_set_fullscreen(window->kui, fullscreen);
}

/*
 * Returns a monotonic time in milliseconds (0 when the clock cannot be read).
 */
uint64_t
notes_clock(void)
{
	struct timespec now;
	int status;

	/* The monotonic clock. */
	status = clock_gettime(CLOCK_MONOTONIC, &now);
	if (status != 0)
		return 0U;

	/* Reports it in milliseconds. */
	return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
}

/* Binds the tablet manager (the window has the rest). */
static void
window_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
	struct notes_window *window;
	int match;

	/* The tablet manager gives the pen with its pressure and tilt (tablet.c). */
	(void)version;
	window = data;
	match = strcmp(interface, "zwp_tablet_manager_v2");
	if (match == 0)
		notes_tablet_bind(window, registry, name);
}

/* A global going away does not matter to a window that already bound what it needs. */
static void
window_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t name)
{
	/* Nothing to do. */
	(void)data;
	(void)registry;
	(void)name;
}

/* Takes every input the window queued, and its full screen as the compositor left it. */
static void
window_take(
	struct notes_window *window)
{
	struct kui_window_event event;
	int taken;

	/* Each input, oldest first. */
	for (;;) {
		taken = kui_window_take(window->kui, &event);
		if (taken == 0)
			break;
		window_event(window, &event);
	}

	/* Whether the compositor made the window fullscreen. */
	window->fullscreen = kui_window_fullscreen(window->kui);
}

/* Turns one input of the window into Notes'. */
static void
window_event(
	struct notes_window *window,
	const struct kui_window_event *event)
{
	/* Every input carries the modifiers held. */
	window->modifiers = window_modifiers(event->modifiers);

	/* What it is. */
	switch (event->kind) {
	case KUI_WINDOW_MOTION:
		/* The pointer's place; while the button is held, a sample of the contact. */
		window->pointer_x = (float)event->x;
		window->pointer_y = (float)event->y;
		if (window->pointer_down)
			window_pointer_event(window, NOTES_INPUT_MOTION, event);
		break;
	case KUI_WINDOW_BUTTON:
		window_button(window, event);
		break;
	case KUI_WINDOW_KEY:
		window_key(window, event);
		break;
	case KUI_WINDOW_TOUCH_DOWN:
		window_touch_push(window, NOTES_TOUCH_DOWN, event);
		break;
	case KUI_WINDOW_TOUCH_MOTION:
		window_touch_push(window, NOTES_TOUCH_MOTION, event);
		break;
	case KUI_WINDOW_TOUCH_UP:
		window_touch_push(window, NOTES_TOUCH_UP, event);
		break;
	case KUI_WINDOW_TOUCH_CANCEL:
		window_touch_push(window, NOTES_TOUCH_CANCEL, event);
		break;
	case KUI_WINDOW_RESIZE:
		/* The size the compositor gave, drawn at from the next frame. */
		kui_window_size(window->kui, &window->width, &window->height);
		window->resized = 1;
		break;
	case KUI_WINDOW_CLOSE:
		/* The main loop ends Notes. */
		window->closed = 1;
		break;
	default:
		break;
	}
}

/* The left button starts and ends a contact. */
static void
window_button(
	struct notes_window *window,
	const struct kui_window_event *event)
{
	/* Only the left button draws. */
	if (event->code != WINDOW_BUTTON_LEFT)
		return;

	/* A press starts the contact, a release ends it. */
	if (event->pressed) {
		window->pointer_down = 1;
		window_pointer_event(window, NOTES_INPUT_DOWN, event);
	} else if (window->pointer_down) {
		window->pointer_down = 0;
		window_pointer_event(window, NOTES_INPUT_UP, event);
	}
}

/* Queues a pressed key with the modifiers held; releases and a held key's repeats do nothing. */
static void
window_key(
	struct notes_window *window,
	const struct kui_window_event *event)
{
	/* Only first presses, while the queue has room. */
	if (!event->pressed || event->repeated)
		return;
	if (window->key_count >= NOTES_KEYS)
		return;

	/* Succeeded: queued. */
	window->keys[window->key_count].key = event->code;
	window->keys[window->key_count].modifiers = window->modifiers;
	window->key_count++;
}

/* Queues a pointer event at the pointer's place, with the pointer's fixed pressure and no tilt, at the compositor's time. */
static void
window_pointer_event(
	struct notes_window *window,
	unsigned kind,
	const struct kui_window_event *event)
{
	struct notes_input input;

	/* The event. */
	memset(&input, 0, sizeof(input));
	input.kind = kind;
	input.source = NOTES_SOURCE_POINTER;
	input.x = window->pointer_x;
	input.y = window->pointer_y;
	input.pressure = NOTES_POINTER_PRESSURE;
	input.time_ms = (uint32_t)(event->time_us / 1000U);

	/* Queues it like any other source's. */
	notes_window_input(window, &input);
}

/* Queues a touch input for touch.c; a full queue drops it. */
static void
window_touch_push(
	struct notes_window *window,
	unsigned type,
	const struct kui_window_event *event)
{
	struct notes_touch_event *kept;

	/* A full queue drops the input (the fingers are far ahead of the program). */
	if (window->touch_count >= NOTES_TOUCH_EVENTS)
		return;

	/* The input, after the ones before it: its time as the compositor's milliseconds, and when it was read. */
	kept = &window->touches[window->touch_count];
	window->touch_count++;
	memset(kept, 0, sizeof(*kept));
	kept->type = type;
	kept->id = event->id;
	kept->x = (float)event->x;
	kept->y = (float)event->y;
	kept->time = (uint32_t)(event->time_us / 1000U);
	kept->arrival = event->arrival_us;
}

/* Turns libkeiui's modifier bits into wl_keyboard's, which the shortcuts read. */
static uint32_t
window_modifiers(
	unsigned modifiers)
{
	uint32_t bits;

	/* Shift, Control and Alt. */
	bits = 0U;
	if ((modifiers & KUI_MOD_SHIFT) != 0U)
		bits |= NOTES_MODIFIER_SHIFT;
	if ((modifiers & KUI_MOD_CTRL) != 0U)
		bits |= NOTES_MODIFIER_CONTROL;
	if ((modifiers & KUI_MOD_ALT) != 0U)
		bits |= NOTES_MODIFIER_ALT;

	/* Reports them. */
	return bits;
}
