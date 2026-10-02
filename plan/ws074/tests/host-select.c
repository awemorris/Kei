/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks native select addition and conversion graphs during actual collector pressure. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One synchronous host invocation observes integer addresses without retaining DOM cells. */
struct select_observer {
	struct vm_heap *heap;
	uintptr_t owner;
	uintptr_t old;
	uintptr_t target;
	uintptr_t argument;
	unsigned conversions;
	uintptr_t anchor;
	uintptr_t created;
	unsigned calls;
	int failure;
};

/* Assertions accumulate across callback-driven and embedding-driven collection. */
static unsigned checks;
/* Failed behavioral checks determine the final exit status. */
static unsigned failures;
/* The embedding restores this construction stack boundary after explicit collection. */
static const void *construction_stack;

/* The synchronous callback sees integer-only observer data until its native invocation returns. */
static struct select_observer *active_observer;

static void collection_check(int condition, const char *name);
static int collection_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int collection_case(struct vm_heap *heap);
static int table_collect(void *context, struct dom_node *node);
static int index_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

/*
 * Verifies rooted select mutations and collectible DOM cycles through real host callbacks.
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
	vm_heap_destroy(heap);
	if (error != 0)
		return 2;

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("select addition lifetime checks: %u/%u passed\n", checks - failures, checks);
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
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Collects after severing the links which otherwise hide missing native operation roots. */
static int
table_collect(
	void *context,
	struct dom_node *node)
{
	struct select_observer *observer;
	struct dom_node *owner;
	struct dom_node *anchor;
	struct vm_cell *found;
	uintptr_t created;
	int matches;

	UNUSED_PARAMETER(context);

	/* The actual host context stays intact for unrelated environment collaborators. */
	observer = active_observer;
	observer->calls++;
	owner = node->parent;
	anchor = NULL;
	if (observer->anchor != 0)
		anchor = (struct dom_node *)observer->anchor;
	created = (uintptr_t)node;
	observer->created = created;
	matches = 0;
	if ((uintptr_t)owner == observer->owner)
		matches = 1;
	collection_check(matches, "host sees actual select insertion owner");

	/* The pending operation must retain both new and old nodes after all tree edges are severed. */
	dom_remove(node);
	dom_remove(owner);
	if (anchor != NULL)
		dom_remove(anchor);
	vm_heap_set_stack_base(observer->heap, NULL);
	vm_heap_collect(observer->heap);
	found = vm_heap_find_cell(observer->heap, observer->owner);
	collection_check(found != NULL, "select retained through callback GC");
	found = vm_heap_find_cell(observer->heap, created);
	collection_check(found != NULL, "created or incoming child retained through callback GC");

	/* The operation target survives even when the current insertion parent is a removed header. */
	found = vm_heap_find_cell(observer->heap, observer->target);
	collection_check(found != NULL, "native operation target retained through callback GC");

	/* The incoming native option remains held after all of its original tree edges are removed. */
	if (observer->old != 0) {
		found = vm_heap_find_cell(observer->heap, observer->old);
		collection_check(found != NULL, "incoming native child retained through callback GC");
	}

	/* The insertion anchor remains independently retained if the host removes it too. */
	if (observer->anchor != 0) {
		found = vm_heap_find_cell(observer->heap, observer->anchor);
		collection_check(found != NULL, "removed insertion anchor retained through callback GC");
	}

	/* Converted additions still retain the incoming option before choosing membership. */
	found = vm_heap_find_cell(observer->heap, observer->old);
	collection_check(found != NULL, "incoming native option retained through callback GC");

	/* Restores the embedding's ordinary stack contract before returning its configured host outcome. */
	vm_heap_set_stack_base(observer->heap, construction_stack);
	if (observer->failure)
		return EIO;

	/* Succeeded: collection pressure exercised the complete default mutation path. */
	return 0;
}

