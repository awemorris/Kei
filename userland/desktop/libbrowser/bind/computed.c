/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The computed style of elements for scripts (ws074-p082):
 * window.getComputedStyle.
 *
 * The declaration is a CSSStyleDeclaration (the prototype of the inline
 * style, whose getters come here for it) that holds its element and asks
 * the host for the element's style each time a value is read, so it
 * follows the document as other browsers' does.  A value is the CSSOM's
 * resolved value: the used width, height, margins and paddings of an
 * element with a box, and the computed value of anything else, written
 * as other browsers write them (lengths in px, colors as rgb() or
 * rgba()).  An element outside the document reads as empty strings.  The
 * declaration cannot be changed (NoModificationAllowedError), its cssText
 * is empty (as in Chromium), and a pseudo-element's style is the
 * element's.
 */

#include "bind/internal.h"
#include "css/css.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * The properties a computed declaration reports, in the order item()
 * lists them.
 */
enum computed_property {
	COMPUTED_DISPLAY,
	COMPUTED_POSITION,
	COMPUTED_FLOAT,
	COMPUTED_CLEAR,
	COMPUTED_VISIBILITY,
	COMPUTED_OVERFLOW,
	COMPUTED_OVERFLOW_X,
	COMPUTED_OVERFLOW_Y,
	COMPUTED_BOX_SIZING,
	COMPUTED_WIDTH,
	COMPUTED_HEIGHT,
	COMPUTED_MIN_WIDTH,
	COMPUTED_MIN_HEIGHT,
	COMPUTED_MAX_WIDTH,
	COMPUTED_MAX_HEIGHT,
	COMPUTED_MARGIN_TOP,
	COMPUTED_MARGIN_RIGHT,
	COMPUTED_MARGIN_BOTTOM,
	COMPUTED_MARGIN_LEFT,
	COMPUTED_PADDING_TOP,
	COMPUTED_PADDING_RIGHT,
	COMPUTED_PADDING_BOTTOM,
	COMPUTED_PADDING_LEFT,
	COMPUTED_TOP,
	COMPUTED_RIGHT,
	COMPUTED_BOTTOM,
	COMPUTED_LEFT,
	COMPUTED_BORDER_TOP_WIDTH,
	COMPUTED_BORDER_RIGHT_WIDTH,
	COMPUTED_BORDER_BOTTOM_WIDTH,
	COMPUTED_BORDER_LEFT_WIDTH,
	COMPUTED_BORDER_TOP_STYLE,
	COMPUTED_BORDER_RIGHT_STYLE,
	COMPUTED_BORDER_BOTTOM_STYLE,
	COMPUTED_BORDER_LEFT_STYLE,
	COMPUTED_BORDER_TOP_COLOR,
	COMPUTED_BORDER_RIGHT_COLOR,
	COMPUTED_BORDER_BOTTOM_COLOR,
	COMPUTED_BORDER_LEFT_COLOR,
	COMPUTED_COLOR,
	COMPUTED_BACKGROUND_COLOR,
	COMPUTED_BACKGROUND_IMAGE,
	COMPUTED_OPACITY,
	COMPUTED_Z_INDEX,
	COMPUTED_FONT_SIZE,
	COMPUTED_FONT_WEIGHT,
	COMPUTED_FONT_STYLE,
	COMPUTED_FONT_FAMILY,
	COMPUTED_LINE_HEIGHT,
	COMPUTED_TEXT_ALIGN,
	COMPUTED_TEXT_INDENT,
	COMPUTED_DIRECTION,
	COMPUTED_WHITE_SPACE,
	COMPUTED_CURSOR,
	COMPUTED_VERTICAL_ALIGN,
	COMPUTED_LIST_STYLE_TYPE,
	COMPUTED_BORDER_COLLAPSE,
	COMPUTED_FLEX_DIRECTION,
	COMPUTED_FLEX_WRAP,
	COMPUTED_FLEX_GROW,
	COMPUTED_FLEX_SHRINK,
	COMPUTED_FLEX_BASIS,
	COMPUTED_JUSTIFY_CONTENT,
	COMPUTED_ALIGN_ITEMS,
	COMPUTED_ALIGN_SELF,
	COMPUTED_ALIGN_CONTENT,
	COMPUTED_ORDER,
	COMPUTED_ROW_GAP,
	COMPUTED_COLUMN_GAP,
	COMPUTED_OUTLINE_WIDTH,
	COMPUTED_OUTLINE_STYLE,
	COMPUTED_OUTLINE_COLOR,
	COMPUTED_TRANSFORM,
	COMPUTED_GRID_TEMPLATE_COLUMNS,
	COMPUTED_TEXT_TRANSFORM
};

/*
 * One property of a computed declaration: its CSS name and what it is.
 */
struct computed_entry {
	const char *name;
	int property;
};

/*
 * What a computed declaration holds: its element's object.
 */
struct computed_ref {
	struct vm_cell cell;
	vm_value element;
};

