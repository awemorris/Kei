/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Inline layout: a block's inline content (its text boxes, through the
 * inline boxes that hold them) cut into pieces at the break opportunities,
 * whitespace collapsed as white-space asks, and packed greedily into line
 * boxes that are aligned and stacked.
 *
 * An inline replaced box (an <img>) is a piece of its own, as wide as its
 * margin box, standing on the baseline.  So is an inline block
 * (ws074-p060): laid out on its own, shrunk to fit, it stands on its last
 * line's baseline (on its bottom margin edge when it has no line or clips
 * its overflow).  vertical-align then moves each piece: to the line's top
 * or bottom, to the middle, the parent's text top or bottom, the sub- or
 * superscript position, or by a length; the line grows to hold them.
 *
 * The first pass sets every piece on the baseline; inline boxes contribute
 * their style (through the text they hold) but no borders or padding yet.
 * The floats among the content are placed first, at the content's top
 * (not at the line they are on, in this pass), and each line is as wide
 * as the floats beside it leave.
 */

#include "layout/layout.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* How many spaces a tab stands for in preserved whitespace. */
#define INLINE_TAB_SPACES	8

/*
 * One piece of inline content: a word (or a run that cannot break), a
 * collapsed space, a forced line break, or a replaced box.  A collapsed
 * space is drawn as one space however much whitespace it stands for.
 */
struct inline_piece {
	struct layout_box *box;
	const uint16_t *text;
	size_t length;
	layout_unit width;
	int space;
	int forced_break;
	int replaced;
	int uses_fallback;
	struct text_font font;
};

/*
 * The state of cutting a block's content into pieces: the block's width
 * (which replaced boxes are sized in), the pieces so far, the word being
 * gathered and whether the last character was collapsed whitespace.
 */
struct inline_cutter {
	struct layout_tree *tree;
	layout_unit width;
	struct wb_vector pieces;
	struct layout_box *box;
	const uint16_t *word;
	size_t word_length;
	layout_unit word_width;
	int word_fallback;
	uint32_t previous;
	int after_space;
	struct text_font font;
	int error;
};

/*
 * How far a piece of a line reaches above and below its own baseline: an
 * atomic piece's margin box, or a text piece's line height.
 */
struct inline_reach {
	layout_unit above;
	layout_unit below;
};

/* The one space a collapsed run of whitespace is drawn as. */
static const uint16_t inline_space[1] = { 0x20U };

static void inline_collect(struct inline_cutter *cutter, struct layout_box *box);
static void inline_cut_text(struct inline_cutter *cutter, struct layout_box *box);
static void inline_add_replaced(struct inline_cutter *cutter, struct layout_box *box);
static void inline_add_atomic(struct inline_cutter *cutter, struct layout_box *box);
static int inline_last_baseline(const struct layout_box *box, layout_unit *baseline, int depth);
static void inline_align(struct layout_tree *tree, struct layout_box *box, struct layout_line *line, struct inline_reach *reach, size_t count, layout_unit *above, layout_unit *below);
static layout_unit inline_shift(struct layout_tree *tree, const struct layout_box *box, const struct css_style *style, const struct inline_reach *reach);
static layout_unit inline_x_height(struct layout_tree *tree, const struct css_style *style);
static const struct css_style *inline_align_style(const struct layout_fragment *fragment, const struct layout_box *block);
static void inline_place_atomics(const struct layout_line *line);
static void inline_replaced_extent(const struct layout_box *box, layout_unit *above, layout_unit *below);
static layout_unit inline_margin_height(const struct layout_box *box);
static void inline_flush_word(struct inline_cutter *cutter);
static void inline_add_piece(struct inline_cutter *cutter, const uint16_t *text, size_t length, layout_unit width, int space, int forced_break);
static layout_unit inline_advance(struct inline_cutter *cutter, uint32_t code_point);
static int inline_build_lines(struct layout_tree *tree, struct layout_box *box, const struct inline_piece *pieces, size_t count);
static int inline_finish_line(struct layout_tree *tree, struct layout_box *box, const struct inline_piece *pieces, size_t start, size_t end, layout_unit *cursor, struct wb_vector *lines, int first_line, layout_unit line_left, layout_unit line_width);
static int inline_place_floats(struct layout_tree *tree, struct layout_box *content, struct layout_box *box, int depth);
static void inline_room(struct layout_tree *tree, struct layout_box *box, layout_unit cursor, layout_unit *left, layout_unit *width);
static int inline_wraps(const struct layout_box *box);
static void inline_line_height(struct layout_tree *tree, const struct css_style *style, layout_unit *above, layout_unit *below);
static void inline_font_extent(struct layout_tree *tree, const struct text_font *font, const struct css_style *style, layout_unit *above, layout_unit *below);
static void inline_piece_extent(struct layout_tree *tree, const struct inline_piece *piece, layout_unit *above, layout_unit *below);

/*
 * Lays out a block's inline content into lines; the block's content
 * height becomes the lines' total height.
 */
int
layout_inline(
	struct layout_tree *tree,
	struct layout_box *box)
{
	struct inline_cutter cutter;
	struct layout_box *child;
	int error;

	/* The floats among the content go to their sides first. */
	error = inline_place_floats(tree, box, box, 0);
	if (error != 0)
		return error;

