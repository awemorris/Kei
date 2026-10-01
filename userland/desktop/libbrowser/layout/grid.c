/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Grid layout (ws074-p072), the common part of CSS Grid Layout 1: the
 * explicit column and row templates (lengths, percentages, fr, auto,
 * minmax()'s minimum), items placed by line numbers (negative from the
 * end) and spans or else row after row in the first cells that are free,
 * implicit auto tracks for what goes past the templates, the gaps, and
 * each item laid out as a block in its area and aligned in it
 * (align-self, align-items).  Columns are sized in three steps: the fixed
 * ones, then the auto ones at their items' content width, then the fr
 * ones sharing what is left (auto columns share it when there is no fr);
 * rows are as tall as their items unless the template fixes them.  Not in
 * this pass: named lines and areas, dense packing, justify-items and
 * justify-self, order, baselines, subgrid, and auto-fill and auto-fit.
 */

#include "layout/layout.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The most implicit rows a grid places items in (items past them go on the last). */
#define GRID_ROWS_MAX	4096

/*
 * One item on its way through the algorithm: its box, and its area's
 * first column and row and how many of each it spans.
 */
struct grid_item {
	struct layout_box *box;
	int column;
	int column_span;
	int row;
	int row_span;
};

/*
 * The grid being laid out: its items, its columns' and rows' sizes and
 * offsets, and which cells are taken (rows of column_count cells).
 */
struct grid_state {
	struct grid_item *items;
	size_t item_count;
	int column_count;
	int row_count;
	layout_unit *columns;
	layout_unit *rows;
	unsigned char *taken;
	int taken_rows;
};

/*
 * The part of an item's style that its layout in its area overrides, kept
 * to be put back afterwards.
 */
struct grid_sizes {
	int box_sizing;
	struct css_length width;
	struct css_length min_width;
	struct css_length max_width;
	struct css_length height;
	struct css_length min_height;
	struct css_length max_height;
	struct css_length margin[4];
};

static int grid_collect(struct layout_box *box, struct grid_state *grid);
static void grid_resolve(const struct css_grid_place *place, int explicit_count, int *start, int *span);
static int grid_place(struct grid_state *grid);
static int grid_fits(const struct grid_state *grid, int row, int column, int row_span, int column_span);
static int grid_take(struct grid_state *grid, int row, int column, int row_span, int column_span);
static int grid_size_columns(struct layout_tree *tree, struct layout_box *box, struct grid_state *grid, layout_unit gap);
static int grid_item_width(struct layout_tree *tree, struct layout_box *item, layout_unit *width);
static int grid_lay_item(struct layout_tree *tree, struct layout_box *item, layout_unit width);
static void grid_size_rows(struct layout_box *box, struct grid_state *grid, layout_unit gap);
static int grid_relay_item(struct layout_tree *tree, struct layout_box *item, layout_unit width, layout_unit height, int align);
static layout_unit grid_outer_height(const struct layout_box *item);
static layout_unit grid_track_length(const struct css_length *length, layout_unit whole, int measuring);
static void grid_release(struct grid_state *grid);

/*
 * Lays out the content of a grid container: its in-flow children as grid
 * items in their areas; sets their positions (their border boxes relative
 * to the container's content box), the container's content height, and
 * the width its columns need at their content's sizes (for the
 * containers around it that shrink to fit).
 */
int
layout_grid(
	struct layout_tree *tree,
	struct layout_box *box)
{
	struct grid_state grid;
	struct grid_item *item;
	struct layout_box *child;
	layout_unit column_gap;
	layout_unit row_gap;
	layout_unit x;
	layout_unit y;
	layout_unit area_width;
	layout_unit area_height;
	layout_unit outer;
	layout_unit frame;
	layout_unit room;
	layout_unit used;
	layout_unit shift;
	size_t index;
	int position;
	int align;
	int error;

	/* The children out of the flow have their static place at the content box's corner. */
	for (child = box->first_child; child != NULL; child = child->next) {
		if (!child->out_of_flow)
			continue;
		child->static_x = 0;
		child->static_y = 0;
	}

	/* The items, placed in their cells. */
	memset(&grid, 0, sizeof(grid));
	error = grid_collect(box, &grid);
	if (error == 0)
		error = grid_place(&grid);
	if (error != 0) {
		grid_release(&grid);
		return error;
	}

