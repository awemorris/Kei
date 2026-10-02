/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks retained activation and event-path cells during actual listener-triggered GC. */

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
static int click_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

/*
 * Verifies collection caches and collectible DOM cycles through ordinary bindings.
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
	printed = printf("click lifetime checks: %u/%u passed\n", checks - failures, checks);
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

/* Removes the pending target and prior selection, then collects with all C stacks excluded. */
static int
click_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	struct dom_node *target;
	struct dom_node *previous;
	struct dom_node *parent;
	struct vm_cell *found;
	uintptr_t target_address;
	uintptr_t previous_address;
	uintptr_t parent_address;
	uintptr_t event_address;
	int same;

	UNUSED_PARAMETER(receiver);

	/* Actual Event.target supplies the input; there are no external registered fixture roots. */
	event = bind_event_of(js_argument(args, count, 0));
	target = bind_node_of(event->target);
	parent = target->parent;
	previous = parent->first_child;
	target_address = (uintptr_t)target;
	previous_address = (uintptr_t)previous;
	parent_address = (uintptr_t)parent;
	event_address = (uintptr_t)event;

	/* Removing the parent and prior radio breaks the DOM's only edge to the saved selection. */
	dom_remove(parent);
	dom_remove(previous);
	dom_remove(target);
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, previous_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "previous removed radio survives only pending activation roots");
	found = vm_heap_find_cell(realm->heap, target_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "target survives listener GC after tree removal");
	found = vm_heap_find_cell(realm->heap, parent_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "original event path retains removed ancestor");
	found = vm_heap_find_cell(realm->heap, event_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "current native event survives callback collection");
	vm_heap_set_stack_base(realm->heap, construction_stack);

	/* Cancellation runs after this callback, when the old radio is no longer a current peer. */
	event->canceled = 1;
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Exercises native click without a script call frame retaining the target or old radio. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct dom_node *form;
	struct dom_element *target;
	struct vm_cell *found;
	uintptr_t previous_address;
	uintptr_t parent_address;
	uintptr_t target_address;
	vm_value answer;
	int checked;
	int status;

	/* Construct the ordinary production binding and register a synchronous native listener. */
	status = vm_realm_create(heap, &realm);
	if (status != 0)
		return status;
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* A manual primary owns the fixture; no host task or synthetic engine switch participates. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Only the primary owns this fixture; the listener has no external event root. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* This native callback forces collection inside the actual production event dispatcher. */
	status = js_builtin_method(realm, realm->global, "collectClick", 1, click_collect);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return status;
	}

	/* The IIFE leaves no global script object holding the form or either radio. */
	status = collection_script(realm,
		"(function(){var f=document.createElement('form');"
		"f.innerHTML='<input type=radio name=pair checked><input type=radio name=pair>';"
		"f.elements[1].onclick=collectClick;document.appendChild(f);return f;})()", &answer);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return status;
	}

	/* Invoke click directly so the saved previous radio exists only in native activation state. */
	form = bind_node_of(answer);
	target = (struct dom_element *)form->last_child;
	previous_address = (uintptr_t)form->first_child;
	parent_address = (uintptr_t)form;
	target_address = (uintptr_t)target;
	answer = VM_VALUE_UNDEFINED;
	status = bind_click(window, target);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return status;
	}

	/* A removed prior selection is not restored; cancellation leaves the target unchecked. */
	checked = dom_control_checked(target);
	collection_check(!checked, "current-group cancellation after GC unchecks target");
	collection_check(!target->click_in_progress, "click guard released after callback collection");
	collection_check(target->control->checked_dirty, "script click establishes dirty target checkedness");

	/* All activation and path roots must disappear when the operation returns. */
	target = NULL;
	form = NULL;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, previous_address);
	collection_check(found == NULL, "previous radio reclaimed after pending activation ends");
	found = vm_heap_find_cell(heap, parent_address);
	collection_check(found == NULL, "removed event ancestor reclaimed after dispatch ends");
	found = vm_heap_find_cell(heap, target_address);
	collection_check(found == NULL, "target reclaimed after last activation root released");
	vm_heap_set_stack_base(heap, construction_stack);

	/* The primary binding then tears down without a retained root slot into this stack. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	return 0;
}
