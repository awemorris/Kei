/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * What a page's scripts ask the page about its layout and its selectors
 * (ws074-p031): the window's host callbacks behind querySelector,
 * getBoundingClientRect, clientWidth, scrollY and the like.
 *
 * A question about the geometry lays the page out first when the document
 * changed since the last layout, at the view's size and with its fonts,
 * as other browsers lay out when a script asks; a page whose view has not
 * given it fonts yet (or whose fonts do not open) has no layout, and its
 * nodes no boxes.  The selectors are matched with a style engine of their
 * own, which lives as long as the page: the styling's engine is made anew
 * at each change of the document.
 *
 * For ws074-p082 a script also asks for an element's computed style (its
 * box's, or the cascade's from the root down when it has no box: display
 * none, or no layout) and moves the scroll (kept inside the laid out
 * document; the view takes it as its own).
 */

#include "page/page.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/*
 * One slot of the page's index of boxes: a node and its first box (an
 * empty slot has no node).
 */
struct page_box_slot {
	const struct dom_node *node;
	const struct layout_box *box;
};

/* The fewest slots the index of boxes has. */
#define GEOMETRY_INDEX_MIN	64U

/* The deepest element whose style is computed without a box. */
#define GEOMETRY_STYLE_DEPTH	512

static int geometry_tree_box(const struct layout_tree *tree, const struct layout_box *first, struct dom_node *node, struct bind_box *box);
static int geometry_child_box(struct page *page, struct dom_node *node, struct bind_box *box);
static int geometry_layout(struct page *page);
static const struct layout_box *geometry_first_box(struct page *page, const struct dom_node *node);
static int geometry_own_place(const struct layout_box *box);
static int geometry_index(struct page *page);
static size_t geometry_count_boxes(const struct layout_box *box, int depth);
static void geometry_index_boxes(struct page *page, const struct layout_box *box, int depth);
static size_t geometry_slot_of(const struct page *page, const struct dom_node *node);
static int geometry_cascade(struct page *page, struct dom_element *element, struct css_style *style);

/*
 * Finds the style engine a script's selectors are matched with (the
 * bind_host's selector_engine), made when first asked for; NULL when it
 * cannot be made.
 */
struct css_engine *
page_selector_engine(
	void *context)
{
	struct page *page;
	int error;

	/* The engine made before. */
	page = context;
	if (page->query_css != NULL)
		return page->query_css;

	/* A new one; without memory, none. */
	error = css_engine_create(&page->query_css, page->heap);
	if (error != 0) {
		page->query_css = NULL;
		return NULL;
	}

	/* Succeeded: the engine. */
	return page->query_css;
}

/*
 * Finds where a node is on the page as it is laid out now (the
 * bind_host's node_box), laying it out first when it changed; reports
 * whether the node has a box.
 */
int
page_node_box(
	void *context,
	struct dom_node *node,
	struct bind_box *box)
{
	struct page *page;
	const struct layout_box *first;
	int laid_out;
	int found;

	/* The Page owns fonts and primary layout resources for this synchronous query. */
	page = context;
	memset(box, 0, sizeof(*box));

	/* Foreign nodes use their own active child cascade and actual content viewport. */
	if (node->document != page->document) {
		found = geometry_child_box(page, node, box);
		return found;
	}

	/* Primary layout keeps its existing indexed first-box lookup. */
	laid_out = geometry_layout(page);
	if (!laid_out)
		return 0;
	first = geometry_first_box(page, node);
	found = geometry_tree_box(&page->layout, first, node, box);
	if (!found)
		return 0;

	/* Succeeded: the current native primary layout supplied the node's geometry. */
	return 1;
}

/*
 * Finds the laid out document's width and height (the bind_host's
 * document_size), laying the page out first when it changed; 0 when it
 * cannot be laid out.
 */