	/* The gaps between the tracks. */
	column_gap = 0;
	if (box->style.column_gap.unit == CSS_UNIT_PX)
		column_gap = layout_from_px(box->style.column_gap.value);
	if (box->style.column_gap.unit == CSS_UNIT_PERCENT)
		column_gap = (layout_unit)((float)box->width * box->style.column_gap.value / 100.0f);
	row_gap = 0;
	if (box->style.row_gap.unit == CSS_UNIT_PX)
		row_gap = layout_from_px(box->style.row_gap.value);

	/* The columns' widths. */
	error = grid_size_columns(tree, box, &grid, column_gap);
	if (error != 0) {
		grid_release(&grid);
		return error;
	}

	/* The columns side by side: where they start (justify-content moves a grid narrower than its box). */
	used = column_gap * (layout_unit)(grid.column_count - 1);
	for (position = 0; position < grid.column_count; position++)
		used += grid.columns[position];
	box->grid_content = used;
	shift = 0;
	room = box->width - used;
	if (room > 0 && box->style.justify_content == CSS_ALIGN_CENTER)
		shift = room / 2;
	if (room > 0 && box->style.justify_content == CSS_ALIGN_END)
		shift = room;

	/* Each item laid out in its area's width. */
	for (index = 0; index < grid.item_count; index++) {
		item = &grid.items[index];
		area_width = column_gap * (layout_unit)(item->column_span - 1);
		for (position = item->column; position < item->column + item->column_span; position++)
			area_width += grid.columns[position];
		error = grid_lay_item(tree, item->box, area_width);
		if (error != 0) {
			grid_release(&grid);
			return error;
		}
	}

	/* The rows' heights from the items. */
	grid_size_rows(box, &grid, row_gap);

	/* Each item in its area, aligned in it across the rows. */
	for (index = 0; index < grid.item_count; index++) {
		item = &grid.items[index];
		child = item->box;

		/* The area's corner and height. */
		x = shift;
		for (position = 0; position < item->column; position++)
			x += grid.columns[position] + column_gap;
		y = 0;
		for (position = 0; position < item->row; position++)
			y += grid.rows[position] + row_gap;
		area_height = row_gap * (layout_unit)(item->row_span - 1);
		for (position = item->row; position < item->row + item->row_span; position++)
			area_height += grid.rows[position];
		area_width = column_gap * (layout_unit)(item->column_span - 1);
		for (position = item->column; position < item->column + item->column_span; position++)
			area_width += grid.columns[position];

		/* The item's alignment: its align-self, or the container's align-items (baseline as start). */
		align = child->style.align_self;
		if (align == CSS_ALIGN_AUTO)
			align = box->style.align_items;
		if (align == CSS_ALIGN_BASELINE)
			align = CSS_ALIGN_START;

		/* A definite grid area must reach percentage descendants of a stretched item. */
		error = grid_relay_item(tree, child, area_width, area_height, align);
		if (error != 0) {
			grid_release(&grid);
			return error;
		}

		/* A stretching item without a height of its own fills the area's height. */
		frame = child->border[CSS_TOP] + child->padding[CSS_TOP] + child->padding[CSS_BOTTOM] + child->border[CSS_BOTTOM];
		if (align == CSS_ALIGN_STRETCH && child->style.height.unit == CSS_UNIT_AUTO) {
			child->height = area_height - child->margin[CSS_TOP] - child->margin[CSS_BOTTOM] - frame;
			if (child->height < 0)
				child->height = 0;
		}

		/* Its place, moved down by the room it leaves when it is at the end or the center. */
		child->x = x + child->margin[CSS_LEFT];
		child->y = y + child->margin[CSS_TOP];
		outer = grid_outer_height(child);
		if (align == CSS_ALIGN_END && area_height > outer)
			child->y += area_height - outer;
		if (align == CSS_ALIGN_CENTER && area_height > outer)
			child->y += (area_height - outer) / 2;
	}

	/* The container's content height: its rows and the gaps between them. */
	box->height = row_gap * (layout_unit)(grid.row_count - 1);
	if (grid.row_count == 0)
		box->height = 0;
	for (position = 0; position < grid.row_count; position++)
		box->height += grid.rows[position];

