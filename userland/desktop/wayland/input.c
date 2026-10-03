/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Evdev pointers and keyboards read by the seat.
 *
 * Every /dev/input/eventN node that reports REL_X and REL_Y or ABS_X and
 * ABS_Y is a pointer; one that reports the letter keys is a keyboard; a node
 * may be both.  A node that reports BTN_TOOL_PEN and ABS_PRESSURE is a pen
 * tablet instead, whose reports tablet.c applies, and one that speaks
 * multitouch protocol B (ABS_MT_SLOT, ABS_MT_TRACKING_ID and both
 * ABS_MT_POSITION axes) is a touch screen, whose reports touch.c applies.
 * Nodes are read without blocking.  Events are gathered until
 * SYN_REPORT and applied as one group: motion first, then buttons and keys in
 * the order they arrived, then wheel scrolling, then a pointer frame.
 */

#include "zwl.h"
#include "tablet.h"
#include "touch.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Bound the reads one device gets per event-loop pass, so others still run. */
#define INPUT_READS_PER_PASS	16U

/* The events one read of a device takes at most. */
#define INPUT_READ_EVENTS	32U

/*
 * One ready device's events while the devices are read together: those read
 * and not yet applied, and how many reads it has had this pass.
 */
struct input_source {
	struct zwl_input_device *device;
	struct input_event events[INPUT_READ_EVENTS];
	size_t count;
	size_t next;
	unsigned reads;
	unsigned done;
};

/* The mouse buttons, BTN_LEFT through BTN_TASK, delivered as wl_pointer buttons. */
#define INPUT_BUTTON_FIRST	0x110U
#define INPUT_BUTTON_LAST	0x117U

/* Key codes from BTN_MISC up to the end of the button block are not keyboard keys. */
#define INPUT_BUTTON_BLOCK_FIRST	0x100U
#define INPUT_BUTTON_BLOCK_END		0x160U

/* The uapi header does not name the meta keys; these are their evdev codes. */
#define INPUT_KEY_LEFTMETA	125U
#define INPUT_KEY_RIGHTMETA	126U

/* The xkb default modifier mask bits reported in wl_keyboard.modifiers. */
#define MODIFIER_SHIFT		0x01U
#define MODIFIER_CONTROL	0x04U
#define MODIFIER_ALT		0x08U
#define MODIFIER_META		0x40U

/* The locked modifier bits, and the evdev codes of the keys that toggle them (ws035-p078). */
#define MODIFIER_CAPS_LOCK	0x02U
#define MODIFIER_NUM_LOCK	0x10U
#define INPUT_KEY_CAPSLOCK	58U
#define INPUT_KEY_NUMLOCK	69U

/* Bits of server->modifier_keys, one per held modifier key. */
#define HELD_LEFTSHIFT		0x01U
#define HELD_RIGHTSHIFT		0x02U
#define HELD_LEFTCTRL		0x04U
#define HELD_RIGHTCTRL		0x08U
#define HELD_LEFTALT		0x10U
#define HELD_RIGHTALT		0x20U
#define HELD_LEFTMETA		0x40U
#define HELD_RIGHTMETA		0x80U

/* The number of bits in one word of an evdev capability bitmap. */
#define BITMAP_WORD_BITS	(8U * sizeof(unsigned long))

static int read_ranges(int descriptor, struct input_absinfo *x, struct input_absinfo *y);
static int bit_is_set(const unsigned long *bits, unsigned code);
static void consume_event(struct zwl_server *server, struct zwl_input_device *device, const struct input_event *event);
static int source_fill(struct zwl_server *server, struct input_source *source);
static int event_earlier(const struct input_event *event, const struct input_event *other);
static void apply_frame(struct zwl_server *server, struct zwl_input_device *device, uint32_t time);
static void apply_key(struct zwl_server *server, uint32_t time, uint32_t key, int32_t value);
static int32_t scale_absolute(int32_t value, int32_t minimum, int32_t maximum, uint32_t size);
static int32_t clamp_position(int64_t position, uint32_t size);
static uint32_t event_time(const struct input_event *event);
static void update_capabilities(struct zwl_server *server);
static int attach_tablet(struct zwl_server *server, int descriptor, const char *path);
static int attach_touch(struct zwl_server *server, int descriptor, const char *path);
static int multitouch(const struct zwl_input_caps *capabilities);

