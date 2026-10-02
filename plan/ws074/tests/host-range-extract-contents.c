/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies direct native Range extraction and observer ownership during actual allocation GC. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Inert ordinary allocation approaches the unchanged collector threshold. */
static const struct vm_cell_type pressure_type = { "range-extract-pressure", NULL, NULL };
/* Each independent native state and lifetime observation contributes to the result. */
static unsigned checks;
/* Failure count survives the release of native fixture state. */
static unsigned failures;

static void extract_check(int condition, const char *name);
static int extract_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int extract_case(struct vm_realm *realm, struct bind_window *window);

/*
 * Runs a direct registered extract operation after genuine Window and Range construction.
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
	status = extract_case(realm, window);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Publish complete observations after the embedding has released its roots. */
	printed = printf("native Range extract contents: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any missing collection, wrong record or lingering detached cell rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native extraction and notification graph lifetime were verified. */
	return 0;
}

/* Records one native observation while preserving later independent checks. */
static void
extract_check(
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
}

/* Constructs genuine DOM and observer participants through the ordinary interpreter. */
static int
extract_script(
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
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: a normal interpreter value is available to the embedding. */
	return 0;
}

/* Extracts a wide genuine interval while an observer records every moved child. */
static int
extract_case(
	struct vm_realm *realm,
	struct bind_window *window)
{
	struct dom_node *root;
	struct dom_node *output;
	struct vm_object *wrapper;
	struct vm_cell *state;
	struct vm_cell *roots[2];
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value receiver;
	vm_value answer;
	vm_value key;
	uintptr_t state_address;
	uintptr_t moved_address;
	unsigned index;
	unsigned registered;
	int truth;
	int status;

	UNUSED_PARAMETER(window);

	/* Two construction slots protect the real receiver and returned output at distinct times. */
	roots[0] = NULL;
	roots[1] = NULL;
	registered = 0;
	status = 0;
	for (index = 0; index < 2U; index++) {
		status = vm_heap_add_root(realm->heap, &roots[index]);
		if (status != 0)
			break;
		registered++;
	}

