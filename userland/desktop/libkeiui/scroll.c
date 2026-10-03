/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The scroll of the library (ws090-p003, plan/ws090/design.md section 6):
 * one part of a window whose content is larger than the part.
 *
 * Three things move the content, one at a time: a glide (the wheel and
 * the keys), which closes on its target by e every KUI_SCROLL_GLIDE_US and
 * is a pure function of the time since it started (Text Editor's glide);
 * a finger, whose drag and fling libkeiland's scroller follows with its
 * inertia and rubber band; and a move to a place at once.  The latest one
 * takes over: a wheel turned while the content flies stops the flight, a
 * finger that touches stops a glide.
 */

#include <keiui.h>
#include <keiland.h>

#include <errno.h>
#include <math.h>
#include <string.h>

/* A glide stops within this distance of its target, in pixels. */
#define SCROLL_GLIDE_STOP	0.5

/* The bars: their thickness, their shortest length, their gap from the viewport's edges and their darkest alpha. */
#define SCROLL_BAR_WIDTH	4
#define SCROLL_BAR_MIN		24
#define SCROLL_BAR_GAP		3
#define SCROLL_BAR_ALPHA	110.0

static void scroll_bounds(struct kui_scroll *scroll);
static void scroll_clamp(struct kui_scroll *scroll);
static void scroll_place(struct kui_scroll *scroll, double x, double y, uint64_t now_us);
static double scroll_within(double value, double limit);

/*
 * Makes a scroll along some axes (KUI_SCROLL_X, KUI_SCROLL_Y), at 0, 0
 * with nothing to scroll yet.
 *
 * Returns 0, EINVAL (no axis) or ENOMEM.
 */
int
kui_scroll_init(
	struct kui_scroll *scroll,
	unsigned axes)
{
	/* Nothing moves yet. */
	memset(scroll, 0, sizeof(*scroll));

	/* At least one axis. */
	if ((axes & (KUI_SCROLL_X | KUI_SCROLL_Y)) == 0U)
		return EINVAL;
	scroll->axes = axes;

	/* The finger's scroller. */
	scroll->scroller = keiland_scroller_create();
	if (scroll->scroller == NULL)
		return ENOMEM;

	/* Succeeded: the scroll waits for its sizes. */
	return 0;
}

/*
 * Frees what a scroll holds.
 */
void
kui_scroll_release(
	struct kui_scroll *scroll)
{
	/* The scroller, when it was made. */
	if (scroll->scroller != NULL)
		keiland_scroller_destroy(scroll->scroller);
	memset(scroll, 0, sizeof(*scroll));
}

/*
 * Sets the content's size and the viewport's; a position past the new
 * ends moves back within them (unless a finger holds the content).
 */
void
kui_scroll_set_size(
	struct kui_scroll *scroll,
	double content_width,
	double content_height,
	double viewport_width,
	double viewport_height)
{
	/* The sizes. */
	scroll->content_width = content_width;
	scroll->content_height = content_height;
	scroll->viewport_width = viewport_width;
	scroll->viewport_height = viewport_height;

	/* The scroller's bounds, and the position within them. */
	scroll_bounds(scroll);
	if (!scroll->touched)
		scroll_clamp(scroll);

	/* A glide's target within the new ends too. */
	scroll->to_x = scroll_within(scroll->to_x, kui_scroll_limit_x(scroll));
	scroll->to_y = scroll_within(scroll->to_y, kui_scroll_limit_y(scroll));
}

/*
 * Turns the wheel: the content glides on by dx, dy pixels (added to a
 * glide under way), within its ends.
 */
void
kui_scroll_wheel(
	struct kui_scroll *scroll,
	double dx,
	double dy,
	uint64_t now_us)
{
	double x;
	double y;

	/* From the glide's target when one is under way, else from where the content is. */
	x = scroll->x;
	y = scroll->y;
	if (scroll->gliding) {
		x = scroll->to_x;
		y = scroll->to_y;
	}

	/* Only the axes the scroll moves along. */
	if ((scroll->axes & KUI_SCROLL_X) != 0U)
		x += dx;
	if ((scroll->axes & KUI_SCROLL_Y) != 0U)
		y += dy;

	/* A glide there. */
	kui_scroll_move_to(scroll, x, y, 1, now_us);
}

/*
 * Moves the content to a place within its ends: gliding there, or at once.
 * A finger's flight stops.
 */
void
kui_scroll_move_to(
	struct kui_scroll *scroll,
	double x,
	double y,
	int glide,
	uint64_t now_us)
{
	/* The place within the ends. */
	x = scroll_within(x, kui_scroll_limit_x(scroll));
	y = scroll_within(y, kui_scroll_limit_y(scroll));

	/* The finger no longer owns the content. */
	scroll->touched = 0;
	scroll->released = 0;

	/* At once. */
	if (!glide) {
		scroll->gliding = 0;
		scroll_place(scroll, x, y, now_us);
		return;
	}

	/* A glide from here, starting now. */
	scroll->from_x = scroll->x;
	scroll->from_y = scroll->y;
	scroll->to_x = x;
	scroll->to_y = y;
	scroll->glide_us = now_us;
	scroll->gliding = 1;
}

/*
 * Glides the content just far enough to show a rectangle of it (content
 * coordinates) whole, or its start when it is larger than the viewport.
 */
void
kui_scroll_reveal(
	struct kui_scroll *scroll,
	const struct kui_rect *rect,
	uint64_t now_us)
{
	double x;
	double y;

	/* From the glide's target when one is under way. */
	x = scroll->x;
	y = scroll->y;
	if (scroll->gliding) {
		x = scroll->to_x;
		y = scroll->to_y;
	}

	/* Across: past the right edge, then the left (which wins). */
	if ((double)(rect->x + rect->width) > x + scroll->viewport_width)
		x = (double)(rect->x + rect->width) - scroll->viewport_width;
	if ((double)rect->x < x)
		x = (double)rect->x;

	/* Down: past the bottom, then the top (which wins). */
	if ((double)(rect->y + rect->height) > y + scroll->viewport_height)
		y = (double)(rect->y + rect->height) - scroll->viewport_height;
	if ((double)rect->y < y)
		y = (double)rect->y;

	/* Nothing to move. */
	if (x == scroll->x && y == scroll->y && !scroll->gliding)
		return;

	/* A glide there. */
	kui_scroll_move_to(scroll, x, y, 1, now_us);
}

/*
 * Carries out a scrolling key: the arrows by a line, Page Up and Page Down
 * by the viewport less a line, Home and End (with Control) to the ends.
 * Returns 1 when the key was a scrolling one.
 */
int
kui_scroll_key(
	struct kui_scroll *scroll,
	uint32_t key,
	unsigned modifiers,
	double line,
	uint64_t now_us)
{
	double page;
	double x;
	double y;

	/* From the glide's target when one is under way. */
	x = scroll->x;
	y = scroll->y;
	if (scroll->gliding) {
		x = scroll->to_x;
		y = scroll->to_y;
	}

	/* A page keeps a line of what showed. */
	page = scroll->viewport_height - line;
	if (page < line)
		page = line;

	/* The key's move. */
	switch (key) {
	case KUI_KEY_UP:
		y -= line;
		break;
	case KUI_KEY_DOWN:
		y += line;
		break;
	case KUI_KEY_LEFT:
		x -= line;
		break;
	case KUI_KEY_RIGHT:
		x += line;
		break;
	case KUI_KEY_PAGEUP:
		y -= page;
		break;
	case KUI_KEY_PAGEDOWN:
	case KUI_KEY_SPACE:
		/* Space pages down, and Shift+Space up. */
		if (key == KUI_KEY_SPACE && (modifiers & KUI_MOD_SHIFT) != 0U) {
			y -= page;
			break;
		}

		/* The rest go down a page. */
		y += page;
		break;
	case KUI_KEY_HOME:
		/* Home goes to the start (with or without Control). */
		y = 0.0;
		if ((modifiers & KUI_MOD_CTRL) == 0U)
			x = 0.0;
		break;
	case KUI_KEY_END:
		y = kui_scroll_limit_y(scroll);
		break;
	default:
		return 0;
	}

	/* A glide there. */
	kui_scroll_move_to(scroll, x, y, 1, now_us);
	return 1;
}

/*
 * A finger touches the content: the glide stops and the scroller holds
 * it.  Returns 1 when the touch caught content that was flying (the touch
 * then only stops it and does not tap).
 */
int
kui_scroll_press(
	struct kui_scroll *scroll,
	uint64_t now_us)
{
	int caught;

	/* The scroller from where the content is (a glide stops there). */
	if (!scroll->touched) {
		scroll_bounds(scroll);
		keiland_scroller_set_position(scroll->scroller, scroll->x, scroll->y);
	}

	/* A glide under way ends where the content is. */
	scroll->gliding = 0;

	/* The finger holds the content. */
	caught = keiland_scroller_press(scroll->scroller, now_us);
	scroll->touched = 1;
	scroll->released = 0;

	/* Succeeded: whether a flight was caught. */
	return caught;
}

/*
 * The finger has moved by dx, dy since it touched (the total).
 */
void
kui_scroll_drag(
	struct kui_scroll *scroll,
	double dx,
	double dy)
{
	/* Only a held content follows a finger. */
	if (!scroll->touched)
		return;

	/* The scroller moves the content the other way (a finger moving down shows what is above). */
	keiland_scroller_drag(scroll->scroller, dx, dy);
}

/*
 * The finger lifts with a velocity (pixels a second, as the finger moved):
 * the content flies on, or settles within its ends.
 */
void
kui_scroll_fling(
	struct kui_scroll *scroll,
	double vx,
	double vy,
	uint64_t now_us)
{
	/* Only a held content flies. */
	if (!scroll->touched)
		return;

	/* The scroller takes the velocity; the content is the scroller's until it rests. */
	keiland_scroller_release(scroll->scroller, now_us, vx, vy);
	scroll->released = 1;
}

/*
 * The fingers were taken away: no flight, and content past an end springs
 * back.
 */
void
kui_scroll_cancel(
	struct kui_scroll *scroll,
	uint64_t now_us)
{
	/* Only a held content. */
	if (!scroll->touched)
		return;

	/* The scroller springs back. */
	keiland_scroller_cancel(scroll->scroller, now_us);
	scroll->released = 1;
}

/*
 * Moves time on: the position at a time, from a finger's scroller or a
 * glide.  Returns 1 while the content moves by itself (the window should
 * draw the next frame).
 */
int
kui_scroll_step(
	struct kui_scroll *scroll,
	uint64_t now_us)
{
	double share;
	double left_x;
	double left_y;
	double x;
	double y;
	int moving;

	/* A finger's content is where the scroller has it. */
	if (scroll->touched) {
		moving = keiland_scroller_step(scroll->scroller, now_us, &x, &y);
		scroll_place(scroll, x, y, now_us);

		/* At rest after the finger lifted: the content is the scroll's again. */
		if (!moving && scroll->released) {
			scroll->touched = 0;
			scroll->released = 0;
			scroll_clamp(scroll);
		}

		/* Reports whether it flies on (a held content moves with the finger's input, not by itself). */
		return moving;
	}

	/* No glide: nothing moves. */
	if (!scroll->gliding)
		return 0;

	/* The glide's share done by now: 1 - e^(-t / time constant). */
	share = 1.0;
	if (now_us > scroll->glide_us)
		share = 1.0 - exp(-(double)(now_us - scroll->glide_us) / (double)KUI_SCROLL_GLIDE_US);
	if (now_us <= scroll->glide_us)
		share = 0.0;
	x = scroll->from_x + (scroll->to_x - scroll->from_x) * share;
	y = scroll->from_y + (scroll->to_y - scroll->from_y) * share;

	/* Within half a pixel on both axes: there, and the glide ends. */
	left_x = fabs(scroll->to_x - x);
	left_y = fabs(scroll->to_y - y);
	if (left_x < SCROLL_GLIDE_STOP && left_y < SCROLL_GLIDE_STOP) {
		scroll->gliding = 0;
		scroll_place(scroll, scroll->to_x, scroll->to_y, now_us);
		return 0;
	}

	/* Succeeded: under way. */
	scroll_place(scroll, x, y, now_us);
	return 1;
}

/*
 * Reports how far the content scrolls across (0 when it fits or does not
 * scroll across).
 */
double
kui_scroll_limit_x(
	const struct kui_scroll *scroll)
{
	/* An axis the scroll does not move along. */
	if ((scroll->axes & KUI_SCROLL_X) == 0U)
		return 0.0;

	/* Content that fits. */
	if (scroll->content_width <= scroll->viewport_width)
		return 0.0;

	/* Reports what does not show. */
	return scroll->content_width - scroll->viewport_width;
}

/*
 * Reports how far the content scrolls down (0 when it fits or does not
 * scroll down).
 */
double
kui_scroll_limit_y(
	const struct kui_scroll *scroll)
{
	/* An axis the scroll does not move along. */
	if ((scroll->axes & KUI_SCROLL_Y) == 0U)
		return 0.0;

	/* Content that fits. */
	if (scroll->content_height <= scroll->viewport_height)
		return 0.0;

	/* Reports what does not show. */
	return scroll->content_height - scroll->viewport_height;
}

/*
 * Draws the scroll bars inside a viewport while the content moves and as
 * they fade after it stops.  Returns 1 while they fade (the window should
 * draw again).
 */
int
kui_scroll_draw_bars(
	const struct kui_scroll *scroll,
	struct kui_canvas *canvas,
	const struct kui_rect *viewport,
	const struct kui_theme *theme,
	uint64_t now_us)
{
	double since;
	double alpha;
	double length;
	double place;
	double limit;
	kui_color color;

	/* Never moved, or long enough ago: no bars. */
	if (scroll->moved_us == 0U || now_us < scroll->moved_us)
		return 0;
	since = (double)(now_us - scroll->moved_us);
	if (since >= (double)KUI_SCROLL_FADE_US)
		return 0;

	/* The ink, fading over the last half of the time. */
	alpha = SCROLL_BAR_ALPHA;
	if (since > (double)KUI_SCROLL_FADE_US / 2.0)
		alpha *= 1.0 - (since - (double)KUI_SCROLL_FADE_US / 2.0) / ((double)KUI_SCROLL_FADE_US / 2.0);
	color = (theme->icon & 0x00ffffffU) | ((uint32_t)alpha << 24);

	/* The vertical bar, when the content is taller than the viewport. */
	limit = kui_scroll_limit_y(scroll);
	if (limit > 0.0) {
		length = (double)viewport->height * scroll->viewport_height / scroll->content_height;
		if (length < (double)SCROLL_BAR_MIN)
			length = (double)SCROLL_BAR_MIN;
		place = scroll_within(scroll->y, limit) / limit * ((double)viewport->height - length - 2.0 * SCROLL_BAR_GAP);
		kui_canvas_round(canvas, (float)(viewport->x + viewport->width - SCROLL_BAR_WIDTH - SCROLL_BAR_GAP),
				 (float)((double)viewport->y + SCROLL_BAR_GAP + place),
				 (float)SCROLL_BAR_WIDTH,
				 (float)length,
				 (float)SCROLL_BAR_WIDTH / 2.0f,
				 color);
	}

	/* The horizontal bar, when the content is wider. */
	limit = kui_scroll_limit_x(scroll);
	if (limit > 0.0) {
		length = (double)viewport->width * scroll->viewport_width / scroll->content_width;
		if (length < (double)SCROLL_BAR_MIN)
			length = (double)SCROLL_BAR_MIN;
		place = scroll_within(scroll->x, limit) / limit * ((double)viewport->width - length - 2.0 * SCROLL_BAR_GAP);
		kui_canvas_round(canvas, (float)((double)viewport->x + SCROLL_BAR_GAP + place),
				 (float)(viewport->y + viewport->height - SCROLL_BAR_WIDTH - SCROLL_BAR_GAP),
				 (float)length,
				 (float)SCROLL_BAR_WIDTH,
				 (float)SCROLL_BAR_WIDTH / 2.0f,
				 color);
	}

	/* Succeeded: the bars fade on. */
	return 1;
}

/* Gives the scroller the ends and the viewport (it needs a viewport above zero). */
static void
scroll_bounds(
	struct kui_scroll *scroll)
{
	double width;
	double height;

	/* A viewport of at least a pixel. */
	width = scroll->viewport_width;
	if (width < 1.0)
		width = 1.0;
	height = scroll->viewport_height;
	if (height < 1.0)
		height = 1.0;

	/* The ends of each axis (an axis that does not move has none). */
	(void)keiland_scroller_set_bounds(scroll->scroller, 0.0, kui_scroll_limit_x(scroll), 0.0, kui_scroll_limit_y(scroll), width, height);
}

/* Keeps the position within the ends. */
static void
scroll_clamp(
	struct kui_scroll *scroll)
{
	/* Each axis within its end. */
	scroll->x = scroll_within(scroll->x, kui_scroll_limit_x(scroll));
	scroll->y = scroll_within(scroll->y, kui_scroll_limit_y(scroll));
}

/* Puts the content at a place, noting when it moved. */
static void
scroll_place(
	struct kui_scroll *scroll,
	double x,
	double y,
	uint64_t now_us)
{
	/* The same place: nothing moved. */
	if (x == scroll->x && y == scroll->y)
		return;

	/* The place, and the time the bars show from. */
	scroll->x = x;
	scroll->y = y;
	scroll->moved_us = now_us;
	if (scroll->moved_us == 0U)
		scroll->moved_us = 1U;
}

/* Reports a value within 0..limit. */
static double
scroll_within(
	double value,
	double limit)
{
	/* Below the start. */
	if (value < 0.0)
		return 0.0;

	/* Past the end. */
	if (value > limit)
		return limit;

	/* Within. */
	return value;
}

/*
 * Draws an overlay scroll bar (scroll-bar.c) on a canvas: the faint track
 * while the bar is thick, and the thumb.  Returns 1 while the bar still
 * changes with time (the window should draw again), 0 otherwise.
 */
int
kui_scroll_bar_draw(
	const struct kui_scroll_bar *bar,
	struct kui_canvas *canvas,
	const struct kui_rect *viewport,
	double content,
	double offset,
	uint64_t now_us)
{
	struct kui_scroll_bar_shape shape;
	kui_color track;
	kui_color thumb;
	int shown;
	int busy;

	/* What shows now; nothing is drawn of a bar that does not. */
	shown = kui_scroll_bar_shape(bar, viewport, content, offset, now_us, &shape);
	busy = kui_scroll_bar_busy(bar, now_us);
	if (shown == 0)
		return busy;

	/* The track, light and faint, only while the bar is thick. */
	if (shape.thick != 0) {
		track = 0x00f4f4f4U | ((uint32_t)(150.0 * shape.alpha) << 24);
		kui_canvas_round(canvas, (float)shape.track_x, (float)shape.track_y, (float)shape.track_width,
				 (float)shape.track_height, (float)shape.track_width / 2.0f, track);
	}

	/* The thumb, a dark grey rounded at its ends. */
	thumb = 0x00303030U | ((uint32_t)(SCROLL_BAR_ALPHA * shape.alpha) << 24);
	kui_canvas_round(canvas, (float)shape.thumb_x, (float)shape.thumb_y, (float)shape.thumb_width,
			 (float)shape.thumb_height, (float)shape.thumb_width / 2.0f, thumb);

	/* Succeeded: reports whether it changes on. */
	return busy;
}