static void computed_trace(struct vm_heap *heap, struct vm_cell *cell);
static int computed_find(const struct vm_string *name, int *property);
static int computed_write(struct bind_window *window, struct dom_element *element, int property, struct wb_units *out);
static int computed_write_used(const struct css_style *style, const struct bind_box *box, int found, int property, struct wb_units *out);
static int computed_write_keyword(const struct css_style *style, int property, struct wb_units *out);
static int computed_write_overflow(const struct css_style *style, struct wb_units *out);
static int computed_write_url(const struct vm_string *url, struct wb_units *out);
static int computed_needs_quotes(const struct vm_string *family);
static int computed_write_percent(const struct css_length *length, struct wb_units *out);
static int computed_write_url_quoted(const struct vm_string *text, struct wb_units *out);
static int computed_write_offset(const struct css_style *style, int side, struct wb_units *out);
static int computed_write_font_family(const struct css_style *style, struct wb_units *out);
static int computed_write_tracks(const struct css_style *style, struct wb_units *out);
static int computed_name(const char *const *names, size_t count, int value, struct wb_units *out);
static int computed_length(const struct css_length *length, struct wb_units *out);
static int computed_pixels(double value, struct wb_units *out);
static int computed_number(double value, struct wb_units *out);
static int computed_color(uint32_t color, struct wb_units *out);
static int computed_ascii(const char *ascii, struct wb_units *out);

/* The state of a computed declaration, which marks its element's object. */
static const struct vm_cell_type computed_ref_type = { "computed-style", computed_trace, NULL };

/*
 * The properties, in the order item() lists them.  The table is constant
 * for the life of the program.
 */
static const struct computed_entry computed_entries[] = {
	{ "display", COMPUTED_DISPLAY },
	{ "position", COMPUTED_POSITION },
	{ "float", COMPUTED_FLOAT },
	{ "clear", COMPUTED_CLEAR },
	{ "visibility", COMPUTED_VISIBILITY },
	{ "overflow", COMPUTED_OVERFLOW },
	{ "overflow-x", COMPUTED_OVERFLOW_X },
	{ "overflow-y", COMPUTED_OVERFLOW_Y },
	{ "box-sizing", COMPUTED_BOX_SIZING },
	{ "width", COMPUTED_WIDTH },
	{ "height", COMPUTED_HEIGHT },
	{ "min-width", COMPUTED_MIN_WIDTH },
	{ "min-height", COMPUTED_MIN_HEIGHT },
	{ "max-width", COMPUTED_MAX_WIDTH },
	{ "max-height", COMPUTED_MAX_HEIGHT },
	{ "margin-top", COMPUTED_MARGIN_TOP },
	{ "margin-right", COMPUTED_MARGIN_RIGHT },
	{ "margin-bottom", COMPUTED_MARGIN_BOTTOM },
	{ "margin-left", COMPUTED_MARGIN_LEFT },
	{ "padding-top", COMPUTED_PADDING_TOP },
	{ "padding-right", COMPUTED_PADDING_RIGHT },
	{ "padding-bottom", COMPUTED_PADDING_BOTTOM },
	{ "padding-left", COMPUTED_PADDING_LEFT },
	{ "top", COMPUTED_TOP },
	{ "right", COMPUTED_RIGHT },
	{ "bottom", COMPUTED_BOTTOM },
	{ "left", COMPUTED_LEFT },
	{ "border-top-width", COMPUTED_BORDER_TOP_WIDTH },
	{ "border-right-width", COMPUTED_BORDER_RIGHT_WIDTH },
	{ "border-bottom-width", COMPUTED_BORDER_BOTTOM_WIDTH },
	{ "border-left-width", COMPUTED_BORDER_LEFT_WIDTH },
	{ "border-top-style", COMPUTED_BORDER_TOP_STYLE },
	{ "border-right-style", COMPUTED_BORDER_RIGHT_STYLE },
	{ "border-bottom-style", COMPUTED_BORDER_BOTTOM_STYLE },
	{ "border-left-style", COMPUTED_BORDER_LEFT_STYLE },
	{ "border-top-color", COMPUTED_BORDER_TOP_COLOR },
	{ "border-right-color", COMPUTED_BORDER_RIGHT_COLOR },
	{ "border-bottom-color", COMPUTED_BORDER_BOTTOM_COLOR },
	{ "border-left-color", COMPUTED_BORDER_LEFT_COLOR },
	{ "color", COMPUTED_COLOR },
	{ "background-color", COMPUTED_BACKGROUND_COLOR },
	{ "background-image", COMPUTED_BACKGROUND_IMAGE },
	{ "opacity", COMPUTED_OPACITY },
	{ "z-index", COMPUTED_Z_INDEX },
	{ "font-size", COMPUTED_FONT_SIZE },
	{ "font-weight", COMPUTED_FONT_WEIGHT },
	{ "font-style", COMPUTED_FONT_STYLE },
	{ "font-family", COMPUTED_FONT_FAMILY },
	{ "line-height", COMPUTED_LINE_HEIGHT },
	{ "text-align", COMPUTED_TEXT_ALIGN },
	{ "text-indent", COMPUTED_TEXT_INDENT },
	{ "direction", COMPUTED_DIRECTION },
	{ "white-space", COMPUTED_WHITE_SPACE },
	{ "text-transform", COMPUTED_TEXT_TRANSFORM },
	{ "cursor", COMPUTED_CURSOR },
	{ "vertical-align", COMPUTED_VERTICAL_ALIGN },
	{ "list-style-type", COMPUTED_LIST_STYLE_TYPE },
	{ "border-collapse", COMPUTED_BORDER_COLLAPSE },
	{ "flex-direction", COMPUTED_FLEX_DIRECTION },
	{ "flex-wrap", COMPUTED_FLEX_WRAP },
	{ "flex-grow", COMPUTED_FLEX_GROW },
	{ "flex-shrink", COMPUTED_FLEX_SHRINK },
	{ "flex-basis", COMPUTED_FLEX_BASIS },
	{ "justify-content", COMPUTED_JUSTIFY_CONTENT },
	{ "align-items", COMPUTED_ALIGN_ITEMS },
	{ "align-self", COMPUTED_ALIGN_SELF },
	{ "align-content", COMPUTED_ALIGN_CONTENT },
	{ "order", COMPUTED_ORDER },
	{ "row-gap", COMPUTED_ROW_GAP },
	{ "column-gap", COMPUTED_COLUMN_GAP },
	{ "outline-width", COMPUTED_OUTLINE_WIDTH },
	{ "outline-style", COMPUTED_OUTLINE_STYLE },
	{ "outline-color", COMPUTED_OUTLINE_COLOR },
	{ "transform", COMPUTED_TRANSFORM },
	{ "grid-template-columns", COMPUTED_GRID_TEMPLATE_COLUMNS },
	{ NULL, 0 }
};

