/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Links: the <a href> under a point of the laid out page, and a link's
 * target resolved against the file the page came from.
 *
 * A target is resolved as a URL against the page's location (the
 * absolute path of its file, or its URL) with the WHATWG URL parser
 * (net/url.c): a file: target becomes a path again; data: and http:
 * targets stay URLs, and page_fetch reads what any of them names.
 */

#include "page/page.h"
#include "net/net.h"

#include <errno.h>
#include <string.h>

/* The deepest element nesting searched for a link (the parser caps nesting too). */
#define LINK_DEPTH		512

static int link_resolve(const char *base, const char *href, struct net_url *target);
static int link_named(const char *scheme, const char *name);
static const char *link_file_mime(const char *path);

/*
 * Finds the link under a point of the page (pixels from the top left of
 * the document) and writes its href as UTF-8; *found says whether there
 * was one.
 */
int
page_link_at(
	struct page *page,
	int x,
	int y,
	struct wb_buffer *href,
	int *found)
{
	const struct layout_box *box;
	const struct dom_node *node;
	const struct dom_attribute *attribute;
	struct vm_string *name;
	int is_link;
	int depth;
	int error;

	/* Nothing is found until an <a href> is. */
	*found = 0;
	if (!page->laid_out)
		return 0;

	/* The text under the point. */
	box = layout_hit(&page->layout, (layout_unit)x * LAYOUT_UNIT, (layout_unit)y * LAYOUT_UNIT);
	if (box == NULL || box->node == NULL)
		return 0;

	/* The attribute's name, as the atom the elements keep. */
	name = vm_atom_from_ascii(page->heap, "href");
	if (name == NULL)
		return ENOMEM;

	/* The nearest <a> with an href among the text's ancestors. */
	node = box->node;
	for (depth = 0; node != NULL && depth < LINK_DEPTH; depth++) {
		/* An HTML <a> element with an href is the link. */
		is_link = dom_element_is(node, DOM_NS_HTML, DOM_TAG_A);
		if (is_link) {
			attribute = dom_element_find_attribute((const struct dom_element *)node, DOM_NS_NONE, name);
			if (attribute != NULL)
				break;
		}

		/* Otherwise its parent. */
		node = node->parent;
	}

	/* No link around the text. */
	if (node == NULL || depth == LINK_DEPTH)
		return 0;

	/* The href's value. */
	error = vm_string_to_utf8(attribute->value, href);
	if (error != 0)
		return error;

	/* Succeeded: the link is found. */
	*found = 1;
	return 0;
}

/*
 * Resolves a link's target (a URL, usually relative) against the absolute
 * path of the page's file and writes the absolute path of the file it
 * names.
 *
 * Returns EINVAL for a target that is not a URL, and EPROTONOSUPPORT for a
 * URL of another scheme than file.
 */
int
page_resolve_file(
	const char *base,
	const char *href,
	struct wb_buffer *out)
{
	struct net_url target;
	int error;

	/* The target against the page's file. */
	error = link_resolve(base, href, &target);
	if (error != 0)
		return error;

	/* The file a file: URL names. */
	error = net_url_file_path(&target, out);
	net_url_release(&target);
	if (error != 0)
		return error;

	/* Succeeded: the target's path is written. */
	return 0;
}

/*
 * Resolves a link's target against a page's location (the absolute path
 * of its file, or its URL) and writes the target's location: the path of
 * a file: URL, or the URL itself for other schemes.
 */
int
page_resolve_location(
	const char *base,
	const char *href,
	struct wb_buffer *out)
{
	struct net_url target;
	int error;

	/* The target against the page's location. */
	error = link_resolve(base, href, &target);
	if (error != 0)
		return error;

	/* A local file's path, or the URL. */
	error = net_url_file_path(&target, out);
	if (error == EPROTONOSUPPORT || error == ENOENT) {
		wb_buffer_clear(out);
		error = net_url_serialize(&target, 0, out);
	}

	/* The target is written. */
	net_url_release(&target);
	if (error != 0)
		return error;

