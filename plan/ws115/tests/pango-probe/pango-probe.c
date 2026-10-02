/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Pango probe (ws115-p007): draws a few lines of text that mix Latin,
 * Japanese and a colour emoji with Pango onto a Cairo image surface,
 * on the target, and writes the picture as a PNG for a person to look at.
 * It prints one PASS line, or the step that failed.
 *
 *   pango-probe <output.png>
 *
 * The text is laid out with the "sans-serif" family, which fontconfig
 * resolves; characters the first font lacks fall back to the other fonts in
 * /usr/share/fonts, which is the part of the stack the probe is for.  A
 * character no font covers is counted, and any such count fails the probe.
 */

#include <stdio.h>

#include <cairo.h>
#include <pango/pangocairo.h>

/*
 * The size of the picture, in pixels.
 */
#define PROBE_WIDTH 960
#define PROBE_HEIGHT 300

/*
 * The margin around the text, in pixels.
 */
#define PROBE_MARGIN 24

/*
 * The text drawn: Latin, Japanese (kana and kanji) and U+1F600, a colour
 * glyph.  No font zedBSD installs covers a right-to-left script, so none is
 * drawn; Pango still runs every paragraph through FriBidi.
 */
#define PROBE_TEXT \
	"zedBSD: Pango \xe3\x81\xa8 Cairo\n" \
	"\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x81\xae\xe6\x96\x87\xe5\xad\x97\xe3\x82\x92\xe6\x8f\x8f\xe3\x81\x8f\xe3\x80\x82\n" \
	"zedBSD \xf0\x9f\x98\x80 \xe7\xb5\xb5\xe6\x96\x87\xe5\xad\x97"

static const char *probe_run(const char *output_path, int *unknown_glyphs);
static void probe_draw(cairo_t *cr, int *unknown_glyphs);

/*
 * Draws the text and writes the picture, then reports the result.
 */
int
main(
	int argc,
	char **argv)
{
	const char *failed_step;
	int unknown_glyphs;

	/* Refuses a call without the output file. */
	if (argc != 2) {
		fprintf(stderr, "usage: pango-probe <output.png>\n");
		return 2;
	}

	/* Draws and writes, stopping at the first step that fails. */
	unknown_glyphs = 0;
	failed_step = probe_run(argv[1], &unknown_glyphs);
	if (failed_step != NULL) {
		printf("pango-probe: FAIL %s (unknown glyphs %d)\n", failed_step, unknown_glyphs);
		return 1;
	}

	/* Reports what drew the picture and where it went. */
	printf("pango-probe: PASS pango %s cairo %s wrote %s\n", pango_version_string(),
	       cairo_version_string(), argv[1]);

	/* Succeeded: the picture is written for a person to look at. */
	return 0;
}

/* Draws the text on an image surface and writes it, naming a failed step. */
static const char *
probe_run(
	const char *output_path,
	int *unknown_glyphs)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_status_t status;

	/* Makes the image surface the text is drawn on. */
	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, PROBE_WIDTH, PROBE_HEIGHT);
	status = cairo_surface_status(surface);
	if (status != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return "cairo-surface";
	}

	/* Draws the background and the text. */
	cr = cairo_create(surface);
	probe_draw(cr, unknown_glyphs);
	status = cairo_status(cr);
	cairo_destroy(cr);
	if (status != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(surface);
		return "cairo-draw";
	}

	/* Writes the picture through libpng. */
	status = cairo_surface_write_to_png(surface, output_path);
	cairo_surface_destroy(surface);
	if (status != CAIRO_STATUS_SUCCESS)
		return "cairo-png";

	/* Refuses a picture with a character no installed font could draw. */
	if (*unknown_glyphs != 0)
		return "pango-coverage";

	/* Succeeded: every character was drawn and the PNG was written. */
	return NULL;
}

/* Fills the surface white and lays out and draws the text in black. */
static void
probe_draw(
	cairo_t *cr,
	int *unknown_glyphs)
{
	PangoLayout *layout;
	PangoFontDescription *description;

	/* Paints the background white. */
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	/* Lays the text out in the default sans-serif face at 36 points. */
	layout = pango_cairo_create_layout(cr);
	description = pango_font_description_from_string("sans-serif 36");
	pango_layout_set_font_description(layout, description);
	pango_font_description_free(description);
	pango_layout_set_text(layout, PROBE_TEXT, -1);

	/* Counts the characters that came out as boxes for lack of a font. */
	*unknown_glyphs = pango_layout_get_unknown_glyphs_count(layout);

	/* Draws the layout in black inside the margin. */
	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	cairo_move_to(cr, PROBE_MARGIN, PROBE_MARGIN);
	pango_cairo_show_layout(cr, layout);
	g_object_unref(layout);
}
