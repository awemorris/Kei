/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies actual child Page geometry and temporary owner roots during callback GC. */

#include "bind/internal.h"
#include "page/page.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One synchronous parent embedding callback deliberately collects or retires its frame. */
struct image_fixture {
	struct vm_heap *heap;
	unsigned callbacks;
	int retire;
};

/* Count independent geometry and lifetime observations across the native fixture. */
static unsigned checks;

/* Preserve failed observations until the final result is printed. */
static unsigned failures;

static void image_check(int condition, const char *name);
static int image_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int image_case(struct page *page);
static int image_box(void *context, struct dom_node *node, struct bind_box *box);

/*
 * Runs real Page layout and native image getter lifetime observations.
 */
int
main(
	void)
{
	struct page *page;
	struct text_font_paths paths;
	static const unsigned char html[] = "<!doctype html><html><body></body></html>";
	int status;
	int printed;

	/* Ordinary checked-in fonts supply real layout without a fixture implementation. */
	paths.sans = "userland/desktop/fonts/Inter.ttf";
	paths.mono = "userland/desktop/fonts/JetBrainsMono-Regular.ttf";
	paths.fallback = "userland/desktop/fonts/DroidSansFallbackFull.ttf";

	/* Allocates the real Page before installing its borrowed font paths. */
	status = page_create(&page, __builtin_frame_address(0));
	if (status != 0)
		return 2;

	/* Gives ordinary layout its fonts and primary viewport. */
	page_set_fonts(page, &paths);
	page_set_viewport(page, 800, 600);

	/* Load a genuine primary Page before constructing the binding-owned child. */
	status = page_load_html(page, html, sizeof(html) - 1U);
	if (status != 0) {
		page_destroy(page);
		return 2;
	}

	/* Exercises child layout before releasing the primary Page. */
	status = image_case(page);
	if (status != 0) {
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		page_destroy(page);
		return 2;
	}

	/* Restores normal stack ownership before releasing the complete Page. */
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	page_destroy(page);

	/* Publish complete observations after every production resource was released. */
	printed = printf("native image geometry: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any failed independent geometry or lifetime check rejects this fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: actual layout and callback lifetime were verified. */
	return 0;
}

/* Records an independent layout or lifetime observation. */
static void
image_check(
	int condition,
	const char *name)
{
	int printed;

	/* Retain all failures while later independent observations still run. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation remains in the final result. */
	return;
}

/* Executes ordinary scripts to construct production managed child contexts. */
static int
image_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Convert fixture input to the interpreter's ordinary source representation. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Execute genuine DOM operations without a production test switch. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Releases source storage after the interpreter has finished borrowing it. */
	wb_units_release(&units);

	/* Succeeded: the script supplied actual native state. */
	return 0;
}

/* Supplies embedding geometry while actual GC challenges temporary child ownership. */
static int
image_box(
	void *context,
	struct dom_node *node,
	struct bind_box *box)
{
	struct image_fixture *fixture;

	/* A removed iframe loses its permanent child edge before callback collection. */
	fixture = context;
	fixture->callbacks++;
	if (fixture->retire)
		dom_remove(node);

	/* Only the production geometry helper's roots protect the retired child. */
	vm_heap_collect(fixture->heap);

	/* Ordinary host geometry supplies a real content viewport to the child engine. */
	memset(box, 0, sizeof(*box));
	box->width = 120;
	box->height = 80;

	/* Succeeded: the embedding has an actual host box. */
	return 1;
}

/* Checks native Page layout and image getter roots through actual callback retirement. */
static int
image_case(
	struct page *page)
{
	struct dom_node *node;
	struct bind_window *owner;
	struct bind_box box;
	struct image_fixture fixture;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value receiver;
	vm_value answer;
	uintptr_t owner_address;
	uintptr_t document_address;
	uintptr_t image_address;
	int status;
	int laid_out;

	/* A real child HTML stream and cascade create the queried native image. */
	status = image_script(page->realm,
			      "var f=document.createElement('iframe');f.style.cssText='display:block;width:120px;height:80px';document.body.appendChild(f);"
			      "var d=f.contentDocument;d.open();d.write('<style>html,body{margin:0;height:100%}img{display:block;width:50%;height:25%}</style><img height=7>');d.close();d.images[0]",
			      &receiver);
	if (status != 0)
		return status;

	/* Rejects failed native binding before reading its owner graph. */
	node = bind_node_of(receiver);
	if (node == NULL)
		return EINVAL;

	/* The genuine child image must have a managed layout owner. */
	owner = node->document->view;
	if (owner == NULL)
		return EINVAL;

	/* Integer addresses observe collection without adding conservative roots. */
	owner_address = (uintptr_t)&owner->realm->cell;
	document_address = (uintptr_t)&node->document->node.cell;
	image_address = (uintptr_t)&node->cell;

	/* Native Page queries borrow ordinary fonts and the actual child content viewport. */
	laid_out = page_node_box(page, node, &box);
	image_check(laid_out == 1 && box.width == 60 && box.height == 20, "real child layout resolves image percentages");

	/* Reads rendered height through the actual native image binding. */
	status = bind_html_image_element_interface.attributes[3].getter(page->realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	image_check(answer == vm_value_int32(20), "direct native getter returns actual Page content height");

	/* Removes script aliases before testing native ownership alone. */
	status = image_script(page->realm, "f=null;d=null", &answer);
	if (status != 0)
		return status;

	/* The child keeps its original Page host while its parent's viewport callback collects. */
	memset(&fixture, 0, sizeof(fixture));
	fixture.heap = page->heap;
	page->window->host.context = &fixture;
	page->window->host.node_box = image_box;
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_stats(page->heap, &before);
	laid_out = page_node_box(page, node, &box);
	vm_heap_stats(page->heap, &after);
	image_check(laid_out == 1 && box.height == 20, "direct Page query protects actual child graph through callback GC");
	image_check(after.collections > before.collections && fixture.callbacks != 0, "real callback GC runs without conservative stack roots");

	/* Repeats the actual image getter while callback collection is active. */
	status = bind_html_image_element_interface.attributes[3].getter(page->realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	image_check(answer == vm_value_int32(20), "direct getter retains owner and native image through actual GC");

	/* Retirement inside the real embedding callback must refuse stale rendered dimensions. */
	fixture.retire = 1;
	status = bind_html_image_element_interface.attributes[3].getter(page->realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	image_check(answer == vm_value_int32(7), "callback retirement returns nonrendered attribute fallback");
	image_check(owner->detached && owner->frame == NULL, "callback retirement preserves actual detached owner state");

	/* All temporary geometry roots disappear after the getter returns. */
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, owner_address);
	image_check(found == NULL, "retired owner realm is collectible after getter");
	found = vm_heap_find_cell(page->heap, document_address);
	image_check(found == NULL, "transient layout retains no retired Document");
	found = vm_heap_find_cell(page->heap, image_address);
	image_check(found == NULL, "transient layout retains no queried native image");
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	page->window->host.context = page;
	page->window->host.node_box = page_node_box;

	/* Succeeded: real layout and collectible callback retirement were observed. */
	return 0;
}
