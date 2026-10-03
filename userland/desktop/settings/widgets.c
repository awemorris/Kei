/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The parts Settings' pages are built of: the page's header, a card with
 * its title, a row of a label and its value, and the Kei mark.
 *
 * Each part is drawn from a top edge within a column and returns the edge
 * below it, so that a page lays its parts out from the top down.
 */

#include "settings.h"

#include "../artwork/mark.h"

#include <stdio.h>
#include <string.h>

/* The header's text sizes: the page's name and its summary. */
#define WIDGETS_TEXT_TITLE	30U
#define WIDGETS_TEXT_SUMMARY	15U

/* A card's corners, its inner margin, its title's and subtitle's sizes, and the space its title takes. */
#define WIDGETS_CARD_RADIUS	14.0f
#define WIDGETS_CARD_PAD	18
#define WIDGETS_TEXT_CARD	16U
#define WIDGETS_TEXT_CARD_SUB	13U
#define WIDGETS_CARD_TITLE	46

/* A row's height and text size, and the share of the row its label takes. */
#define WIDGETS_ROW_HEIGHT	40
#define WIDGETS_TEXT_ROW	14U
#define WIDGETS_LABEL_SHARE	0.34f

/* The largest Kei mark drawn, in pixels a side (its layers are kept rendered at the last size). */
#define WIDGETS_MARK_MAX	160U

/* A switch's size. */
#define WIDGETS_TOGGLE_WIDTH	44
#define WIDGETS_TOGGLE_HEIGHT	24

/* A slider's height (the knob's room). */
#define WIDGETS_SLIDER_HEIGHT	28

/* A button's height, its side margins and its text size. */
#define WIDGETS_BUTTON_HEIGHT	32
#define WIDGETS_BUTTON_SIDE	16
#define WIDGETS_TEXT_BUTTON	14U

/* The keys of a US keyboard by their evdev codes (from code 2), without and with Shift; 0 is a key that types nothing. */
static const char widgets_keys[] = "1234567890-=\0\0qwertyuiop[]\0\0asdfghjkl;'`\0\\zxcvbnm,./";
static const char widgets_shifted[] = "!@#$%^&*()_+\0\0QWERTYUIOP{}\0\0ASDFGHJKL:\"~\0|ZXCVBNM<>?";

/* The first evdev code the tables above start from. */
#define WIDGETS_KEY_FIRST	2U

/*
 * Draws a page's header: its name large and its summary under it.
 * Returns the edge below the header.
 */
int
se_page_header(
	struct se_app *app,
	struct fm_canvas *canvas,
	const struct se_page *page,
	int x,
	int top,
	int width)
{
	struct fm_text_line title;
	struct fm_text_line summary;
	int baseline;

	/* The two lines' measurements. */
	fm_text_metrics(app->text, WIDGETS_TEXT_TITLE, &title);
	fm_text_metrics(app->text, WIDGETS_TEXT_SUMMARY, &summary);

	/* The name, bold. */
	baseline = top + title.ascent;
	(void)fm_text_draw_fit(app->text, canvas, x, baseline, page->name, WIDGETS_TEXT_TITLE, 1, width, SE_COLOR_TEXT);

	/* The summary under it. */
	baseline = top + title.height + 2 + summary.ascent;
	(void)fm_text_draw_fit(app->text, canvas, x, baseline, page->summary, WIDGETS_TEXT_SUMMARY, 0, width, SE_COLOR_TEXT_SECONDARY);

	/* The edge below the summary. */
	return top + title.height + 2 + summary.height;
}

/*
 * Draws a card's ground and its title (and subtitle, when one is given)
 * in a rectangle.  Returns the edge where the card's content starts.
 */
