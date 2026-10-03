/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The System Monitor's input (design.md sections 3.9 and 3.10): the
 * fingers, the pointer and the keys.
 *
 *   tap / click             a plate comes forward as a card; outside it,
 *                           the card goes back (unless pinned)
 *   long press / right click   pins the card (it stays), or unpins it
 *   swipe (one finger)      across a plate: the time range, longer to the
 *                           left, shorter to the right
 *   pinch                   apart: the plate under the fingers as a card
 *                           (detail); together: no card (overview)
 *   two-finger tap          detail or overview, in turn
 *   drag on the core        turns it a little; it comes back when let go
 *   Tab, Enter, Esc, P      the keyboard's plate, its card, back, pin
 *   Left, Right, Shift+wheel   the time range
 *
 * Taps, long presses and drags are libkeiland's gestures (keiland.h); the
 * two-finger tap is told here, since the gestures give no tap for two
 * fingers.  Every action is logged (ZMON CARD, VIEW, RANGE, FOCUS, CORE)
 * for the tests.
 */

#include "app.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* The evdev codes of the keys. */
#define KEY_ESC			1U
#define KEY_TAB			15U
#define KEY_ENTER		28U
#define KEY_SPACE		57U
#define KEY_P			25U
#define KEY_LEFT		105U
#define KEY_RIGHT		106U

/* How long a card takes to come out or go back, in milliseconds. */
#define CARD_MS			360.0f

/* A swipe: this fast (px/s) or this far (px), mostly sideways. */
#define SWIPE_SPEED		400.0
#define SWIPE_DISTANCE		120.0f

/* A long press with the pointer, and how far a press may move to stay a click, in milliseconds and pixels. */
#define PRESS_LONG_MS		500U
#define PRESS_SLOP		8.0f

/* A two-finger tap: both down within this of each other and up within this, in microseconds. */
#define TWO_TAP_US		300000U

/* A pinch acts past these ratios of the fingers' distance. */
#define PINCH_OUT		1.25
#define PINCH_IN		0.8

/* How far the core turns, at most, in radians, and per pixel dragged. */
#define CORE_TURN_MAX		0.44f
#define CORE_TURN_PER_PIXEL	0.004f

/* The plates' names in the log, and the keyboard's order through them. */
static const char *const plate_names[SM_PLATES] = {
	"cpu", "gpu", "memory", "network", "disk", "cores", "state", "graphics", "flow", "strata", "lanes", "latency", "events"
};

static const enum sm_plate keyboard_order[] = {
	SM_PLATE_CPU, SM_PLATE_GPU, SM_PLATE_MEMORY, SM_PLATE_NETWORK, SM_PLATE_DISK, SM_PLATE_CORES, SM_PLATE_STATE,
	SM_PLATE_GRAPHICS, SM_PLATE_FLOW, SM_PLATE_STRATA, SM_PLATE_LANES, SM_PLATE_EVENTS
};

static void card_open(struct sm_app *app, int plate, const char *how);
static void card_close(struct sm_app *app, const char *how);
static void card_pin(struct sm_app *app);
static void act_tap(struct sm_app *app, float x, float y, const char *how);
static void act_long_press(struct sm_app *app, float x, float y);
static void act_swipe(struct sm_app *app, double dx);
static void act_view(struct sm_app *app, int detail, float x, float y, const char *how);
static int hit_plate(const struct sm_app *app, float x, float y);
static int on_core(const struct sm_app *app, float x, float y);
static int finger_slot(const struct sm_touch *touch, int32_t id);
static void touch_event(struct sm_app *app, const struct kui_window_event *event);
static void pointer_event(struct sm_app *app, const struct kui_window_event *event, uint64_t now_ms);
static void key_event(struct sm_app *app, const struct kui_window_event *event);
static void gesture_events(struct sm_app *app, uint64_t now_us);

/*
 * Makes the gestures of the window's fingers; returns 0 or -1.
 */
