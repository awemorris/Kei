/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks table cache cycles and sole cells-collection retention using actual production GC. */

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
 * Verifies table collection caches and collectible DOM cycles through ordinary bindings.
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
	construction_stack = __builtin_frame_address(0);
	vm_heap_set_stack_base(heap, construction_stack);
	error = collection_case(heap);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Release the completed fixture heap before reporting its observations. */
	vm_heap_destroy(heap);

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("table collection GC checks: %u/%u passed\n", checks - failures, checks);
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

	/* Release the borrowed script input after checking execution. */
	wb_units_release(&units);

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Tests the four table-family caches and a sole live collection across actual tracing. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct dom_node *node;
	struct dom_element *table;
	struct dom_element *body;
	struct dom_element *row;
	struct vm_cell *found;
	uintptr_t addresses[7];
	vm_value answer;
	unsigned index;
	int same;
	int error;

	/* A live manual primary supplies native prototypes while the XML table graph remains collectible. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
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
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Only the table remains globally reachable after all four native caches have been created. */
	error = collection_script(realm,
				  "var xml=document.implementation.createDocument('http://www.w3.org/1999/xhtml','table');"
				  "var table=xml.documentElement;var body=xml.createElementNS('http://www.w3.org/1999/xhtml','tbody');"
				  "var row=xml.createElementNS('http://www.w3.org/1999/xhtml','tr');"
				  "var cell=xml.createElementNS('http://www.w3.org/1999/xhtml','td');cell.id='held';"
				  "table.appendChild(body);body.appendChild(row);row.appendChild(cell);"
				  "table.tBodies;table.rows;body.rows;row.cells;xml=null;body=null;row=null;cell=null;table",
				  &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Validate generated nodes before observing their unrooted integer addresses. */
	node = bind_node_of(answer);
	if (node == NULL || node->type != DOM_ELEMENT) {
		error = EIO;
		goto cleanup;
	}

	/* Follow the checked table into its generated section. */
	table = (struct dom_element *)node;
	node = table->node.first_child;
	if (node == NULL || node->type != DOM_ELEMENT) {
		error = EIO;
		goto cleanup;
	}

	/* Follow the checked section into its generated row. */
	body = (struct dom_element *)node;
	node = body->node.first_child;
	if (node == NULL || node->type != DOM_ELEMENT) {
		error = EIO;
		goto cleanup;
	}

	/* Inspect the checked row and every materialized cache. */
	row = (struct dom_element *)node;
	if (table->bodies_collection == NULL ||
	    table->rows_collection == NULL ||
	    body->rows_collection == NULL ||
	    row->cells_collection == NULL ||
	    row->node.first_child == NULL) {
		error = EIO;
		goto cleanup;
	}

	/* Integer addresses supply no collector root. */
	addresses[0] = (uintptr_t)table;
	addresses[1] = (uintptr_t)table->bodies_collection;
	addresses[2] = (uintptr_t)table->rows_collection;
	addresses[3] = (uintptr_t)body->rows_collection;
	addresses[4] = (uintptr_t)row->cells_collection;
	addresses[5] = (uintptr_t)row;
	addresses[6] = (uintptr_t)row->node.first_child;
	node = NULL;
	table = NULL;
	body = NULL;
	row = NULL;
	answer = VM_VALUE_UNDEFINED;

	/* Actual root-element tracing must preserve every cache with all C stacks excluded. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	for (index = 0; index < 7U; index++) {
		found = vm_heap_find_cell(heap, addresses[index]);
		collection_check(found != NULL, "table-only root retains native table cache/member graph");
	}

	/* The native cache field identities must remain stable after a real collection. */
	found = vm_heap_find_cell(heap, addresses[0]);
	if (found == NULL) {
		error = EIO;
		goto cleanup;
	}

	/* A missing table has already failed the checked observation above. */
	table = (struct dom_element *)found;
	collection_check((uintptr_t)table->bodies_collection == addresses[1], "table bodies cache SameObject after GC");
	collection_check((uintptr_t)table->rows_collection == addresses[2], "table rows cache SameObject after GC");
	vm_heap_set_stack_base(heap, construction_stack);

	/* A sole row.cells collection retains its actual root, Document prototypes and remaining cache cycles. */
	error = collection_script(realm, "var saved=table.rows[0].cells;table=null;saved", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* No native temporary participates in this sole-collection reachability observation. */
	table = NULL;
	answer = VM_VALUE_UNDEFINED;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	for (index = 0; index < 7U; index++) {
		found = vm_heap_find_cell(heap, addresses[index]);
		collection_check(found != NULL, "sole cells collection retains native owner/cache/member graph");
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
	collection_check(same, "sole saved cells native APIs and XML prototypes remain usable");
	error = collection_script(realm, "saved[0].remove();saved.length===0&&saved.held===undefined", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Mutation changes the saved native collection rather than an obsolete snapshot. */
	same = vm_to_boolean(answer);
	collection_check(same, "saved cells remains live after native member removal");
	error = collection_script(realm, "saved=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* No root may keep the table, its member or any of the four native cache cycles alive. */
	answer = VM_VALUE_UNDEFINED;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	for (index = 0; index < 7U; index++) {
		found = vm_heap_find_cell(heap, addresses[index]);
		collection_check(found == NULL, "last table collection root release reclaims cache/member graph");
	}

	/* Ordinary native construction resumes with the original stack boundary. */
	vm_heap_set_stack_base(heap, construction_stack);

	/* All native ownership observations completed before shared teardown. */
	error = 0;

cleanup:
	/* Restore the construction contract even after a missing collector observation. */
	vm_heap_set_stack_base(heap, construction_stack);
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (error != 0)
		return error;

	/* Succeeded: all native caches and sole collection ownership were observed. */
	return 0;
}
