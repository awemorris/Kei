/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies genuine child-list record construction through allocation-triggered GC without caller frames. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Ordinary inert allocation supplies collector pressure without any participant trace edge. */
static const struct vm_cell_type pressure_type = { "mutation-record-pressure", NULL, NULL };
/* Each independent graph, interval and lifetime observation contributes to the outcome. */
static unsigned checks;
/* Failed ownership observations survive until the final process exit. */
static unsigned failures;

static void mutation_check(int condition, const char *name);
static int mutation_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int mutation_case(struct vm_realm *realm, struct bind_window *window);

/*
 * Runs a genuine registered child-list notification without outer receiver or conservative stack roots.
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

	/* Ordinary fixture construction uses the production collector's real C stack convention. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Production builtins and a primary Window provide real native factory and prototype snapshots. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The primary Document supplies genuine binding installation ownership. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* No host mutation callback supplies an alternate record or rooting path. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The native case temporarily disables stack scanning only around the actual operation. */
	status = mutation_case(realm, window);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Print all observations after the production embedding has released its participants. */
	printed = printf("native mutation record GC: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any incomplete graph, changed interval or retained released root rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: actual mid-record collection and complete record lifetime are verified. */
	return 0;
}

/* Records one graph or lifetime observation while preserving later independent checks. */
static void
mutation_check(
	int condition,
	const char *name)
{
	int printed;

	/* A failed observation remains part of the final process-wide failure count. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Constructs genuine native factory outputs through the ordinary production interpreter. */
static int
mutation_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* The fixture source uses the same character conversion and parser as ordinary script execution. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* No alternate constructor or test-only brand supplies the XML Document. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the ordinary native factory completion is available to the embedding. */
	return 0;
}

/* Verifies three finite record shapes across actual allocation-triggered collection. */
static int
mutation_case(
	struct vm_realm *realm,
	struct bind_window *window)
{
	struct dom_node *parent;
	struct dom_node *added;
	struct dom_node *removed;
	struct dom_node *reported;
	struct vm_cell *roots[2];
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value answer;
	uintptr_t removed_address;
	unsigned mode;
	unsigned index;
	unsigned registered;
	size_t margin;
	int truth;
	int status;

	/* Explicit construction roots end before each direct callee invocation. */
	memset(roots, 0, sizeof(roots));
	registered = 0;
	status = 0;
	for (index = 0; index < 2U; index++) {
		status = vm_heap_add_root(realm->heap, &roots[index]);
		if (status != 0)
			break;
		registered++;
	}