/*
 * Classifies an open device and attaches it to the seat.
 *
 * The seat takes the descriptor and closes it when unsupported or full.
 */
void
zwl_input_probe(
	struct zwl_server *server,
	int descriptor,
	const char *path,
	const struct zwl_input_caps *capabilities)
{
	struct input_absinfo x;
	struct input_absinfo y;
	unsigned pointer;
	unsigned keyboard;
	unsigned absolute;
	int touch_screen;
	int has_type;
	int has_first;
	int has_second;
	int error;

	/* A pen tablet reports BTN_TOOL_PEN, ABS_PRESSURE and both position axes (tablet.c, WS079 p003). */
	has_type = bit_is_set(capabilities->event, EV_KEY);
	has_first = bit_is_set(capabilities->key, BTN_TOOL_PEN);
	has_second = bit_is_set(capabilities->absolute, ABS_PRESSURE);
	if (has_type &&
	    has_first &&
	    has_second) {
		/* Both position ranges must be readable and nonempty to be mapped. */
		error = read_ranges(descriptor, &x, &y);
		if (error == 0) {
			(void)attach_tablet(server, descriptor, path);
			return;
		}
	}

	/* A touch screen speaks multitouch protocol B (touch.c, WS079 p013); its ABS_X/Y are not a pointer. */
	touch_screen = multitouch(capabilities);
	if (touch_screen) {
		(void)attach_touch(server, descriptor, path);
		return;
	}

	/* A keyboard reports key events including the letter keys A and Z. */
	keyboard = 0;
	has_type = bit_is_set(capabilities->event, EV_KEY);
	has_first = bit_is_set(capabilities->key, KEY_A);
	has_second = bit_is_set(capabilities->key, KEY_Z);
	if (has_type &&
	    has_first &&
	    has_second)
		keyboard = 1;

	/* An absolute pointer reports ABS_X and ABS_Y. */
	absolute = 0;
	has_type = bit_is_set(capabilities->event, EV_ABS);
	has_first = bit_is_set(capabilities->absolute, ABS_X);
	has_second = bit_is_set(capabilities->absolute, ABS_Y);
	if (has_type &&
	    has_first &&
	    has_second) {
		/* Both axis ranges must be readable and nonempty to be mapped. */
		error = read_ranges(descriptor, &x, &y);
		if (error == 0)
			absolute = 1;
	}

	/* A relative pointer reports REL_X and REL_Y. */
	pointer = absolute;
	has_type = bit_is_set(capabilities->event, EV_REL);
	has_first = bit_is_set(capabilities->relative, REL_X);
	has_second = bit_is_set(capabilities->relative, REL_Y);
	if (has_type &&
	    has_first &&
	    has_second)
		pointer = 1;

	/* A node that is neither is not the seat's business. */
	if (!pointer && !keyboard) {
		zwl_input_device_close(server, descriptor);
		return;
	}

	/* Keep the node; an absolute pointer brings its ranges along. */
	if (absolute) {
		(void)zwl_input_attach(server, descriptor, path, pointer, keyboard, &x, &y);
	} else {
		(void)zwl_input_attach(server, descriptor, path, pointer, keyboard, NULL, NULL);
	}

	/* Succeeded: the node has been classified. */
	return;
}

/*
 * Adopts an open evdev descriptor as a pointer, a keyboard or both.
 *
 * The device takes ownership of the descriptor, closing it when no slot is
 * free.  An absolute pointer supplies both axis ranges; a relative one passes
 * NULL for both.
 */