	/* Cuts the content into pieces. */
	memset(&cutter, 0, sizeof(cutter));
	cutter.tree = tree;
	cutter.width = box->width;
	cutter.after_space = 1;
	wb_vector_init(&cutter.pieces, sizeof(struct inline_piece));
	for (child = box->first_child; child != NULL; child = child->next)
		inline_collect(&cutter, child);
	inline_flush_word(&cutter);
	if (cutter.error != 0) {
		wb_vector_release(&cutter.pieces);
		return cutter.error;
	}

	/* Packs the pieces into lines. */
	error = inline_build_lines(tree, box, cutter.pieces.items, cutter.pieces.count);
	wb_vector_release(&cutter.pieces);
	if (error != 0)
		return error;

	/* Succeeded: the block has its lines and its height. */
	return 0;
}

/* Cuts an inline-level box's content: text, a line break, or an inline box's children. */
static void
inline_collect(
	struct inline_cutter *cutter,
	struct layout_box *box)
{
	struct layout_box *child;

	/* A box out of the flow, or a float, is no part of the lines. */
	if (box->out_of_flow || box->floating != CSS_FLOAT_NONE)
		return;

	/* Text is cut into words and spaces. */
	if (box->kind == LAYOUT_TEXT) {
		inline_cut_text(cutter, box);
		return;
	}

	/* A replaced box is a piece of its own. */
	if (box->kind == LAYOUT_REPLACED) {
		inline_add_replaced(cutter, box);
		return;
	}

	/* So is an inline block, laid out on its own first. */
	if (box->atomic) {
		inline_add_atomic(cutter, box);
		return;
	}

	/* A line break ends the line. */
	if (box->kind == LAYOUT_LINE_BREAK) {
		inline_flush_word(cutter);
		cutter->box = box;
		inline_add_piece(cutter, NULL, 0, 0, 0, 1);
		cutter->after_space = 1;
		return;
	}

	/* An inline box's children, in order. */
	for (child = box->first_child; child != NULL; child = child->next)
		inline_collect(cutter, child);
}

/* Cuts a text box's text into words, collapsed spaces and forced breaks. */
static void
inline_cut_text(
	struct inline_cutter *cutter,
	struct layout_box *box)
{
	struct text_font font;
	uint32_t code_point;
	size_t offset;
	size_t used;
	int font_changed;
	int preserve;
	int wrap_lines;
	int space;
	int opportunity;
	int tab;

	/* The font and the white-space behaviour of the text. */
	layout_font_of(cutter->tree, &box->style, &font);
	preserve = 0;
	if (box->style.white_space == CSS_WHITE_SPACE_PRE || box->style.white_space == CSS_WHITE_SPACE_PRE_WRAP)
		preserve = 1;
	wrap_lines = 1;
	if (box->style.white_space == CSS_WHITE_SPACE_PRE || box->style.white_space == CSS_WHITE_SPACE_NOWRAP)
		wrap_lines = 0;

	/* A new box or font ends the word before it (pieces have one font). */
	font_changed = memcmp(&font, &cutter->font, sizeof(font));
	if (cutter->word != NULL) {
		if (cutter->box != box) {
			/* The word so far belongs to the box before. */
			inline_flush_word(cutter);
		} else if (font_changed != 0) {
			/* The word so far is in another font. */
			inline_flush_word(cutter);
		}
	}

	/* The characters that follow belong to this box and font. */
	cutter->box = box;
	cutter->font = font;

	/* Walks the code points. */
	offset = 0;
	while (offset < box->text_length && cutter->error == 0) {
		used = wb_utf16_decode(box->text + offset, box->text_length - offset, &code_point);
		space = text_is_space(code_point);

		/* A line feed is a forced break where line breaks are kept. */
		if (code_point == 0x0aU && (preserve || box->style.white_space == CSS_WHITE_SPACE_PRE_LINE)) {
			inline_flush_word(cutter);
			inline_add_piece(cutter, NULL, 0, 0, 0, 1);
			cutter->after_space = 1;
			cutter->previous = 0;
			offset += used;
			continue;
		}

		/* Collapsible whitespace becomes one space, and none after another. */
		if (space && !preserve) {
			inline_flush_word(cutter);
			if (!cutter->after_space)
				inline_add_piece(cutter, inline_space, 1, inline_advance(cutter, 0x20U), 1, 0);
			cutter->after_space = 1;
			cutter->previous = 0x20U;
			offset += used;
			continue;
		}

		/* A break opportunity before this character ends the word (only where lines wrap). */
		opportunity = text_break_between(cutter->previous, code_point);
		if (cutter->word != NULL && opportunity && wrap_lines)
			inline_flush_word(cutter);

		/* The character joins the word; a preserved tab is as wide as several spaces. */
		if (cutter->word == NULL) {
			cutter->word = box->text + offset;
			cutter->word_length = 0;
			cutter->word_width = 0;
			cutter->word_fallback = 0;
		}

		/* The character's advance widens the word. */
		cutter->word_length += used;
		tab = 0;
		if (code_point == 0x09U)
			tab = 1;
		if (tab) {
			cutter->word_width += inline_advance(cutter, 0x20U) * INLINE_TAB_SPACES;
		} else {
			cutter->word_width += inline_advance(cutter, code_point);
		}

		/* The next character follows a character that is not collapsed whitespace. */
		cutter->after_space = 0;
		cutter->previous = code_point;
		offset += used;
	}
}

