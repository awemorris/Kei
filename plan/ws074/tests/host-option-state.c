/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks selected index conversion ownership during actual collector pressure. */

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
static int index_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

/*
 * Verifies rooted selection conversion and collectible DOM cycles through real host callbacks.
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
	printed = printf("option selectedness lifetime checks: %u/%u passed\n", checks - failures, checks);
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

	/* Reject an invocation outside the finite observer lifetime. */
	if (active_observer == NULL)
		return EIO;

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
	*result = vm_value_int32(1);

	/* A real conversion error must unwind both temporary native roots. */
	if (active_observer->failure)
		return EIO;

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
	struct bind_host host;
	struct select_observer observer;
	struct dom_node *table;
	struct dom_element *chosen;
	struct vm_cell *found;
	vm_value receiver;
	vm_value argument;
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

	/* Successful and failed numeric conversion use the exact registered native setter. */
	for (kind = 0; kind < 2U; kind++) {
		/* The target has native option state but no independent script variable or external root. */
		status = collection_script(realm,
					   "(function(){var s=document.createElement('select');"
					   "s.add(document.createElement('option'));s.add(document.createElement('option'));return s;})()",
					   &receiver);
		if (status != 0)
			goto cleanup;
		table = bind_node_of(receiver);
		if (table == NULL ||
		    table->type != DOM_ELEMENT ||
		    table->first_child == NULL ||
		    table->last_child == NULL) {
			status = EIO;
			goto cleanup;
		}

		/* Publish addresses only after checking the generated option graph. */
		memset(&observer, 0, sizeof(observer));
		observer.heap = heap;
		observer.target = (uintptr_t)table;
		observer.old = (uintptr_t)table->last_child;
		status = collection_script(realm, "({valueOf:collectIndex})", &argument);
		if (status != 0)
			goto cleanup;
		observer.argument = (uintptr_t)vm_value_as_cell(argument);

		/* Conversion itself detaches and collects the target and argument graph. */
		expected = 0;
		if (kind == 1) {
			observer.failure = 1;
			expected = EIO;
		}

		/* Call the production registry setter without an outer VM receiver frame. */
		active_observer = &observer;
		status = bind_html_select_element_interface.attributes[1].setter(realm, receiver, &argument, 1, &answer);
		if (status != expected) {
			active_observer = NULL;
			status = EIO;
			goto cleanup;
		}

		/* The observer ends before inspecting current owned option state. */
		active_observer = NULL;
		found = vm_heap_find_cell(heap, observer.target);
		if (found == NULL) {
			status = EIO;
			goto cleanup;
		}

		/* Inspect only the target located in the actual heap after collection. */
		table = (struct dom_node *)found;
		collection_check(status == expected, "exact selected index conversion outcome propagated");
		collection_check(observer.conversions == 1U, "index conversion runs once during collector pressure");

		/* Owned state changes only after successful conversion, with no attribute-only consumer fallback. */
		if (status == 0) {
			chosen = dom_select_chosen((struct dom_element *)table);
			collection_check((uintptr_t)chosen == observer.old, "chosen consumer sees post-conversion owned selection");
			collection_check(((struct dom_element *)table->last_child)->option_dirty, "matching indexed member becomes dirty");
			collection_check(!((struct dom_element *)table->first_child)->option_selected, "other current member deselected after conversion");
		} else {
			collection_check(((struct dom_element *)table->first_child)->option_selected, "failed conversion preserves previous owned selection");
		}

		/* No receiver, argument or C construction stack survives the native invocation. */
		receiver = VM_VALUE_UNDEFINED;
		argument = VM_VALUE_UNDEFINED;
		answer = VM_VALUE_UNDEFINED;
		table = NULL;
		chosen = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.target);
		collection_check(found == NULL, "index target reclaimed after native roots unwind");
		found = vm_heap_find_cell(heap, observer.old);
		collection_check(found == NULL, "owned member graph reclaimed after native roots unwind");
		found = vm_heap_find_cell(heap, observer.argument);
		collection_check(found == NULL, "conversion argument reclaimed after native roots unwind");
		vm_heap_set_stack_base(heap, construction_stack);
		status = 0;
	}

	/* Succeeded: every native sample completed before shared teardown. */
	status = 0;

cleanup:
	/* Primary teardown observes no expired callback or construction stack contract. */
	active_observer = NULL;
	vm_heap_set_stack_base(heap, construction_stack);
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (status != 0)
		return status;

	/* Succeeded: every finite select conversion/insertion/host-error lifetime contract was exercised. */
	return 0;
}
