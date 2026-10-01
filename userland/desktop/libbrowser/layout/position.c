/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Positioned boxes: the placing of absolutely positioned and fixed boxes
 * in their containing blocks (CSS 2 §10.3.7 and §10.6.4, simplified), and
 * the painting order of every positioned box.
 *
 * A box out of the flow is placed after the normal flow, in tree order, so
 * its containing block (the padding box of its nearest positioned
 * ancestor, or the viewport's rectangle at the top of the document) is
 * already placed.  An auto width with left or right auto shrinks to fit:
 * the box is laid out once very wide to measure its content, then at the
 * narrower of that and the room it has.  A fixed box is placed like an
 * absolute one against the viewport's rectangle and scrolls with the page
 * in this pass.
 *
 * The painting order lists every positioned box as a layer.  A box under
 * an ancestor with an explicit z-index stays in that ancestor's outer
 * stacking level; this keeps the whole subtree above or below the
 * ancestor's siblings.  Within a level, tree order keeps an ancestor
 * before its positioned descendants.  The negative levels are painted
 * first, then the normal flow, then level zero and the positive levels.
 */

#include "layout/layout.h"

#include <errno.h>
#include <string.h>

/* The width a box is laid out in to measure its content's widest line (262144 pixels). */
#define POSITION_MEASURE_WIDTH	((layout_unit)1 << 24)

/*
 * A containing block: the rectangle of a positioned box's padding box, or
 * of the viewport, in absolute layout units.
 */
struct position_block {
	layout_unit x;
	layout_unit y;
	layout_unit width;
	layout_unit height;
};

/*
 * One positioned box with its z-index, while the painting order is sorted.
 */
struct position_entry {
	const struct layout_box *box;
	int z;
};

static int position_walk(struct layout_tree *tree, struct layout_box *box, const struct position_block *containing, const struct position_block *viewport, int depth);
static int position_place(struct layout_tree *tree, struct layout_box *box, const struct position_block *containing);
static void position_padding_box(const struct layout_box *box, struct position_block *block);
static int position_offset(const struct css_length *length, layout_unit size, layout_unit *value);
static layout_unit position_frame(const struct layout_box *box);
static layout_unit position_outer_height(const struct layout_box *box);
static int position_collect(const struct layout_box *box, struct wb_vector *entries, int depth, int constrained, int outer_z);
static layout_unit position_inline_floats(const struct layout_box *box, int depth);

/*
 * Places every box out of the flow of a laid out tree (whose normal flow
 * has absolute positions), and makes the document tall enough for those
 * that scroll with it.
 */
int
layout_position(
	struct layout_tree *tree)
{
	struct position_block viewport;
	int error;

	/* The initial containing block is the viewport's rectangle at the document's top left. */
	if (tree->root == NULL)
		return 0;
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = tree->viewport_width;
	viewport.height = tree->viewport_height;

	/* Walks from the root, whose own containing block is the viewport. */
	error = position_walk(tree, tree->root, &viewport, &viewport, 0);
	if (error != 0)
		return error;

	/* Succeeded: every box is placed. */
	return 0;
}

/*
 * Lists the positioned boxes of a tree in painting order, and reports at
 * which index of the list the normal flow is painted (after the boxes
 * with a negative z-index).  boxes holds const struct layout_box pointers.
 */
int
layout_stacking_order(
	const struct layout_tree *tree,
	struct wb_vector *boxes,
	size_t *flow_index)
{
	struct wb_vector entries;
	struct position_entry *entry;
	struct position_entry moved;
	size_t index;
	size_t place;
	int error;

	/* The positioned boxes in tree order. */
	*flow_index = 0;
	wb_vector_init(&entries, sizeof(struct position_entry));
	error = 0;
	if (tree->root != NULL)
		error = position_collect(tree->root, &entries, 0, 0, 0);
	if (error != 0) {
		wb_vector_release(&entries);
		return error;
	}

	/* A stable insertion sort by z-index keeps tree order among equals. */
	for (index = 1; index < entries.count; index++) {
		entry = wb_vector_at(&entries, index);
		moved = *entry;
		for (place = index; place > 0; place--) {
			entry = wb_vector_at(&entries, place - 1U);
			if (entry->z <= moved.z)
				break;
			*(struct position_entry *)wb_vector_at(&entries, place) = *entry;
		}

		/* The entry goes into the gap. */
		*(struct position_entry *)wb_vector_at(&entries, place) = moved;
	}

	/* The boxes in that order; the normal flow goes after the negative ones. */
	for (index = 0; index < entries.count; index++) {
		entry = wb_vector_at(&entries, index);
		if (entry->z < 0)
			*flow_index = index + 1U;
		error = wb_vector_push(boxes, &entry->box);
		if (error != 0) {
			wb_vector_release(&entries);
			return error;
		}
	}

	/* Succeeded: the order is listed. */
	wb_vector_release(&entries);
	return 0;
}

/*
 * Lays out a box whose width is auto so it shrinks to its content: at the
 * narrower of its content's width without a limit (layout_max_content) and
 * the room (minus its margins, borders and paddings).  A box with a width
 * is laid out in the room as it is.  The style keeps its auto width, so a
 * later layout in another room shrinks the box again.
 */
int
layout_shrink_to_fit(
	struct layout_tree *tree,
	struct layout_box *box,
	layout_unit room)
{
	struct css_length width;
	layout_unit content;
	layout_unit outside;
	int indefinite_percentage;
	int error;

	/* A percentage is indefinite while an ancestor's intrinsic width is being measured. */
	width = box->style.width;
	indefinite_percentage = tree->measuring != 0 && width.unit == CSS_UNIT_PERCENT;
	if (width.unit != CSS_UNIT_AUTO && !indefinite_percentage) {
		error = layout_block(tree, box, room);
		return error;
	}

	/* An indefinite percentage behaves as auto only for this measurement. */
	if (indefinite_percentage)
		box->style.width.unit = CSS_UNIT_AUTO;

	/* The content's width without a limit. */
	error = layout_max_content(tree, box, &content);
	if (error != 0) {
		box->style.width = width;
		return error;
	}

	/* No wider than the room leaves, and not negative. */
	layout_box_model(box, room);
	outside = position_frame(box) + box->margin[CSS_LEFT] + box->margin[CSS_RIGHT];
	if (content > room - outside)
		content = room - outside;
	if (content < 0)
		content = 0;

	/* Under box-sizing: border-box the width given is the border box's. */
	if (box->style.box_sizing == CSS_BOX_SIZING_BORDER)
		content += position_frame(box);

	/* The box is laid out at that width, given for this layout only. */
	box->style.width.unit = CSS_UNIT_PX;
	box->style.width.value = layout_to_px(content);
	box->style.width.offset = 0;
	error = layout_block(tree, box, room);
	box->style.width = width;
	if (error != 0)
		return error;

	/* Succeeded: the box has shrunk to fit. */
	return 0;
}

/*
 * Measures the width a box's content takes laid out without a limit (its
 * max-content width): the box laid out very wide, while the count of
 * measurements in progress makes the percentages inside indefinite.  The
 * width does not depend on the room the box is later laid out in, so it
 * is measured once and kept in the box.
 */
int
layout_max_content(
	struct layout_tree *tree,
	struct layout_box *box,
	layout_unit *width)
{
	struct layout_context context;
	struct wb_vector floats;
	int error;

	/* A box measured before has its width already. */
	if (box->max_content_known) {
		*width = box->max_content;
		return 0;
	}

	/*
	 * The content, laid out very wide in a formatting context of its own, so
	 * that the floats of the measurement do not stay in the context the box
	 * is in (ws074-p074: a box that starts no context of its own is measured
	 * too, for an intrinsic width).
	 */
	tree->measuring++;
	layout_context_begin(tree, &context, &floats);
	error = layout_block(tree, box, POSITION_MEASURE_WIDTH);
	layout_context_end(tree, &context);
	if (error != 0) {
		tree->measuring--;
		return error;
	}

	/* The content's own width, with the measurement over. */
	box->max_content = layout_content_width(box, 0);
	box->max_content_known = 1;
	tree->measuring--;

	/* Succeeded: the width is measured. */
	*width = box->max_content;
	return 0;
}

/*
 * Measures the width a laid out box's content needs: its longest line (or
 * its widest float among its inline content), or its widest child's margin
 * box.
 */
layout_unit
layout_content_width(
	const struct layout_box *box,
	int depth)
{
	const struct layout_line *line;
	const struct layout_fragment *last;
	const struct layout_box *child;
	layout_unit widest;
	layout_unit width;
	layout_unit floats_left;
	layout_unit floats_right;
	layout_unit edge;
	size_t index;

	/* Stops at the depth the layout stops at. */
	widest = 0;
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/*
	 * A replaced box's content is as wide as it was sized; one sized by a
	 * percentage of the width being measured is as wide as its image (a
	 * control as its natural width), since that width is indefinite.
	 */
	if (box->replaced && box->style.width.unit == CSS_UNIT_PERCENT) {
		if (box->image != NULL)
			return (layout_unit)box->image->width * LAYOUT_UNIT;
		return box->natural_width;
	}

	/* Any other replaced box. */
	if (box->replaced)
		return box->width;

	/* A grid or a table is as wide as its columns at their content's sizes (ws074-p072, ws074-p037). */
	if (box->style.display == CSS_DISPLAY_GRID)
		return box->grid_content;
	if (box->style.display == CSS_DISPLAY_TABLE || box->style.display == CSS_DISPLAY_INLINE_TABLE)
		return box->grid_content;

	/* A row of flex items is as wide as the items were before they flexed. */
	if (box->style.display == CSS_DISPLAY_FLEX &&
	    (box->style.flex_direction == CSS_FLEX_ROW || box->style.flex_direction == CSS_FLEX_ROW_REVERSE)) {
		width = layout_flex_content_width(box);
		return width;
	}

	/* Lines: the end of each line's last piece. */
	if (box->children_inline) {
		for (index = 0; index < box->line_count; index++) {
			line = &box->lines[index];
			if (line->fragment_count == 0)
				continue;
			last = &line->fragments[line->fragment_count - 1U];
			width = last->x + last->width;
			if (width > widest)
				widest = width;
		}

		/* A float among the content needs its margin box at least. */
		width = position_inline_floats(box, depth);
		if (width > widest)
			widest = width;

		/* The longest line. */
		return widest;
	}

	/*
	 * Blocks: each child's margin box, its content measured when its width
	 * is auto or a percentage (a percentage of the width being measured is
	 * indefinite, so it contributes its content, as in Chromium).
	 */
	floats_left = 0;
	floats_right = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow)
			continue;
		width = child->width;
		if (child->style.width.unit == CSS_UNIT_AUTO) {
			width = layout_content_width(child, depth + 1);
		} else if (child->style.width.unit == CSS_UNIT_PERCENT) {
			width = layout_content_width(child, depth + 1);
		}

		/*
		 * The child's margins, borders and paddings go around it.  An auto
		 * margin contributes zero to an intrinsic size; its resolved value
		 * only places the child in the deliberately very wide measuring box.
		 */
		if (child->style.margin[CSS_LEFT].unit != CSS_UNIT_AUTO)
			width += child->margin[CSS_LEFT];
		width += position_frame(child);
		if (child->style.margin[CSS_RIGHT].unit != CSS_UNIT_AUTO)
			width += child->margin[CSS_RIGHT];

		/*
		 * Floats stand side by side (ws074-p074): a right one adds its margin
		 * box, a left one reaches its right margin edge (placed relative to
		 * the content box).
		 */
		if (child->floating == CSS_FLOAT_RIGHT) {
			floats_right += width;
			continue;
		}

		/* A left float. */
		if (child->floating != CSS_FLOAT_NONE) {
			edge = child->x + position_frame(child) + child->width + child->margin[CSS_RIGHT];
			if (edge > floats_left)
				floats_left = edge;
			continue;
		}

		/* A block in the flow. */
		if (width > widest)
			widest = width;
	}

	/* The widest block, or the floats side by side. */
	if (floats_left + floats_right > widest)
		widest = floats_left + floats_right;
	return widest;
}