/*
 * Sizes an inline replaced box in the block's width and adds it as a
 * piece as wide as its margin box; the lines may break before and after
 * it.
 */
static void
inline_add_replaced(
	struct inline_cutter *cutter,
	struct layout_box *box)
{
	struct inline_piece *piece;
	layout_unit width;

	/* Its margins, borders, paddings and size. */
	layout_box_model(box, cutter->width);
	layout_replaced_size(box, cutter->width, cutter->tree->containing_height, cutter->tree->containing_height_definite);
	width = box->margin[CSS_LEFT] + box->border[CSS_LEFT] + box->padding[CSS_LEFT] + box->width +
	    box->padding[CSS_RIGHT] + box->border[CSS_RIGHT] + box->margin[CSS_RIGHT];

	/* The word before it ends, and the box is a piece of its own. */
	inline_flush_word(cutter);
	cutter->box = box;
	inline_add_piece(cutter, NULL, 0, width, 0, 0);
	if (cutter->error != 0)
		return;
	piece = wb_vector_at(&cutter->pieces, cutter->pieces.count - 1U);
	piece->replaced = 1;

	/* What follows is not after a space, and starts a new word. */
	cutter->after_space = 0;
	cutter->previous = 0;
}

/*
 * Lays an inline block out, shrunk to fit the block's width in a
 * formatting context of its own, finds its baseline, and adds it as a
 * piece as wide as its margin box; the lines may break before and after it.
 */
static void
inline_add_atomic(
	struct inline_cutter *cutter,
	struct layout_box *box)
{
	struct inline_piece *piece;
	layout_unit origin_x;
	layout_unit origin_y;
	layout_unit baseline;
	layout_unit width;
	int found;
	int clips;
	int error;

	/* The inline block's own layout, which leaves the origin of the block around it as it was. */
	origin_x = cutter->tree->origin_x;
	origin_y = cutter->tree->origin_y;
	error = layout_shrink_to_fit(cutter->tree, box, cutter->width);
	cutter->tree->origin_x = origin_x;
	cutter->tree->origin_y = origin_y;
	if (error != 0) {
		cutter->error = error;
		return;
	}

	/* Its baseline is its last line's, unless it clips its overflow (then its bottom margin edge stands on the line's). */
	box->has_baseline = 0;
	box->control_baseline = 0;
	clips = layout_clips(box);
	if (!clips) {
		found = inline_last_baseline(box, &baseline, 0);
		if (found) {
			box->has_baseline = 1;
			box->control_baseline = baseline;
		}
	}

	/* Its margin box's width. */
	width = box->margin[CSS_LEFT] + box->border[CSS_LEFT] + box->padding[CSS_LEFT] + box->width +
	    box->padding[CSS_RIGHT] + box->border[CSS_RIGHT] + box->margin[CSS_RIGHT];

	/* The word before it ends, and the box is a piece of its own. */
	inline_flush_word(cutter);
	cutter->box = box;
	inline_add_piece(cutter, NULL, 0, width, 0, 0);
	if (cutter->error != 0)
		return;
	piece = wb_vector_at(&cutter->pieces, cutter->pieces.count - 1U);
	piece->replaced = 1;

	/* What follows is not after a space, and starts a new word. */
	cutter->after_space = 0;
	cutter->previous = 0;
}

/*
 * Finds the baseline of a laid out block's last line in the normal flow,
 * searching its last blocks that have one, as a distance below the top of
 * its content box.  Reports whether there is one.
 */
static int
inline_last_baseline(
	const struct layout_box *box,
	layout_unit *baseline,
	int depth)
{
	const struct layout_box *child;
	const struct layout_line *line;
	layout_unit inner;
	int found;
	int any;

	/* Stops at the depth the layout stops at. */
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/* A block of lines: its last line's baseline. */
	if (box->children_inline) {
		if (box->line_count == 0)
			return 0;
		line = &box->lines[box->line_count - 1U];
		*baseline = line->y + line->baseline;
		return 1;
	}

	/* A block of blocks: the last child in the flow that has a baseline, below where that child's content starts. */
	any = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow || child->floating != CSS_FLOAT_NONE)
			continue;
		if (child->kind != LAYOUT_BLOCK && child->kind != LAYOUT_ANONYMOUS_BLOCK)
			continue;

		/* The child's own last baseline, moved into this box's content box. */
		found = inline_last_baseline(child, &inner, depth + 1);
		if (found) {
			*baseline = child->y + child->border[CSS_TOP] + child->padding[CSS_TOP] + inner;
			any = 1;
		}
	}

	/* Reports whether any child had one. */
	return any;
}

/*
 * Measures how far an inline replaced box reaches above and below the
 * baseline: an image's whole margin box stands on it; a form control's
 * text's baseline is on it, the rest of its margin box below.
 */