int
sm_interact_open(
	struct sm_app *app)
{
	/* No card, no keyboard plate, the gestures. */
	app->focus.plate = -1;
	app->focus.keyboard = -1;
	app->touch.gesture = keiland_gesture_create();
	if (app->touch.gesture == NULL)
		return -1;

	/* Succeeded: the input can be taken. */
	return 0;
}

/*
 * Releases the gestures.
 */
void
sm_interact_close(
	struct sm_app *app)
{
	/* The gestures, when made. */
	if (app->touch.gesture != NULL)
		keiland_gesture_destroy(app->touch.gesture);
	app->touch.gesture = NULL;
}

/*
 * Takes one input of the window: a finger, the pointer or a key.
 */
void
sm_interact_event(
	struct sm_app *app,
	const struct kui_window_event *event,
	uint64_t now_ms)
{
	/* Each kind its own way. */
	switch (event->kind) {
	case KUI_WINDOW_TOUCH_DOWN:
	case KUI_WINDOW_TOUCH_MOTION:
	case KUI_WINDOW_TOUCH_UP:
	case KUI_WINDOW_TOUCH_CANCEL:
		touch_event(app, event);
		break;
	case KUI_WINDOW_MOTION:
	case KUI_WINDOW_LEAVE:
	case KUI_WINDOW_BUTTON:
	case KUI_WINDOW_AXIS:
		pointer_event(app, event, now_ms);
		break;
	case KUI_WINDOW_KEY:
		key_event(app, event);
		break;
	default:
		break;
	}
}

/*
 * Keeps the input's time: the fingers' gestures (a long press comes by
 * the clock), a pinch, the core's drag, the pointer's long press, and the
 * card's coming and going.  Returns 1 while something moves that needs
 * frames.
 */
int
sm_interact_tick(
	struct sm_app *app,
	uint64_t now_ms)
{
	struct sm_touch *touch;
	struct sm_focus *focus;
	double scale;
	double x;
	double y;
	double dx;
	double dy;
	float seconds;
	int error;
	int moving;

	/* The gestures. */
	touch = &app->touch;
	focus = &app->focus;
	gesture_events(app, kui_clock_us());
	moving = 0;

	/* Two fingers: a pinch acts once it passes a ratio. */
	if (touch->fingers == 2U && !touch->pinched) {
		error = keiland_gesture_pinch(touch->gesture, kui_clock_us(), &scale, &x, &y);
		if (error == 0 && scale >= PINCH_OUT) {
			touch->pinched = 1;
			act_view(app, 1, (float)x, (float)y, "pinch");
		} else if (error == 0 && scale <= PINCH_IN) {
			touch->pinched = 1;
			act_view(app, 0, (float)x, (float)y, "pinch");
		}
	}

	/* A finger dragging the core turns it. */
	if (touch->dragging && touch->drag_core) {
		error = keiland_gesture_drag_offset(touch->gesture, kui_clock_us(), &dx, &dy);
		if (error == 0)
			touch->core_turn = fmaxf(-CORE_TURN_MAX, fminf(CORE_TURN_MAX, (float)dx * CORE_TURN_PER_PIXEL));
		moving = 1;
	}

	/* The pointer held still on a plate: a long press, once. */
	if (touch->pressed && !touch->press_moved && !touch->press_long && now_ms >= touch->press_ms + PRESS_LONG_MS) {
		touch->press_long = 1;
		act_long_press(app, touch->press_x, touch->press_y);
	}

	/* The core comes back when nothing holds it. */
	if (!(touch->dragging && touch->drag_core) && !(touch->pressed && touch->press_core) && touch->core_turn != 0.0f) {
		touch->core_turn *= 0.85f;
		if (fabsf(touch->core_turn) < 0.002f)
			touch->core_turn = 0.0f;
		moving = 1;
	}

	/* The card comes out or goes back over CARD_MS (at once with the clock stopped). */
	seconds = 0.0f;
	if (focus->last_ms != 0U && now_ms > focus->last_ms)
		seconds = (float)(now_ms - focus->last_ms) / 1000.0f;
	focus->last_ms = now_ms;
	if (app->fixed_clock)
		seconds = 1.0f;
	if (focus->opening && focus->progress < 1.0f) {
		focus->progress = fminf(1.0f, focus->progress + seconds * 1000.0f / CARD_MS);
		moving = 1;
	} else if (!focus->opening && focus->progress > 0.0f) {
		focus->progress = fmaxf(0.0f, focus->progress - seconds * 1000.0f / CARD_MS);
		if (focus->progress <= 0.0f)
			focus->plate = -1;
		moving = 1;
	}

	/* Succeeded: whether frames are needed. */
	return moving;
}