int
zwl_input_attach(
	struct zwl_server *server,
	int descriptor,
	const char *path,
	unsigned pointer,
	unsigned keyboard,
	const struct input_absinfo *x,
	const struct input_absinfo *y)
{
	struct zwl_input_device *device;
	unsigned index;

	/* Find a free slot in the fixed device table. */
	device = NULL;
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		/* A slot not in use can hold the new device. */
		if (!server->inputs[index].live) {
			device = &server->inputs[index];
			break;
		}
	}

	/* A full table cannot take another device. */
	if (device == NULL) {
		zwl_input_device_close(server, descriptor);
		return ENOSPC;
	}

	/* The slot now describes this node. */
	memset(device, 0, sizeof(*device));
	device->fd = descriptor;
	device->pointer = pointer;
	device->keyboard = keyboard;
	snprintf(device->path, sizeof(device->path), "%s", path);

	/* An absolute pointer maps its axis ranges onto the surface. */
	if (pointer && x != NULL && y != NULL) {
		device->absolute = 1;
		device->abs_x_minimum = x->minimum;
		device->abs_x_maximum = x->maximum;
		device->abs_y_minimum = y->minimum;
		device->abs_y_maximum = y->maximum;
		device->abs_x = x->value;
		device->abs_y = y->value;
	}

	/* The slot is published only when it is completely filled in. */
	device->live = 1;

	/* One line per role lets a test see which devices the seat uses. */
	if (pointer)
		printf("ZWL INPUT device=%s kind=pointer abs=%u\n", device->path, device->absolute);
	if (keyboard)
		printf("ZWL INPUT device=%s kind=keyboard abs=0\n", device->path);

	/* Bound seats learn about a new class of device. */
	update_capabilities(server);

	/* Succeeded: the event loop now reads this device. */
	return 0;
}

/*
 * Drains the events the ready devices have and applies them in the order
 * they were made (BUG-142).
 *
 * Each device's reports are gathered in the device until its SYN_REPORT, so
 * the order between devices is the order of their events' times: a key
 * pressed before a click is applied before it even when both waited while
 * the compositor was busy.  Events of one time keep the devices' order, and
 * one device's events keep their own.  A read error other than "nothing
 * ready" means the device went away, and it is closed.
 */
void
zwl_input_read_devices(
	struct zwl_server *server,
	struct zwl_input_device **devices,
	size_t count)
{
	static struct input_source sources[ZWL_INPUT_MAX];
	struct input_source *earliest;
	size_t used;
	size_t index;
	int ready;
	int before;

	/* One source for each device given, up to the seat's devices. */
	used = count;
	if (used > ZWL_INPUT_MAX)
		used = ZWL_INPUT_MAX;
	for (index = 0; index < used; index++) {
		sources[index].device = devices[index];
		sources[index].count = 0;
		sources[index].next = 0;
		sources[index].reads = 0;
		sources[index].done = 0;
	}

	/* Applies the earliest waiting event until no device has one this pass. */
	for (;;) {
		earliest = NULL;
		for (index = 0; index < used; index++) {
			/* A source that ran out reads again, within its reads for the pass. */
			ready = source_fill(server, &sources[index]);
			if (!ready)
				continue;

			/* The first source with the earliest event wins a tie. */
			if (earliest == NULL) {
				earliest = &sources[index];
				continue;
			}

			/* A later source wins only with an earlier event. */
			before = event_earlier(&sources[index].events[sources[index].next],
					       &earliest->events[earliest->next]);
			if (before)
				earliest = &sources[index];
		}

		/* Nothing is left to apply. */
		if (earliest == NULL)
			return;

		/* Applies the event and moves its source on. */
		consume_event(server, earliest->device, &earliest->events[earliest->next]);
		earliest->next++;
	}
}

/*
 * Makes sure a source has an event to apply, reading its device when the
 * events read before are used up.  Reports 1 when it has one, 0 when the
 * device has nothing more this pass (nothing ready, its reads used, or gone).
 */
static int
source_fill(
	struct zwl_server *server,
	struct input_source *source)
{
	ssize_t count;
	int error;

	/* Events read before are still waiting. */
	if (source->next < source->count)
		return 1;

	/* Reads until the device gives events, has none, or the pass's reads are used. */
	while (!source->done) {
		/* A closed slot, or one that had its reads, has nothing more this pass. */
		if (!source->device->live || source->reads >= INPUT_READS_PER_PASS) {
			source->done = 1;
			break;
		}

		/* Reads as many whole events as the buffer holds. */
		source->reads++;
		count = zwl_input_device_read(source->device->fd, source->events, INPUT_READ_EVENTS);
		if (count > 0) {
			source->count = (size_t)count;
			source->next = 0;
			return 1;
		}

		/* End of file means the node is gone. */
		if (count == 0) {
			printf("ZWL INPUT_CLOSED device=%s errno=%d\n", source->device->path, EIO);
			zwl_input_close(server, source->device);
			source->done = 1;
			break;
		}

		/* Nothing more is ready; the next poll will say when there is. */
		error = errno;
		if (error == EAGAIN || error == EWOULDBLOCK) {
			source->done = 1;
			break;
		}

		/* Any other failure but an interruption means the device is gone. */
		if (error != EINTR) {
			printf("ZWL INPUT_CLOSED device=%s errno=%d\n", source->device->path, error);
			zwl_input_close(server, source->device);
			source->done = 1;
		}
	}

