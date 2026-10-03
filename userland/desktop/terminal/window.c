/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Wayland window of terminal: libkeiui's window (ws090-p011), whose
 * surface the terminal draws on with its own Vulkan (KUI_PRESENT_NONE).
 *
 * The window queues its input; this file takes it into what the main loop
 * reads.  A key press is turned into bytes at once (keys.c) and kept until
 * the main loop writes them to the shell, and a held key's repeats (which
 * the window makes after the compositor's events are in, BUG-111) are typed
 * the same way.  The pointer's presses, releases and motions are kept for
 * the selection (ws035-p093), the wheel as notches of the view's scroll
 * (ws035-p114), and the fingers for touch.c (ws081-p011).  The clipboard,
 * drops and drags (clipboard.c) and the primary selection (primary.c) bind
 * their managers from a registry of the terminal's own, on the window's
 * seat.
 */

#include "terminal.h"

#include <errno.h>
#include <string.h>
#include <time.h>

/* The evdev codes of Page Up and Page Down, which with Shift scroll the view (ws035-p114). */
#define WINDOW_KEY_PAGEUP	104U
#define WINDOW_KEY_PAGEDOWN	109U

/* The evdev codes of the pointer's left and middle buttons. */
#define WINDOW_BUTTON_LEFT	0x110U
#define WINDOW_BUTTON_MIDDLE	0x112U

/*
 * How far the window's wheel moves for one notch of zdesktop's (its step of
 * 15 surface units, which libkeiui scales by 4): the terminal scrolls by
 * notches.
 */
#define WINDOW_WHEEL_NOTCH	60.0

static void window_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void window_global_remove(void *data, struct wl_registry *registry, uint32_t name);
static void window_take(struct terminal_window *window);
static void window_event(struct terminal_window *window, const struct kui_window_event *event);
static void window_press(struct terminal_window *window, uint32_t key);
static void window_wheel(struct terminal_window *window, double dy);
static void window_pointer_event(struct terminal_window *window, unsigned kind, const struct kui_window_event *event);
static void window_touch_push(struct terminal_window *window, unsigned type, const struct kui_window_event *event);
static uint32_t window_modifiers(unsigned modifiers);

/* The terminal's registry: the clipboard's and the primary selection's managers. */
static const struct wl_registry_listener registry_listener = {
	window_global, window_global_remove
};

/*
 * Connects to the compositor and makes a toplevel window of a size.
 *
 * Returns 0 once the first configure is acknowledged, or -1 with errno set.
 */
int
terminal_window_open(
	struct terminal_window *window,
	const char *display,
	uint32_t width,
	uint32_t height)
{
	struct kui_window_options options;
	int status;

	/* Nothing held yet. */
	memset(window, 0, sizeof(*window));

	/* libkeiui's window, the size asked for until the compositor gives one; the terminal draws on it itself. */
	memset(&options, 0, sizeof(options));
	options.display = display;
	options.title = "Terminal";
	options.application = "terminal";
	options.width = width;
	options.height = height;
	options.present = KUI_PRESENT_NONE;
	window->kui = kui_window_open(&options);
	if (window->kui == NULL)
		return -1;

	/* The window's objects the terminal's parts use, and the size it was given. */
	window->display = kui_window_display(window->kui);
	window->seat = kui_window_seat(window->kui);
	window->surface = kui_window_surface(window->kui);
	window->toplevel = kui_window_toplevel(window->kui);
	kui_window_size(window->kui, &window->width, &window->height);
	window->fullscreen = kui_window_fullscreen(window->kui);

	/*
	 * zdesktop's titlebar with the tabs (tabs.c), asked for before anything
	 * is drawn: the roundtrip below acknowledges the configure it brings, so
	 * the first image is shown with its titlebar.  Asked for after the
	 * renderer and the menus, the first image was committed before that
	 * configure was acknowledged, and the titlebar came only with a later
	 * image, seconds after the window (ws099-p023, BUG-137).
	 */
	terminal_tabs_open(window);

	/* The terminal's registry, for the clipboard's and the primary selection's managers. */
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

	/* The seat's data device, for the clipboard and drops (clipboard.c), and the primary selection's (primary.c). */
	terminal_clipboard_start(window);
	terminal_primary_start(window);

	/* What the first configure left in the window's queue (its size is already known). */
	window_take(window);
	window->resized = 0;

	/* Succeeded: the window can be drawn into. */
	return 0;
}