int
se_card_begin(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width,
	int height,
	const char *title,
	const char *subtitle)
{
	struct fm_text_line line;
	int baseline;

	/* The card: a whiter veil with a bright edge. */
	fm_canvas_round(canvas, (float)x, (float)top, (float)width, (float)height, WIDGETS_CARD_RADIUS, SE_COLOR_CARD);
	fm_canvas_round_border(canvas, (float)x, (float)top, (float)width, (float)height, WIDGETS_CARD_RADIUS, 1.0f, SE_COLOR_CARD_EDGE);

	/* A card without a title starts at its margin. */
	if (title == NULL)
		return top + WIDGETS_CARD_PAD;

	/* The title, bold. */
	fm_text_metrics(app->text, WIDGETS_TEXT_CARD, &line);
	baseline = top + WIDGETS_CARD_PAD + line.ascent;
	(void)fm_text_draw_fit(app->text, canvas, x + WIDGETS_CARD_PAD + 2, baseline, title, WIDGETS_TEXT_CARD, 1, width - 2 * WIDGETS_CARD_PAD, SE_COLOR_TEXT);

	/* The subtitle under it, when there is one. */
	if (subtitle != NULL) {
		baseline += line.descent + 4;
		fm_text_metrics(app->text, WIDGETS_TEXT_CARD_SUB, &line);
		baseline += line.ascent;
		(void)fm_text_draw_fit(app->text, canvas, x + WIDGETS_CARD_PAD + 2, baseline, subtitle, WIDGETS_TEXT_CARD_SUB, 0, width - 2 * WIDGETS_CARD_PAD, SE_COLOR_TEXT_SECONDARY);
		return baseline + line.descent + 10;
	}

	/* The content starts under the title. */
	return top + WIDGETS_CARD_TITLE;
}

/*
 * Reports how tall a card is that holds some rows of values, with or
 * without a title (no subtitle).
 */
int
se_card_height(
	int rows,
	int titled)
{
	int height;

	/* The margins and the rows. */
	height = 2 * WIDGETS_CARD_PAD + rows * WIDGETS_ROW_HEIGHT;

	/* The title's room. */
	if (titled != 0)
		height += WIDGETS_CARD_TITLE - WIDGETS_CARD_PAD;

	/* The card's height. */
	return height;
}

/*
 * Draws one row of a card: a label and its value beside it, with a thin
 * line under the row unless it is the card's last.  x and width are the
 * card's.  Returns the edge below the row.
 */
int
se_row_value(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width,
	const char *label,
	const char *value,
	int last)
{
	int baseline;
	int label_width;
	int left;
	int right;

	/* The row's text line, and the columns of the label and the value. */
	baseline = fm_text_center(WIDGETS_TEXT_ROW, top, WIDGETS_ROW_HEIGHT);
	left = x + WIDGETS_CARD_PAD + 2;
	right = x + width - WIDGETS_CARD_PAD;
	label_width = (int)((float)(right - left) * WIDGETS_LABEL_SHARE);

	/* The label, quiet, and the value, plain. */
	(void)fm_text_draw_fit(app->text, canvas, left, baseline, label, WIDGETS_TEXT_ROW, 0, label_width - 12, SE_COLOR_TEXT_SECONDARY);
	(void)fm_text_draw_fit(app->text, canvas, left + label_width, baseline, value, WIDGETS_TEXT_ROW, 0, right - left - label_width, SE_COLOR_TEXT);

	/* The line under the row, unless it is the last. */
	if (last == 0)
		fm_canvas_line(canvas, (float)left, (float)(top + WIDGETS_ROW_HEIGHT) - 0.5f, (float)right, (float)(top + WIDGETS_ROW_HEIGHT) - 0.5f, 1.0f, SE_COLOR_SEPARATOR);

	/* The edge below the row. */
	return top + WIDGETS_ROW_HEIGHT;
}

/*
 * Draws the Kei mark in a square of a size in pixels at (x, y), as opaque
 * as asked (0..1): its seven layers (userland/desktop/artwork/mark.c), each
 * in its colour, as the file manager draws it (ws035-p109).
 */
