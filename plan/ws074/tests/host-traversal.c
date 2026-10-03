/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks TreeWalker candidates and foreign current nodes using actual production GC. */

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

static void traversal_check(int condition, const char *name);
static int traversal_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int traversal_filter(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int traversal_case(struct vm_heap *heap);

/*
 * Verifies callback candidates and foreign current graphs through ordinary bindings.
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
	error = traversal_case(heap);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Releases the embedding only after its complete result is checked. */
	vm_heap_destroy(heap);

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("traversal GC checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Every independently observed GC contract must hold. */
	if (failures != 0)
		return 1;

	/* Succeeded: selected candidate and current-node graphs survived actual GC. */
	return 0;
}

/* Records one named ownership contract without hiding later independent observations. */
static void
traversal_check(
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
traversal_script(
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

	/* The actual script engine creates every node, walker and callback object. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Source conversion storage is no longer borrowed after script execution. */
	wb_units_release(&units);

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Removes the sole candidate and collects while only the walker pending edge retains it. */
static int
traversal_filter(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct vm_cell *found;
	vm_value argument;
	uintptr_t address;
	int same;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The callback's argument is a genuine production Node wrapper. */
	argument = js_argument(args, count, 0);
	status = bind_argument_node(realm, argument, &node);
	if (status != 0)
		return status;

	/* Unlinking removes the root tree's reference before stack scanning is disabled. */
	address = (uintptr_t)node;
	dom_remove(node);
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, address);
	same = 0;
	if (found != NULL)
		same = 1;
	traversal_check(same, "pending candidate survives unlink and GC without stack scanning");
	vm_heap_set_stack_base(realm->heap, construction_stack);

	/* Returning ACCEPT exercises publication of that detached candidate. */
	*result = vm_value_int32(1);

	/* Succeeded: the callback completed after actual candidate collection pressure. */
	return 0;
}

/* Builds bounded real graphs and checks collection while their primary host remains usable. */
static int
traversal_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct dom_node *node;
	struct vm_cell *found;
	struct vm_object *weak_wrapper;
	struct bind_window *window;
	struct bind_host host;
	vm_value answer;
	uintptr_t child_address;
	uintptr_t iterator_address;
	int same;
	int valid;
	int error;

	/* The primary embedding uses ordinary manual ownership and realm tracing. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The primary Document exists before its binding is installed. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* No async host resource or test-only engine mode participates in this fixture. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* A synchronous native callback explicitly collects while the production walker is active. */
	error = js_builtin_method(realm, realm->global, "collectCandidate", 1, traversal_filter);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The IIFE leaves no global candidate variable or external registered Node root. */
	error = traversal_script(realm,
				 "var walker=(function(){var d=document.implementation.createDocument(null,'Root');"
				 "d.documentElement.appendChild(d.createElement('Candidate'));"
				 "return d.createTreeWalker(d.documentElement,1,collectCandidate);})();"
				 "var selected=walker.nextNode();"
				 "selected.tagName==='Candidate' && selected.parentNode===null && walker.currentNode===selected",
				 &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Accepted candidates remain usable after their only pending edge was released. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	traversal_check(same, "removed accepted candidate publishes intact wrapper and current state");

	/* Iterator repair changes its candidate position while the original filter argument stays alive. */
	error = traversal_script(realm,
				 "var iterator=(function(){var d=document.implementation.createDocument(null,'Root');"
				 "d.documentElement.appendChild(d.createElement('Candidate'));"
				 "return d.createNodeIterator(d.documentElement,1,function(n){"
				 "if(n.tagName==='Root')return 3;return collectCandidate(n);});})();"
				 "var iterated=iterator.nextNode();"
				 "iterated.tagName==='Candidate' && iterated.parentNode===null && "
				 "iterator.referenceNode.tagName==='Root' && !iterator.pointerBeforeReferenceNode",
				 &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The published Node remains distinct from the removal-adjusted cursor reference. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	traversal_check(same, "iterator original argument survives candidate repair and actual GC");

	/* A live Document must not retain a discarded actual iterator through its weak token. */
	error = traversal_script(realm,
				 "var liveDocument=document.implementation.createDocument(null,'Live');"
				 "var discarded=liveDocument.createNodeIterator(liveDocument.documentElement,1,null);discarded",
				 &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The native iterator cell address is observed without registering it as a root. */
	valid = vm_value_is_object(answer);
	if (!valid) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return EIO;
	}

	/* Only an actual native iterator wrapper exposes the observed weak state. */
	weak_wrapper = (struct vm_object *)vm_value_as_cell(answer);
	found = vm_value_as_cell(weak_wrapper->internal);
	if (weak_wrapper->kind != VM_KIND_PLATFORM || found == NULL) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return EIO;
	}

	/* Observes state identity without adding a strong collector edge. */
	iterator_address = (uintptr_t)found;
	error = traversal_script(realm, "discarded=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Its Document stays globally reachable while explicit GC excludes every C local. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, iterator_address);
	same = 0;
	if (found == NULL)
		same = 1;
	traversal_check(same, "live actual Document does not retain discarded NodeIterator");
	vm_heap_set_stack_base(heap, construction_stack);

	/* Later mutation cannot notify the reclaimed iterator through a stale weak token. */
	error = traversal_script(realm,
				 "liveDocument.documentElement.appendChild(liveDocument.createElement('After'));"
				 "liveDocument.documentElement.removeChild(liveDocument.documentElement.firstChild);"
				 "liveDocument.documentElement.firstChild===null",
				 &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Production mutation remains usable after subscriber reclamation. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	traversal_check(same, "Document mutation remains safe after iterator collection");

	/* A primary-created walker retains a different child solely through currentNode. */
	error = traversal_script(realm,
				 "document.appendChild(document.createElement('html'));"
				 "document.documentElement.appendChild(document.createElement('body'));"
				 "var frame=document.createElement('iframe');document.body.appendChild(frame);"
				 "var child=frame.contentDocument;var held=document.createTreeWalker(document.body,1,null);"
				 "held.currentNode=child.body;document.body.removeChild(frame);frame=null;child=null;"
				 "walker=null;selected=null;iterator=null;iterated=null;held.currentNode",
				 &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Captures the foreign owner address without registering it as an embedding root. */
	node = bind_node_of(answer);
	if (node == NULL ||
	    node->document == NULL ||
	    node->document->context == NULL) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return EIO;
	}

	/* Saves the managed owner identity before excluding conservative roots. */
	child_address = (uintptr_t)node->document->context;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, child_address);
	same = 0;
	if (found != NULL)
		same = 1;
	traversal_check(same, "foreign currentNode alone retains removed managed child");
	vm_heap_set_stack_base(heap, construction_stack);

	/* The original primary walker getter still returns the intact child-owned Node. */
	error = traversal_script(realm,
				 "held.currentNode.nodeName==='BODY' && held.currentNode.ownerDocument.defaultView===null", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The getter completion is inspected before the next collection point. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	traversal_check(same, "foreign currentNode remains usable through primary walker");

	/* Moving current back to the primary root drops its only child owner edge. */
	error = traversal_script(realm, "held.currentNode=held.root;held=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* With stack roots excluded, only traced live graphs may retain the owner. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, child_address);
	same = 0;
	if (found == NULL)
		same = 1;
	traversal_check(same, "releasing foreign currentNode permits managed owner reclamation");
	vm_heap_set_stack_base(heap, construction_stack);

	/* The host no longer roots any candidate or walker after complete manual teardown. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: these graph cases used default production traversal and collection. */
	return 0;
}
