/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks native dirty checkedness independently of observable checked attributes.
 */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Counts each native current-state, dirty-flag and clone observation until main reports. */
static unsigned checks;

/* Failed behavioral checks determine the final exit status. */
static unsigned failures;

/* Borrows main's live stack boundary to protect fixture construction during normal GC. */
static const void *construction_stack;

static void collection_check(int condition, const char *name);
static int collection_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int collection_case(struct vm_heap *heap);

/*
 * Verifies native radio checkedness and dirty-state independence through ordinary bindings.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	int error;
	int printed;

	/* The collector uses the real live stack throughout ordinary fixture construction. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;
	construction_stack = __builtin_frame_address(0);
	vm_heap_set_stack_base(heap, construction_stack);

	/* Exercises native setters and current group ownership on the ordinary live heap. */
	error = collection_case(heap);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Releases all test-owned cells after the primary realm has been destroyed. */
	vm_heap_destroy(heap);

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("radio state checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Every independently observed radio-state contract must hold. */
	if (failures != 0)
		return 1;

	/* Succeeded: native current state, dirty flags and clone storage obeyed their contracts. */
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

/* Verifies dirty flags through actual public setters rather than script-visible substitutes. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct dom_node *node;
	struct dom_element *a;
	struct dom_element *b;
	struct dom_element *clone;
	vm_value answer;
	int same;
	int error;
	int observed;

	/* The fixture invokes ordinary script over the actual production DOM and bindings. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;

	/* The fixture uses ordinary intrinsics before installing Document bindings. */
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* A manual primary owns the fixture; no asynchronous host effects participate. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* No host task or hidden test-only engine switch affects the fixture. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* List value selection changes clean current state without setting either dirty flag. */
	error = collection_script(
		realm,
		"var form=document.createElement('form');"
		"form.innerHTML='<input type=radio name=group value=a checked><input type=radio name=group value=b>';"
		"form.elements.group.value='b';form",
		&answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Reads the actual current group members and each stored checked state. */
	node = bind_node_of(answer);
	a = (struct dom_element *)node->first_child;
	b = (struct dom_element *)node->last_child;
	same = dom_control_checked(a);

	/* List selection unchecks original clean default radio. */
	observed = 0;
	if (!same)
		observed = 1;
	collection_check(observed, "list selection unchecks original clean default radio");
	same = dom_control_checked(b);
	collection_check(same, "list selection checks target radio");

	/* Group unchecking preserves clean dirty flag. */
	observed = 0;
	if (a->control->checked_dirty == 0)
		observed = 1;
	collection_check(observed, "group unchecking preserves clean dirty flag");

	/* List setter preserves target clean dirty flag. */
	observed = 0;
	if (b->control->checked_dirty == 0)
		observed = 1;
	collection_check(observed, "list setter preserves target clean dirty flag");

	/* A dirty IDL setter and a later direct list assignment have independent flag effects. */
	error = collection_script(
		realm,
		"form.elements[1].checked=false;form.elements.group.value='b';true",
		&answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The target becomes dirty while group peers retain their existing clean state. */

	/* IDL checked setter establishes target dirty flag. */
	observed = 0;
	if (b->control->checked_dirty == 1)
		observed = 1;
	collection_check(observed, "IDL checked setter establishes target dirty flag");

	/* IDL target change leaves other dirty flag clean. */
	observed = 0;
	if (a->control->checked_dirty == 0)
		observed = 1;
	collection_check(observed, "IDL target change leaves other dirty flag clean");
	same = dom_control_checked(b);
	collection_check(same, "list setter preserves existing target dirty flag while checking");

	/* Cloning a clean current group-unchecked radio copies current state without dirtying it. */
	error = collection_script(
		realm,
		"var clone=form.elements[0].cloneNode(false);clone",
		&answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The clone copies clean current checkedness into independently owned storage. */
	node = bind_node_of(answer);
	clone = (struct dom_element *)node;
	same = dom_control_checked(clone);

	/* Clean group-unchecked clone retains current false state. */
	observed = 0;
	if (!same)
		observed = 1;
	collection_check(observed, "clean group-unchecked clone retains current false state");

	/* Clean current-state clone remains clean. */
	observed = 0;
	if (clone->control->checked_dirty == 0)
		observed = 1;
	collection_check(observed, "clean current-state clone remains clean");

	/* Clone checked storage independent from source. */
	observed = 0;
	if (clone->control != a->control)
		observed = 1;
	collection_check(observed, "clone checked storage independent from source");

	/* Selecting the originally clean radio preserves its own clean flag and other's dirty flag. */
	error = collection_script(realm, "form.elements.group.value='a';true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Switching the selected member preserves both original dirty-state histories. */

	/* List selection preserves original clean flag. */
	observed = 0;
	if (a->control->checked_dirty == 0)
		observed = 1;
	collection_check(observed, "list selection preserves original clean flag");

	/* Group unchecking preserves other existing dirty flag. */
	observed = 0;
	if (b->control->checked_dirty == 1)
		observed = 1;
	collection_check(observed, "group unchecking preserves other existing dirty flag");

	/* Every owned control buffer and realm graph is released through normal finalizers. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: current checkedness and dirty checkedness followed independent native contracts. */
	return 0;
}
