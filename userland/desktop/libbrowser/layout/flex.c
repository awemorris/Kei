/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Flexible box layout (ws074-p035), the common part of CSS Flexible Box
 * Layout 1: rows and columns (and their reverses), wrapping into lines,
 * the flex factors and basis, gaps, auto margins, justify-content,
 * align-items and align-self.  Each item is laid out as a block at the
 * main size the flexing gives it.  Not in this pass: align-content other
 * than start, wrap-reverse's order of lines, baselines (as start),
 * percentages of an indefinite height, and a column's automatic minimum
 * size (its items shrink to zero at most).  A row's item whose min-width
 * is auto (the initial value) and whose content does not scroll shrinks
 * no narrower than its content's min-content width, or its width when
 * that is given in pixels and narrower (the automatic minimum size,
 * ws074-p084), measured only for a line that has to shrink.
 */

#include "layout/layout.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*
 * One item on its way through the algorithm: its box, the frame (borders
 * and paddings) and margins along the main axis, which of those margins
 * are auto, its hypothetical and then its final content size along the
 * main axis, and the limits of that size.
 */
struct flex_item {
	struct layout_box *box;
	layout_unit frame;
	layout_unit margin_start;
	layout_unit margin_end;
	int auto_start;
	int auto_end;
	layout_unit base;
	layout_unit size;
	layout_unit minimum;
	layout_unit maximum;
	int auto_minimum;
	int frozen;
};

/*
 * The part of an item's style that its layout at the flexed size
 * overrides, kept to be put back afterwards.
 */
struct flex_sizes {
	int box_sizing;
	struct css_length width;
	struct css_length min_width;
	struct css_length max_width;
	struct css_length height;
	struct css_length min_height;
	struct css_length max_height;
	struct css_length margin[4];
};

static int flex_collect(struct layout_box *box, struct flex_item **items, size_t *count);
static int flex_base_size(struct layout_tree *tree, struct layout_box *box, struct flex_item *item, int row, layout_unit available);
static int flex_auto_minimums(struct layout_tree *tree, struct flex_item *items, size_t count, layout_unit available, layout_unit gap);
static int flex_min_content(struct layout_tree *tree, struct layout_box *box, layout_unit *width);
static void flex_shrink(struct flex_item *items, size_t count, layout_unit available, layout_unit gap);
static void flex_resolve(struct flex_item *items, size_t count, layout_unit available, layout_unit gap);
static int flex_lay_item(struct layout_tree *tree, struct layout_box *box, struct flex_item *item, int row);
static int flex_lay_sized(struct layout_tree *tree, struct layout_box *box, struct flex_item *item, int row);
static int flex_relay_cross(struct layout_tree *tree, struct layout_box *box, struct flex_item *items, size_t count, int row, layout_unit line_cross, layout_unit percentage_basis);
static void flex_place_line(struct layout_box *box, struct flex_item *items, size_t count, int row, layout_unit available, layout_unit gap, layout_unit cross_start, layout_unit line_cross);
static layout_unit flex_cross_outer(const struct layout_box *item, int row);
static int flex_align_of(const struct layout_box *box, const struct layout_box *item);
static layout_unit flex_length(const struct css_length *length, layout_unit basis);

/*
 * Lays out the content of a flex container: its in-flow children as flex
 * items, in lines along its main axis; sets their positions (their border
 * boxes relative to the container's content box) and the container's
 * content height.
 */
int
layout_flex(
	struct layout_tree *tree,
	struct layout_box *box)
{
	struct flex_item *items;
	struct layout_box *child;
	layout_unit available;
	layout_unit gap_main;
	layout_unit gap_cross;
	layout_unit used;
	layout_unit outer;
	layout_unit cross;
	layout_unit line_cross;
	layout_unit cross_cursor;
	layout_unit sizing;
	layout_unit percentage_basis;
	size_t count;
	size_t start;
	size_t end;
	size_t index;
	int row;
	int wrap;
	int error;

	/* The children out of the flow have their static place at the content box's corner. */
	for (child = box->first_child; child != NULL; child = child->next) {
		if (!child->out_of_flow)
			continue;
		child->static_x = 0;
		child->static_y = 0;
	}

	/* The items, in their order. */
	error = flex_collect(box, &items, &count);
	if (error != 0)
		return error;