	/* The state is no longer needed. */
	grid_release(&grid);

	/* Succeeded: the container's content is laid out. */
	return 0;
}

/* Gathers a container's in-flow children as items, with the places their styles give on each axis. */
static int
grid_collect(
	struct layout_box *box,
	struct grid_state *grid)
{
	struct layout_box *child;
	struct grid_item *item;
	size_t count;

	/* Counts the in-flow children. */
	count = 0;
	for (child = box->first_child; child != NULL; child = child->next) {
		if (!child->out_of_flow)
			count++;
	}

	/* The array (one place at least, so that none is not NULL). */
	grid->items = calloc(count + 1U, sizeof(*grid->items));
	if (grid->items == NULL)
		return ENOMEM;

	/* The columns and rows the templates make. */
	grid->column_count = box->style.column_count;
	grid->row_count = box->style.row_count;

	/* Each child, its places resolved against the templates. */
	for (child = box->first_child; child != NULL; child = child->next) {
		if (child->out_of_flow)
			continue;
		item = &grid->items[grid->item_count];
		item->box = child;
		grid_resolve(&child->style.grid_column, box->style.column_count, &item->column, &item->column_span);
		grid_resolve(&child->style.grid_row, box->style.row_count, &item->row, &item->row_span);
		grid->item_count++;
	}

	/* Succeeded: the items are gathered. */
	return 0;
}

/*
 * Resolves an item's place on one axis to its first track (-1 for auto)
 * and its span: lines count from 1, negative lines from the end of the
 * explicit tracks; a start and an end line give the span between them.
 */
static void
grid_resolve(
	const struct css_grid_place *place,
	int explicit_count,
	int *start,
	int *span)
{
	int first;
	int last;

	/* The start line's track, or none. */
	first = -1;
	if (place->start > 0)
		first = place->start - 1;
	if (place->start < 0)
		first = explicit_count + 1 + place->start;
	if (place->start < 0 && first < 0)
		first = 0;

	/* The end line's track (the one after the area), or none. */
	last = -1;
	if (place->end > 0)
		last = place->end - 1;
	if (place->end < 0)
		last = explicit_count + 1 + place->end;
	if (place->end < 0 && last < 0)
		last = 0;

	/* The span: between the lines, or the one given, or one. */
	*span = place->start_span;
	if (place->end_span > *span)
		*span = place->end_span;
	if (*span < 1)
		*span = 1;
	if (first >= 0 && last >= 0) {
		if (last < first) {
			*span = first - last;
			first = last;
		} else if (last > first) {
			*span = last - first;
		}
	} else if (first < 0 && last >= 0) {
		/* Only the end is known: the area ends there. */
		first = last - *span;
		if (first < 0)
			first = 0;
	}

	/* The first track, or -1 for auto. */
	*start = first;
}

/*
 * Places every item in the cells: the ones with both lines given first,
 * then the ones with a row, then the rest row after row in the first free
 * cells after the last one placed.  The grid grows to hold them.
 */
static int
grid_place(
	struct grid_state *grid)
{
	struct grid_item *item;
	size_t index;
	int cursor_row;
	int cursor_column;
	int error;
	int pass;
	int fits;