	/* Nothing to apply from this device this pass. */
	return 0;
}

/* Reports whether an event was made before another (by its evdev time). */
static int
event_earlier(
	const struct input_event *event,
	const struct input_event *other)
{
	/* The seconds decide first. */
	if (event->time.tv_sec < other->time.tv_sec)
		return 1;
	if (event->time.tv_sec > other->time.tv_sec)
		return 0;

	/* Then the microseconds. */
	if (event->time.tv_usec < other->time.tv_usec)
		return 1;

	/* Made at the same time or later. */
	return 0;
}

/*
 * Closes one device and tells bound seats when a device class disappears.
 */
void
zwl_input_close(
	struct zwl_server *server,
	struct zwl_input_device *device)
{
	/* A slot that is not in use has no descriptor. */
	if (!device->live)
		return;

	/* A pen tablet's clients hear that it and its tools are gone (tablet.c). */
	if (device->tablet)
		zwl_tablet_remove(server, device, 1);

	/* A touch screen's fingers end (touch.c). */
	if (device->touch)
		zwl_touch_remove(server, device, 1);

	/* The slot is free once its descriptor is closed. */
	zwl_input_device_close(server, device->fd);
	device->fd = -1;
	device->live = 0;

	/* Bound seats learn that a class of device may be gone. */
	update_capabilities(server);

	/* Succeeded: the device is no longer read. */
	return;
}

/*
 * Closes every device during service shutdown without notifying clients.
 */
void
zwl_input_cleanup(
	struct zwl_server *server)
{
	unsigned index;

	/* Every slot in use owns one descriptor. */
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		/* A free slot owns nothing. */
		if (!server->inputs[index].live)
			continue;

		/* A pen tablet's state is forgotten without telling anyone (tablet.c). */
		if (server->inputs[index].tablet)
			zwl_tablet_remove(server, &server->inputs[index], 0);

		/* A touch screen's fingers are forgotten the same way (touch.c). */
		if (server->inputs[index].touch)
			zwl_touch_remove(server, &server->inputs[index], 0);

		/* Close the descriptor and free the slot. */
		zwl_input_device_close(server, server->inputs[index].fd);
		server->inputs[index].fd = -1;
		server->inputs[index].live = 0;
	}

	/* No client remains to hear about it, so the capabilities are simply cleared. */
	server->capabilities = 0;

	/* Succeeded: the seat holds no device descriptors. */
	return;
}

/* Reads both absolute axis ranges and refuses a range that cannot be mapped. */
static int
read_ranges(
	int descriptor,
	struct input_absinfo *x,
	struct input_absinfo *y)
{
	int error;

	/* The horizontal range maps onto the surface width. */
	memset(x, 0, sizeof(*x));
	error = zwl_input_device_absinfo(descriptor, ABS_X, x);
	if (error != 0)
		return error;

	/* The vertical range maps onto the surface height. */
	memset(y, 0, sizeof(*y));
	error = zwl_input_device_absinfo(descriptor, ABS_Y, y);
	if (error != 0)
		return error;

	/* An empty or inverted range has no pixel to map to. */
	if (x->maximum <= x->minimum || y->maximum <= y->minimum)
		return EINVAL;

	/* Succeeded: both axes can be scaled onto the surface. */
	return 0;
}

/* Reports whether one code is set in an evdev bitmap of unsigned long words. */
static int
bit_is_set(
	const unsigned long *bits,
	unsigned code)
{
	unsigned long word;

	/* The word holding the code, and the code's bit within it. */
	word = bits[code / BITMAP_WORD_BITS];
	if ((word & (1UL << (code % BITMAP_WORD_BITS))) != 0)
		return 1;

	/* The code is not reported. */
	return 0;
}

