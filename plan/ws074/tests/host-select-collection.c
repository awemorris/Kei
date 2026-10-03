/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks select cache cycles and sole live options retention using actual production GC. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Assertions accumulate across callback-driven and embedding-driven collection. */
static unsigned checks;

/* Failed behavioral checks determine the final exit status. */
static unsigned failures;

/* The embedding restores this construction stack boundary after explicit collection. */
static const void *construction_stack;

static void collection_check(int condition, const char *name);
static int collection_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int collection_case(struct vm_heap *heap);

/*
 * Verifies select collection cache and collectible DOM cycles through ordinary bindings.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	int error;
	int printed;

	/* The collector uses real construction stacks until a test explicitly excludes them. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;

	/* Retains the construction boundary for later stack-excluded observations. */
	construction_stack = __builtin_frame_address(0);

	/* Resume ordinary native construction after stack-excluded collection. */
	vm_heap_set_stack_base(heap, construction_stack);

	/* Exercises actual native cache ownership before releasing the heap. */
	error = collection_case(heap);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Releases the completed fixture's heap after its graphs have been observed. */
	vm_heap_destroy(heap);

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("select collection GC checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Every independently observed GC contract must hold. */
	if (failures != 0)
		return 1;

	/* Succeeded: selected collection cache and root graphs survived actual GC. */
	return 0;
}

/* Records one named ownership contract without hiding later independent observations. */
static void
collection_check(
	int condition,
	const char *name)
{
	int printed;

	/* Each observation contributes to the final assertion accounting. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the named contract was recorded. */
	return;
}

/* Executes ordinary fixture script through the production parser and interpreter. */
static int
collection_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int error;

	/* Converts input before executing the production bindings. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* The actual script engine creates every node and collection wrapper. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Releases source storage after the interpreter has finished borrowing it. */
	wb_units_release(&units);

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Tests the select options cache and a sole live collection across actual tracing. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct dom_element *select;
	struct dom_node *node;
	struct vm_cell *found;
	uintptr_t addresses[3];
	vm_value answer;
	unsigned index;
	int same;
	int error;

	/* A live manual primary supplies native prototypes for the collectible XML select. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;

	/* Installs ordinary native builtin prototypes in the completed realm. */
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* No callback or hidden engine switch controls reachability in this fixture. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* The primary host schedules no external effects and outlives each observation. */
	memset(&host, 0, sizeof(host));

	/* Installs the primary Window after preparing its empty host. */
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Only the XML select remains globally reachable after its native cache is created. */
	error = collection_script(realm,
				  "var xml=document.implementation.createDocument('http://www.w3.org/1999/xhtml','select');"
				  "var table=xml.documentElement;var option=xml.createElementNS('http://www.w3.org/1999/xhtml','option');"
				  "option.id='held';table.add(option);table.options;xml=null;option=null;table",
				  &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Rejects absent or non-element bindings before observing native collection fields. */
	node = bind_node_of(answer);
	if (node == NULL || node->type != DOM_ELEMENT) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return EINVAL;
	}

	/* Integer addresses observe ownership without retaining any native graph. */
	select = (struct dom_element *)node;
	addresses[0] = (uintptr_t)select;
	addresses[1] = (uintptr_t)select->options_collection;
	addresses[2] = (uintptr_t)select->node.first_child;
	select = NULL;
	answer = VM_VALUE_UNDEFINED;

	/* Actual root-element tracing must preserve every cache with all C stacks excluded. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	for (index = 0; index < 3U; index++) {
		found = vm_heap_find_cell(heap, addresses[index]);
		collection_check(found != NULL, "select-only root retains native options cache/member graph");
	}

	/* The native cache field identities must remain stable after a real collection. */
	found = vm_heap_find_cell(heap, addresses[0]);
	if (found == NULL) {
		vm_heap_set_stack_base(heap, construction_stack);
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return EINVAL;
	}

	/* Reads the surviving select's actual SameObject cache field. */
	select = (struct dom_element *)found;
	collection_check((uintptr_t)select->options_collection == addresses[1], "select options cache SameObject after GC");

	/* Resume ordinary native construction after stack-excluded collection. */
	vm_heap_set_stack_base(heap, construction_stack);

	/* A sole select.options collection retains its actual root, Document prototypes and remaining cache cycles. */
	error = collection_script(realm, "var saved=table.options;table=null;saved", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* No native temporary participates in this sole-collection reachability observation. */
	select = NULL;
	answer = VM_VALUE_UNDEFINED;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	for (index = 0; index < 3U; index++) {
		found = vm_heap_find_cell(heap, addresses[index]);
		collection_check(found != NULL, "sole options collection retains native owner/cache/member graph");
	}

	/* Ordinary native construction resumes with the original stack boundary. */
	vm_heap_set_stack_base(heap, construction_stack);

	/* Public native methods remain live after collection, using the original XML prototype snapshot. */
	error = collection_script(realm,
				  "saved.length===1&&saved.item(0).id==='held'&&saved.namedItem('held')===saved[0]", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Record actual public indexed, named and method observations after GC. */
	same = vm_to_boolean(answer);
	collection_check(same, "sole saved options native APIs and XML prototypes remain usable");

	/* Removes the actual member and queries the same live collection. */
	error = collection_script(realm, "saved[0].remove();saved.length===0&&saved.held===undefined", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Mutation changes the saved native collection rather than an obsolete snapshot. */
	same = vm_to_boolean(answer);
	collection_check(same, "saved options remains live after native member removal");

	/* Discards the last script edge before testing collection of the complete cycle. */
	error = collection_script(realm, "saved=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* No root may keep the select, its member or any native options cache cycle alive. */
	answer = VM_VALUE_UNDEFINED;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	for (index = 0; index < 3U; index++) {
		found = vm_heap_find_cell(heap, addresses[index]);
		collection_check(found == NULL, "last select collection root release reclaims cache/member graph");
	}

	/* Ordinary native construction resumes with the original stack boundary. */
	vm_heap_set_stack_base(heap, construction_stack);

	/* The manual primary tears down after all native ownership contracts have been observed. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: native cache retention and final cycle collection were observed. */
	return 0;
}
