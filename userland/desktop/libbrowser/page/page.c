/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Loading a page: bytes to text, text to a DOM (running the scripts as the
 * parser reaches them), the DOM's <style> elements to the style engine;
 * and the text dumps the tests read.
 */

#include "page/page.h"
#include "net/net.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The most bytes of live cells a page's heap may hold. */
#define PAGE_HEAP_LIMIT		((size_t)1024U * 1024U * 1024U)

/* How many times more a layout is made at most for the query containers' sizes (ws074-p075). */
#define PAGE_CONTAINER_PASSES	2

/* The deepest element nesting the style dump descends (the parser caps nesting too). */
#define PAGE_DUMP_DEPTH		512

static int page_gather_styles(struct page *page);
static int page_container_size(void *context, const struct dom_element *container, float *width, float *height);
static int page_containers_moved(const struct page *page);
static int page_text_of(const struct dom_node *node, struct wb_units *units);
static const struct dom_node *page_find_title(const struct dom_node *node, int depth);
static int page_dump_node(const struct dom_node *node, int depth, struct wb_buffer *out);
static int page_dump_style_node(struct page *page, struct dom_element *element, const struct css_style *parent, int depth, struct wb_buffer *out);
static int page_dump_style_line(const struct dom_element *element, const struct css_style *style, int depth, struct wb_buffer *out);
static int page_indent(struct wb_buffer *out, int depth);
static int page_append_string(struct wb_buffer *out, const struct vm_string *string);
static int page_append_length(struct wb_buffer *out, const struct css_length *length);

/*
 * Makes an empty page: a heap whose C stack ends at stack_base, and an
 * empty document.
 */
int
page_create(
	struct page **page,
	const void *stack_base)
{
	struct page *created;
	int error;

	/* Allocates the page and its heap. */
	created = calloc(1, sizeof(*created));
	if (created == NULL)
		return ENOMEM;
	error = vm_heap_create(&created->heap, PAGE_HEAP_LIMIT);
	if (error != 0) {
		free(created);
		return error;
	}

	/* Its stack ends where the caller says, and it has no images yet. */
	vm_heap_set_stack_base(created->heap, stack_base);
	page_images_init(created);
	page_sheets_init(created);
	page_fonts_init(created);
	page_scripts_init(created);
	page_frames_init(created);

	/* Makes the document and keeps it alive as a root. */
	created->document = dom_document_create(created->heap);
	if (created->document == NULL) {
		vm_heap_destroy(created->heap);
		free(created);
		return ENOMEM;
	}

	/* Keeps the document alive as a root. */
	error = vm_heap_add_root(created->heap, (struct vm_cell **)&created->document);
	if (error != 0) {
		vm_heap_destroy(created->heap);
		free(created);
		return error;
	}

	/* Keeps the focused element alive as a root (none yet). */
	error = vm_heap_add_root(created->heap, (struct vm_cell **)&created->focused);
	if (error != 0) {
		vm_heap_destroy(created->heap);
		free(created);
		return error;
	}

	/* A page starts in a view that has its program's focus (the view says otherwise). */
	created->window_focused = 1;

	/* The realm whose global object is the document's window. */
	error = page_start_scripts(created);
	if (error != 0) {
		page_destroy(created);
		return error;
	}

	/* Succeeded: the page is empty. */
	*page = created;
	return 0;
}

/*
 * Destroys a page, its styles and its heap.
 */
void
page_destroy(
	struct page *page)
{
	/* A NULL page is nothing to destroy. */
	if (page == NULL)
		return;

