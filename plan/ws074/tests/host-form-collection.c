/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks form cache cycles and sole duplicate-list retention using actual production GC. */

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
	printed = printf("form collection GC checks: %u/%u passed\n", checks - failures, checks);
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

/* Verifies cache-only and duplicate-only ownership without conservative stack roots. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct dom_node *node;
	struct dom_element *form;
	struct vm_cell *found;
	struct vm_cell *observed;
	vm_value answer;
	uintptr_t form_address;
	uintptr_t cache_address;
	uintptr_t member_address;
	uintptr_t list_address;
	int same;
	int error;

	/* The ordinary embedding retains its primary while the XML graph is collectible. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* No callback or hidden engine switch affects collection reachability. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Initialize the ordinary host with no scheduled external effects. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Form alone retains its SameObject cache; the original duplicate-name key is dynamic. */
	error = collection_script(realm,
				  "var xml=document.implementation.createDocument(null,'Root');"
				  "var form=xml.createElementNS('http://www.w3.org/1999/xhtml','form');"
				  "xml.documentElement.appendChild(form);"
				  "var key='held-'+String(1234);"
				  "var a=xml.createElementNS('http://www.w3.org/1999/xhtml','input');a.id=key;a.checked=true;a.value='owned';a.defaultValue='raw';form.appendChild(a);"
				  "var b=xml.createElementNS('http://www.w3.org/1999/xhtml','button');b.setAttribute('name',key);form.appendChild(b);"
				  "form.elements;xml=null;a=null;b=null;form",
				  &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Selected addresses are observations, not registered roots or raw post-GC dereferences. */
	node = bind_node_of(answer);
	form = (struct dom_element *)node;
	form_address = (uintptr_t)form;
	cache_address = (uintptr_t)form->controls_collection;
	member_address = (uintptr_t)node->first_child;

	/* Native DOM tracing must preserve the cache even without a saved collection variable. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, cache_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "form alone retains controls SameObject cache");
	vm_heap_set_stack_base(heap, construction_stack);

	/* The getter consumes the surviving cache through the actual public binding. */
	error = collection_script(realm, "form.elements", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Record the independently observed public result. */
	same = 0;
	observed = vm_value_as_cell(answer);
	if ((uintptr_t)observed == cache_address)
		same = 1;
	collection_check(same, "controls identity unchanged after cache-only GC");

	/* Only the list remains as a script root; its name has no independent variable. */
	error = collection_script(realm,
				  "var saved=form.elements.namedItem(key);form=null;key=null;saved", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Keep only an address for subsequent reachability observations. */
	list_address = (uintptr_t)vm_value_as_cell(answer);

	/* The duplicate state must retain its form, members, owner and original name. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, form_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "sole duplicate list retains owning form");
	found = vm_heap_find_cell(heap, member_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "sole duplicate list retains actual member");
	found = vm_heap_find_cell(heap, cache_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "sole duplicate list reaches form cache cycle");
	vm_heap_set_stack_base(heap, construction_stack);

	/* Public live membership uses the captured name after real collection. */
	error = collection_script(realm,
				  "saved.length===2&&saved.item(0).id==='held-1234'&&saved.item(1).getAttribute('name')==='held-1234'", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Record the independently observed public result. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "duplicate captured name and XML prototypes survive actual GC");

	/* Owned input text remains independent from its default attribute after actual GC. */
	error = collection_script(realm,
				  "saved[0].value==='owned'&&saved[0].defaultValue==='raw'", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The only script root reaches input native accessors and separately owned value storage. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "sole list retains dirty input value independently of default");

	/* Checkedness shares native owned state but never creates a content default. */
	error = collection_script(realm,
				  "saved[0].checked===true&&!saved[0].hasAttribute('checked')", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Current boolean state survives through the same sole list and actual GC graph. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "sole list retains actual checkedness without a default attribute");

	/* Member mutation is observed without retaining a removed snapshot member in the list. */
	error = collection_script(realm,
				  "saved[0].id='changed';saved.length===1&&saved.item(0).getAttribute('name')==='held-1234'", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Record the independently observed public result. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "sole duplicate list remains live after GC and name mutation");

	/* Removing the last script edge makes the entire list/form/cache cycle collectible. */
	error = collection_script(realm, "saved=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Exclude C temporaries while observing the selected graph. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, list_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last duplicate root removal reclaims list");
	found = vm_heap_find_cell(heap, form_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last duplicate root removal reclaims form");
	found = vm_heap_find_cell(heap, cache_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last duplicate root removal reclaims controls cache cycle");
	found = vm_heap_find_cell(heap, member_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last duplicate root removal reclaims member");
	vm_heap_set_stack_base(heap, construction_stack);

	/* Primary ownership is retired only after every observation has completed. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: every selected graph was retained and then reclaimed by actual GC. */
	return 0;
}