	/* The columns: the template's, and as many more as the items given a column need. */
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->column >= 0 && item->column + item->column_span > grid->column_count)
			grid->column_count = item->column + item->column_span;
		if (item->column_span > grid->column_count)
			grid->column_count = item->column_span;
	}

	/* A grid has one column at least. */
	if (grid->column_count < 1)
		grid->column_count = 1;

	/* An auto item's span is at most the columns there are. */
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->column_span > grid->column_count)
			item->column_span = grid->column_count;
	}

	/* The items with a column and a row, then those with a row only (in the first free columns of their rows). */
	for (pass = 0; pass < 2; pass++) {
		for (index = 0; index < grid->item_count; index++) {
			item = &grid->items[index];
			if (item->row < 0)
				continue;
			if (pass == 0 && item->column < 0)
				continue;
			if (pass == 1 && item->column >= 0)
				continue;

			/* An item with a row only takes the first columns free in it. */
			if (item->column < 0) {
				item->column = 0;
				fits = grid_fits(grid, item->row, item->column, item->row_span, item->column_span);
				while (!fits && item->column + item->column_span < grid->column_count) {
					item->column++;
					fits = grid_fits(grid, item->row, item->column, item->row_span, item->column_span);
				}
			}

			/* The item takes its cells. */
			error = grid_take(grid, item->row, item->column, item->row_span, item->column_span);
			if (error != 0)
				return error;
		}
	}

	/* The rest, row after row from the last one placed. */
	cursor_row = 0;
	cursor_column = 0;
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->row >= 0)
			continue;

		/* A column given: the first row from the cursor where the area is free. */
		if (item->column >= 0) {
			if (item->column < cursor_column)
				cursor_row++;
			fits = grid_fits(grid, cursor_row, item->column, item->row_span, item->column_span);
			while (!fits && cursor_row < GRID_ROWS_MAX) {
				cursor_row++;
				fits = grid_fits(grid, cursor_row, item->column, item->row_span, item->column_span);
			}

			/* The cursor is at the item's column. */
			cursor_column = item->column;
		} else {
			/* No column: the first free cells from the cursor, row after row. */
			fits = 0;
			while (!fits && cursor_row < GRID_ROWS_MAX) {
				if (cursor_column + item->column_span > grid->column_count) {
					cursor_row++;
					cursor_column = 0;
					continue;
				}

				/* The area at the cursor, or the next column. */
				fits = grid_fits(grid, cursor_row, cursor_column, item->row_span, item->column_span);
				if (!fits)
					cursor_column++;
			}
		}

		/* The item takes its cells, and the cursor moves past them. */
		if (item->column < 0)
			item->column = cursor_column;
		item->row = cursor_row;
		error = grid_take(grid, item->row, item->column, item->row_span, item->column_span);
		if (error != 0)
			return error;
		cursor_column = item->column + item->column_span;
	}

	/* The rows: the template's, and as many more as the items took. */
	if (grid->taken_rows > grid->row_count)
		grid->row_count = grid->taken_rows;

	/* Succeeded: every item has its area. */
	return 0;
}

/* Tells whether an area's cells are all free (rows past the grid's are free). */
static int
grid_fits(
	const struct grid_state *grid,
	int row,
	int column,
	int row_span,
	int column_span)
{
	int y;
	int x;

	/* Each cell of the area inside the rows there are. */
	for (y = row; y < row + row_span && y < grid->taken_rows; y++) {
		for (x = column; x < column + column_span; x++) {
			if (grid->taken[(size_t)y * (size_t)grid->column_count + (size_t)x])
				return 0;
		}
	}

	/* The area is free. */
	return 1;
}

/* Marks an area's cells taken, growing the rows to hold it (an area past the most rows is held in the last). */
static int
grid_take(
	struct grid_state *grid,
	int row,
	int column,
	int row_span,
	int column_span)
{
	unsigned char *taken;
	int rows;
	int y;
	int x;

	/* The area stays inside the most rows. */
	if (row >= GRID_ROWS_MAX)
		row = GRID_ROWS_MAX - 1;
	if (row + row_span > GRID_ROWS_MAX)
		row_span = GRID_ROWS_MAX - row;
	if (column + column_span > grid->column_count)
		column_span = grid->column_count - column;

	/* More rows when the area reaches past them. */
	rows = row + row_span;
	if (rows > grid->taken_rows) {
		taken = realloc(grid->taken, (size_t)rows * (size_t)grid->column_count);
		if (taken == NULL)
			return ENOMEM;
		memset(taken + (size_t)grid->taken_rows * (size_t)grid->column_count, 0,
		    (size_t)(rows - grid->taken_rows) * (size_t)grid->column_count);
		grid->taken = taken;
		grid->taken_rows = rows;
	}

	/* The cells. */
	for (y = row; y < row + row_span; y++) {
		for (x = column; x < column + column_span; x++)
			grid->taken[(size_t)y * (size_t)grid->column_count + (size_t)x] = 1;
	}

	/* Succeeded: the area is taken. */
	return 0;
}

/*
 * Sizes the columns: fixed tracks at their lengths, auto tracks at their
 * one-column items' content widths (widened for the items spanning them),
 * then the fr tracks sharing what is left of the container's width (at
 * least their minimum), or the auto tracks sharing it when there is no fr.
 * While a container is measured its fr tracks are sized as auto ones.
 */