/*
 * Waits for the compositor or another descriptor (the shells') and runs
 * the compositor's events, taking the window's input.
 *
 * `timeout` is in milliseconds (-1 waits for ever); ready[i] tells whether
 * others[i] has something to read or has hung up.  Returns 0, or -1 when
 * the connection is broken.
 */
int
terminal_window_dispatch(
	struct terminal_window *window,
	const int *others,
	unsigned count,
	int timeout,
	int *ready)
{
	int status;

	/* The compositor and the shells, together. */
	status = kui_window_dispatch_fds(window->kui, others, count, timeout, ready);
	if (status != 0)
		return -1;

	/* The input the compositor's events queued. */
	window_take(window);

	/* Succeeded: the events so far have run. */
	return 0;
}

/*
 * Presses the held key again when its repeat is due (after the compositor's
 * events, so that a release read with them stops it first, BUG-111).
 */
void
terminal_window_repeat(
	struct terminal_window *window,
	uint64_t now)
{
	/* The window's repeat queues the key; it is typed like any press. */
	(void)now;
	(void)kui_window_repeat(window->kui, kui_clock_us());
	window_take(window);
}

/*
 * Reports in how many milliseconds the held key's repeat is due (0: now,
 * -1 when no key is held).
 */
int
terminal_window_repeat_wait(
	const struct terminal_window *window)
{
	int wait;

	/* The window's answer. */
	wait = kui_window_repeat_wait(window->kui, kui_clock_us());

	/* Reports it. */
	return wait;
}

/*
 * Destroys the window's objects and disconnects.
 */
void
terminal_window_close(
	struct terminal_window *window)
{
	/* The menus, before the window they are shown on, and the clipboard before the seat. */
	terminal_menu_close(window);
	terminal_primary_close(window);
	terminal_clipboard_close(window);

	/* The terminal's registry, then the window and its connection. */
	if (window->registry != NULL)
		wl_registry_destroy(window->registry);
	if (window->kui != NULL)
		kui_window_close(window->kui);
	memset(window, 0, sizeof(*window));
}

/*
 * Adds bytes to what the shell reads next, as if typed (the Session menu's
 * interrupt and end of file); what does not fit is dropped.
 */
void
terminal_window_type(
	struct terminal_window *window,
	const char *bytes,
	size_t length)
{
	/* Only what fits in the buffer. */
	if (length > sizeof(window->input) - window->input_length)
		length = sizeof(window->input) - window->input_length;

	/* The bytes after those typed before. */
	memcpy(window->input + window->input_length, bytes, length);
	window->input_length += length;
}

/*
 * Asks the compositor to make the window fullscreen, or to end it; the
 * configure that follows says what it did.
 */
void
terminal_window_set_fullscreen(
	struct terminal_window *window,
	int fullscreen)
{
	/* On the default output, or back to a window. */
	kui_window_set_fullscreen(window->kui, fullscreen);
}

/*
 * Returns a monotonic time in milliseconds (0 when the clock cannot be read).
 */
uint64_t
terminal_clock(void)
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