	/* Every error and completed extraction shares exact temporary root cleanup. */
	if (status == 0) {
		do {
			/* Native first/last Text boundaries enclose1800 original complete children. */
			status = extract_script(realm,
			    "var root=document.createElement('div');document.appendChild(root);"
			    "var left=document.createElement('b'),leftText=document.createTextNode('AB');left.appendChild(leftText);root.appendChild(left);"
			    "for(var i=0;i<1800;i++){var child=document.createElement('i');child.setAttribute('data-id',String(i));root.appendChild(child);}"
			    "var right=document.createElement('b'),rightText=document.createTextNode('XYZ');right.appendChild(rightText);root.appendChild(right);"
			    "var observer=new MutationObserver(function(){});observer.observe(root,{childList:true});"
			    "var r=document.createRange();r.setStart(leftText,1);r.setEnd(rightText,2);r", &receiver);
			if (status != 0)
				break;
			wrapper = (struct vm_object *)vm_value_as_cell(receiver);
			roots[0] = &wrapper->cell;
			state = vm_value_as_cell(wrapper->internal);
			state_address = (uintptr_t)state;

			/* An integer address records original middle identity without rooting it in the caller. */
			status = extract_script(realm, "root", &answer);
			if (status != 0)
				break;
			root = bind_node_of(answer);
			moved_address = (uintptr_t)root->first_child->next;
			status = extract_script(realm, "r=null", &answer);
			if (status != 0)
				break;
			vm_heap_set_stack_base(realm->heap, NULL);
			vm_heap_collect(realm->heap);
			pressure = vm_heap_alloc(realm->heap, &pressure_type, 7U * 1024U * 1024U);
			if (pressure == NULL) {
				status = ENOMEM;
				break;
			}

			/* Only the registered method owns the private state, source and pending output during GC. */
			roots[0] = NULL;
			vm_heap_stats(realm->heap, &before);
			answer = VM_VALUE_EMPTY;
			status = bind_range_interface.operations[16].method(realm, receiver, NULL, 0, &answer);
			vm_heap_stats(realm->heap, &after);
			if (status != 0)
				break;
			roots[1] = vm_value_as_cell(answer);
			output = bind_node_of(answer);
			extract_check(after.collections > before.collections, "direct extract triggers actual allocation GC without caller receiver frame");
			extract_check(output != NULL && output->type == DOM_DOCUMENT_FRAGMENT && output->document == root->document, "complete Fragment result uses actual current source owner");
			if (output == NULL || output->first_child == NULL || output->last_child == NULL) {
				status = EINVAL;
				break;
			}

			/* The first moved node is the exact native identity captured before extraction. */
			extract_check(output->first_child->next == (struct dom_node *)moved_address, "first fully contained native child retains exact original identity");
			extract_check(root->first_child->next == root->last_child && root->first_child->parent == root, "source retains only original partial boundary Elements");

			/* Genuine private state remains available for getter inspection without its original wrapper. */
			roots[0] = state;
			wrapper = vm_object_create(realm->heap, NULL);
			if (wrapper == NULL) {
				status = ENOMEM;
				break;
			}

			/* Only the temporary receiver is new; its internal cell is the production-created Range. */
			wrapper->kind = VM_KIND_PLATFORM;
			wrapper->internal = vm_value_cell(state);
			roots[0] = &wrapper->cell;
			receiver = vm_value_cell(wrapper);
			status = bind_abstract_range_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
			if (status != 0)
				break;
			extract_check(bind_node_of(answer) == root, "private start collapses to original native parent");
			status = bind_abstract_range_interface.attributes[1].getter(realm, receiver, NULL, 0, &answer);
			if (status != 0)
				break;
			extract_check(bind_node_of(answer) == root, "private end collapses to original native parent");
			status = bind_abstract_range_interface.attributes[2].getter(realm, receiver, NULL, 0, &answer);
			if (status != 0)
				break;
			extract_check(answer == vm_value_number(1), "private start offset is original first-branch index plus one");
			status = bind_abstract_range_interface.attributes[3].getter(realm, receiver, NULL, 0, &answer);
			if (status != 0)
				break;
			extract_check(answer == vm_value_number(1), "private end offset matches the collapsed start");
			roots[0] = NULL;

			/* Inspect the complete cloned boundaries and moved source order after mid-call GC. */
			vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
			extract_check(output->first_child->type == DOM_ELEMENT && output->last_child->type == DOM_ELEMENT, "partial outer branches remain genuine native Elements");
			status = extract_script(realm, "leftText.data==='A'&&rightText.data==='Z'&&root.childNodes.length===2", &answer);
			if (status != 0)
				break;
			truth = vm_to_boolean(answer);
			extract_check(truth, "native source keeps exact first suffix and last prefix remainders");

			/* A temporary global output reference lets the Page API inspect every moved identity. */
			key = vm_key_from_ascii(realm->heap, "extracted");
			if (key == VM_VALUE_EMPTY) {
				status = ENOMEM;
				break;
			}

			/* Global publication permits ordinary script identity inspection while C roots remain explicit. */
			status = vm_object_define(realm->heap, realm->global, key, vm_value_cell(roots[1]), VM_PROPERTY_DEFAULT);
			if (status != 0)
				break;
			status = extract_script(realm,
			    "var frag=extracted,good=frag.childNodes.length===1802&&frag.firstChild.localName==='b'&&"
			    "frag.firstChild.firstChild.data==='B'&&frag.lastChild.firstChild.data==='XY';"
			    "for(var i=0;i<1800;i++){var n=frag.childNodes[i+1];if(n.localName!=='i'||n.getAttribute('data-id')!==String(i))good=false;}good", &answer);
			if (status != 0)
				break;
			truth = vm_to_boolean(answer);
			extract_check(truth, "all1800 original complete nodes and exact partial copies appear in output order");

			/* Source notifications retain every moved identity in the original order. */
			status = extract_script(realm,
			    "var records=observer.takeRecords(),good=records.length===1800;"
			    "for(var i=0;i<records.length;i++){var x=records[i];"
			    "if(x.type!=='childList'||x.target!==root||x.addedNodes.length!==0||x.removedNodes.length!==1||"
			    "x.removedNodes[0]!==frag.childNodes[i+1])good=false;}good", &answer);
			if (status != 0)
				break;
			truth = vm_to_boolean(answer);
			extract_check(truth, "all1800 FIFO child-list records refer to exact moved native identities");

			/* Releasing output, records, source and observer makes temporary graphs collectible. */
			status = extract_script(realm,
			    "observer.disconnect();observer=null;records=null;x=null;frag=null;extracted=null;document.removeChild(root);"
			    "root=null;left=null;right=null;leftText=null;rightText=null;child=null;n=null", &answer);
			if (status != 0)
				break;
			roots[1] = NULL;
			vm_heap_set_stack_base(realm->heap, NULL);
			vm_heap_collect(realm->heap);
			found = vm_heap_find_cell(realm->heap, moved_address);
			extract_check(found == NULL, "moved native node collectible after output and observer release");
			found = vm_heap_find_cell(realm->heap, state_address);
			extract_check(found == NULL, "private Range collectible after callee and inspection roots release");
		} while (0);
	}

	/* Restore fixture stack policy and release only registered construction slots. */
	vm_heap_set_stack_base(realm->heap, __builtin_frame_address(0));
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(realm->heap, &roots[registered]);
	}

	/* Report a native allocation or interpreter failure rather than claiming completion. */
	if (status != 0)
		return status;

	/* Succeeded: original and copied/moved graph identities and lifetimes were inspected. */
	return 0;
}