/* The names of display, by enum css_display. */
static const char *const computed_displays[] = {
	"inline", "block", "inline-block", "list-item", "none", "table", "table-row", "table-cell", "flex", "contents", "grid",
	"table-row-group", "table-caption", "inline-table", "table-column"
};

/* The names of position, by enum css_position. */
static const char *const computed_positions[] = { "static", "relative", "absolute", "fixed", "sticky" };

/* The names of float and clear, by enum css_float. */
static const char *const computed_floats[] = { "none", "left", "right", "both" };

/* The names of visibility. */
static const char *const computed_visibilities[] = { "visible", "hidden" };

/* The names of overflow, by enum css_overflow. */
static const char *const computed_overflows[] = { "visible", "hidden", "clip", "scroll", "auto" };

/* The names of box-sizing, by enum css_box_sizing. */
static const char *const computed_box_sizings[] = { "content-box", "border-box" };

/* The names of the border styles, by enum css_border_style. */
static const char *const computed_border_styles[] = { "none", "solid", "dashed", "dotted", "double", "inset", "outset" };

/* The names of text-align, by enum css_text_align. */
static const char *const computed_text_aligns[] = { "start", "left", "right", "center", "justify", "end" };

/* The names of direction, by enum css_direction. */
static const char *const computed_directions[] = { "ltr", "rtl" };

/* The canonical cursor names, indexed by enum css_cursor for computed serialization. */
static const char *const computed_cursors[] = {
	"auto",
	"default",
	"none",
	"context-menu",
	"help",
	"pointer",
	"progress",
	"wait",
	"cell",
	"crosshair",
	"text",
	"vertical-text",
	"alias",
	"copy",
	"move",
	"no-drop",
	"not-allowed",
	"e-resize",
	"n-resize",
	"ne-resize",
	"nw-resize",
	"s-resize",
	"se-resize",
	"sw-resize",
	"w-resize",
	"ew-resize",
	"ns-resize",
	"nesw-resize",
	"nwse-resize",
	"col-resize",
	"row-resize",
	"all-scroll",
	"grab",
	"grabbing",
	"zoom-in",
	"zoom-out",
};

/* The resolved CSS2 transform names follow the native computed keyword enum. */
static const char *const computed_text_transforms[] = { "none", "capitalize", "uppercase", "lowercase" };

/* The names of white-space, by enum css_white_space. */
static const char *const computed_white_spaces[] = { "normal", "pre", "nowrap", "pre-wrap", "pre-line" };

/* The names of vertical-align's keywords, by enum css_vertical_align. */
static const char *const computed_vertical_aligns[] = {
	"baseline", "top", "middle", "bottom", "text-top", "text-bottom", "sub", "super"
};

/* The names of list-style-type, by enum css_list_style. */
static const char *const computed_list_styles[] = { "disc", "circle", "square", "decimal", "none" };

/* The names of border-collapse. */
static const char *const computed_collapses[] = { "separate", "collapse" };

/* The names of flex-direction, by enum css_flex_direction. */
static const char *const computed_flex_directions[] = { "row", "row-reverse", "column", "column-reverse" };

/* The names of flex-wrap, by enum css_flex_wrap. */
static const char *const computed_flex_wraps[] = { "nowrap", "wrap", "wrap-reverse" };

/* The names of the alignments, by enum css_align (stretch is normal's). */
static const char *const computed_aligns[] = {
	"auto", "normal", "flex-start", "flex-end", "center", "baseline", "space-between", "space-around", "space-evenly"
};

/*
 * Makes the computed declaration of an element's style
 * (window.getComputedStyle); the pseudo-element argument is not read.
 */
int
bind_get_computed_style(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct computed_ref *ref;
	struct vm_object *declaration;
	struct vm_cell *ref_root;
	struct dom_node *node;
	vm_value given;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The element. */
	window = bind_window_of(realm);
	given = js_argument(args, count, 0);
	node = bind_node_of(given);
	if (node == NULL || node->type != DOM_ELEMENT) {
		status = vm_throw_type_error(realm, "Failed to execute 'getComputedStyle' on 'Window': parameter 1 is not of type 'Element'.");
		return status;
	}

	/* The state, which keeps the element's object. */
	ref = vm_heap_alloc(realm->heap, &computed_ref_type, sizeof(*ref));
	if (ref == NULL)
		return ENOMEM;
	ref->element = given;
	ref_root = &ref->cell;
	status = vm_heap_add_root(realm->heap, &ref_root);
	if (status != 0)
		return status;

	/* The declaration with CSSStyleDeclaration's prototype. */
	declaration = vm_object_create(realm->heap, window->prototypes[BIND_CSS_STYLE_DECLARATION]);
	if (declaration == NULL) {
		vm_heap_remove_root(realm->heap, &ref_root);
		return ENOMEM;
	}

	/* The published declaration now traces its private computed-style state. */
	declaration->kind = VM_KIND_PLATFORM;
	declaration->internal = vm_value_cell(ref);

	/* Succeeded: the declaration. */
	*result = vm_value_cell(declaration);
	vm_heap_remove_root(realm->heap, &ref_root);
	return 0;
}