/*
 * Names a plate for the log.
 */
const char *
sm_plate_name(
	enum sm_plate plate)
{
	/* The table's name. */
	return plate_names[plate];
}

/* Brings a plate forward as a card (or keeps the one that is), logging how. */
static void
card_open(
	struct sm_app *app,
	int plate,
	const char *how)
{
	/* Another card goes first; the new one comes out. */
	if (app->focus.plate != plate) {
		app->focus.progress = 0.0f;
		app->focus.pinned = 0;
	}

	app->focus.plate = plate;
	app->focus.opening = 1;
	app->dirty = 1;
	printf("ZMON CARD open plate=%s via=%s\n", plate_names[plate], how);
}

/* Sends the card back, logging how. */
static void
card_close(
	struct sm_app *app,
	const char *how)
{
	/* Nothing out. */
	if (app->focus.plate < 0 || !app->focus.opening)
		return;

	/* Going back, unpinned. */
	app->focus.opening = 0;
	app->focus.pinned = 0;
	app->dirty = 1;
	printf("ZMON CARD close plate=%s via=%s\n", plate_names[app->focus.plate], how);
}

/* Pins the card, or unpins it. */
static void
card_pin(
	struct sm_app *app)
{
	/* Only a card that is out. */
	if (app->focus.plate < 0 || !app->focus.opening)
		return;

	/* The other way. */
	app->focus.pinned = !app->focus.pinned;
	app->dirty = 1;
	printf("ZMON CARD pin=%d plate=%s\n", app->focus.pinned, plate_names[app->focus.plate]);
}

/* A tap or a click at a point: a plate forward, or the card back when outside it. */
static void
act_tap(
	struct sm_app *app,
	float x,
	float y,
	const char *how)
{
	int plate;

	/* With a card out: a tap outside it sends it back unless pinned. */
	if (app->focus.plate >= 0 && app->focus.opening) {
		if (app->focus.pinned)
			return;
		card_close(app, how);
		return;
	}

	/* Otherwise the plate there comes forward. */
	plate = hit_plate(app, x, y);
	if (plate >= 0)
		card_open(app, plate, how);
}

/* A long press: pins the card, or brings the plate there forward pinned. */
static void
act_long_press(
	struct sm_app *app,
	float x,
	float y)
{
	int plate;

	/* A card out: pinned or unpinned. */
	if (app->focus.plate >= 0 && app->focus.opening) {
		card_pin(app);
		return;
	}

	/* Otherwise the plate there, forward and pinned. */
	plate = hit_plate(app, x, y);
	if (plate < 0)
		return;
	card_open(app, plate, "long-press");
	card_pin(app);
}

/* A sideways swipe: a longer range to the left, a shorter one to the right. */
static void
act_swipe(
	struct sm_app *app,
	double dx)
{
	/* Leftwards: longer. */
	if (dx < 0.0 && app->range + 1U < SM_RANGES) {
		sm_set_range(app, app->range + 1U);
		return;
	}

	/* Rightwards: shorter. */
	if (dx > 0.0 && app->range > 0U)
		sm_set_range(app, app->range - 1U);
}