void
se_mark_draw(
	struct fm_canvas *canvas,
	int x,
	int y,
	unsigned pixels,
	float opacity)
{
	/*
	 * The bar pale, its shade deeper, the leaf clearer and its shade the
	 * deep blue of the splash, the overlap deeper still, then the white
	 * light along the edges and the sheen (colour, alpha).  The panes are
	 * translucent, so what is behind shows through.
	 */
	static const uint32_t colours[KEILAND_MARK_LAYERS][2] = {
		{ 0xa9c3f6U, 175U },
		{ 0x7fa2f0U, 90U },
		{ 0xa3d8faU, 170U },
		{ 0x3a86f5U, 170U },
		{ 0x2f7cf3U, 200U },
		{ 0xffffffU, 170U },
		{ 0xffffffU, 60U }
	};

	/*
	 * The layers at the size last drawn (zero before the first); a new size
	 * renders them again.  They live for the program's life.
	 */
	static uint8_t layers[KEILAND_MARK_LAYERS][WIDGETS_MARK_MAX * WIDGETS_MARK_MAX];
	static unsigned layers_pixels;
	uint32_t alpha;
	unsigned layer;

	/* A mark larger than the kept layers is drawn at their largest; an empty one not at all. */
	if (pixels > WIDGETS_MARK_MAX)
		pixels = WIDGETS_MARK_MAX;
	if (pixels == 0U)
		return;

	/* The layers at this size. */
	if (layers_pixels != pixels) {
		for (layer = 0; layer < KEILAND_MARK_LAYERS; layer++)
			keiland_mark_raster(layer, pixels, layers[layer], pixels);
		layers_pixels = pixels;
	}

	/* Each layer in its colour, as opaque as asked. */
	for (layer = 0; layer < KEILAND_MARK_LAYERS; layer++) {
		alpha = (uint32_t)((float)colours[layer][1] * opacity + 0.5f);
		fm_canvas_mask(canvas, x, y, layers[layer], (int)pixels, (int)pixels, pixels, FM_RGBA(colours[layer][0], alpha));
	}
}

/*
 * Draws a switch with its top left at (x, y): the accent with the knob on
 * the right when on, grey with the knob on the left when off, faded when
 * it does nothing.  An enabled switch is a page's control (index).
 */
void
se_toggle_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int y,
	int on,
	int enabled,
	int index)
{
	struct fm_rect rect;
	fm_color track;
	fm_color knob;
	float knob_x;

	/* The track's colour and the knob's place. */
	track = FM_RGB(0xc9d1dc);
	knob_x = (float)x + 12.0f;
	if (on != 0) {
		track = SE_COLOR_ACCENT;
		knob_x = (float)(x + WIDGETS_TOGGLE_WIDTH) - 12.0f;
	}

	/* The knob is white. */
	knob = FM_RGB(0xffffff);

	/* A switch that does nothing is faded. */
	if (enabled == 0) {
		track = fm_color_mix(track, FM_RGB(0xeef1f5), 0.6f);
		knob = FM_RGB(0xf6f7f9);
	}

	/* The track and the knob. */
	fm_canvas_round(canvas, (float)x, (float)y, (float)WIDGETS_TOGGLE_WIDTH, (float)WIDGETS_TOGGLE_HEIGHT, (float)WIDGETS_TOGGLE_HEIGHT * 0.5f, track);
	fm_canvas_circle(canvas, knob_x, (float)y + (float)WIDGETS_TOGGLE_HEIGHT * 0.5f, 9.5f, knob);

	/* An enabled switch is clickable. */
	if (enabled != 0) {
		rect.x = x - 4;
		rect.y = y - 4;
		rect.width = WIDGETS_TOGGLE_WIDTH + 8;
		rect.height = WIDGETS_TOGGLE_HEIGHT + 8;
		se_ui_hit(app, &rect, SE_HIT_CONTROL, index);
	}
}

/*
 * Draws a slider from (x, y), width long: a track filled in the accent up
 * to a fraction (0..1) and a white knob there, faded when it does nothing.
 * An enabled slider is a page's control (index) whose rectangle is given
 * back for a drag (se_slider_fraction).
 */
void
se_slider_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int y,
	int width,
	float fraction,
	int enabled,
	int index,
	struct fm_rect *rect)
{
	fm_color fill;
	fm_color knob;
	fm_color ring;
	float knob_x;
	float middle;

	/* Within the track. */
	if (fraction < 0.0f)
		fraction = 0.0f;
	if (fraction > 1.0f)
		fraction = 1.0f;

	/* The track, then the part up to the knob in the accent (faded when it does nothing). */
	middle = (float)y + (float)WIDGETS_SLIDER_HEIGHT * 0.5f;
	knob_x = (float)x + fraction * (float)width;
	fill = SE_COLOR_ACCENT;
	if (enabled == 0)
		fill = fm_color_mix(SE_COLOR_ACCENT, FM_RGB(0xeef1f5), 0.6f);
	fm_canvas_round(canvas, (float)x, middle - 3.0f, (float)width, 6.0f, 3.0f, FM_RGB(0xd3d9e2));
	fm_canvas_round(canvas, (float)x, middle - 3.0f, knob_x - (float)x, 6.0f, 3.0f, fill);