/*
 * Tells whether a value is a computed declaration, and finds its element
 * when it is.
 */
int
bind_computed_element(
	vm_value value,
	struct dom_element **element)
{
	struct computed_ref *ref;
	struct vm_object *object;
	struct vm_cell *cell;
	int is_object;
	int is_cell;

	/* A platform object. */
	*element = NULL;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;
	object = (struct vm_object *)vm_value_as_cell(value);
	if (object->kind != VM_KIND_PLATFORM)
		return 0;

	/* Whose cell is a computed declaration's state. */
	is_cell = vm_value_is_cell(object->internal);
	if (!is_cell)
		return 0;
	cell = vm_value_as_cell(object->internal);
	if (cell->type != &computed_ref_type)
		return 0;

	/* Succeeded: its element. */
	ref = (struct computed_ref *)cell;
	*element = (struct dom_element *)bind_node_of(ref->element);
	return 1;
}

/*
 * Reports the resolved value of a property (by its CSS name) of an
 * element's style, or the empty string for a property this pass does not
 * report or an element outside the document.
 */
int
bind_computed_value(
	struct vm_realm *realm,
	struct dom_element *element,
	const struct vm_string *name,
	vm_value *result)
{
	struct bind_window *window;
	struct wb_units units;
	int property;
	int found;
	int status;

	/* A property not reported is empty. */
	found = computed_find(name, &property);
	if (!found) {
		status = bind_string(realm, "", result);
		return status;
	}

	/* Uses the element's owning Document even for a borrowed parent method. */
	window = bind_window_of(realm);
	if (element->node.document->view != NULL)
		window = element->node.document->view;

	/* The value's text. */
	wb_units_init(&units);
	status = computed_write(window, element, property, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Succeeded: as a string. */
	status = bind_units(realm, units.data, units.length, result);
	wb_units_release(&units);
	if (status != 0)
		return status;
	return 0;
}

/* Reports how many properties a computed declaration lists (length). */
size_t
bind_computed_count(void)
{
	/* The table's entries. */
	return sizeof(computed_entries) / sizeof(computed_entries[0]) - 1U;
}

/* Reports the name of the property at a place of the list, or NULL past its end (item). */
const char *
bind_computed_name(
	size_t index)
{
	size_t count;

	/* Past the end there is none. */
	count = bind_computed_count();
	if (index >= count)
		return NULL;

	/* Succeeded: the name. */
	return computed_entries[index].name;
}

/* Marks the element's object a computed declaration holds. */
static void
computed_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct computed_ref *ref;

	/* The element. */
	ref = (struct computed_ref *)cell;
	vm_heap_mark_value(heap, ref->element);
}

/* Finds the property a CSS name reports; reports whether there is one. */
static int
computed_find(
	const struct vm_string *name,
	int *property)
{
	size_t index;
	int same;

	/* Each entry's name. */
	for (index = 0; computed_entries[index].name != NULL; index++) {
		same = vm_string_equal_ascii(name, computed_entries[index].name);
		if (same) {
			*property = computed_entries[index].property;
			return 1;
		}
	}

	/* No such property. */
	return 0;
}

/*
 * Writes the resolved value of a property of an element: nothing for an
 * element outside the document.
 */
static int
computed_write(
	struct bind_window *window,
	struct dom_element *element,
	int property,
	struct wb_units *out)
{
	struct css_style *style;
	struct bind_box box;
	int found;
	int error;

	/* Only active initial children can compute without a primary page host. */
	if (window->host.computed_style == NULL) {
		if (!window->owned ||
		    window->context_depth == 0 ||
		    window->detached)
			return 0;
	}

	/* The element's style (on the heap: it is large). */
	style = malloc(sizeof(*style));
	if (style == NULL)
		return ENOMEM;

	/* Primary pages keep their host path; initial children own their cascade. */
	if (window->host.computed_style != NULL) {
		error = window->host.computed_style(window->host.context, element, style);
	} else {
		error = bind_style_context_compute(window, element, style);
	}

	/* Detached or disconnected elements have empty resolved values. */
	if (error == ENOENT) {
		free(style);
		return 0;
	}

	/* Any other failure. */
	if (error != 0) {
		free(style);
		return error;
	}

	/* Its box, for the used values. */
	memset(&box, 0, sizeof(box));
	found = 0;
	if (window->host.node_box != NULL)
		found = window->host.node_box(window->host.context, &element->node, &box);

	/* Succeeded: the value, and the style is freed. */
	error = computed_write_used(style, &box, found, property, out);
	free(style);
	if (error != 0)
		return error;
	return 0;
}

/*
 * Writes a property's resolved value: the used value of the sizes, the
 * margins and the paddings of an element with a box, and the computed
 * value of the others.
 */
static int
computed_write_used(
	const struct css_style *style,
	const struct bind_box *box,
	int found,
	int property,
	struct wb_units *out)
{
	double width;
	double height;
	double value;
	int has_used;
	int used;
	int error;

	/* A block's (or a replaced or atomic box's) sizes are used ones; an inline box's are not. */
	used = 0;
	if (found && box->block && style->display != CSS_DISPLAY_NONE)
		used = 1;