	/* The axes: the main size available (a column's only with a height of its own), and the gaps. */
	row = 0;
	if (box->style.flex_direction == CSS_FLEX_ROW || box->style.flex_direction == CSS_FLEX_ROW_REVERSE)
		row = 1;
	wrap = 0;
	if (box->style.flex_wrap != CSS_FLEX_NOWRAP)
		wrap = 1;
	available = box->width;
	gap_main = flex_length(&box->style.column_gap, box->width);
	gap_cross = flex_length(&box->style.row_gap, box->width);
	if (!row) {
		available = -1;
		if (box->style.height.unit == CSS_UNIT_PX) {
			sizing = 0;
			if (box->style.box_sizing == CSS_BOX_SIZING_BORDER)
				sizing = box->border[CSS_TOP] + box->padding[CSS_TOP] + box->padding[CSS_BOTTOM] + box->border[CSS_BOTTOM];
			available = layout_from_px(box->style.height.value) - sizing;
		}

		/* A column's main axis is vertical: the gaps swap. */
		gap_main = flex_length(&box->style.row_gap, box->width);
		gap_cross = flex_length(&box->style.column_gap, box->width);
	}

	/* Each item's hypothetical main size. */
	for (index = 0; index < count; index++) {
		error = flex_base_size(tree, box, &items[index], row, available);
		if (error != 0) {
			free(items);
			return error;
		}
	}

	/* Line by line: the items that fit, flexed, laid out and placed. */
	cross_cursor = 0;
	start = 0;
	while (start < count) {
		/* The items of the line: all of them without wrapping or a size to wrap in. */
		end = start;
		used = 0;
		while (end < count) {
			outer = items[end].base + items[end].frame + items[end].margin_start + items[end].margin_end;
			if (end > start)
				outer += gap_main;
			if (wrap && available >= 0 && end > start && used + outer > available)
				break;
			used += outer;
			end++;
		}

		/* The automatic minimums of a line that has to shrink. */
		error = flex_auto_minimums(tree, items + start, end - start, available, gap_main);
		if (error != 0) {
			free(items);
			return error;
		}

		/* The free space shared, then each item laid out at its size. */
		flex_resolve(items + start, end - start, available, gap_main);
		line_cross = 0;
		for (index = start; index < end; index++) {
			error = flex_lay_item(tree, box, &items[index], row);
			if (error != 0) {
				free(items);
				return error;
			}

			/* The line is as thick as its thickest item. */
			cross = flex_cross_outer(items[index].box, row);
			if (cross > line_cross)
				line_cross = cross;
		}

		/* A single line of a row fills a container of a height of its own; a column's line fills its width. */
		if (!wrap && row && box->style.height.unit == CSS_UNIT_PX) {
			sizing = 0;
			if (box->style.box_sizing == CSS_BOX_SIZING_BORDER)
				sizing = box->border[CSS_TOP] + box->padding[CSS_TOP] + box->padding[CSS_BOTTOM] + box->border[CSS_BOTTOM];
			line_cross = layout_from_px(box->style.height.value) - sizing;
		}

		/* A column's single line is as wide as the container. */
		if (!wrap && !row)
			line_cross = box->width;

		/* Stretch and a definite percentage cross size must reach the item's descendants too. */
		percentage_basis = -1;
		if (!wrap && row && box->style.height.unit == CSS_UNIT_PX)
			percentage_basis = line_cross;
		error = flex_relay_cross(tree, box, items + start, end - start, row, line_cross, percentage_basis);
		if (error != 0) {
			free(items);
			return error;
		}

		/* The items' places along both axes. */
		if (cross_cursor > 0)
			cross_cursor += gap_cross;
		flex_place_line(box, items + start, end - start, row, available, gap_main, cross_cursor, line_cross);
		cross_cursor += line_cross;
		start = end;
	}

	/* The container's content height: its lines' (a row's), or its items' (a column's). */
	box->height = cross_cursor;
	if (!row) {
		box->height = 0;
		for (index = 0; index < count; index++) {
			child = items[index].box;
			outer = child->y + child->border[CSS_TOP] + child->padding[CSS_TOP] + child->height +
			    child->padding[CSS_BOTTOM] + child->border[CSS_BOTTOM] + child->margin[CSS_BOTTOM];
			if (outer > box->height)
				box->height = outer;
		}
	}

	/* The items are placed. */
	free(items);

	/* Succeeded: the container's content is laid out. */
	return 0;
}

/*
 * Measures the width a laid out row flex container's content needs: its
 * items' hypothetical margin boxes side by side, with the gaps between.
 */
layout_unit
layout_flex_content_width(
	const struct layout_box *box)
{
	const struct layout_box *child;
	layout_unit width;
	layout_unit gap;
	int items;

	/* Each item's margin box before it flexed. */
	width = 0;
	items = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow)
			continue;
		width += child->flex_hypothetical;
		items++;
	}

	/* The gaps between them. */
	gap = flex_length(&box->style.column_gap, 0);
	if (items > 1)
		width += gap * (items - 1);

	/* Reports the width. */
	return width;
}