static void
inline_replaced_extent(
	const struct layout_box *box,
	layout_unit *above,
	layout_unit *below)
{
	layout_unit height;

	/* The margin box. */
	height = inline_margin_height(box);

	/* An image, or a control without text, is all above the baseline. */
	if (!box->has_baseline) {
		*above = height;
		*below = 0;
		return;
	}

	/* A control's text baseline is its margin, border and padding above its content, and its text's baseline in it. */
	*above = box->margin[CSS_TOP] + box->border[CSS_TOP] + box->padding[CSS_TOP] + box->control_baseline;
	*below = height - *above;
	if (*below < 0)
		*below = 0;
}

/* Measures a box's margin box height. */
static layout_unit
inline_margin_height(
	const struct layout_box *box)
{
	layout_unit height;

	/* The margins, borders and paddings above and below the content. */
	height = box->margin[CSS_TOP] + box->border[CSS_TOP] + box->padding[CSS_TOP] + box->height +
	    box->padding[CSS_BOTTOM] + box->border[CSS_BOTTOM] + box->margin[CSS_BOTTOM];
	return height;
}

/* Ends the word being gathered and adds it as a piece. */
static void
inline_flush_word(
	struct inline_cutter *cutter)
{
	/* Nothing gathered is nothing to add. */
	if (cutter->word == NULL)
		return;

	/* Adds the word. */
	inline_add_piece(cutter, cutter->word, cutter->word_length, cutter->word_width, 0, 0);
	cutter->word = NULL;
	cutter->word_length = 0;
	cutter->word_width = 0;
	cutter->word_fallback = 0;
}

/* Adds a piece of the current box and font. */
static void
inline_add_piece(
	struct inline_cutter *cutter,
	const uint16_t *text,
	size_t length,
	layout_unit width,
	int space,
	int forced_break)
{
	struct inline_piece piece;
	int error;

	/* Fills the piece. */
	memset(&piece, 0, sizeof(piece));
	piece.box = cutter->box;
	piece.text = text;
	piece.length = length;
	piece.width = width;
	piece.space = space;
	piece.forced_break = forced_break;
	piece.font = cutter->font;

	/* A word drawn partly in the fallback face remembers it for the line's height. */
	if (text != NULL && !space)
		piece.uses_fallback = cutter->word_fallback;

	/* Appends it. */
	error = wb_vector_push(&cutter->pieces, &piece);
	if (error != 0)
		cutter->error = error;
}

/* Measures a code point's advance in the current font, in layout units. */
static layout_unit
inline_advance(
	struct inline_cutter *cutter,
	uint32_t code_point)
{
	struct text_glyph glyph;
	int error;

	/* Asks the text system for the glyph. */
	error = text_glyph(cutter->tree->text, &cutter->font, code_point, 0, &glyph);
	if (error != 0) {
		cutter->error = error;
		return 0;
	}

	/* A glyph the font's own face lacks came from the fallback face. */
	if (glyph.face != cutter->font.face)
		cutter->word_fallback = 1;

	/* Reports the advance in layout units. */
	return (layout_unit)glyph.advance_units;
}

/* Tells whether a box's white-space lets lines wrap. */
static int
inline_wraps(
	const struct layout_box *box)
{
	/* pre and nowrap keep a line whole. */
	if (box->style.white_space == CSS_WHITE_SPACE_PRE || box->style.white_space == CSS_WHITE_SPACE_NOWRAP)
		return 0;

	/* The others wrap. */
	return 1;
}

/* Packs pieces into lines greedily and stores the lines in the block. */
static int
inline_build_lines(
	struct layout_tree *tree,
	struct layout_box *box,
	const struct inline_piece *pieces,
	size_t count)
{
	struct wb_vector lines;
	layout_unit cursor;
	layout_unit x;
	layout_unit line_left;
	layout_unit line_width;
	layout_unit below;
	size_t start;
	size_t index;
	int wrap_lines;
	int wraps;
	int error;

	/* Lines wrap unless the block's white-space forbids it. */
	wrap_lines = inline_wraps(box);

	/* Walks the pieces, ending a line at a forced break or when the next word does not fit. */
	wb_vector_init(&lines, sizeof(struct layout_line));
	cursor = 0;
	start = 0;
	x = 0;
	index = 0;
	inline_room(tree, box, cursor, &line_left, &line_width);
	while (index < count) {
		/* Spaces at the start of a line are dropped. */
		if (pieces[index].space && index == start) {
			start++;
			index++;
			continue;
		}

		/* A word that does not fit beside the floats at a line's start moves the line below the next one to end. */
		if (wrap_lines && index == start && pieces[index].width > line_width && line_width < box->width) {
			below = layout_below_float(tree, cursor);
			if (below > cursor) {
				cursor = below;
				inline_room(tree, box, cursor, &line_left, &line_width);
				continue;
			}
		}

		/* A forced break ends the line with what came before it. */
		if (pieces[index].forced_break) {
			error = inline_finish_line(tree, box, pieces, start, index, &cursor, &lines, lines.count == 0, line_left, line_width);
			if (error != 0) {
				wb_vector_release(&lines);
				return error;
			}

			/* The next line starts after the break. */
			index++;
			start = index;
			x = 0;
			inline_room(tree, box, cursor, &line_left, &line_width);
			continue;
		}

		/*
		 * A word that overflows a line that has something ends the line
		 * before it, where the white-space of the piece before (the space
		 * or the text the break follows) lets lines wrap (ws074-p037: a
		 * link with white-space: normal in a nowrap item wraps).
		 */
		wraps = 0;
		if (index > start)
			wraps = inline_wraps(pieces[index - 1U].box);
		if (wraps && !pieces[index].space && x + pieces[index].width > line_width) {
			error = inline_finish_line(tree, box, pieces, start, index, &cursor, &lines, lines.count == 0, line_left, line_width);
			if (error != 0) {
				wb_vector_release(&lines);
				return error;
			}

			/* The next line starts with the word. */
			start = index;
			x = 0;
			inline_room(tree, box, cursor, &line_left, &line_width);
			continue;
		}

		/* The piece goes on the line. */
		x += pieces[index].width;
		index++;
	}

	/* The last line. */
	if (start < count) {
		error = inline_finish_line(tree, box, pieces, start, count, &cursor, &lines, lines.count == 0, line_left, line_width);
		if (error != 0) {
			wb_vector_release(&lines);
			return error;
		}
	}

	/* Moves the lines into the arena. */
	box->line_count = lines.count;
	box->lines = NULL;
	if (lines.count != 0) {
		box->lines = wb_arena_alloc(&tree->arena, lines.count * sizeof(struct layout_line));
		if (box->lines == NULL) {
			wb_vector_release(&lines);
			return ENOMEM;
		}

		/* Copies the lines out of the growing vector. */
		memcpy(box->lines, lines.items, lines.count * sizeof(struct layout_line));
	}

	/* The lines now live in the arena. */
	wb_vector_release(&lines);

	/* The block is as tall as its lines. */
	box->height = cursor;

	/* Succeeded: the lines are stored. */
	return 0;
}