	/* Frees the display list, the layout, the fonts and the style engine, then every cell with the heap. */
	if (page->painted)
		paint_release(&page->paint);
	if (page->laid_out)
		layout_release(&page->layout);
	if (page->previous_laid_out)
		layout_release(&page->previous_layout);
	if (page->text_open)
		text_system_close(&page->text);
	page_images_release(page);
	css_engine_destroy(page->css);
	css_engine_destroy(page->query_css);
	page_box_index_release(page);
	page_sheets_release(page);
	page_fonts_release(page);
	page_frames_release(page);
	page_scripts_release(page);
	bind_window_destroy(page->window);
	vm_realm_destroy(page->realm);
	vm_heap_destroy(page->heap);
	free(page->base);
	free(page);
}

/*
 * Loads an HTML document from its bytes (UTF-8 in this pass) and gathers
 * its style sheets.
 */
int
page_load_html(
	struct page *page,
	const unsigned char *bytes,
	size_t length)
{
	struct html_parser *parser;
	struct wb_units units;
	int error;

	/* Decodes the bytes, dropping a UTF-8 byte order mark. */
	if (length >= 3 && bytes[0] == 0xefU && bytes[1] == 0xbbU && bytes[2] == 0xbfU) {
		bytes += 3;
		length -= 3;
	}

	/* Decodes the text. */
	wb_units_init(&units);
	error = wb_utf8_to_units(bytes, length, &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Parses the text into the document, with scripting: each script runs when the parser reaches its end. */
	error = html_parser_create(&parser, page->document, 1);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* The page runs the scripts the parser reaches. */
	html_parser_set_script_hook(parser, page_run_script_element, page);
	page->parser = parser;

	/* Feeds it all and finishes. */
	error = html_parser_feed(parser, units.data, units.length);
	if (error == 0)
		error = html_parser_finish(parser);
	page->parser = NULL;
	html_parser_destroy(parser);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* The document is parsed: DOMContentLoaded, then the window's load. */
	error = page_fire_load(page);
	if (error != 0)
		return error;

	/* The style sheets of the document as the scripts left it. */
	error = page_gather_styles(page);
	if (error != 0)
		return error;

	/* Succeeded: the page holds the document and its styles. */
	return 0;
}

/*
 * Loads an HTML document from its bytes, as fetched from a location (the
 * page's location for its links, scripts and images).
 */
int
page_load_bytes(
	struct page *page,
	const unsigned char *bytes,
	size_t length,
	const char *location)
{
	int error;

	/* The location. */
	free(page->base);
	page->base = strdup(location);
	if (page->base == NULL)
		return ENOMEM;

	/* The document. */
	error = page_load_html(page, bytes, length);
	if (error != 0)
		return error;

	/* Succeeded: the page is loaded. */
	return 0;
}

/*
 * Loads an HTML document from a file.
 */
int
page_load_file(
	struct page *page,
	const char *path)
{
	struct wb_buffer buffer;
	int error;

	/* The page's scripts and images find their files from the page's, by its absolute path. */
	free(page->base);
	page->base = realpath(path, NULL);
	if (page->base == NULL)
		page->base = strdup(path);
	if (page->base == NULL)
		return ENOMEM;

	/* Reads the file. */
	wb_buffer_init(&buffer);
	error = wb_file_read(path, &buffer);
	if (error != 0) {
		wb_buffer_release(&buffer);
		return error;
	}

	/* Loads its bytes. */
	error = page_load_html(page, buffer.data, buffer.length);
	wb_buffer_release(&buffer);
	if (error != 0)
		return error;

	/* Succeeded: the page is loaded. */
	return 0;
}

/*
 * Loads an HTML document from a location: a file's path (relative to the
 * working directory, or absolute), or a URL (file:, data: or http:).
 * The page's location becomes the file's path, or the URL after its
 * redirects.
 */
int
page_load_location(
	struct page *page,
	const char *location)
{
	struct net_url url;
	struct wb_buffer bytes;
	struct wb_buffer final_url;
	struct wb_buffer where;
	int error;

	/* What is not a URL is a file's path. */
	error = net_url_parse(location, strlen(location), NULL, &url);
	if (error == EINVAL) {
		error = page_load_file(page, location);
		return error;
	}

	/* A URL that cannot be parsed for another reason (memory). */
	if (error != 0)
		return error;
	net_url_release(&url);

	/* The document's bytes and its final URL. */
	wb_buffer_init(&bytes);
	wb_buffer_init(&final_url);
	wb_buffer_init(&where);
	error = page_fetch(location, location, &bytes, &final_url);

	/* The page's location: a file's path, or the final URL. */
	if (error == 0)
		error = page_resolve_location(wb_buffer_string(&final_url), wb_buffer_string(&final_url), &where);
	free(page->base);
	page->base = NULL;
	if (error == 0) {
		page->base = strdup(wb_buffer_string(&where));
		if (page->base == NULL)
			error = ENOMEM;
	}

	/* The document. */
	if (error == 0)
		error = page_load_html(page, bytes.data, bytes.length);
	wb_buffer_release(&bytes);
	wb_buffer_release(&final_url);
	wb_buffer_release(&where);
	if (error != 0)
		return error;

	/* Succeeded: the page is loaded. */
	return 0;
}

/*
 * Tells the page's scripts the size of the viewport the page is shown in
 * (before the page is laid out at it).
 */
void
page_set_viewport(
	struct page *page,
	int width,
	int height)
{
	/* The size a script's question lays the page out at, and the window object's. */
	page->viewport_width = width;
	page->viewport_height = height;
	bind_window_set_viewport(page->window, width, height);
}

/*
 * Tells the page the fonts its view draws with, which a script's question
 * about the geometry lays the page out with (the paths must outlive the
 * page).
 */
void
page_set_fonts(
	struct page *page,
	const struct text_font_paths *paths)
{
	/* The view's paths. */
	page->font_paths = paths;
}

/*
 * Tells the page how far its view is scrolled, in CSS pixels (the
 * scripts' scrollX, scrollY and client rectangles).
 */
void
page_set_scroll(
	struct page *page,
	double x,
	double y)
{
	/* The two distances. */
	page->scroll_x = x;
	page->scroll_y = y;
}

/*
 * Sends the page's console (its scripts' console.log and the like) to a
 * function of the caller's.
 */
void
page_set_console(
	struct page *page,
	page_console console,
	void *context)
{
	/* The function and what it gets back. */
	page->console = console;
	page->console_context = context;
}

/*
 * Describes why the last load failed beyond its error number (the TLS
 * verification's reason, for an https page); empty when there is nothing
 * more to say.
 */
const char *
page_failure_reason(void)
{
	const char *reason;

	/* The network's reason. */
	reason = net_tls_error();
	return reason;
}

/*
 * Opens the fonts the page's text is drawn with.
 */
int
page_open_fonts(
	struct page *page,
	const struct text_font_paths *paths)
{
	int error;

	/* Opens them once. */
	if (page->text_open)
		return 0;
	error = text_system_open(&page->text, paths);
	if (error != 0)
		return error;

	/* Succeeded: the fonts are open. */
	page->text_open = 1;
	return 0;
}

/*
 * Lays the page out for a viewport of width by height pixels (the fonts
 * must be open).
 */
int
page_layout(
	struct page *page,
	int width,
	int height)
{
	int missed;
	int pass;
	int error;
	uint32_t old_laid_out_images;

	/* A failed rebuild must leave the prior complete layout's image generation intact. */
	old_laid_out_images = page->laid_out_images;

	/* Throws the old display list and layout away. */
	if (page->painted) {
		paint_release(&page->paint);
		page->painted = 0;
	}

	/*
	 * The layout: the old one is kept while the new one is made, for the
	 * sizes of the query containers of the container-relative units
	 * (ws074-p075).
	 */
	if (page->laid_out) {
		page->previous_layout = page->layout;
		page->previous_laid_out = 1;
		memset(&page->layout, 0, sizeof(page->layout));
		page->laid_out = 0;
	}

	/* The style sheets again when the document or its sheets changed since they were gathered. */
	error = page_update_styles(page);
	if (error != 0)
		goto cleanup;

	/* The web fonts that arrived join the text system. */
	page_fonts_install(page);

	/* The window's size for the scripts, and the size their questions lay the page out at. */
	page->viewport_width = width;
	page->viewport_height = height;
	bind_window_set_viewport(page->window, width, height);

	/* The images the document names, fetched and decoded when they are new. */
	error = page_load_images(page);
	if (error != 0)
		goto cleanup;

	/* Builds and lays out the box tree, the images found by their elements (the ones there are now). */
	page->laid_out_images = page->images_generation;
	css_engine_set_container_lookup(page->css, page_container_size, page);
	error = layout_build(&page->layout, page->css, &page->text, page->document, page_image_of, page_image_by_url, page, width, height);
	page->laid_out = 1;
	if (error != 0)
		goto cleanup;

	/*
	 * A query container the old layout did not have (the first layout), or
	 * one this layout gave another size than the styles used: the page is
	 * laid out again with the sizes it has now (twice more at most).
	 */
	for (pass = 0; pass < PAGE_CONTAINER_PASSES; pass++) {
		missed = css_engine_container_missed(page->css);
		if (!missed)
			missed = page_containers_moved(page);
		if (!missed)
			break;

		/* This layout is the one the sizes come from now. */
		if (page->previous_laid_out)
			layout_release(&page->previous_layout);
		page->previous_layout = page->layout;
		page->previous_laid_out = 1;
		memset(&page->layout, 0, sizeof(page->layout));
		css_engine_forget_styles(page->css);
		error = layout_build(&page->layout, page->css, &page->text, page->document, page_image_of, page_image_by_url, page, width, height);
		if (error != 0)
			goto cleanup;
	}

	/* The old layout is no longer needed. */
	if (page->previous_laid_out) {
		layout_release(&page->previous_layout);
		page->previous_laid_out = 0;
	}

	/* Only a complete layout advances observable generation and geometry caches. */
	page->laid_out_generation = page->document->generation;
	page->layout_serial++;

	/* Succeeded: the page is laid out. */
	return 0;

cleanup:
	/* Discard a partly built layout and restore the last complete one, if any. */
	if (page->laid_out) {
		layout_release(&page->layout);
		page->laid_out = 0;
	}

	/* The last complete layout remains available after a failed rebuild. */
	if (page->previous_laid_out) {
		page->layout = page->previous_layout;
		page->laid_out = 1;
		page->previous_laid_out = 0;
		memset(&page->previous_layout, 0, sizeof(page->previous_layout));
	}

	/* Failed layout attempts do not consume a new image generation. */
	page->laid_out_images = old_laid_out_images;
	return error;
}

/*
 * Builds the display list of the laid out page.
 */
int
page_paint(
	struct page *page)
{
	int error;

	/* A page that is not laid out has nothing to paint. */
	if (!page->laid_out)
		return EINVAL;

	/* Throws the old display list away. */
	if (page->painted) {
		paint_release(&page->paint);
		page->painted = 0;
	}

	/* Walks the layout into a new one. */
	error = paint_build(&page->paint, &page->layout);
	if (error != 0)
		return error;
	page->painted = 1;

	/* The focus ring over it, when the keyboard put the focus somewhere. */
	error = page_paint_focus(page);
	if (error != 0)
		return error;

	/* The caret of a focused text control. */
	error = page_paint_caret(page);
	if (error != 0)
		return error;

	/* Succeeded: the page has its display list. */
	return 0;
}

/*
 * Writes the document's title (its first <title> element's text, with
 * whitespace collapsed and trimmed) as UTF-8; empty when there is none.
 */
int
page_title(
	const struct page *page,
	struct wb_buffer *out)
{
	const struct dom_node *title;
	struct wb_units text;
	struct wb_units collapsed;
	uint16_t unit;
	size_t index;
	int space;
	int error;

	/* The first <title> in the document. */
	title = page_find_title(&page->document->node, 0);
	if (title == NULL)
		return 0;

	/* Its text. */
	wb_units_init(&text);
	wb_units_init(&collapsed);
	error = page_text_of(title, &text);

	/* Each run of whitespace becomes one space, and none at either end. */
	space = 0;
	for (index = 0; error == 0 && index < text.length; index++) {
		unit = text.data[index];

		/* A whitespace character is remembered until something follows it. */
		if (unit == 0x20U ||
		    unit == 0x09U ||
		    unit == 0x0aU ||
		    unit == 0x0cU ||
		    unit == 0x0dU) {
			space = 1;
			continue;
		}

		/* The space before a character, when there is text before it. */
		if (space && collapsed.length != 0)
			error = wb_units_append_code_point(&collapsed, 0x20U);
		space = 0;

		/* The character itself. */
		if (error == 0)
			error = wb_units_append(&collapsed, &text.data[index], 1);
	}

	/* The UTF-8 of the collapsed text. */
	if (error == 0)
		error = wb_units_to_utf8(collapsed.data, collapsed.length, out);
	wb_units_release(&text);
	wb_units_release(&collapsed);
	if (error != 0)
		return error;

	/* Succeeded: the title is written. */
	return 0;
}

/*
 * Writes the document tree in html5lib's test format.
 */
int
page_dump_dom(
	const struct page *page,
	struct wb_buffer *out)
{
	int error;

	/* Dumps the document's children. */
	error = page_dump_node(&page->document->node, 0, out);
	if (error != 0)
		return error;

	/* Succeeded: the tree is in the buffer. */
	return 0;
}

/*
 * Writes every element's computed style, one line each, indented by depth.
 */
int
page_dump_style(
	struct page *page,
	struct wb_buffer *out)
{
	struct dom_node *node;
	int error;

	/* The style sheets as the document and its fetched sheets are now. */
	error = page_update_styles(page);
	if (error != 0)
		return error;

	/* Styles from each element child of the document. */
	for (node = page->document->node.first_child; node != NULL; node = node->next) {
		if (node->type != DOM_ELEMENT)
			continue;
		error = page_dump_style_node(page, (struct dom_element *)node, NULL, 0, out);
		if (error != 0)
			return error;
	}

	/* Succeeded: the styles are in the buffer. */
	return 0;
}

/*
 * Tells whether a query container the styles used has another size (by
 * more than half a pixel) in the page's layout, or none (ws074-p075).
 */
static int
page_containers_moved(
	const struct page *page)
{
	const struct dom_element *container;
	const struct layout_box *box;
	float width;
	float height;
	float difference;
	size_t count;
	size_t index;

	/* Each container the engine used. */
	count = css_engine_container_uses(page->css);
	for (index = 0; index < count; index++) {
		css_engine_container_use(page->css, index, &container, &width, &height);
		box = layout_box_of(&page->layout, &container->node);
		if (box == NULL)
			return 1;

		/* Its width or its height moved. */
		difference = layout_to_px(box->width) - width;
		if (difference > 0.5f || difference < -0.5f)
			return 1;
		difference = layout_to_px(box->height) - height;
		if (difference > 0.5f || difference < -0.5f)
			return 1;
	}

	/* Every container is as the styles used it. */
	return 0;
}

/*
 * Finds a query container's content box size in the page's previous
 * layout (ws074-p075), for the container-relative units of the style
 * engine; 0 when that layout did not have the container.
 */
static int
page_container_size(
	void *context,
	const struct dom_element *container,
	float *width,
	float *height)
{
	const struct layout_box *box;
	struct page *page;

	/* The previous layout's box of the container. */
	page = context;
	if (!page->previous_laid_out)
		return 0;
	box = layout_box_of(&page->previous_layout, &container->node);
	if (box == NULL)
		return 0;

	/* Its content box. */
	*width = layout_to_px(box->width);
	*height = layout_to_px(box->height);
	return 1;
}

/*
 * Gathers the style sheets again when a script changed the document, or
 * a fetched sheet arrived, since they were last gathered.
 */
int
page_update_styles(
	struct page *page)
{
	int pending;
	int error;

	/* Styles that are up to date stay. */
	if (page->css != NULL &&
	    page->styled_generation == page->document->generation &&
	    page->styled_sheets == page->sheets_generation)
		return 0;

	/*
	 * So do styles of the same document while more sheets are on their way:
	 * the page is styled again once they are all here (ws074-p071).
	 */
	pending = page_sheets_pending(page);
	if (page->css != NULL && page->styled_generation == page->document->generation && pending)
		return 0;

	/* A new engine with the sheets as they are now. */
	error = page_gather_styles(page);
	if (error != 0)
		return error;

	/* Succeeded: the styles match the document. */
	return 0;
}

/* Makes the style engine anew with the document's style sheets in order. */
static int
page_gather_styles(
	struct page *page)
{
	int error;

	/* A new engine with the user agent's sheet. */
	css_engine_destroy(page->css);
	page->css = NULL;
	error = css_engine_create(&page->css, page->heap);
	if (error != 0)
		return error;

	/* The document's sheets, parsed once and lent to the engine. */
	error = page_sheets_add(page);
	if (error != 0)
		return error;

	/* Succeeded: the sheets match the document of this generation. */
	page->styled_generation = page->document->generation;
	return 0;
}

/* Appends the text of a node's text children. */
static int
page_text_of(
	const struct dom_node *node,
	struct wb_units *units)
{
	const struct dom_node *child;
	const struct dom_character_data *text;
	int error;

	/* Joins the text children. */
	for (child = node->first_child; child != NULL; child = child->next) {
		if (child->type != DOM_TEXT)
			continue;
		text = (const struct dom_character_data *)child;
		error = wb_units_append(units, text->data.data, text->data.length);
		if (error != 0)
			return error;
	}

	/* Succeeded: the text is appended. */
	return 0;
}

/* Dumps a node's children at a depth. */
static int
page_dump_node(
	const struct dom_node *node,
	int depth,
	struct wb_buffer *out)
{
	const struct dom_node *child;
	const struct dom_element *element;
	const struct dom_character_data *text;
	size_t index;
	int error;

	/* One line per child, elements followed by their attributes and children. */
	for (child = node->first_child; child != NULL; child = child->next) {
		error = page_indent(out, depth);
		if (error != 0)
			return error;
		switch (child->type) {
		case DOM_ELEMENT:
			element = (const struct dom_element *)child;
			error = wb_buffer_append_string(out, "<");
			if (error != 0)
				return error;
			if (element->ns == DOM_NS_SVG) {
				error = wb_buffer_append_string(out, "svg ");
				if (error != 0)
					return error;
			}

			/* Foreign MathML elements keep their namespace marker. */
			if (element->ns == DOM_NS_MATHML) {
				error = wb_buffer_append_string(out, "math ");
				if (error != 0)
					return error;
			}

			/* The local name and closing delimiter finish the element line. */
			error = page_append_string(out, element->local_name);
			if (error != 0)
				return error;
			error = wb_buffer_append_string(out, ">\n");
			if (error != 0)
				return error;
			for (index = 0; index < element->attribute_count; index++) {
				error = page_indent(out, depth + 1);
				if (error != 0)
					return error;
				error = page_append_string(out, element->attributes[index].name);
				if (error != 0)
					return error;
				error = wb_buffer_append_string(out, "=\"");
				if (error != 0)
					return error;
				error = page_append_string(out, element->attributes[index].value);
				if (error != 0)
					return error;
				error = wb_buffer_append_string(out, "\"\n");
				if (error != 0)
					return error;
			}

			/* Then its children. */
			error = page_dump_node(child, depth + 1, out);
			if (error != 0)
				return error;
			break;
		case DOM_TEXT:
			text = (const struct dom_character_data *)child;
			error = wb_buffer_append_string(out, "\"");
			if (error != 0)
				return error;
			error = wb_units_to_utf8(text->data.data, text->data.length, out);
			if (error != 0)
				return error;
			error = wb_buffer_append_string(out, "\"\n");
			if (error != 0)
				return error;
			break;
		case DOM_COMMENT:
			text = (const struct dom_character_data *)child;
			error = wb_buffer_append_string(out, "<!-- ");
			if (error != 0)
				return error;
			error = wb_units_to_utf8(text->data.data, text->data.length, out);
			if (error != 0)
				return error;
			error = wb_buffer_append_string(out, " -->\n");
			if (error != 0)
				return error;
			break;
		case DOM_DOCUMENT_TYPE:
			error = wb_buffer_append_string(out, "<!DOCTYPE ");
			if (error != 0)
				return error;
			error = page_append_string(out, ((const struct dom_doctype *)child)->name);
			if (error != 0)
				return error;
			error = wb_buffer_append_string(out, ">\n");
			if (error != 0)
				return error;
			break;
		default:
			error = wb_buffer_append_string(out, "?\n");
			if (error != 0)
				return error;
			break;
		}
	}

	/* Succeeded: the children are dumped. */
	return 0;
}

/* Computes and dumps an element's style, then its element children's. */
static int
page_dump_style_node(
	struct page *page,
	struct dom_element *element,
	const struct css_style *parent,
	int depth,
	struct wb_buffer *out)
{
	struct css_style *style;
	struct dom_node *child;
	int error;

	/* Computes the style (on the heap: the recursion would otherwise use much stack). */
	style = malloc(sizeof(*style));
	if (style == NULL)
		return ENOMEM;
	error = css_engine_compute(page->css, element, parent, style);
	if (error != 0) {
		free(style);
		return error;
	}

	/* Emits one complete style line before recursively visiting children. */
	error = page_dump_style_line(element, style, depth, out);
	if (error != 0) {
		free(style);
		return error;
	}

	/* The element children, below the depth limit. */
	error = 0;
	if (depth < PAGE_DUMP_DEPTH) {
		for (child = element->node.first_child; child != NULL && error == 0; child = child->next) {
			if (child->type == DOM_ELEMENT)
				error = page_dump_style_node(page, (struct dom_element *)child, style, depth + 1, out);
		}
	}

	/* The style is no longer needed. */
	free(style);
	if (error != 0)
		return error;

	/* Succeeded: the subtree's styles are dumped. */
	return 0;
}

/* Appends one computed-style line, checking every buffer expansion. */
static int
page_dump_style_line(
	const struct dom_element *element,
	const struct css_style *style,
	int depth,
	struct wb_buffer *out)
{
	static const char *const displays[] = {
		"inline", "block", "inline-block", "list-item", "none", "table", "table-row", "table-cell", "flex", "contents", "grid",
		"table-row-group", "table-caption", "inline-table", "table-column"
	};
	int side;
	int error;

	/* Appends the element name and main computed properties. */
	error = page_indent(out, depth);
	if (error != 0)
		return error;
	error = page_append_string(out, element->local_name);
	if (error != 0)
		return error;
	error = wb_buffer_printf(out, " display=%s font-size=%.2f weight=%d italic=%d color=#%08x background=#%08x",
	    displays[style->display], (double)style->font_size, style->font_weight, style->font_italic,
	    (unsigned)style->color, (unsigned)style->background_color);
	if (error != 0)
		return error;

	/* Appends all four margins in established order. */
	error = wb_buffer_append_string(out, " margin=");
	if (error != 0)
		return error;
	for (side = 0; side < 4; side++) {
		if (side > 0) {
			error = wb_buffer_append_string(out, ",");
			if (error != 0)
				return error;
		}

		/* The current margin follows its separator. */
		error = page_append_length(out, &style->margin[side]);
		if (error != 0)
			return error;
	}

	/* Appends all four paddings in established order. */
	error = wb_buffer_append_string(out, " padding=");
	if (error != 0)
		return error;
	for (side = 0; side < 4; side++) {
		if (side > 0) {
			error = wb_buffer_append_string(out, ",");
			if (error != 0)
				return error;
		}

		/* The current padding follows its separator. */
		error = page_append_length(out, &style->padding[side]);
		if (error != 0)
			return error;
	}

	/* Appends the border widths and final computed width. */
	error = wb_buffer_printf(out, " border=%.1f,%.1f,%.1f,%.1f width=", (double)style->border_width[0], (double)style->border_width[1],
	    (double)style->border_width[2], (double)style->border_width[3]);
	if (error != 0)
		return error;
	error = page_append_length(out, &style->width);
	if (error != 0)
		return error;
	error = wb_buffer_append_string(out, "\n");
	if (error != 0)
		return error;

	/* Succeeded: the line is complete and its caller may visit children. */
	return 0;
}

/* Appends two spaces per level. */
static int
page_indent(
	struct wb_buffer *out,
	int depth)
{
	int level;
	int error;

	/* The html5lib format's prefix and the indentation. */
	error = wb_buffer_append_string(out, "| ");
	if (error != 0)
		return error;
	for (level = 0; level < depth; level++) {
		error = wb_buffer_append_string(out, "  ");
		if (error != 0)
			return error;
	}

	/* Succeeded: the next field starts at the requested tree depth. */
	return 0;
}

/* Appends a VM string as UTF-8. */
static int
page_append_string(
	struct wb_buffer *out,
	const struct vm_string *string)
{
	int error;

	/* Converts the string. */
	error = vm_string_to_utf8(string, out);
	if (error != 0)
		return error;

	/* Succeeded: all string bytes are in the dump. */
	return 0;
}

/* Appends a computed length as text. */
static int
page_append_length(
	struct wb_buffer *out,
	const struct css_length *length)
{
	int error;

	/* The unit decides the form. */
	switch (length->unit) {
	case CSS_UNIT_PX:
		error = wb_buffer_printf(out, "%.2f", (double)length->value);
		break;
	case CSS_UNIT_PERCENT:
		error = wb_buffer_printf(out, "%.2f%%", (double)length->value);
		if (error != 0)
			return error;
		if (length->offset != 0) {
			error = wb_buffer_printf(out, "%+.2f", (double)length->offset);
			if (error != 0)
				return error;
		}

		break;
	case CSS_UNIT_AUTO:
		error = wb_buffer_append_string(out, "auto");
		break;
	default:
		error = wb_buffer_append_string(out, "?");
		break;
	}

	/* A failed append cannot be reported as a complete length. */
	if (error != 0)
		return error;

	/* Succeeded: the complete length representation was appended. */
	return 0;
}

/* Finds the first HTML <title> element under a node, in document order. */
static const struct dom_node *
page_find_title(
	const struct dom_node *node,
	int depth)
{
	const struct dom_node *child;
	const struct dom_node *found;
	int is_title;

	/* The search stops at the depth the dumps stop at. */
	if (depth > PAGE_DUMP_DEPTH)
		return NULL;

	/* The node itself. */
	is_title = dom_element_is(node, DOM_NS_HTML, DOM_TAG_TITLE);
	if (is_title)
		return node;

	/* Otherwise its children, in order. */
	for (child = node->first_child; child != NULL; child = child->next) {
		found = page_find_title(child, depth + 1);
		if (found != NULL)
			return found;
	}

	/* No title under the node. */
	return NULL;
}