static int
grid_size_columns(
	struct layout_tree *tree,
	struct layout_box *box,
	struct grid_state *grid,
	layout_unit gap)
{
	const struct css_track *track;
	struct grid_item *item;
	layout_unit available;
	layout_unit width;
	layout_unit used;
	layout_unit share;
	layout_unit spanned;
	float fr_total;
	size_t index;
	int column;
	int autos;
	int error;

	/* The row of widths. */
	grid->columns = calloc((size_t)grid->column_count, sizeof(*grid->columns));
	if (grid->columns == NULL)
		return ENOMEM;

	/* The fixed tracks, and the least of the fr tracks. */
	fr_total = 0;
	for (column = 0; column < grid->column_count; column++) {
		if (column >= box->style.column_count)
			continue;
		track = &box->style.columns[column];
		if (track->kind == CSS_TRACK_LENGTH)
			grid->columns[column] = grid_track_length(&track->size, box->width, tree->measuring);
		if (track->kind == CSS_TRACK_FR && tree->measuring == 0) {
			fr_total += track->fr;
			grid->columns[column] = grid_track_length(&track->minimum, box->width, tree->measuring);
		}
	}

	/* The auto tracks (and the fr ones while measuring): their one-column items' widths. */
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->column_span != 1)
			continue;
		column = item->column;
		if (column < box->style.column_count) {
			track = &box->style.columns[column];
			if (track->kind == CSS_TRACK_LENGTH)
				continue;
			if (track->kind == CSS_TRACK_FR && tree->measuring == 0)
				continue;
		}

		/* The item's margin box at its content's width. */
		error = grid_item_width(tree, item->box, &width);
		if (error != 0)
			return error;
		if (width > grid->columns[column])
			grid->columns[column] = width;
	}

	/* An item spanning auto tracks widens them, equally, to hold it. */
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->column_span < 2)
			continue;

		/* The spanned width and how many auto tracks share any more. */
		spanned = gap * (layout_unit)(item->column_span - 1);
		autos = 0;
		for (column = item->column; column < item->column + item->column_span; column++) {
			spanned += grid->columns[column];
			if (column >= box->style.column_count || box->style.columns[column].kind == CSS_TRACK_AUTO)
				autos++;
		}

		/* Spanning only fixed or fr tracks widens nothing. */
		if (autos == 0)
			continue;

		/* The item's width against it. */
		error = grid_item_width(tree, item->box, &width);
		if (error != 0)
			return error;
		if (width <= spanned)
			continue;
		share = (width - spanned) / autos;
		for (column = item->column; column < item->column + item->column_span; column++) {
			if (column >= box->style.column_count || box->style.columns[column].kind == CSS_TRACK_AUTO)
				grid->columns[column] += share;
		}
	}

	/* What is left of the container's width after the tracks and the gaps. */
	available = box->width - gap * (layout_unit)(grid->column_count - 1);
	used = 0;
	for (column = 0; column < grid->column_count; column++) {
		track = NULL;
		if (column < box->style.column_count)
			track = &box->style.columns[column];
		if (track != NULL && track->kind == CSS_TRACK_FR && tree->measuring == 0)
			continue;
		used += grid->columns[column];
	}

	/* The fr tracks share it by their factors (a sum under one shares only that much of it), each at least its minimum. */
	if (fr_total > 0) {
		if (fr_total < 1.0f)
			fr_total = 1.0f;
		for (column = 0; column < box->style.column_count && column < grid->column_count; column++) {
			track = &box->style.columns[column];
			if (track->kind != CSS_TRACK_FR)
				continue;
			width = (layout_unit)((float)(available - used) * track->fr / fr_total);
			if (width > grid->columns[column])
				grid->columns[column] = width;
		}

		/* The fr tracks took the room. */
		return 0;
	}

	/* Without fr tracks, the auto tracks share the room left (not while the container is measured). */
	if (tree->measuring != 0 || available <= used)
		return 0;
	autos = 0;
	for (column = 0; column < grid->column_count; column++) {
		if (column >= box->style.column_count || box->style.columns[column].kind == CSS_TRACK_AUTO)
			autos++;
	}

	/* Without auto tracks the room stays empty. */
	if (autos == 0)
		return 0;
	share = (available - used) / autos;
	for (column = 0; column < grid->column_count; column++) {
		if (column >= box->style.column_count || box->style.columns[column].kind == CSS_TRACK_AUTO)
			grid->columns[column] += share;
	}

	/* Succeeded: the columns are sized. */
	return 0;
}

