/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies pure pre-insertion validation against genuine rooted native graphs. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Host notifications are observed without retaining any collectible participant. */
struct insertion_observer {
	unsigned inserted;
	unsigned removed;
};

/* Independent native observations accumulate until the embedding is destroyed. */
static unsigned checks;
/* A failed observation determines the exit status after later checks run. */
static unsigned failures;

static void insertion_check(int condition, const char *name);
static int insertion_notify(void *context, struct dom_node *node);
static void insertion_removed(void *context, struct dom_node *node);
static int insertion_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int insertion_case(struct vm_realm *realm, struct insertion_observer *observer, const void *stack_base);

/*
 * Exercises pure validation with actual binding exceptions and native host observations.
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
	struct insertion_observer observer;
	int status;
	int printed;

	/* Construction uses the real collector stack convention. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Production builtins and a genuine Document supply ordinary DOM exceptions. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The primary Document supplies the host while incoming XML owners remain distinct. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* A successful validation must never reach the normal insertion host hook. */
	memset(&observer, 0, sizeof(observer));
	memset(&host, 0, sizeof(host));
	host.context = &observer;
	host.node_inserted = insertion_notify;
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Observe validation separately from actual mutation with explicit caller ownership. */
	status = insertion_case(realm, &observer, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Behavior failures are distinct from embedding allocation failures. */
	printed = printf("pre-insertion native checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any observed mutation by validation invalidates the prerequisite. */
	if (failures != 0)
		return 1;

	/* Succeeded: validation preserved the rooted native participants. */
	return 0;
}

/* Records one independently observable graph or host contract. */
static void
insertion_check(
	int condition,
	const char *name)
{
	int printed;

	/* Preserve accounting even when an earlier native contract failed. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation contributes to the final count. */
	return;
}

/* Counts genuine insertion deliveries without changing the graph. */
static int
insertion_notify(
	void *context,
	struct dom_node *node)
{
	struct insertion_observer *observer;

	UNUSED_PARAMETER(node);

	/* Only actual insertion is allowed to change this host counter. */
	observer = context;
	observer->inserted++;

	/* Succeeded: the ordinary host delivery was observed. */
	return 0;
}

/* Counts original-Document removal deliveries before unlinking. */
static void
insertion_removed(
	void *context,
	struct dom_node *node)
{
	struct insertion_observer *observer;

	UNUSED_PARAMETER(node);

	/* Validation cannot start removal or adoption notifications. */
	observer = context;
	observer->removed++;

	/* Succeeded: the actual removal was observed. */
	return;
}

/* Constructs genuine nodes through the default production script bindings. */
static int
insertion_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Convert the fixture without invoking any alternate binding path. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* The actual script engine supplies native wrappers and original owner graphs. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the genuine completion is available to the embedding. */
	return 0;
}

