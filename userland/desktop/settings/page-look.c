/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The pages of the desktop's look and of the machine's screen and disks
 * (ws089-p004):
 *
 *   Appearance  the windows' transparency, a slider saved when let go;
 *   Wallpaper   the pictures, the default first, one click to choose;
 *   Display     the screen's mode, read only (changing it comes later);
 *   Storage     each file system's use.
 *
 * What is saved goes into the user's preferences (look.c), which zdesktop
 * follows within a second.
 */

#include "settings.h"

#include <stdio.h>
#include <string.h>

/* The controls of the look's pages (hit indices); a picture is its index past LOOK_PICTURE_FIRST. */
#define LOOK_OPACITY		1
#define LOOK_PICTURE_FIRST	100

/* The space between two cards, a card's inner margin, and the text sizes. */
#define LOOK_GAP		16
#define LOOK_PAD		18
#define LOOK_TEXT_TITLE		15U
#define LOOK_TEXT_SMALL		13U

/* The opacity's range, in percent. */
#define LOOK_OPACITY_MIN	85
#define LOOK_OPACITY_MAX	100

/* A picture's tile: its narrowest width, the picture's proportions, and the room of its name. */
#define LOOK_TILE_WIDTH		200
#define LOOK_TILE_GAP		16
#define LOOK_TILE_NAME		30

/* A file system's bar. */
#define LOOK_BAR_HEIGHT		10

static int look_note(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width, const char *text);
static int look_message(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
static void look_tile(struct se_app *app, struct fm_canvas *canvas, unsigned index, int x, int y, int width, int height);
static int look_volume(struct se_app *app, struct fm_canvas *canvas, const struct se_volume *volume, int x, int top, int width);

/*
 * Draws the Appearance page: the windows' transparency, and what comes
 * later.  Returns the edge below it.
 */
int
se_appearance_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width)
{
	struct fm_text_line line;
	char value[32];
	float fraction;
	int enabled;
	int height;
	int card;
	int y;
	int value_width;

	/* A message about saving first, when there is one. */
	card = look_message(app, canvas, x, top, width);

	/* The card of the windows: a title, a line, and the slider with its value. */
	height = 150;
	y = se_card_begin(app, canvas, x, card, width, height, "Windows", "How much of the desktop shows through the windows.");
	fm_text_metrics(app->text, LOOK_TEXT_TITLE, &line);
	(void)fm_text_draw_fit(app->text, canvas, x + LOOK_PAD + 2, y + line.ascent, "Window opacity", LOOK_TEXT_TITLE, 1, width / 2, SE_COLOR_TEXT);

	/* The value at the right, as the slider shows it. */
	if (app->look.opacity >= LOOK_OPACITY_MAX) {
		(void)snprintf(value, sizeof(value), "%s", "Opaque");
	} else {
		(void)snprintf(value, sizeof(value), "%d%%", app->look.opacity);
	}

	/* Drawn against the card's right margin. */
	value_width = fm_text_width(app->text, value, strlen(value), LOOK_TEXT_TITLE, 0);
	(void)fm_text_draw(app->text, canvas, x + width - LOOK_PAD - value_width, y + line.ascent, value, strlen(value), LOOK_TEXT_TITLE, 0, SE_COLOR_TEXT_SECONDARY);

	/* The slider: see-through at the left, opaque at the right; it works only when the preferences can be saved. */
	fraction = (float)(app->look.opacity - LOOK_OPACITY_MIN) / (float)(LOOK_OPACITY_MAX - LOOK_OPACITY_MIN);
	enabled = 0;
	if (app->look.preferences != NULL)
		enabled = 1;
	se_slider_draw(app, canvas, x + LOOK_PAD + 14, y + 26, width - 2 * LOOK_PAD - 28, fraction, enabled, LOOK_OPACITY, &app->look.slider);

	/* The ends' words under the slider. */
	fm_text_metrics(app->text, LOOK_TEXT_SMALL, &line);
	(void)fm_text_draw(app->text, canvas, x + LOOK_PAD + 2, y + 66 + line.ascent, "See-through", strlen("See-through"), LOOK_TEXT_SMALL, 0, SE_COLOR_TEXT_FAINT);
	value_width = fm_text_width(app->text, "Opaque", strlen("Opaque"), LOOK_TEXT_SMALL, 0);
	(void)fm_text_draw(app->text, canvas, x + width - LOOK_PAD - value_width, y + 66 + line.ascent, "Opaque", strlen("Opaque"), LOOK_TEXT_SMALL, 0, SE_COLOR_TEXT_FAINT);

	/* What comes later, under the card. */
	y = look_note(app, canvas, x, card + height + LOOK_GAP, width, "Accent colours and a dark look are coming in a later version of Kei.");

	/* The edge below the cards. */
	return y;
}

