/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies direct native Range deletion and observer ownership during actual allocation GC. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Inert ordinary allocation approaches the unchanged collector threshold. */
static const struct vm_cell_type pressure_type = { "range-delete-pressure", NULL, NULL };
/* Each independent native state and lifetime observation contributes to the result. */
static unsigned checks;
/* Failure count survives the release of native fixture state. */
static unsigned failures;

static void delete_check(int condition, const char *name);
static int delete_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int delete_case(struct vm_realm *realm, struct bind_window *window);

/*
 * Runs a direct registered delete operation after genuine Window and Range construction.
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

	/* Ordinary fixture installation supplies actual private state and observer APIs. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Builtins and primary Document own the production binding graph. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* A primary Window is installed on an initially empty native Document. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* No alternate host removal callback changes the native path under test. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Direct native invocation disables conservative stack scanning only inside this case. */
	status = delete_case(realm, window);
	if (status != 0) {
		vm_heap_set_stack_base(heap, __builtin_frame_address(0));
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Restore conservative construction before completed fixture teardown. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Publish complete observations after the embedding has released its roots. */
	printed = printf("native Range delete contents: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any missing collection, wrong record or lingering detached cell rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native deletion and notification graph lifetime were verified. */
	return 0;
}

/* Records one native observation while preserving later independent checks. */
static void
delete_check(
	int condition,
	const char *name)
{
	int printed;

	/* Keep every failed identity or lifetime check in the fixture's final result. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: this named observation contributes to the final fixture outcome. */
	return;
}

/* Constructs genuine DOM and observer participants through the ordinary interpreter. */
static int
delete_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* The browser parser sees exactly the script syntax the Page probe uses. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Ordinary script execution provides the actual registered Range private cell. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release source storage only after interpreter completion was checked. */
	wb_units_release(&units);

	/* Succeeded: a normal interpreter value is available to the embedding. */
	return 0;
}

/* Deletes a wide genuine child interval while an observer allocates after each removal. */
static int
delete_case(
	struct vm_realm *realm,
	struct bind_window *window)
{
	struct dom_node *root;
	struct dom_node *container;
	struct vm_object *wrapper;
	struct vm_cell *state;
	struct vm_cell *roots[1];
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value receiver;
	vm_value answer;
	uintptr_t state_address;
	uintptr_t first_address;
	unsigned registered;
	int is_object;
	int is_cell;
	int truth;
	int status;

	UNUSED_PARAMETER(window);

	/* One temporary construction root owns the receiver until direct callee entry. */
	roots[0] = NULL;
	registered = 0;
	status = vm_heap_add_root(realm->heap, &roots[0]);
	if (status != 0)
		return status;
	registered = 1;

	/* A genuine primary Document needs its first Element before this host fixture can select it. */
	status = delete_script(realm,
			       "var root=document.createElement('div');document.appendChild(root);"
			       "for(var i=0;i<1800;i++){var child=document.createElement('i');child.setAttribute('data-id',String(i));root.appendChild(child);}"
			       "var observer=new MutationObserver(function(){});observer.observe(root,{childList:true});"
			       "var r=document.createRange();r.selectNodeContents(root);r",
			       &receiver);
	if (status != 0)
		goto cleanup;
	is_object = vm_value_is_object(receiver);
	if (!is_object) {
		status = EIO;
		goto cleanup;
	}

	/* Retain only the checked native Range wrapper for construction. */
	wrapper = (struct vm_object *)vm_value_as_cell(receiver);
	if (wrapper->kind != VM_KIND_PLATFORM) {
		status = EIO;
		goto cleanup;
	}

	/* Native state must be a real private cell before its address is observed. */
	is_cell = vm_value_is_cell(wrapper->internal);
	if (!is_cell) {
		status = EIO;
		goto cleanup;
	}