/* Places the boxes out of the flow under a box, in tree order, with the containing block its descendants have. */
static int
position_walk(
	struct layout_tree *tree,
	struct layout_box *box,
	const struct position_block *containing,
	const struct position_block *viewport,
	int depth)
{
	struct position_block own;
	struct layout_box *child;
	const struct position_block *inner;
	int positioned;
	int error;

	/* Stops at the depth the layout stops at. */
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/* A positioned block is the containing block of the boxes out of the flow inside it. */
	inner = containing;
	positioned = layout_is_positioned(box);
	if (positioned) {
		position_padding_box(box, &own);
		inner = &own;
	}

	/* Each child: one out of the flow is placed first (a fixed one against the viewport), then searched. */
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow && child->style.position == CSS_POSITION_FIXED) {
			error = position_place(tree, child, viewport);
		} else if (child->out_of_flow) {
			error = position_place(tree, child, inner);
		} else {
			error = 0;
		}

		/* A box that could not be laid out stops the walk. */
		if (error != 0)
			return error;

		/* Its descendants. */
		error = position_walk(tree, child, inner, viewport, depth + 1);
		if (error != 0)
			return error;
	}

	/* Succeeded: the boxes under this one are placed. */
	return 0;
}

/* Lays out and places one box out of the flow in its containing block. */
static int
position_place(
	struct layout_tree *tree,
	struct layout_box *box,
	const struct position_block *containing)
{
	const struct css_length *offset;
	layout_unit left;
	layout_unit right;
	layout_unit top;
	layout_unit bottom;
	layout_unit room;
	layout_unit frame;
	layout_unit height;
	layout_unit x;
	layout_unit y;
	layout_unit bottom_edge;
	int has_left;
	int has_right;
	int has_top;
	int has_bottom;
	int error;

	/* The offsets that are not auto, against the containing block's size. */
	offset = box->style.offset;
	has_left = position_offset(&offset[CSS_LEFT], containing->width, &left);
	has_right = position_offset(&offset[CSS_RIGHT], containing->width, &right);
	has_top = position_offset(&offset[CSS_TOP], containing->height, &top);
	has_bottom = position_offset(&offset[CSS_BOTTOM], containing->height, &bottom);

	/* The room between the offsets that are set. */
	room = containing->width;
	if (has_left)
		room -= left;
	if (has_right)
		room -= right;
	if (room < 0)
		room = 0;

	/* Auto margins are zero unless both sides are set (they then center a box of a set width). */
	if (!has_left || !has_right) {
		if (box->style.margin[CSS_LEFT].unit == CSS_UNIT_AUTO) {
			box->style.margin[CSS_LEFT].unit = CSS_UNIT_PX;
			box->style.margin[CSS_LEFT].value = 0.0f;
		}

		/* The same on the right. */
		if (box->style.margin[CSS_RIGHT].unit == CSS_UNIT_AUTO) {
			box->style.margin[CSS_RIGHT].unit = CSS_UNIT_PX;
			box->style.margin[CSS_RIGHT].value = 0.0f;
		}
	}

	/* A width set, or both sides set, lays the box out in that room; otherwise it shrinks to fit. */
	if (box->style.width.unit != CSS_UNIT_AUTO || (has_left && has_right)) {
		error = layout_block(tree, box, room);
	} else {
		error = layout_shrink_to_fit(tree, box, room);
	}

	/* A box that could not be laid out stops the placing. */
	if (error != 0)
		return error;

	/* A height left auto between a top and a bottom fills the room between them. */
	if (box->style.height.unit == CSS_UNIT_AUTO && has_top && has_bottom) {
		height = containing->height - top - bottom - box->margin[CSS_TOP] - box->margin[CSS_BOTTOM] -
		    box->border[CSS_TOP] - box->padding[CSS_TOP] - box->padding[CSS_BOTTOM] - box->border[CSS_BOTTOM];
		if (height < 0)
			height = 0;
		box->height = height;
	}

	/* Horizontally: from the left, from the right, or where it would have been. */
	frame = position_frame(box);
	if (has_left) {
		x = containing->x + left + box->margin[CSS_LEFT];
	} else if (has_right) {
		x = containing->x + containing->width - right - box->margin[CSS_RIGHT] - (box->width + frame);
	} else {
		x = box->static_x + box->margin[CSS_LEFT];
	}

	/* Vertically: from the top, from the bottom, or where it would have been. */
	if (has_top) {
		y = containing->y + top + box->margin[CSS_TOP];
	} else if (has_bottom) {
		y = containing->y + containing->height - bottom - box->margin[CSS_BOTTOM] - position_outer_height(box);
	} else {
		y = box->static_y + box->margin[CSS_TOP];
	}

	/* The box and its descendants take absolute positions from there. */
	box->x = 0;
	box->y = 0;
	layout_absolute(box, x, y);

	/* A box that scrolls with the document makes it tall enough to hold it. */
	bottom_edge = box->y + position_outer_height(box) + box->margin[CSS_BOTTOM];
	if (box->style.position != CSS_POSITION_FIXED && bottom_edge > tree->document_height)
		tree->document_height = bottom_edge;

	/* Succeeded: the box is placed. */
	return 0;
}