/* Adds one event to the device's pending report, or completes the report. */
static void
consume_event(
	struct zwl_server *server,
	struct zwl_input_device *device,
	const struct input_event *event)
{
	uint32_t time;

	/* The kernel dropped events: forget the partial report and skip to the next one. */
	if (event->type == EV_SYN && event->code == SYN_DROPPED) {
		device->frame_count = 0;
		device->discarding = 1;
		return;
	}

	/* SYN_REPORT completes a report, unless the report was damaged by a drop. */
	if (event->type == EV_SYN && event->code == SYN_REPORT) {
		/* A damaged report is thrown away; the next one is applied again. */
		if (device->discarding) {
			device->discarding = 0;
			device->frame_count = 0;
			return;
		}

		/* An intact report is applied as one group: a pen tablet's by the tablet (tablet.c), a touch screen's by touch.c. */
		time = event_time(event);
		device->frame_time_us = (uint64_t)event->time.tv_sec * 1000000U + (uint64_t)event->time.tv_usec;
		if (device->tablet) {
			zwl_tablet_frame(server, device, time);
		} else if (device->touch) {
			zwl_touch_frame(server, device, time);
		} else {
			apply_frame(server, device, time);
		}

		/* The next report starts empty. */
		device->frame_count = 0;
		return;
	}

	/* Other synchronization events and a damaged report carry nothing to keep. */
	if (event->type == EV_SYN || device->discarding)
		return;

	/* The exit summary counts every evdev event that was not a synchronization. */
	server->input_events++;

	/* A report too large to hold is thrown away like a dropped one. */
	if (device->frame_count == ZWL_INPUT_FRAME_MAX) {
		device->frame_count = 0;
		device->discarding = 1;
		return;
	}

	/* Keep the event until its report completes. */
	device->frame[device->frame_count] = *event;
	device->frame_count++;

	/* Succeeded: the event waits for SYN_REPORT. */
	return;
}