/* The detail (the plate at a point as a card) or the overview (no card). */
static void
act_view(
	struct sm_app *app,
	int detail,
	float x,
	float y,
	const char *how)
{
	int plate;

	/* The overview: the card back, pinned or not. */
	if (!detail) {
		printf("ZMON VIEW overview via=%s\n", how);
		app->focus.pinned = 0;
		card_close(app, how);
		return;
	}

	/* The detail of the plate there. */
	plate = hit_plate(app, x, y);
	if (plate < 0)
		plate = SM_PLATE_STATE;
	printf("ZMON VIEW detail plate=%s via=%s\n", plate_names[plate], how);
	card_open(app, plate, how);
}

/* The plate at a point (the latency's in front of the disks'), or -1. */
static int
hit_plate(
	const struct sm_app *app,
	float x,
	float y)
{
	const struct sm_box *box;
	int plate;

	/* From the front-most down. */
	for (plate = SM_PLATES - 1; plate >= 0; plate--) {
		box = &app->view.plates[plate];
		if (x >= box->x && x < box->x + box->width && y >= box->y && y < box->y + box->height)
			return plate;
	}

	/* None. */
	return -1;
}

/* Whether a point is on the state's core (the middle of its plate). */
static int
on_core(
	const struct sm_app *app,
	float x,
	float y)
{
	const struct sm_box *box;

	/* The plate's middle, between its title and its words. */
	box = &app->view.plates[SM_PLATE_STATE];
	if (x < box->x + box->width * 0.2f || x > box->x + box->width * 0.8f)
		return 0;
	if (y < box->y + box->height * 0.15f || y > box->y + box->height * 0.75f)
		return 0;

	/* Succeeded: on the core. */
	return 1;
}

/* A finger's slot by its id, or -1. */
static int
finger_slot(
	const struct sm_touch *touch,
	int32_t id)
{
	unsigned slot;

	/* The slots in use. */
	for (slot = 0; slot < SM_FINGERS; slot++) {
		if (touch->ids[slot] == id + 1)
			return (int)slot;
	}

	/* None. */
	return -1;
}

/* A finger's event: to the gestures, and into the two-finger tap's and the swipe's records. */
static void
touch_event(
	struct sm_app *app,
	const struct kui_window_event *event)
{
	struct sm_touch *touch;
	float dx;
	float dy;
	int slot;
	unsigned free_slot;

	/* Each kind. */
	touch = &app->touch;
	slot = finger_slot(touch, event->id);
	switch (event->kind) {
	case KUI_WINDOW_TOUCH_DOWN:
		/* A new finger: its slot, its place, and the count. */
		(void)keiland_gesture_down(touch->gesture, event->id, event->time_us, event->arrival_us, event->x, event->y);
		if (slot >= 0)
			break;
		for (free_slot = 0; free_slot < SM_FINGERS; free_slot++) {
			if (touch->ids[free_slot] == 0)
				break;
		}

		if (free_slot == SM_FINGERS)
			break;
		touch->ids[free_slot] = event->id + 1;
		touch->down_x[free_slot] = (float)event->x;
		touch->down_y[free_slot] = (float)event->y;
		touch->last_x[free_slot] = (float)event->x;
		touch->last_y[free_slot] = (float)event->y;
		if (touch->fingers == 0U) {
			touch->most = 0;
			touch->moved = 0;
			touch->pinched = 0;
		}

		touch->fingers++;
		if (touch->fingers > touch->most)
			touch->most = touch->fingers;
		if (touch->fingers == 2U)
			touch->second_us = event->time_us;
		break;
	case KUI_WINDOW_TOUCH_MOTION:
		/* A finger moving: past the slop it is no tap. */
		(void)keiland_gesture_motion(touch->gesture, event->id, event->time_us, event->arrival_us, event->x, event->y);
		if (slot < 0)
			break;
		touch->last_x[slot] = (float)event->x;
		touch->last_y[slot] = (float)event->y;
		dx = (float)event->x - touch->down_x[slot];
		dy = (float)event->y - touch->down_y[slot];
		if (dx * dx + dy * dy > PRESS_SLOP * PRESS_SLOP)
			touch->moved = 1;
		break;
	case KUI_WINDOW_TOUCH_UP:
		/* A finger lifting; the last of two still ones soon after they touched is a two-finger tap. */
		(void)keiland_gesture_up(touch->gesture, event->id, event->time_us);
		if (slot < 0)
			break;
		touch->ids[slot] = 0;
		if (touch->fingers > 0U)
			touch->fingers--;
		if (touch->fingers == 0U && touch->most == 2U && !touch->moved && !touch->pinched &&
		    event->time_us < touch->second_us + TWO_TAP_US) {
			act_view(app, app->focus.plate < 0 || !app->focus.opening, touch->down_x[slot], touch->down_y[slot], "two-finger-tap");
		}

		break;
	default:
		/* The compositor took the fingers. */
		keiland_gesture_cancel(touch->gesture);
		memset(touch->ids, 0, sizeof(touch->ids));
		touch->fingers = 0;
		touch->dragging = 0;
		break;
	}

	/* The gestures want the clock soon. */
	app->dirty = 1;
}

