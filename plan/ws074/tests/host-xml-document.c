/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercises XML creation graph retention after the explicit primary host dies. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Counts independently named ownership checks across the bounded three graph cases. */
static unsigned checks;
/* Failed graph observations accumulate until the final exit status is reported. */
static unsigned failures;

static void xml_check(int condition, const char *name);
static int xml_sample(struct vm_heap *heap, unsigned kind, struct vm_cell **root, struct dom_document **expected);
static int xml_case(unsigned kind);

/*
 * Verifies connected, detached and implementation-only XML graphs under actual GC.
 */
int
main(
	void)
{
	unsigned kind;
	int error;
	int printed;

	/* Each case leaves exactly one explicitly registered graph root after teardown. */
	for (kind = 0; kind < 3; kind++) {
		error = xml_case(kind);
		if (error != 0)
			return 2;
	}

	/* Behavioral failures remain distinguishable from fixture allocation errors. */
	printed = printf("XML document GC checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Every independently observed ownership contract must hold. */
	if (failures != 0)
		return 1;

	/* Succeeded: the bounded XML ownership and reclamation cases passed. */
	return 0;
}

/* Records one graph observation without suppressing later independent checks. */
static void
xml_check(
	int condition,
	const char *name)
{
	int printed;

	/* All checks contribute to the final accounting and failing exit status. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation was retained for the final report. */
	return;
}

/* Constructs an XML graph through production JS and removes all primary C roots. */
static int
xml_sample(
	struct vm_heap *heap,
	unsigned kind,
	struct vm_cell **root,
	struct dom_document **expected)
{
	struct vm_realm *realm;
	struct dom_document *primary;
	struct dom_node *node;
	struct bind_window *window;
	struct bind_host host;
	struct wb_units units;
	struct js_syntax_error syntax;
	const char *source;
	vm_value answer;
	vm_value key;
	vm_value document_value;
	int error;

	/* The primary embedding explicitly owns the realm, as an ordinary browser tab does. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Installs a real binding with no asynchronous external resource owner. */
	primary = dom_document_create(heap);
	if (primary == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* No test-specific engine switch participates in creation or collection. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, primary, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The connected case retains an XML document root solely through its saved descendant. */
	source = "var xml=document.implementation.createDocument('urn:runtime','Root');"
		"var held=xml.createElement('Held');xml.documentElement.appendChild(held);"
		"held.appendChild(xml.createTextNode('survives'));held";
	if (kind == 1) {
		/* A detached parent is likewise retained through the selected child edge. */
		source = "var xml=document.implementation.createDocument(null,null);"
			"var parent=xml.createElement('Detached');var held=xml.createElement('Held');"
			"parent.appendChild(held);held.appendChild(xml.createTextNode('survives'));held";
	} else if (kind == 2) {
		/* The native implementation's associated-document trace is the only remaining edge. */
		source = "var xml=document.implementation.createDocument('urn:runtime','Root');"
			"xml.documentElement.appendChild(xml.createTextNode('survives'));xml.implementation";
	}

	/* Converts the selected fixture before production parsing and execution. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Executes creation, insertion and implementation caching on the default production paths. */
	error = js_run_script(realm, units.data, units.length, 0, &answer, &syntax);
	wb_units_release(&units);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Captures the expected Document as an observation pointer, not a registered root. */
	key = vm_key_from_ascii(heap, "xml");
	if (key == VM_VALUE_EMPTY) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Reads the observation without introducing another registered root. */
	error = vm_get(realm, vm_value_cell(realm->global), key, &document_value);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Selects raw node roots independently of wrappers, or only the implementation wrapper. */
	node = bind_node_of(document_value);
	*expected = node->document;
	*root = vm_value_as_cell(answer);
	if (kind != 2) {
		node = bind_node_of(answer);
		*root = &node->cell;
	}

	/* Retired primary native methods are not invoked after their manual realm is destroyed. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: the one registered root now owns the selected graph. */
	return 0;
}

/* Collects without stack scanning and verifies retention, wrapper identity and reclamation. */
static int
xml_case(
	unsigned kind)
{
	struct vm_heap *heap;
	struct vm_cell *root;
	struct dom_document *document;
	struct dom_node *node;
	struct dom_node *text;
	struct vm_realm *caller;
	struct vm_heap_stats retained;
	struct vm_heap_stats dropped;
	struct vm_cell *remaining_document;
	struct vm_cell *remaining_snapshot;
	struct vm_cell *remaining_root;
	uintptr_t document_address;
	uintptr_t snapshot_address;
	uintptr_t root_address;
	vm_value wrapper;
	vm_value implementation;
	vm_value retained_implementation;
	int same;
	int error;

	/* Real-stack scanning protects construction until the explicit graph root is ready. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	root = NULL;
	error = vm_heap_add_root(heap, &root);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Constructs the finite selected graph before eliminating conservative old references. */
	error = xml_sample(heap, kind, &root, &document);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		return error;
	}

	/* Only the explicit root can preserve objects across both real collections. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_collect(heap);
	vm_heap_stats(heap, &retained);
	same = 0;
	if (document->node.type == DOM_DOCUMENT && document->content == DOM_CONTENT_XML)
		same = 1;
	xml_check(same, "saved graph retains its XML owner Document");

	/* Script-created documents retain no C view pointer after primary teardown. */
	same = 0;
	if (document->view == NULL && document->binding_prototypes != NULL)
		same = 1;
	xml_check(same, "XML graph retains prototypes without a raw Window pointer");

	/* A saved raw descendant retains its connected or detached parent independently. */
	if (kind != 2) {
		node = (struct dom_node *)root;
		same = 0;
		if (node->document == document && node->parent != NULL && node->first_child != NULL)
			same = 1;
		xml_check(same, "raw XML child retains parent and text links");
	} else {
		/* The associated-document edge survives even with no retained Node wrapper root. */
		same = 0;
		if (document->implementation == (struct vm_object *)root && document->node.first_child != NULL)
			same = 1;
		xml_check(same, "sole implementation retains its associated XML tree");
	}

	/* Allows fresh allocation using the surviving graph while never calling retired JS natives. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	text = dom_text_create(document, NULL, 0);
	if (text == NULL) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Wrapping relies solely on the surviving private prototype snapshot. */
	error = bind_wrap(NULL, text, &wrapper);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		return error;
	}

	/* An unwrapped XML node obtains its original prototype without any live Window argument. */
	node = bind_node_of(wrapper);
	same = 0;
	if (node == text)
		same = 1;
	xml_check(same, "fresh XML wrapper uses snapshot after primary host teardown");

	/* A direct cached getter uses an unrelated caller without invoking a retired native function. */
	if (kind == 2) {
		error = vm_realm_create(heap, &caller);
		if (error != 0) {
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return error;
		}

		/* Direct native access only observes the already-created SameObject cache. */
		error = bind_document_implementation(caller, vm_value_cell(document->node.wrapper), NULL, 0, &implementation);
		vm_realm_destroy(caller);
		if (error != 0) {
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return error;
		}

		/* The cached implementation remains the same object after its Document-only edge survived GC. */
		retained_implementation = vm_value_cell(root);
		same = 0;
		if (implementation == retained_implementation)
			same = 1;
		xml_check(same, "cached implementation identity survives repeated GC");
	}

	/* Stores observation addresses before collection; none are registered roots. */
	document_address = (uintptr_t)document;
	snapshot_address = (uintptr_t)document->binding_prototypes;
	root_address = (uintptr_t)root;

	/* Dropping the sole graph root permits collection despite obsolete C observation words. */
	root = NULL;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_collect(heap);
	vm_heap_stats(heap, &dropped);
	same = 0;
remaining_document = vm_heap_find_cell(heap, document_address);
	remaining_snapshot = vm_heap_find_cell(heap, snapshot_address);
	remaining_root = vm_heap_find_cell(heap, root_address);
	if (dropped.live_cells < retained.live_cells &&
	    remaining_document == NULL &&
	    remaining_snapshot == NULL &&
	    remaining_root == NULL)
		same = 1;
	xml_check(same, "dropping the XML root reclaims nodes and private prototype graph");
	vm_heap_remove_root(heap, &root);
	vm_heap_destroy(heap);

	/* Succeeded: this saved-graph retention and release case used real production GC. */
	return 0;
}
