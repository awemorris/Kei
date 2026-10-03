/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks native submission and constructor graphs during actual collector pressure. */

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
static int submit_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int dictionary_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

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
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Completed native submission and constructor observations precede heap finalization. */
	vm_heap_destroy(heap);

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("submit lifetime checks: %u/%u passed\n", checks - failures, checks);
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

	/* Checked script completion no longer borrows converted fixture storage. */
	wb_units_release(&units);

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Collects while a native constructor has converted its type but not allocated its Event. */
static int
dictionary_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Explicit collection excludes every C temporary, including the constructor's converted type. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, construction_stack);
	*result = VM_VALUE_UNDEFINED;

	/* Succeeded: the real native collection checkpoint completed. */
	return 0;
}

/* Unlinks the actual form and submitter before collecting inside native event dispatch. */
static int
submit_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	vm_value argument;
	struct dom_node *form;
	struct dom_node *input;
	struct vm_cell *found;
	uintptr_t form_address;
	uintptr_t input_address;
	uintptr_t event_address;
	int same;

	UNUSED_PARAMETER(receiver);

	/* Only actual native event fields supply the callback's owning graph. */
	argument = js_argument(args, count, 0);
	event = bind_event_of(argument);
	if (event == NULL)
		return EINVAL;

	/* The branded native submission event supplies its actual target and submitter. */
	form = bind_node_of(event->target);
	input = bind_node_of(event->submitter);
	if (form == NULL ||
	    input == NULL ||
	    form->type != DOM_ELEMENT)
		return EINVAL;

	/* Integer observations add no retaining native event or DOM root. */
	form_address = (uintptr_t)form;
	input_address = (uintptr_t)input;
	event_address = (uintptr_t)event;
	collection_check(event->interface == BIND_SUBMIT_EVENT, "actual native SubmitEvent subclass");
	collection_check(
		event->trusted &&
		event->bubbles &&
		event->cancelable,
		"native submit flags during callback");
	collection_check(((struct dom_element *)form)->firing_submission_events, "form guard covers submit listeners");

	/* No DOM link remains between the native submitter and current form at collection. */
	dom_remove(form);
	dom_remove(input);
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, input_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "removed submitter survives native event trace and pending roots");
	found = vm_heap_find_cell(realm->heap, form_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "removed form survives original event dispatch");
	found = vm_heap_find_cell(realm->heap, event_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "native submission event survives callback GC");
	vm_heap_set_stack_base(realm->heap, construction_stack);

	/* Canceling ends this finite event stage without any host navigation continuation. */
	event->canceled = 1;
	*result = VM_VALUE_UNDEFINED;

	/* Succeeded: the real native collection checkpoint completed. */
	return 0;
}