/* Gathers a container's in-flow children, stably sorted by order, into a new array. */
static int
flex_collect(
	struct layout_box *box,
	struct flex_item **items,
	size_t *count)
{
	struct flex_item *list;
	struct flex_item moving;
	struct layout_box *child;
	size_t made;
	size_t index;
	size_t place;

	/* Counts the in-flow children. */
	made = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (!child->out_of_flow)
			made++;
	}

	/* The array (one place at least, so that none is not NULL). */
	list = calloc(made + 1U, sizeof(*list));
	if (list == NULL)
		return ENOMEM;

	/* Each child, inserted after the ones of the same or a smaller order. */
	made = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow)
			continue;
		list[made].box = child;
		place = made;
		while (place > 0 && list[place - 1U].box->style.order > child->style.order) {
			moving = list[place - 1U];
			list[place] = moving;
			place--;
		}

		/* The child takes the place made for it. */
		list[place].box = child;
		made++;
	}

	/* The other fields start empty. */
	for (index = 0; index < made; index++) {
		child = list[index].box;
		memset(&list[index], 0, sizeof(list[index]));
		list[index].box = child;
	}

	/* Succeeded: the items are gathered. */
	*items = list;
	*count = made;
	return 0;
}

/*
 * Finds an item's frame and margins along the main axis and its
 * hypothetical main size: its flex basis (or its width or height), or its
 * content's size, held between its minimum and maximum.
 */
static int
flex_base_size(
	struct layout_tree *tree,
	struct layout_box *box,
	struct flex_item *item,
	int row,
	layout_unit available)
{
	struct layout_box *child;
	const struct css_length *basis;
	layout_unit sizing;
	int start_side;
	int end_side;
	int error;

	/* The item's box model against the container's width. */
	child = item->box;
	layout_box_model(child, box->width);

	/* The sides the main axis starts and ends at: left and right for a row, top and bottom for a column. */
	start_side = CSS_TOP;
	end_side = CSS_BOTTOM;
	if (row) {
		start_side = CSS_LEFT;
		end_side = CSS_RIGHT;
	}

	/* Its frame and margins along the main axis (an auto margin counts as none until the free space is shared). */
	item->frame = child->border[start_side] + child->padding[start_side] + child->padding[end_side] + child->border[end_side];
	item->margin_start = child->margin[start_side];
	item->margin_end = child->margin[end_side];

	/* An auto margin at the start takes a share of the free space later. */
	item->auto_start = 0;
	if (child->style.margin[start_side].unit == CSS_UNIT_AUTO)
		item->auto_start = 1;

	/* So does one at the end. */
	item->auto_end = 0;
	if (child->style.margin[end_side].unit == CSS_UNIT_AUTO)
		item->auto_end = 1;

	/* Under border-box sizing the given sizes include the frame. */
	sizing = 0;
	if (child->style.box_sizing == CSS_BOX_SIZING_BORDER)
		sizing = item->frame;

	/* The basis: flex-basis, or the width or height when it is auto. */
	basis = &child->style.flex_basis;
	if (basis->unit == CSS_UNIT_AUTO) {
		basis = &child->style.width;
		if (!row)
			basis = &child->style.height;
	}

	/*
	 * A definite basis: pixels, or a percentage of a definite main size (a
	 * container being measured has none: its width is only the measuring
	 * width).
	 */
	item->base = -1;
	if (basis->unit == CSS_UNIT_PX)
		item->base = layout_from_px(basis->value) - sizing;
	if (basis->unit == CSS_UNIT_PERCENT && available >= 0 && tree->measuring == 0)
		item->base = flex_length(basis, available) - sizing;

	/* Otherwise the content's size: its widest line, or its height at the container's width. */
	if (item->base < 0 && row) {
		if (child->replaced) {
			error = layout_block(tree, child, box->width);
			if (error != 0)
				return error;
			item->base = child->width;
		} else {
			/* The content's width without a limit (measured once, its percentages indefinite). */
			error = layout_max_content(tree, child, &item->base);
			if (error != 0)
				return error;
		}
	} else if (item->base < 0) {
		error = layout_block(tree, child, box->width);
		if (error != 0)
			return error;
		item->base = child->height;
	}

	/* A size is never negative. */
	if (item->base < 0)
		item->base = 0;

	/* The limits: min-width and max-width (min-height and max-height for a column). */
	item->minimum = 0;
	item->maximum = -1;
	item->auto_minimum = 0;
	if (row &&
	    child->style.min_width.unit == CSS_UNIT_AUTO &&
	    (child->style.overflow_x == CSS_OVERFLOW_VISIBLE || child->style.overflow_x == CSS_OVERFLOW_CLIP))
		item->auto_minimum = 1;
	if (row) {
		if (child->style.min_width.unit == CSS_UNIT_PX || child->style.min_width.unit == CSS_UNIT_PERCENT)
			item->minimum = flex_length(&child->style.min_width, box->width) - sizing;
		if (child->style.max_width.unit == CSS_UNIT_PX || child->style.max_width.unit == CSS_UNIT_PERCENT)
			item->maximum = flex_length(&child->style.max_width, box->width) - sizing;
	} else {
		if (child->style.min_height.unit == CSS_UNIT_PX)
			item->minimum = layout_from_px(child->style.min_height.value) - sizing;
		if (child->style.max_height.unit == CSS_UNIT_PX)
			item->maximum = layout_from_px(child->style.max_height.value) - sizing;
	}

	/* Neither is the minimum. */
	if (item->minimum < 0)
		item->minimum = 0;

	/* The hypothetical size is the base within the limits. */
	if (item->maximum >= 0 && item->base > item->maximum)
		item->base = item->maximum;
	if (item->base < item->minimum)
		item->base = item->minimum;
	item->size = item->base;

	/* The container's measure of its content counts the item's margin box at that size. */
	child->flex_hypothetical = item->base + item->frame + item->margin_start + item->margin_end;

	/* Succeeded: the item has its hypothetical size. */
	return 0;
}