	/* The content box's, or with border-box sizing the border box's. */
	width = box->width;
	height = box->height;
	if (style->box_sizing != CSS_BOX_SIZING_BORDER) {
		width -= box->border_left + box->border_right + box->padding_left + box->padding_right;
		height -= box->border_top + box->border_bottom + box->padding_top + box->padding_bottom;
	}

	/* The used value of the properties that have one. */
	value = 0.0;
	has_used = 0;
	switch (property) {
	case COMPUTED_WIDTH:
		has_used = used;
		value = width;
		break;
	case COMPUTED_HEIGHT:
		has_used = used;
		value = height;
		break;
	case COMPUTED_MARGIN_TOP:
		has_used = found;
		value = box->margin_top;
		break;
	case COMPUTED_MARGIN_RIGHT:
		has_used = found;
		value = box->margin_right;
		break;
	case COMPUTED_MARGIN_BOTTOM:
		has_used = found;
		value = box->margin_bottom;
		break;
	case COMPUTED_MARGIN_LEFT:
		has_used = found;
		value = box->margin_left;
		break;
	case COMPUTED_PADDING_TOP:
		has_used = found;
		value = box->padding_top;
		break;
	case COMPUTED_PADDING_RIGHT:
		has_used = found;
		value = box->padding_right;
		break;
	case COMPUTED_PADDING_BOTTOM:
		has_used = found;
		value = box->padding_bottom;
		break;
	case COMPUTED_PADDING_LEFT:
		has_used = found;
		value = box->padding_left;
		break;
	default:
		break;
	}

	/* A used value, in pixels. */
	if (has_used) {
		error = computed_pixels(value, out);
		return error;
	}

	/* Every other property's computed value. */
	error = computed_write_keyword(style, property, out);
	if (error != 0)
		return error;

	/* Succeeded: the value is written. */
	return 0;
}

/* Writes a property's computed value (the ones that do not depend on the layout). */
static int
computed_write_keyword(
	const struct css_style *style,
	int property,
	struct wb_units *out)
{
	int error;

