/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * CSS (plan/ws074/design.md §5): style sheets parsed into rules, selectors
 * matched against elements, and the cascade that gives every element its
 * computed style.
 *
 * The first pass covers the common case: type, class, id and attribute
 * selectors with the four combinators, the origins and specificity, the
 * inherited properties, and some thirty properties.  Names are atoms of
 * the document's heap, which live as long as the heap, so the style sheets
 * can keep them without being traced.
 */

#ifndef KEILAND_BROWSER_CSS_H
#define KEILAND_BROWSER_CSS_H

#include "dom/dom.h"

/* The four sides, in the order of the margin and padding shorthands. */
#define CSS_TOP		0
#define CSS_RIGHT	1
#define CSS_BOTTOM	2
#define CSS_LEFT	3

/* How many font families a computed style keeps. */
#define CSS_FAMILIES_MAX	8

/*
 * The units a length can have after the cascade: pixels, a percentage of
 * the containing block (resolved by the layout), or a keyword.
 */
enum css_unit {
	CSS_UNIT_PX,
	CSS_UNIT_PERCENT,
	CSS_UNIT_AUTO,
	CSS_UNIT_NONE,
	CSS_UNIT_NORMAL,
	CSS_UNIT_NUMBER,
	CSS_UNIT_MAX_CONTENT,
	CSS_UNIT_MIN_CONTENT,
	CSS_UNIT_FIT_CONTENT
};

/*
 * A computed length: a value and its unit.  A percentage may carry pixels
 * added to it (offset, from a calc() that mixes the two, ws074-p061);
 * the layout resolves the percentage and adds them.
 */
struct css_length {
	float value;
	int unit;
	float offset;
};

/* An element's custom properties (css/internal.h). */
struct css_custom;

/* A media query list (css/internal.h). */
struct css_media;

/* The most media lists a sheet of the cascade is under (a <link>'s and its imports'). */
#define CSS_MEDIA_CHAIN_MAX	10

/* The values of display. */
enum css_display {
	CSS_DISPLAY_INLINE,
	CSS_DISPLAY_BLOCK,
	CSS_DISPLAY_INLINE_BLOCK,
	CSS_DISPLAY_LIST_ITEM,
	CSS_DISPLAY_NONE,
	CSS_DISPLAY_TABLE,
	CSS_DISPLAY_TABLE_ROW,
	CSS_DISPLAY_TABLE_CELL,
	CSS_DISPLAY_FLEX,
	CSS_DISPLAY_CONTENTS,
	CSS_DISPLAY_GRID,
	CSS_DISPLAY_TABLE_ROW_GROUP,
	CSS_DISPLAY_TABLE_CAPTION,
	CSS_DISPLAY_INLINE_TABLE,
	CSS_DISPLAY_TABLE_COLUMN
};

/* The values of position. */
enum css_position {
	CSS_POSITION_STATIC,
	CSS_POSITION_RELATIVE,
	CSS_POSITION_ABSOLUTE,
	CSS_POSITION_FIXED,
	CSS_POSITION_STICKY
};

/* The values of float, and the sides clear clears (both is the two together). */
enum css_float {
	CSS_FLOAT_NONE,
	CSS_FLOAT_LEFT,
	CSS_FLOAT_RIGHT,
	CSS_CLEAR_BOTH
};

/*
 * The values of vertical-align (ws074-p060): the baseline, the line box's
 * top or bottom, the middle (the parent's baseline plus half its
 * x-height), the parent's text top or bottom, the parent's subscript or
 * superscript position, or a length (vertical_offset: pixels or a
 * percentage of the line height) above the baseline.
 */
enum css_vertical_align {
	CSS_VALIGN_BASELINE,
	CSS_VALIGN_TOP,
	CSS_VALIGN_MIDDLE,
	CSS_VALIGN_BOTTOM,
	CSS_VALIGN_TEXT_TOP,
	CSS_VALIGN_TEXT_BOTTOM,
	CSS_VALIGN_SUB,
	CSS_VALIGN_SUPER,
	CSS_VALIGN_LENGTH
};