/* Observes validation separately from insertion with only explicit roots during collection. */
static int
insertion_case(
	struct vm_realm *realm,
	struct insertion_observer *observer,
	const void *stack_base)
{
	struct vm_cell *destination_root;
	struct vm_cell *source_root;
	struct dom_node *destination;
	struct dom_node *source;
	struct dom_node *fragment;
	struct dom_node *child;
	struct dom_document *owner;
	struct dom_removal_subscription *subscription;
	vm_value answer;
	uint32_t destination_generation;
	uint32_t source_generation;
	unsigned baseline;
	unsigned removals;
	uintptr_t address;
	struct vm_cell *found;
	int status;
	int same;

	/* Retain a detached XML Document solely through its returned native wrapper. */
	destination_root = NULL;
	source_root = NULL;
	status = vm_heap_add_root(realm->heap, &destination_root);
	if (status != 0)
		return status;
	status = vm_heap_add_root(realm->heap, &source_root);
	if (status != 0) {
		vm_heap_remove_root(realm->heap, &destination_root);
		return status;
	}

	/* Bounded setup permits cleanup after any failed script or subscription allocation. */
	subscription = NULL;
	do {
		/* Destination and source have distinct creators and no global script names. */
		status = insertion_script(realm, "document.implementation.createDocument(null,null,null)", &answer);
		if (status != 0)
			break;
		destination_root = vm_value_as_cell(answer);
		destination = bind_node_of(answer);
		status = insertion_script(realm,
					  "(function(){var d=document.implementation.createDocument(null,null,null);"
					  "var p=d.createElement('parent'),f=d.createDocumentFragment();"
					  "f.appendChild(d.createElement('one'));p.appendChild(f.firstChild);"
					  "p.appendChild(d.createElement('two'));return p;})()",
					  &answer);
		if (status != 0)
			break;
		source_root = vm_value_as_cell(answer);
		source = bind_node_of(answer);
		owner = source->document;
		child = source->first_child;
		baseline = observer->inserted;
		status = dom_removal_subscribe(owner, observer, insertion_removed, &subscription);
		if (status != 0)
			break;

		/* Explicit wrapper roots keep both native creators alive without conservative stack scanning. */
		vm_heap_set_stack_base(realm->heap, NULL);
		vm_heap_collect(realm->heap);
		address = (uintptr_t)owner;
		found = vm_heap_find_cell(realm->heap, address);
		insertion_check(found != NULL, "caller root retains distinct incoming creator during collection");
		destination_generation = destination->document->generation;
		source_generation = owner->generation;

		/* A valid cross-Document insertion is tested without actually removing or adopting its child. */
		status = bind_validate_insert(realm, destination, child, NULL);
		insertion_check(status == 0, "pure cross-Document element validation succeeds");
		insertion_check(child->parent == source, "validation leaves incoming child attached");
		insertion_check(child->document == owner, "validation never adopts incoming owner");
		insertion_check(destination->first_child == NULL, "validation never publishes destination children");
		insertion_check(destination->document->generation == destination_generation, "valid validation preserves destination generation");
		insertion_check(owner->generation == source_generation, "valid validation preserves original generation");
		insertion_check(observer->inserted == baseline, "valid validation never invokes insertion host");
		insertion_check(observer->removed == 0, "valid validation never notifies original removal");

		/* The actual insertion still supplies both original removal and normal host notifications. */
		status = bind_insert(realm, destination, child, NULL);
		if (status != 0) {
			vm_heap_set_stack_base(realm->heap, stack_base);
			break;
		}

		/* Original and destination observers distinguish actual insertion from pure checks. */
		insertion_check(observer->inserted == baseline + 1U, "actual insertion invokes normal host exactly once");
		insertion_check(observer->removed == 1U, "actual adoption notifies original removal exactly once");
		insertion_check(child->document == destination->document, "actual insertion performs checked adoption");

		/* A valid Fragment check leaves its attached children and owner unchanged. */
		vm_heap_set_stack_base(realm->heap, stack_base);
		fragment = dom_fragment_create(owner);
		if (fragment == NULL) {
			status = ENOMEM;
			break;
		}

		/* Native setup places the transferred element back into its original owner Fragment. */
		dom_append_child(source, fragment);
		status = dom_adopt(owner, child);
		if (status != 0)
			break;
		dom_append_child(fragment, child);

		/* Snapshot every externally observable mutation counter before pure validation. */
		destination_generation = destination->document->generation;
		source_generation = owner->generation;
		baseline = observer->inserted;
		status = bind_validate_insert(realm, destination, fragment, NULL);
		insertion_check(status == 0, "pure whole Fragment validation succeeds");
		same = 0;
		if (fragment->parent == source &&
		    fragment->first_child == child &&
		    child->parent == fragment)
			same = 1;
		insertion_check(same, "valid Fragment validation preserves all native links");
		insertion_check(owner->generation == source_generation, "valid Fragment preserves original generation");
		insertion_check(destination->document->generation == destination_generation, "valid Fragment preserves destination generation");
		insertion_check(observer->inserted == baseline, "valid Fragment never invokes host");

		/* A second element makes the whole Fragment invalid before any participant moves. */
		dom_append_child(fragment, source->first_child);
		destination_generation = destination->document->generation;
		source_generation = owner->generation;
		removals = observer->removed;
		status = bind_validate_insert(realm, destination, fragment, NULL);
		insertion_check(status == VM_THROWN, "invalid Fragment yields actual DOM exception");
		insertion_check(fragment->parent == source, "invalid Fragment remains attached to original graph");
		insertion_check(fragment->first_child == child, "invalid Fragment retains its first child");
		insertion_check(child->next == fragment->last_child, "invalid Fragment retains complete child order");
		insertion_check(fragment->document == owner, "invalid Fragment is never adopted");
		insertion_check(owner->generation == source_generation, "invalid Fragment preserves original generation");
		insertion_check(destination->document->generation == destination_generation, "invalid Fragment preserves destination generation");
		insertion_check(observer->inserted == baseline, "invalid Fragment never invokes insertion host");
		insertion_check(observer->removed == removals, "invalid Fragment never invokes original removal");

		/* Null embedding inputs fail without requiring an exception allocation. */
		status = bind_validate_insert(NULL, destination, fragment, NULL);
		insertion_check(status == EINVAL, "null realm fails with EINVAL");
		status = bind_validate_insert(realm, NULL, fragment, NULL);
		insertion_check(status == EINVAL, "null parent fails with EINVAL");
		status = bind_validate_insert(realm, destination, NULL, NULL);
		insertion_check(status == EINVAL, "null incoming node fails with EINVAL");
		status = 0;
	} while (0);

	/* Tokens and explicit roots are released before their embedding can be destroyed. */
	dom_removal_unsubscribe(subscription);
	vm_heap_remove_root(realm->heap, &source_root);
	vm_heap_remove_root(realm->heap, &destination_root);
	if (status != 0)
		return status;

	/* Succeeded: pure validation and actual insertion supplied distinct observed effects. */
	return 0;
}
