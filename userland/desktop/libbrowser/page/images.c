/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * A page's images: the <img> sources and <object> data fetched and decoded before
 * the page is laid out, and the background images when the layout asks
 * for them, kept by their location for the life of the page (a source
 * that could not be fetched or decoded is remembered as failed, and not
 * fetched again).  With the embedder's loader, an http or https image is
 * fetched without blocking: its entry waits (no image yet) until the
 * loader's callback decodes it and counts it in the page's
 * images_generation, which lays the page out again.  Other locations (a
 * file, a data: URL), and every location without a loader, are read at
 * once.
 */

#include "page/page.h"
#include "net/net.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The deepest element nesting the walk descends (the parser caps nesting too). */
#define IMAGES_DEPTH		512

/*
 * One image of the page: the location its source resolved to (the key),
 * the decoded bitmap, whether it failed, and while it is being fetched its
 * request and the page it is for.  The table holds pointers to these, so
 * that a bitmap the layout and the display list point at does not move
 * when the table grows.
 */
struct page_image {
	char *location;
	struct img_bitmap bitmap;
	int failed;
	struct page *page;
	struct net_request *request;
};

static int images_walk(struct page *page, const struct dom_node *node, const struct vm_string *src, const struct vm_string *data, int depth);
static int images_source(struct page *page, const struct dom_element *element, const struct vm_string *src, struct wb_buffer *location, int *found);
static struct page_image *images_find(const struct page *page, const char *location);
static int images_load(struct page *page, const struct dom_element *element, const struct vm_string *src);
static int images_fetch(struct page *page, const char *location, struct page_image **loaded);
static void images_arrived(void *context, struct net_request *request);

/*
 * Starts a page's table of images empty.
 */
void
page_images_init(
	struct page *page)
{
	/* No image yet. */
	wb_vector_init(&page->images, sizeof(struct page_image *));
}

/*
 * Fetches and decodes the source of every <img> of the document that is
 * not in the page's table yet.
 */
int
page_load_images(
	struct page *page)
{
	struct vm_string *src;
	struct vm_string *data;
	int error;

	/* The attributes' names, as the atoms the elements keep. */
	src = vm_atom_from_ascii(page->heap, "src");
	if (src == NULL)
		return ENOMEM;
	data = vm_atom_from_ascii(page->heap, "data");
	if (data == NULL)
		return ENOMEM;

	/* Every image and object element of the document. */
	error = images_walk(page, &page->document->node, src, data, 0);
	if (error != 0)
		return error;

	/* Succeeded: every image the document names is in the table. */
	return 0;
}

/*
 * Finds the decoded image an <img> element shows, for the layout (its
 * context is the page); NULL for an element with no source, or one that
 * failed or was not loaded.
 */
const struct img_bitmap *
page_image_of(
	void *context,
	const struct dom_element *element)
{
	struct page *page;
	struct page_image *image;
	struct vm_string *source;
	const char *source_name;
	struct wb_buffer location;
	int found;
	int error;

	/* The element's source or data, resolved against the page's location. */
	page = context;
	source_name = "src";
	if (element->tag == DOM_TAG_OBJECT)
		source_name = "data";
	source = vm_atom_from_ascii(page->heap, source_name);
	if (source == NULL)
		return NULL;
	wb_buffer_init(&location);
	error = images_source(page, element, source, &location, &found);
	if (error != 0 || !found) {
		wb_buffer_release(&location);
		return NULL;
	}

	/* The image of that location, when it was decoded (not while it is fetched). */
	image = images_find(page, wb_buffer_string(&location));
	wb_buffer_release(&location);
	if (image == NULL || image->failed || image->request != NULL)
		return NULL;

	/* Succeeded: the image's bitmap. */
	return &image->bitmap;
}

/*
 * Finds the decoded image a style's URL names (resolved against the
 * page's location), fetching and decoding it the first time, for the
 * layout (its context is the page); NULL when it cannot be had.
 */