/* The most sources an @font-face rule keeps (ws074-p070; the ones after are dropped). */
#define CSS_FONT_SOURCES	4

/* The formats an @font-face source declares (format("...")), or unknown when it declares none. */
enum css_font_format {
	CSS_FONT_FORMAT_UNKNOWN,
	CSS_FONT_FORMAT_WOFF,
	CSS_FONT_FORMAT_WOFF2,
	CSS_FONT_FORMAT_TRUETYPE,
	CSS_FONT_FORMAT_OTHER
};

/*
 * One @font-face rule (ws074-p070): the family it names (an atom), the
 * range of weights it covers, whether it is italic (or oblique), and its
 * sources in order of preference (URLs as atoms, resolved against the
 * sheet once the page resolves the sheet's URLs, each with its format;
 * local() sources are not kept).
 */
struct css_font_face {
	struct vm_string *family;
	int weight_min;
	int weight_max;
	int italic;
	struct vm_string *sources[CSS_FONT_SOURCES];
	int formats[CSS_FONT_SOURCES];
	size_t source_count;
};

/* The most box shadows a style keeps (ws074-p062; the ones after are dropped). */
#define CSS_SHADOWS	4

/* The corners of border-radius, in the order its shorthand lists them. */
#define CSS_TOP_LEFT		0
#define CSS_TOP_RIGHT		1
#define CSS_BOTTOM_RIGHT	2
#define CSS_BOTTOM_LEFT		3

/*
 * One box shadow (ws074-p062): its offset, blur radius and spread in
 * pixels, its color, and whether it is drawn inside the box.
 */
struct css_shadow {
	float x;
	float y;
	float blur;
	float spread;
	uint32_t color;
	int inset;
};

/* The most tracks a grid template keeps (ws074-p072; the ones after are dropped). */
#define CSS_TRACKS	24

/* The kinds of grid track size: a length (or percentage), a share of the free space, or the content's. */
enum css_track_kind {
	CSS_TRACK_LENGTH,
	CSS_TRACK_FR,
	CSS_TRACK_AUTO
};

/*
 * One track of a grid template (ws074-p072): its kind, its size (a
 * length or percentage of the grid's content box, or its fr share) and
 * the smallest it may be (minmax(); a length, or auto for none given).
 */
struct css_track {
	int kind;
	struct css_length size;
	float fr;
	struct css_length minimum;
};

/*
 * Where a grid item goes on one axis (ws074-p072): its start and end,
 * each a line (0 for auto; a negative line counts from the end) or a span
 * (0 for none).
 */
struct css_grid_place {
	int start;
	int start_span;
	int end;
	int end_span;
};

/* The values of container-type (ws074-p075): not a container, a container of its width, or of both sizes. */
enum css_container_type {
	CSS_CONTAINER_NORMAL,
	CSS_CONTAINER_INLINE_SIZE,
	CSS_CONTAINER_SIZE
};

/* The values of direction (ws074-p073): the inline base direction. */
enum css_direction {
	CSS_DIRECTION_LTR,
	CSS_DIRECTION_RTL
};

/* The values of overflow-x and overflow-y. */
enum css_overflow {
	CSS_OVERFLOW_VISIBLE,
	CSS_OVERFLOW_HIDDEN,
	CSS_OVERFLOW_CLIP,
	CSS_OVERFLOW_SCROLL,
	CSS_OVERFLOW_AUTO
};

/* The values of text-align. */
enum css_text_align {
	CSS_TEXT_ALIGN_START,
	CSS_TEXT_ALIGN_LEFT,
	CSS_TEXT_ALIGN_RIGHT,
	CSS_TEXT_ALIGN_CENTER,
	CSS_TEXT_ALIGN_JUSTIFY,
	CSS_TEXT_ALIGN_END
};