/*
 * Makes one line of pieces [start, end): its fragments, their vertical
 * alignment, the line's height, baseline and horizontal alignment, and
 * the places of the inline blocks on it.
 */
static int
inline_finish_line(
	struct layout_tree *tree,
	struct layout_box *box,
	const struct inline_piece *pieces,
	size_t start,
	size_t end,
	layout_unit *cursor,
	struct wb_vector *lines,
	int first_line,
	layout_unit line_left,
	layout_unit line_width)
{
	struct layout_line line;
	struct layout_fragment *fragment;
	struct inline_reach *reach;
	struct text_metrics metrics;
	struct text_font font;
	struct text_glyph glyph;
	layout_unit above;
	layout_unit below;
	layout_unit x;
	layout_unit room;
	size_t count;
	size_t piece_count;
	size_t index;
	int align;
	int error;

	/* Trailing spaces hang past the line and are dropped. */
	while (end > start && pieces[end - 1U].space)
		end--;

	/* The strut: the block's own font and line height. */
	inline_line_height(tree, &box->style, &above, &below);

	/* Counts the fragments: every piece but the forced breaks. */
	memset(&line, 0, sizeof(line));
	count = 0;
	for (index = start; index < end; index++) {
		if (!pieces[index].forced_break)
			count++;
	}

	/* A list item's first line holds its marker too. */
	if (first_line)
		count++;

	/* Allocates the fragments of a line that has any. */
	reach = NULL;
	if (count != 0) {
		line.fragments = wb_arena_zalloc(&tree->arena, count * sizeof(struct layout_fragment));
		if (line.fragments == NULL)
			return ENOMEM;

		/* And how far each reaches, which the alignment reads (the marker reaches nowhere). */
		reach = calloc(count, sizeof(*reach));
		if (reach == NULL)
			return ENOMEM;
	}

	/* Sets one fragment per piece along the line, and how far it reaches above and below its baseline. */
	x = 0;
	for (index = start; index < end; index++) {
		/* A forced break has no fragment. */
		if (pieces[index].forced_break)
			continue;

		/* The fragment's text, font and place. */
		fragment = &line.fragments[line.fragment_count];
		line.fragment_count++;
		fragment->box = pieces[index].box;
		fragment->text = pieces[index].text;
		fragment->length = pieces[index].length;
		fragment->font = pieces[index].font;
		fragment->color = pieces[index].box->style.color;
		fragment->underline = pieces[index].box->style.underline;
		fragment->x = x;
		fragment->width = pieces[index].width;

		/*
		 * A replaced box stands on the baseline: all of its margin box is above
		 * it, unless it is a control with text, whose text's baseline is the
		 * line's.
		 */
		if (pieces[index].replaced) {
			inline_replaced_extent(pieces[index].box, &fragment->ascent, &fragment->descent);
			reach[line.fragment_count - 1U].above = fragment->ascent;
			reach[line.fragment_count - 1U].below = fragment->descent;
			x += pieces[index].width;
			continue;
		}

		/* The font's ascent and descent, which the painting places the glyphs by. */
		error = text_font_metrics(tree->text, &pieces[index].font, &metrics);
		if (error != 0) {
			free(reach);
			return error;
		}

		/* In layout units. */
		fragment->ascent = (layout_unit)metrics.ascent * LAYOUT_UNIT;
		fragment->descent = (layout_unit)metrics.descent * LAYOUT_UNIT;

		/* The piece's line reaches above and below its baseline. */
		inline_piece_extent(tree, &pieces[index], &reach[line.fragment_count - 1U].above, &reach[line.fragment_count - 1U].below);

		/* The next piece starts where this one ends. */
		x += pieces[index].width;
	}

	/* The pieces' fragments come before the marker's. */
	piece_count = line.fragment_count;

	/* A list item's marker hangs to the left of its first line. */
	if (first_line && box->marker_length != 0) {
		fragment = &line.fragments[line.fragment_count];
		line.fragment_count++;
		layout_font_of(tree, &box->style, &font);
		fragment->box = box;
		fragment->text = box->marker;
		fragment->length = box->marker_length;
		fragment->font = font;
		fragment->color = box->style.color;
		fragment->width = 0;

		/* The marker is as wide as its characters. */
		for (index = 0; index < box->marker_length; index++) {
			error = text_glyph(tree->text, &font, box->marker[index], 0, &glyph);
			if (error != 0) {
				free(reach);
				return error;
			}

			/* The character widens the marker. */
			fragment->width += glyph.advance_units;
		}

		/* It ends a space's width before the content's left edge. */
		error = text_glyph(tree->text, &font, 0x20U, 0, &glyph);
		if (error != 0) {
			free(reach);
			return error;
		}

		/* Its right edge is that space before the content. */
		fragment->x = -(fragment->width + glyph.advance_units);

		/* The font's ascent and descent, which the painting places the glyphs by. */
		error = text_font_metrics(tree->text, &font, &metrics);
		if (error != 0) {
			free(reach);
			return error;
		}

		/* In layout units. */
		fragment->ascent = (layout_unit)metrics.ascent * LAYOUT_UNIT;
		fragment->descent = (layout_unit)metrics.descent * LAYOUT_UNIT;
	}

	/* vertical-align places the pieces, and the line grows to hold them over the strut. */
	inline_align(tree, box, &line, reach, piece_count, &above, &below);
	free(reach);

	/* Aligns the line in the room the floats leave it: start and end are the right and the left in a right-to-left block. */
	room = line_width - x;
	line.left = line_left;
	align = box->style.text_align;
	if (align == CSS_TEXT_ALIGN_START || align == CSS_TEXT_ALIGN_JUSTIFY) {
		align = CSS_TEXT_ALIGN_LEFT;
		if (box->style.direction == CSS_DIRECTION_RTL)
			align = CSS_TEXT_ALIGN_RIGHT;
	} else if (align == CSS_TEXT_ALIGN_END) {
		align = CSS_TEXT_ALIGN_RIGHT;
		if (box->style.direction == CSS_DIRECTION_RTL)
			align = CSS_TEXT_ALIGN_LEFT;
	}

	/* The room goes half to each side, or all to the left. */
	if (room > 0 && align == CSS_TEXT_ALIGN_CENTER)
		line.left = line_left + room / 2;
	if (room > 0 && align == CSS_TEXT_ALIGN_RIGHT)
		line.left = line_left + room;

	/* Stacks the line under the ones before. */
	line.y = *cursor;
	line.height = above + below;
	line.baseline = above;
	*cursor += line.height;

	/* The inline blocks on the line now know where they are. */
	inline_place_atomics(&line);

	/* Adds it. */
	error = wb_vector_push(lines, &line);
	if (error != 0)
		return ENOMEM;

	/* Succeeded: the line is made. */
	return 0;
}

