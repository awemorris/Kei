/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies actual Document.images caches and collection-only child lifetime during real GC. */

#include "bind/internal.h"
#include "html/html.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Independent image collection lifetime observations survive fixture teardown. */
static unsigned checks;

/* All failed observations remain visible in the final native result. */
static unsigned failures;

static void images_check(int condition, const char *name);
static int images_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int images_case(struct vm_realm *realm);

/*
 * Runs native image collection observations against a genuine managed iframe context.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	int status;
	int printed;

	/* Ordinary fixture installation supplies genuine managed Window and iframe state. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;

	/* Protects ordinary native installation with its construction stack. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));

	/* Creates the realm whose prototypes supply the actual Document bindings. */
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Builtins and primary Document own the production binding graph. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* A primary Window is installed on an initially empty native Document. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The fixture uses ordinary child binding ownership without host callbacks. */
	memset(&host, 0, sizeof(host));

	/* Installs the primary Window after preparing its empty host. */
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Only the direct lifetime case disables conservative stack scanning. */
	status = images_case(realm);
	if (status != 0) {
		vm_heap_set_stack_base(heap, __builtin_frame_address(0));
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Restores ordinary stack scanning before releasing the primary graph. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Publish complete observations after the embedding has released its roots. */
	printed = printf("native Document.images lifetime: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any missing image graph or lingering child context rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native image collection caches and context finalization were verified. */
	return 0;
}

/* Records one native observation while preserving later independent checks. */
static void
images_check(
	int condition,
	const char *name)
{
	int printed;

	/* Keep every failed identity or lifetime check in the fixture's final result. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: this observation remains in the final result. */
	return;
}

/* Constructs genuine DOM and managed contexts through the ordinary interpreter. */
static int
images_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* The ordinary interpreter constructs the same iframe binding graph as a Page. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Ordinary script execution provides genuine managed iframe state. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Releases source storage after the interpreter has stopped borrowing it. */
	wb_units_release(&units);

	/* Succeeded: a normal interpreter value is available to the embedding. */
	return 0;
}

/* Checks actual image collection-only ownership without conservative native stack roots. */
static int
images_case(
	struct vm_realm *realm)
{
	struct dom_node *frame;
	struct dom_node *member;
	struct dom_node *container;
	struct vm_realm *child_realm;
	struct bind_window *child;
	struct vm_object *collection;
	struct vm_cell *root;
	struct vm_cell *found;
	vm_value answer;
	vm_value receiver;
	vm_value expected;
	uintptr_t child_address;
	uintptr_t document_address;
	uintptr_t collection_address;
	uintptr_t member_address;
	int status;

	/* Genuine child bindings install the cache and discard all direct script references. */
	status = images_script(realm,
			       "var root=document.createElement('div');document.appendChild(root);"
			       "var f=document.createElement('iframe');root.appendChild(f);"
			       "var d=f.contentDocument;d.open();d.write('<img id=member>');d.close();d.images;f",
			       &answer);
	if (status != 0)
		return status;

	/* Rejects absent or non-element native frame bindings before a cast. */
	frame = bind_node_of(answer);
	if (frame == NULL || frame->type != DOM_ELEMENT)
		return EINVAL;

	/* Requires the iframe's genuine managed realm. */
	child_realm = (struct vm_realm *)((struct dom_element *)frame)->child_context;
	if (child_realm == NULL)
		return EINVAL;

	/* Resolves the Window that owns the installed child Document. */
	child = child_realm->host;
	if (child == NULL)
		return EINVAL;

	/* Requires the actual SameObject image collection created by the script. */
	collection = child->document->images_collection;
	if (collection == NULL)
		return EINVAL;

	/* Checks the parsed HTML tree before resolving its body. */
	container = child->document->node.last_child;
	if (container == NULL)
		return EINVAL;

	/* Checks the parsed body before reading its first image. */
	container = container->last_child;
	if (container == NULL)
		return EINVAL;

	/* Requires the actual image whose lifetime the collection should retain. */
	member = container->first_child;
	if (member == NULL)
		return EINVAL;

	/* Integer addresses observe collection without adding conservative roots. */
	child_address = (uintptr_t)&child_realm->cell;
	document_address = (uintptr_t)&child->document->node.cell;
	collection_address = (uintptr_t)&collection->cell;
	member_address = (uintptr_t)&member->cell;

	/* Removes script aliases before testing the connected Document's native trace. */
	status = images_script(realm, "d=null;f=null", &answer);
	if (status != 0)
		return status;

	/* The ordinary connected child Document alone must trace its SameObject cache. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, collection_address);
	images_check(found != NULL, "actual native Document trace retains image cache without stack roots");

	/* Reads the SameObject cache through the actual native Document getter. */
	receiver = vm_value_cell(child->document->node.wrapper);
	status = bind_document_images(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	expected = vm_value_cell(collection);
	images_check(answer == expected, "borrowed native getter returns identical cached object after GC");

	/* A saved collection becomes the sole registered edge after native frame retirement. */
	root = &collection->cell;
	status = vm_heap_add_root(realm->heap, &root);
	if (status != 0)
		return status;
	dom_remove(frame);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	images_check(found != NULL && child->detached, "collection-only root retains retired actual child realm");
	found = vm_heap_find_cell(realm->heap, document_address);
	images_check(found != NULL, "collection-only root retains actual child Document");
	found = vm_heap_find_cell(realm->heap, member_address);
	images_check(found != NULL, "collection-only root retains actual image member");

	/* Queries the retired collection through its actual branded native getter. */
	receiver = vm_value_cell(collection);
	status = bind_html_collection_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
	if (status != 0) {
		vm_heap_remove_root(realm->heap, &root);
		return status;
	}

	/* Compares the surviving native collection count with its genuine member. */
	expected = vm_value_int32(1);
	images_check(answer == expected, "retired native image list remains live and branded after GC");

	/* Removing the last edge must collect the Document/cache/member/context cycle. */
	vm_heap_remove_root(realm->heap, &root);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	images_check(found == NULL, "released image collection cycle does not leak child realm");
	found = vm_heap_find_cell(realm->heap, document_address);
	images_check(found == NULL, "released image collection cycle collects native Document");
	found = vm_heap_find_cell(realm->heap, collection_address);
	images_check(found == NULL, "released image collection cycle collects cached wrapper");
	found = vm_heap_find_cell(realm->heap, member_address);
	images_check(found == NULL, "released image collection cycle collects native image member");

	/* Succeeded: cache identity, native rooted membership and eventual reclamation were verified. */
	return 0;
}
