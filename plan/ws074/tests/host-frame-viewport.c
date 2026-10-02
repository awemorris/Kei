/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies direct native viewport refresh and context ownership during real host callback GC. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One host embedding box and callback action, owned by the synchronous native fixture. */
struct viewport_fixture {
	struct vm_heap *heap;
	struct bind_box box;
	unsigned callbacks;
	int found;
	int retire;
};

/* Independent dimension and lifetime observations survive fixture teardown. */
static unsigned checks;
/* All failed observations remain visible in the final native result. */
static unsigned failures;

static void viewport_check(int condition, const char *name);
static int viewport_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int viewport_case(struct vm_realm *realm, struct bind_window *window, struct viewport_fixture *fixture);
static int viewport_box(void *context, struct dom_node *node, struct bind_box *box);

/*
 * Runs native viewport refresh against a genuine managed iframe context.
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
	struct viewport_fixture fixture;
	int status;
	int printed;

	/* Ordinary fixture installation supplies genuine managed Window and iframe state. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
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

	/* No alternate host removal callback changes the native path under test. */
	memset(&fixture, 0, sizeof(fixture));
	fixture.heap = heap;
	fixture.found = 1;
	memset(&host, 0, sizeof(host));
	host.context = &fixture;
	host.node_box = viewport_box;
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Direct native invocation disables conservative stack scanning only inside this case. */
	status = viewport_case(realm, window, &fixture);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Publish complete observations after the embedding has released its roots. */
	printed = printf("native iframe viewport: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any missing collection, wrong record or lingering detached cell rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native viewport observation and notification graph lifetime were verified. */
	return 0;
}

/* Records one native observation while preserving later independent checks. */
static void
viewport_check(
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
}

/* Constructs genuine DOM and observer participants through the ordinary interpreter. */
static int
viewport_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* The browser parser sees exactly the script syntax the Page probe uses. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Ordinary script execution provides the actual registered Range private cell. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: a normal interpreter value is available to the embedding. */
	return 0;
}

/* Resolves embedding geometry while deliberately collecting or retiring the child. */
static int
viewport_box(
	void *context,
	struct dom_node *node,
	struct bind_box *box)
{
	struct viewport_fixture *fixture;

	/* Retiring the connected frame removes its child edge before actual callback GC. */
	fixture = context;
	fixture->callbacks++;
	if (fixture->retire)
		dom_remove(node);

	/* Only production helper roots protect a child retired inside this host callback. */
	vm_heap_collect(fixture->heap);
	*box = fixture->box;

	/* Return the real fixture host's box availability, without a production test control. */
	return fixture->found;
}

/* Checks active content dimensions and collectible retirement during a real host callback. */
static int
viewport_case(
	struct vm_realm *realm,
	struct bind_window *window,
	struct viewport_fixture *fixture)
{
	struct vm_realm *child_realm;
	struct dom_node *frame_node;
	struct bind_window *child;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value answer;
	uintptr_t child_address;
	unsigned callbacks;
	int status;

	/* Build a genuine connected iframe and then discard script references to its child. */
	status = viewport_script(realm,
	    "var root=document.createElement('div');document.appendChild(root);"
	    "var f=document.createElement('iframe');root.appendChild(f);var w=f.contentWindow;f", &answer);
	if (status != 0)
		return status;
	frame_node = bind_node_of(answer);
	child_realm = (struct vm_realm *)((struct dom_element *)frame_node)->child_context;
	child = child_realm->host;
	child_address = (uintptr_t)&child_realm->cell;
	status = viewport_script(realm, "w=null;f=null", &answer);
	if (status != 0)
		return status;

	/* Exact zero is not replaced with primary viewport dimensions. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_stats(realm->heap, &before);
	status = bind_frame_viewport(child);
	vm_heap_stats(realm->heap, &after);
	if (status != 0)
		return status;
	viewport_check(child->viewport_width == 0 && child->viewport_height == 0, "zero box produces exact zero child viewport");
	viewport_check(after.collections > before.collections, "real layout callback GC occurs with stack scanning disabled");

	/* Used border and padding are excluded independently on both axes. */
	fixture->box.width = 140;
	fixture->box.height = 130;
	fixture->box.border_left = 2;
	fixture->box.border_right = 3;
	fixture->box.padding_left = 15;
	fixture->box.padding_right = 20;
	fixture->box.border_top = 3;
	fixture->box.border_bottom = 7;
	fixture->box.padding_top = 5;
	fixture->box.padding_bottom = 15;
	status = bind_frame_viewport(child);
	if (status != 0)
		return status;
	viewport_check(child->viewport_width == 100 && child->viewport_height == 100, "actual content box subtracts native borders and padding");

	/* An absent embedding box has no rendered child viewport. */
	fixture->found = 0;
	status = bind_frame_viewport(child);
	if (status != 0)
		return status;
	viewport_check(child->viewport_width == 0 && child->viewport_height == 0, "no host box produces zero viewport");

	/* Primary windows preserve their embedder-supplied dimensions and do not call this box host. */
	bind_window_set_viewport(window, 800, 600);
	callbacks = fixture->callbacks;
	status = bind_frame_viewport(window);
	if (status != 0)
		return status;
	viewport_check(window->viewport_width == 800 && window->viewport_height == 600 && fixture->callbacks == callbacks, "primary embedding viewport is unchanged");

	/* Empty host geometry preserves the existing explicitly supplied child viewport contract. */
	window->host.node_box = NULL;
	bind_window_set_viewport(child, 400, 300);
	status = bind_frame_viewport(child);
	if (status != 0)
		return status;
	viewport_check(child->viewport_width == 400 && child->viewport_height == 300, "no-layout host retains explicit child viewport");
	window->host.node_box = viewport_box;

	/* Negative native content extents clamp without affecting the other axis. */
	fixture->found = 1;
	fixture->box.width = 1;
	status = bind_frame_viewport(child);
	if (status != 0)
		return status;
	viewport_check(child->viewport_width == 0 && child->viewport_height == 100, "negative content width clamps independently");

	/* Unrepresentable host dimensions are refused before any integer publication. */
	fixture->box.width = 1.0e30;
	status = bind_frame_viewport(child);
	viewport_check(status == EOVERFLOW && child->viewport_width == 0 && child->viewport_height == 100, "out-of-int geometry leaves previous viewport unchanged");

	/* Retirement destroys the ordinary frame edge before GC while the helper alone owns the child. */
	fixture->box.width = 140;
	fixture->retire = 1;
	status = bind_frame_viewport(child);
	if (status != 0)
		return status;
	viewport_check(child->detached && child->frame == NULL && child->viewport_width == 0, "callback retirement preserves detached state instead of publishing geometry");

	/* Released helper roots leave the retired managed context collectible. */
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	viewport_check(found == NULL, "retired child collectible after temporary callback ownership ends");
	vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));

	/* Succeeded: content dimensions, fallback and callback retirement were observed. */
	return 0;
}