/* Measures an item's margin box at its content's width (or at its own width, when it has one in pixels). */
static int
grid_item_width(
	struct layout_tree *tree,
	struct layout_box *item,
	layout_unit *width)
{
	layout_unit content;
	layout_unit outside;
	int error;

	/* Its margins, borders and paddings. */
	layout_box_model(item, 0);
	outside = item->margin[CSS_LEFT] + item->border[CSS_LEFT] + item->padding[CSS_LEFT] +
	    item->padding[CSS_RIGHT] + item->border[CSS_RIGHT] + item->margin[CSS_RIGHT];

	/* A width of its own in pixels. */
	if (item->style.width.unit == CSS_UNIT_PX) {
		content = layout_from_px(item->style.width.value);
		if (item->style.box_sizing == CSS_BOX_SIZING_BORDER)
			content -= item->border[CSS_LEFT] + item->padding[CSS_LEFT] + item->padding[CSS_RIGHT] + item->border[CSS_RIGHT];
		*width = content + outside;
		return 0;
	}

	/* Otherwise its content's width without a limit. */
	error = layout_max_content(tree, item, &content);
	if (error != 0)
		return error;
	layout_box_model(item, 0);
	*width = content + outside;
	return 0;
}

/*
 * Lays an item out as a block in its area's width: one without a width of
 * its own fills the area (its style's sizes and margins are put back
 * afterwards), another is laid out in it as it is.
 */
static int
grid_lay_item(
	struct layout_tree *tree,
	struct layout_box *item,
	layout_unit width)
{
	struct grid_sizes saved;
	layout_unit content;
	int side;
	int error;

	/* An item with a width of its own is laid out in the area as a block is. */
	if (item->style.width.unit != CSS_UNIT_AUTO) {
		error = layout_block(tree, item, width);
		return error;
	}

	/* Keeps the style's sizes and margins, which the layout at the area's width overrides. */
	saved.box_sizing = item->style.box_sizing;
	saved.width = item->style.width;
	saved.min_width = item->style.min_width;
	saved.max_width = item->style.max_width;
	memcpy(saved.margin, item->style.margin, sizeof(saved.margin));

	/* The content width that fills the area: the area less the margins (auto ones are none), borders and paddings. */
	layout_box_model(item, width);
	for (side = CSS_RIGHT; side <= CSS_LEFT; side += 2) {
		if (item->style.margin[side].unit == CSS_UNIT_AUTO) {
			item->style.margin[side].unit = CSS_UNIT_PX;
			item->style.margin[side].value = 0;
			item->style.margin[side].offset = 0;
			item->margin[side] = 0;
		}
	}

	/* What is left of the area for the content. */
	content = width - item->margin[CSS_LEFT] - item->margin[CSS_RIGHT] - item->border[CSS_LEFT] - item->padding[CSS_LEFT] -
	    item->padding[CSS_RIGHT] - item->border[CSS_RIGHT];
	if (content < 0)
		content = 0;

	/* The layout at that width. */
	item->style.box_sizing = CSS_BOX_SIZING_CONTENT;
	item->style.width.unit = CSS_UNIT_PX;
	item->style.width.value = layout_to_px(content);
	item->style.width.offset = 0;
	item->style.min_width.unit = CSS_UNIT_PX;
	item->style.min_width.value = 0;
	item->style.max_width.unit = CSS_UNIT_NONE;
	error = layout_block(tree, item, width);

	/* The style is the item's own again. */
	item->style.box_sizing = saved.box_sizing;
	item->style.width = saved.width;
	item->style.min_width = saved.min_width;
	item->style.max_width = saved.max_width;
	memcpy(item->style.margin, saved.margin, sizeof(saved.margin));

	/* Reports an item that could not be laid out. */
	if (error != 0)
		return error;

	/* Succeeded: the item fills its area's width. */
	return 0;
}