/* The pointer: its place (the parallax), a click, a long press, a drag on the core, the wheel. */
static void
pointer_event(
	struct sm_app *app,
	const struct kui_window_event *event,
	uint64_t now_ms)
{
	struct sm_touch *touch;
	float dx;
	float dy;

	/* The pointer's place, from -1 to 1 across the window. */
	touch = &app->touch;
	if (event->kind == KUI_WINDOW_MOTION) {
		if (app->layout.width > 0.0f && app->layout.height > 0.0f) {
			app->motion.pointer_x = (float)event->x / app->layout.width * 2.0f - 1.0f;
			app->motion.pointer_y = (float)event->y / app->layout.height * 2.0f - 1.0f;
		}

		/* A press moving: past the slop no click; on the core it turns it. */
		if (touch->pressed) {
			dx = (float)event->x - touch->press_x;
			dy = (float)event->y - touch->press_y;
			if (dx * dx + dy * dy > PRESS_SLOP * PRESS_SLOP)
				touch->press_moved = 1;
			if (touch->press_core)
				touch->core_turn = fmaxf(-CORE_TURN_MAX, fminf(CORE_TURN_MAX, dx * CORE_TURN_PER_PIXEL));
		}

		app->dirty = 1;
		return;
	}

	/* The pointer leaving: the camera straight again. */
	if (event->kind == KUI_WINDOW_LEAVE) {
		app->motion.pointer_x = 0.0f;
		app->motion.pointer_y = 0.0f;
		return;
	}

	/* Shift and the wheel: the range. */
	if (event->kind == KUI_WINDOW_AXIS) {
		if ((event->modifiers & KUI_MOD_SHIFT) != 0U && event->dy != 0.0)
			act_swipe(app, -event->dy);
		return;
	}

	/* The right button: pin. */
	if (event->code == KUI_BUTTON_RIGHT) {
		if (event->pressed)
			act_long_press(app, (float)event->x, (float)event->y);
		return;
	}

	/* The left button pressed: a press begins (on the core, a turn). */
	if (event->code != KUI_BUTTON_LEFT)
		return;
	if (event->pressed) {
		touch->pressed = 1;
		touch->press_moved = 0;
		touch->press_long = 0;
		touch->press_x = (float)event->x;
		touch->press_y = (float)event->y;
		touch->press_ms = now_ms;
		touch->press_core = on_core(app, (float)event->x, (float)event->y);
		return;
	}

	/* Released: a click unless it moved or was a long press; a turn of the core ends. */
	if (touch->pressed && !touch->press_moved && !touch->press_long)
		act_tap(app, (float)event->x, (float)event->y, "click");
	if (touch->pressed && touch->press_core && touch->press_moved)
		printf("ZMON CORE turn=%.2f\n", touch->core_turn);
	touch->pressed = 0;
	touch->press_core = 0;
}