/* Binds the clipboard's and the primary selection's managers (the window has the rest). */
static void
window_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
	struct terminal_window *window;
	int match;

	/* The data device manager shares the clipboard with other clients (clipboard.c). */
	window = data;
	match = strcmp(interface, "wl_data_device_manager");
	if (match == 0 && window->data_manager == NULL) {
		terminal_clipboard_bind(window, registry, name, version);
		return;
	}

	/* The primary selection manager shares the selected text with other clients (primary.c). */
	match = strcmp(interface, "zwp_primary_selection_device_manager_v1");
	if (match == 0 && window->primary_manager == NULL)
		terminal_primary_bind(window, registry, name);
}

/* A global going away does not matter to a terminal that already bound what it needs. */
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

/* Takes every input the window queued, and its size and full screen as the compositor left them. */
static void
window_take(
	struct terminal_window *window)
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

	/* Whether the compositor made the window fullscreen (the View menu shows it). */
	window->fullscreen = kui_window_fullscreen(window->kui);
}

/* Turns one input of the window into the terminal's. */
static void
window_event(
	struct terminal_window *window,
	const struct kui_window_event *event)
{
	/* Every input carries the modifiers held; one with a serial names a selection the terminal may set. */
	window->modifiers = window_modifiers(event->modifiers);
	if (event->serial != 0U)
		window->serial = event->serial;

	/* What it is. */
	switch (event->kind) {
	case KUI_WINDOW_MOTION:
		/* The pointer's place, and a motion for the main loop. */
		window->pointer_x = (int32_t)event->x;
		window->pointer_y = (int32_t)event->y;
		window_pointer_event(window, TERMINAL_POINTER_MOTION, event);
		break;
	case KUI_WINDOW_BUTTON:
		/* The middle button's press pastes the primary selection (ws035-p100). */
		if (event->code == WINDOW_BUTTON_MIDDLE) {
			if (event->pressed)
				window_pointer_event(window, TERMINAL_POINTER_MIDDLE, event);
			break;
		}

		/* Otherwise only the left button, pressed or released. */
		if (event->code != WINDOW_BUTTON_LEFT)
			break;
		if (event->pressed) {
			window_pointer_event(window, TERMINAL_POINTER_PRESS, event);
		} else {
			window_pointer_event(window, TERMINAL_POINTER_RELEASE, event);
		}

		break;
	case KUI_WINDOW_AXIS:
		window_wheel(window, event->dy);
		break;
	case KUI_WINDOW_KEY:
		/* A press (or a held key's repeat) types; a release does nothing. */
		if (event->pressed)
			window_press(window, event->code);
		break;
	case KUI_WINDOW_TOUCH_DOWN:
		window_touch_push(window, TERMINAL_TOUCH_DOWN, event);
		break;
	case KUI_WINDOW_TOUCH_MOTION:
		window_touch_push(window, TERMINAL_TOUCH_MOTION, event);
		break;
	case KUI_WINDOW_TOUCH_UP:
		window_touch_push(window, TERMINAL_TOUCH_UP, event);
		break;
	case KUI_WINDOW_TOUCH_CANCEL:
		window_touch_push(window, TERMINAL_TOUCH_CANCEL, event);
		break;
	case KUI_WINDOW_RESIZE:
		/* The size the compositor gave, drawn at from the next frame. */
		kui_window_size(window->kui, &window->width, &window->height);
		window->resized = 1;
		break;
	case KUI_WINDOW_CLOSE:
		/* The main loop ends the terminal. */
		window->closed = 1;
		break;
	default:
		break;
	}
}

/* Adds a key's bytes to what the shell reads next. */
static void
window_press(
	struct terminal_window *window,
	uint32_t key)
{
	size_t length;
	int shift;

	/* Shift with Page Up scrolls the view a page back instead of typing (ws035-p114). */
	shift = 0;
	if ((window->modifiers & TERMINAL_MODIFIER_SHIFT) != 0U)
		shift = 1;
	if (shift && key == WINDOW_KEY_PAGEUP) {
		window->scroll_pages++;
		return;
	}

	/* Shift with Page Down scrolls a page toward the live screen. */
	if (shift && key == WINDOW_KEY_PAGEDOWN) {
		window->scroll_pages--;
		return;
	}

	/* The bytes, if they fit in what is left of the buffer. */
	length = terminal_key_bytes(key, window->modifiers, window->input + window->input_length, sizeof(window->input) - window->input_length);
	window->input_length += length;
}