	/* Chooses by the property. */
	switch (property) {
	case COMPUTED_DISPLAY:
		error = computed_name(computed_displays, sizeof(computed_displays) / sizeof(computed_displays[0]), style->display, out);
		break;
	case COMPUTED_POSITION:
		error = computed_name(computed_positions, sizeof(computed_positions) / sizeof(computed_positions[0]), style->position, out);
		break;
	case COMPUTED_FLOAT:
		error = computed_name(computed_floats, 3, style->float_side, out);
		break;
	case COMPUTED_CLEAR:
		error = computed_name(computed_floats, 4, style->clear, out);
		break;
	case COMPUTED_VISIBILITY:
		error = computed_name(computed_visibilities, 2, style->visibility, out);
		break;
	case COMPUTED_OVERFLOW:
		error = computed_write_overflow(style, out);
		break;
	case COMPUTED_OVERFLOW_X:
		error = computed_name(computed_overflows, 5, style->overflow_x, out);
		break;
	case COMPUTED_OVERFLOW_Y:
		error = computed_name(computed_overflows, 5, style->overflow_y, out);
		break;
	case COMPUTED_BOX_SIZING:
		error = computed_name(computed_box_sizings, 2, style->box_sizing, out);
		break;
	case COMPUTED_WIDTH:
		error = computed_length(&style->width, out);
		break;
	case COMPUTED_HEIGHT:
		error = computed_length(&style->height, out);
		break;
	case COMPUTED_MARGIN_TOP:
	case COMPUTED_MARGIN_RIGHT:
	case COMPUTED_MARGIN_BOTTOM:
	case COMPUTED_MARGIN_LEFT:
		error = computed_length(&style->margin[property - COMPUTED_MARGIN_TOP], out);
		break;
	case COMPUTED_PADDING_TOP:
	case COMPUTED_PADDING_RIGHT:
	case COMPUTED_PADDING_BOTTOM:
	case COMPUTED_PADDING_LEFT:
		error = computed_length(&style->padding[property - COMPUTED_PADDING_TOP], out);
		break;
	case COMPUTED_MIN_WIDTH:
		error = computed_length(&style->min_width, out);
		break;
	case COMPUTED_MIN_HEIGHT:
		error = computed_length(&style->min_height, out);
		break;
	case COMPUTED_MAX_WIDTH:
		error = computed_length(&style->max_width, out);
		break;
	case COMPUTED_MAX_HEIGHT:
		error = computed_length(&style->max_height, out);
		break;
	case COMPUTED_TOP:
	case COMPUTED_RIGHT:
	case COMPUTED_BOTTOM:
	case COMPUTED_LEFT:
		error = computed_write_offset(style, property - COMPUTED_TOP, out);
		break;
	case COMPUTED_BORDER_TOP_WIDTH:
	case COMPUTED_BORDER_RIGHT_WIDTH:
	case COMPUTED_BORDER_BOTTOM_WIDTH:
	case COMPUTED_BORDER_LEFT_WIDTH:
		error = computed_pixels(style->border_width[property - COMPUTED_BORDER_TOP_WIDTH], out);
		break;
	case COMPUTED_BORDER_TOP_STYLE:
	case COMPUTED_BORDER_RIGHT_STYLE:
	case COMPUTED_BORDER_BOTTOM_STYLE:
	case COMPUTED_BORDER_LEFT_STYLE:
		error = computed_name(computed_border_styles, 7, style->border_style[property - COMPUTED_BORDER_TOP_STYLE], out);
		break;
	case COMPUTED_BORDER_TOP_COLOR:
	case COMPUTED_BORDER_RIGHT_COLOR:
	case COMPUTED_BORDER_BOTTOM_COLOR:
	case COMPUTED_BORDER_LEFT_COLOR:
		error = computed_color(style->border_color[property - COMPUTED_BORDER_TOP_COLOR], out);
		break;
	case COMPUTED_COLOR:
		error = computed_color(style->color, out);
		break;
	case COMPUTED_BACKGROUND_COLOR:
		error = computed_color(style->background_color, out);
		break;
	case COMPUTED_BACKGROUND_IMAGE:
		/* None, or the URL in quotes. */
		if (style->background_image == NULL) {
			error = computed_ascii("none", out);
		} else {
			error = computed_write_url(style->background_image, out);
		}

		break;
	case COMPUTED_OPACITY:
		error = computed_number(style->opacity, out);
		break;
	case COMPUTED_Z_INDEX:
		/* Auto, or the level. */
		if (style->z_index_auto) {
			error = computed_ascii("auto", out);
		} else {
			error = computed_number(style->z_index, out);
		}

		break;
	case COMPUTED_FONT_SIZE:
		error = computed_pixels(style->font_size, out);
		break;
	case COMPUTED_FONT_WEIGHT:
		error = computed_number(style->font_weight, out);
		break;
	case COMPUTED_FONT_STYLE:
		/* Italic, or normal. */
		if (style->font_italic) {
			error = computed_ascii("italic", out);
		} else {
			error = computed_ascii("normal", out);
		}

		break;
	case COMPUTED_FONT_FAMILY:
		error = computed_write_font_family(style, out);
		break;
	case COMPUTED_LINE_HEIGHT:
		/* A number is that many times the font size. */
		if (style->line_height.unit == CSS_UNIT_NUMBER) {
			error = computed_pixels(style->line_height.value * style->font_size, out);
		} else {
			error = computed_length(&style->line_height, out);
		}

		break;
	case COMPUTED_TEXT_ALIGN:
		error = computed_name(computed_text_aligns, 6, style->text_align, out);
		break;
	case COMPUTED_TEXT_INDENT:
		error = computed_length(&style->text_indent, out);
		break;
	case COMPUTED_DIRECTION:
		error = computed_name(computed_directions, 2, style->direction, out);
		break;
	case COMPUTED_CURSOR:
		error = computed_name(computed_cursors, sizeof(computed_cursors) / sizeof(computed_cursors[0]), style->cursor, out);
		break;
	case COMPUTED_TEXT_TRANSFORM:
		/* Serialize the actual cascaded keyword in CSS and camel-case accessors alike. */
		error = computed_name(computed_text_transforms, 4, style->text_transform, out);
		break;
	case COMPUTED_WHITE_SPACE:
		error = computed_name(computed_white_spaces, 5, style->white_space, out);
		break;
	case COMPUTED_VERTICAL_ALIGN:
		/* A length, or a keyword. */
		if (style->vertical_align == CSS_VALIGN_LENGTH) {
			error = computed_length(&style->vertical_offset, out);
		} else {
			error = computed_name(computed_vertical_aligns, 8, style->vertical_align, out);
		}

		break;
	case COMPUTED_LIST_STYLE_TYPE:
		error = computed_name(computed_list_styles, 5, style->list_style, out);
		break;
	case COMPUTED_BORDER_COLLAPSE:
		error = computed_name(computed_collapses, 2, style->border_collapse, out);
		break;
	case COMPUTED_FLEX_DIRECTION:
		error = computed_name(computed_flex_directions, 4, style->flex_direction, out);
		break;
	case COMPUTED_FLEX_WRAP:
		error = computed_name(computed_flex_wraps, 3, style->flex_wrap, out);
		break;
	case COMPUTED_FLEX_GROW:
		error = computed_number(style->flex_grow, out);
		break;
	case COMPUTED_FLEX_SHRINK:
		error = computed_number(style->flex_shrink, out);
		break;
	case COMPUTED_FLEX_BASIS:
		error = computed_length(&style->flex_basis, out);
		break;
	case COMPUTED_JUSTIFY_CONTENT:
		error = computed_name(computed_aligns, 9, style->justify_content, out);
		break;
	case COMPUTED_ALIGN_ITEMS:
		error = computed_name(computed_aligns, 9, style->align_items, out);
		break;
	case COMPUTED_ALIGN_SELF:
		error = computed_name(computed_aligns, 9, style->align_self, out);
		break;
	case COMPUTED_ALIGN_CONTENT:
		error = computed_name(computed_aligns, 9, style->align_content, out);
		break;
	case COMPUTED_ORDER:
		error = computed_number(style->order, out);
		break;
	case COMPUTED_ROW_GAP:
		error = computed_length(&style->row_gap, out);
		break;
	case COMPUTED_COLUMN_GAP:
		error = computed_length(&style->column_gap, out);
		break;
	case COMPUTED_OUTLINE_WIDTH:
		error = computed_pixels(style->outline_width, out);
		break;
	case COMPUTED_OUTLINE_STYLE:
		error = computed_name(computed_border_styles, 7, style->outline_style, out);
		break;
	case COMPUTED_OUTLINE_COLOR:
		error = computed_color(style->outline_color, out);
		break;
	case COMPUTED_GRID_TEMPLATE_COLUMNS:
		error = computed_write_tracks(style, out);
		break;
	default:
		/* transform: the engine has none. */
		error = computed_ascii("none", out);
		break;
	}

	/* Reports the writing. */
	return error;
}