/* The predefined cursor selected by computed style; images are not stored here. */
enum css_cursor {
	CSS_CURSOR_AUTO,
	CSS_CURSOR_DEFAULT,
	CSS_CURSOR_NONE,
	CSS_CURSOR_CONTEXT_MENU,
	CSS_CURSOR_HELP,
	CSS_CURSOR_POINTER,
	CSS_CURSOR_PROGRESS,
	CSS_CURSOR_WAIT,
	CSS_CURSOR_CELL,
	CSS_CURSOR_CROSSHAIR,
	CSS_CURSOR_TEXT,
	CSS_CURSOR_VERTICAL_TEXT,
	CSS_CURSOR_ALIAS,
	CSS_CURSOR_COPY,
	CSS_CURSOR_MOVE,
	CSS_CURSOR_NO_DROP,
	CSS_CURSOR_NOT_ALLOWED,
	CSS_CURSOR_E_RESIZE,
	CSS_CURSOR_N_RESIZE,
	CSS_CURSOR_NE_RESIZE,
	CSS_CURSOR_NW_RESIZE,
	CSS_CURSOR_S_RESIZE,
	CSS_CURSOR_SE_RESIZE,
	CSS_CURSOR_SW_RESIZE,
	CSS_CURSOR_W_RESIZE,
	CSS_CURSOR_EW_RESIZE,
	CSS_CURSOR_NS_RESIZE,
	CSS_CURSOR_NESW_RESIZE,
	CSS_CURSOR_NWSE_RESIZE,
	CSS_CURSOR_COL_RESIZE,
	CSS_CURSOR_ROW_RESIZE,
	CSS_CURSOR_ALL_SCROLL,
	CSS_CURSOR_GRAB,
	CSS_CURSOR_GRABBING,
	CSS_CURSOR_ZOOM_IN,
	CSS_CURSOR_ZOOM_OUT
};

/* The four CSS2 text-transform computed keywords retained by each native style. */
enum css_text_transform {
	CSS_TEXT_TRANSFORM_NONE,
	CSS_TEXT_TRANSFORM_CAPITALIZE,
	CSS_TEXT_TRANSFORM_UPPERCASE,
	CSS_TEXT_TRANSFORM_LOWERCASE
};

/* The values of white-space. */
enum css_white_space {
	CSS_WHITE_SPACE_NORMAL,
	CSS_WHITE_SPACE_PRE,
	CSS_WHITE_SPACE_NOWRAP,
	CSS_WHITE_SPACE_PRE_WRAP,
	CSS_WHITE_SPACE_PRE_LINE
};

/*
 * The values of border-style this pass tells apart (groove and ridge parse
 * as solid; every style is drawn solid, but a form control whose borders
 * are all inset or outset, as the user agent's sheet gives it, is drawn
 * the way the platform draws such a control).
 */
enum css_border_style {
	CSS_BORDER_NONE,
	CSS_BORDER_SOLID,
	CSS_BORDER_DASHED,
	CSS_BORDER_DOTTED,
	CSS_BORDER_DOUBLE,
	CSS_BORDER_INSET,
	CSS_BORDER_OUTSET
};

/* The values of box-sizing: what width and height size. */
enum css_box_sizing {
	CSS_BOX_SIZING_CONTENT,
	CSS_BOX_SIZING_BORDER
};

/* The values of list-style-type. */
enum css_list_style {
	CSS_LIST_DISC,
	CSS_LIST_CIRCLE,
	CSS_LIST_SQUARE,
	CSS_LIST_DECIMAL,
	CSS_LIST_NONE
};

/* The values of background-repeat (the two axes together). */
enum css_background_repeat {
	CSS_REPEAT_BOTH,
	CSS_REPEAT_X,
	CSS_REPEAT_Y,
	CSS_REPEAT_NONE
};

/* The values of background-attachment that affect painting. */
enum css_background_attachment {
	CSS_BACKGROUND_SCROLL,
	CSS_BACKGROUND_FIXED
};