void
page_document_size(
	void *context,
	double *width,
	double *height)
{
	struct page *page;
	int laid_out;

	/* Nothing until the page is laid out. */
	page = context;
	*width = 0.0;
	*height = 0.0;
	laid_out = geometry_layout(page);
	if (!laid_out)
		return;

	/* The viewport's width, and the document's height. */
	*width = layout_to_px(page->layout.viewport_width);
	*height = layout_to_px(page->layout.document_height);
}

/*
 * Finds how far the page's view is scrolled, in CSS pixels (the
 * bind_host's scroll).
 */
void
page_scroll(
	void *context,
	double *x,
	double *y)
{
	struct page *page;

	/* What the view last said. */
	page = context;
	*x = page->scroll_x;
	*y = page->scroll_y;
}

/* Finds the top element at a point in viewport coordinates. */
struct dom_node *
page_element_at(
	void *context,
	double x,
	double y)
{
	struct page *page;
	struct dom_node *node;
	int laid_out;

	/* Points outside the viewport do not hit the document. */
	page = context;
	if (!(x >= 0.0) || !(y >= 0.0) || x >= page->viewport_width || y >= page->viewport_height)
		return NULL;
	laid_out = geometry_layout(page);
	if (!laid_out)
		return NULL;

	/* Layout uses document coordinates; DOM hit testing returns an element. */
	x += page->scroll_x;
	y += page->scroll_y;
	node = layout_hit_node(&page->layout, (layout_unit)(x * LAYOUT_UNIT), (layout_unit)(y * LAYOUT_UNIT));
	while (node != NULL && node->type != DOM_ELEMENT)
		node = node->parent;
	return node;
}

/*
 * Computes an element's style (the bind_host's computed_style): its box's
 * when the page lays out and the element has one, otherwise the
 * cascade's.  Reports ENOENT for an element outside the document.
 */
int
page_computed_style(
	void *context,
	struct dom_element *element,
	struct css_style *style)
{
	const struct layout_box *box;
	const struct dom_node *walk;
	struct page *page;
	int laid_out;
	int error;

	/* An element outside the document has no style. */
	page = context;
	for (walk = &element->node; walk->parent != NULL; walk = walk->parent)
		continue;
	if (walk != &page->document->node)
		return ENOENT;

	/* The box's style, when the element has a box. */
	laid_out = geometry_layout(page);
	box = NULL;
	if (laid_out)
		box = geometry_first_box(page, &element->node);
	if (box != NULL) {
		*style = box->style;
		return 0;
	}

	/* Otherwise the cascade's. */
	error = geometry_cascade(page, element, style);
	if (error != 0)
		return error;

	/* Succeeded: the style. */
	return 0;
}

/*
 * Moves the page's scroll where a script asks (the bind_host's
 * scroll_to), kept inside the laid out document: the page does not scroll
 * sideways, and not past the last view's worth of the document.
 */
void
page_scroll_to(
	void *context,
	double x,
	double y)
{
	struct page *page;
	double limit;
	int laid_out;

	UNUSED_PARAMETER(x);

	/* The document's height less the view's is the furthest down. */
	page = context;
	limit = 0.0;
	laid_out = geometry_layout(page);
	if (laid_out)
		limit = layout_to_px(page->layout.document_height) - page->viewport_height;
	if (limit < 0.0)
		limit = 0.0;

	/* The place, within the limits (a number that is not one is the top). */
	if (!(y >= 0.0))
		y = 0.0;
	if (y > limit)
		y = limit;

	/* Succeeded: the view takes it. */
	page->scroll_x = 0.0;
	page->scroll_y = y;
	page->scroll_requested = 1;
}

/*
 * Frees the page's index of boxes.
 */
void
page_box_index_release(
	struct page *page)
{
	/* The slots. */
	free(page->box_index);
	page->box_index = NULL;
	page->box_index_capacity = 0;
}