	/* The knob, with a quiet ring round it; greyed when the slider does nothing (ws089-p012 C4), so it does not look as if it could be dragged. */
	knob = FM_RGB(0xffffff);
	ring = FM_RGBA(0x5a6b85, 50);
	if (enabled == 0) {
		knob = FM_RGB(0xeef1f5);
		ring = FM_RGBA(0x5a6b85, 24);
	}

	/* The ring, then the knob in it. */
	fm_canvas_circle(canvas, knob_x, middle, 11.0f, ring);
	fm_canvas_circle(canvas, knob_x, middle, 10.0f, knob);

	/* The whole track takes presses and drags (a little taller than it looks). */
	rect->x = x - 12;
	rect->y = y;
	rect->width = width + 24;
	rect->height = WIDGETS_SLIDER_HEIGHT;
	if (enabled != 0)
		se_ui_hit(app, rect, SE_HIT_CONTROL, index);
}

/*
 * Reports where along a slider a pointer at x is, from 0 (its left end)
 * to 1 (its right end).
 */
float
se_slider_fraction(
	const struct fm_rect *rect,
	int x)
{
	float fraction;

	/* The track lies 12 pixels inside the rectangle at each end. */
	fraction = (float)(x - rect->x - 12) / (float)(rect->width - 24);

	/* Before the left end. */
	if (fraction < 0.0f)
		return 0.0f;

	/* After the right end. */
	if (fraction > 1.0f)
		return 1.0f;

	/* On the track. */
	return fraction;
}

/*
 * Reports how wide a button with a label is.
 */
int
se_button_width(
	struct se_app *app,
	const char *label)
{
	int text;

	/* The label and the margins. */
	text = fm_text_width(app->text, label, strlen(label), WIDGETS_TEXT_BUTTON, 1);

	/* The button's width. */
	return text + 2 * WIDGETS_BUTTON_SIDE;
}

/*
 * Draws a button with its top left at (x, y): the accent with white text
 * when primary, else a white one with an edge; darker under the pointer,
 * faded when it does nothing.  An enabled button is a page's control
 * (index).  Returns its width.
 */
int
se_button_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int y,
	const char *label,
	int primary,
	int enabled,
	int index)
{
	struct fm_rect rect;
	fm_color ground;
	fm_color edge;
	fm_color ink;
	int width;
	int lit;

	/* The button's size. */
	width = se_button_width(app, label);
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = WIDGETS_BUTTON_HEIGHT;

	/* Its colours: the accent for the primary one, white for the others. */
	ground = FM_RGBA(0xffffff, 225);
	edge = FM_RGBA(0x8a96aa, 70);
	ink = SE_COLOR_TEXT;
	if (primary != 0) {
		ground = SE_COLOR_ACCENT;
		edge = SE_COLOR_ACCENT;
		ink = FM_RGB(0xffffff);
	}

	/* Darker under the pointer, faded when it does nothing. */
	lit = se_ui_lit(app, SE_HIT_CONTROL, index);
	if (enabled != 0 && lit != 0)
		ground = fm_color_mix(ground, FM_RGB(0x1e2632), 0.08f);
	if (enabled == 0) {
		ground = fm_color_mix(ground, FM_RGB(0xeef1f5), 0.6f);
		ink = SE_COLOR_TEXT_FAINT;
	}

	/* The button and its label, centred. */
	fm_canvas_round(canvas, (float)x, (float)y, (float)width, (float)WIDGETS_BUTTON_HEIGHT, 8.0f, ground);
	fm_canvas_round_border(canvas, (float)x, (float)y, (float)width, (float)WIDGETS_BUTTON_HEIGHT, 8.0f, 1.0f, edge);
	(void)fm_text_draw(app->text, canvas, x + WIDGETS_BUTTON_SIDE, fm_text_center(WIDGETS_TEXT_BUTTON, y, WIDGETS_BUTTON_HEIGHT), label, strlen(label), WIDGETS_TEXT_BUTTON, 1, ink);

	/* An enabled button is clickable. */
	if (enabled != 0)
		se_ui_hit(app, &rect, SE_HIT_CONTROL, index);

	/* The button's width. */
	return width;
}

/*
 * Draws a status dot (green for connected, grey for not) centred at (cx, cy).
 */