	/* Publish the checked construction receiver before reading its private cell. */
	roots[0] = &wrapper->cell;
	state = vm_value_as_cell(wrapper->internal);
	state_address = (uintptr_t)state;
	status = delete_script(realm, "root", &answer);
	if (status != 0)
		goto cleanup;
	root = bind_node_of(answer);
	if (root == NULL || root->first_child == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Record a native address without an additional retaining root. */
	first_address = (uintptr_t)root->first_child;
	status = delete_script(realm, "r=null", &answer);
	if (status != 0)
		goto cleanup;

	/* Construction roots keep the genuine private Range until pressure has been prepared. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	pressure = vm_heap_alloc(realm->heap, &pressure_type, 7U * 1024U * 1024U);
	if (pressure == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Only the registered native callee may retain state during observer allocations. */
	roots[0] = NULL;
	vm_heap_stats(realm->heap, &before);
	answer = VM_VALUE_EMPTY;
	status = bind_range_interface.operations[15].method(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		goto cleanup;

	/* Inspect collector accounting only after complete native deletion. */
	vm_heap_stats(realm->heap, &after);
	delete_check(answer == VM_VALUE_UNDEFINED, "direct delete returns undefined after complete mutation");
	delete_check(after.collections > before.collections, "direct delete triggers actual allocation GC without caller receiver frame");
	delete_check(root->first_child == NULL && root->last_child == NULL, "all1800 genuine native children removed in original order");

	/* Retain only the genuine private state while constructing a temporary inspection receiver. */
	state = vm_heap_find_cell(realm->heap, state_address);
	if (state == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Retain actual private state across inspection-wrapper allocation. */
	roots[0] = state;

	/* Allocate inspection storage only after rooting the actual retained state. */
	wrapper = vm_object_create(realm->heap, NULL);
	if (wrapper == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Existing getters read the real private cell even after its original wrapper was released. */
	wrapper->kind = VM_KIND_PLATFORM;
	wrapper->internal = vm_value_cell(state);
	roots[0] = &wrapper->cell;
	receiver = vm_value_cell(wrapper);
	status = bind_abstract_range_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		goto cleanup;
	container = bind_node_of(answer);
	delete_check(container == root, "private start container collapses to the original native parent");
	status = bind_abstract_range_interface.attributes[1].getter(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		goto cleanup;
	container = bind_node_of(answer);
	delete_check(container == root, "private end container collapses to the original native parent");
	status = bind_abstract_range_interface.attributes[2].getter(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		goto cleanup;
	delete_check(answer == vm_value_number(0), "private start offset equals the pre-mutation collapse point");
	status = bind_abstract_range_interface.attributes[3].getter(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		goto cleanup;
	delete_check(answer == vm_value_number(0), "private end offset equals the pre-mutation collapse point");
	roots[0] = NULL;

	/* Pending records retain detached identities after the callee's roots have returned. */
	vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
	status = delete_script(realm,
			       "var records=observer.takeRecords(),good=records.length===1800;"
			       "for(var i=0;i<records.length;i++){var x=records[i];"
			       "if(x.type!=='childList'||x.target!==root||x.addedNodes.length!==0||x.removedNodes.length!==1||"
			       "x.removedNodes[0].getAttribute('data-id')!==String(i)||x.removedNodes[0].parentNode!==null)good=false;}good",
			       &answer);
	if (status != 0)
		goto cleanup;
	truth = vm_to_boolean(answer);
	delete_check(truth, "all1800 FIFO child-list records own exact detached native identities");
	status = delete_script(realm, "root.childNodes.length===0&&records.length===1800", &answer);
	if (status != 0)
		goto cleanup;
	truth = vm_to_boolean(answer);
	delete_check(truth, "native root and published record list remain complete after collection");

	/* Releasing records and the source root makes the detached graph collectible. */
	status = delete_script(realm,
			       "observer.disconnect();records=null;observer=null;document.removeChild(root);root=null;child=null;x=null", &answer);
	if (status != 0)
		goto cleanup;
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, first_address);
	delete_check(found == NULL, "detached first child collectible after record and source release");
	found = vm_heap_find_cell(realm->heap, state_address);
	delete_check(found == NULL, "genuine private Range collectible after callee ownership ends");

	/* All native samples completed before shared construction-root teardown. */
	status = 0;

cleanup:
	/* Restore ordinary fixture stack policy and unregister exactly one construction root. */
	vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(realm->heap, &roots[registered]);
	}

	/* A native allocation or interpreter error rejects the bounded direct case. */
	if (status != 0)
		return status;

	/* Succeeded: all retained identities and later release states were inspected. */
	return 0;
}