/* Reports a box's padding box as a containing block. */
static void
position_padding_box(
	const struct layout_box *box,
	struct position_block *block)
{
	/* Inside the borders, around the paddings. */
	block->x = box->x + box->border[CSS_LEFT];
	block->y = box->y + box->border[CSS_TOP];
	block->width = box->padding[CSS_LEFT] + box->width + box->padding[CSS_RIGHT];
	block->height = box->padding[CSS_TOP] + box->height + box->padding[CSS_BOTTOM];
}

/* Resolves an offset (top, right, bottom or left) against a size; zero when it is auto. */
static int
position_offset(
	const struct css_length *length,
	layout_unit size,
	layout_unit *value)
{
	/* Pixels. */
	*value = 0;
	if (length->unit == CSS_UNIT_PX) {
		*value = layout_from_px(length->value);
		return 1;
	}

	/* A percentage of the containing block. */
	if (length->unit == CSS_UNIT_PERCENT) {
		*value = (layout_unit)((float)size * length->value / 100.0f) + layout_from_px(length->offset);
		return 1;
	}

	/* auto. */
	return 0;
}


/* Reports the horizontal borders and paddings of a box. */
static layout_unit
position_frame(
	const struct layout_box *box)
{
	/* The four widths. */
	return box->border[CSS_LEFT] + box->padding[CSS_LEFT] + box->padding[CSS_RIGHT] + box->border[CSS_RIGHT];
}

