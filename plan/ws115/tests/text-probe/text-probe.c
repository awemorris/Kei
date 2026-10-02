/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The text probe (ws115-p006): exercises fontconfig, FreeType, HarfBuzz and
 * libpng together, the way Pango will, on the target, and prints one PASS
 * line or the step that failed.
 *
 *   text-probe <text-font-directory> <colour-emoji-font>
 *
 * fontconfig picks the font for "sans-serif", FreeType opens it, HarfBuzz
 * shapes a word with it through hb-ft, and FreeType decodes a colour emoji
 * glyph, which the emoji font keeps as a PNG image, through libpng.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>

/*
 * The word the probe shapes; each of its letters has a glyph in any Latin font.
 */
#define PROBE_WORD "zedBSD"

/*
 * The number of characters, and so of glyphs, in the word.
 */
#define PROBE_WORD_LENGTH 6

/*
 * The emoji the probe draws: U+1F600 GRINNING FACE.
 */
#define PROBE_EMOJI 0x1F600

static const char *probe_run(const char *font_directory, const char *emoji_path);
static bool probe_match(const char *font_directory, char *path, size_t path_size);
static bool probe_shape(FT_Library library, const char *path);
static bool probe_emoji(FT_Library library, const char *emoji_path);

/*
 * Runs every probe step and reports the result.
 */
int
main(
	int argc,
	char **argv)
{
	const char *failed_step;

	/* Refuses a call without the font directory and the emoji font. */
	if (argc != 3) {
		fprintf(stderr, "usage: text-probe <text-font-directory> <colour-emoji-font>\n");
		return 2;
	}

	/* Runs the steps, which stop at the first one that fails. */
	failed_step = probe_run(argv[1], argv[2]);
	if (failed_step != NULL) {
		printf("text-probe: FAIL %s\n", failed_step);
		return 1;
	}

	/* Reports that every step passed, with the HarfBuzz release that shaped. */
	printf("text-probe: PASS harfbuzz %s fontconfig %d freetype %d.%d.%d\n",
	       hb_version_string(), FcGetVersion(), FREETYPE_MAJOR, FREETYPE_MINOR,
	       FREETYPE_PATCH);

	/* Succeeded: every step passed. */
	return 0;
}

/* Runs the probe steps in order and names the first that fails. */
static const char *
probe_run(
	const char *font_directory,
	const char *emoji_path)
{
	FT_Library library;
	FT_Error error;
	char path[1024];
	bool passed;

	/* Lets fontconfig choose the file that "sans-serif" means here. */
	passed = probe_match(font_directory, path, sizeof(path));
	if (!passed)
		return "fontconfig-match";

	/* Starts FreeType for the two font steps. */
	error = FT_Init_FreeType(&library);
	if (error != 0)
		return "freetype-init";

	/* Shapes the word with the matched font through hb-ft. */
	passed = probe_shape(library, path);
	if (!passed) {
		FT_Done_FreeType(library);
		return "harfbuzz-shape";
	}

	/* Decodes the PNG of a colour emoji glyph. */
	passed = probe_emoji(library, emoji_path);
	FT_Done_FreeType(library);
	if (!passed)
		return "freetype-png-emoji";

	/* Succeeded: no step failed. */
	return NULL;
}