/*
 * Shrinks the items of a line that do not fit (CSS Flexbox 9.7): the
 * space missing is taken from the items that are not frozen, in
 * proportion to their flex-shrink times their base; an item that would go
 * below its minimum is held there and frozen, and the rest share what is
 * still missing, until no item goes below its minimum.
 */
static void
flex_shrink(
	struct flex_item *items,
	size_t count,
	layout_unit available,
	layout_unit gap)
{
	layout_unit used;
	layout_unit missing;
	layout_unit share;
	float total;
	float weight;
	size_t index;
	size_t round;
	int violated;

	/* No item is frozen yet. */
	for (index = 0; index < count; index++)
		items[index].frozen = 0;

	/* At most one round per item, as each round freezes one at least. */
	for (round = 0; round <= count; round++) {
		/* The space the frozen items take at their sizes and the others at their bases. */
		used = 0;
		total = 0;
		for (index = 0; index < count; index++) {
			used += items[index].frame + items[index].margin_start + items[index].margin_end;
			if (items[index].frozen) {
				used += items[index].size;
			} else {
				used += items[index].base;
				total += items[index].box->style.flex_shrink * (float)items[index].base;
			}
		}

		/* The gaps take room too, and the rest is what is missing. */
		if (count > 1)
			used += gap * (layout_unit)(count - 1U);
		missing = used - available;

		/* Enough room now, or nothing left that shrinks: the others keep their bases. */
		if (missing <= 0 || total <= 0) {
			for (index = 0; index < count; index++) {
				if (!items[index].frozen)
					items[index].size = items[index].base;
			}

			/* The sizes stand. */
			return;
		}

		/* Each other item's part of the missing space; the ones below their minimums are held and frozen. */
		violated = 0;
		for (index = 0; index < count; index++) {
			if (items[index].frozen)
				continue;
			weight = items[index].box->style.flex_shrink * (float)items[index].base;
			share = (layout_unit)((float)missing * weight / total);
			items[index].size = items[index].base - share;
			if (items[index].size < items[index].minimum) {
				items[index].size = items[index].minimum;
				items[index].frozen = 1;
				violated = 1;
			}
		}

		/* No item held: the sizes stand. */
		if (!violated)
			return;
	}
}

/*
 * Finds the automatic minimum size of the items of a line whose items do
 * not fit (the others keep a minimum of zero, as they do not shrink): the
 * content's min-content width, or the width given in pixels when that is
 * narrower, within max-width.
 */