/* Reports a box's border-box height. */
static layout_unit
position_outer_height(
	const struct layout_box *box)
{
	/* The content, paddings and borders. */
	return box->border[CSS_TOP] + box->padding[CSS_TOP] + box->height + box->padding[CSS_BOTTOM] + box->border[CSS_BOTTOM];
}

/*
 * Adds the positioned boxes under a box (not the root) to the entries, in
 * tree order.  An explicit z-index starts a stacking level: positioned
 * descendants keep its outer z-index instead of escaping above or below
 * the level's siblings.
 */
static int
position_collect(
	const struct layout_box *box,
	struct wb_vector *entries,
	int depth,
	int constrained,
	int outer_z)
{
	struct position_entry entry;
	const struct layout_box *child;
	int child_constrained;
	int child_outer_z;
	int child_z;
	int positioned;
	int error;

	/* Stops at the depth the layout stops at. */
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/* Each child: a positioned one is listed at its outer stacking level, then searched. */
	for (child = box->first_child; child != NULL; child = child->next) {
		positioned = layout_is_positioned(child);
		child_z = child->style.z_index;
		if (child->style.z_index_auto)
			child_z = 0;
		child_constrained = constrained;
		child_outer_z = outer_z;
		if (positioned) {
			entry.box = child;
			entry.z = child_z;
			if (constrained)
				entry.z = outer_z;
			error = wb_vector_push(entries, &entry);
			if (error != 0)
				return error;

			/* The first explicit z-index contains all the levels below it. */
			if (!constrained && !child->style.z_index_auto) {
				child_constrained = 1;
				child_outer_z = child_z;
			}
		}

		/* Its descendants. */
		error = position_collect(child, entries, depth + 1, child_constrained, child_outer_z);
		if (error != 0)
			return error;
	}

	/* Succeeded: the boxes are listed. */
	return 0;
}