/* Serializes the same actual border/content geometry from primary or child box trees. */
static int
geometry_tree_box(
	const struct layout_tree *tree,
	const struct layout_box *first,
	struct dom_node *node,
	struct bind_box *box)
{
	struct layout_rect rect;
	int own;
	int found;
	int block;

	/* A block's own border box differs from the union of ordinary inline fragments. */
	own = geometry_own_place(first);
	if (own) {
		rect.x = first->x;
		rect.y = first->y;
		rect.width = first->border[CSS_LEFT] + first->padding[CSS_LEFT] + first->width + first->padding[CSS_RIGHT] + first->border[CSS_RIGHT];
		rect.height = first->border[CSS_TOP] + first->padding[CSS_TOP] + first->height + first->padding[CSS_BOTTOM] + first->border[CSS_BOTTOM];
	} else {
		found = layout_node_bounds(tree, node, &rect);
		if (!found)
			return 0;
	}

	/* The rectangle in pixels. */
	box->x = layout_to_px(rect.x);
	box->y = layout_to_px(rect.y);
	box->width = layout_to_px(rect.width);
	box->height = layout_to_px(rect.height);

	/* The first box: a block (or a replaced or atomic box) has a client area and borders, an inline box not. */
	block = 0;
	if (first != NULL) {
		if (first->kind == LAYOUT_BLOCK) {
			block = 1;
		} else if (first->kind == LAYOUT_REPLACED) {
			block = 1;
		} else if (first->atomic) {
			block = 1;
		}
	}

	/* Its borders, in the order top, right, bottom, left. */
	box->block = block;
	if (block) {
		box->border_top = layout_to_px(first->border[0]);
		box->border_right = layout_to_px(first->border[1]);
		box->border_bottom = layout_to_px(first->border[2]);
		box->border_left = layout_to_px(first->border[3]);
	}

	/* Its used margins and paddings. */
	if (first != NULL) {
		box->margin_top = layout_to_px(first->margin[0]);
		box->margin_right = layout_to_px(first->margin[1]);
		box->margin_bottom = layout_to_px(first->margin[2]);
		box->margin_left = layout_to_px(first->margin[3]);
		box->padding_top = layout_to_px(first->padding[0]);
		box->padding_right = layout_to_px(first->padding[1]);
		box->padding_bottom = layout_to_px(first->padding[2]);
		box->padding_left = layout_to_px(first->padding[3]);
	}

	/* Succeeded: the node has a box. */
	return 1;
}

/* Builds a transient child layout without retaining native DOM edges in a C cache. */
static int
geometry_child_box(
	struct page *page,
	struct dom_node *node,
	struct bind_box *box)
{
	struct bind_window *window;
	struct dom_document *document;
	struct css_engine *engine;
	struct layout_tree tree;
	const struct layout_box *first;
	struct vm_cell *roots[2];
	unsigned index;
	int width;
	int height;
	int connected;
	int status;
	int found;

	/* Only a connected node of an actual binding-owned child can have child geometry. */
	document = node->document;
	window = document->view;
	if (window == NULL || document->context == NULL)
		return 0;
	connected = dom_is_inclusive_ancestor(&node->document->node, node);
	if (!connected)
		return 0;

	/* Owner Document and queried node survive parent layout callbacks that retire the child. */
	roots[0] = &document->node.cell;
	roots[1] = &node->cell;
	for (index = 0; index < 2U; index++) {
		status = vm_heap_add_root(page->heap, &roots[index]);
		if (status != 0) {
			/* Failed root publication releases only slots that were registered earlier. */
			while (index != 0) {
				index--;
				vm_heap_remove_root(page->heap, &roots[index]);
			}

			/* No partial root registration authorizes child layout. */
			return 0;
		}
	}

	/* Open borrowed fonts only after the actual child graph is rooted through primary allocations. */
	found = 0;
	engine = NULL;
	status = geometry_layout(page);
	if (status) {
		/* The binding bridge rejects retirement after its actual viewport refresh callback. */
		status = bind_window_child_styles(window, &engine, &width, &height);
	}