/* Verifies native submission and constructor roots without an outer interpreted call frame. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct dom_element *form;
	struct dom_element *input;
	struct dom_node *node;
	struct bind_event *event;
	struct vm_cell *root;
	struct vm_cell *found;
	uintptr_t form_address;
	uintptr_t input_address;
	vm_value answer;
	int same;
	int canceled;
	int registered;
	int status;

	/* Construct the actual engine environment and two native collection callbacks. */
	status = vm_realm_create(heap, &realm);
	if (status != 0)
		return status;
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* The manual primary remains alive throughout this bounded fixture. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* No host or test-specific production hook supplies hidden submission behavior. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* The optional Event wrapper root belongs only to the final saved-state observation. */
	root = NULL;
	registered = 0;

	/* A native listener forces GC inside the pending form submission event. */
	status = js_builtin_method(realm, realm->global, "collectSubmit", 1, submit_collect);
	if (status != 0) {
		goto cleanup;
	}

	/* A separate dictionary getter exercises converted-type retention before event allocation. */
	status = js_builtin_method(realm, realm->global, "collectDictionary", 0, dictionary_collect);
	if (status != 0) {
		goto cleanup;
	}

	/* No global script binding or active call frame retains the returned form or its input. */
	status = collection_script(
	    realm,
	    "(function(){var f=document.createElement('form');var i=document.createElement('input');"
	    "i.type='submit';f.appendChild(i);f.onsubmit=collectSubmit;document.appendChild(f);return f;})()",
	    &answer);
	if (status != 0) {
		goto cleanup;
	}

	/* Native submission alone owns the callback graph during explicit collection. */
	node = bind_node_of(answer);
	if (node == NULL ||
	    node->type != DOM_ELEMENT ||
	    node->first_child == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* A real native form and its constructed submitter supply the pending dispatch graph. */
	form = (struct dom_element *)node;
	input = (struct dom_element *)form->node.first_child;
	form_address = (uintptr_t)form;
	input_address = (uintptr_t)input;
	answer = VM_VALUE_UNDEFINED;
	status = bind_submit_event(window, form, input, &canceled);
	if (status != 0) {
		goto cleanup;
	}

	/* Callback cancellation and the form guard survive removal and are reconciled before return. */
	collection_check(canceled, "native callback cancellation reported");
	collection_check(!form->firing_submission_events, "form guard released after submit callback GC");
	form = NULL;
	input = NULL;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, form_address);
	collection_check(found == NULL, "form reclaimed after last dispatch root released");
	found = vm_heap_find_cell(heap, input_address);
	collection_check(found == NULL, "submitter reclaimed after last dispatch root released");
	vm_heap_set_stack_base(heap, construction_stack);

	/* The converted temporary type string must survive getters which invoke actual collection. */
	status = collection_script(
	    realm,
	    "(function(){var e=new SubmitEvent({toString:function(){return 'ephemeral-'+'type';}},"
	    "{get bubbles(){collectDictionary();return true;},get composed(){collectDictionary();return true;}});"
	    "return e.type==='ephemeral-type' && e.bubbles && e.composed && e.submitter===null;})()",
	    &answer);
	if (status != 0) {
		goto cleanup;
	}

	/* Publication proves that converted native state survived the preallocation getter checkpoints. */
	same = vm_to_boolean(answer);
	collection_check(same, "constructor converted type retained across dictionary GC");

	/* A sole constructed Event, not the tree, retains its actual disconnected submitter. */
	status = collection_script(
	    realm,
	    "(function(){var i=document.createElement('input');i.value='held';"
	    "return new SubmitEvent('submit',{submitter:i});})()",
	    &answer);
	if (status != 0) {
		goto cleanup;
	}

	/* Register only the Event wrapper, then exclude every native stack from tracing. */
	event = bind_event_of(answer);
	if (event == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* The successful constructed Event must retain a real disconnected submitter. */
	node = bind_node_of(event->submitter);
	if (node == NULL || node->type != DOM_ELEMENT) {
		status = EINVAL;
		goto cleanup;
	}

	/* Only this complete Event receives the original post-construction caller root. */
	root = vm_value_as_cell(answer);
	input = (struct dom_element *)node;
	input_address = (uintptr_t)input;
	status = vm_heap_add_root(heap, &root);
	if (status != 0) {
		goto cleanup;
	}

	/* Cleanup owns the wrapper slot only after its successful registration. */
	registered = 1;

	/* Only the native event trace supplies reachability to this disconnected input. */
	input = NULL;
	answer = VM_VALUE_UNDEFINED;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, input_address);
	collection_check(found != NULL, "sole SubmitEvent strongly retains native submitter");
	root = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, input_address);
	collection_check(found == NULL, "submitter reclaimed after sole event root released");
	vm_heap_set_stack_base(heap, construction_stack);
	vm_heap_remove_root(heap, &root);
	registered = 0;

cleanup:
	/* Submission cleanup restores the embedding even after a script or callback refusal. */
	vm_heap_set_stack_base(heap, construction_stack);
	if (registered)
		vm_heap_remove_root(heap, &root);

	/* No expired constructor or submission root slot survives primary teardown. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (status != 0)
		return status;

	/* Succeeded: native submission, construction and eventual release were observed. */
	return 0;
}