/*
 * Draws the Wallpaper page: the pictures as tiles, the one shown ringed in
 * the accent.  Returns the edge below it.
 */
int
se_wallpaper_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width)
{
	unsigned index;
	int columns;
	int column;
	int tile;
	int picture;
	int y;

	/* The pictures, found the first time the page is shown. */
	se_look_scan(app);

	/* A message about saving first, when there is one. */
	y = look_message(app, canvas, x, top, width);

	/* How many tiles a row holds, their width, and the pictures' height (16:10). */
	columns = (width + LOOK_TILE_GAP) / (LOOK_TILE_WIDTH + LOOK_TILE_GAP);
	if (columns < 1)
		columns = 1;
	tile = (width - (columns - 1) * LOOK_TILE_GAP) / columns;
	picture = tile * 10 / 16;

	/* Each picture, a new row when one fills. */
	column = 0;
	for (index = 0; index < app->look.wallpaper_count; index++) {
		/* A full row moves down one. */
		if (column == columns) {
			column = 0;
			y += picture + LOOK_TILE_NAME + LOOK_TILE_GAP;
		}

		/* The tile in the next column. */
		look_tile(app, canvas, index, x + column * (tile + LOOK_TILE_GAP), y, tile, picture);
		column++;
	}

	/* The edge below the last row. */
	return y + picture + LOOK_TILE_NAME;
}

/*
 * Draws the Display page: the screen's mode and the graphics device, read
 * only.  Returns the edge below it.
 */
int
se_display_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width)
{
	const char *mode;
	const char *graphics;
	int y;

	/* The values, or a dash for what is not known. */
	mode = app->about.display;
	if (mode[0] == '\0')
		mode = "-";
	graphics = app->about.graphics;
	if (graphics[0] == '\0')
		graphics = "-";

	/* The screen's card. */
	y = se_card_begin(app, canvas, x, top, width, se_card_height(3, 1), "Built-in Screen", NULL);
	y = se_row_value(app, canvas, x, y, width, "Mode", mode, 0);
	y = se_row_value(app, canvas, x, y, width, "Scale", "100%", 0);
	y = se_row_value(app, canvas, x, y, width, "Graphics", graphics, 1);

	/* What comes later. */
	y = look_note(app, canvas, x, top + se_card_height(3, 1) + LOOK_GAP, width, "Changing the resolution and the scale is coming in a later version of Kei.");

	/* The edge below the cards. */
	return y;
}

/*
 * Draws the Storage page: each file system's use, read now.  Returns the
 * edge below it.
 */
int
se_storage_draw(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width)
{
	unsigned index;
	int y;

	/* The file systems as they are now. */
	se_look_volumes(app);

	/* None can be read. */
	if (app->look.volume_count == 0U) {
		y = look_note(app, canvas, x, top, width, "No disk could be read.");
		return y;
	}

	/* A card each. */
	y = top;
	for (index = 0; index < app->look.volume_count; index++)
		y = look_volume(app, canvas, &app->look.volumes[index], x, y, width) + LOOK_GAP;

	/* The edge below the last card. */
	return y - LOOK_GAP;
}

/*
 * Carries out a click on a control of the look's pages: a picture is
 * chosen.
 */
void
se_look_press(
	struct se_app *app,
	int index)
{
	/* A picture's tile. */
	if (index >= LOOK_PICTURE_FIRST)
		se_look_set_wallpaper(app, index - LOOK_PICTURE_FIRST);
}