/*
 * The wheel turns: each notch toward the user (down) goes forward toward
 * the live screen, away from the user back into the scrollback
 * (ws035-p114); a distance short of a notch waits for the rest.
 */
static void
window_wheel(
	struct terminal_window *window,
	double dy)
{
	int notches;

	/* The distance so far, in notches. */
	window->wheel += dy / WINDOW_WHEEL_NOTCH;
	notches = (int)window->wheel;

	/* Whole notches are kept for the main loop, back as positive. */
	window->scroll_notches -= notches;
	window->wheel -= (double)notches;
}

/* Adds a pointer event at the pointer's place for the main loop (a motion after a motion replaces it; a full queue drops it). */
static void
window_pointer_event(
	struct terminal_window *window,
	unsigned kind,
	const struct kui_window_event *event)
{
	struct terminal_pointer_event *kept;
	uint32_t time;

	/* The compositor's time, in milliseconds of the monotonic clock (the low 32 bits). */
	time = (uint32_t)(event->time_us / 1000U);

	/* A motion after a motion replaces it. */
	if (kind == TERMINAL_POINTER_MOTION && window->pointer_event_count > 0U) {
		kept = &window->pointer_events[window->pointer_event_count - 1U];
		if (kept->kind == TERMINAL_POINTER_MOTION) {
			kept->x = window->pointer_x;
			kept->y = window->pointer_y;
			kept->time = time;
			return;
		}
	}

	/* Room for it. */
	if (window->pointer_event_count >= TERMINAL_POINTER_EVENTS)
		return;

	/* Succeeded: kept, a press with its serial. */
	kept = &window->pointer_events[window->pointer_event_count++];
	kept->kind = kind;
	kept->x = window->pointer_x;
	kept->y = window->pointer_y;
	kept->time = time;
	kept->serial = 0U;
	if (kind != TERMINAL_POINTER_MOTION)
		kept->serial = event->serial;
	kept->modifiers = window->modifiers;
}

/* Queues a touch input for touch.c; a full queue drops it. */
static void
window_touch_push(
	struct terminal_window *window,
	unsigned type,
	const struct kui_window_event *event)
{
	struct terminal_touch_event *kept;

	/* A full queue drops the input (the fingers are far ahead of the program). */
	if (window->touch_count >= TERMINAL_TOUCH_EVENTS)
		return;

	/* The input, after the ones before it: its time as the compositor's milliseconds, when it was read, a down's serial. */
	kept = &window->touches[window->touch_count];
	window->touch_count++;
	memset(kept, 0, sizeof(*kept));
	kept->type = type;
	kept->id = event->id;
	kept->x = (float)event->x;
	kept->y = (float)event->y;
	kept->time = (uint32_t)(event->time_us / 1000U);
	kept->arrival = event->arrival_us;
	if (type == TERMINAL_TOUCH_DOWN)
		kept->serial = event->serial;
}

/* Turns libkeiui's modifier bits into wl_keyboard's, which keys.c and the selection read. */
static uint32_t
window_modifiers(
	unsigned modifiers)
{
	uint32_t bits;

	/* Shift, Control and Alt. */
	bits = 0U;
	if ((modifiers & KUI_MOD_SHIFT) != 0U)
		bits |= TERMINAL_MODIFIER_SHIFT;
	if ((modifiers & KUI_MOD_CTRL) != 0U)
		bits |= TERMINAL_MODIFIER_CONTROL;
	if ((modifiers & KUI_MOD_ALT) != 0U)
		bits |= TERMINAL_MODIFIER_ALT;

	/* Reports them. */
	return bits;
}
