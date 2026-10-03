/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws127-p002: checks libkeiui's overlay scroll bar (scroll-bar.c) on the host: hidden until the content moves,
 * thin while it moves, thick near the edge, the thumb's size and place, a drag of the thumb, a page from the
 * track, and the fade.  Prints one line a check and "scroll-bar-test: PASS" or "FAIL".
 *   sh plan/ws127/tests/scroll-bar-test.sh
 */

#include <keiui.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void check(int condition, const char *what);
static int near(double value, double expected);

int
main(
	void)
{
	struct kui_scroll_bar bar;
	struct kui_scroll_bar_shape shape;
	struct kui_rect viewport;
	double offset;
	uint64_t now;
	int result;

	/* A view 400 pixels tall at (100, 50), 300 wide, over content 1600 tall. */
	memset(&bar, 0, sizeof(bar));
	viewport.x = 100;
	viewport.y = 50;
	viewport.width = 300;
	viewport.height = 400;
	now = 10000000U;

	/* 1. Nothing shows before the content moved, and a bar of content that fits never shows. */
	result = kui_scroll_bar_shape(&bar, &viewport, 1600.0, 0.0, now, &shape);
	check(result == 0, "hidden before the content moves");
	kui_scroll_bar_moved(&bar, now);
	result = kui_scroll_bar_shape(&bar, &viewport, 300.0, 0.0, now, &shape);
	check(result == 0, "no bar for content that fits");

	/* 2. Thin while it moves: at the right edge, a quarter of the track long, at the top. */
	result = kui_scroll_bar_shape(&bar, &viewport, 1600.0, 0.0, now, &shape);
	check(result == 1 && shape.thick == 0 && near(shape.thumb_width, KUI_SCROLL_BAR_THIN), "thin while the content moves");
	check(near(shape.thumb_x, 400.0 - KUI_SCROLL_BAR_GAP - KUI_SCROLL_BAR_THIN), "at the right edge");
	check(near(shape.thumb_height, (400.0 - 2.0 * KUI_SCROLL_BAR_GAP) / 4.0), "as long as the share that shows");
	check(near(shape.thumb_y, 50.0 + KUI_SCROLL_BAR_GAP), "at the top for offset 0");
	result = kui_scroll_bar_shape(&bar, &viewport, 1600.0, 1200.0, now, &shape);
	check(near(shape.thumb_y + shape.thumb_height, 450.0 - KUI_SCROLL_BAR_GAP), "at the bottom for the last offset");

	/* 3. The pointer near the edge makes it thick; away, thin again. */
	result = kui_scroll_bar_hover(&bar, &viewport, 1600.0, 395.0, 200.0, now);
	check(result == 1 && bar.near == 1, "near the edge: changed");
	kui_scroll_bar_shape(&bar, &viewport, 1600.0, 0.0, now, &shape);
	check(shape.thick == 1 && near(shape.thumb_width, KUI_SCROLL_BAR_THICK), "thick near the edge");
	check(kui_scroll_bar_busy(&bar, now + 5000000U) == 0, "a bar under the pointer does not fade");

	/* 4. A press on the thumb drags it: the thumb's middle to the view's middle is about half the scroll. */
	result = kui_scroll_bar_press(&bar, &viewport, 1600.0, 0.0, 395.0, 60.0, now, &offset);
	check(result == 1 && bar.dragging == 1 && near(offset, 0.0), "a press on the thumb starts a drag");
	result = kui_scroll_bar_drag(&bar, &viewport, 1600.0, 60.0 + (400.0 - 2.0 * KUI_SCROLL_BAR_GAP - 99.0) / 2.0, now, &offset);
	check(result == 1 && fabs(offset - 600.0) < 2.0, "the drag moves the content with the thumb");
	result = kui_scroll_bar_drag(&bar, &viewport, 1600.0, 2000.0, now, &offset);
	check(near(offset, 1200.0), "the drag stops at the end");
	result = kui_scroll_bar_release(&bar, now);
	check(result == 1 && bar.dragging == 0, "the release ends the drag");

	/* 5. A press on the track below the thumb pages down; one away from the edge is not the bar's. */
	result = kui_scroll_bar_press(&bar, &viewport, 1600.0, 0.0, 395.0, 400.0, now, &offset);
	check(result == 1 && near(offset, 360.0), "a press on the track pages towards it");
	result = kui_scroll_bar_press(&bar, &viewport, 1600.0, 0.0, 200.0, 400.0, now, &offset);
	check(result == 0, "a press away from the edge is not the bar's");

	/* 6. The pointer leaves: full for a while, then the fade, then gone. */
	result = kui_scroll_bar_leave(&bar, now);
	check(result == 1 && bar.near == 0, "leaving shrinks it");
	kui_scroll_bar_shape(&bar, &viewport, 1600.0, 0.0, now + KUI_SCROLL_BAR_SHOW_US / 2U, &shape);
	check(near(shape.alpha, 1.0) && kui_scroll_bar_busy(&bar, now + 1U) == 1, "full while it waits, busy");
	kui_scroll_bar_shape(&bar, &viewport, 1600.0, 0.0, now + KUI_SCROLL_BAR_SHOW_US + KUI_SCROLL_BAR_FADE_US / 2U, &shape);
	check(fabs(shape.alpha - 0.5) < 0.01, "half way through the fade");
	result = kui_scroll_bar_shape(&bar, &viewport, 1600.0, 0.0, now + KUI_SCROLL_BAR_SHOW_US + KUI_SCROLL_BAR_FADE_US, &shape);
	check(result == 0 && kui_scroll_bar_busy(&bar, now + KUI_SCROLL_BAR_SHOW_US + KUI_SCROLL_BAR_FADE_US) == 0, "gone after the fade");

	/* The verdict. */
	printf("scroll-bar-test: %s\n", failures == 0 ? "PASS" : "FAIL");
	if (failures != 0)
		return 1;

	/* Succeeded: every check held. */
	return 0;
}

/* Prints one check's line and counts a failure. */
static void
check(
	int condition,
	const char *what)
{
	/* The line, ok or FAIL. */
	if (condition) {
		printf("ok: %s\n", what);
	} else {
		printf("FAIL: %s\n", what);
		failures++;
	}
}

/* Tells whether a value is within a hundredth of a pixel of another. */
static int
near(
	double value,
	double expected)
{
	/* Close enough for pixels. */
	if (fabs(value - expected) < 0.01)
		return 1;

	/* Too far. */
	return 0;
}