	/* A callback cannot publish layout for an adopted node or a retired owner. */
	if (node->document != document || document->view != window)
		engine = NULL;

	/* Only a successfully refreshed active owner supplies a borrowed engine to layout. */
	if (status == 0 && engine != NULL) {
		/* Checked viewport bounds precede the layout engine's signed fixed-unit multiplication. */
		if (width >= 0 &&
		    height >= 0 &&
		    width <= INT_MAX / LAYOUT_UNIT &&
		    height <= INT_MAX / LAYOUT_UNIT) {
			status = layout_build(&tree, engine, &page->text, document, page_image_of, page_image_by_url, page, width, height);
			if (status == 0) {
				first = layout_box_of(&tree, node);
				found = geometry_tree_box(&tree, first, node, box);
			}

			/* Every transient arena is released before returning to script or another query. */
			layout_release(&tree);
		}
	}

	/* No queried DOM or child context remains rooted by this completed observation. */
	for (index = 0; index < 2U; index++)
		vm_heap_remove_root(page->heap, &roots[index]);
	if (!found)
		return 0;

	/* Succeeded: actual child layout supplied a copied native box without retained storage. */
	return 1;
}

/* Lays the primary Page out with its real fonts before synchronous geometry queries. */
static int
geometry_layout(
	struct page *page)
{
	int changed;
	int error;

	/* A page laid out as it is now. */
	changed = page_needs_layout(page);
	if (!changed)
		return 1;

	/* A page whose view has given it no fonts or no size yet cannot be laid out. */
	if (page->font_paths == NULL)
		return 0;
	if (page->viewport_width <= 0 || page->viewport_height <= 0)
		return 0;

	/* The fonts, opened once. */
	error = page_open_fonts(page, page->font_paths);
	if (error != 0)
		return 0;

	/* The layout; one that fails leaves the page without boxes. */
	error = page_layout(page, page->viewport_width, page->viewport_height);
	if (error != 0)
		return 0;

	/* Succeeded: the page is laid out. */
	return 1;
}

/*
 * Finds a node's first box (in tree order) in the page's layout through
 * the index, made again after each layout; NULL when the node has none.
 * Without memory for the index the layout is searched.
 */
static const struct layout_box *
geometry_first_box(
	struct page *page,
	const struct dom_node *node)
{
	size_t slot;
	int error;

	/* The index of this layout. */
	error = geometry_index(page);
	if (error != 0)
		return layout_box_of(&page->layout, node);

	/* Succeeded: the node's slot's box, or none. */
	slot = geometry_slot_of(page, node);
	return page->box_index[slot].box;
}

/*
 * Tells whether a node's first box has a place of its own that is the
 * node's rectangle: a block, or a replaced box outside a line.
 */
static int
geometry_own_place(
	const struct layout_box *box)
{
	/* No box. */
	if (box == NULL)
		return 0;

	/* A block, atomic or not. */
	if (box->kind == LAYOUT_BLOCK)
		return 1;

	/* A replaced box among inline content is a piece of its line. */
	if (box->kind != LAYOUT_REPLACED)
		return 0;
	if (box->parent != NULL && box->parent->children_inline)
		return 0;

	/* Succeeded: a replaced box that stands as a block. */
	return 1;
}

/*
 * Makes the index of the page's boxes when the layout changed since it
 * was made: twice as many slots as boxes, a power of two.
 */
static int
geometry_index(
	struct page *page)
{
	size_t count;
	size_t capacity;

	/* An index of this layout stays. */
	if (page->box_index != NULL && page->box_index_serial == page->layout_serial)
		return 0;

	/* The slots for the boxes there are. */
	count = 0;
	if (page->layout.root != NULL)
		count = geometry_count_boxes(page->layout.root, 0);
	capacity = GEOMETRY_INDEX_MIN;
	while (capacity < count * 2U)
		capacity *= 2U;