const struct img_bitmap *
page_image_by_url(
	void *context,
	const struct vm_string *url)
{
	struct page *page;
	struct page_image *image;
	struct wb_buffer text;
	struct wb_buffer location;
	int error;

	/* A page without a location resolves nothing. */
	page = context;
	if (page->base == NULL)
		return NULL;

	/* The URL's text. */
	wb_buffer_init(&text);
	wb_buffer_init(&location);
	error = vm_string_to_utf8(url, &text);

	/* Its location against the page's. */
	if (error == 0)
		error = page_resolve_location(page->base, wb_buffer_string(&text), &location);
	wb_buffer_release(&text);
	if (error != 0) {
		wb_buffer_release(&location);
		return NULL;
	}

	/* The image of that location, loaded now when it is new. */
	error = images_fetch(page, wb_buffer_string(&location), &image);
	wb_buffer_release(&location);
	if (error != 0 || image->failed || image->request != NULL)
		return NULL;

	/* Succeeded: the image's bitmap. */
	return &image->bitmap;
}

/*
 * Frees the page's images.
 */
void
page_images_release(
	struct page *page)
{
	struct page_image *image;
	size_t index;

	/* Each image's location and pixels, then the table. */
	for (index = 0; index < page->images.count; index++) {
		image = *(struct page_image **)wb_vector_at(&page->images, index);
		net_request_cancel(image->request);
		free(image->location);
		img_bitmap_release(&image->bitmap);
		free(image);
	}

	/* The table itself. */
	wb_vector_release(&page->images);
}

/* Loads the images of a node's element descendants (and its own, when it is an <img> or <object>). */
static int
images_walk(
	struct page *page,
	const struct dom_node *node,
	const struct vm_string *src,
	const struct vm_string *data,
	int depth)
{
	const struct dom_element *element;
	const struct dom_node *child;
	int error;

	/* Stops at the depth the parser stops at. */
	if (depth > IMAGES_DEPTH)
		return 0;

	/* An HTML <img> loads its source; an <object> tries its data as an image for replaced rendering. */
	if (node->type == DOM_ELEMENT) {
		element = (const struct dom_element *)node;
		if (element->ns == DOM_NS_HTML && element->tag == DOM_TAG_IMG) {
			error = images_load(page, element, src);
			if (error != 0)
				return error;
		}

		/* An object tries its data as an image independently of an image before it. */
		if (element->ns == DOM_NS_HTML && element->tag == DOM_TAG_OBJECT) {
			error = images_load(page, element, data);
			if (error != 0)
				return error;
		}
	}

	/* The children, in document order. */
	for (child = node->first_child; child != NULL; child = child->next) {
		error = images_walk(page, child, src, data, depth + 1);
		if (error != 0)
			return error;
	}

	/* Succeeded: the subtree's images are loaded. */
	return 0;
}

/*
 * Writes an element's source resolved against the page's location;
 * *found is 0 for an element with no source (or a page with no location
 * to resolve it against).
 */
static int
images_source(
	struct page *page,
	const struct dom_element *element,
	const struct vm_string *src,
	struct wb_buffer *location,
	int *found)
{
	const struct dom_attribute *attribute;
	struct wb_buffer href;
	int error;

	/* The src attribute, when there is one and the page has a location. */
	*found = 0;
	attribute = dom_element_find_attribute(element, DOM_NS_NONE, src);
	if (attribute == NULL || page->base == NULL)
		return 0;

	/* Its value. */
	wb_buffer_init(&href);
	error = vm_string_to_utf8(attribute->value, &href);
	if (error != 0) {
		wb_buffer_release(&href);
		return error;
	}

	/* The location it names; a source that is not a URL has none. */
	error = page_resolve_location(page->base, wb_buffer_string(&href), location);
	wb_buffer_release(&href);
	if (error == EINVAL)
		return 0;
	if (error != 0)
		return error;

	/* Succeeded: the location is written. */
	*found = 1;
	return 0;
}