/*
 * Writes an inset (top, right, bottom or left: side 0 to 3): a relatively
 * positioned box's auto one is the opposite one's negation in pixels, as
 * the box moves by it; any other is its computed value.
 */
static int
computed_write_offset(
	const struct css_style *style,
	int side,
	struct wb_units *out)
{
	const struct css_length *length;
	const struct css_length *opposite;
	int error;

	/* The side and the one across from it. */
	length = &style->offset[side];
	opposite = &style->offset[(side + 2) % 4];

	/* A relative box's auto side against a side in pixels. */
	if (style->position == CSS_POSITION_RELATIVE &&
	    length->unit == CSS_UNIT_AUTO &&
	    opposite->unit == CSS_UNIT_PX) {
		error = computed_pixels(-opposite->value, out);
		return error;
	}

	/* A relative box with both auto has not moved. */
	if (style->position == CSS_POSITION_RELATIVE &&
	    length->unit == CSS_UNIT_AUTO &&
	    opposite->unit == CSS_UNIT_AUTO) {
		error = computed_pixels(0.0, out);
		return error;
	}

	/* Succeeded: the computed value. */
	error = computed_length(length, out);
	return error;
}

/*
 * Writes font-family: each family as the style names it, a name with a
 * character other than a letter, a digit or a hyphen in double quotes,
 * separated by ", ".
 */
static int
computed_write_font_family(
	const struct css_style *style,
	struct wb_units *out)
{
	const struct vm_string *family;
	size_t index;
	int quoted;
	int error;

	/* Each family. */
	for (index = 0; index < (size_t)style->family_count; index++) {
		family = style->families[index];

		/* The separator before all but the first. */
		if (index > 0) {
			error = computed_ascii(", ", out);
			if (error != 0)
				return error;
		}

		/* The name, in quotes when it needs them. */
		quoted = computed_needs_quotes(family);
		if (quoted) {
			error = computed_write_url_quoted(family, out);
		} else {
			error = vm_string_append_units(family, out);
		}

		/* A failed name. */
		if (error != 0)
			return error;
	}

	/* Succeeded: the families are written. */
	return 0;
}

/*
 * Writes grid-template-columns as the style has the tracks (none, or each
 * track's length, fraction or auto; the used sizes are not kept).
 */
static int
computed_write_tracks(
	const struct css_style *style,
	struct wb_units *out)
{
	const struct css_track *track;
	int index;
	int error;

	/* No tracks is none. */
	if (style->column_count == 0) {
		error = computed_ascii("none", out);
		return error;
	}

	/* Each track, separated by spaces. */
	for (index = 0; index < style->column_count; index++) {
		track = &style->columns[index];

		/* A space before all but the first. */
		if (index > 0) {
			error = computed_ascii(" ", out);
			if (error != 0)
				return error;
		}

		/* By its kind. */
		if (track->kind == CSS_TRACK_FR) {
			error = computed_number(track->fr, out);
			if (error == 0)
				error = computed_ascii("fr", out);
		} else if (track->kind == CSS_TRACK_AUTO) {
			error = computed_ascii("auto", out);
		} else {
			error = computed_length(&track->size, out);
		}

		/* A failed track. */
		if (error != 0)
			return error;
	}

	/* Succeeded: the tracks are written. */
	return 0;
}

/* Writes overflow: one keyword when the two axes agree, otherwise both. */
static int
computed_write_overflow(
	const struct css_style *style,
	struct wb_units *out)
{
	int error;

	/* The horizontal axis's. */
	error = computed_name(computed_overflows, 5, style->overflow_x, out);
	if (error != 0)
		return error;

	/* The same on both is one keyword. */
	if (style->overflow_y == style->overflow_x)
		return 0;

	/* Succeeded: the vertical axis's after a space. */
	error = computed_ascii(" ", out);
	if (error != 0)
		return error;
	error = computed_name(computed_overflows, 5, style->overflow_y, out);
	return error;
}

/* Writes a URL as url("...") (background-image). */
static int
computed_write_url(
	const struct vm_string *url,
	struct wb_units *out)
{
	int error;

	/* The function around the quoted URL. */
	error = computed_ascii("url(", out);
	if (error != 0)
		return error;
	error = computed_write_url_quoted(url, out);
	if (error != 0)
		return error;
	error = computed_ascii(")", out);
	return error;
}

/* Writes a string in double quotes. */
static int
computed_write_url_quoted(
	const struct vm_string *text,
	struct wb_units *out)
{
	int error;

	/* The quotes around the string. */
	error = computed_ascii("\"", out);
	if (error != 0)
		return error;
	error = vm_string_append_units(text, out);
	if (error != 0)
		return error;
	error = computed_ascii("\"", out);
	return error;
}

/* Tells whether a font family's name is written in quotes: it has a character other than a letter, a digit or a hyphen. */
static int
computed_needs_quotes(
	const struct vm_string *family)
{
	size_t index;
	uint16_t unit;

	/* Each character. */
	for (index = 0; index < family->length; index++) {
		unit = vm_string_at(family, index);
		if (unit >= 'a' && unit <= 'z')
			continue;
		if (unit >= 'A' && unit <= 'Z')
			continue;
		if (unit >= '0' && unit <= '9')
			continue;
		if (unit == '-')
			continue;
		return 1;
	}

	/* Every character is plain. */
	return 0;
}

/* Writes a percentage, in calc() with the pixels it has added. */
static int
computed_write_percent(
	const struct css_length *length,
	struct wb_units *out)
{
	const char *sign;
	int error;