	/* Empty slots. */
	page_box_index_release(page);
	page->box_index = calloc(capacity, sizeof(*page->box_index));
	if (page->box_index == NULL)
		return ENOMEM;
	page->box_index_capacity = capacity;

	/* Succeeded: each node's first box is in its slot. */
	if (page->layout.root != NULL)
		geometry_index_boxes(page, page->layout.root, 0);
	page->box_index_serial = page->layout_serial;
	return 0;
}

/* Counts a box and its descendants, down to the depth the layout stops at. */
static size_t
geometry_count_boxes(
	const struct layout_box *box,
	int depth)
{
	const struct layout_box *child;
	size_t count;

	/* Past the depth there are none. */
	if (depth > LAYOUT_DEPTH_MAX)
		return 0;

	/* The box and each child's. */
	count = 1;
	for (child = box->first_child; child != NULL; child = child->next)
		count += geometry_count_boxes(child, depth + 1);
	return count;
}

/* Puts a box, then its descendants in tree order, in the index where their nodes have no box yet. */
static void
geometry_index_boxes(
	struct page *page,
	const struct layout_box *box,
	int depth)
{
	const struct layout_box *child;
	size_t slot;

	/* Past the depth the layout stops at. */
	if (depth > LAYOUT_DEPTH_MAX)
		return;

	/* The node's first box. */
	if (box->node != NULL) {
		slot = geometry_slot_of(page, box->node);
		if (page->box_index[slot].node == NULL) {
			page->box_index[slot].node = box->node;
			page->box_index[slot].box = box;
		}
	}

	/* Each child. */
	for (child = box->first_child; child != NULL; child = child->next)
		geometry_index_boxes(page, child, depth + 1);
}

/* Finds a node's slot in the index: its own, or the empty one it would take. */
static size_t
geometry_slot_of(
	const struct page *page,
	const struct dom_node *node)
{
	size_t mask;
	size_t slot;

	/* From the hash of the node's address, on to the next slot while another node has it. */
	mask = page->box_index_capacity - 1U;
	slot = (size_t)((((uintptr_t)node >> 4) * 2654435761U) & mask);
	while (page->box_index[slot].node != NULL && page->box_index[slot].node != node)
		slot = (slot + 1U) & mask;
	return slot;
}

/*
 * Computes an element's style with the cascade, from the root element
 * down to it (an element without a box has no style of its own kept).
 */
static int
geometry_cascade(
	struct page *page,
	struct dom_element *element,
	struct css_style *style)
{
	struct dom_element **chain;
	struct css_style *parent;
	const struct css_style *inherited;
	struct dom_node *walk;
	size_t count;
	size_t index;
	int error;

	/* The styles as the document and its sheets are now. */
	error = page_update_styles(page);
	if (error != 0)
		return error;

	/* The element's ancestors that are elements, it first. */
	chain = malloc(GEOMETRY_STYLE_DEPTH * sizeof(*chain));
	if (chain == NULL)
		return ENOMEM;
	count = 0;
	for (walk = &element->node; walk != NULL && walk->type == DOM_ELEMENT && count < GEOMETRY_STYLE_DEPTH; walk = walk->parent) {
		chain[count] = (struct dom_element *)walk;
		count++;
	}

	/* The parent's style, which each step computes the next one from. */
	parent = malloc(sizeof(*parent));
	if (parent == NULL) {
		free(chain);
		return ENOMEM;
	}

	/* Each one from the root down; the last is the element's. */
	inherited = NULL;
	for (index = count; index > 0; index--) {
		error = css_engine_compute(page->css, chain[index - 1U], inherited, style);
		if (error != 0)
			break;

		/* The next one inherits from this one. */
		*parent = *style;
		inherited = parent;
	}

	/* The chain and the copy are no longer needed. */
	free(parent);
	free(chain);
	if (error != 0)
		return error;

	/* Succeeded: the style. */
	return 0;
}
