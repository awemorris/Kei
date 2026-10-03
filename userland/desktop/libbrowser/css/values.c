/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * CSS values: the property table, the parsing of a declaration's value
 * into one or more longhand declarations, colors and lengths.
 */

#include "css/internal.h"

#include <errno.h>
#include <string.h>

/* The most items a content value keeps. */
#define VALUES_CONTENT_ITEMS	8

/* The widest supported shorthand expands border's four sides into twelve longhands. */
#define VALUES_EXPANSION_MAX	16

/* The shorthands this pass expands (numbered after the longhands). */
enum values_shorthand {
	SHORT_MARGIN = CSS_PROP_COUNT,
	SHORT_PADDING,
	SHORT_BORDER,
	SHORT_BORDER_TOP,
	SHORT_BORDER_RIGHT,
	SHORT_BORDER_BOTTOM,
	SHORT_BORDER_LEFT,
	SHORT_BORDER_WIDTH,
	SHORT_BORDER_STYLE,
	SHORT_BORDER_COLOR,
	SHORT_BACKGROUND,
	SHORT_BACKGROUND_POSITION,
	SHORT_BACKGROUND_SIZE,
	SHORT_FONT,
	SHORT_TEXT_DECORATION,
	SHORT_LIST_STYLE,
	SHORT_MARGIN_INLINE,
	SHORT_MARGIN_BLOCK,
	SHORT_PADDING_INLINE,
	SHORT_PADDING_BLOCK,
	SHORT_INSET,
	SHORT_INSET_INLINE,
	SHORT_INSET_BLOCK,
	SHORT_FLEX,
	SHORT_FLEX_FLOW,
	SHORT_GAP,
	SHORT_BORDER_RADIUS,
	SHORT_RADIUS_TOP_LEFT,
	SHORT_RADIUS_TOP_RIGHT,
	SHORT_RADIUS_BOTTOM_RIGHT,
	SHORT_RADIUS_BOTTOM_LEFT,
	SHORT_OUTLINE,
	SHORT_GRID_COLUMN,
	SHORT_GRID_ROW,
	SHORT_GRID_AREA,
	SHORT_PLACE_ITEMS,
	SHORT_CONTAINER,
	SHORT_BORDER_SPACING
};

/*
 * One property name and its number (a longhand or a shorthand).
 */
struct values_name {
	const char *name;
	int property;
};

/*
 * One keyword of a property and the value it stands for.
 */
struct values_keyword {
	const char *name;
	int value;
};

/*
 * One operand of a calculation while it is read: a plain number, or a sum
 * of lengths.
 */
struct values_term {
	int is_number;
	float number;
	struct css_calc_sum sum;
};

/*
 * A named color and its RGB value.
 */
struct values_color {
	const char *name;
	uint32_t rgb;
};