	/* A plain percentage. */
	if (length->offset == 0.0f) {
		error = computed_number(length->value, out);
		if (error != 0)
			return error;
		error = computed_ascii("%", out);
		return error;
	}

	/* The sign between the two parts. */
	sign = "% + ";
	if (length->offset < 0.0f)
		sign = "% - ";

	/* Succeeded: calc(P% + Npx). */
	error = computed_ascii("calc(", out);
	if (error != 0)
		return error;
	error = computed_number(length->value, out);
	if (error != 0)
		return error;
	error = computed_ascii(sign, out);
	if (error != 0)
		return error;
	error = computed_pixels(fabsf(length->offset), out);
	if (error != 0)
		return error;
	error = computed_ascii(")", out);
	return error;
}

/* Writes the name of a keyword from a table, or nothing for a value past its end. */
static int
computed_name(
	const char *const *names,
	size_t count,
	int value,
	struct wb_units *out)
{
	int error;

	/* A value the table does not name. */
	if (value < 0 || (size_t)value >= count)
		return 0;

	/* Succeeded: its name. */
	error = computed_ascii(names[value], out);
	return error;
}

/* Writes a computed length: pixels, a percentage (with the pixels calc() added), or a keyword. */
static int
computed_length(
	const struct css_length *length,
	struct wb_units *out)
{
	int error;

	/* Chooses by the unit. */
	switch (length->unit) {
	case CSS_UNIT_PX:
		error = computed_pixels(length->value, out);
		return error;
	case CSS_UNIT_PERCENT:
		/* A percentage, in calc() when pixels are added to it. */
		error = computed_write_percent(length, out);
		return error;
	case CSS_UNIT_AUTO:
		error = computed_ascii("auto", out);
		return error;
	case CSS_UNIT_NONE:
		error = computed_ascii("none", out);
		return error;
	case CSS_UNIT_NORMAL:
		error = computed_ascii("normal", out);
		return error;
	case CSS_UNIT_NUMBER:
		error = computed_number(length->value, out);
		return error;
	case CSS_UNIT_MAX_CONTENT:
		error = computed_ascii("max-content", out);
		return error;
	case CSS_UNIT_MIN_CONTENT:
		error = computed_ascii("min-content", out);
		return error;
	default:
		break;
	}

	/* The last keyword. */
	error = computed_ascii("fit-content", out);
	return error;
}

/* Writes a number of pixels ("12px", "12.5px"). */
static int
computed_pixels(
	double value,
	struct wb_units *out)
{
	int error;

	/* The number and the unit. */
	error = computed_number(value, out);
	if (error != 0)
		return error;
	error = computed_ascii("px", out);
	if (error != 0)
		return error;

	/* Succeeded: the pixels are written. */
	return 0;
}

/*
 * Writes a number as other browsers write the ones of CSS: a whole number
 * without a point, anything else with at most six significant digits and
 * no trailing zeros.
 */
static int
computed_number(
	double value,
	struct wb_units *out)
{
	char text[64];
	double whole;
	double size;

	/* Negative zero, or a number that is not one, is 0. */
	if (value == 0.0 || value != value)
		value = 0.0;

	/* A whole or a large number without a point, any other with six significant digits. */
	whole = floor(value);
	size = fabs(value);
	if (whole == value || size >= 1e6) {
		snprintf(text, sizeof(text), "%.0f", value);
	} else {
		snprintf(text, sizeof(text), "%.6g", value);
	}

	/* Succeeded: the text. */
	return computed_ascii(text, out);
}

/*
 * Writes a color as rgb(r, g, b), or rgba(r, g, b, a) when it is not
 * opaque, the alpha with the fewest decimals that stand for its byte.
 */
static int
computed_color(
	uint32_t color,
	struct wb_units *out)
{
	char text[64];
	unsigned alpha;
	unsigned back;
	double fraction;
	double rounded;
	double scale;
	int digits;

	/* An opaque color. */
	alpha = (color >> 24) & 0xffU;
	if (alpha == 0xffU) {
		snprintf(text, sizeof(text), "rgb(%u, %u, %u)", (unsigned)((color >> 16) & 0xffU), (unsigned)((color >> 8) & 0xffU),
		    (unsigned)(color & 0xffU));
		return computed_ascii(text, out);
	}

	/* The shortest fraction (up to three decimals) whose byte is the alpha's. */
	fraction = alpha / 255.0;
	rounded = fraction;
	for (digits = 1; digits <= 3; digits++) {
		scale = pow(10.0, digits);
		rounded = floor(fraction * scale + 0.5) / scale;
		back = (unsigned)floor(rounded * 255.0 + 0.5);
		if (back == alpha)
			break;
	}

	/* The fraction found, or the one of three decimals. */
	fraction = rounded;

	/* Succeeded: the color with its alpha. */
	snprintf(text, sizeof(text), "rgba(%u, %u, %u, %g)", (unsigned)((color >> 16) & 0xffU), (unsigned)((color >> 8) & 0xffU),
	    (unsigned)(color & 0xffU), fraction);
	return computed_ascii(text, out);
}

/* Appends ASCII characters. */
static int
computed_ascii(
	const char *ascii,
	struct wb_units *out)
{
	uint16_t unit;
	size_t index;
	int error;

	/* Each character. */
	for (index = 0; ascii[index] != '\0'; index++) {
		unit = (uint16_t)(unsigned char)ascii[index];
		error = wb_units_append(out, &unit, 1);
		if (error != 0)
			return error;
	}

	/* Succeeded: the characters are appended. */
	return 0;
}