/*
 * The keywords of background-size (a size of lengths is none of them);
 * numbered past the units, as the declared width's keyword carries them.
 */
enum css_background_size {
	CSS_BACKGROUND_SIZE_LENGTHS = 0,
	CSS_BACKGROUND_SIZE_CONTAIN = 100,
	CSS_BACKGROUND_SIZE_COVER = 101
};

/*
 * The pseudo-elements that make boxes (ws074-p069), as numbers of the
 * cascade and as bits of a style's pseudo_elements.
 */
#define CSS_PSEUDO_ELEMENT_NONE		0
#define CSS_PSEUDO_ELEMENT_BEFORE	1
#define CSS_PSEUDO_ELEMENT_AFTER	2
#define CSS_PSEUDO_ELEMENT_OTHER	3

/* What a ::before or ::after box holds: nothing, or a list of strings and attributes' values. */
enum css_content_kind {
	CSS_CONTENT_NONE,
	CSS_CONTENT_LIST
};

/*
 * One item of generated content: a string, or the name of an attribute
 * whose value is shown (both atoms).
 */
struct css_content_item {
	int is_attribute;
	struct vm_string *text;
};

/*
 * The generated content of a ::before or ::after: its items in order, in
 * the arena of the sheet that declared it.
 */
struct css_content {
	const struct css_content_item *items;
	size_t count;
};

/* The values of flex-direction (ws074-p035). */
enum css_flex_direction {
	CSS_FLEX_ROW,
	CSS_FLEX_ROW_REVERSE,
	CSS_FLEX_COLUMN,
	CSS_FLEX_COLUMN_REVERSE
};

/* The values of flex-wrap. */
enum css_flex_wrap {
	CSS_FLEX_NOWRAP,
	CSS_FLEX_WRAP,
	CSS_FLEX_WRAP_REVERSE
};

/*
 * The places of justify-content, align-items, align-self and
 * align-content (auto is align-self's only; normal and stretch are one).
 */
enum css_align {
	CSS_ALIGN_AUTO,
	CSS_ALIGN_STRETCH,
	CSS_ALIGN_START,
	CSS_ALIGN_END,
	CSS_ALIGN_CENTER,
	CSS_ALIGN_BASELINE,
	CSS_ALIGN_SPACE_BETWEEN,
	CSS_ALIGN_SPACE_AROUND,
	CSS_ALIGN_SPACE_EVENLY
};

/* The generic font families. */
enum css_generic_family {
	CSS_FAMILY_SERIF,
	CSS_FAMILY_SANS_SERIF,
	CSS_FAMILY_MONOSPACE
};

/*
 * The computed style of one element: every property this pass knows, with
 * lengths in pixels where they do not depend on the layout.  Colors are
 * 0xAARRGGBB, not premultiplied.
 */
struct css_style {
	/* The box. */
	int display;
	int position;
	int float_side;
	int clear;
	int overflow_x;
	int overflow_y;
	int visibility;
	struct css_length width;
	struct css_length height;
	struct css_length min_width;
	struct css_length max_width;
	struct css_length min_height;
	struct css_length max_height;
	struct css_length margin[4];
	struct css_length padding[4];
	struct css_length offset[4];
	int box_sizing;

	/*
	 * The box's decoration (ws074-p062): each corner's radius, horizontal
	 * then vertical (pixels or a percentage of the border box), the opacity
	 * of the box and its content (0 to 1), its shadows, and its outline
	 * (width and offset in pixels, a border style, a color).
	 */
	struct css_length radius[4][2];
	float opacity;
	struct css_shadow shadows[CSS_SHADOWS];
	int shadow_count;
	float outline_width;
	int outline_style;
	uint32_t outline_color;
	float outline_offset;