static int
flex_auto_minimums(
	struct layout_tree *tree,
	struct flex_item *items,
	size_t count,
	layout_unit available,
	layout_unit gap)
{
	struct layout_box *child;
	layout_unit used;
	layout_unit least;
	layout_unit given;
	size_t index;
	int error;

	/* A line without a definite size, or that fits, does not shrink. */
	if (available < 0)
		return 0;
	used = 0;
	for (index = 0; index < count; index++)
		used += items[index].base + items[index].frame + items[index].margin_start + items[index].margin_end;
	if (count > 1)
		used += gap * (layout_unit)(count - 1U);
	if (used <= available)
		return 0;

	/* Each item whose minimum is automatic. */
	for (index = 0; index < count; index++) {
		if (!items[index].auto_minimum)
			continue;
		child = items[index].box;

		/* The content's narrowest width. */
		error = flex_min_content(tree, child, &least);
		if (error != 0)
			return error;

		/* A width in pixels that is narrower is the minimum instead. */
		if (child->style.width.unit == CSS_UNIT_PX) {
			given = layout_from_px(child->style.width.value);
			if (child->style.box_sizing == CSS_BOX_SIZING_BORDER)
				given -= items[index].frame;
			if (given < least)
				least = given;
		}

		/* Within max-width, and never above the base the item would shrink from. */
		if (items[index].maximum >= 0 && least > items[index].maximum)
			least = items[index].maximum;
		if (least > items[index].base)
			least = items[index].base;
		if (least > items[index].minimum)
			items[index].minimum = least;
	}

	/* Succeeded: the minimums are known. */
	return 0;
}

/*
 * Measures a box's min-content width: its content laid out with a line at
 * every opportunity (as the intrinsic min-content width is measured).
 */
static int
flex_min_content(
	struct layout_tree *tree,
	struct layout_box *box,
	layout_unit *width)
{
	struct layout_context context;
	struct wb_vector floats;
	struct css_length given;
	int error;

	/* A replaced box's content is its image, as wide as it is. */
	*width = 0;
	if (box->replaced) {
		*width = box->width;
		return 0;
	}

	/* The content at no width, with an auto width, while the tree measures. */
	given = box->style.width;
	box->style.width.unit = CSS_UNIT_AUTO;
	tree->measuring++;
	layout_context_begin(tree, &context, &floats);
	error = layout_block(tree, box, 0);
	layout_context_end(tree, &context);
	tree->measuring--;
	box->style.width = given;
	if (error != 0)
		return error;

	/* Succeeded: the widest thing that could not be broken. */
	*width = layout_content_width(box, 0);
	return 0;
}

/*
 * Shares a line's free space among its items: growing by flex-grow when
 * there is room, shrinking by flex-shrink weighted by the basis when there
 * is too little (each once, held at its limits), then giving what is left
 * to auto margins.  Without a definite main size nothing flexes.
 */
static void
flex_resolve(
	struct flex_item *items,
	size_t count,
	layout_unit available,
	layout_unit gap)
{
	layout_unit used;
	layout_unit free_space;
	layout_unit share;
	float total;
	float weight;
	size_t index;
	int autos;

	/* The space the items take as they are. */
	used = 0;
	for (index = 0; index < count; index++)
		used += items[index].size + items[index].frame + items[index].margin_start + items[index].margin_end;
	if (count > 1)
		used += gap * (layout_unit)(count - 1U);
	if (available < 0)
		return;
	free_space = available - used;

	/* Room left: the growing items share it (all of it, or their factors' sum of it when that is under one). */
	if (free_space > 0) {
		total = 0;
		for (index = 0; index < count; index++)
			total += items[index].box->style.flex_grow;
		if (total > 0) {
			weight = total;
			if (weight < 1)
				weight = 1;
			for (index = 0; index < count; index++) {
				share = (layout_unit)((float)free_space * items[index].box->style.flex_grow / weight);
				items[index].size += share;
				if (items[index].maximum >= 0 && items[index].size > items[index].maximum)
					items[index].size = items[index].maximum;
			}
		}
	}

	/* Too little room: the items shrink in proportion to their factors times their bases, those held at their minimums frozen. */
	if (free_space < 0)
		flex_shrink(items, count, available, gap);

	/* What is still free goes to the auto margins. */
	used = 0;
	autos = 0;
	for (index = 0; index < count; index++) {
		used += items[index].size + items[index].frame + items[index].margin_start + items[index].margin_end;
		autos += items[index].auto_start + items[index].auto_end;
	}

	/* The gaps between the items take room too. */
	if (count > 1)
		used += gap * (layout_unit)(count - 1U);
	free_space = available - used;
	if (free_space <= 0 || autos == 0)
		return;

	/* Each auto margin takes an equal part. */
	share = free_space / autos;
	for (index = 0; index < count; index++) {
		if (items[index].auto_start)
			items[index].margin_start += share;
		if (items[index].auto_end)
			items[index].margin_end += share;
	}
}

/*
 * Lays an item out as a block at its final main size, with its style's
 * sizes and margins as they were afterwards: the next layout of the item
 * (the real one after a measurement, or the next line's) starts from its
 * own style again, not from the size this one gave it.
 */