/*
 * Sizes the rows: a fixed track at its length, the others as tall as
 * their one-row items' margin boxes; an item spanning rows makes the last
 * of them taller when they do not hold it.
 */
static void
grid_size_rows(
	struct layout_box *box,
	struct grid_state *grid,
	layout_unit gap)
{
	const struct css_track *track;
	struct grid_item *item;
	layout_unit available;
	layout_unit height;
	layout_unit minimum;
	layout_unit spanned;
	layout_unit used;
	float fr_total;
	size_t index;
	int row;

	/* The column of heights (none when the grid has no rows). */
	grid->rows = calloc((size_t)grid->row_count + 1U, sizeof(*grid->rows));
	if (grid->rows == NULL) {
		grid->row_count = 0;
		return;
	}

	/* A pixel height is the definite content height available to the tracks. */
	available = -1;
	if (box->style.height.unit == CSS_UNIT_PX) {
		available = layout_from_px(box->style.height.value);
		if (box->style.box_sizing == CSS_BOX_SIZING_BORDER) {
			available -= box->border[CSS_TOP] + box->padding[CSS_TOP] +
			    box->padding[CSS_BOTTOM] + box->border[CSS_BOTTOM];
		}

		/* No available content height is negative. */
		if (available < 0)
			available = 0;
	}

	/* The fixed rows, and each fractional row's minimum. */
	fr_total = 0;
	for (row = 0; row < grid->row_count && row < box->style.row_count; row++) {
		track = &box->style.rows[row];
		if (track->kind == CSS_TRACK_LENGTH)
			grid->rows[row] = grid_track_length(&track->size, available, available < 0);
		if (track->kind == CSS_TRACK_FR && available >= 0) {
			fr_total += track->fr;
			grid->rows[row] = grid_track_length(&track->minimum, available, 0);
		}
	}

	/* The auto rows, and fractional rows without a definite height: their one-row items. */
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->row_span != 1 || item->row >= grid->row_count)
			continue;
		if (item->row < box->style.row_count) {
			track = &box->style.rows[item->row];
			if (track->kind == CSS_TRACK_LENGTH || (track->kind == CSS_TRACK_FR && available >= 0))
				continue;
		}

		/* The item's outer height raises its auto row. */
		height = grid_outer_height(item->box);
		if (height > grid->rows[item->row])
			grid->rows[item->row] = height;
	}

	/* The items spanning rows. */
	for (index = 0; index < grid->item_count; index++) {
		item = &grid->items[index];
		if (item->row_span < 2)
			continue;
		spanned = gap * (layout_unit)(item->row_span - 1);
		for (row = item->row; row < item->row + item->row_span && row < grid->row_count; row++)
			spanned += grid->rows[row];
		height = grid_outer_height(item->box);
		row = item->row + item->row_span - 1;
		if (height > spanned && row < grid->row_count)
			grid->rows[row] += height - spanned;
	}

	/* Fractional rows share the definite room left after fixed and auto rows and the gaps. */
	if (fr_total > 0) {
		used = gap * (layout_unit)(grid->row_count - 1);
		for (row = 0; row < grid->row_count; row++) {
			if (row < box->style.row_count && box->style.rows[row].kind == CSS_TRACK_FR)
					continue;
				used += grid->rows[row];
			}

			/* A sum below one leaves the corresponding share unused. */
			if (fr_total < 1.0f)
			fr_total = 1.0f;
		for (row = 0; row < grid->row_count && row < box->style.row_count; row++) {
			track = &box->style.rows[row];
			if (track->kind != CSS_TRACK_FR)
				continue;
			minimum = (layout_unit)((float)(available - used) * track->fr / fr_total);
			if (minimum > grid->rows[row])
				grid->rows[row] = minimum;
		}
	}
}

/*
 * Lays a grid item out again at the area's definite cross size when it is
 * stretched, or when its percentage height can use the area.  Placement
 * used to change only item->height, leaving its percentage descendants at
 * the intrinsic height from the first pass.
 */