/* Asks fontconfig for the sans-serif font and checks where it lives. */
static bool
probe_match(
	const char *font_directory,
	char *path,
	size_t path_size)
{
	FcPattern *pattern;
	FcPattern *match;
	FcResult result;
	FcChar8 *file;
	FcChar8 *family;
	FcResult found;
	size_t directory_length;
	int compared;

	/* Builds the request a toolkit makes for its default face. */
	pattern = FcNameParse((const FcChar8 *)"sans-serif");
	if (pattern == NULL)
		return false;

	/* Completes the request with the configuration and the defaults. */
	FcConfigSubstitute(NULL, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	/* Matches it against the fonts the configuration found. */
	match = FcFontMatch(NULL, pattern, &result);
	FcPatternDestroy(pattern);
	if (match == NULL)
		return false;

	/* Takes the file of the match. */
	found = FcPatternGetString(match, FC_FILE, 0, &file);
	if (found != FcResultMatch) {
		FcPatternDestroy(match);
		return false;
	}

	/* Refuses a file outside the directory the fonts are installed in. */
	directory_length = strlen(font_directory);
	compared = strncmp((const char *)file, font_directory, directory_length);
	if (compared != 0) {
		printf("text-probe: matched %s\n", (const char *)file);
		FcPatternDestroy(match);
		return false;
	}

	/* Reports the family for the record, when the font names one. */
	found = FcPatternGetString(match, FC_FAMILY, 0, &family);
	if (found == FcResultMatch)
		printf("text-probe: sans-serif is %s (%s)\n", (const char *)family, (const char *)file);

	/* Copies the path out before the pattern that holds it goes. */
	snprintf(path, path_size, "%s", (const char *)file);
	FcPatternDestroy(match);

	/* Succeeded: the configuration found a font in the font directory. */
	return true;
}

/* Shapes the probe word with the font at path and checks the glyphs. */
static bool
probe_shape(
	FT_Library library,
	const char *path)
{
	FT_Face face;
	FT_Error error;
	hb_font_t *font;
	hb_buffer_t *buffer;
	hb_glyph_info_t *infos;
	hb_glyph_position_t *positions;
	unsigned int count;
	unsigned int i;
	hb_position_t advance;
	bool every_glyph;

	/* Opens the font at 16 points on a 72 dpi grid. */
	error = FT_New_Face(library, path, 0, &face);
	if (error != 0)
		return false;

	/* Sizes the face, which hb-ft reads its scale from. */
	error = FT_Set_Char_Size(face, 16 * 64, 0, 72, 0);
	if (error != 0) {
		FT_Done_Face(face);
		return false;
	}

	/* Shapes the word as HarfBuzz would for any left-to-right text. */
	font = hb_ft_font_create_referenced(face);
	buffer = hb_buffer_create();
	hb_buffer_add_utf8(buffer, PROBE_WORD, -1, 0, -1);
	hb_buffer_guess_segment_properties(buffer);
	hb_shape(font, buffer, NULL, 0);

	/* Reads the shaped glyphs and their positions. */
	infos = hb_buffer_get_glyph_infos(buffer, &count);
	positions = hb_buffer_get_glyph_positions(buffer, &count);

	/* Sums the advances and notes a glyph the font lacks (glyph 0). */
	advance = 0;
	every_glyph = true;
	for (i = 0; i < count; i++) {
		if (infos[i].codepoint == 0)
			every_glyph = false;
		advance += positions[i].x_advance;
	}

	/* Releases the shaping objects; the font holds the face's reference. */
	hb_buffer_destroy(buffer);
	hb_font_destroy(font);
	FT_Done_Face(face);

	/* Refuses a result without one real glyph per letter. */
	if (count != PROBE_WORD_LENGTH || !every_glyph)
		return false;

	/* Refuses a word that does not move the pen forward. */
	if (advance <= 0)
		return false;

	/* Succeeded: one real glyph per letter, laid out left to right. */
	printf("text-probe: shaped %u glyphs, advance %d/64 px\n", count, (int)advance);
	return true;
}

/* Loads the colour emoji glyph and checks that it came out as a BGRA image. */
static bool
probe_emoji(
	FT_Library library,
	const char *emoji_path)
{
	FT_Face face;
	FT_Error error;
	FT_UInt glyph;
	unsigned char pixel_mode;
	unsigned int width;

	/* Opens the colour emoji font. */
	error = FT_New_Face(library, emoji_path, 0, &face);
	if (error != 0)
		return false;

	/* Refuses a font without colour glyphs or without a bitmap strike. */
	if (!FT_HAS_COLOR(face) || face->num_fixed_sizes < 1) {
		FT_Done_Face(face);
		return false;
	}

	/* Selects the font's strike, whose glyphs are PNG images. */
	error = FT_Select_Size(face, 0);
	if (error != 0) {
		FT_Done_Face(face);
		return false;
	}

	/* Finds the emoji's glyph. */
	glyph = FT_Get_Char_Index(face, PROBE_EMOJI);
	if (glyph == 0) {
		FT_Done_Face(face);
		return false;
	}

	/* Decodes the glyph's PNG into a colour bitmap. */
	error = FT_Load_Glyph(face, glyph, FT_LOAD_COLOR);
	if (error != 0) {
		FT_Done_Face(face);
		return false;
	}

	/* Keeps what the check needs before the face goes. */
	pixel_mode = face->glyph->bitmap.pixel_mode;
	width = face->glyph->bitmap.width;
	FT_Done_Face(face);

	/* Refuses anything but a non-empty BGRA image. */
	if (pixel_mode != FT_PIXEL_MODE_BGRA || width == 0)
		return false;

	/* Succeeded: libpng decoded the glyph's image. */
	printf("text-probe: emoji glyph %u is a %u px wide BGRA image\n", glyph, width);
	return true;
}