	/* Each case uses a distinct near-threshold allocation and native record shape. */
	if (status == 0) {
		for (mode = 0; mode < 3U; mode++) {
			/* Genuine observers use their ordinary native target registrations. */
			vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
			status = mutation_script(realm,
			    "var parent=document.createElement('div');var child=document.createElement('b');"
			    "var removed=document.createComment('removed');parent.appendChild(removed);"
			    "var one=new MutationObserver(function(){}),two=new MutationObserver(function(){});"
			    "one.observe(parent,{childList:true});two.observe(parent,{childList:true,subtree:true});parent", &answer);
			if (status != 0)
				break;
			parent = bind_node_of(answer);

			/* Capture exact native input identities while script globals still protect them. */
			status = mutation_script(realm, "child", &answer);
			if (status != 0)
				break;
			added = bind_node_of(answer);
			roots[0] = &added->cell;

			/* Detached removal input has no parent link when its record is constructed. */
			status = mutation_script(realm, "removed", &answer);
			if (status != 0)
				break;
			removed = bind_node_of(answer);
			roots[1] = &removed->cell;
			removed_address = (uintptr_t)removed;
			dom_remove(removed);

			/* Native source links match the supplied added-only or combined record shape. */
			if (mode != 2U) {
				dom_append_child(parent, added);
			}

			/* The fixture gives no script global or conservative stack edge to the detached node. */
			status = mutation_script(realm, "child=null;removed=null", &answer);
			if (status != 0)
				break;
			vm_heap_set_stack_base(realm->heap, NULL);
			vm_heap_collect(realm->heap);

			/* Ordinary inert allocation approaches the unchanged production threshold. */
			margin = 128U + mode * 256U;
			pressure = vm_heap_alloc(realm->heap, &pressure_type, 8U * 1024U * 1024U - margin);
			if (pressure == NULL) {
				status = ENOMEM;
				break;
			}

			/* Only the native callee owns the supplied participants during its allocations. */
			roots[0] = NULL;
			roots[1] = NULL;
			vm_heap_stats(realm->heap, &before);
			if (mode == 0U) {
				status = bind_environment_child_mutation(window, parent, added, removed);
			} else if (mode == 1U) {
				status = bind_environment_child_mutation(window, parent, added, NULL);
			} else {
				status = bind_environment_child_mutation(window, parent, NULL, removed);
			}

			/* Actual collector statistics prove collection occurred inside record construction. */
			vm_heap_stats(realm->heap, &after);
			if (status != 0)
				break;
			mutation_check(after.collections > before.collections, "direct child-list callee triggered actual GC without caller stack roots");

			/* Both observers publish independent complete records through their existing takeRecords API. */
			vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
			status = mutation_script(realm,
			    "var first=one.takeRecords(),second=two.takeRecords();"
			    "first.length===1&&second.length===1&&first!==second&&first[0]!==second[0]&&"
			    "first[0].type==='childList'&&first[0].target===parent&&second[0].target===parent&&"
			    "first[0].addedNodes.constructor.name==='NodeList'&&first[0].removedNodes.constructor.name==='NodeList'&&"
			    "first[0].addedNodes!==second[0].addedNodes&&first[0].removedNodes!==second[0].removedNodes&&"
			    "first[0].previousSibling===null&&first[0].nextSibling===null&&first[0].attributeName===null&&"
			    "first[0].attributeNamespace===null&&first[0].oldValue===null", &answer);
			if (status != 0)
				break;
			truth = vm_to_boolean(answer);
			mutation_check(truth, "complete independent record fields and NodeList constructors survive mid-construction GC");

			/* Added-node identity is the actual native child, including an empty removed sequence. */
			if (mode != 2U) {
				status = mutation_script(realm, "first[0].addedNodes[0]", &answer);
				if (status != 0)
					break;
				reported = bind_node_of(answer);
				mutation_check(reported == added, "added record preserves exact native node identity");
			}

			/* Detached removed-node identity remains owned by the published record. */
			if (mode != 1U) {
				status = mutation_script(realm, "first[0].removedNodes[0]", &answer);
				if (status != 0)
					break;
				reported = bind_node_of(answer);
				mutation_check(reported == removed && reported->parent == NULL, "removed record preserves exact detached native node identity");
			}

			/* The three finite shapes preserve empty and single-entry collection lengths. */
			if (mode == 0U) {
				status = mutation_script(realm, "first[0].addedNodes.length===1&&first[0].removedNodes.length===1", &answer);
			} else if (mode == 1U) {
				status = mutation_script(realm, "first[0].addedNodes.length===1&&first[0].removedNodes.length===0", &answer);
			} else {
				status = mutation_script(realm, "first[0].addedNodes.length===0&&first[0].removedNodes.length===1", &answer);
			}

			/* No list decoration or record construction changes collection cardinality. */
			if (status != 0)
				break;
			truth = vm_to_boolean(answer);
			mutation_check(truth, "zero and one entry NodeList-shaped sequences retain exact lengths");

			/* Disconnect and release both returned arrays before inspecting native collection. */
			status = mutation_script(realm, "one.disconnect();two.disconnect();first=null;second=null;parent=null;one=null;two=null", &answer);
			if (status != 0)
				break;
			vm_heap_set_stack_base(realm->heap, NULL);
			vm_heap_collect(realm->heap);
			found = vm_heap_find_cell(realm->heap, removed_address);
			mutation_check(found == NULL, "released detached node is collectible after callee roots and record arrays disappear");
		}
	}

	/* Restore fixture stack scanning and remove only successfully registered construction slots. */
	vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(realm->heap, &roots[registered]);
	}

	/* A parser, native mutation or allocation failure rejects the bounded fixture. */
	if (status != 0)
		return status;

	/* Succeeded: all three actual allocation-GC record shapes were inspected. */
	return 0;
}