/* A key: the keyboard's plate, its card, back, pin, the range. */
static void
key_event(
	struct sm_app *app,
	const struct kui_window_event *event)
{
	unsigned count;
	unsigned index;
	int step;

	/* Only presses. */
	if (!event->pressed)
		return;

	/* Each key. */
	count = sizeof(keyboard_order) / sizeof(keyboard_order[0]);
	switch (event->code) {
	case KEY_TAB:
		/* The next plate (the one before with Shift). */
		step = 1;
		if ((event->modifiers & KUI_MOD_SHIFT) != 0U)
			step = (int)count - 1;
		index = 0;
		while (index < count && (int)keyboard_order[index] != app->focus.keyboard)
			index++;
		if (index == count)
			index = count - 1U;
		index = (index + (unsigned)step) % count;
		app->focus.keyboard = (int)keyboard_order[index];
		app->dirty = 1;
		printf("ZMON FOCUS plate=%s\n", plate_names[app->focus.keyboard]);
		break;
	case KEY_ENTER:
	case KEY_SPACE:
		/* The keyboard's plate as a card, or the card back. */
		if (app->focus.plate >= 0 && app->focus.opening)
			card_close(app, "key");
		else if (app->focus.keyboard >= 0)
			card_open(app, app->focus.keyboard, "key");
		break;
	case KEY_ESC:
		/* The card back even when pinned. */
		app->focus.pinned = 0;
		card_close(app, "key");
		break;
	case KEY_P:
		card_pin(app);
		break;
	case KEY_LEFT:
		if (app->range > 0U)
			sm_set_range(app, app->range - 1U);
		break;
	case KEY_RIGHT:
		if (app->range + 1U < SM_RANGES)
			sm_set_range(app, app->range + 1U);
		break;
	default:
		break;
	}
}

/* Takes the gestures due: taps, long presses, the drags' starts and ends. */
static void
gesture_events(
	struct sm_app *app,
	uint64_t now_us)
{
	struct keiland_gesture_event gesture;
	struct sm_touch *touch;
	float moved;
	int found;
	unsigned slot;

	/* Each gesture. */
	touch = &app->touch;
	for (;;) {
		found = keiland_gesture_next(touch->gesture, now_us, &gesture);
		if (!found)
			break;

		/* Each kind. */
		switch (gesture.kind) {
		case KEILAND_GESTURE_TAP:
			act_tap(app, (float)gesture.x, (float)gesture.y, "tap");
			break;
		case KEILAND_GESTURE_LONG_PRESS:
			act_long_press(app, (float)gesture.x, (float)gesture.y);
			break;
		case KEILAND_GESTURE_DRAG_BEGIN:
			/* One finger on the core turns it; otherwise it may be a swipe. */
			touch->dragging = 1;
			touch->drag_core = gesture.fingers == 1U && on_core(app, (float)gesture.x, (float)gesture.y);
			break;
		case KEILAND_GESTURE_DRAG_END:
			/* A swipe: one finger all along, not on the core, fast or far and mostly sideways. */
			moved = 0.0f;
			for (slot = 0; slot < SM_FINGERS; slot++) {
				if (fabsf(touch->last_x[slot] - touch->down_x[slot]) > fabsf(moved))
					moved = touch->last_x[slot] - touch->down_x[slot];
			}

			if (touch->most == 1U && !touch->drag_core) {
				if ((fabs(gesture.vx) >= SWIPE_SPEED && fabs(gesture.vx) > 2.0 * fabs(gesture.vy)) || fabsf(moved) >= SWIPE_DISTANCE)
					act_swipe(app, fabsf(moved) >= SWIPE_DISTANCE ? (double)moved : gesture.vx);
			}

			/* A turn of the core ends. */
			if (touch->drag_core)
				printf("ZMON CORE turn=%.2f\n", touch->core_turn);
			touch->dragging = 0;
			touch->drag_core = 0;
			break;
		default:
			touch->dragging = 0;
			touch->drag_core = 0;
			break;
		}
	}
}