/*
 * Reports the width the floats among a block's inline content take side by
 * side: the right margin edge of the furthest left float (they were placed
 * relative to the content box), and the margin boxes of the right floats
 * added to it.
 */
static layout_unit
position_inline_floats(
	const struct layout_box *box,
	int depth)
{
	const struct layout_box *child;
	layout_unit left;
	layout_unit right;
	layout_unit width;
	layout_unit edge;

	/* Stops at the depth the layout stops at. */
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/* A float is measured; an inline box is searched; a box out of the flow or an inline block (its line holds it) is not. */
	left = 0;
	right = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow || child->atomic)
			continue;
		if (child->floating == CSS_FLOAT_NONE) {
			width = position_inline_floats(child, depth + 1);
			if (width > left)
				left = width;
			continue;
		}

		/* A right float adds its margin box; a left one reaches its right margin edge. */
		width = child->margin[CSS_LEFT] + position_frame(child) + child->width + child->margin[CSS_RIGHT];
		if (child->floating == CSS_FLOAT_RIGHT) {
			right += width;
			continue;
		}

		/* A left float, at least as wide as its margin box. */
		edge = child->x + position_frame(child) + child->width + child->margin[CSS_RIGHT];
		if (edge < width)
			edge = width;
		if (edge > left)
			left = edge;
	}

	/* Reports the two sides together. */
	return left + right;
}