/*
 * Follows a drag on the opacity slider: the value moves with the pointer,
 * and is saved when the button is let go.
 */
void
se_look_drag(
	struct se_app *app,
	int index,
	int x,
	unsigned phase)
{
	float fraction;
	int percent;

	/* Only the slider is dragged. */
	if (index != LOOK_OPACITY)
		return;

	/* The value under the pointer, in whole percent. */
	fraction = se_slider_fraction(&app->look.slider, x);
	percent = LOOK_OPACITY_MIN + (int)(fraction * (float)(LOOK_OPACITY_MAX - LOOK_OPACITY_MIN) + 0.5f);

	/* While held the page shows it; the release saves it. */
	app->look.opacity = percent;
	app->dirty = 1;
	app->look.dragging = 1;
	if (phase != SE_DRAG_END)
		return;

	/* Let go: saved, and zdesktop follows. */
	app->look.dragging = 0;
	se_look_set_opacity(app, percent);
}

/* Draws a quiet card with one line; returns the edge below it. */
static int
look_note(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width,
	const char *text)
{
	int height;
	int baseline;

	/* One line in a low card. */
	height = 56;
	(void)se_card_begin(app, canvas, x, top, width, height, NULL, NULL);
	baseline = fm_text_center(LOOK_TEXT_SMALL, top, height);
	(void)fm_text_draw_fit(app->text, canvas, x + LOOK_PAD + 2, baseline, text, LOOK_TEXT_SMALL, 0, width - 2 * LOOK_PAD, SE_COLOR_TEXT_SECONDARY);

	/* The edge below the card. */
	return top + height;
}

/* Draws the line about saving (a failure in red), when there is one; returns the edge below it. */
static int
look_message(
	struct se_app *app,
	struct fm_canvas *canvas,
	int x,
	int top,
	int width)
{
	const char *text;
	fm_color ink;

	/* The message kept, or the lack of a home. */
	text = app->look.message;
	if (text[0] == '\0' && app->look.preferences == NULL)
		text = "Settings cannot be saved: this account has no home folder.";
	if (text[0] == '\0')
		return top;

	/* One line, red for a failure. */
	ink = SE_COLOR_TEXT_SECONDARY;
	if (app->look.message_bad != 0 || app->look.preferences == NULL)
		ink = SE_COLOR_BAD;
	(void)fm_text_draw_fit(app->text, canvas, x + 2, top + 16, text, LOOK_TEXT_SMALL, 0, width, ink);

	/* The edge below the line. */
	return top + 30;
}