/*
 * Aligns the pieces of a line vertically (vertical-align): each piece's
 * shift from the line's baseline, and how far the line reaches above and
 * below its baseline (above and below come in as the strut's).  The
 * pieces aligned with the baseline come first; a piece aligned with the
 * line's top or bottom then makes the line tall enough for it and is put
 * at that edge.
 */
static void
inline_align(
	struct layout_tree *tree,
	struct layout_box *box,
	struct layout_line *line,
	struct inline_reach *reach,
	size_t count,
	layout_unit *above,
	layout_unit *below)
{
	struct layout_fragment *fragment;
	const struct css_style *style;
	layout_unit top_height;
	layout_unit bottom_height;
	layout_unit height;
	size_t index;
	int align;

	/* The pieces placed from the baseline raise the line; those at its top or bottom wait. */
	top_height = 0;
	bottom_height = 0;
	for (index = 0; index < count; index++) {
		fragment = &line->fragments[index];
		height = reach[index].above + reach[index].below;

		/* The piece's alignment: the baseline, unless a style around it says otherwise. */
		align = CSS_VALIGN_BASELINE;
		style = inline_align_style(fragment, box);
		if (style != NULL)
			align = style->vertical_align;

		/* A piece at the line's top or bottom needs the line at least as tall as itself. */
		if (align == CSS_VALIGN_TOP) {
			if (height > top_height)
				top_height = height;
			continue;
		}

		/* Likewise a piece at its bottom. */
		if (align == CSS_VALIGN_BOTTOM) {
			if (height > bottom_height)
				bottom_height = height;
			continue;
		}

		/* The shift from the baseline, and how far the shifted piece reaches. */
		fragment->shift = 0;
		if (style != NULL)
			fragment->shift = inline_shift(tree, box, style, &reach[index]);
		if (reach[index].above - fragment->shift > *above)
			*above = reach[index].above - fragment->shift;
		if (reach[index].below + fragment->shift > *below)
			*below = reach[index].below + fragment->shift;
	}

	/* A piece at the top grows the line downwards, one at the bottom upwards. */
	if (top_height > *above + *below)
		*below = top_height - *above;
	if (bottom_height > *above + *below)
		*above = bottom_height - *below;

	/* The pieces at the top and the bottom go to those edges. */
	for (index = 0; index < count; index++) {
		fragment = &line->fragments[index];
		style = inline_align_style(fragment, box);
		if (style == NULL)
			continue;

		/* Its top on the line's top, or its bottom on the line's bottom. */
		if (style->vertical_align == CSS_VALIGN_TOP)
			fragment->shift = reach[index].above - *above;
		if (style->vertical_align == CSS_VALIGN_BOTTOM)
			fragment->shift = *below - reach[index].below;
	}
}