/* Finds an image of the table by its location. */
static struct page_image *
images_find(
	const struct page *page,
	const char *location)
{
	struct page_image *image;
	size_t index;
	int differs;

	/* Each image of the table. */
	for (index = 0; index < page->images.count; index++) {
		image = *(struct page_image **)wb_vector_at(&page->images, index);
		differs = strcmp(image->location, location);
		if (differs == 0)
			return image;
	}

	/* The location is not in the table. */
	return NULL;
}

/*
 * Loads an element's image location into the table, unless it is there already: its
 * bytes fetched and decoded, or the failure remembered.
 */
static int
images_load(
	struct page *page,
	const struct dom_element *element,
	const struct vm_string *src)
{
	struct wb_buffer location;
	struct page_image *known;
	int found;
	int error;

	/* The source's location; an element without one loads nothing. */
	wb_buffer_init(&location);
	error = images_source(page, element, src, &location, &found);
	if (error != 0 || !found) {
		wb_buffer_release(&location);
		return error;
	}

	/* The location's image, fetched when it is new. */
	error = images_fetch(page, wb_buffer_string(&location), &known);
	wb_buffer_release(&location);
	if (error != 0)
		return error;

	/* Succeeded: the image is in the table. */
	return 0;
}

/*
 * Finds a location's image in the table, or fetches and decodes it into
 * a new entry (a failure to fetch or decode is remembered in the entry).
 */
static int
images_fetch(
	struct page *page,
	const char *location,
	struct page_image **loaded)
{
	struct page_image *image;
	struct page_image *known;
	struct wb_buffer bytes;
	int remote;
	int error;

	/* A location loaded before, well or not, is not fetched again. */
	known = images_find(page, location);
	if (known != NULL) {
		*loaded = known;
		return 0;
	}

	/* The table's entry for the location, allocated alone so that its bitmap stays where it is while the table grows. */
	image = calloc(1, sizeof(*image));
	if (image == NULL)
		return ENOMEM;

	/* Its location. */
	image->location = strdup(location);
	if (image->location == NULL) {
		free(image);
		return ENOMEM;
	}

	/* An http or https image with a loader is fetched without blocking; it waits in the table meanwhile. */
	image->page = page;
	remote = net_loader_takes(image->location);
	if (page->loader != NULL && remote) {
		error = net_loader_fetch(page->loader, image->location, images_arrived, image, &image->request);
		if (error != 0)
			image->failed = 1;
	} else {
		/* The bytes, then the decoding; either failing marks the image failed. */
		wb_buffer_init(&bytes);
		error = page_fetch(page->base, image->location, &bytes, NULL);
		if (error == 0)
			error = img_decode(bytes.data, bytes.length, &image->bitmap);
		wb_buffer_release(&bytes);
		if (error != 0)
			image->failed = 1;
	}

	/* The entry goes into the table. */
	error = wb_vector_push(&page->images, &image);
	if (error != 0) {
		net_request_cancel(image->request);
		free(image->location);
		img_bitmap_release(&image->bitmap);
		free(image);
		return error;
	}

	/* Succeeded: the image is in the table. */
	*loaded = image;
	return 0;
}

/*
 * The loader's callback for an image: its body decoded when it came (a
 * failure, or a status other than 2xx, marks the image failed), and the
 * page told to lay itself out again.
 */
static void
images_arrived(
	void *context,
	struct net_request *request)
{
	const struct net_response *response;
	struct page_image *image;
	int error;

	/* The image, which waits no longer. */
	image = context;
	image->request = NULL;

	/* A response with the image's bytes, decoded. */
	error = net_request_error(request);
	response = net_request_response(request);
	if (error == 0 && (response->status < 200 || response->status > 299))
		error = EINVAL;
	if (error == 0)
		error = img_decode(response->body.data, response->body.length, &image->bitmap);
	if (error != 0)
		image->failed = 1;

	/* The page is laid out again with it. */
	image->page->images_generation++;
}