static int values_keyword(const struct values_keyword *table, const struct css_token *token, int *value);
static int values_unit(const struct css_token *token, int *unit);
static int values_length(const struct css_token *token, int allow_keywords, struct css_value *value);
static int values_single(struct css_parse *parse, int property, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_families(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *value);
static size_t values_components(const struct css_token *tokens, size_t count, size_t *starts, size_t *lengths, size_t max);
static int values_four(struct css_parse *parse, int first, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_border(struct css_parse *parse, int side, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static void values_add(struct css_declaration *out, size_t *made, int property, const struct css_value *value);
static int values_hex_digit(uint16_t unit);
static int values_rgb_function(const struct css_token *tokens, size_t count, uint32_t *color);
static void values_wide_keyword(int property, const struct css_value *value, struct css_declaration *out, size_t *made);
static int values_background(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static size_t values_function_length(const struct css_token *tokens, size_t count);
static int values_url(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_repeat_parts(const struct css_token *const *parts, size_t count, struct css_value *value);
static int values_position_parts(const struct css_token *const *parts, size_t count, struct css_value *x, struct css_value *y);
static int values_position_part(const struct css_token *token, struct css_value *value);
static int values_position_axis(const struct css_token *token);
static int values_size_parts(const struct css_token *const *parts, size_t count, struct css_value *width, struct css_value *height);
static int values_background_position(const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_background_size(const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_takes_length(int property);
static int values_flex(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_flex_flow(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_content(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_pair(struct css_parse *parse, int first, int second, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_calc(struct css_parse *parse, int property, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_calc_arguments(struct wb_arena *arena, const struct css_token *tokens, size_t count, int operation, struct css_calc *calc);
static int values_calc_sum(struct wb_arena *arena, const struct css_token *tokens, size_t count, size_t *index, struct values_term *term);
static int values_calc_product(struct wb_arena *arena, const struct css_token *tokens, size_t count, size_t *index, struct values_term *term);
static int values_calc_value(struct wb_arena *arena, const struct css_token *tokens, size_t count, size_t *index, struct values_term *term);
static int values_calc_unit(const struct css_token *token, struct css_calc_sum *sum);
static void values_calc_scale(struct css_calc_sum *sum, float factor);
static int values_calc_add(struct css_calc_sum *sum, const struct css_calc_sum *other, float sign);
static size_t values_skip_space(const struct css_token *tokens, size_t count, size_t index);
static int values_border_radius(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_corner_radius(struct css_parse *parse, int corner, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_radii(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *radii);
static int values_box_shadow(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_one_shadow(const struct css_token *tokens, size_t count, struct css_declared_shadow *shadow);
static int values_outline(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_clip_path(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_tracks(struct css_parse *parse, const struct css_token *tokens, size_t count, struct css_value *value);
static int values_track_list(const struct css_token *tokens, size_t count, struct css_track_list *list, int depth);
static int values_one_track(const struct css_token *tokens, size_t count, struct css_declared_track *track);
static int values_track_size(const struct css_token *tokens, size_t count, int *kind, struct css_value *size);
static size_t values_split_at(const struct css_token *tokens, size_t count, int type, uint32_t delim);
static int values_grid_line(const struct css_token *tokens, size_t count, struct css_value *value);
static int values_grid_pair(int start, int end, const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);
static int values_grid_area(const struct css_token *tokens, size_t count, struct css_declaration *out, size_t *made);

/* The property names this pass knows. */
static const struct values_name values_names[] = {
	{ "display", CSS_PROP_DISPLAY },
	{ "position", CSS_PROP_POSITION },
	{ "float", CSS_PROP_FLOAT },
	{ "visibility", CSS_PROP_VISIBILITY },
	{ "width", CSS_PROP_WIDTH },
	{ "height", CSS_PROP_HEIGHT },
	{ "min-width", CSS_PROP_MIN_WIDTH },
	{ "max-width", CSS_PROP_MAX_WIDTH },
	{ "min-height", CSS_PROP_MIN_HEIGHT },
	{ "max-height", CSS_PROP_MAX_HEIGHT },
	{ "margin-top", CSS_PROP_MARGIN_TOP },
	{ "margin-right", CSS_PROP_MARGIN_RIGHT },
	{ "margin-bottom", CSS_PROP_MARGIN_BOTTOM },
	{ "margin-left", CSS_PROP_MARGIN_LEFT },
	{ "padding-top", CSS_PROP_PADDING_TOP },
	{ "padding-right", CSS_PROP_PADDING_RIGHT },
	{ "padding-bottom", CSS_PROP_PADDING_BOTTOM },
	{ "padding-left", CSS_PROP_PADDING_LEFT },
	{ "border-top-width", CSS_PROP_BORDER_TOP_WIDTH },
	{ "border-right-width", CSS_PROP_BORDER_RIGHT_WIDTH },
	{ "border-bottom-width", CSS_PROP_BORDER_BOTTOM_WIDTH },
	{ "border-left-width", CSS_PROP_BORDER_LEFT_WIDTH },
	{ "border-top-style", CSS_PROP_BORDER_TOP_STYLE },
	{ "border-right-style", CSS_PROP_BORDER_RIGHT_STYLE },
	{ "border-bottom-style", CSS_PROP_BORDER_BOTTOM_STYLE },
	{ "border-left-style", CSS_PROP_BORDER_LEFT_STYLE },
	{ "border-top-color", CSS_PROP_BORDER_TOP_COLOR },
	{ "border-right-color", CSS_PROP_BORDER_RIGHT_COLOR },
	{ "border-bottom-color", CSS_PROP_BORDER_BOTTOM_COLOR },
	{ "border-left-color", CSS_PROP_BORDER_LEFT_COLOR },
	{ "color", CSS_PROP_COLOR },
	{ "background-color", CSS_PROP_BACKGROUND_COLOR },
	{ "background-image", CSS_PROP_BACKGROUND_IMAGE },
	{ "background-repeat", CSS_PROP_BACKGROUND_REPEAT },
	{ "background-attachment", CSS_PROP_BACKGROUND_ATTACHMENT },
	{ "background-position-x", CSS_PROP_BACKGROUND_POSITION_X },
	{ "background-position-y", CSS_PROP_BACKGROUND_POSITION_Y },
	{ "font-size", CSS_PROP_FONT_SIZE },
	{ "font-weight", CSS_PROP_FONT_WEIGHT },
	{ "font-style", CSS_PROP_FONT_STYLE },
	{ "font-family", CSS_PROP_FONT_FAMILY },
	{ "line-height", CSS_PROP_LINE_HEIGHT },
	{ "text-align", CSS_PROP_TEXT_ALIGN },
	{ "text-indent", CSS_PROP_TEXT_INDENT },
	{ "white-space", CSS_PROP_WHITE_SPACE },
	{ "text-transform", CSS_PROP_TEXT_TRANSFORM },
	{ "cursor", CSS_PROP_CURSOR },
	{ "text-decoration-line", CSS_PROP_TEXT_DECORATION_LINE },
	{ "list-style-type", CSS_PROP_LIST_STYLE_TYPE },
	{ "top", CSS_PROP_TOP },
	{ "right", CSS_PROP_RIGHT },
	{ "bottom", CSS_PROP_BOTTOM },
	{ "left", CSS_PROP_LEFT },
	{ "z-index", CSS_PROP_Z_INDEX },
	{ "clear", CSS_PROP_CLEAR },
	{ "overflow", CSS_PROP_OVERFLOW },
	{ "overflow-x", CSS_PROP_OVERFLOW_X },
	{ "overflow-y", CSS_PROP_OVERFLOW_Y },
	{ "box-sizing", CSS_PROP_BOX_SIZING },
	{ "-webkit-box-sizing", CSS_PROP_BOX_SIZING },
	{ "content", CSS_PROP_CONTENT },
	{ "flex-direction", CSS_PROP_FLEX_DIRECTION },
	{ "-webkit-flex-direction", CSS_PROP_FLEX_DIRECTION },
	{ "flex-wrap", CSS_PROP_FLEX_WRAP },
	{ "-webkit-flex-wrap", CSS_PROP_FLEX_WRAP },
	{ "justify-content", CSS_PROP_JUSTIFY_CONTENT },
	{ "-webkit-justify-content", CSS_PROP_JUSTIFY_CONTENT },
	{ "align-items", CSS_PROP_ALIGN_ITEMS },
	{ "-webkit-align-items", CSS_PROP_ALIGN_ITEMS },
	{ "align-content", CSS_PROP_ALIGN_CONTENT },
	{ "align-self", CSS_PROP_ALIGN_SELF },
	{ "-webkit-align-self", CSS_PROP_ALIGN_SELF },
	{ "row-gap", CSS_PROP_ROW_GAP },
	{ "grid-row-gap", CSS_PROP_ROW_GAP },
	{ "column-gap", CSS_PROP_COLUMN_GAP },
	{ "grid-column-gap", CSS_PROP_COLUMN_GAP },
	{ "flex-grow", CSS_PROP_FLEX_GROW },
	{ "-webkit-flex-grow", CSS_PROP_FLEX_GROW },
	{ "flex-shrink", CSS_PROP_FLEX_SHRINK },
	{ "-webkit-flex-shrink", CSS_PROP_FLEX_SHRINK },
	{ "flex-basis", CSS_PROP_FLEX_BASIS },
	{ "-webkit-flex-basis", CSS_PROP_FLEX_BASIS },
	{ "order", CSS_PROP_ORDER },
	{ "vertical-align", CSS_PROP_VERTICAL_ALIGN },
	{ "opacity", CSS_PROP_OPACITY },
	{ "box-shadow", CSS_PROP_BOX_SHADOW },
	{ "-webkit-box-shadow", CSS_PROP_BOX_SHADOW },
	{ "-moz-box-shadow", CSS_PROP_BOX_SHADOW },
	{ "outline-width", CSS_PROP_OUTLINE_WIDTH },
	{ "outline-style", CSS_PROP_OUTLINE_STYLE },
	{ "outline-color", CSS_PROP_OUTLINE_COLOR },
	{ "outline-offset", CSS_PROP_OUTLINE_OFFSET },
	{ "clip-path", CSS_PROP_CLIP_PATH },
	{ "direction", CSS_PROP_DIRECTION },
	{ "grid-template-columns", CSS_PROP_GRID_TEMPLATE_COLUMNS },
	{ "grid-template-rows", CSS_PROP_GRID_TEMPLATE_ROWS },
	{ "grid-column-start", CSS_PROP_GRID_COLUMN_START },
	{ "grid-column-end", CSS_PROP_GRID_COLUMN_END },
	{ "grid-row-start", CSS_PROP_GRID_ROW_START },
	{ "grid-row-end", CSS_PROP_GRID_ROW_END },
	{ "container-type", CSS_PROP_CONTAINER_TYPE },
	{ "border-collapse", CSS_PROP_BORDER_COLLAPSE },
	{ "-webkit-clip-path", CSS_PROP_CLIP_PATH },
	{ "flex", SHORT_FLEX },
	{ "-webkit-flex", SHORT_FLEX },
	{ "flex-flow", SHORT_FLEX_FLOW },
	{ "gap", SHORT_GAP },
	{ "grid-gap", SHORT_GAP },
	{ "border-radius", SHORT_BORDER_RADIUS },
	{ "-webkit-border-radius", SHORT_BORDER_RADIUS },
	{ "-moz-border-radius", SHORT_BORDER_RADIUS },
	{ "border-top-left-radius", SHORT_RADIUS_TOP_LEFT },
	{ "border-top-right-radius", SHORT_RADIUS_TOP_RIGHT },
	{ "border-bottom-right-radius", SHORT_RADIUS_BOTTOM_RIGHT },
	{ "border-bottom-left-radius", SHORT_RADIUS_BOTTOM_LEFT },
	{ "-webkit-border-top-left-radius", SHORT_RADIUS_TOP_LEFT },
	{ "-webkit-border-top-right-radius", SHORT_RADIUS_TOP_RIGHT },
	{ "-webkit-border-bottom-right-radius", SHORT_RADIUS_BOTTOM_RIGHT },
	{ "-webkit-border-bottom-left-radius", SHORT_RADIUS_BOTTOM_LEFT },
	{ "border-start-start-radius", SHORT_RADIUS_TOP_LEFT },
	{ "border-start-end-radius", SHORT_RADIUS_TOP_RIGHT },
	{ "border-end-end-radius", SHORT_RADIUS_BOTTOM_RIGHT },
	{ "border-end-start-radius", SHORT_RADIUS_BOTTOM_LEFT },
	{ "outline", SHORT_OUTLINE },
	{ "grid-column", SHORT_GRID_COLUMN },
	{ "grid-row", SHORT_GRID_ROW },
	{ "grid-area", SHORT_GRID_AREA },
	{ "place-items", SHORT_PLACE_ITEMS },
	{ "container", SHORT_CONTAINER },
	{ "border-spacing", SHORT_BORDER_SPACING },
	{ "margin", SHORT_MARGIN },
	{ "padding", SHORT_PADDING },
	{ "border", SHORT_BORDER },
	{ "border-top", SHORT_BORDER_TOP },
	{ "border-right", SHORT_BORDER_RIGHT },
	{ "border-bottom", SHORT_BORDER_BOTTOM },
	{ "border-left", SHORT_BORDER_LEFT },
	{ "border-width", SHORT_BORDER_WIDTH },
	{ "border-style", SHORT_BORDER_STYLE },
	{ "border-color", SHORT_BORDER_COLOR },
	{ "background", SHORT_BACKGROUND },
	{ "background-position", SHORT_BACKGROUND_POSITION },
	{ "background-size", SHORT_BACKGROUND_SIZE },
	{ "font", SHORT_FONT },
	{ "text-decoration", SHORT_TEXT_DECORATION },
	{ "list-style", SHORT_LIST_STYLE },

	/* The logical properties, in a horizontal left-to-right writing mode (ws074-p061). */
	{ "margin-inline-start", CSS_PROP_MARGIN_LEFT },
	{ "margin-inline-end", CSS_PROP_MARGIN_RIGHT },
	{ "margin-block-start", CSS_PROP_MARGIN_TOP },
	{ "margin-block-end", CSS_PROP_MARGIN_BOTTOM },
	{ "padding-inline-start", CSS_PROP_PADDING_LEFT },
	{ "padding-inline-end", CSS_PROP_PADDING_RIGHT },
	{ "padding-block-start", CSS_PROP_PADDING_TOP },
	{ "padding-block-end", CSS_PROP_PADDING_BOTTOM },
	{ "inset-inline-start", CSS_PROP_LEFT },
	{ "inset-inline-end", CSS_PROP_RIGHT },
	{ "inset-block-start", CSS_PROP_TOP },
	{ "inset-block-end", CSS_PROP_BOTTOM },
	{ "inline-size", CSS_PROP_WIDTH },
	{ "block-size", CSS_PROP_HEIGHT },
	{ "min-inline-size", CSS_PROP_MIN_WIDTH },
	{ "max-inline-size", CSS_PROP_MAX_WIDTH },
	{ "min-block-size", CSS_PROP_MIN_HEIGHT },
	{ "max-block-size", CSS_PROP_MAX_HEIGHT },
	{ "border-inline-start-width", CSS_PROP_BORDER_LEFT_WIDTH },
	{ "border-inline-end-width", CSS_PROP_BORDER_RIGHT_WIDTH },
	{ "border-block-start-width", CSS_PROP_BORDER_TOP_WIDTH },
	{ "border-block-end-width", CSS_PROP_BORDER_BOTTOM_WIDTH },
	{ "border-inline-start-style", CSS_PROP_BORDER_LEFT_STYLE },
	{ "border-inline-end-style", CSS_PROP_BORDER_RIGHT_STYLE },
	{ "border-block-start-style", CSS_PROP_BORDER_TOP_STYLE },
	{ "border-block-end-style", CSS_PROP_BORDER_BOTTOM_STYLE },
	{ "border-inline-start-color", CSS_PROP_BORDER_LEFT_COLOR },
	{ "border-inline-end-color", CSS_PROP_BORDER_RIGHT_COLOR },
	{ "border-block-start-color", CSS_PROP_BORDER_TOP_COLOR },
	{ "border-block-end-color", CSS_PROP_BORDER_BOTTOM_COLOR },
	{ "border-inline-start", SHORT_BORDER_LEFT },
	{ "border-inline-end", SHORT_BORDER_RIGHT },
	{ "border-block-start", SHORT_BORDER_TOP },
	{ "border-block-end", SHORT_BORDER_BOTTOM },
	{ "margin-inline", SHORT_MARGIN_INLINE },
	{ "margin-block", SHORT_MARGIN_BLOCK },
	{ "padding-inline", SHORT_PADDING_INLINE },
	{ "padding-block", SHORT_PADDING_BLOCK },
	{ "inset", SHORT_INSET },
	{ "inset-inline", SHORT_INSET_INLINE },
	{ "inset-block", SHORT_INSET_BLOCK },
	{ NULL, 0 }
};

/* The keywords of display. */
static const struct values_keyword values_display[] = {
	{ "inline", CSS_DISPLAY_INLINE },
	{ "block", CSS_DISPLAY_BLOCK },
	{ "inline-block", CSS_DISPLAY_INLINE_BLOCK },
	{ "list-item", CSS_DISPLAY_LIST_ITEM },
	{ "none", CSS_DISPLAY_NONE },
	{ "table", CSS_DISPLAY_TABLE },
	{ "inline-table", CSS_DISPLAY_INLINE_TABLE },
	{ "table-row", CSS_DISPLAY_TABLE_ROW },
	{ "table-cell", CSS_DISPLAY_TABLE_CELL },
	{ "table-row-group", CSS_DISPLAY_TABLE_ROW_GROUP },
	{ "table-header-group", CSS_DISPLAY_TABLE_ROW_GROUP },
	{ "table-footer-group", CSS_DISPLAY_TABLE_ROW_GROUP },
	{ "table-caption", CSS_DISPLAY_TABLE_CAPTION },
	{ "table-column", CSS_DISPLAY_TABLE_COLUMN },
	{ "table-column-group", CSS_DISPLAY_TABLE_COLUMN },
	{ "flex", CSS_DISPLAY_FLEX },
	{ "inline-flex", CSS_DISPLAY_FLEX },
	{ "-webkit-flex", CSS_DISPLAY_FLEX },
	{ "-webkit-inline-flex", CSS_DISPLAY_FLEX },
	{ "-webkit-box", CSS_DISPLAY_BLOCK },
	{ "-webkit-inline-box", CSS_DISPLAY_INLINE_BLOCK },
	{ "grid", CSS_DISPLAY_GRID },
	{ "inline-grid", CSS_DISPLAY_GRID },
	{ "flow-root", CSS_DISPLAY_BLOCK },
	{ "contents", CSS_DISPLAY_CONTENTS },
	{ NULL, 0 }
};

/* The keywords of border-collapse (ws074-p037). */
static const struct values_keyword values_border_collapse[] = {
	{ "separate", 0 },
	{ "collapse", 1 },
	{ NULL, 0 }
};

/* The keywords of container-type (ws074-p075). */
static const struct values_keyword values_container_type[] = {
	{ "normal", CSS_CONTAINER_NORMAL },
	{ "inline-size", CSS_CONTAINER_INLINE_SIZE },
	{ "size", CSS_CONTAINER_SIZE },
	{ NULL, 0 }
};

/* The keywords of direction (ws074-p073). */
static const struct values_keyword values_direction[] = {
	{ "ltr", CSS_DIRECTION_LTR },
	{ "rtl", CSS_DIRECTION_RTL },
	{ NULL, 0 }
};

/* The keywords of vertical-align (a length or percentage is the other form). */
static const struct values_keyword values_vertical_align[] = {
	{ "baseline", CSS_VALIGN_BASELINE },
	{ "top", CSS_VALIGN_TOP },
	{ "middle", CSS_VALIGN_MIDDLE },
	{ "bottom", CSS_VALIGN_BOTTOM },
	{ "text-top", CSS_VALIGN_TEXT_TOP },
	{ "text-bottom", CSS_VALIGN_TEXT_BOTTOM },
	{ "sub", CSS_VALIGN_SUB },
	{ "super", CSS_VALIGN_SUPER },
	{ NULL, 0 }
};

/* The keywords of flex-direction. */
static const struct values_keyword values_flex_direction[] = {
	{ "row", CSS_FLEX_ROW },
	{ "row-reverse", CSS_FLEX_ROW_REVERSE },
	{ "column", CSS_FLEX_COLUMN },
	{ "column-reverse", CSS_FLEX_COLUMN_REVERSE },
	{ NULL, 0 }
};

/* The keywords of flex-wrap. */
static const struct values_keyword values_flex_wrap[] = {
	{ "nowrap", CSS_FLEX_NOWRAP },
	{ "wrap", CSS_FLEX_WRAP },
	{ "wrap-reverse", CSS_FLEX_WRAP_REVERSE },
	{ NULL, 0 }
};

/* The keywords of the alignment properties (the left and right of justify-content are its start and end here). */
static const struct values_keyword values_align[] = {
	{ "auto", CSS_ALIGN_AUTO },
	{ "normal", CSS_ALIGN_STRETCH },
	{ "stretch", CSS_ALIGN_STRETCH },
	{ "flex-start", CSS_ALIGN_START },
	{ "start", CSS_ALIGN_START },
	{ "self-start", CSS_ALIGN_START },
	{ "left", CSS_ALIGN_START },
	{ "flex-end", CSS_ALIGN_END },
	{ "end", CSS_ALIGN_END },
	{ "self-end", CSS_ALIGN_END },
	{ "right", CSS_ALIGN_END },
	{ "center", CSS_ALIGN_CENTER },
	{ "baseline", CSS_ALIGN_BASELINE },
	{ "first", CSS_ALIGN_BASELINE },
	{ "space-between", CSS_ALIGN_SPACE_BETWEEN },
	{ "space-around", CSS_ALIGN_SPACE_AROUND },
	{ "space-evenly", CSS_ALIGN_SPACE_EVENLY },
	{ NULL, 0 }
};

/* The keywords of position. */
static const struct values_keyword values_position[] = {
	{ "static", CSS_POSITION_STATIC },
	{ "relative", CSS_POSITION_RELATIVE },
	{ "absolute", CSS_POSITION_ABSOLUTE },
	{ "fixed", CSS_POSITION_FIXED },
	{ "sticky", CSS_POSITION_STICKY },
	{ NULL, 0 }
};

/* The keywords of float. */
static const struct values_keyword values_float[] = {
	{ "none", CSS_FLOAT_NONE },
	{ "left", CSS_FLOAT_LEFT },
	{ "right", CSS_FLOAT_RIGHT },
	{ "inline-start", CSS_FLOAT_LEFT },
	{ "inline-end", CSS_FLOAT_RIGHT },
	{ NULL, 0 }
};

/* The keywords of overflow (one value for both axes in this pass). */
static const struct values_keyword values_overflow[] = {
	{ "visible", CSS_OVERFLOW_VISIBLE },
	{ "hidden", CSS_OVERFLOW_HIDDEN },
	{ "clip", CSS_OVERFLOW_CLIP },
	{ "scroll", CSS_OVERFLOW_SCROLL },
	{ "auto", CSS_OVERFLOW_AUTO },
	{ "overlay", CSS_OVERFLOW_AUTO },
	{ NULL, 0 }
};

/* The keywords of clear. */
static const struct values_keyword values_clear[] = {
	{ "none", CSS_FLOAT_NONE },
	{ "left", CSS_FLOAT_LEFT },
	{ "right", CSS_FLOAT_RIGHT },
	{ "both", CSS_CLEAR_BOTH },
	{ "inline-start", CSS_FLOAT_LEFT },
	{ "inline-end", CSS_FLOAT_RIGHT },
	{ NULL, 0 }
};

/* The keywords of visibility. */
static const struct values_keyword values_visibility[] = {
	{ "visible", 0 },
	{ "hidden", 1 },
	{ "collapse", 1 },
	{ NULL, 0 }
};

/* The keywords of border-style. */
static const struct values_keyword values_border_style[] = {
	{ "none", CSS_BORDER_NONE },
	{ "hidden", CSS_BORDER_NONE },
	{ "solid", CSS_BORDER_SOLID },
	{ "dashed", CSS_BORDER_DASHED },
	{ "dotted", CSS_BORDER_DOTTED },
	{ "double", CSS_BORDER_DOUBLE },
	{ "groove", CSS_BORDER_SOLID },
	{ "ridge", CSS_BORDER_SOLID },
	{ "inset", CSS_BORDER_INSET },
	{ "outset", CSS_BORDER_OUTSET },
	{ NULL, 0 }
};

/* The keywords of box-sizing. */
static const struct values_keyword values_box_sizing[] = {
	{ "content-box", CSS_BOX_SIZING_CONTENT },
	{ "border-box", CSS_BOX_SIZING_BORDER },
	{ NULL, 0 }
};

/* The keywords of border widths, in pixels. */
static const struct values_keyword values_border_width[] = {
	{ "thin", 1 },
	{ "medium", 3 },
	{ "thick", 5 },
	{ NULL, 0 }
};

/* The keywords of font-weight. */
static const struct values_keyword values_font_weight[] = {
	{ "normal", 400 },
	{ "bold", 700 },
	{ "bolder", 700 },
	{ "lighter", 300 },
	{ NULL, 0 }
};

/* The keywords of font-style. */
static const struct values_keyword values_font_style[] = {
	{ "normal", 0 },
	{ "italic", 1 },
	{ "oblique", 1 },
	{ NULL, 0 }
};

/* The keywords of font-size, in pixels at the default size of 16. */
static const struct values_keyword values_font_size[] = {
	{ "xx-small", 9 },
	{ "x-small", 10 },
	{ "small", 13 },
	{ "medium", 16 },
	{ "large", 18 },
	{ "x-large", 24 },
	{ "xx-large", 32 },
	{ "xxx-large", 48 },
	{ NULL, 0 }
};

/* The keywords of text-align. */
static const struct values_keyword values_text_align[] = {
	{ "start", CSS_TEXT_ALIGN_START },
	{ "left", CSS_TEXT_ALIGN_LEFT },
	{ "right", CSS_TEXT_ALIGN_RIGHT },
	{ "center", CSS_TEXT_ALIGN_CENTER },
	{ "justify", CSS_TEXT_ALIGN_JUSTIFY },
	{ "end", CSS_TEXT_ALIGN_END },
	{ "-webkit-center", CSS_TEXT_ALIGN_CENTER },
	{ NULL, 0 }
};

/* The supported predefined cursor keywords, shared by ordinary and inline declarations. */
static const struct values_keyword values_cursor[] = {
	{ "auto", CSS_CURSOR_AUTO },
	{ "default", CSS_CURSOR_DEFAULT },
	{ "none", CSS_CURSOR_NONE },
	{ "context-menu", CSS_CURSOR_CONTEXT_MENU },
	{ "help", CSS_CURSOR_HELP },
	{ "pointer", CSS_CURSOR_POINTER },
	{ "progress", CSS_CURSOR_PROGRESS },
	{ "wait", CSS_CURSOR_WAIT },
	{ "cell", CSS_CURSOR_CELL },
	{ "crosshair", CSS_CURSOR_CROSSHAIR },
	{ "text", CSS_CURSOR_TEXT },
	{ "vertical-text", CSS_CURSOR_VERTICAL_TEXT },
	{ "alias", CSS_CURSOR_ALIAS },
	{ "copy", CSS_CURSOR_COPY },
	{ "move", CSS_CURSOR_MOVE },
	{ "no-drop", CSS_CURSOR_NO_DROP },
	{ "not-allowed", CSS_CURSOR_NOT_ALLOWED },
	{ "e-resize", CSS_CURSOR_E_RESIZE },
	{ "n-resize", CSS_CURSOR_N_RESIZE },
	{ "ne-resize", CSS_CURSOR_NE_RESIZE },
	{ "nw-resize", CSS_CURSOR_NW_RESIZE },
	{ "s-resize", CSS_CURSOR_S_RESIZE },
	{ "se-resize", CSS_CURSOR_SE_RESIZE },
	{ "sw-resize", CSS_CURSOR_SW_RESIZE },
	{ "w-resize", CSS_CURSOR_W_RESIZE },
	{ "ew-resize", CSS_CURSOR_EW_RESIZE },
	{ "ns-resize", CSS_CURSOR_NS_RESIZE },
	{ "nesw-resize", CSS_CURSOR_NESW_RESIZE },
	{ "nwse-resize", CSS_CURSOR_NWSE_RESIZE },
	{ "col-resize", CSS_CURSOR_COL_RESIZE },
	{ "row-resize", CSS_CURSOR_ROW_RESIZE },
	{ "all-scroll", CSS_CURSOR_ALL_SCROLL },
	{ "grab", CSS_CURSOR_GRAB },
	{ "grabbing", CSS_CURSOR_GRABBING },
	{ "zoom-in", CSS_CURSOR_ZOOM_IN },
	{ "zoom-out", CSS_CURSOR_ZOOM_OUT },
	{ NULL, 0 }
};

/* The native computed text-transform keywords accepted by the CSS2 property. */
static const struct values_keyword values_text_transform[] = {
	{ "none", CSS_TEXT_TRANSFORM_NONE },
	{ "capitalize", CSS_TEXT_TRANSFORM_CAPITALIZE },
	{ "uppercase", CSS_TEXT_TRANSFORM_UPPERCASE },
	{ "lowercase", CSS_TEXT_TRANSFORM_LOWERCASE },
	{ NULL, 0 }
};

/* The keywords of white-space. */
static const struct values_keyword values_white_space[] = {
	{ "normal", CSS_WHITE_SPACE_NORMAL },
	{ "pre", CSS_WHITE_SPACE_PRE },
	{ "nowrap", CSS_WHITE_SPACE_NOWRAP },
	{ "pre-wrap", CSS_WHITE_SPACE_PRE_WRAP },
	{ "pre-line", CSS_WHITE_SPACE_PRE_LINE },
	{ NULL, 0 }
};

/* The keywords of text-decoration-line (only underline is drawn in this pass). */
static const struct values_keyword values_decoration[] = {
	{ "none", 0 },
	{ "underline", 1 },
	{ "overline", 0 },
	{ "line-through", 0 },
	{ NULL, 0 }
};

/* The keywords of list-style-type. */
static const struct values_keyword values_list_style[] = {
	{ "disc", CSS_LIST_DISC },
	{ "circle", CSS_LIST_CIRCLE },
	{ "square", CSS_LIST_SQUARE },
	{ "decimal", CSS_LIST_DECIMAL },
	{ "none", CSS_LIST_NONE },
	{ NULL, 0 }
};

/* The CSS-wide keywords. */
static const struct values_keyword values_wide[] = {
	{ "inherit", CSS_VALUE_INHERIT },
	{ "initial", CSS_VALUE_INITIAL },
	{ "unset", CSS_VALUE_UNSET },
	{ "revert", CSS_VALUE_UNSET },
	{ "revert-layer", CSS_VALUE_UNSET },
	{ NULL, 0 }
};

/* The values of background-repeat's keywords (space and round are drawn as repeat). */
static const struct values_keyword values_repeat[] = {
	{ "repeat", CSS_REPEAT_BOTH },
	{ "repeat-x", CSS_REPEAT_X },
	{ "repeat-y", CSS_REPEAT_Y },
	{ "no-repeat", CSS_REPEAT_NONE },
	{ "space", CSS_REPEAT_BOTH },
	{ "round", CSS_REPEAT_BOTH },
	{ NULL, 0 }
};

/* The values of background-attachment (local currently scrolls with its box). */
static const struct values_keyword values_attachment[] = {
	{ "scroll", CSS_BACKGROUND_SCROLL },
	{ "fixed", CSS_BACKGROUND_FIXED },
	{ "local", CSS_BACKGROUND_SCROLL },
	{ NULL, 0 }
};

/* The keywords of background-position, as percentages of the room. */
static const struct values_keyword values_position_keywords[] = {
	{ "left", 0 },
	{ "center", 50 },
	{ "right", 100 },
	{ "top", 0 },
	{ "bottom", 100 },
	{ NULL, 0 }
};

/* The keywords a length property may take instead of a length. */
static const struct values_keyword values_length_keywords[] = {
	{ "auto", CSS_UNIT_AUTO },
	{ "none", CSS_UNIT_NONE },
	{ "normal", CSS_UNIT_NORMAL },
	{ "max-content", CSS_UNIT_MAX_CONTENT },
	{ "-webkit-max-content", CSS_UNIT_MAX_CONTENT },
	{ "-moz-max-content", CSS_UNIT_MAX_CONTENT },
	{ "min-content", CSS_UNIT_MIN_CONTENT },
	{ "-webkit-min-content", CSS_UNIT_MIN_CONTENT },
	{ "-moz-min-content", CSS_UNIT_MIN_CONTENT },
	{ "fit-content", CSS_UNIT_FIT_CONTENT },
	{ "-webkit-fit-content", CSS_UNIT_FIT_CONTENT },
	{ "-moz-fit-content", CSS_UNIT_FIT_CONTENT },
	{ NULL, 0 }
};

/* The units of lengths. */
static const struct values_keyword values_units[] = {
	{ "px", CSS_DUNIT_PX },
	{ "em", CSS_DUNIT_EM },
	{ "rem", CSS_DUNIT_REM },
	{ "ex", CSS_DUNIT_EX },
	{ "ch", CSS_DUNIT_EX },
	{ "pt", CSS_DUNIT_PT },
	{ "pc", CSS_DUNIT_PC },
	{ "in", CSS_DUNIT_IN },
	{ "cm", CSS_DUNIT_CM },
	{ "mm", CSS_DUNIT_MM },
	{ "vw", CSS_DUNIT_VW },
	{ "vh", CSS_DUNIT_VH },
	{ "vmin", CSS_DUNIT_VMIN },
	{ "vmax", CSS_DUNIT_VMAX },
	{ "svw", CSS_DUNIT_VW },
	{ "lvw", CSS_DUNIT_VW },
	{ "dvw", CSS_DUNIT_VW },
	{ "svh", CSS_DUNIT_VH },
	{ "lvh", CSS_DUNIT_VH },
	{ "dvh", CSS_DUNIT_VH },
	{ "cqw", CSS_DUNIT_CQW },
	{ "cqi", CSS_DUNIT_CQW },
	{ "cqmin", CSS_DUNIT_CQW },
	{ "cqmax", CSS_DUNIT_CQW },
	{ "cqh", CSS_DUNIT_CQH },
	{ "cqb", CSS_DUNIT_CQH },
	{ NULL, 0 }
};

/* The relative font sizes, as percentages of the parent's. */
static const struct values_keyword values_relative_size[] = {
	{ "smaller", 83 },
	{ "larger", 120 },
	{ NULL, 0 }
};

/* The words of the font shorthand this pass reads past. */
static const struct values_keyword values_font_ignored[] = {
	{ "normal", 0 },
	{ "small-caps", 0 },
	{ NULL, 0 }
};

/* The color keywords that are not named colors (CSS_CURRENT_COLOR stands for currentcolor). */
static const struct values_keyword values_color_keywords[] = {
	{ "transparent", 0 },
	{ "currentcolor", (int)CSS_CURRENT_COLOR },
	{ NULL, 0 }
};

/* The named colors of CSS Color 4. */
static const struct values_color values_colors[] = {
	{ "aliceblue", 0xf0f8ffU },
	{ "antiquewhite", 0xfaebd7U },
	{ "aqua", 0x00ffffU },
	{ "aquamarine", 0x7fffd4U },
	{ "azure", 0xf0ffffU },
	{ "beige", 0xf5f5dcU },
	{ "bisque", 0xffe4c4U },
	{ "black", 0x000000U },
	{ "blanchedalmond", 0xffebcdU },
	{ "blue", 0x0000ffU },
	{ "blueviolet", 0x8a2be2U },
	{ "brown", 0xa52a2aU },
	{ "burlywood", 0xdeb887U },
	{ "cadetblue", 0x5f9ea0U },
	{ "chartreuse", 0x7fff00U },
	{ "chocolate", 0xd2691eU },
	{ "coral", 0xff7f50U },
	{ "cornflowerblue", 0x6495edU },
	{ "cornsilk", 0xfff8dcU },
	{ "crimson", 0xdc143cU },
	{ "cyan", 0x00ffffU },
	{ "darkblue", 0x00008bU },
	{ "darkcyan", 0x008b8bU },
	{ "darkgoldenrod", 0xb8860bU },
	{ "darkgray", 0xa9a9a9U },
	{ "darkgreen", 0x006400U },
	{ "darkgrey", 0xa9a9a9U },
	{ "darkkhaki", 0xbdb76bU },
	{ "darkmagenta", 0x8b008bU },
	{ "darkolivegreen", 0x556b2fU },
	{ "darkorange", 0xff8c00U },
	{ "darkorchid", 0x9932ccU },
	{ "darkred", 0x8b0000U },
	{ "darksalmon", 0xe9967aU },
	{ "darkseagreen", 0x8fbc8fU },
	{ "darkslateblue", 0x483d8bU },
	{ "darkslategray", 0x2f4f4fU },
	{ "darkslategrey", 0x2f4f4fU },
	{ "darkturquoise", 0x00ced1U },
	{ "darkviolet", 0x9400d3U },
	{ "deeppink", 0xff1493U },
	{ "deepskyblue", 0x00bfffU },
	{ "dimgray", 0x696969U },
	{ "dimgrey", 0x696969U },
	{ "dodgerblue", 0x1e90ffU },
	{ "firebrick", 0xb22222U },
	{ "floralwhite", 0xfffaf0U },
	{ "forestgreen", 0x228b22U },
	{ "fuchsia", 0xff00ffU },
	{ "gainsboro", 0xdcdcdcU },
	{ "ghostwhite", 0xf8f8ffU },
	{ "gold", 0xffd700U },
	{ "goldenrod", 0xdaa520U },
	{ "gray", 0x808080U },
	{ "green", 0x008000U },
	{ "greenyellow", 0xadff2fU },
	{ "grey", 0x808080U },
	{ "honeydew", 0xf0fff0U },
	{ "hotpink", 0xff69b4U },
	{ "indianred", 0xcd5c5cU },
	{ "indigo", 0x4b0082U },
	{ "ivory", 0xfffff0U },
	{ "khaki", 0xf0e68cU },
	{ "lavender", 0xe6e6faU },
	{ "lavenderblush", 0xfff0f5U },
	{ "lawngreen", 0x7cfc00U },
	{ "lemonchiffon", 0xfffacdU },
	{ "lightblue", 0xadd8e6U },
	{ "lightcoral", 0xf08080U },
	{ "lightcyan", 0xe0ffffU },
	{ "lightgoldenrodyellow", 0xfafad2U },
	{ "lightgray", 0xd3d3d3U },
	{ "lightgreen", 0x90ee90U },
	{ "lightgrey", 0xd3d3d3U },
	{ "lightpink", 0xffb6c1U },
	{ "lightsalmon", 0xffa07aU },
	{ "lightseagreen", 0x20b2aaU },
	{ "lightskyblue", 0x87cefaU },
	{ "lightslategray", 0x778899U },
	{ "lightslategrey", 0x778899U },
	{ "lightsteelblue", 0xb0c4deU },
	{ "lightyellow", 0xffffe0U },
	{ "lime", 0x00ff00U },
	{ "limegreen", 0x32cd32U },
	{ "linen", 0xfaf0e6U },
	{ "magenta", 0xff00ffU },
	{ "maroon", 0x800000U },
	{ "mediumaquamarine", 0x66cdaaU },
	{ "mediumblue", 0x0000cdU },
	{ "mediumorchid", 0xba55d3U },
	{ "mediumpurple", 0x9370dbU },
	{ "mediumseagreen", 0x3cb371U },
	{ "mediumslateblue", 0x7b68eeU },
	{ "mediumspringgreen", 0x00fa9aU },
	{ "mediumturquoise", 0x48d1ccU },
	{ "mediumvioletred", 0xc71585U },
	{ "midnightblue", 0x191970U },
	{ "mintcream", 0xf5fffaU },
	{ "mistyrose", 0xffe4e1U },
	{ "moccasin", 0xffe4b5U },
	{ "navajowhite", 0xffdeadU },
	{ "navy", 0x000080U },
	{ "oldlace", 0xfdf5e6U },
	{ "olive", 0x808000U },
	{ "olivedrab", 0x6b8e23U },
	{ "orange", 0xffa500U },
	{ "orangered", 0xff4500U },
	{ "orchid", 0xda70d6U },
	{ "palegoldenrod", 0xeee8aaU },
	{ "palegreen", 0x98fb98U },
	{ "paleturquoise", 0xafeeeeU },
	{ "palevioletred", 0xdb7093U },
	{ "papayawhip", 0xffefd5U },
	{ "peachpuff", 0xffdab9U },
	{ "peru", 0xcd853fU },
	{ "pink", 0xffc0cbU },
	{ "plum", 0xdda0ddU },
	{ "powderblue", 0xb0e0e6U },
	{ "purple", 0x800080U },
	{ "rebeccapurple", 0x663399U },
	{ "red", 0xff0000U },
	{ "rosybrown", 0xbc8f8fU },
	{ "royalblue", 0x4169e1U },
	{ "saddlebrown", 0x8b4513U },
	{ "salmon", 0xfa8072U },
	{ "sandybrown", 0xf4a460U },
	{ "seagreen", 0x2e8b57U },
	{ "seashell", 0xfff5eeU },
	{ "sienna", 0xa0522dU },
	{ "silver", 0xc0c0c0U },
	{ "skyblue", 0x87ceebU },
	{ "slateblue", 0x6a5acdU },
	{ "slategray", 0x708090U },
	{ "slategrey", 0x708090U },
	{ "snow", 0xfffafaU },
	{ "springgreen", 0x00ff7fU },
	{ "steelblue", 0x4682b4U },
	{ "tan", 0xd2b48cU },
	{ "teal", 0x008080U },
	{ "thistle", 0xd8bfd8U },
	{ "tomato", 0xff6347U },
	{ "turquoise", 0x40e0d0U },
	{ "violet", 0xee82eeU },
	{ "wheat", 0xf5deb3U },
	{ "white", 0xffffffU },
	{ "whitesmoke", 0xf5f5f5U },
	{ "yellow", 0xffff00U },
	{ "yellowgreen", 0x9acd32U },
	{ NULL, 0 }
};

/*
 * Reports the name of the index-th property this pass knows (a longhand
 * or a shorthand), or NULL past the last one: the names the inline style
 * of scripts has members for (ws074-p031).
 */
const char *
css_property_name(
	size_t index)
{
	size_t at;

	/* Counts up to the index, stopping at the table's end. */
	for (at = 0; at < index; at++) {
		if (values_names[at].name == NULL)
			return NULL;
	}

	/* Succeeded: the name, or NULL at the end. */
	return values_names[index].name;
}

/*
 * Finds a property's number by its name, or -1 for an unknown property.
 */
int
css_property_lookup(
	const struct css_token *name)
{
	size_t index;
	int same;

	/* Compares with each known name. */
	for (index = 0; values_names[index].name != NULL; index++) {
		same = css_ident_equal(name, values_names[index].name);
		if (same)
			return values_names[index].property;
	}

	/* The property is not one this pass knows. */
	return -1;
}

/*
 * Parses a declaration's value into longhand declarations.
 *
 * Returns EINVAL for an unknown property or an invalid value (the
 * declaration is then dropped) and ENOMEM when memory runs out.
 */
int
css_parse_value(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	const struct css_token *name,
	struct css_declaration *out,
	size_t *out_count,
	size_t out_capacity)
{
	struct css_declaration expanded[VALUES_EXPANSION_MAX];
	size_t made;
	int property;
	int error;

	/* Finds the property. */
	*out_count = 0;
	property = css_property_lookup(name);
	if (property < 0)
		return EINVAL;

	/* Parse into bounded scratch before publishing declarations to a caller's smaller buffer. */
	made = 0;
	error = css_parse_property(parse, property, tokens, count, expanded, &made);
	if (error != 0)
		return error;
	if (made > out_capacity)
		return EOVERFLOW;
	if (made != 0)
		memcpy(out, expanded, made * sizeof(*out));
	*out_count = made;

	/* Succeeded: the longhands are declared. */
	return 0;
}

/*
 * Parses a value as a property found before (a longhand or a shorthand)
 * into longhand declarations; the same errors as css_parse_value.
 */
int
css_parse_property(
	struct css_parse *parse,
	int property,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *out_count)
{
	struct css_value value;
	size_t starts[3];
	size_t lengths[3];
	size_t components;
	int error;
	int first;
	int wide;

	/* Nothing is no value. */
	*out_count = 0;
	while (count > 0 && tokens[0].type == CSS_TOKEN_WHITESPACE) {
		tokens++;
		count--;
	}
	while (count > 0 && tokens[count - 1U].type == CSS_TOKEN_WHITESPACE)
		count--;
	if (count == 0)
		return EINVAL;

	/* The CSS-wide keywords apply to every longhand a shorthand stands for. */
	memset(&value, 0, sizeof(value));
	wide = values_keyword(values_wide, &tokens[0], &value.kind);
	if (count == 1 && wide) {
		values_wide_keyword(property, &value, out, out_count);
		return 0;
	}

	/* The shorthands. */
	switch (property) {
	case SHORT_MARGIN:
		return values_four(parse, CSS_PROP_MARGIN_TOP, tokens, count, out, out_count);
	case SHORT_PADDING:
		return values_four(parse, CSS_PROP_PADDING_TOP, tokens, count, out, out_count);
	case SHORT_BORDER_WIDTH:
		return values_four(parse, CSS_PROP_BORDER_TOP_WIDTH, tokens, count, out, out_count);
	case SHORT_BORDER_STYLE:
		return values_four(parse, CSS_PROP_BORDER_TOP_STYLE, tokens, count, out, out_count);
	case SHORT_BORDER_COLOR:
		return values_four(parse, CSS_PROP_BORDER_TOP_COLOR, tokens, count, out, out_count);
	case SHORT_BORDER:
		return values_border(parse, -1, tokens, count, out, out_count);
	case SHORT_BORDER_TOP:
		return values_border(parse, CSS_TOP, tokens, count, out, out_count);
	case SHORT_BORDER_RIGHT:
		return values_border(parse, CSS_RIGHT, tokens, count, out, out_count);
	case SHORT_BORDER_BOTTOM:
		return values_border(parse, CSS_BOTTOM, tokens, count, out, out_count);
	case SHORT_BORDER_LEFT:
		return values_border(parse, CSS_LEFT, tokens, count, out, out_count);
	case SHORT_BACKGROUND:
		return values_background(parse, tokens, count, out, out_count);
	case SHORT_BACKGROUND_POSITION:
		return values_background_position(tokens, count, out, out_count);
	case SHORT_BACKGROUND_SIZE:
		return values_background_size(tokens, count, out, out_count);
	case SHORT_TEXT_DECORATION:
		return css_parse_value_as(parse, CSS_PROP_TEXT_DECORATION_LINE, tokens, 1, out, out_count);
	case SHORT_LIST_STYLE:
		for (first = 0; first < (int)count; first++) {
			error = values_single(parse, CSS_PROP_LIST_STYLE_TYPE, &tokens[first], 1, &value);
			if (error == 0) {
				values_add(out, out_count, CSS_PROP_LIST_STYLE_TYPE, &value);
				return 0;
			}
		}

		/* A list style without a type is not used in this pass. */
		return EINVAL;
	case SHORT_FONT:
		return css_parse_font(parse, tokens, count, out, out_count);
	case SHORT_MARGIN_INLINE:
		return values_pair(parse, CSS_PROP_MARGIN_LEFT, CSS_PROP_MARGIN_RIGHT, tokens, count, out, out_count);
	case SHORT_MARGIN_BLOCK:
		return values_pair(parse, CSS_PROP_MARGIN_TOP, CSS_PROP_MARGIN_BOTTOM, tokens, count, out, out_count);
	case SHORT_PADDING_INLINE:
		return values_pair(parse, CSS_PROP_PADDING_LEFT, CSS_PROP_PADDING_RIGHT, tokens, count, out, out_count);
	case SHORT_PADDING_BLOCK:
		return values_pair(parse, CSS_PROP_PADDING_TOP, CSS_PROP_PADDING_BOTTOM, tokens, count, out, out_count);
	case SHORT_INSET:
		return values_four(parse, CSS_PROP_TOP, tokens, count, out, out_count);
	case SHORT_INSET_INLINE:
		return values_pair(parse, CSS_PROP_LEFT, CSS_PROP_RIGHT, tokens, count, out, out_count);
	case SHORT_INSET_BLOCK:
		return values_pair(parse, CSS_PROP_TOP, CSS_PROP_BOTTOM, tokens, count, out, out_count);
	case SHORT_FLEX:
		return values_flex(parse, tokens, count, out, out_count);
	case SHORT_FLEX_FLOW:
		return values_flex_flow(parse, tokens, count, out, out_count);
	case SHORT_GAP:
		return values_pair(parse, CSS_PROP_ROW_GAP, CSS_PROP_COLUMN_GAP, tokens, count, out, out_count);
	case SHORT_BORDER_RADIUS:
		return values_border_radius(parse, tokens, count, out, out_count);
	case SHORT_RADIUS_TOP_LEFT:
		return values_corner_radius(parse, CSS_TOP_LEFT, tokens, count, out, out_count);
	case SHORT_RADIUS_TOP_RIGHT:
		return values_corner_radius(parse, CSS_TOP_RIGHT, tokens, count, out, out_count);
	case SHORT_RADIUS_BOTTOM_RIGHT:
		return values_corner_radius(parse, CSS_BOTTOM_RIGHT, tokens, count, out, out_count);
	case SHORT_RADIUS_BOTTOM_LEFT:
		return values_corner_radius(parse, CSS_BOTTOM_LEFT, tokens, count, out, out_count);
	case SHORT_OUTLINE:
		return values_outline(parse, tokens, count, out, out_count);
	case SHORT_GRID_COLUMN:
		return values_grid_pair(CSS_PROP_GRID_COLUMN_START, CSS_PROP_GRID_COLUMN_END, tokens, count, out, out_count);
	case SHORT_GRID_ROW:
		return values_grid_pair(CSS_PROP_GRID_ROW_START, CSS_PROP_GRID_ROW_END, tokens, count, out, out_count);
	case SHORT_GRID_AREA:
		return values_grid_area(tokens, count, out, out_count);
	case SHORT_BORDER_SPACING:
		/* One length for both directions, or across then down. */
		return values_pair(parse, CSS_PROP_BORDER_SPACING_X, CSS_PROP_BORDER_SPACING_Y, tokens, count, out, out_count);
	case SHORT_CONTAINER:
		/* container: a name, then a slash and the type (the name is not kept in this pass). */
		first = (int)values_split_at(tokens, count, CSS_TOKEN_DELIM, '/');
		if ((size_t)first < count) {
			first = (int)values_skip_space(tokens, count, (size_t)first + 1U);
			return css_parse_value_as(parse, CSS_PROP_CONTAINER_TYPE, tokens + first, count - (size_t)first, out, out_count);
		}

		/* A name alone makes a container of no type. */
		return 0;
	case SHORT_PLACE_ITEMS:
		/* align-items (the justify-items after it is not kept in this pass). */
		components = values_components(tokens, count, starts, lengths, 3);
		if (components == 0)
			return EINVAL;
		return css_parse_value_as(parse, CSS_PROP_ALIGN_ITEMS, tokens + starts[0], lengths[0], out, out_count);
	default:
		break;
	}

	/* A longhand. */
	error = values_single(parse, property, tokens, count, &value);
	if (error != 0)
		return error;
	values_add(out, out_count, property, &value);

	/* Succeeded: one declaration. */
	return 0;
}

/*
 * Parses a longhand's value from tokens as a given property (used by the
 * shorthands).
 */
int
css_parse_value_as(
	struct css_parse *parse,
	int property,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *out_count)
{
	struct css_value value;
	int error;

	/* Parses the value and adds the declaration. */
	memset(&value, 0, sizeof(value));
	error = values_single(parse, property, tokens, count, &value);
	if (error != 0)
		return error;
	values_add(out, out_count, property, &value);

	/* Succeeded: one declaration. */
	return 0;
}

/*
 * Parses the font shorthand: [style] [weight] size[/line-height] family.
 */
int
css_parse_font(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *out_count)
{
	struct css_value value;
	size_t index;
	int keyword;
	int found;
	int error;

	/* Style and weight keywords come before the size. */
	index = 0;
	for (;;) {
		while (index < count && tokens[index].type == CSS_TOKEN_WHITESPACE)
			index++;
		if (index >= count)
			return EINVAL;
		memset(&value, 0, sizeof(value));
		found = values_keyword(values_font_style, &tokens[index], &keyword);
		if (found && keyword != 0) {
			value.kind = CSS_VALUE_KEYWORD;
			value.keyword = keyword;
			values_add(out, out_count, CSS_PROP_FONT_STYLE, &value);
			index++;
			continue;
		}

		/* A weight: a keyword or a number from 1 to 1000. */
		found = values_keyword(values_font_weight, &tokens[index], &keyword);
		if (!found && tokens[index].type == CSS_TOKEN_NUMBER && tokens[index].number >= 1 && tokens[index].number <= 1000) {
			found = 1;
			keyword = (int)tokens[index].number;
		}

		/* The weight is declared. */
		if (found) {
			value.kind = CSS_VALUE_KEYWORD;
			value.keyword = keyword;
			values_add(out, out_count, CSS_PROP_FONT_WEIGHT, &value);
			index++;
			continue;
		}

		/* The variants this pass does not draw are read past. */
		found = values_keyword(values_font_ignored, &tokens[index], &keyword);
		if (found) {
			index++;
			continue;
		}

		/* Anything else is the size. */
		break;
	}

	/* The size, and an optional line height after a slash. */
	error = css_parse_value_as(parse, CSS_PROP_FONT_SIZE, &tokens[index], 1, out, out_count);
	if (error != 0)
		return error;
	index++;
	if (index < count && tokens[index].type == CSS_TOKEN_DELIM && tokens[index].delim == '/') {
		index++;
		while (index < count && tokens[index].type == CSS_TOKEN_WHITESPACE)
			index++;
		if (index >= count)
			return EINVAL;
		error = css_parse_value_as(parse, CSS_PROP_LINE_HEIGHT, &tokens[index], 1, out, out_count);
		if (error != 0)
			return error;
		index++;
	}

	/* The families take the rest. */
	while (index < count && tokens[index].type == CSS_TOKEN_WHITESPACE)
		index++;
	if (index >= count)
		return EINVAL;
	error = css_parse_value_as(parse, CSS_PROP_FONT_FAMILY, &tokens[index], count - index, out, out_count);
	if (error != 0)
		return error;

	/* Succeeded: the longhands are declared. */
	return 0;
}

/*
 * Parses a color: a named color, transparent, currentcolor, #rgb, #rgba,
 * #rrggbb, #rrggbbaa, rgb() or rgba().
 *
 * currentcolor is reported as 0x00000001, a value no real color has with
 * zero alpha, which the cascade replaces with the color property.
 */
int
css_parse_color(
	const struct css_token *tokens,
	size_t count,
	uint32_t *color)
{
	const struct css_token *token;
	uint32_t value;
	size_t index;
	size_t used;
	int keyword;
	int digit;
	int same;

	/* Skips leading whitespace. */
	while (count > 0 && tokens[0].type == CSS_TOKEN_WHITESPACE) {
		tokens++;
		count--;
	}
	while (count > 0 && tokens[count - 1U].type == CSS_TOKEN_WHITESPACE)
		count--;

	/* Nothing is not a color. */
	if (count == 0)
		return EINVAL;
	token = &tokens[0];

	/* Keywords and named colors. */
	if (token->type == CSS_TOKEN_IDENT) {
		if (count != 1U)
			return EINVAL;
		same = values_keyword(values_color_keywords, token, &keyword);
		if (same) {
			*color = (uint32_t)keyword;
			return 0;
		}

		/* The named colors. */
		for (index = 0; values_colors[index].name != NULL; index++) {
			same = css_ident_equal(token, values_colors[index].name);
			if (same) {
				*color = 0xff000000U | values_colors[index].rgb;
				return 0;
			}
		}

		/* An unknown word is not a color. */
		return EINVAL;
	}

	/* Hex colors of 3, 4, 6 or 8 digits. */
	if (token->type == CSS_TOKEN_HASH) {
		if (count != 1U)
			return EINVAL;
		value = 0;
		for (index = 0; index < token->length; index++) {
			digit = values_hex_digit(token->text[index]);
			if (digit < 0)
				return EINVAL;
			value = value * 16U + (uint32_t)digit;
		}

		/* The number of digits picks the form. */
		switch (token->length) {
		case 3:
			*color = 0xff000000U |
			    ((value >> 8) & 0xfU) * 0x110000U |
			    ((value >> 4) & 0xfU) * 0x1100U |
			    (value & 0xfU) * 0x11U;
			return 0;
		case 4:
			*color = ((value & 0xfU) * 0x11U) << 24 |
			    ((value >> 12) & 0xfU) * 0x110000U |
			    ((value >> 8) & 0xfU) * 0x1100U |
			    ((value >> 4) & 0xfU) * 0x11U;
			return 0;
		case 6:
			*color = 0xff000000U | value;
			return 0;
		case 8:
			*color = (value << 24) | (value >> 8);
			return 0;
		default:
			return EINVAL;
		}
	}

	/* rgb() and rgba(). */
	if (token->type == CSS_TOKEN_FUNCTION) {
		same = css_ident_equal(token, "rgb");
		if (!same)
			same = css_ident_equal(token, "rgba");
		if (!same)
			return EINVAL;
		if (count < 2U || tokens[count - 1U].type != CSS_TOKEN_CLOSE_PAREN)
			return EINVAL;
		used = values_function_length(tokens, count);
		if (used != count)
			return EINVAL;
		return values_rgb_function(tokens + 1, count - 1U, color);
	}

	/* Anything else is not a color. */
	return EINVAL;
}

/* Looks a keyword up in a table; returns 1 and its value when it is there. */
static int
values_keyword(
	const struct values_keyword *table,
	const struct css_token *token,
	int *value)
{
	size_t index;
	int same;

	/* Only identifiers are keywords. */
	if (token->type != CSS_TOKEN_IDENT)
		return 0;

	/* Compares with each keyword. */
	for (index = 0; table[index].name != NULL; index++) {
		same = css_ident_equal(token, table[index].name);
		if (same) {
			*value = table[index].value;
			return 1;
		}
	}

	/* The word is not one of the table's. */
	return 0;
}

/* Looks a dimension's unit up; returns 1 and the unit when it is a length unit. */
static int
values_unit(
	const struct css_token *token,
	int *unit)
{
	size_t index;
	int same;

	/* Only dimensions have units. */
	if (token->type != CSS_TOKEN_DIMENSION)
		return 0;

	/* Compares the unit with each length unit. */
	for (index = 0; values_units[index].name != NULL; index++) {
		same = css_ident_equal(token, values_units[index].name);
		if (same) {
			*unit = values_units[index].value;
			return 1;
		}
	}

	/* The unit is not a length unit. */
	return 0;
}

/* Parses a length or percentage (and, when allowed, auto, none and normal). */
static int
values_length(
	const struct css_token *token,
	int allow_keywords,
	struct css_value *value)
{
	int found;

	/* Keywords. */
	value->kind = CSS_VALUE_LENGTH;
	if (token->type == CSS_TOKEN_IDENT && allow_keywords) {
		found = values_keyword(values_length_keywords, token, &value->keyword);
		if (!found)
			return EINVAL;
		value->kind = CSS_VALUE_KEYWORD;
		return 0;
	}

	/* Zero, percentages and dimensions. */
	if (token->type == CSS_TOKEN_NUMBER && token->number == 0) {
		value->number = 0;
		value->unit = CSS_DUNIT_PX;
		return 0;
	}

	/* A percentage. */
	if (token->type == CSS_TOKEN_PERCENTAGE) {
		value->number = (float)token->number;
		value->unit = CSS_DUNIT_PERCENT;
		return 0;
	}

	/* Otherwise a dimension with a known unit. */
	if (token->type != CSS_TOKEN_DIMENSION)
		return EINVAL;
	value->number = (float)token->number;
	found = values_unit(token, &value->unit);
	if (!found)
		return EINVAL;

	/* Succeeded: the length and its unit. */
	return 0;
}

/* Parses a longhand's value. */
static int
values_single(
	struct css_parse *parse,
	int property,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	const struct values_keyword *table;
	const struct css_token *parts[2];
	size_t used;
	int keyword;
	int is_auto;
	int found;
	int takes_length;
	int error;

	/* Skips surrounding whitespace. */
	while (count > 0 && tokens[0].type == CSS_TOKEN_WHITESPACE) {
		tokens++;
		count--;
	}
	while (count > 0 && tokens[count - 1U].type == CSS_TOKEN_WHITESPACE)
		count--;
	if (count == 0)
		return EINVAL;
	memset(value, 0, sizeof(*value));

	/* Families take a list; colors may be a function of several tokens. */
	if (property == CSS_PROP_FONT_FAMILY)
		return values_families(parse, tokens, count, value);
	if (property == CSS_PROP_COLOR || property == CSS_PROP_BACKGROUND_COLOR ||
	    (property >= CSS_PROP_BORDER_TOP_COLOR && property <= CSS_PROP_BORDER_LEFT_COLOR)) {
		error = css_parse_color(tokens, count, &value->color);
		if (error != 0)
			return error;
		value->kind = CSS_VALUE_COLOR;
		return 0;
	}

	/* An image: url(...) or none. */
	if (property == CSS_PROP_BACKGROUND_IMAGE)
		return values_url(parse, tokens, count, value);

	/* background-repeat takes one keyword or one for each axis. */
	if (property == CSS_PROP_BACKGROUND_REPEAT) {
		parts[0] = &tokens[0];
		parts[1] = &tokens[count - 1U];
		used = 2;
		if (count == 1)
			used = 1;
		error = values_repeat_parts(parts, used, value);
		return error;
	}

	/* background-attachment takes one scrolling keyword. */
	if (property == CSS_PROP_BACKGROUND_ATTACHMENT) {
		found = values_keyword(values_attachment, &tokens[0], &keyword);
		if (!found || count != 1)
			return EINVAL;
		value->kind = CSS_VALUE_KEYWORD;
		value->keyword = keyword;
		return 0;
	}

	/* content takes a list of strings and functions. */
	if (property == CSS_PROP_CONTENT)
		return values_content(parse, tokens, count, value);

	/* box-shadow takes a list of shadows. */
	if (property == CSS_PROP_BOX_SHADOW)
		return values_box_shadow(parse, tokens, count, value);

	/* clip-path takes a shape. */
	if (property == CSS_PROP_CLIP_PATH)
		return values_clip_path(parse, tokens, count, value);

	/* A grid template takes a list of tracks (ws074-p072). */
	if (property == CSS_PROP_GRID_TEMPLATE_COLUMNS || property == CSS_PROP_GRID_TEMPLATE_ROWS)
		return values_tracks(parse, tokens, count, value);

	/* A grid line: auto, a number, or span and a number. */
	if (property >= CSS_PROP_GRID_COLUMN_START && property <= CSS_PROP_GRID_ROW_END)
		return values_grid_line(tokens, count, value);

	/* The outline's width is a border's. */
	if (property == CSS_PROP_OUTLINE_WIDTH)
		return values_single(parse, CSS_PROP_BORDER_TOP_WIDTH, tokens, count, value);

	/* Its style is a border's, and auto a solid one. */
	if (property == CSS_PROP_OUTLINE_STYLE) {
		is_auto = css_ident_equal(&tokens[0], "auto");
		if (count == 1 && is_auto) {
			value->kind = CSS_VALUE_KEYWORD;
			value->keyword = CSS_BORDER_SOLID;
			return 0;
		}

		/* Any other style. */
		return values_single(parse, CSS_PROP_BORDER_TOP_STYLE, tokens, count, value);
	}

	/* Its color is a border's, and invert the current color. */
	if (property == CSS_PROP_OUTLINE_COLOR) {
		is_auto = css_ident_equal(&tokens[0], "invert");
		if (count == 1 && is_auto) {
			value->kind = CSS_VALUE_COLOR;
			value->color = CSS_CURRENT_COLOR;
			return 0;
		}

		/* Any other color. */
		return values_single(parse, CSS_PROP_BORDER_TOP_COLOR, tokens, count, value);
	}

	/* A length may be a calculation, which is a function of several tokens. */
	takes_length = values_takes_length(property);
	if (takes_length && tokens[0].type == CSS_TOKEN_FUNCTION) {
		used = values_function_length(tokens, count);
		if (used == count) {
			error = values_calc(parse, property, tokens, count, value);
			return error;
		}
	}

	/* The other properties take one token. */
	if (count != 1)
		return EINVAL;

	/* One axis of background-position: a keyword or a length. */
	if (property == CSS_PROP_BACKGROUND_POSITION_X || property == CSS_PROP_BACKGROUND_POSITION_Y)
		return values_position_part(&tokens[0], value);

	/* The keyword properties. */
	table = NULL;
	switch (property) {
	case CSS_PROP_DISPLAY:
		table = values_display;
		break;
	case CSS_PROP_POSITION:
		table = values_position;
		break;
	case CSS_PROP_FLOAT:
		table = values_float;
		break;
	case CSS_PROP_CLEAR:
		table = values_clear;
		break;
	case CSS_PROP_OVERFLOW:
	case CSS_PROP_OVERFLOW_X:
	case CSS_PROP_OVERFLOW_Y:
		table = values_overflow;
		break;
	case CSS_PROP_VISIBILITY:
		table = values_visibility;
		break;
	case CSS_PROP_BORDER_TOP_STYLE:
	case CSS_PROP_BORDER_RIGHT_STYLE:
	case CSS_PROP_BORDER_BOTTOM_STYLE:
	case CSS_PROP_BORDER_LEFT_STYLE:
		table = values_border_style;
		break;
	case CSS_PROP_FONT_STYLE:
		table = values_font_style;
		break;
	case CSS_PROP_TEXT_ALIGN:
		table = values_text_align;
		break;
	case CSS_PROP_DIRECTION:
		table = values_direction;
		break;
	case CSS_PROP_CONTAINER_TYPE:
		table = values_container_type;
		break;
	case CSS_PROP_BORDER_COLLAPSE:
		table = values_border_collapse;
		break;
	case CSS_PROP_CURSOR:
		table = values_cursor;
		break;
	case CSS_PROP_TEXT_TRANSFORM:
		/* Use the exact four native computed transform keywords. */
		table = values_text_transform;
		break;
	case CSS_PROP_WHITE_SPACE:
		table = values_white_space;
		break;
	case CSS_PROP_TEXT_DECORATION_LINE:
		table = values_decoration;
		break;
	case CSS_PROP_LIST_STYLE_TYPE:
		table = values_list_style;
		break;
	case CSS_PROP_BOX_SIZING:
		table = values_box_sizing;
		break;
	case CSS_PROP_FLEX_DIRECTION:
		table = values_flex_direction;
		break;
	case CSS_PROP_FLEX_WRAP:
		table = values_flex_wrap;
		break;
	case CSS_PROP_JUSTIFY_CONTENT:
	case CSS_PROP_ALIGN_ITEMS:
	case CSS_PROP_ALIGN_CONTENT:
	case CSS_PROP_ALIGN_SELF:
		table = values_align;
		break;
	default:
		break;
	}

	/* A keyword property takes a keyword of its table. */
	if (table != NULL) {
		found = values_keyword(table, &tokens[0], &keyword);
		if (!found)
			return EINVAL;
		value->kind = CSS_VALUE_KEYWORD;
		value->keyword = keyword;
		return 0;
	}

	/* vertical-align: a keyword, or a length or percentage that raises the box. */
	if (property == CSS_PROP_VERTICAL_ALIGN) {
		found = values_keyword(values_vertical_align, &tokens[0], &keyword);
		if (found) {
			value->kind = CSS_VALUE_KEYWORD;
			value->keyword = keyword;
			return 0;
		}

		/* A length or a percentage. */
		error = values_length(&tokens[0], 0, value);
		return error;
	}

	/* opacity: a number or a percentage, held between 0 and 1. */
	if (property == CSS_PROP_OPACITY) {
		value->kind = CSS_VALUE_NUMBER;
		if (tokens[0].type == CSS_TOKEN_NUMBER) {
			value->number = (float)tokens[0].number;
		} else if (tokens[0].type == CSS_TOKEN_PERCENTAGE) {
			value->number = (float)tokens[0].number / 100.0f;
		} else {
			return EINVAL;
		}

		/* The number, held in range. */
		if (value->number < 0)
			value->number = 0;
		if (value->number > 1)
			value->number = 1;
		return 0;
	}

	/* A corner's radius on one axis: a length or a percentage that is not negative. */
	if (property >= CSS_PROP_RADIUS_TOP_LEFT_X && property <= CSS_PROP_RADIUS_BOTTOM_LEFT_Y) {
		error = values_length(&tokens[0], 0, value);
		if (error != 0)
			return error;
		if (value->number < 0)
			return EINVAL;
		return 0;
	}

	/* text-indent, outline-offset and border-spacing: a length. */
	if (property == CSS_PROP_TEXT_INDENT || property == CSS_PROP_OUTLINE_OFFSET ||
	    property == CSS_PROP_BORDER_SPACING_X || property == CSS_PROP_BORDER_SPACING_Y)
		return values_length(&tokens[0], 0, value);

	/* flex-grow and flex-shrink: a number that is not negative. */
	if (property == CSS_PROP_FLEX_GROW || property == CSS_PROP_FLEX_SHRINK) {
		if (tokens[0].type != CSS_TOKEN_NUMBER || tokens[0].number < 0)
			return EINVAL;
		value->kind = CSS_VALUE_NUMBER;
		value->number = (float)tokens[0].number;
		return 0;
	}

	/* order: an integer. */
	if (property == CSS_PROP_ORDER) {
		if (tokens[0].type != CSS_TOKEN_NUMBER || !tokens[0].integer)
			return EINVAL;
		value->kind = CSS_VALUE_NUMBER;
		value->number = (float)tokens[0].number;
		return 0;
	}

	/* The gaps: normal (none) or a length. */
	if (property == CSS_PROP_ROW_GAP || property == CSS_PROP_COLUMN_GAP) {
		is_auto = css_ident_equal(&tokens[0], "normal");
		if (is_auto) {
			value->kind = CSS_VALUE_LENGTH;
			value->number = 0;
			value->unit = CSS_DUNIT_PX;
			return 0;
		}

		/* A length. */
		return values_length(&tokens[0], 0, value);
	}

	/* flex-basis: auto, content (as auto) or a length. */
	if (property == CSS_PROP_FLEX_BASIS) {
		is_auto = css_ident_equal(&tokens[0], "content");
		if (is_auto) {
			value->kind = CSS_VALUE_KEYWORD;
			value->keyword = CSS_UNIT_AUTO;
			return 0;
		}

		/* auto or a length. */
		return values_length(&tokens[0], 1, value);
	}

	/* z-index: auto or an integer. */
	if (property == CSS_PROP_Z_INDEX) {
		is_auto = css_ident_equal(&tokens[0], "auto");
		if (is_auto) {
			value->kind = CSS_VALUE_KEYWORD;
			value->keyword = 0;
			return 0;
		}

		/* An integer (a number token without a fraction). */
		if (tokens[0].type != CSS_TOKEN_NUMBER || !tokens[0].integer)
			return EINVAL;
		value->kind = CSS_VALUE_NUMBER;
		value->number = (float)tokens[0].number;
		return 0;
	}

	/* font-weight: a keyword or a number from 1 to 1000. */
	if (property == CSS_PROP_FONT_WEIGHT) {
		found = values_keyword(values_font_weight, &tokens[0], &keyword);
		if (!found && tokens[0].type == CSS_TOKEN_NUMBER && tokens[0].number >= 1 && tokens[0].number <= 1000) {
			found = 1;
			keyword = (int)tokens[0].number;
		}

		/* Anything else is not a weight. */
		if (!found)
			return EINVAL;
		value->kind = CSS_VALUE_KEYWORD;
		value->keyword = keyword;
		return 0;
	}

	/* font-size: a keyword or a length. */
	if (property == CSS_PROP_FONT_SIZE) {
		found = values_keyword(values_font_size, &tokens[0], &keyword);
		if (found) {
			/* A keyword's size at the default size, marked as a keyword so the monospace family can rescale it. */
			value->kind = CSS_VALUE_LENGTH;
			value->number = (float)keyword;
			value->unit = CSS_DUNIT_FONT_KEYWORD;
			return 0;
		}

		/* The relative sizes. */
		found = values_keyword(values_relative_size, &tokens[0], &keyword);
		if (found) {
			value->kind = CSS_VALUE_LENGTH;
			value->number = (float)keyword;
			value->unit = CSS_DUNIT_PERCENT;
			return 0;
		}

		/* Otherwise a length (no keywords). */
		return values_length(&tokens[0], 0, value);
	}

	/* line-height: normal, a number or a length. */
	if (property == CSS_PROP_LINE_HEIGHT) {
		if (tokens[0].type == CSS_TOKEN_NUMBER) {
			value->kind = CSS_VALUE_NUMBER;
			value->number = (float)tokens[0].number;
			return 0;
		}

		/* Or a length, or normal. */
		return values_length(&tokens[0], 1, value);
	}

	/* Border widths: a keyword or a length. */
	if (property >= CSS_PROP_BORDER_TOP_WIDTH && property <= CSS_PROP_BORDER_LEFT_WIDTH) {
		found = values_keyword(values_border_width, &tokens[0], &keyword);
		if (found) {
			value->kind = CSS_VALUE_LENGTH;
			value->number = (float)keyword;
			value->unit = CSS_DUNIT_PX;
			return 0;
		}

		/* Or a length. */
		return values_length(&tokens[0], 0, value);
	}

	/* The other lengths: sizes, margins and paddings. */
	return values_length(&tokens[0], 1, value);
}

/* Parses font-family: names (identifiers joined by spaces, or strings) separated by commas. */
static int
values_families(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	struct wb_units name;
	size_t index;
	int error;

	/* Reads each family between the commas. */
	value->kind = CSS_VALUE_FAMILIES;
	value->family_count = 0;
	wb_units_init(&name);
	index = 0;
	while (index <= count) {
		/* A comma or the end finishes the name gathered. */
		if (index == count || tokens[index].type == CSS_TOKEN_COMMA) {
			while (name.length > 0 && name.data[name.length - 1U] == ' ')
				name.length--;
			if (name.length != 0 && value->family_count < CSS_DECLARED_FAMILIES) {
				value->families[value->family_count] = vm_atom_from_units(parse->heap, name.data, name.length);
				if (value->families[value->family_count] == NULL) {
					wb_units_release(&name);
					return ENOMEM;
				}

				/* The family is kept. */
				value->family_count++;
			}

			/* The next name starts empty. */
			wb_units_clear(&name);
			index++;
			continue;
		}

		/* Identifiers and strings make the name; whitespace between words is one space. */
		if (tokens[index].type == CSS_TOKEN_IDENT || tokens[index].type == CSS_TOKEN_STRING) {
			error = wb_units_append(&name, tokens[index].text, tokens[index].length);
			if (error != 0) {
				wb_units_release(&name);
				return ENOMEM;
			}
		} else if (tokens[index].type == CSS_TOKEN_WHITESPACE && name.length != 0) {
			error = wb_units_append_code_point(&name, ' ');
			if (error != 0) {
				wb_units_release(&name);
				return ENOMEM;
			}
		} else if (tokens[index].type != CSS_TOKEN_WHITESPACE) {
			wb_units_release(&name);
			return EINVAL;
		}

		/* The next token. */
		index++;
	}

	/* The gathered name is no longer needed. */
	wb_units_release(&name);

	/* A family list needs a family. */
	if (value->family_count == 0)
		return EINVAL;

	/* Succeeded: the families in order. */
	return 0;
}

/* Splits a value into its space-separated components (functions whole); returns how many. */
static size_t
values_components(
	const struct css_token *tokens,
	size_t count,
	size_t *starts,
	size_t *lengths,
	size_t max)
{
	size_t index;
	size_t made;
	size_t start;
	int depth;

	/* Walks the tokens, keeping function arguments with their function. */
	made = 0;
	index = 0;
	while (index < count && made < max) {
		if (tokens[index].type == CSS_TOKEN_WHITESPACE) {
			index++;
			continue;
		}

		/* The component starts here and runs to whitespace outside a function. */
		start = index;
		depth = 0;
		do {
			if (tokens[index].type == CSS_TOKEN_FUNCTION || tokens[index].type == CSS_TOKEN_OPEN_PAREN)
				depth++;
			if (tokens[index].type == CSS_TOKEN_CLOSE_PAREN)
				depth--;
			index++;
		} while (index < count && (depth > 0 || tokens[index].type != CSS_TOKEN_WHITESPACE));
		starts[made] = start;
		lengths[made] = index - start;
		made++;
	}

	/* Reports how many components there are. */
	return made;
}

/* Parses a four-sided shorthand (margin, padding, border-width/style/color). */
static int
values_four(
	struct css_parse *parse,
	int first,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value values[4];
	size_t starts[5];
	size_t lengths[5];
	size_t components;
	size_t index;
	int error;

	/* One to four components. */
	components = values_components(tokens, count, starts, lengths, 5);
	if (components == 0 || components > 4)
		return EINVAL;
	for (index = 0; index < components; index++) {
		error = values_single(parse, first, tokens + starts[index], lengths[index], &values[index]);
		if (error != 0)
			return error;
	}

	/* Top, right, bottom, left, the missing ones copied from their opposites. */
	if (components < 2)
		values[1] = values[0];
	if (components < 3)
		values[2] = values[0];
	if (components < 4)
		values[3] = values[1];
	for (index = 0; index < 4; index++)
		values_add(out, made, first + (int)index, &values[index]);

	/* Succeeded: four longhands. */
	return 0;
}

/* Parses a border shorthand for one side, or all four when side is -1. */
static int
values_border(
	struct css_parse *parse,
	int side,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value width;
	struct css_value style;
	struct css_value color;
	struct css_value candidate;
	size_t starts[4];
	size_t lengths[4];
	size_t components;
	size_t index;
	int error;
	int first;
	int last;

	/* The defaults: medium, none, currentcolor. */
	memset(&width, 0, sizeof(width));
	width.kind = CSS_VALUE_LENGTH;
	width.number = 3;
	width.unit = CSS_DUNIT_PX;
	memset(&style, 0, sizeof(style));
	style.kind = CSS_VALUE_KEYWORD;
	style.keyword = CSS_BORDER_NONE;
	memset(&color, 0, sizeof(color));
	color.kind = CSS_VALUE_COLOR;
	color.color = CSS_CURRENT_COLOR;

	/* Each component is a width, a style or a color, in any order. */
	components = values_components(tokens, count, starts, lengths, 4);
	if (components == 0 || components > 3)
		return EINVAL;
	for (index = 0; index < components; index++) {
		error = values_single(parse, CSS_PROP_BORDER_TOP_STYLE, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			style = candidate;
			continue;
		}

		/* Or a width. */
		error = values_single(parse, CSS_PROP_BORDER_TOP_WIDTH, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			width = candidate;
			continue;
		}

		/* Or a color. */
		error = values_single(parse, CSS_PROP_BORDER_TOP_COLOR, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			color = candidate;
			continue;
		}

		/* Anything else spoils the shorthand. */
		return EINVAL;
	}

	/* Declares the three longhands on the side or sides. */
	first = side;
	last = side;
	if (side < 0) {
		first = CSS_TOP;
		last = CSS_LEFT;
	}

	/* Each side gets the three. */
	for (index = (size_t)first; index <= (size_t)last; index++) {
		values_add(out, made, CSS_PROP_BORDER_TOP_WIDTH + (int)index, &width);
		values_add(out, made, CSS_PROP_BORDER_TOP_STYLE + (int)index, &style);
		values_add(out, made, CSS_PROP_BORDER_TOP_COLOR + (int)index, &color);
	}

	/* Succeeded: the longhands are declared. */
	return 0;
}

/* Adds one longhand declaration to the expansion. */
static void
values_add(
	struct css_declaration *out,
	size_t *made,
	int property,
	const struct css_value *value)
{
	/* Fills the next place: an ordinary declaration, neither custom nor pending. */
	memset(&out[*made], 0, sizeof(out[*made]));
	out[*made].property = property;
	out[*made].important = 0;
	out[*made].value = *value;
	(*made)++;
}

/* Reads a hexadecimal digit, or reports -1. */
static int
values_hex_digit(
	uint16_t unit)
{
	/* The three ranges of hex digits. */
	if (unit >= '0' && unit <= '9')
		return unit - '0';
	if (unit >= 'a' && unit <= 'f')
		return unit - 'a' + 10;
	if (unit >= 'A' && unit <= 'F')
		return unit - 'A' + 10;

	/* Anything else is not a hex digit. */
	return -1;
}

/* Parses the arguments of rgb() or rgba(): three numbers or percentages and an optional alpha. */
static int
values_rgb_function(
	const struct css_token *tokens,
	size_t count,
	uint32_t *color)
{
	double channels[4];
	size_t index;
	int made;
	double value;

	/* Reads up to four numeric arguments, separated by commas, spaces or a slash. */
	made = 0;
	channels[3] = 1.0;
	for (index = 0; index < count && tokens[index].type != CSS_TOKEN_CLOSE_PAREN; index++) {
		if (tokens[index].type == CSS_TOKEN_NUMBER || tokens[index].type == CSS_TOKEN_PERCENTAGE) {
			if (made >= 4)
				return EINVAL;
			value = tokens[index].number;
			if (tokens[index].type == CSS_TOKEN_PERCENTAGE && made == 3) {
				/* An alpha percentage is a fraction of one. */
				value = value / 100.0;
			} else if (tokens[index].type == CSS_TOKEN_PERCENTAGE) {
				/* A channel percentage is a fraction of 255. */
				value = value * 255.0 / 100.0;
			}

			/* The channel is kept. */
			channels[made] = value;
			made++;
			continue;
		}

		/* Separators are skipped. */
		if (tokens[index].type == CSS_TOKEN_WHITESPACE || tokens[index].type == CSS_TOKEN_COMMA)
			continue;
		if (tokens[index].type == CSS_TOKEN_DELIM && tokens[index].delim == '/')
			continue;
		return EINVAL;
	}

	/* Three channels are needed. */
	if (made < 3)
		return EINVAL;

	/* Clamps and packs the channels. */
	for (index = 0; index < 3; index++) {
		if (channels[index] < 0)
			channels[index] = 0;
		if (channels[index] > 255)
			channels[index] = 255;
	}

	/* The alpha between zero and one. */
	if (channels[3] < 0)
		channels[3] = 0;
	if (channels[3] > 1)
		channels[3] = 1;
	*color = (uint32_t)(channels[3] * 255.0 + 0.5) << 24 |
	    (uint32_t)(channels[0] + 0.5) << 16 |
	    (uint32_t)(channels[1] + 0.5) << 8 |
	    (uint32_t)(channels[2] + 0.5);

	/* Succeeded: the color is packed. */
	return 0;
}

/* Declares a CSS-wide keyword for a longhand, or for every longhand of a four-sided shorthand. */
static void
values_wide_keyword(
	int property,
	const struct css_value *value,
	struct css_declaration *out,
	size_t *made)
{
	int first;

	/* A longhand takes it itself. */
	if (property < CSS_PROP_COUNT) {
		values_add(out, made, property, value);
		return;
	}

	/* The four-sided shorthands give it to their four longhands. */
	first = -1;
	switch (property) {
	case SHORT_MARGIN:
		first = CSS_PROP_MARGIN_TOP;
		break;
	case SHORT_PADDING:
		first = CSS_PROP_PADDING_TOP;
		break;
	case SHORT_BORDER_WIDTH:
		first = CSS_PROP_BORDER_TOP_WIDTH;
		break;
	case SHORT_BORDER_STYLE:
		first = CSS_PROP_BORDER_TOP_STYLE;
		break;
	case SHORT_BORDER_COLOR:
		first = CSS_PROP_BORDER_TOP_COLOR;
		break;
	case SHORT_BACKGROUND_POSITION:
		values_add(out, made, CSS_PROP_BACKGROUND_POSITION_X, value);
		values_add(out, made, CSS_PROP_BACKGROUND_POSITION_Y, value);
		return;
	case SHORT_BACKGROUND_SIZE:
		values_add(out, made, CSS_PROP_BACKGROUND_SIZE_WIDTH, value);
		values_add(out, made, CSS_PROP_BACKGROUND_SIZE_HEIGHT, value);
		return;
	case SHORT_BACKGROUND:
		values_add(out, made, CSS_PROP_BACKGROUND_COLOR, value);
		values_add(out, made, CSS_PROP_BACKGROUND_IMAGE, value);
		values_add(out, made, CSS_PROP_BACKGROUND_REPEAT, value);
		values_add(out, made, CSS_PROP_BACKGROUND_POSITION_X, value);
		values_add(out, made, CSS_PROP_BACKGROUND_POSITION_Y, value);
		values_add(out, made, CSS_PROP_BACKGROUND_SIZE_WIDTH, value);
		values_add(out, made, CSS_PROP_BACKGROUND_SIZE_HEIGHT, value);
		return;
	case SHORT_BORDER_RADIUS:
		/* The eight radii (ws074-p062). */
		for (first = CSS_PROP_RADIUS_TOP_LEFT_X; first <= CSS_PROP_RADIUS_BOTTOM_LEFT_Y; first++)
			values_add(out, made, first, value);
		return;
	case SHORT_RADIUS_TOP_LEFT:
	case SHORT_RADIUS_TOP_RIGHT:
	case SHORT_RADIUS_BOTTOM_RIGHT:
	case SHORT_RADIUS_BOTTOM_LEFT:
		/* A corner's two radii. */
		first = CSS_PROP_RADIUS_TOP_LEFT_X + 2 * (property - SHORT_RADIUS_TOP_LEFT);
		values_add(out, made, first, value);
		values_add(out, made, first + 1, value);
		return;
	case SHORT_OUTLINE:
		/* The outline's width, style and color. */
		values_add(out, made, CSS_PROP_OUTLINE_WIDTH, value);
		values_add(out, made, CSS_PROP_OUTLINE_STYLE, value);
		values_add(out, made, CSS_PROP_OUTLINE_COLOR, value);
		return;
	case SHORT_GRID_COLUMN:
		/* A grid item's column lines. */
		values_add(out, made, CSS_PROP_GRID_COLUMN_START, value);
		values_add(out, made, CSS_PROP_GRID_COLUMN_END, value);
		return;
	case SHORT_GRID_ROW:
		/* Its row lines. */
		values_add(out, made, CSS_PROP_GRID_ROW_START, value);
		values_add(out, made, CSS_PROP_GRID_ROW_END, value);
		return;
	case SHORT_GRID_AREA:
		/* All four. */
		for (first = CSS_PROP_GRID_COLUMN_START; first <= CSS_PROP_GRID_ROW_END; first++)
			values_add(out, made, first, value);
		return;
	default:
		break;
	}

	/* The other shorthands are left alone in this pass. */
	if (first < 0)
		return;

	/* Declares the four. */
	values_add(out, made, first, value);
	values_add(out, made, first + 1, value);
	values_add(out, made, first + 2, value);
	values_add(out, made, first + 3, value);
}

/*
 * Parses the background shorthand: its color, image, repeat, position
 * and, after a slash, its size, in any order; what it leaves out takes
 * its initial value (transparent, none, repeat, 0% 0%, auto).
 */
static int
values_background(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	const struct css_token *positions[2];
	const struct css_token *sizes[2];
	const struct css_token *repeats[2];
	struct css_value color;
	struct css_value image;
	struct css_value repeat;
	struct css_value attachment;
	struct css_value x;
	struct css_value y;
	struct css_value width;
	struct css_value height;
	size_t position_count;
	size_t size_count;
	size_t repeat_count;
	size_t index;
	size_t rest;
	int after_slash;
	int attachment_seen;
	int color_seen;
	int image_seen;
	int is_url;
	int named;
	int keyword;
	int found;
	int error;

	/* The initial values. */
	memset(&color, 0, sizeof(color));
	color.kind = CSS_VALUE_COLOR;
	memset(&image, 0, sizeof(image));
	image.kind = CSS_VALUE_KEYWORD;
	memset(&attachment, 0, sizeof(attachment));
	attachment.kind = CSS_VALUE_KEYWORD;
	attachment.keyword = CSS_BACKGROUND_SCROLL;
	position_count = 0;
	size_count = 0;
	repeat_count = 0;
	after_slash = 0;
	attachment_seen = 0;
	color_seen = 0;
	image_seen = 0;

	/* Each component, token by token (a function with its arguments). */
	index = 0;
	while (index < count) {
		/* Whitespace separates components. */
		if (tokens[index].type == CSS_TOKEN_WHITESPACE) {
			index++;
			continue;
		}

		/* A slash: the size follows. */
		if (tokens[index].type == CSS_TOKEN_DELIM && tokens[index].delim == '/') {
			after_slash = 1;
			index++;
			continue;
		}

		/* The tokens of the component: one, or a function to its closing parenthesis. */
		rest = values_function_length(tokens + index, count - index);

		/* An image: url(...) or none. */
		is_url = 0;
		named = css_ident_equal(&tokens[index], "url");
		if (tokens[index].type == CSS_TOKEN_URL)
			is_url = 1;
		if (tokens[index].type == CSS_TOKEN_FUNCTION && named)
			is_url = 1;
		if (is_url) {
			if (image_seen)
				return EINVAL;
			error = values_url(parse, tokens + index, rest, &image);
			if (error != 0)
				return error;
			image_seen = 1;
			index += rest;
			continue;
		}

		/* none is no image. */
		named = css_ident_equal(&tokens[index], "none");
		if (named) {
			if (image_seen)
				return EINVAL;
			image.kind = CSS_VALUE_KEYWORD;
			image.keyword = 0;
			image_seen = 1;
			index += rest;
			continue;
		}

		/* The attachment controls whether positioning uses the box or the viewport. */
		named = values_keyword(values_attachment, &tokens[index], &keyword);
		if (named) {
			if (attachment_seen)
				return EINVAL;
			attachment_seen = 1;
			attachment.keyword = keyword;
			index += rest;
			continue;
		}

		/* A repeat keyword. */
		found = values_keyword(values_repeat, &tokens[index], &keyword);
		if (found && repeat_count < 2) {
			repeats[repeat_count] = &tokens[index];
			repeat_count++;
			index += rest;
			continue;
		}

		/* After the slash, a part of the size. */
		if (after_slash && size_count < 2) {
			sizes[size_count] = &tokens[index];
			size_count++;
			index += rest;
			continue;
		}

		/* A position keyword or a length is a part of the position. */
		found = values_keyword(values_position_keywords, &tokens[index], &keyword);
		if (tokens[index].type == CSS_TOKEN_NUMBER || tokens[index].type == CSS_TOKEN_PERCENTAGE || tokens[index].type == CSS_TOKEN_DIMENSION)
			found = 1;
		if (found && position_count < 2) {
			positions[position_count] = &tokens[index];
			position_count++;
			index += rest;
			continue;
		}

		/* Anything else must be the color. */
		if (color_seen)
			return EINVAL;
		error = css_parse_color(&tokens[index], rest, &color.color);
		if (error != 0)
			return EINVAL;
		color_seen = 1;
		index += rest;
	}

	/* The repeat, the position and the size from their parts. */
	error = values_repeat_parts(repeats, repeat_count, &repeat);
	if (error != 0)
		return error;
	error = values_position_parts(positions, position_count, &x, &y);
	if (error != 0)
		return error;
	error = values_size_parts(sizes, size_count, &width, &height);
	if (error != 0)
		return error;

	/* Declares every longhand of the shorthand. */
	values_add(out, made, CSS_PROP_BACKGROUND_COLOR, &color);
	values_add(out, made, CSS_PROP_BACKGROUND_IMAGE, &image);
	values_add(out, made, CSS_PROP_BACKGROUND_REPEAT, &repeat);
	values_add(out, made, CSS_PROP_BACKGROUND_ATTACHMENT, &attachment);
	values_add(out, made, CSS_PROP_BACKGROUND_POSITION_X, &x);
	values_add(out, made, CSS_PROP_BACKGROUND_POSITION_Y, &y);
	values_add(out, made, CSS_PROP_BACKGROUND_SIZE_WIDTH, &width);
	values_add(out, made, CSS_PROP_BACKGROUND_SIZE_HEIGHT, &height);

	/* Succeeded: the longhands are declared. */
	return 0;
}

/* Measures a component's tokens: one, or a function or a parenthesis with its arguments. */
static size_t
values_function_length(
	const struct css_token *tokens,
	size_t count)
{
	size_t index;
	int depth;

	/* A plain token is one. */
	if (tokens[0].type != CSS_TOKEN_FUNCTION && tokens[0].type != CSS_TOKEN_OPEN_PAREN)
		return 1;

	/* To the parenthesis that closes it (or the end). */
	depth = 0;
	for (index = 0; index < count; index++) {
		if (tokens[index].type == CSS_TOKEN_FUNCTION || tokens[index].type == CSS_TOKEN_OPEN_PAREN)
			depth++;
		if (tokens[index].type == CSS_TOKEN_CLOSE_PAREN)
			depth--;
		if (depth == 0)
			return index + 1U;
	}

	/* An unclosed function runs to the end. */
	return count;
}

/*
 * Parses an image: url(...) (a URL token, or the url function with a
 * string) into an atom of its text, or none.
 */
static int
values_url(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	size_t index;
	int named;

	/* none. */
	memset(value, 0, sizeof(*value));
	named = css_ident_equal(&tokens[0], "none");
	if (count == 1 && named) {
		value->kind = CSS_VALUE_KEYWORD;
		return 0;
	}

	/* A URL token carries its text. */
	if (count == 1 && tokens[0].type == CSS_TOKEN_URL) {
		value->url = vm_atom_from_units(parse->heap, tokens[0].text, tokens[0].length);
		if (value->url == NULL)
			return ENOMEM;
		value->kind = CSS_VALUE_URL;
		return 0;
	}

	/* url( "string" ): the function, whitespace, the string, whitespace, the parenthesis. */
	named = css_ident_equal(&tokens[0], "url");
	if (tokens[0].type != CSS_TOKEN_FUNCTION || !named)
		return EINVAL;
	index = 1;
	while (index < count && tokens[index].type == CSS_TOKEN_WHITESPACE)
		index++;
	if (index >= count || tokens[index].type != CSS_TOKEN_STRING)
		return EINVAL;

	/* The string's text. */
	value->url = vm_atom_from_units(parse->heap, tokens[index].text, tokens[index].length);
	if (value->url == NULL)
		return ENOMEM;

	/* Succeeded: the URL. */
	value->kind = CSS_VALUE_URL;
	return 0;
}

/*
 * Makes background-repeat from its keywords: one (repeat, repeat-x,
 * repeat-y, no-repeat; space and round repeat), or one for each axis;
 * none is repeat.
 */
static int
values_repeat_parts(
	const struct css_token *const *parts,
	size_t count,
	struct css_value *value)
{
	int second;
	int across;
	int down;
	int found;

	/* The initial value. */
	memset(value, 0, sizeof(*value));
	value->kind = CSS_VALUE_KEYWORD;
	value->keyword = CSS_REPEAT_BOTH;
	if (count == 0)
		return 0;

	/* One keyword names both axes. */
	found = values_keyword(values_repeat, parts[0], &value->keyword);
	if (!found)
		return EINVAL;
	if (count == 1)
		return 0;

	/* Two keywords: each axis repeats or not (repeat-x and repeat-y name both axes, and cannot be one of two). */
	found = values_keyword(values_repeat, parts[1], &second);
	if (!found)
		return EINVAL;
	if (value->keyword == CSS_REPEAT_X || value->keyword == CSS_REPEAT_Y)
		return EINVAL;
	if (second == CSS_REPEAT_X || second == CSS_REPEAT_Y)
		return EINVAL;

	/* The axes that do not repeat. */
	across = 0;
	if (value->keyword == CSS_REPEAT_NONE)
		across = 1;
	down = 0;
	if (second == CSS_REPEAT_NONE)
		down = 1;

	/* Decide by the axes that do not repeat. */
	if (!across && !down) {
		value->keyword = CSS_REPEAT_BOTH;
	} else if (!across) {
		value->keyword = CSS_REPEAT_X;
	} else if (!down) {
		value->keyword = CSS_REPEAT_Y;
	} else {
		value->keyword = CSS_REPEAT_NONE;
	}

	/* Succeeded: the repeat. */
	return 0;
}

/*
 * Makes background-position's two axes from one or two parts: keywords
 * (left, center, right, top, bottom, as percentages) or lengths.  One part
 * is centred on the other axis; top and bottom name the vertical axis
 * wherever they are.
 */
static int
values_position_parts(
	const struct css_token *const *parts,
	size_t count,
	struct css_value *x,
	struct css_value *y)
{
	struct css_value first;
	struct css_value second;
	int first_vertical;
	int second_horizontal;
	int axis;
	int error;

	/* The initial value, 0% 0%. */
	memset(x, 0, sizeof(*x));
	x->kind = CSS_VALUE_LENGTH;
	x->unit = CSS_DUNIT_PERCENT;
	*y = *x;
	if (count == 0)
		return 0;

	/* The first part. */
	error = values_position_part(parts[0], &first);
	if (error != 0)
		return error;
	axis = values_position_axis(parts[0]);
	first_vertical = 0;
	if (axis == 2)
		first_vertical = 1;

	/* One part: it and the centre of the other axis. */
	if (count == 1) {
		second.kind = CSS_VALUE_LENGTH;
		second.number = 50.0f;
		second.unit = CSS_DUNIT_PERCENT;
		*x = first;
		*y = second;
		if (first_vertical) {
			*x = second;
			*y = first;
		}

		/* Succeeded: one axis given, the other centred. */
		return 0;
	}

	/* Two parts, horizontal first unless the keywords say otherwise. */
	error = values_position_part(parts[1], &second);
	if (error != 0)
		return error;
	axis = values_position_axis(parts[1]);
	second_horizontal = 0;
	if (axis == 1)
		second_horizontal = 1;
	*x = first;
	*y = second;
	if (first_vertical || second_horizontal) {
		*x = second;
		*y = first;
	}

	/* Succeeded: both axes. */
	return 0;
}

/* Tells which axis a part of a position names: 1 for left and right, 2 for top and bottom, 0 for any other. */
static int
values_position_axis(
	const struct css_token *token)
{
	int named;

	/* left and right are horizontal. */
	named = css_ident_equal(token, "left");
	if (named)
		return 1;
	named = css_ident_equal(token, "right");
	if (named)
		return 1;

	/* top and bottom are vertical. */
	named = css_ident_equal(token, "top");
	if (named)
		return 2;
	named = css_ident_equal(token, "bottom");
	if (named)
		return 2;

	/* center and lengths name neither. */
	return 0;
}

/* Reads one part of a position: a keyword as a percentage, or a length. */
static int
values_position_part(
	const struct css_token *token,
	struct css_value *value)
{
	int keyword;
	int found;
	int error;

	/* A keyword is a percentage of the room. */
	memset(value, 0, sizeof(*value));
	found = values_keyword(values_position_keywords, token, &keyword);
	if (found) {
		value->kind = CSS_VALUE_LENGTH;
		value->number = (float)keyword;
		value->unit = CSS_DUNIT_PERCENT;
		return 0;
	}

	/* A length or a percentage. */
	error = values_length(token, 0, value);
	if (error != 0)
		return error;

	/* Succeeded: the length. */
	return 0;
}

/*
 * Makes background-size's width and height from one or two parts:
 * contain or cover, or a width (and a height; auto when there is none),
 * each a length, a percentage or auto.
 */
static int
values_size_parts(
	const struct css_token *const *parts,
	size_t count,
	struct css_value *width,
	struct css_value *height)
{
	int named;
	int error;

	/* The initial value, auto auto. */
	memset(width, 0, sizeof(*width));
	width->kind = CSS_VALUE_KEYWORD;
	width->keyword = CSS_UNIT_AUTO;
	*height = *width;
	if (count == 0)
		return 0;

	/* contain and cover size both sides. */
	named = css_ident_equal(parts[0], "contain");
	if (named)
		width->keyword = CSS_BACKGROUND_SIZE_CONTAIN;
	named = css_ident_equal(parts[0], "cover");
	if (named)
		width->keyword = CSS_BACKGROUND_SIZE_COVER;
	if (width->keyword != CSS_UNIT_AUTO && count != 1)
		return EINVAL;
	if (width->keyword != CSS_UNIT_AUTO)
		return 0;

	/* The width: a length, a percentage or auto. */
	error = values_length(parts[0], 1, width);
	if (error != 0)
		return error;
	if (count == 1)
		return 0;

	/* The height likewise. */
	error = values_length(parts[1], 1, height);
	if (error != 0)
		return error;

	/* Succeeded: both sides. */
	return 0;
}

/* Parses background-position into its two longhands. */
static int
values_background_position(
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	const struct css_token *parts[2];
	struct css_value x;
	struct css_value y;
	size_t found;
	size_t index;
	int error;

	/* Its parts: at most two tokens. */
	found = 0;
	for (index = 0; index < count; index++) {
		if (tokens[index].type == CSS_TOKEN_WHITESPACE)
			continue;
		if (found == 2)
			return EINVAL;
		parts[found] = &tokens[index];
		found++;
	}

	/* The two axes. */
	error = values_position_parts(parts, found, &x, &y);
	if (error != 0)
		return error;
	values_add(out, made, CSS_PROP_BACKGROUND_POSITION_X, &x);
	values_add(out, made, CSS_PROP_BACKGROUND_POSITION_Y, &y);

	/* Succeeded: both declared. */
	return 0;
}

/* Parses background-size into its two longhands. */
static int
values_background_size(
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	const struct css_token *parts[2];
	struct css_value width;
	struct css_value height;
	size_t found;
	size_t index;
	int error;

	/* Its parts: at most two tokens. */
	found = 0;
	for (index = 0; index < count; index++) {
		if (tokens[index].type == CSS_TOKEN_WHITESPACE)
			continue;
		if (found == 2)
			return EINVAL;
		parts[found] = &tokens[index];
		found++;
	}

	/* The two sides. */
	error = values_size_parts(parts, found, &width, &height);
	if (error != 0)
		return error;
	values_add(out, made, CSS_PROP_BACKGROUND_SIZE_WIDTH, &width);
	values_add(out, made, CSS_PROP_BACKGROUND_SIZE_HEIGHT, &height);

	/* Succeeded: both declared. */
	return 0;
}

/* Tells whether a longhand takes a length, and so may take a calculation. */
static int
values_takes_length(
	int property)
{
	/* The sizes, the four-sided lengths, the offsets and the font's lengths. */
	switch (property) {
	case CSS_PROP_RADIUS_TOP_LEFT_X:
	case CSS_PROP_RADIUS_TOP_LEFT_Y:
	case CSS_PROP_RADIUS_TOP_RIGHT_X:
	case CSS_PROP_RADIUS_TOP_RIGHT_Y:
	case CSS_PROP_RADIUS_BOTTOM_RIGHT_X:
	case CSS_PROP_RADIUS_BOTTOM_RIGHT_Y:
	case CSS_PROP_RADIUS_BOTTOM_LEFT_X:
	case CSS_PROP_RADIUS_BOTTOM_LEFT_Y:
	case CSS_PROP_WIDTH:
	case CSS_PROP_HEIGHT:
	case CSS_PROP_MIN_WIDTH:
	case CSS_PROP_MAX_WIDTH:
	case CSS_PROP_MIN_HEIGHT:
	case CSS_PROP_MAX_HEIGHT:
	case CSS_PROP_MARGIN_TOP:
	case CSS_PROP_MARGIN_RIGHT:
	case CSS_PROP_MARGIN_BOTTOM:
	case CSS_PROP_MARGIN_LEFT:
	case CSS_PROP_PADDING_TOP:
	case CSS_PROP_PADDING_RIGHT:
	case CSS_PROP_PADDING_BOTTOM:
	case CSS_PROP_PADDING_LEFT:
	case CSS_PROP_BORDER_TOP_WIDTH:
	case CSS_PROP_BORDER_RIGHT_WIDTH:
	case CSS_PROP_BORDER_BOTTOM_WIDTH:
	case CSS_PROP_BORDER_LEFT_WIDTH:
	case CSS_PROP_TOP:
	case CSS_PROP_RIGHT:
	case CSS_PROP_BOTTOM:
	case CSS_PROP_LEFT:
	case CSS_PROP_FONT_SIZE:
	case CSS_PROP_LINE_HEIGHT:
	case CSS_PROP_TEXT_INDENT:
	case CSS_PROP_ROW_GAP:
	case CSS_PROP_COLUMN_GAP:
	case CSS_PROP_FLEX_BASIS:
	case CSS_PROP_BACKGROUND_SIZE_WIDTH:
	case CSS_PROP_BACKGROUND_SIZE_HEIGHT:
		return 1;
	default:
		return 0;
	}
}

/*
 * Parses calc(), min(), max() or clamp() (the whole value) into a
 * calculation in the parse's arena: a length whose unit is
 * CSS_DUNIT_CALC, or for line-height a plain number.
 */
static int
values_calc(
	struct css_parse *parse,
	int property,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	struct css_calc calc;
	struct css_calc *kept;
	size_t inner;
	size_t used;
	size_t after;
	int operation;
	int same;
	int error;

	/* The function's name picks what it does. */
	operation = -1;
	same = css_ident_equal(&tokens[0], "calc");
	if (!same)
		same = css_ident_equal(&tokens[0], "-webkit-calc");
	if (same)
		operation = CSS_CALC_SUM;
	same = css_ident_equal(&tokens[0], "min");
	if (same)
		operation = CSS_CALC_MIN;
	same = css_ident_equal(&tokens[0], "max");
	if (same)
		operation = CSS_CALC_MAX;
	same = css_ident_equal(&tokens[0], "clamp");
	if (same)
		operation = CSS_CALC_CLAMP;
	if (operation < 0 || count < 2 || tokens[count - 1U].type != CSS_TOKEN_CLOSE_PAREN)
		return EINVAL;

	/* calc() around one min(), max() or clamp() is that function. */
	inner = values_skip_space(tokens, count - 1U, 1);
	if (operation == CSS_CALC_SUM && inner < count - 1U && tokens[inner].type == CSS_TOKEN_FUNCTION) {
		used = values_function_length(tokens + inner, count - 1U - inner);
		after = values_skip_space(tokens, count - 1U, inner + used);
		if (after == count - 1U) {
			error = values_calc(parse, property, tokens + inner, used, value);
			if (error == 0)
				return 0;
		}
	}

	/* The arguments, as sums. */
	memset(&calc, 0, sizeof(calc));
	error = values_calc_arguments(parse->arena, tokens + 1, count - 2U, operation, &calc);
	if (error != 0)
		return error;

	/* A calculation of plain numbers is a number: line-height's, or zero for a length. */
	if (calc.count == 0) {
		if (property == CSS_PROP_LINE_HEIGHT) {
			value->kind = CSS_VALUE_NUMBER;
			value->number = calc.sums[0].px;
			return 0;
		}

		/* Only zero is a length without a unit. */
		if (calc.sums[0].px != 0)
			return EINVAL;
		value->kind = CSS_VALUE_LENGTH;
		value->number = 0;
		value->unit = CSS_DUNIT_PX;
		return 0;
	}

	/* The calculation is kept in the arena. */
	kept = wb_arena_alloc(parse->arena, sizeof(*kept));
	if (kept == NULL)
		return ENOMEM;
	*kept = calc;

	/* Succeeded: a calculated length. */
	value->kind = CSS_VALUE_LENGTH;
	value->unit = CSS_DUNIT_CALC;
	value->number = 0;
	value->calc = kept;
	return 0;
}

/*
 * Reads a calculation's arguments (the tokens inside its parentheses):
 * one sum for calc(), one to three separated by commas for min() and
 * max(), three for clamp().  A calculation of plain numbers leaves count
 * zero with the number in the first sum's px.
 */
static int
values_calc_arguments(
	struct wb_arena *arena,
	const struct css_token *tokens,
	size_t count,
	int operation,
	struct css_calc *calc)
{
	struct values_term term;
	size_t index;
	size_t made;
	int numbers;
	int error;

	/* Each argument in turn. */
	calc->operation = operation;
	index = 0;
	made = 0;
	numbers = 0;
	for (;;) {
		/* Too many arguments spoil the calculation. */
		if (made >= CSS_CALC_ARGUMENTS)
			return EINVAL;

		/* The argument's sum. */
		error = values_calc_sum(arena, tokens, count, &index, &term);
		if (error != 0)
			return error;

		/* A plain number, or a sum of lengths. */
		if (term.is_number) {
			numbers++;
			memset(&calc->sums[made], 0, sizeof(calc->sums[made]));
			calc->sums[made].px = term.number;
		} else {
			calc->sums[made] = term.sum;
		}

		/* One more argument. */
		made++;

		/* A comma before the next argument, or the end. */
		index = values_skip_space(tokens, count, index);
		if (index >= count)
			break;
		if (tokens[index].type != CSS_TOKEN_COMMA || operation == CSS_CALC_SUM)
			return EINVAL;
		index++;
	}

	/* clamp() takes three; numbers and lengths are not mixed. */
	if (operation == CSS_CALC_CLAMP && made != 3)
		return EINVAL;
	if (numbers != 0 && numbers != (int)made)
		return EINVAL;

	/* A calculation of numbers alone is reported as its number. */
	calc->count = made;
	if (numbers != 0) {
		if (made != 1)
			return EINVAL;
		calc->count = 0;
	}

	/* Succeeded: the arguments are read. */
	return 0;
}

/* Reads a sum: products joined by + and -. */
static int
values_calc_sum(
	struct wb_arena *arena,
	const struct css_token *tokens,
	size_t count,
	size_t *index,
	struct values_term *term)
{
	struct values_term right;
	float sign;
	int error;

	/* The first product. */
	error = values_calc_product(arena, tokens, count, index, term);
	if (error != 0)
		return error;

	/* Then + or - and another product, while they follow. */
	for (;;) {
		*index = values_skip_space(tokens, count, *index);
		if (*index >= count || tokens[*index].type != CSS_TOKEN_DELIM)
			break;
		if (tokens[*index].delim != '+' && tokens[*index].delim != '-')
			break;

		/* The operator's sign, then the product after it. */
		sign = 1.0f;
		if (tokens[*index].delim == '-')
			sign = -1.0f;
		(*index)++;
		error = values_calc_product(arena, tokens, count, index, &right);
		if (error != 0)
			return error;

		/* Numbers add to numbers. */
		if (term->is_number && right.is_number) {
			term->number += sign * right.number;
			continue;
		}

		/* A zero number counts as a length; any other number with a length spoils the sum. */
		if (term->is_number) {
			if (term->number != 0)
				return EINVAL;
			memset(&term->sum, 0, sizeof(term->sum));
			term->is_number = 0;
		}

		/* The same for the right side. */
		if (right.is_number) {
			if (right.number != 0)
				return EINVAL;
			continue;
		}

		/* Two lengths (with one min(), max() or clamp() between them at most). */
		error = values_calc_add(&term->sum, &right.sum, sign);
		if (error != 0)
			return error;
	}

	/* Succeeded: the sum is read. */
	return 0;
}

/* Reads a product: values joined by * and /, one side of each a number. */
static int
values_calc_product(
	struct wb_arena *arena,
	const struct css_token *tokens,
	size_t count,
	size_t *index,
	struct values_term *term)
{
	struct values_term right;
	int divide;
	int error;

	/* The first value. */
	error = values_calc_value(arena, tokens, count, index, term);
	if (error != 0)
		return error;

	/* Then * or / and another value, while they follow. */
	for (;;) {
		*index = values_skip_space(tokens, count, *index);
		if (*index >= count || tokens[*index].type != CSS_TOKEN_DELIM)
			break;
		if (tokens[*index].delim != '*' && tokens[*index].delim != '/')
			break;

		/* The operator, then the value after it. */
		divide = 0;
		if (tokens[*index].delim == '/')
			divide = 1;
		(*index)++;
		error = values_calc_value(arena, tokens, count, index, &right);
		if (error != 0)
			return error;

		/* Division is by a number that is not zero. */
		if (divide) {
			if (!right.is_number || right.number == 0)
				return EINVAL;
			if (term->is_number) {
				term->number /= right.number;
			} else {
				values_calc_scale(&term->sum, 1.0f / right.number);
			}

			/* The quotient is the term now. */
			continue;
		}

		/* Multiplication has a number on one side at least. */
		if (term->is_number && right.is_number) {
			term->number *= right.number;
		} else if (term->is_number) {
			values_calc_scale(&right.sum, term->number);
			*term = right;
		} else if (right.is_number) {
			values_calc_scale(&term->sum, right.number);
		} else {
			return EINVAL;
		}
	}

	/* Succeeded: the product is read. */
	return 0;
}

/* Reads one value: a number, a percentage, a length, or a sum in parentheses or calc(). */
static int
values_calc_value(
	struct wb_arena *arena,
	const struct css_token *tokens,
	size_t count,
	size_t *index,
	struct values_term *term)
{
	const struct css_token *token;
	struct css_calc *nested;
	size_t used;
	int operation;
	int is_calc;
	int error;

	/* The token past whitespace. */
	memset(term, 0, sizeof(*term));
	*index = values_skip_space(tokens, count, *index);
	if (*index >= count)
		return EINVAL;
	token = &tokens[*index];

	/* A plain number. */
	if (token->type == CSS_TOKEN_NUMBER) {
		term->is_number = 1;
		term->number = (float)token->number;
		(*index)++;
		return 0;
	}

	/* A percentage. */
	if (token->type == CSS_TOKEN_PERCENTAGE) {
		term->sum.percent = (float)token->number;
		(*index)++;
		return 0;
	}

	/* A length. */
	if (token->type == CSS_TOKEN_DIMENSION) {
		error = values_calc_unit(token, &term->sum);
		if (error != 0)
			return error;
		(*index)++;
		return 0;
	}

	/* A min(), max() or clamp() inside the sum (ws074-p074): its arguments, kept in the arena, times one. */
	operation = -1;
	if (token->type == CSS_TOKEN_FUNCTION) {
		is_calc = css_ident_equal(token, "min");
		if (is_calc)
			operation = CSS_CALC_MIN;
		is_calc = css_ident_equal(token, "max");
		if (is_calc)
			operation = CSS_CALC_MAX;
		is_calc = css_ident_equal(token, "clamp");
		if (is_calc)
			operation = CSS_CALC_CLAMP;
	}

	/* The function's arguments, as a calculation of its own. */
	if (operation >= 0) {
		used = values_function_length(tokens + *index, count - *index);
		if (used < 2U || tokens[*index + used - 1U].type != CSS_TOKEN_CLOSE_PAREN)
			return EINVAL;
		nested = wb_arena_zalloc(arena, sizeof(*nested));
		if (nested == NULL)
			return ENOMEM;
		error = values_calc_arguments(arena, tokens + *index + 1U, used - 2U, operation, nested);
		if (error != 0)
			return error;
		if (nested->count == 0)
			return EINVAL;
		term->sum.nested = nested;
		term->sum.nested_factor = 1.0f;
		*index += used;
		return 0;
	}

	/* A sum in parentheses, or in a nested calc(). */
	if (token->type == CSS_TOKEN_FUNCTION) {
		is_calc = css_ident_equal(token, "calc");
		if (!is_calc)
			return EINVAL;
	} else if (token->type != CSS_TOKEN_OPEN_PAREN) {
		return EINVAL;
	}

	/* The sum inside, then the closing parenthesis. */
	(*index)++;
	error = values_calc_sum(arena, tokens, count, index, term);
	if (error != 0)
		return error;
	*index = values_skip_space(tokens, count, *index);
	if (*index >= count || tokens[*index].type != CSS_TOKEN_CLOSE_PAREN)
		return EINVAL;
	(*index)++;

	/* Succeeded: the value is read. */
	return 0;
}

/* Adds a dimension to a sum in the field of its unit (the absolute units as pixels). */
static int
values_calc_unit(
	const struct css_token *token,
	struct css_calc_sum *sum)
{
	float number;
	int unit;
	int found;

	/* The unit must be a length unit. */
	found = values_unit(token, &unit);
	if (!found)
		return EINVAL;
	number = (float)token->number;

	/* Its field. */
	switch (unit) {
	case CSS_DUNIT_PX:
		sum->px += number;
		break;
	case CSS_DUNIT_PT:
		sum->px += number * 96.0f / 72.0f;
		break;
	case CSS_DUNIT_PC:
		sum->px += number * 16.0f;
		break;
	case CSS_DUNIT_IN:
		sum->px += number * 96.0f;
		break;
	case CSS_DUNIT_CM:
		sum->px += number * 96.0f / 2.54f;
		break;
	case CSS_DUNIT_MM:
		sum->px += number * 96.0f / 25.4f;
		break;
	case CSS_DUNIT_EM:
		sum->em += number;
		break;
	case CSS_DUNIT_EX:
		sum->ex += number;
		break;
	case CSS_DUNIT_REM:
		sum->rem += number;
		break;
	case CSS_DUNIT_VW:
		sum->vw += number;
		break;
	case CSS_DUNIT_VH:
		sum->vh += number;
		break;
	case CSS_DUNIT_VMIN:
		sum->vmin += number;
		break;
	case CSS_DUNIT_VMAX:
		sum->vmax += number;
		break;
	case CSS_DUNIT_CQW:
		sum->cqw += number;
		break;
	case CSS_DUNIT_CQH:
		sum->cqh += number;
		break;
	default:
		return EINVAL;
	}

	/* Succeeded: the length is in the sum. */
	return 0;
}

/* Multiplies every field of a sum by a factor. */
static void
values_calc_scale(
	struct css_calc_sum *sum,
	float factor)
{
	/* Each kind of length. */
	sum->px *= factor;
	sum->percent *= factor;
	sum->em *= factor;
	sum->ex *= factor;
	sum->rem *= factor;
	sum->vw *= factor;
	sum->vh *= factor;
	sum->vmin *= factor;
	sum->vmax *= factor;
	sum->cqw *= factor;
	sum->cqh *= factor;

	/* And the function's factor. */
	sum->nested_factor *= factor;
}

/*
 * Adds (sign 1) or subtracts (sign -1) one sum to or from another; two
 * sums that each hold a min(), max() or clamp() are not added in this
 * pass (EINVAL).
 */
static int
values_calc_add(
	struct css_calc_sum *sum,
	const struct css_calc_sum *other,
	float sign)
{
	/* The other's function, when it has one, becomes the sum's. */
	if (other->nested != NULL) {
		if (sum->nested != NULL)
			return EINVAL;
		sum->nested = other->nested;
		sum->nested_factor = sign * other->nested_factor;
	}

	/* Each kind of length. */
	sum->px += sign * other->px;
	sum->percent += sign * other->percent;
	sum->em += sign * other->em;
	sum->ex += sign * other->ex;
	sum->rem += sign * other->rem;
	sum->vw += sign * other->vw;
	sum->vh += sign * other->vh;
	sum->vmin += sign * other->vmin;
	sum->vmax += sign * other->vmax;
	sum->cqw += sign * other->cqw;
	sum->cqh += sign * other->cqh;

	/* Succeeded: the sums are added. */
	return 0;
}

/* Skips whitespace tokens from index; returns the first other index (or count). */
static size_t
values_skip_space(
	const struct css_token *tokens,
	size_t count,
	size_t index)
{
	/* Moves past the whitespace. */
	while (index < count && tokens[index].type == CSS_TOKEN_WHITESPACE)
		index++;

	/* Reports where the rest starts. */
	return index;
}

/* Parses a two-sided logical shorthand (margin-inline and the like): one value for both sides, or one each. */
static int
values_pair(
	struct css_parse *parse,
	int first,
	int second,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value values[2];
	size_t starts[3];
	size_t lengths[3];
	size_t components;
	size_t index;
	int error;

	/* One or two components. */
	components = values_components(tokens, count, starts, lengths, 3);
	if (components == 0 || components > 2)
		return EINVAL;
	for (index = 0; index < components; index++) {
		error = values_single(parse, first, tokens + starts[index], lengths[index], &values[index]);
		if (error != 0)
			return error;
	}

	/* The second side copies the first when it is missing. */
	if (components < 2)
		values[1] = values[0];
	values_add(out, made, first, &values[0]);
	values_add(out, made, second, &values[1]);

	/* Succeeded: two longhands. */
	return 0;
}

/*
 * Parses content: none or normal (no box), or a list of strings and
 * attr(name) items, kept in the parse's arena (counters, quotes and images
 * add nothing in this pass; the alternative text after a slash is not
 * drawn).
 */
static int
values_content(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	struct css_content_item items[VALUES_CONTENT_ITEMS];
	struct css_content_item *kept_items;
	struct css_content *kept;
	size_t made;
	size_t index;
	size_t name;
	size_t used;
	int is_word;
	int is_attribute;

	/* none and normal. */
	if (count == 1 && tokens[0].type == CSS_TOKEN_IDENT) {
		is_word = css_ident_equal(&tokens[0], "none");
		if (!is_word)
			is_word = css_ident_equal(&tokens[0], "normal");
		if (!is_word)
			return EINVAL;
		value->kind = CSS_VALUE_KEYWORD;
		value->keyword = 0;
		return 0;
	}

	/* The strings and attr() items in order (up to the room there is). */
	made = 0;
	index = 0;
	while (index < count && made < VALUES_CONTENT_ITEMS) {
		if (tokens[index].type == CSS_TOKEN_DELIM && tokens[index].delim == '/')
			break;

		/* A string. */
		if (tokens[index].type == CSS_TOKEN_STRING) {
			items[made].is_attribute = 0;
			items[made].text = vm_atom_from_units(parse->heap, tokens[index].text, tokens[index].length);
			if (items[made].text == NULL)
				return ENOMEM;
			made++;
		}

		/* attr(name). */
		is_attribute = 0;
		if (tokens[index].type == CSS_TOKEN_FUNCTION)
			is_attribute = css_ident_equal(&tokens[index], "attr");
		used = values_function_length(tokens + index, count - index);
		if (is_attribute) {
			name = values_skip_space(tokens, index + used, index + 1U);
			if (name < index + used && tokens[name].type == CSS_TOKEN_IDENT) {
				items[made].is_attribute = 1;
				items[made].text = vm_atom_from_units(parse->heap, tokens[name].text, tokens[name].length);
				if (items[made].text == NULL)
					return ENOMEM;
				made++;
			}
		}

		/* The next item (a function's arguments with it). */
		index += used;
	}

	/* The list, kept in the arena. */
	kept = wb_arena_zalloc(parse->arena, sizeof(*kept));
	if (kept == NULL)
		return ENOMEM;
	kept_items = wb_arena_alloc(parse->arena, made * sizeof(*kept_items) + 1U);
	if (kept_items == NULL)
		return ENOMEM;
	memcpy(kept_items, items, made * sizeof(*kept_items));
	kept->items = kept_items;
	kept->count = made;

	/* Succeeded: the content is a list. */
	value->kind = CSS_VALUE_CONTENT;
	value->content = kept;
	return 0;
}

/*
 * Parses the flex shorthand: none (0 0 auto), auto (1 1 auto), or a grow
 * factor, an optional shrink factor and an optional basis in any order
 * the grammar allows (a number alone makes the basis 0%).
 */
static int
values_flex(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value grow;
	struct css_value shrink;
	struct css_value basis;
	struct css_value candidate;
	size_t starts[4];
	size_t lengths[4];
	size_t components;
	size_t index;
	int numbers;
	int is_word;
	int error;

	/* The defaults of a flex value that names some parts: 1 1 0%. */
	memset(&grow, 0, sizeof(grow));
	grow.kind = CSS_VALUE_NUMBER;
	grow.number = 1;
	shrink = grow;
	memset(&basis, 0, sizeof(basis));
	basis.kind = CSS_VALUE_LENGTH;
	basis.number = 0;
	basis.unit = CSS_DUNIT_PERCENT;

	/* none and auto. */
	components = values_components(tokens, count, starts, lengths, 4);
	if (components == 1 && tokens[starts[0]].type == CSS_TOKEN_IDENT) {
		is_word = css_ident_equal(&tokens[starts[0]], "none");
		if (is_word) {
			grow.number = 0;
			shrink.number = 0;
		} else {
			is_word = css_ident_equal(&tokens[starts[0]], "auto");
			if (!is_word)
				return EINVAL;
		}

		/* Both keep an auto basis. */
		basis.kind = CSS_VALUE_KEYWORD;
		basis.keyword = CSS_UNIT_AUTO;
		values_add(out, made, CSS_PROP_FLEX_GROW, &grow);
		values_add(out, made, CSS_PROP_FLEX_SHRINK, &shrink);
		values_add(out, made, CSS_PROP_FLEX_BASIS, &basis);
		return 0;
	}

	/* One to three parts: numbers are the factors in order, anything else the basis. */
	if (components == 0 || components > 3)
		return EINVAL;
	numbers = 0;
	for (index = 0; index < components; index++) {
		if (lengths[index] == 1 && tokens[starts[index]].type == CSS_TOKEN_NUMBER && tokens[starts[index]].number != 0) {
			if (numbers == 0)
				grow.number = (float)tokens[starts[index]].number;
			else if (numbers == 1)
				shrink.number = (float)tokens[starts[index]].number;
			else
				return EINVAL;
			numbers++;
			continue;
		}

		/* A zero is a factor where one is expected, else a basis. */
		if (lengths[index] == 1 && tokens[starts[index]].type == CSS_TOKEN_NUMBER && numbers < 2 && (numbers == 0 || index == 1)) {
			if (numbers == 0)
				grow.number = 0;
			else
				shrink.number = 0;
			numbers++;
			continue;
		}

		/* The basis. */
		error = values_single(parse, CSS_PROP_FLEX_BASIS, tokens + starts[index], lengths[index], &candidate);
		if (error != 0)
			return error;
		basis = candidate;
	}

	/* The three longhands. */
	values_add(out, made, CSS_PROP_FLEX_GROW, &grow);
	values_add(out, made, CSS_PROP_FLEX_SHRINK, &shrink);
	values_add(out, made, CSS_PROP_FLEX_BASIS, &basis);

	/* Succeeded: the flex longhands are declared. */
	return 0;
}

/* Parses flex-flow: a direction and a wrap, in either order, each optional. */
static int
values_flex_flow(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value direction;
	struct css_value wrap;
	struct css_value candidate;
	size_t starts[3];
	size_t lengths[3];
	size_t components;
	size_t index;
	int error;

	/* The defaults: row, nowrap. */
	memset(&direction, 0, sizeof(direction));
	direction.kind = CSS_VALUE_KEYWORD;
	direction.keyword = CSS_FLEX_ROW;
	wrap = direction;
	wrap.keyword = CSS_FLEX_NOWRAP;

	/* Each part is one or the other. */
	components = values_components(tokens, count, starts, lengths, 3);
	if (components == 0 || components > 2)
		return EINVAL;
	for (index = 0; index < components; index++) {
		error = values_single(parse, CSS_PROP_FLEX_DIRECTION, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			direction = candidate;
			continue;
		}

		/* Or a wrap. */
		error = values_single(parse, CSS_PROP_FLEX_WRAP, tokens + starts[index], lengths[index], &candidate);
		if (error != 0)
			return error;
		wrap = candidate;
	}

	/* The two longhands. */
	values_add(out, made, CSS_PROP_FLEX_DIRECTION, &direction);
	values_add(out, made, CSS_PROP_FLEX_WRAP, &wrap);

	/* Succeeded: both declared. */
	return 0;
}

/*
 * Parses border-radius (ws074-p062): one to four horizontal radii (top
 * left, top right, bottom right, bottom left, the missing ones copied as
 * the four-sided shorthands copy them), then optionally a slash and one to
 * four vertical radii (otherwise the same as the horizontal ones).
 */
static int
values_border_radius(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value horizontal[4];
	struct css_value vertical[4];
	size_t slash;
	size_t corner;
	int error;

	/* The slash between the two lists, if there is one. */
	slash = count;
	for (corner = 0; corner < count; corner++) {
		if (tokens[corner].type == CSS_TOKEN_DELIM && tokens[corner].delim == '/') {
			slash = corner;
			break;
		}
	}

	/* The horizontal radii. */
	error = values_radii(parse, tokens, slash, horizontal);
	if (error != 0)
		return error;

	/* The vertical radii, or the horizontal ones again. */
	if (slash < count) {
		error = values_radii(parse, tokens + slash + 1U, count - slash - 1U, vertical);
		if (error != 0)
			return error;
	} else {
		memcpy(vertical, horizontal, sizeof(vertical));
	}

	/* Each corner's two longhands. */
	for (corner = 0; corner < 4U; corner++) {
		values_add(out, made, CSS_PROP_RADIUS_TOP_LEFT_X + 2 * (int)corner, &horizontal[corner]);
		values_add(out, made, CSS_PROP_RADIUS_TOP_LEFT_Y + 2 * (int)corner, &vertical[corner]);
	}

	/* Succeeded: eight longhands. */
	return 0;
}

/* Parses one to four radii into the four corners, the missing ones copied from their opposites. */
static int
values_radii(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *radii)
{
	size_t starts[5];
	size_t lengths[5];
	size_t components;
	size_t index;
	int error;

	/* One to four components, each a radius. */
	components = values_components(tokens, count, starts, lengths, 5);
	if (components == 0 || components > 4)
		return EINVAL;
	for (index = 0; index < components; index++) {
		error = values_single(parse, CSS_PROP_RADIUS_TOP_LEFT_X, tokens + starts[index], lengths[index], &radii[index]);
		if (error != 0)
			return error;
	}

	/* The missing ones, as margin copies its sides. */
	if (components < 2)
		radii[1] = radii[0];
	if (components < 3)
		radii[2] = radii[0];
	if (components < 4)
		radii[3] = radii[1];

	/* Succeeded: four radii. */
	return 0;
}

/* Parses one corner's radius: a horizontal radius and optionally a vertical one (otherwise the same). */
static int
values_corner_radius(
	struct css_parse *parse,
	int corner,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	/* The pair of longhands of the corner. */
	return values_pair(parse, CSS_PROP_RADIUS_TOP_LEFT_X + 2 * corner, CSS_PROP_RADIUS_TOP_LEFT_Y + 2 * corner, tokens, count, out, made);
}

/*
 * Parses box-shadow (ws074-p062): none, or a comma-separated list of
 * shadows (the first CSS_SHADOWS of them are kept), in the parse's arena.
 */
static int
values_box_shadow(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	struct css_shadow_list *list;
	size_t start;
	size_t index;
	size_t used;
	int is_none;
	int error;

	/* The list, kept in the arena. */
	list = wb_arena_zalloc(parse->arena, sizeof(*list));
	if (list == NULL)
		return ENOMEM;
	value->kind = CSS_VALUE_SHADOWS;
	value->shadows = list;

	/* none is the empty list. */
	if (count == 1 && tokens[0].type == CSS_TOKEN_IDENT) {
		is_none = css_ident_equal(&tokens[0], "none");
		if (!is_none)
			return EINVAL;
		return 0;
	}

	/* Each shadow runs to a comma outside the functions. */
	start = 0;
	index = 0;
	while (index <= count) {
		if (index < count && tokens[index].type != CSS_TOKEN_COMMA) {
			used = 1;
			if (tokens[index].type == CSS_TOKEN_FUNCTION)
				used = values_function_length(tokens + index, count - index);
			index += used;
			continue;
		}

		/* One shadow from start to here, kept while there is room. */
		if (list->count < CSS_SHADOWS) {
			error = values_one_shadow(tokens + start, index - start, &list->shadows[list->count]);
			if (error != 0)
				return error;
			list->count++;
		}

		/* The next shadow starts after the comma. */
		index++;
		start = index;
	}

	/* Succeeded: the shadows are parsed. */
	return 0;
}

/* Parses one shadow: inset, two to four lengths and a color, the keyword and the color at either end. */
static int
values_one_shadow(
	const struct css_token *tokens,
	size_t count,
	struct css_declared_shadow *shadow)
{
	struct css_value candidate;
	size_t starts[7];
	size_t lengths[7];
	size_t components;
	size_t index;
	size_t lengths_seen;
	int is_inset;
	int error;

	/* No shadow is the default: black, all lengths zero pixels. */
	memset(shadow, 0, sizeof(*shadow));
	shadow->color = CSS_CURRENT_COLOR;
	for (index = 0; index < 4U; index++) {
		shadow->lengths[index].kind = CSS_VALUE_LENGTH;
		shadow->lengths[index].unit = CSS_DUNIT_PX;
	}

	/* Each component is inset, a length or a color. */
	components = values_components(tokens, count, starts, lengths, 7);
	if (components < 2 || components > 6)
		return EINVAL;
	lengths_seen = 0;
	for (index = 0; index < components; index++) {
		is_inset = 0;
		if (lengths[index] == 1 && tokens[starts[index]].type == CSS_TOKEN_IDENT)
			is_inset = css_ident_equal(&tokens[starts[index]], "inset");
		if (is_inset) {
			shadow->inset = 1;
			continue;
		}

		/* A length (four at most). */
		memset(&candidate, 0, sizeof(candidate));
		error = EINVAL;
		if (lengths[index] == 1 && lengths_seen < 4U)
			error = values_length(&tokens[starts[index]], 0, &candidate);
		if (error == 0) {
			shadow->lengths[lengths_seen] = candidate;
			lengths_seen++;
			continue;
		}

		/* Otherwise a color. */
		error = css_parse_color(tokens + starts[index], lengths[index], &shadow->color);
		if (error != 0)
			return error;
	}

	/* The two offsets are required. */
	if (lengths_seen < 2U)
		return EINVAL;

	/* Succeeded: the shadow is parsed. */
	return 0;
}

/* Parses the outline shorthand: a width, a style and a color in any order (medium, none and currentcolor when missing). */
static int
values_outline(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value width;
	struct css_value style;
	struct css_value color;
	struct css_value candidate;
	size_t starts[4];
	size_t lengths[4];
	size_t components;
	size_t index;
	int error;

	/* The defaults: medium, none, currentcolor. */
	memset(&width, 0, sizeof(width));
	width.kind = CSS_VALUE_LENGTH;
	width.number = 3;
	width.unit = CSS_DUNIT_PX;
	memset(&style, 0, sizeof(style));
	style.kind = CSS_VALUE_KEYWORD;
	style.keyword = CSS_BORDER_NONE;
	memset(&color, 0, sizeof(color));
	color.kind = CSS_VALUE_COLOR;
	color.color = CSS_CURRENT_COLOR;

	/* Each component is a style, a width or a color. */
	components = values_components(tokens, count, starts, lengths, 4);
	if (components == 0 || components > 3)
		return EINVAL;
	for (index = 0; index < components; index++) {
		error = values_single(parse, CSS_PROP_OUTLINE_STYLE, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			style = candidate;
			continue;
		}

		/* Or a width. */
		error = values_single(parse, CSS_PROP_OUTLINE_WIDTH, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			width = candidate;
			continue;
		}

		/* Or a color. */
		error = values_single(parse, CSS_PROP_OUTLINE_COLOR, tokens + starts[index], lengths[index], &candidate);
		if (error == 0) {
			color = candidate;
			continue;
		}

		/* Anything else spoils the shorthand. */
		return EINVAL;
	}

	/* The three longhands. */
	values_add(out, made, CSS_PROP_OUTLINE_WIDTH, &width);
	values_add(out, made, CSS_PROP_OUTLINE_STYLE, &style);
	values_add(out, made, CSS_PROP_OUTLINE_COLOR, &color);

	/* Succeeded: the longhands are declared. */
	return 0;
}

/*
 * Parses clip-path (ws074-p062): inset() with one to four lengths (the
 * missing ones copied as margin copies its sides; a "round" and its radii
 * after them are not kept) as an inset in the parse's arena; none, and the
 * shapes this pass does not clip by, as the keyword 0.
 */
static int
values_clip_path(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	struct css_declared_inset *inset;
	size_t starts[5];
	size_t lengths[5];
	size_t components;
	size_t used;
	size_t index;
	int is_inset;
	int is_round;
	int error;

	/* Anything but inset() clips nothing in this pass. */
	value->kind = CSS_VALUE_KEYWORD;
	value->keyword = 0;
	is_inset = 0;
	if (tokens[0].type == CSS_TOKEN_FUNCTION)
		is_inset = css_ident_equal(&tokens[0], "inset");
	if (!is_inset)
		return 0;

	/* The arguments, between the function's name and its closing parenthesis. */
	used = values_function_length(tokens, count);
	if (used < 2U)
		return EINVAL;
	components = values_components(tokens + 1, used - 2U, starts, lengths, 5);

	/* The inset, kept in the arena, zero pixels on every side to start with. */
	inset = wb_arena_zalloc(parse->arena, sizeof(*inset));
	if (inset == NULL)
		return ENOMEM;
	for (index = 0; index < 4U; index++) {
		inset->lengths[index].kind = CSS_VALUE_LENGTH;
		inset->lengths[index].unit = CSS_DUNIT_PX;
	}

	/* Up to four lengths, until round. */
	for (index = 0; index < components && index < 4U; index++) {
		is_round = 0;
		if (tokens[1U + starts[index]].type == CSS_TOKEN_IDENT)
			is_round = css_ident_equal(&tokens[1U + starts[index]], "round");
		if (is_round)
			break;
		if (lengths[index] != 1)
			return EINVAL;
		error = values_length(&tokens[1U + starts[index]], 0, &inset->lengths[index]);
		if (error != 0)
			return error;
	}

	/* No length at all is no inset. */
	if (index == 0)
		return EINVAL;

	/* The missing ones, as margin copies its sides. */
	if (index < 2U)
		inset->lengths[1] = inset->lengths[0];
	if (index < 3U)
		inset->lengths[2] = inset->lengths[0];
	if (index < 4U)
		inset->lengths[3] = inset->lengths[1];

	/* Succeeded: the inset is parsed. */
	value->kind = CSS_VALUE_INSET;
	value->inset = inset;
	return 0;
}

/*
 * Parses a grid template (ws074-p072): none, or a list of track sizes
 * (lengths, percentages, fr, auto, min-content and max-content as auto,
 * minmax(), fit-content() as auto, and repeat() with a count), the line
 * names in brackets passed by; kept in the parse's arena.
 */
static int
values_tracks(
	struct css_parse *parse,
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	struct css_track_list *list;
	int is_none;
	int error;

	/* The list, kept in the arena. */
	list = wb_arena_zalloc(parse->arena, sizeof(*list));
	if (list == NULL)
		return ENOMEM;
	value->kind = CSS_VALUE_TRACKS;
	value->tracks = list;

	/* none is the empty list. */
	if (count == 1 && tokens[0].type == CSS_TOKEN_IDENT) {
		is_none = css_ident_equal(&tokens[0], "none");
		if (is_none)
			return 0;
	}

	/* The tracks. */
	error = values_track_list(tokens, count, list, 0);
	if (error != 0)
		return error;

	/* An empty list is not a template. */
	if (list->count == 0)
		return EINVAL;

	/* Succeeded: the template is parsed. */
	return 0;
}

/* Appends the tracks of a track list (repeat() expanded, one level deep) to a list, up to its room. */
static int
values_track_list(
	const struct css_token *tokens,
	size_t count,
	struct css_track_list *list,
	int depth)
{
	struct css_declared_track track;
	size_t index;
	size_t end;
	size_t inner;
	size_t comma;
	size_t times;
	size_t time;
	size_t first;
	size_t added;
	int is_repeat;
	int error;

	/* Walks the list: whitespace, bracketed names, repeat() and tracks. */
	index = 0;
	while (index < count) {
		/* Whitespace between tracks. */
		if (tokens[index].type == CSS_TOKEN_WHITESPACE) {
			index++;
			continue;
		}

		/* Line names in brackets are passed by. */
		if (tokens[index].type == CSS_TOKEN_OPEN_SQUARE) {
			while (index < count && tokens[index].type != CSS_TOKEN_CLOSE_SQUARE)
				index++;
			index++;
			continue;
		}

		/* One component: a function with its arguments, or one token. */
		end = index + 1U;
		if (tokens[index].type == CSS_TOKEN_FUNCTION)
			end = index + values_function_length(tokens + index, count - index);

		/* repeat(count, tracks), expanded (auto-fill and auto-fit are not read in this pass). */
		is_repeat = 0;
		if (tokens[index].type == CSS_TOKEN_FUNCTION)
			is_repeat = css_ident_equal(&tokens[index], "repeat");
		if (is_repeat) {
			if (depth > 0)
				return EINVAL;
			inner = values_skip_space(tokens, end, index + 1U);
			if (inner >= end || tokens[inner].type != CSS_TOKEN_NUMBER || !tokens[inner].integer || tokens[inner].number < 1)
				return EINVAL;
			times = (size_t)tokens[inner].number;
			comma = inner + 1U + values_split_at(tokens + inner + 1U, end - inner - 1U, CSS_TOKEN_COMMA, 0);
			if (comma >= end)
				return EINVAL;

			/* The tracks once, then again until the count (or the room) runs out. */
			first = list->count;
			error = values_track_list(tokens + comma + 1U, end - comma - 2U, list, depth + 1);
			if (error != 0)
				return error;
			added = list->count - first;
			for (time = 1; time < times && added != 0; time++) {
				for (inner = 0; inner < added && list->count < CSS_TRACKS; inner++) {
					list->tracks[list->count] = list->tracks[first + inner];
					list->count++;
				}
			}

			/* On past the function. */
			index = end;
			continue;
		}

		/* One track. */
		error = values_one_track(tokens + index, end - index, &track);
		if (error != 0)
			return error;
		if (list->count < CSS_TRACKS) {
			list->tracks[list->count] = track;
			list->count++;
		}

		/* On past it. */
		index = end;
	}

	/* Succeeded: the tracks are appended. */
	return 0;
}

/* Parses one track: a size, minmax(minimum, size), or fit-content() (as auto). */
static int
values_one_track(
	const struct css_token *tokens,
	size_t count,
	struct css_declared_track *track)
{
	size_t comma;
	size_t start;
	int is_minmax;
	int is_fit;
	int error;

	/* No minimum unless minmax() gives one. */
	memset(track, 0, sizeof(*track));
	track->minimum_kind = CSS_TRACK_AUTO;

	/* A function: minmax() or fit-content(). */
	if (tokens[0].type == CSS_TOKEN_FUNCTION) {
		is_fit = css_ident_equal(&tokens[0], "fit-content");
		if (is_fit) {
			track->kind = CSS_TRACK_AUTO;
			return 0;
		}

		/* Any other function is not a track size. */
		is_minmax = css_ident_equal(&tokens[0], "minmax");
		if (!is_minmax || count < 3U)
			return EINVAL;

		/* The minimum before the comma, the size after it (without the closing parenthesis). */
		comma = 1U + values_split_at(tokens + 1, count - 2U, CSS_TOKEN_COMMA, 0);
		if (comma >= count - 1U)
			return EINVAL;
		start = values_skip_space(tokens, comma, 1U);
		error = values_track_size(tokens + start, comma - start, &track->minimum_kind, &track->minimum);
		if (error != 0)
			return error;
		start = values_skip_space(tokens, count - 1U, comma + 1U);
		error = values_track_size(tokens + start, count - 1U - start, &track->kind, &track->size);
		return error;
	}

	/* A plain size. */
	error = values_track_size(tokens, count, &track->kind, &track->size);
	return error;
}

/* Parses a track size: fr, auto (min-content, max-content), or a length or percentage. */
static int
values_track_size(
	const struct css_token *tokens,
	size_t count,
	int *kind,
	struct css_value *size)
{
	int same;

	/* The size is one token, whitespace after it aside. */
	while (count > 1U && tokens[count - 1U].type == CSS_TOKEN_WHITESPACE)
		count--;
	if (count != 1)
		return EINVAL;
	memset(size, 0, sizeof(*size));

	/* A share of the free space. */
	if (tokens[0].type == CSS_TOKEN_DIMENSION) {
		same = css_ident_equal(&tokens[0], "fr");
		if (same) {
			if (tokens[0].number < 0)
				return EINVAL;
			*kind = CSS_TRACK_FR;
			size->kind = CSS_VALUE_NUMBER;
			size->number = (float)tokens[0].number;
			return 0;
		}
	}

	/* The content's size. */
	if (tokens[0].type == CSS_TOKEN_IDENT) {
		same = css_ident_equal(&tokens[0], "auto");
		if (!same)
			same = css_ident_equal(&tokens[0], "min-content");
		if (!same)
			same = css_ident_equal(&tokens[0], "max-content");
		if (!same)
			return EINVAL;
		*kind = CSS_TRACK_AUTO;
		return 0;
	}

	/* A length or a percentage. */
	*kind = CSS_TRACK_LENGTH;
	return values_length(&tokens[0], 0, size);
}

/* Finds the first token of a type (and a delimiter's character) outside functions and brackets; count when none. */
static size_t
values_split_at(
	const struct css_token *tokens,
	size_t count,
	int type,
	uint32_t delim)
{
	size_t index;
	int depth;

	/* Walks the tokens, keeping count of the open functions and parentheses. */
	depth = 0;
	for (index = 0; index < count; index++) {
		if (tokens[index].type == CSS_TOKEN_FUNCTION || tokens[index].type == CSS_TOKEN_OPEN_PAREN) {
			depth++;
			continue;
		}

		/* A closing parenthesis ends one. */
		if (tokens[index].type == CSS_TOKEN_CLOSE_PAREN) {
			depth--;
			continue;
		}

		/* A token of the type at the top. */
		if (depth == 0 && tokens[index].type == type && (type != CSS_TOKEN_DELIM || tokens[index].delim == delim))
			return index;
	}

	/* None. */
	return count;
}

/*
 * Parses a grid line (ws074-p072): auto, a line number (negative from the
 * end), or span with a count; a named line is taken as auto.
 */
static int
values_grid_line(
	const struct css_token *tokens,
	size_t count,
	struct css_value *value)
{
	size_t index;
	int is_span;
	int is_auto;

	/* auto, and the forms this pass does not read, are auto. */
	value->kind = CSS_VALUE_GRID_LINE;
	value->number = 0;
	value->keyword = 0;
	while (count > 0 && tokens[count - 1U].type == CSS_TOKEN_WHITESPACE)
		count--;
	if (count == 0)
		return EINVAL;
	is_auto = 0;
	if (tokens[0].type == CSS_TOKEN_IDENT)
		is_auto = css_ident_equal(&tokens[0], "auto");
	if (is_auto)
		return 0;

	/* span and a count (one when it has none). */
	is_span = 0;
	if (tokens[0].type == CSS_TOKEN_IDENT)
		is_span = css_ident_equal(&tokens[0], "span");
	if (is_span) {
		value->keyword = 1;
		index = values_skip_space(tokens, count, 1U);
		if (index < count && tokens[index].type == CSS_TOKEN_NUMBER && tokens[index].integer && tokens[index].number >= 1)
			value->keyword = (int)tokens[index].number;
		return 0;
	}

	/* A line number other than zero. */
	if (tokens[0].type == CSS_TOKEN_NUMBER && tokens[0].integer && tokens[0].number != 0) {
		value->number = (float)tokens[0].number;
		return 0;
	}

	/* A named line, taken as auto. */
	return 0;
}

/* Parses grid-column or grid-row: a start line, then optionally a slash and an end line (auto when missing). */
static int
values_grid_pair(
	int start,
	int end,
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	struct css_value first;
	struct css_value second;
	size_t slash;
	size_t index;
	int error;

	/* The start before the slash. */
	slash = values_split_at(tokens, count, CSS_TOKEN_DELIM, '/');
	error = values_grid_line(tokens, slash, &first);
	if (error != 0)
		return error;

	/* The end after it, or auto. */
	memset(&second, 0, sizeof(second));
	second.kind = CSS_VALUE_GRID_LINE;
	if (slash < count) {
		index = values_skip_space(tokens, count, slash + 1U);
		error = values_grid_line(tokens + index, count - index, &second);
		if (error != 0)
			return error;
	}

	/* The two longhands. */
	values_add(out, made, start, &first);
	values_add(out, made, end, &second);

	/* Succeeded: both are declared. */
	return 0;
}

/* Parses grid-area in its line form: row start / column start / row end / column end (the missing ones auto). */
static int
values_grid_area(
	const struct css_token *tokens,
	size_t count,
	struct css_declaration *out,
	size_t *made)
{
	static const int properties[4] = {
		CSS_PROP_GRID_ROW_START, CSS_PROP_GRID_COLUMN_START, CSS_PROP_GRID_ROW_END, CSS_PROP_GRID_COLUMN_END
	};
	struct css_value line;
	size_t start;
	size_t slash;
	size_t part;
	int error;

	/* Each part up to its slash. */
	start = 0;
	for (part = 0; part < 4U; part++) {
		memset(&line, 0, sizeof(line));
		line.kind = CSS_VALUE_GRID_LINE;
		if (start < count) {
			start = values_skip_space(tokens, count, start);
			slash = start + values_split_at(tokens + start, count - start, CSS_TOKEN_DELIM, '/');
			error = values_grid_line(tokens + start, slash - start, &line);
			if (error != 0)
				return error;
			start = slash + 1U;
		}

		/* The part's longhand. */
		values_add(out, made, properties[part], &line);
	}

	/* Succeeded: the four are declared. */
	return 0;
}