static int
flex_lay_item(
	struct layout_tree *tree,
	struct layout_box *box,
	struct flex_item *item,
	int row)
{
	struct flex_sizes saved;
	struct layout_box *child;
	int error;

	/* Keeps the style's sizes and margins, which the layout at the flexed size overrides. */
	child = item->box;
	saved.box_sizing = child->style.box_sizing;
	saved.width = child->style.width;
	saved.min_width = child->style.min_width;
	saved.max_width = child->style.max_width;
	saved.height = child->style.height;
	saved.min_height = child->style.min_height;
	saved.max_height = child->style.max_height;
	memcpy(saved.margin, child->style.margin, sizeof(saved.margin));

	/* The layout at the flexed size. */
	error = flex_lay_sized(tree, box, item, row);

	/* The style is the item's own again, whatever the layout reported. */
	child->style.box_sizing = saved.box_sizing;
	child->style.width = saved.width;
	child->style.min_width = saved.min_width;
	child->style.max_width = saved.max_width;
	child->style.height = saved.height;
	child->style.min_height = saved.min_height;
	child->style.max_height = saved.max_height;
	memcpy(child->style.margin, saved.margin, sizeof(saved.margin));

	/* Reports an item that could not be laid out. */
	if (error != 0)
		return error;

	/* Succeeded: the item is laid out at its size. */
	return 0;
}

/*
 * Lays an item out as a block at its final main size: its width (a row's
 * item) or height (a column's) set to that size, its main-axis margins to
 * the shared ones; a column's item that does not stretch shrinks to fit.
 */
static int
flex_lay_sized(
	struct layout_tree *tree,
	struct layout_box *box,
	struct flex_item *item,
	int row)
{
	struct layout_box *child;
	layout_unit size;
	int align;
	int error;

	/*
	 * The flexed size is the content box's and its limits are applied
	 * already.  Keep the item's box sizing so that its untouched cross
	 * size retains the author's meaning; express the main size as a border
	 * box when that is what layout_block will consume.
	 */
	child = item->box;
	size = item->size;
	if (child->style.box_sizing == CSS_BOX_SIZING_BORDER)
		size += item->frame;

	/* A row's item: its width and its horizontal margins. */
	if (row) {
		child->style.width.unit = CSS_UNIT_PX;
		child->style.width.value = layout_to_px(size);
		child->style.width.offset = 0;
		child->style.min_width.unit = CSS_UNIT_PX;
		child->style.min_width.value = 0;
		child->style.max_width.unit = CSS_UNIT_NONE;
		child->style.margin[CSS_LEFT].unit = CSS_UNIT_PX;
		child->style.margin[CSS_LEFT].value = layout_to_px(item->margin_start);
		child->style.margin[CSS_LEFT].offset = 0;
		child->style.margin[CSS_RIGHT].unit = CSS_UNIT_PX;
		child->style.margin[CSS_RIGHT].value = layout_to_px(item->margin_end);
		child->style.margin[CSS_RIGHT].offset = 0;
		error = layout_block(tree, child, box->width);
		return error;
	}

	/* A column's item: its height, its vertical margins, and its width by its alignment. */
	child->style.height.unit = CSS_UNIT_PX;
	child->style.height.value = layout_to_px(size);
	child->style.min_height.unit = CSS_UNIT_PX;
	child->style.min_height.value = 0;
	child->style.max_height.unit = CSS_UNIT_NONE;
	child->style.margin[CSS_TOP].unit = CSS_UNIT_PX;
	child->style.margin[CSS_TOP].value = layout_to_px(item->margin_start);
	child->style.margin[CSS_TOP].offset = 0;
	child->style.margin[CSS_BOTTOM].unit = CSS_UNIT_PX;
	child->style.margin[CSS_BOTTOM].value = layout_to_px(item->margin_end);
	child->style.margin[CSS_BOTTOM].offset = 0;
	align = flex_align_of(box, child);
	if (align == CSS_ALIGN_STRETCH || child->style.width.unit != CSS_UNIT_AUTO) {
		error = layout_block(tree, child, box->width);
		return error;
	}

	/* An item that does not stretch is as wide as its content. */
	error = layout_shrink_to_fit(tree, child, box->width);
	if (error != 0)
		return error;

	/* Succeeded: the item is laid out. */
	return 0;
}

/*
 * Lays row items out again after the line's used cross size is known.  A
 * stretched auto-height item used to have only its box height changed at
 * placement time, after its descendants had been laid out.  Giving the
 * item a temporary definite height here lets percentage-height descendants
 * use the stretched size.  A percentage item gets the same treatment when
 * the flex container has a definite height.
 */