	/*
	 * clip-path: inset() (ws074-p062): whether the box's painting is clipped,
	 * and how far in from each side of its border box (top, right, bottom,
	 * left; pixels or a percentage of the box's height or width; negative
	 * reaches outside).  The other shapes are not clipped in this pass.
	 */
	int clip_inset;
	struct css_length clip[4];
	int vertical_align;
	struct css_length vertical_offset;
	int z_index;
	int z_index_auto;
	float border_width[4];
	int border_style[4];
	uint32_t border_color[4];

	/* The colors. */
	uint32_t color;
	uint32_t background_color;

	/*
	 * The background image: its URL as the style sheet wrote it (an atom,
	 * which lives as long as the heap; NULL for none), how it repeats,
	 * where it sits (x and y: pixels or a percentage of the room left in the
	 * padding box) and its size (contain, cover, or a width and height, each
	 * pixels, a percentage of the padding box or auto).
	 */
	struct vm_string *background_image;
	int background_repeat;
	int background_attachment;
	struct css_length background_position[2];
	int background_size_keyword;
	struct css_length background_size[2];

	/*
	 * The font.  font_size_keyword is 1 while the size still follows the
	 * keyword scale (medium by default, or an em or a percentage of such a
	 * size), which a change to or from the monospace family rescales.
	 */
	float font_size;
	int font_size_keyword;
	int font_weight;
	int font_italic;
	int generic_family;
	struct vm_string *families[CSS_FAMILIES_MAX];
	int family_count;

	/* The text. */
	struct css_length line_height;
	int text_align;
	struct css_length text_indent;
	int direction;
	int container_type;

	/* Tables (ws074-p037): the spacing between the cells (across, then down, in pixels), and whether the borders collapse. */
	float border_spacing[2];
	int border_collapse;
	int white_space;
	int text_transform;
	int cursor;
	int underline;
	int list_style;

	/*
	 * Generated content (ws074-p069): what content gives a ::before or
	 * ::after (content_kind, and the list of its items, in the declaring
	 * sheet's arena), and for an element, which of its pseudo-elements some rule
	 * matches (1 << CSS_PSEUDO_ELEMENT_*), so the layout asks for their
	 * styles only then.
	 */
	int content_kind;
	const struct css_content *content;
	int pseudo_elements;

	/* Flexible boxes (ws074-p035): the container's, then the item's. */
	int flex_direction;
	int flex_wrap;
	int justify_content;
	int align_items;
	int align_content;
	struct css_length row_gap;
	struct css_length column_gap;

	/* Grids (ws074-p072): the container's templates, then the item's places. */
	struct css_track columns[CSS_TRACKS];
	int column_count;
	struct css_track rows[CSS_TRACKS];
	int row_count;
	struct css_grid_place grid_column;
	struct css_grid_place grid_row;
	float flex_grow;
	float flex_shrink;
	struct css_length flex_basis;
	int align_self;
	int order;

	/*
	 * The element's custom properties (inherited; its own first, then its
	 * parent's), in the style engine's arena: good while the engine that
	 * computed the style lives.
	 */
	const struct css_custom *custom;
};

/*
 * The style sheets of a document and what the cascade needs: the user
 * agent's sheet and the author's, in document order.
 */
struct css_engine;

/*
 * One parsed author style sheet (a <style> element's text, or a sheet a
 * <link> or an @import fetched), with its rule index.  A page keeps the
 * sheets it parsed and lends them to each engine it makes, so a sheet is
 * parsed once however often the page is styled again.
 */
struct css_sheet;

/*
 * Resolves a URL a sheet's value names (an atom) against the sheet's own
 * location, into *resolved (an atom of the same heap); returns 0 or an
 * errno value (EINVAL leaves the URL as it is).
 */
typedef int (*css_url_resolver)(void *context, const struct vm_string *url, struct vm_string **resolved);

/*
 * Finds a query container's content box size in pixels (ws074-p075): the
 * page answers from its last layout; 0 when the container was not laid
 * out there, which the engine counts (css_engine_container_missed).
 */