/* Applies one completed report: motion, buttons and keys, scrolling, frame. */
static void
apply_frame(
	struct zwl_server *server,
	struct zwl_input_device *device,
	uint32_t time)
{
	const struct input_event *event;
	int64_t delta_x;
	int64_t delta_y;
	int32_t wheel;
	int32_t horizontal_wheel;
	int32_t x;
	int32_t y;
	int32_t old_x;
	int32_t old_y;
	unsigned absolute_seen;
	unsigned pointer_activity;
	unsigned index;

	/* Collect relative movement, wheel steps and the latest absolute position. */
	delta_x = 0;
	delta_y = 0;
	wheel = 0;
	horizontal_wheel = 0;
	absolute_seen = 0;
	for (index = 0; index < device->frame_count; index++) {
		event = &device->frame[index];

		/* Only a pointer's axes move the pointer. */
		if (!device->pointer)
			break;

		/* Relative axes add up over the report. */
		if (event->type == EV_REL) {
			/* Each relative code feeds its own sum. */
			if (event->code == REL_X) {
				delta_x += event->value;
			} else if (event->code == REL_Y) {
				delta_y += event->value;
			} else if (event->code == REL_WHEEL) {
				wheel += event->value;
			} else if (event->code == REL_HWHEEL) {
				horizontal_wheel += event->value;
			}
		}

		/* Absolute axes replace the device's remembered position. */
		if (event->type == EV_ABS && device->absolute) {
			/* Each axis is remembered separately, as a report may carry only one. */
			if (event->code == ABS_X) {
				device->abs_x = event->value;
				absolute_seen = 1;
			} else if (event->code == ABS_Y) {
				device->abs_y = event->value;
				absolute_seen = 1;
			}
		}
	}

	/*
	 * The device placed or moved the pointer, even onto the place it
	 * already had: its arrow is shown from now on, drawn where it is
	 * (ws035-p116).
	 */
	if (server->pointer_unmoved &&
	    (absolute_seen ||
	     delta_x != 0 ||
	     delta_y != 0)) {
		server->pointer_unmoved = 0U;
		zwl_damage_pointer(server, server->pointer_x, server->pointer_y);
	}

	/* An absolute report places the pointer; relative movement is added and clamped. */
	x = server->pointer_x;
	y = server->pointer_y;
	if (absolute_seen) {
		x = scale_absolute(device->abs_x, device->abs_x_minimum, device->abs_x_maximum, server->width);
		y = scale_absolute(device->abs_y, device->abs_y_minimum, device->abs_y_maximum, server->height);
	}

	/*
	 * The relative movement at the user's speed (ws089-p007, a percentage),
	 * the hundredths of a pixel carried to the next report so that a slow
	 * pointer still moves.
	 */
	if (server->pointer_speed != 100 &&
	    (delta_x != 0 ||
	     delta_y != 0)) {
		delta_x = delta_x * server->pointer_speed + server->pointer_remainder_x;
		delta_y = delta_y * server->pointer_speed + server->pointer_remainder_y;
		server->pointer_remainder_x = delta_x % 100;
		server->pointer_remainder_y = delta_y % 100;
		delta_x /= 100;
		delta_y /= 100;
	}

	/* The relative movement, kept on the output. */
	x = clamp_position((int64_t)x + delta_x, server->width);
	y = clamp_position((int64_t)y + delta_y, server->height);

	/* A changed position is reported as motion before any button of the same report. */
	pointer_activity = 0;
	if (x != server->pointer_x || y != server->pointer_y) {
		old_x = server->pointer_x;
		old_y = server->pointer_y;
		server->pointer_x = x;
		server->pointer_y = y;
		zwl_seat_motion(server, time);

		/* Window mode draws the cursor at its new place (and where it was, damage.c). */
		zwl_damage_pointer(server, old_x, old_y);
		pointer_activity = 1;
	}

	/* Buttons and keys follow in the order the device reported them. */
	for (index = 0; index < device->frame_count; index++) {
		event = &device->frame[index];

		/* Only key-type events are buttons or keys. */
		if (event->type != EV_KEY)
			continue;

		/* Autorepeat (value 2) is not forwarded; the client is told not to repeat either. */
		if (event->value != 0 && event->value != 1)
			continue;

		/* A pointer's mouse buttons become wl_pointer buttons with their BTN_ code. */
		if (device->pointer &&
		    event->code >= INPUT_BUTTON_FIRST &&
		    event->code <= INPUT_BUTTON_LAST) {
			zwl_seat_button(server, time, event->code, (uint32_t)event->value);
			pointer_activity = 1;
			continue;
		}

		/* The rest of the button block is neither a mouse button nor a key. */
		if (event->code >= INPUT_BUTTON_BLOCK_FIRST && event->code < INPUT_BUTTON_BLOCK_END)
			continue;

		/* A keyboard's keys become wl_keyboard keys with their evdev code. */
		if (device->keyboard)
			apply_key(server, time, event->code, event->value);
	}

	/* The user's natural scrolling (ws089-p007) turns the wheel round. */
	if (server->pointer_natural != 0) {
		wheel = -wheel;
		horizontal_wheel = -horizontal_wheel;
	}

	/* Wheel notches scroll; evdev counts up as positive, Wayland counts down as positive. */
	if (wheel != 0 || horizontal_wheel != 0) {
		zwl_seat_axis(server, time, -wheel, horizontal_wheel);
		pointer_activity = 1;
	}

	/* Version 5 pointers are told where this report's events end. */
	if (pointer_activity)
		zwl_seat_frame(server);

	/* Succeeded: the report has been delivered. */
	return;
}