/*
 * Picks the style whose vertical-align moves a fragment, or NULL for the
 * baseline: an atomic piece's own; for text, that of the nearest inline
 * box around it that leaves the baseline (text straight in the block
 * stays on the baseline, since vertical-align does not apply to blocks).
 */
static const struct css_style *
inline_align_style(
	const struct layout_fragment *fragment,
	const struct layout_box *block)
{
	const struct layout_box *ancestor;

	/* A replaced box or an inline block is aligned by its own style. */
	if (fragment->box->kind != LAYOUT_TEXT) {
		if (fragment->box->style.vertical_align == CSS_VALIGN_BASELINE)
			return NULL;
		return &fragment->box->style;
	}

	/* Text: the inline boxes around it, up to the block. */
	for (ancestor = fragment->box->parent; ancestor != NULL && ancestor != block; ancestor = ancestor->parent) {
		if (ancestor->kind != LAYOUT_INLINE)
			break;
		if (ancestor->style.vertical_align != CSS_VALIGN_BASELINE)
			return &ancestor->style;
	}

	/* No inline box around the text leaves the baseline. */
	return NULL;
}

/*
 * Finds how far a piece's baseline sits below the line's (negative: above)
 * for vertical-align relative to the parent, which is the block holding
 * the line in this pass.
 */
static layout_unit
inline_shift(
	struct layout_tree *tree,
	const struct layout_box *box,
	const struct css_style *style,
	const struct inline_reach *reach)
{
	struct text_metrics metrics;
	struct text_font font;
	layout_unit x_height;
	layout_unit line_above;
	layout_unit line_below;
	layout_unit shift;
	int error;

	/* Each keyword moves the piece its own way. */
	shift = 0;
	switch (style->vertical_align) {
	case CSS_VALIGN_MIDDLE:
		/* The piece's middle goes to half the parent's x-height above the baseline. */
		x_height = inline_x_height(tree, &box->style);
		shift = (reach->above - reach->below) / 2 - x_height / 2;
		break;
	case CSS_VALIGN_TEXT_TOP:
	case CSS_VALIGN_TEXT_BOTTOM:
		/* The parent's font's ascent and descent are its text's top and bottom. */
		layout_font_of(tree, &box->style, &font);
		error = text_font_metrics(tree->text, &font, &metrics);
		if (error != 0)
			break;

		/* The piece's top goes to the text's top, or its bottom to the text's bottom. */
		if (style->vertical_align == CSS_VALIGN_TEXT_TOP) {
			shift = reach->above - (layout_unit)metrics.ascent * LAYOUT_UNIT;
		} else {
			shift = (layout_unit)metrics.descent * LAYOUT_UNIT - reach->below;
		}

		/* The shift is found. */
		break;
	case CSS_VALIGN_SUB:
		/* Lowered by a fifth of the parent's font size, as Chromium does. */
		shift = layout_from_px(box->style.font_size / 5.0f + 1.0f);
		break;
	case CSS_VALIGN_SUPER:
		/* Raised by a third of the parent's font size, as Chromium does. */
		shift = -layout_from_px(box->style.font_size / 3.0f + 1.0f);
		break;
	case CSS_VALIGN_LENGTH:
		/* Raised by a length, or by a percentage of the piece's own line height. */
		if (style->vertical_offset.unit == CSS_UNIT_PX) {
			shift = -layout_from_px(style->vertical_offset.value);
		} else if (style->vertical_offset.unit == CSS_UNIT_PERCENT) {
			inline_line_height(tree, style, &line_above, &line_below);
			shift = -(layout_unit)((float)(line_above + line_below) * style->vertical_offset.value / 100.0f);
		}

		/* The shift is found. */
		break;
	default:
		break;
	}

	/* Reports the shift. */
	return shift;
}

/*
 * Measures a style's font's x-height: how far the top of an "x" is above
 * the baseline, or half the font size when the glyph cannot be drawn.
 */
static layout_unit
inline_x_height(
	struct layout_tree *tree,
	const struct css_style *style)
{
	struct text_font font;
	struct text_glyph glyph;
	int error;

	/* The drawn "x" of the style's font. */
	layout_font_of(tree, style, &font);
	error = text_glyph(tree->text, &font, 'x', 1, &glyph);
	if (error != 0 || glyph.top <= 0)
		return layout_from_px(style->font_size / 2.0f);

	/* Its top above the baseline. */
	return (layout_unit)glyph.top * LAYOUT_UNIT;
}

/*
 * Places the inline blocks of a stacked line: each border box relative to
 * the block's content box, from the line's left and its piece's baseline.
 */