static int
flex_relay_cross(
	struct layout_tree *tree,
	struct layout_box *box,
	struct flex_item *items,
	size_t count,
	int row,
	layout_unit line_cross,
	layout_unit percentage_basis)
{
	struct flex_sizes saved;
	struct layout_box *child;
	layout_unit frame;
	layout_unit height;
	size_t index;
	int align;
	int error;

	/* A column's cross axis is width, which the normal width layout already resolves. */
	if (!row)
		return 0;

	/* Each item that needs the definite cross size is laid out again. */
	for (index = 0; index < count; index++) {
		child = items[index].box;
		align = flex_align_of(box, child);
		frame = child->border[CSS_TOP] + child->padding[CSS_TOP] +
		    child->padding[CSS_BOTTOM] + child->border[CSS_BOTTOM];

		/* A definite percentage, or an automatic cross size stretched to the line. */
		if (child->style.height.unit == CSS_UNIT_PERCENT && percentage_basis >= 0) {
			height = flex_length(&child->style.height, percentage_basis);
			if (child->style.box_sizing == CSS_BOX_SIZING_BORDER)
				height -= frame;
		} else if (child->style.height.unit == CSS_UNIT_AUTO && align == CSS_ALIGN_STRETCH) {
			height = line_cross - child->margin[CSS_TOP] - child->margin[CSS_BOTTOM] - frame;
		} else {
			continue;
		}

		/* No cross size is negative. */
		if (height < 0)
			height = 0;

		/* Keep the flexed content width while giving layout_block a definite content height. */
		saved.box_sizing = child->style.box_sizing;
		saved.width = child->style.width;
		saved.min_width = child->style.min_width;
		saved.max_width = child->style.max_width;
		saved.height = child->style.height;
		saved.min_height = child->style.min_height;
		saved.max_height = child->style.max_height;
		memcpy(saved.margin, child->style.margin, sizeof(saved.margin));
		child->style.box_sizing = CSS_BOX_SIZING_CONTENT;
		child->style.width.unit = CSS_UNIT_PX;
		child->style.width.value = layout_to_px(child->width);
		child->style.width.offset = 0;
		child->style.min_width.unit = CSS_UNIT_PX;
		child->style.min_width.value = 0;
		child->style.max_width.unit = CSS_UNIT_NONE;
		child->style.margin[CSS_LEFT].unit = CSS_UNIT_PX;
		child->style.margin[CSS_LEFT].value = layout_to_px(items[index].margin_start);
		child->style.margin[CSS_LEFT].offset = 0;
		child->style.margin[CSS_RIGHT].unit = CSS_UNIT_PX;
		child->style.margin[CSS_RIGHT].value = layout_to_px(items[index].margin_end);
		child->style.margin[CSS_RIGHT].offset = 0;
		child->style.height.unit = CSS_UNIT_PX;
		child->style.height.value = layout_to_px(height);
		child->style.height.offset = 0;
		child->style.min_height.unit = CSS_UNIT_PX;
		child->style.min_height.value = 0;
		child->style.max_height.unit = CSS_UNIT_NONE;
		error = layout_block(tree, child, box->width);
		child->style.box_sizing = saved.box_sizing;
		child->style.width = saved.width;
		child->style.min_width = saved.min_width;
		child->style.max_width = saved.max_width;
		child->style.height = saved.height;
		child->style.min_height = saved.min_height;
		child->style.max_height = saved.max_height;
		memcpy(child->style.margin, saved.margin, sizeof(saved.margin));
		if (error != 0)
			return error;
	}

	/* Succeeded: every applicable item and its descendants use the cross size. */
	return 0;
}

/*
 * Places a line's laid out items: along the main axis by justify-content
 * (and reversed for a reverse direction), across the line by their
 * alignment, a stretching item made as thick as the line.
 */