	/* Succeeded: the location is written. */
	return 0;
}

/*
 * Reads what a URL (resolved against a page's location) names, as a
 * script, a stylesheet or an image would: a data: URL's body, a file:
 * URL's file, or an http: URL's response body.  The final URL (after
 * redirects) goes to final_url when it is not NULL.  Returns
 * EPROTONOSUPPORT for a URL of another scheme.
 */
int
page_fetch(
	const char *base,
	const char *href,
	struct wb_buffer *bytes,
	struct wb_buffer *final_url)
{
	struct wb_buffer path;
	struct wb_buffer text;
	struct net_url target;
	struct net_data data;
	struct net_response response;
	int is_data;
	int is_http;
	int error;

	/* The target against the page's location. */
	error = link_resolve(base, href, &target);
	if (error != 0)
		return error;
	if (final_url != NULL)
		error = net_url_serialize(&target, 0, final_url);

	/* A data: URL carries its bytes. */
	is_data = link_named(target.scheme, "data");
	is_http = link_named(target.scheme, "http") || link_named(target.scheme, "https");
	if (error == 0 && is_data) {
		error = net_data_parse(&target, &data);
		if (error == 0)
			error = wb_buffer_append(bytes, data.body.data, data.body.length);
		if (error == 0)
			net_data_release(&data);
		net_url_release(&target);
		return error;
	}

	/* An http: URL is fetched; its final URL replaces the one asked for. */
	if (error == 0 && is_http) {
		wb_buffer_init(&text);
		error = net_url_serialize(&target, 0, &text);
		net_url_release(&target);
		if (error == 0)
			error = net_http_fetch(wb_buffer_string(&text), &response);
		wb_buffer_release(&text);
		if (error != 0)
			return error;
		error = wb_buffer_append(bytes, response.body.data, response.body.length);
		if (error == 0 && final_url != NULL) {
			wb_buffer_clear(final_url);
			error = wb_buffer_append(final_url, response.url.data, response.url.length);
		}

		/* The response is copied. */
		net_response_release(&response);
		return error;
	}

	/* A file: URL names a file. */
	wb_buffer_init(&path);
	if (error == 0)
		error = net_url_file_path(&target, &path);
	net_url_release(&target);
	if (error == 0)
		error = wb_file_read(wb_buffer_string(&path), bytes);
	wb_buffer_release(&path);
	if (error != 0)
		return error;

	/* Succeeded: the bytes are read. */
	return 0;
}

/*
 * Fetches an owned response with its actual resolved URL, status, declared MIME and body.
 *
 * Fresh output storage is initialized here and remains empty on every failure.
 * Existing byte-only callers retain their separate page_fetch contract.
 */
int
page_fetch_response(
	const char *base,
	const char *href,
	struct net_response *response)
{
	struct net_url target;
	struct net_data data;
	struct wb_buffer location;
	struct wb_buffer path;
	const char *mime;
	int is_data;
	int is_http;
	int is_file;
	int status;

	/* Fresh output has a single cleanup owner even when embedding arguments are invalid. */
	if (response == NULL)
		return EINVAL;
	net_response_init(response);
	if (base == NULL || href == NULL)
		return EINVAL;
	status = link_resolve(base, href, &target);
	if (status != 0)
		return status;
	wb_buffer_init(&location);
	wb_buffer_init(&path);
	is_data = link_named(target.scheme, "data");
	is_http = net_http_is_web(target.scheme);
	is_file = link_named(target.scheme, "file");

	/* HTTP metadata and final redirect URL come from the existing real network implementation. */
	if (is_http) {
		status = net_url_serialize(&target, 0, &location);
		if (status == 0)
			status = net_http_fetch(wb_buffer_string(&location), response);
	} else if (is_data) {
		/* A real data URL supplies its MIME and decoded body independently of any DOM root. */
		status = net_data_parse(&target, &data);
		if (status == 0) {
			status = wb_buffer_append(&response->body, data.body.data, data.body.length);
			if (status == 0)
				status = wb_buffer_append(&response->content_type, data.mime.data, data.mime.length);
			net_data_release(&data);
		}
	} else if (is_file) {
		/* Local resource metadata uses generic decoded extensions, never test names or XML contents. */
		status = net_url_file_path(&target, &path);
		if (status == 0)
			status = wb_file_read(wb_buffer_string(&path), &response->body);
		if (status == 0) {
			mime = link_file_mime(wb_buffer_string(&path));
			status = wb_buffer_append_string(&response->content_type, mime);
		}
	} else {
		/* Unsupported resource schemes cannot publish a normal response body or metadata. */
		status = EPROTONOSUPPORT;
	}