/* Draws one picture's tile: the picture (or the default's drawn stand-in), its name, a ring when it is shown. */
static void
look_tile(
	struct se_app *app,
	struct fm_canvas *canvas,
	unsigned index,
	int x,
	int y,
	int width,
	int height)
{
	const struct se_wallpaper *wallpaper;
	struct fm_rect rect;
	char name[80];
	int chosen;
	int lit;
	int differs;

	/* Whether this is the picture shown: the default when no key is set, else the one whose path is the key. */
	wallpaper = &app->look.wallpapers[index];
	chosen = 0;
	if (index == 0U &&
	    app->look.has_default != 0 &&
	    app->look.wallpaper[0] == '\0')
		chosen = 1;
	if (app->look.wallpaper[0] != '\0') {
		differs = strcmp(app->look.wallpaper, wallpaper->path);
		if (differs == 0)
			chosen = 1;
	}

	/*
	 * The picture; while its small copy is still being read a plain grey
	 * stand-in (BUG-152); for one that could not be read a quiet gradient
	 * with the Kei mark.
	 */
	if (wallpaper->read != 0) {
		fm_canvas_image(canvas, &wallpaper->thumbnail, (float)x, (float)y, (float)width, (float)height, 10.0f, 1.0f);
	} else if (wallpaper->pending != 0) {
		fm_canvas_round_gradient(canvas, (float)x, (float)y, (float)width, (float)height, 10.0f, FM_RGB(0xe9edf3), FM_RGB(0xdde3ec));
	} else {
		fm_canvas_round_gradient(canvas, (float)x, (float)y, (float)width, (float)height, 10.0f, FM_RGB(0xdfeaf7), FM_RGB(0xc8dcc4));
		se_mark_draw(canvas, x + width / 2 - 24, y + height / 2 - 24, 48U, 0.9f);
	}

	/* The ring: the accent round the picture shown, a shade under the pointer. */
	lit = se_ui_lit(app, SE_HIT_CONTROL, LOOK_PICTURE_FIRST + (int)index);
	if (chosen != 0) {
		fm_canvas_round_border(canvas, (float)x - 3.0f, (float)y - 3.0f, (float)width + 6.0f, (float)height + 6.0f, 13.0f, 3.0f, SE_COLOR_ACCENT);
	} else if (lit != 0) {
		fm_canvas_round_border(canvas, (float)x - 2.0f, (float)y - 2.0f, (float)width + 4.0f, (float)height + 4.0f, 12.0f, 2.0f, FM_RGBA(0x5a6b85, 90));
	} else {
		fm_canvas_round_border(canvas, (float)x, (float)y, (float)width, (float)height, 10.0f, 1.0f, SE_COLOR_CARD_EDGE);
	}

	/* The name under it; the default says so. */
	(void)snprintf(name, sizeof(name), "%s", wallpaper->name);
	if (index == 0U && app->look.has_default != 0)
		(void)snprintf(name, sizeof(name), "%s (default)", wallpaper->name);
	(void)fm_text_draw_fit(app->text, canvas, x + 2, y + height + 20, name, LOOK_TEXT_SMALL, chosen, width - 4, SE_COLOR_TEXT);

	/* A click chooses it. */
	rect.x = x;
	rect.y = y;
	rect.width = width;
	rect.height = height + LOOK_TILE_NAME;
	se_ui_hit(app, &rect, SE_HIT_CONTROL, LOOK_PICTURE_FIRST + (int)index);
}

/* Draws one file system's card: where it is, a bar of its use, the bytes used and free. Returns the edge below it. */
static int
look_volume(
	struct se_app *app,
	struct fm_canvas *canvas,
	const struct se_volume *volume,
	int x,
	int top,
	int width)
{
	char used[32];
	char total[32];
	char available[32];
	char line[128];
	char title[80];
	float share;
	int height;
	int y;
	int bar;

	/* The card, named by where the file system is. */
	height = 118;
	(void)snprintf(title, sizeof(title), "Disk at %s", volume->path);
	y = se_card_begin(app, canvas, x, top, width, height, title, NULL);

	/* The bar: the share used in the accent, red when nearly full. */
	share = 0.0f;
	if (volume->total != 0U)
		share = (float)((double)volume->used / (double)volume->total);
	bar = width - 2 * LOOK_PAD;
	fm_canvas_round(canvas, (float)(x + LOOK_PAD), (float)y, (float)bar, (float)LOOK_BAR_HEIGHT, 5.0f, FM_RGB(0xd3d9e2));
	if (share > 0.9f) {
		fm_canvas_round(canvas, (float)(x + LOOK_PAD), (float)y, (float)bar * share, (float)LOOK_BAR_HEIGHT, 5.0f, SE_COLOR_BAD);
	} else {
		fm_canvas_round(canvas, (float)(x + LOOK_PAD), (float)y, (float)bar * share, (float)LOOK_BAR_HEIGHT, 5.0f, SE_COLOR_ACCENT);
	}

	/* The bytes used of the whole, and what is left. */
	se_bytes_text(volume->used, used, sizeof(used));
	se_bytes_text(volume->total, total, sizeof(total));
	se_bytes_text(volume->available, available, sizeof(available));
	(void)snprintf(line, sizeof(line), "%s used of %s  \xc2\xb7  %s available", used, total, available);
	(void)fm_text_draw_fit(app->text, canvas, x + LOOK_PAD + 2, y + LOOK_BAR_HEIGHT + 26, line, LOOK_TEXT_SMALL, 0, bar, SE_COLOR_TEXT_SECONDARY);

	/* The edge below the card. */
	return top + height;
}