static int
grid_relay_item(
	struct layout_tree *tree,
	struct layout_box *item,
	layout_unit width,
	layout_unit height,
	int align)
{
	struct grid_sizes saved;
	layout_unit frame;
	layout_unit content;
	int error;

	/* The target content height, from the percentage or the area's stretch. */
	frame = item->border[CSS_TOP] + item->padding[CSS_TOP] +
	    item->padding[CSS_BOTTOM] + item->border[CSS_BOTTOM];
	if (item->style.height.unit == CSS_UNIT_PERCENT) {
		content = (layout_unit)((float)height * item->style.height.value / 100.0f) +
		    layout_from_px(item->style.height.offset);
		if (item->style.box_sizing == CSS_BOX_SIZING_BORDER)
			content -= frame;
	} else if (item->style.height.unit == CSS_UNIT_AUTO && align == CSS_ALIGN_STRETCH) {
		content = height - item->margin[CSS_TOP] - item->margin[CSS_BOTTOM] - frame;
	} else {
		return 0;
	}

	/* No content size is negative. */
	if (content < 0)
		content = 0;

	/* Keep the item's style while the definite content box is laid out. */
	saved.box_sizing = item->style.box_sizing;
	saved.width = item->style.width;
	saved.min_width = item->style.min_width;
	saved.max_width = item->style.max_width;
	saved.height = item->style.height;
	saved.min_height = item->style.min_height;
	saved.max_height = item->style.max_height;
	memcpy(saved.margin, item->style.margin, sizeof(saved.margin));
	item->style.box_sizing = CSS_BOX_SIZING_CONTENT;
	item->style.width.unit = CSS_UNIT_PX;
	item->style.width.value = layout_to_px(item->width);
	item->style.width.offset = 0;
	item->style.min_width.unit = CSS_UNIT_PX;
	item->style.min_width.value = 0;
	item->style.max_width.unit = CSS_UNIT_NONE;
	item->style.height.unit = CSS_UNIT_PX;
	item->style.height.value = layout_to_px(content);
	item->style.height.offset = 0;
	item->style.min_height.unit = CSS_UNIT_PX;
	item->style.min_height.value = 0;
	item->style.max_height.unit = CSS_UNIT_NONE;
	item->style.margin[CSS_LEFT].unit = CSS_UNIT_PX;
	item->style.margin[CSS_LEFT].value = layout_to_px(item->margin[CSS_LEFT]);
	item->style.margin[CSS_LEFT].offset = 0;
	item->style.margin[CSS_RIGHT].unit = CSS_UNIT_PX;
	item->style.margin[CSS_RIGHT].value = layout_to_px(item->margin[CSS_RIGHT]);
	item->style.margin[CSS_RIGHT].offset = 0;
	error = layout_block(tree, item, width);
	item->style.box_sizing = saved.box_sizing;
	item->style.width = saved.width;
	item->style.min_width = saved.min_width;
	item->style.max_width = saved.max_width;
	item->style.height = saved.height;
	item->style.min_height = saved.min_height;
	item->style.max_height = saved.max_height;
	memcpy(item->style.margin, saved.margin, sizeof(saved.margin));
	return error;
}

/* Measures an item's margin box's height. */
static layout_unit
grid_outer_height(
	const struct layout_box *item)
{
	/* The content, paddings, borders and margins. */
	return item->margin[CSS_TOP] + item->border[CSS_TOP] + item->padding[CSS_TOP] + item->height +
	    item->padding[CSS_BOTTOM] + item->border[CSS_BOTTOM] + item->margin[CSS_BOTTOM];
}

/* Resolves a track's length: pixels, or a percentage of the whole (none while measuring); anything else is zero. */
static layout_unit
grid_track_length(
	const struct css_length *length,
	layout_unit whole,
	int measuring)
{
	/* Pixels. */
	if (length->unit == CSS_UNIT_PX)
		return layout_from_px(length->value);

	/* A percentage of a definite whole. */
	if (length->unit == CSS_UNIT_PERCENT && measuring == 0)
		return (layout_unit)((float)whole * length->value / 100.0f) + layout_from_px(length->offset);

	/* Nothing definite. */
	return 0;
}

/* Frees what a layout's state holds. */
static void
grid_release(
	struct grid_state *grid)
{
	/* The arrays. */
	free(grid->items);
	free(grid->columns);
	free(grid->rows);
	free(grid->taken);
}