typedef int (*css_container_lookup)(void *context, const struct dom_element *container, float *width, float *height);

/* A native ordered top-level source list owns immutable entries until its final release. */
struct css_rule_model;

/* Structural sources, not normative CSSRule.cssText serialization (rule-model.c). */
int css_rule_model_create(struct css_rule_model **model, struct vm_heap *heap, const uint16_t *units, size_t length);
void css_rule_model_destroy(struct css_rule_model *model);
size_t css_rule_model_count(const struct css_rule_model *model);
uint32_t css_rule_model_id(const struct css_rule_model *model, size_t index);
int css_rule_model_source(const struct css_rule_model *model, uint32_t id, const uint16_t **units, size_t *length, int *type, int *present);
int css_rule_model_text(const struct css_rule_model *model, struct wb_units *units);
int css_rule_model_insert(struct css_rule_model *model, const uint16_t *units, size_t length, size_t index);
int css_rule_model_delete(struct css_rule_model *model, size_t index);

/* Sheets (parser.c). */
int css_sheet_create(struct css_sheet **sheet, struct vm_heap *heap, const uint16_t *units, size_t length);
void css_sheet_destroy(struct css_sheet *sheet);
size_t css_sheet_import_count(const struct css_sheet *sheet);
size_t css_sheet_font_face_count(const struct css_sheet *sheet);
const struct css_font_face *css_sheet_font_face(const struct css_sheet *sheet, size_t index);
struct vm_string *css_sheet_import(const struct css_sheet *sheet, size_t index);
size_t css_sheet_rule_count(const struct css_sheet *sheet);
const struct css_media *css_sheet_import_media(const struct css_sheet *sheet, size_t index);
int css_sheet_resolve_urls(struct css_sheet *sheet, css_url_resolver resolve, void *context);

/*
 * A selector list a script gave (querySelector and the like), parsed once
 * and matched against elements (css_query_parse, css_engine_query_matches).
 */
struct css_query;

/* The selector lists of scripts (parser.c, cascade.c; ws074-p031). */
int css_query_parse(struct vm_heap *heap, const uint16_t *units, size_t length, struct css_query **query);
void css_query_destroy(struct css_query *query);

/* The inline style of scripts (parser.c, values.c; ws074-p031). */
int css_declaration_valid(struct vm_heap *heap, const uint16_t *units, size_t length, int *valid);
const char *css_property_name(size_t index);

/* The engine (cascade.c). */
int css_engine_create(struct css_engine **engine, struct vm_heap *heap);
void css_engine_destroy(struct css_engine *engine);
int css_engine_add_sheet(struct css_engine *engine, const uint16_t *units, size_t length);
int css_engine_add_sheet_origin(struct css_engine *engine, const uint16_t *units, size_t length, int origin);
int css_engine_add_parsed(struct css_engine *engine, const struct css_sheet *sheet, const struct css_media *const *media, size_t media_count);
int css_engine_parse_media(struct css_engine *engine, const uint16_t *units, size_t length, const struct css_media **media);
void css_engine_set_viewport(struct css_engine *engine, float width, float height);
void css_engine_set_container_lookup(struct css_engine *engine, css_container_lookup lookup, void *context);
int css_engine_container_missed(const struct css_engine *engine);
size_t css_engine_container_uses(const struct css_engine *engine);
void css_engine_container_use(const struct css_engine *engine, size_t index, const struct dom_element **container, float *width, float *height);
void css_engine_forget_styles(struct css_engine *engine);
int css_engine_compute(struct css_engine *engine, struct dom_element *element, const struct css_style *parent, struct css_style *style);
int css_engine_compute_pseudo(struct css_engine *engine, struct dom_element *element, int pseudo, const struct css_style *element_style, struct css_style *style);
void css_initial_style(struct css_style *style);
void css_engine_query_begin(struct css_engine *engine);
int css_engine_query_matches(struct css_engine *engine, struct dom_element *element, const struct css_query *query);

#endif