void
se_dot_draw(
	struct fm_canvas *canvas,
	float cx,
	float cy,
	fm_color color)
{
	/* A small filled circle. */
	fm_canvas_circle(canvas, cx, cy, 4.5f, color);
}

/*
 * Draws a signal's four bars with their bottom left at (x, y): as many
 * full as the strength (dBm) earns, the rest faint.
 */
void
se_signal_draw(
	struct fm_canvas *canvas,
	float x,
	float y,
	int rssi,
	fm_color color)
{
	fm_color bar;
	float height;
	int bars;
	int index;

	/* The bars the strength earns: four above -55 dBm, one below -75. */
	bars = 1;
	if (rssi > -75)
		bars = 2;
	if (rssi > -65)
		bars = 3;
	if (rssi > -55)
		bars = 4;

	/* Each bar, taller to the right. */
	for (index = 0; index < 4; index++) {
		bar = color;
		if (index >= bars)
			bar = FM_RGBA(0x8a96aa, 90);
		height = 4.0f + 3.5f * (float)index;
		fm_canvas_round(canvas, x + 5.0f * (float)index, y - height, 3.0f, height, 1.0f, bar);
	}
}

/*
 * Writes a number of bytes as text in the largest unit under a thousand
 * of it (B, KB, MB, GB).
 */
void
se_bytes_text(
	uint64_t bytes,
	char *text,
	size_t size)
{
	static const char *const units[] = { "KB", "MB", "GB", "TB" };
	double value;
	unsigned unit;

	/* Under a kilobyte, whole bytes. */
	if (bytes < 1000U) {
		(void)snprintf(text, size, "%u B", (unsigned)bytes);
		return;
	}

	/* The largest unit the value is at least one of. */
	value = (double)bytes / 1000.0;
	unit = 0;
	while (value >= 1000.0 && unit + 1U < sizeof(units) / sizeof(units[0])) {
		value /= 1000.0;
		unit++;
	}

	/* One decimal under ten, none above. */
	if (value < 10.0) {
		(void)snprintf(text, size, "%.1f %s", value, units[unit]);
	} else {
		(void)snprintf(text, size, "%.0f %s", value, units[unit]);
	}
}

/*
 * Types a key press into a text field: a character of a US keyboard
 * (Shift held for the other of the key), or Backspace.  Returns 1 when the
 * field used the key.
 */
int
se_field_key(
	struct se_field *field,
	const struct se_event *event)
{
	unsigned offset;
	char typed;

	/* A key with Ctrl, Alt or Super held is a shortcut, not a character. */
	if ((event->modifiers & (SE_MOD_CTRL | SE_MOD_ALT | SE_MOD_SUPER)) != 0U)
		return 0;

	/* Backspace takes the last character away. */
	if (event->key == SE_KEY_BACKSPACE) {
		if (field->length > 0) {
			field->length--;
			field->text[field->length] = '\0';
		}

		/* The field used the key, also when it was empty. */
		return 1;
	}

	/* The space. */
	typed = '\0';
	if (event->key == SE_KEY_SPACE)
		typed = ' ';

	/* The key's character, shifted when Shift is held. */
	if (event->key >= WIDGETS_KEY_FIRST && event->key < WIDGETS_KEY_FIRST + sizeof(widgets_keys) - 1U) {
		offset = event->key - WIDGETS_KEY_FIRST;
		typed = widgets_keys[offset];
		if ((event->modifiers & SE_MOD_SHIFT) != 0U)
			typed = widgets_shifted[offset];
	}

	/* A key that types nothing is not the field's. */
	if (typed == '\0')
		return 0;

	/* A full field keeps what it has. */
	if (field->length + 1U >= sizeof(field->text))
		return 1;

	/* The character at the end. */
	field->text[field->length] = typed;
	field->length++;
	field->text[field->length] = '\0';

	/* The field used the key. */
	return 1;
}

/*
 * Empties a text field, wiping what was typed (a key's text).
 */
void
se_field_clear(
	struct se_field *field)
{
	volatile char *byte;
	size_t index;

	/* Every byte, through a volatile pointer so that the wipe stays. */
	byte = field->text;
	for (index = 0; index < sizeof(field->text); index++)
		byte[index] = '\0';

	/* Nothing typed. */
	field->length = 0;
}