/* Tracks the modifier keys and delivers one key with any modifier change. */
static void
apply_key(
	struct zwl_server *server,
	uint32_t time,
	uint32_t key,
	int32_t value)
{
	unsigned held;
	uint32_t modifiers;
	uint32_t locked;

	/* Each modifier key has its own held bit, so left and right are independent. */
	held = 0;
	switch (key) {
	case KEY_LEFTSHIFT:
		held = HELD_LEFTSHIFT;
		break;
	case KEY_RIGHTSHIFT:
		held = HELD_RIGHTSHIFT;
		break;
	case KEY_LEFTCTRL:
		held = HELD_LEFTCTRL;
		break;
	case KEY_RIGHTCTRL:
		held = HELD_RIGHTCTRL;
		break;
	case KEY_LEFTALT:
		held = HELD_LEFTALT;
		break;
	case KEY_RIGHTALT:
		held = HELD_RIGHTALT;
		break;
	case INPUT_KEY_LEFTMETA:
		held = HELD_LEFTMETA;
		break;
	case INPUT_KEY_RIGHTMETA:
		held = HELD_RIGHTMETA;
		break;
	default:
		break;
	}

	/*
	 * modifier_keys records which modifier keys are down right now; the
	 * depressed mask sent to clients is derived from it.
	 */
	if (value != 0) {
		server->modifier_keys |= held;
	} else {
		server->modifier_keys &= ~held;
	}

	/* Fold the held keys into the xkb default modifier bits. */
	modifiers = 0;
	if ((server->modifier_keys & (HELD_LEFTSHIFT | HELD_RIGHTSHIFT)) != 0)
		modifiers |= MODIFIER_SHIFT;
	if ((server->modifier_keys & (HELD_LEFTCTRL | HELD_RIGHTCTRL)) != 0)
		modifiers |= MODIFIER_CONTROL;
	if ((server->modifier_keys & (HELD_LEFTALT | HELD_RIGHTALT)) != 0)
		modifiers |= MODIFIER_ALT;
	if ((server->modifier_keys & (HELD_LEFTMETA | HELD_RIGHTMETA)) != 0)
		modifiers |= MODIFIER_META;

	/* A press of Caps Lock or Num Lock toggles its lock (a kernel repeat, value 2, does not). */
	locked = server->locked_modifiers;
	if (value == 1 && key == INPUT_KEY_CAPSLOCK)
		locked ^= MODIFIER_CAPS_LOCK;
	if (value == 1 && key == INPUT_KEY_NUMLOCK)
		locked ^= MODIFIER_NUM_LOCK;

	/* The key itself is reported first. */
	zwl_seat_key(server, time, key, (uint32_t)value);

	/* A changed mask follows the key that changed it. */
	if (modifiers != server->modifiers || locked != server->locked_modifiers) {
		server->modifiers = modifiers;
		server->locked_modifiers = locked;
		zwl_seat_modifiers(server);
	}

	/* Succeeded: the key and the modifier state are delivered. */
	return;
}

/* Maps one absolute axis value onto 0 .. size - 1 surface pixels. */
static int32_t
scale_absolute(
	int32_t value,
	int32_t minimum,
	int32_t maximum,
	uint32_t size)
{
	int64_t offset;
	int64_t range;

	/* The low end of the range, and anything below it, is the first pixel. */
	if (size <= 1U || value <= minimum)
		return 0;

	/* The high end of the range, and anything above it, is the last pixel. */
	if (value >= maximum)
		return (int32_t)size - 1;

	/* Everything between scales linearly, rounding down. */
	offset = (int64_t)value - minimum;
	range = (int64_t)maximum - minimum;

	/* Succeeded: the pixel the value falls on. */
	return (int32_t)((offset * ((int64_t)size - 1)) / range);
}

/* Keeps a pointer coordinate inside 0 .. size - 1. */
static int32_t
clamp_position(
	int64_t position,
	uint32_t size)
{
	/* Nothing lies left of or above the surface. */
	if (position < 0)
		return 0;

	/* Nothing lies right of or below the surface. */
	if (position >= (int64_t)size)
		return (int32_t)size - 1;

	/* Succeeded: the position is already on the surface. */
	return (int32_t)position;
}

/* Converts an evdev timestamp to the wrapping millisecond count Wayland uses. */
static uint32_t
event_time(
	const struct input_event *event)
{
	uint64_t milliseconds;

	/* Seconds and microseconds become milliseconds, keeping the low 32 bits. */
	milliseconds = (uint64_t)event->time.tv_sec * 1000U;
	milliseconds += (uint64_t)event->time.tv_usec / 1000U;

	/* Succeeded: the event time with an undefined base, as the protocol allows. */
	return (uint32_t)milliseconds;
}

/* Recomputes the seat's capability bits and tells bound seats when they change. */
static void
update_capabilities(
	struct zwl_server *server)
{
	unsigned capabilities;
	unsigned index;

	/* The seat offers each class some open device provides. */
	capabilities = 0;
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		/* A free slot provides nothing. */
		if (!server->inputs[index].live)
			continue;

		/*
		 * wl_seat's pointer, keyboard and touch capability bits; a pen
		 * tablet and a touch screen are also a pointer (their fallback for
		 * a client without the tablet or wl_touch).
		 */
		if (server->inputs[index].pointer)
			capabilities |= 1U;
		if (server->inputs[index].tablet)
			capabilities |= 1U;
		if (server->inputs[index].keyboard)
			capabilities |= 2U;
		if (server->inputs[index].touch)
			capabilities |= 1U | 4U;
	}

	/* Unchanged capabilities need no event. */
	if (capabilities == server->capabilities)
		return;

	/* Bound seats learn the new set. */
	server->capabilities = capabilities;
	zwl_seat_capabilities(server);

	/* Succeeded: every seat binding agrees with the open devices. */
	return;
}