static void
inline_place_atomics(
	const struct layout_line *line)
{
	const struct layout_fragment *fragment;
	struct layout_box *atomic;
	size_t index;

	/* Each inline block's margin box starts at its piece, its top its reach above the piece's baseline. */
	for (index = 0; index < line->fragment_count; index++) {
		fragment = &line->fragments[index];
		atomic = fragment->box;
		if (!atomic->atomic)
			continue;
		atomic->x = line->left + fragment->x + atomic->margin[CSS_LEFT];
		atomic->y = line->y + line->baseline + fragment->shift - fragment->ascent + atomic->margin[CSS_TOP];
	}
}

/* Measures how far a style's line reaches above and below the baseline, half-leading included. */
static void
inline_line_height(
	struct layout_tree *tree,
	const struct css_style *style,
	layout_unit *above,
	layout_unit *below)
{
	struct text_font font;

	/* The style's own font under the style's line-height. */
	layout_font_of(tree, style, &font);
	inline_font_extent(tree, &font, style, above, below);
}

/*
 * Measures how far a piece's line reaches above and below the baseline.
 *
 * With line-height: normal, a word drawn partly in the fallback face also
 * reaches as far as that face's own line does, as in Chromium, where every
 * font a run uses counts toward a normal line's height.
 */
static void
inline_piece_extent(
	struct layout_tree *tree,
	const struct inline_piece *piece,
	layout_unit *above,
	layout_unit *below)
{
	struct text_font fallback;
	layout_unit fallback_above;
	layout_unit fallback_below;

	/* The line of the piece's own style. */
	inline_line_height(tree, &piece->box->style, above, below);

	/* A given line height is not raised by the fonts used. */
	if (piece->box->style.line_height.unit != CSS_UNIT_NORMAL)
		return;

	/* A piece in its own face alone needs nothing more. */
	if (!piece->uses_fallback)
		return;

	/* The fallback face at the piece's size raises the extent where it reaches further. */
	fallback = piece->font;
	fallback.face = TEXT_FACE_FALLBACK;
	inline_font_extent(tree, &fallback, &piece->box->style, &fallback_above, &fallback_below);
	if (fallback_above > *above)
		*above = fallback_above;
	if (fallback_below > *below)
		*below = fallback_below;
}

/* Measures how far a font's line reaches above and below the baseline under a style's line-height. */
static void
inline_font_extent(
	struct layout_tree *tree,
	const struct text_font *font,
	const struct css_style *style,
	layout_unit *above,
	layout_unit *below)
{
	struct text_metrics metrics;
	layout_unit content;
	layout_unit height;
	layout_unit leading;
	int error;

	/* The font's measures, or the font size when the font cannot be measured. */
	error = text_font_metrics(tree->text, font, &metrics);
	if (error != 0) {
		metrics.ascent = (int)style->font_size;
		metrics.descent = 0;
		metrics.line_height = (int)style->font_size;
	}

	/* The glyphs' own height, from the ascent to the descent. */
	content = (layout_unit)(metrics.ascent + metrics.descent) * LAYOUT_UNIT;

	/* The used line height: normal, a multiple of the font size, or a length. */
	height = (layout_unit)metrics.line_height * LAYOUT_UNIT;
	if (style->line_height.unit == CSS_UNIT_NUMBER)
		height = layout_from_px(style->line_height.value * style->font_size);
	if (style->line_height.unit == CSS_UNIT_PX)
		height = layout_from_px(style->line_height.value);

	/* Half the leading goes above, half below. */
	leading = height - content;
	*above = (layout_unit)metrics.ascent * LAYOUT_UNIT + leading / 2;
	*below = height - *above;
}

/* Lays out and places the floats among a block's inline content (not inside boxes out of the flow, inline blocks or other floats). */
static int
inline_place_floats(
	struct layout_tree *tree,
	struct layout_box *content,
	struct layout_box *box,
	int depth)
{
	struct layout_box *child;
	layout_unit origin_x;
	layout_unit origin_y;
	int error;

	/* Stops at the depth the layout stops at. */
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/* Each float shrinks to fit and goes to its side at the content's top; inline boxes are searched through. */
	origin_x = tree->origin_x;
	origin_y = tree->origin_y;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow || child->atomic)
			continue;
		if (child->floating == CSS_FLOAT_NONE) {
			error = inline_place_floats(tree, content, child, depth + 1);
			if (error != 0)
				return error;
			continue;
		}

		/* The float's own layout, then its place. */
		error = layout_shrink_to_fit(tree, child, content->width);
		tree->origin_x = origin_x;
		tree->origin_y = origin_y;
		if (error == 0)
			error = layout_place_float(tree, child, 0, content->width);
		if (error != 0)
			return error;
	}

	/* Succeeded: the floats are placed. */
	return 0;
}

/* Finds the room of a line starting at cursor: its left edge and width between the floats beside a line's height. */
static void
inline_room(
	struct layout_tree *tree,
	struct layout_box *box,
	layout_unit cursor,
	layout_unit *left,
	layout_unit *width)
{
	layout_unit above;
	layout_unit below;
	layout_unit right;

	/* The band of a line of the block's own font. */
	inline_line_height(tree, &box->style, &above, &below);
	layout_line_room(tree, cursor, cursor + above + below, box->width, left, &right);

	/* The width between the edges. */
	*width = right - *left;
	if (*width < 0)
		*width = 0;
}