	/* Successful local and data responses retain their actual URL and ordinary successful status. */
	if (status == 0 && !is_http) {
		status = net_url_serialize(&target, 0, &response->url);
		if (status == 0)
			response->status = 200;
	}

	/* All independent C URL and working buffers are released before response publication. */
	net_url_release(&target);
	wb_buffer_release(&location);
	wb_buffer_release(&path);
	if (status != 0) {
		net_response_release(response);
		net_response_init(response);
		return status;
	}

	/* Succeeded: the caller owns every actual response field until normal response release. */
	return 0;
}

/* Parses a target against a location (an absolute path is a file: URL; anything else a URL). */
static int
link_resolve(
	const char *base,
	const char *href,
	struct net_url *target)
{
	struct wb_buffer base_text;
	struct net_url base_url;
	int error;

	/* The base as a URL. */
	wb_buffer_init(&base_text);
	error = 0;
	if (base[0] == '/') {
		error = net_url_from_file_path(base, &base_text);
	} else {
		error = wb_buffer_append_string(&base_text, base);
	}

	/* The base parsed. */
	if (error == 0)
		error = net_url_parse(wb_buffer_string(&base_text), base_text.length, NULL, &base_url);
	wb_buffer_release(&base_text);
	if (error != 0)
		return error;

	/* The target against it. */
	error = net_url_parse(href, strlen(href), &base_url, target);
	net_url_release(&base_url);
	if (error != 0)
		return error;

	/* Succeeded: the target is parsed. */
	return 0;
}

/* Tells whether a scheme is a name. */
static int
link_named(
	const char *scheme,
	const char *name)
{
	int differs;

	/* The two strings. */
	differs = strcmp(scheme, name);
	if (differs != 0)
		return 0;
	return 1;
}

/* Supplies generic local document metadata from a real decoded path extension. */
static const char *
link_file_mime(
	const char *path)
{
	const char *extension;
	const char *cursor;
	char lower[8];
	size_t index;
	int character;
	int same;

	/* A later directory separator invalidates a dot belonging to an enclosing directory. */
	extension = NULL;
	for (cursor = path; *cursor != '\0'; cursor++) {
		if (*cursor == '/')
			extension = NULL;
		else if (*cursor == '.')
			extension = cursor + 1;
	}

	/* Missing and long unknown extensions remain an ordinary binary resource. */
	if (extension == NULL)
		return "application/octet-stream";
	index = 0;
	while (extension[index] != '\0' && index < sizeof(lower) - 1) {
		character = (unsigned char)extension[index];
		if (character >= 'A' && character <= 'Z')
			character += 'a' - 'A';
		lower[index] = (char)character;
		index++;
	}

	/* Only complete ordinary extensions can supply local document metadata. */
	if (extension[index] != '\0')
		return "application/octet-stream";
	lower[index] = '\0';
	same = strcmp(lower, "html");
	if (same == 0)
		return "text/html";
	same = strcmp(lower, "htm");
	if (same == 0)
		return "text/html";
	same = strcmp(lower, "xhtml");
	if (same == 0)
		return "application/xhtml+xml";
	same = strcmp(lower, "xht");
	if (same == 0)
		return "application/xhtml+xml";
	same = strcmp(lower, "svg");
	if (same == 0)
		return "image/svg+xml";
	same = strcmp(lower, "xml");
	if (same == 0)
		return "application/xml";

	/* Unrecognized local formats cannot silently become HTML or XML documents. */
	return "application/octet-stream";
}