/*
 * Adopts an open evdev descriptor as a pen tablet (tablet.c, WS079 p003).
 *
 * The device takes ownership of the descriptor, closing it when no slot is
 * free or the tablet cannot take another device.
 */
static int
attach_tablet(
	struct zwl_server *server,
	int descriptor,
	const char *path)
{
	struct zwl_input_device *device;
	unsigned index;
	int error;

	/* Find a free slot in the fixed device table. */
	device = NULL;
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		/* A slot not in use can hold the new device. */
		if (!server->inputs[index].live) {
			device = &server->inputs[index];
			break;
		}
	}

	/* A full table cannot take another device. */
	if (device == NULL) {
		zwl_input_device_close(server, descriptor);
		return ENOSPC;
	}

	/* The slot now describes this node; it is neither a pointer nor a keyboard of its own. */
	memset(device, 0, sizeof(*device));
	device->fd = descriptor;
	device->tablet = 1;
	snprintf(device->path, sizeof(device->path), "%s", path);

	/* The tablet reads the axes and tells the bound tablet seats (tablet.c). */
	error = zwl_tablet_add(server, device);
	if (error != 0) {
		zwl_input_device_close(server, descriptor);
		device->fd = -1;
		device->tablet = 0;
		return error;
	}

	/* The slot is published only when it is completely filled in. */
	device->live = 1;

	/* One line lets a test see which devices the seat uses. */
	printf("ZWL INPUT device=%s kind=tablet abs=1\n", device->path);

	/* Bound seats learn that a pointer (the pen's fallback) is there. */
	update_capabilities(server);

	/* Succeeded: the event loop now reads this device. */
	return 0;
}

/*
 * Adopts an open evdev descriptor as a touch screen (touch.c, WS079 p013).
 *
 * The device takes ownership of the descriptor, closing it when no slot is
 * free or the touch screens cannot take another device.
 */
static int
attach_touch(
	struct zwl_server *server,
	int descriptor,
	const char *path)
{
	struct zwl_input_device *device;
	unsigned index;
	int error;

	/* Find a free slot in the fixed device table. */
	device = NULL;
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		/* A slot not in use can hold the new device. */
		if (!server->inputs[index].live) {
			device = &server->inputs[index];
			break;
		}
	}

	/* A full table cannot take another device. */
	if (device == NULL) {
		zwl_input_device_close(server, descriptor);
		return ENOSPC;
	}

	/* The slot now describes this node; it is neither a pointer nor a keyboard of its own. */
	memset(device, 0, sizeof(*device));
	device->fd = descriptor;
	device->touch = 1;
	snprintf(device->path, sizeof(device->path), "%s", path);

	/* The touch screen reads its range (touch.c). */
	error = zwl_touch_add(device);
	if (error != 0) {
		zwl_input_device_close(server, descriptor);
		device->fd = -1;
		device->touch = 0;
		return error;
	}

	/* The slot is published only when it is completely filled in. */
	device->live = 1;

	/* One line lets a test see which devices the seat uses. */
	printf("ZWL INPUT device=%s kind=touch abs=1\n", device->path);

	/* Bound seats learn that a touch screen (and the pointer of its fallback) is there. */
	update_capabilities(server);

	/* Succeeded: the event loop now reads this device. */
	return 0;
}

/* Reports whether a node speaks multitouch protocol B: slots, tracking numbers and both places. */
static int
multitouch(
	const struct zwl_input_caps *capabilities)
{
	int present;

	/* Absolute axes at all. */
	present = bit_is_set(capabilities->event, EV_ABS);
	if (!present)
		return 0;

	/* The slot the finger events address, and the finger's number. */
	present = bit_is_set(capabilities->absolute, ABS_MT_SLOT);
	if (!present)
		return 0;
	present = bit_is_set(capabilities->absolute, ABS_MT_TRACKING_ID);
	if (!present)
		return 0;

	/* The finger's place across and down. */
	present = bit_is_set(capabilities->absolute, ABS_MT_POSITION_X);
	if (!present)
		return 0;
	present = bit_is_set(capabilities->absolute, ABS_MT_POSITION_Y);
	if (!present)
		return 0;

	/* Succeeded: the node is a touch screen. */
	return 1;
}