/* Collects inside numeric conversion before current option membership or native membership observation. */
static int
index_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_cell *found;
	struct dom_node *target;

	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The direct native invocation has no script argument frame independently retaining the receiver. */
	active_observer->conversions++;
	target = (struct dom_node *)active_observer->target;
	dom_remove(target);
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, active_observer->target);
	collection_check(found != NULL, "native target survives numeric conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->argument);
	collection_check(found != NULL, "numeric argument survives conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->old);
	collection_check(found != NULL, "incoming option survives numeric union conversion GC");
	vm_heap_set_stack_base(realm->heap, construction_stack);
	*result = vm_value_int32(0);

	/* Succeeded: membership observation can now begin using the converted zero index. */
	return 0;
}

/* Exercises pending select/option/parent/reference graphs, conversion and host-error cleanup. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_window *actual;
	struct bind_host host;
	struct select_observer observer;
	struct dom_node *table;
	struct vm_cell *found;
	vm_value receiver;
	vm_value argument;
	vm_value incoming;
	vm_value arguments[2];
	vm_value answer;
	unsigned kind;
	int status;
	int expected;

	/* Installs the real primary binding without a synthetic engine behavior switch. */
	status = vm_realm_create(heap, &realm);
	if (status != 0)
		return status;
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* Only actual managed child ownership supplies the borrowed mutation case. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* The ordinary host has no callbacks until each finite native invocation starts. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* A real native valueOf callback exercises collection during signed index conversion. */
	status = js_builtin_method(realm, realm->global, "collectIndex", 0, index_collect);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return status;
	}

	/* Each finite case calls the actual registered native method without a retaining script frame. */
	for (kind = 0; kind < 6U; kind++) {
		/* The ordinary target and incoming option start with no independent script roots. */
		window->host.node_inserted = NULL;
		status = collection_script(realm, "document.createElement('select')", &receiver);
		if (status != 0)
			break;
		table = bind_node_of(receiver);
		actual = window;
		memset(&observer, 0, sizeof(observer));
		observer.heap = heap;
		argument = vm_value_int32(0);

		/* A grouped reference supplies an independent parent and insertion anchor. */
		if (kind == 1) {
			status = collection_script(realm,
				"(function(){var s=document.createElement('select'),g=document.createElement('optgroup');"
				"s.appendChild(g);g.appendChild(document.createElement('option'));return s;})()", &receiver);
			if (status != 0)
				break;
			table = bind_node_of(receiver);
			observer.anchor = (uintptr_t)table->first_child->first_child;
		}

		/* A borrowed primary method must notify the actual retained child's host. */
		if (kind == 2) {
			status = collection_script(realm,
				"document.appendChild(document.createElement('html'));"
				"document.documentElement.appendChild(document.createElement('body'));"
				"(function(){var f=document.createElement('iframe');document.body.appendChild(f);"
				"var s=f.contentDocument.createElement('select');f.remove();return s;})()", &receiver);
			if (status != 0)
				break;
			table = bind_node_of(receiver);
			actual = table->document->view;
		}

		/* An incoming group retains its complete option subtree through callback collection. */
		if (kind == 5) {
			status = collection_script(realm,
				"(function(){var g=document.createElement('optgroup');"
				"g.appendChild(document.createElement('option'));return g;})()", &incoming);
		} else {
			status = collection_script(realm, "document.createElement('option')", &incoming);
		}

		/* A failed fixture allocation cannot publish a partially initialized invocation. */
		if (status != 0)
			break;
		observer.old = (uintptr_t)bind_node_of(incoming);

		/* The numeric conversion argument is otherwise reachable only from the native root slot. */
		if (kind == 4) {
			status = collection_script(realm, "({valueOf:collectIndex})", &argument);
			if (status != 0)
				break;
			observer.argument = (uintptr_t)vm_value_as_cell(argument);
		}

		/* Both target and actual insertion parent remain needed during host removal and GC. */
		observer.target = (uintptr_t)table;
		observer.owner = observer.target;
		if (kind == 1)
			observer.owner = (uintptr_t)table->first_child;
		expected = 0;
		if (kind == 3) {
			observer.failure = 1;
			expected = EIO;
		}

		/* Invoke precisely the production registry callback, leaving C stacks out of collection. */
		active_observer = &observer;
		actual->host.node_inserted = table_collect;
		arguments[0] = incoming;
		arguments[1] = argument;
		status = bind_html_select_element_interface.operations[0].method(realm, receiver, arguments, 2, &answer);
		collection_check(status == expected, "exact select host outcome propagated");
		collection_check(observer.calls == 1U, "one actual owner insertion notification");
		actual->host.node_inserted = NULL;
		active_observer = NULL;

		/* Successful addition returns undefined even after callback removal of the incoming node. */
		if (status == 0)
			collection_check(answer == VM_VALUE_UNDEFINED, "native add returns undefined after callback removal");
		if (kind == 4)
			collection_check(observer.conversions == 1U, "native union converts exactly once under actual GC");

		/* No result or script frame retains any detached graph after the native invocation ends. */
		receiver = VM_VALUE_UNDEFINED;
		argument = VM_VALUE_UNDEFINED;
		incoming = VM_VALUE_UNDEFINED;
		answer = VM_VALUE_UNDEFINED;
		arguments[0] = VM_VALUE_UNDEFINED;
		arguments[1] = VM_VALUE_UNDEFINED;
		table = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.target);
		collection_check(found == NULL, "select target reclaimed after invocation roots release");
		found = vm_heap_find_cell(heap, observer.owner);
		collection_check(found == NULL, "actual insertion parent reclaimed after invocation roots release");
		found = vm_heap_find_cell(heap, observer.old);
		collection_check(found == NULL, "incoming option or group reclaimed after invocation roots release");

		/* Detached reference and converted argument cannot leave an expired root slot behind. */
		if (observer.anchor != 0) {
			found = vm_heap_find_cell(heap, observer.anchor);
			collection_check(found == NULL, "removed option anchor reclaimed after native return");
		}

		/* Argument ownership ends together with the invocation, including host errors. */
		if (observer.argument != 0) {
			found = vm_heap_find_cell(heap, observer.argument);
			collection_check(found == NULL, "converted union argument reclaimed after native return");
		}

		/* Resume the conservative construction contract before the next independent sample. */
		vm_heap_set_stack_base(heap, construction_stack);
		status = 0;
	}

	/* Primary teardown observes no root slot pointing into an expired native invocation. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (status != 0)
		return status;

	/* Succeeded: every finite select conversion/insertion/host-error lifetime contract was exercised. */
	return 0;
}