static void
flex_place_line(
	struct layout_box *box,
	struct flex_item *items,
	size_t count,
	int row,
	layout_unit available,
	layout_unit gap,
	layout_unit cross_start,
	layout_unit line_cross)
{
	struct layout_box *child;
	layout_unit used;
	layout_unit free_space;
	layout_unit cursor;
	layout_unit between;
	layout_unit main_outer;
	layout_unit cross_outer;
	layout_unit offset;
	layout_unit frame;
	size_t index;
	int reverse;
	int align;

	/* The room the items leave along the main axis (none without a definite size). */
	used = 0;
	for (index = 0; index < count; index++)
		used += items[index].size + items[index].frame + items[index].margin_start + items[index].margin_end;
	if (count > 1)
		used += gap * (layout_unit)(count - 1U);
	free_space = 0;
	if (available >= 0 && available > used)
		free_space = available - used;

	/* justify-content: where the first item starts and what goes between them. */
	cursor = 0;
	between = gap;
	switch (box->style.justify_content) {
	case CSS_ALIGN_END:
		cursor = free_space;
		break;
	case CSS_ALIGN_CENTER:
		cursor = free_space / 2;
		break;
	case CSS_ALIGN_SPACE_BETWEEN:
		if (count > 1)
			between = gap + free_space / (layout_unit)(count - 1U);
		break;
	case CSS_ALIGN_SPACE_AROUND:
		cursor = free_space / (layout_unit)(count * 2U);
		between = gap + free_space / (layout_unit)count;
		break;
	case CSS_ALIGN_SPACE_EVENLY:
		cursor = free_space / (layout_unit)(count + 1U);
		between = gap + free_space / (layout_unit)(count + 1U);
		break;
	default:
		break;
	}

	/* A reverse direction runs from the far end; so does a row of a right-to-left container (ws074-p073), the other way. */
	reverse = 0;
	if (box->style.flex_direction == CSS_FLEX_ROW_REVERSE || box->style.flex_direction == CSS_FLEX_COLUMN_REVERSE)
		reverse = 1;
	if (row && box->style.direction == CSS_DIRECTION_RTL)
		reverse = !reverse;

	/* Each item along the main axis, then across the line. */
	for (index = 0; index < count; index++) {
		child = items[index].box;
		main_outer = items[index].size + items[index].frame + items[index].margin_start + items[index].margin_end;
		offset = cursor + items[index].margin_start;
		if (reverse && available >= 0)
			offset = available - cursor - main_outer + items[index].margin_start;

		/* Across: stretched, at the start, the end or the center of the line. */
		align = flex_align_of(box, child);
		cross_outer = flex_cross_outer(child, row);
		if (row && align == CSS_ALIGN_STRETCH && child->style.height.unit == CSS_UNIT_AUTO) {
			frame = child->border[CSS_TOP] + child->padding[CSS_TOP] + child->padding[CSS_BOTTOM] + child->border[CSS_BOTTOM];
			child->height = line_cross - child->margin[CSS_TOP] - child->margin[CSS_BOTTOM] - frame;
			if (child->height < 0)
				child->height = 0;
			cross_outer = line_cross;
		}

		/* The item's place along the cross axis within the line. */
		if (row) {
			child->x = offset;
			child->y = cross_start + child->margin[CSS_TOP];
		} else {
			child->y = offset;
			child->x = cross_start + child->margin[CSS_LEFT];
		}

		/* At the end or the center, the item moves by the room the line leaves (half of it). */
		if (align == CSS_ALIGN_END || align == CSS_ALIGN_CENTER) {
			frame = line_cross - cross_outer;
			if (align == CSS_ALIGN_CENTER)
				frame = frame / 2;
			if (row)
				child->y += frame;
			else
				child->x += frame;
		}

		/* The next item starts past this one and what goes between. */
		cursor += main_outer + between;
	}
}

/* Measures an item's margin box across the main axis (its height in a row, its width in a column). */
static layout_unit
flex_cross_outer(
	const struct layout_box *item,
	int row)
{
	/* A row's items are measured down, a column's across. */
	if (row) {
		return item->margin[CSS_TOP] + item->border[CSS_TOP] + item->padding[CSS_TOP] + item->height +
		    item->padding[CSS_BOTTOM] + item->border[CSS_BOTTOM] + item->margin[CSS_BOTTOM];
	}

	/* A column's item across. */
	return item->margin[CSS_LEFT] + item->border[CSS_LEFT] + item->padding[CSS_LEFT] + item->width +
	    item->padding[CSS_RIGHT] + item->border[CSS_RIGHT] + item->margin[CSS_RIGHT];
}

/* Picks an item's alignment across the line: its align-self, or the container's align-items (baseline as start). */
static int
flex_align_of(
	const struct layout_box *box,
	const struct layout_box *item)
{
	int align;

	/* auto defers to the container. */
	align = item->style.align_self;
	if (align == CSS_ALIGN_AUTO)
		align = box->style.align_items;

	/* Baselines are not measured in this pass. */
	if (align == CSS_ALIGN_BASELINE)
		align = CSS_ALIGN_START;

	/* Reports the alignment. */
	return align;
}

/* Resolves a length (pixels, or a percentage with its added pixels) against a basis; anything else is zero. */
static layout_unit
flex_length(
	const struct css_length *length,
	layout_unit basis)
{
	layout_unit value;

	/* Pixels, or a percentage of the basis. */
	value = 0;
	if (length->unit == CSS_UNIT_PX)
		value = layout_from_px(length->value);
	if (length->unit == CSS_UNIT_PERCENT)
		value = (layout_unit)((float)basis * length->value / 100.0f) + layout_from_px(length->offset);

	/* Reports the length. */
	return value;
}
