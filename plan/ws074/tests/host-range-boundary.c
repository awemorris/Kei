/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks range boundary conversion ownership during actual collector pressure. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One synchronous host invocation observes integer addresses without retaining DOM cells. */
struct range_observer {
	uintptr_t state;
	uintptr_t source_state;
	uintptr_t creator;
	uintptr_t old;
	uintptr_t target;
	uintptr_t argument;
	unsigned conversions;
	unsigned failure;
};

/* Assertions accumulate across callback-driven and embedding-driven collection. */
static unsigned checks;
/* Failed behavioral checks determine the final exit status. */
static unsigned failures;
/* The embedding restores this construction stack boundary after explicit collection. */
static const void *construction_stack;

/* The synchronous callback sees integer-only observer data until its native invocation returns. */
static struct range_observer *active_observer;

static void range_check(int condition, const char *name);
static int range_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int range_case(struct vm_heap *heap);
static int offset_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

/*
 * Verifies rooted boundary conversion and collectible DOM cycles through real host callbacks.
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
	error = range_case(heap);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Release the embedding only after its native case completed. */
	vm_heap_destroy(heap);

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("range boundary lifetime checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Every independently observed GC contract must hold. */
	if (failures != 0)
		return 1;

	/* Succeeded: native boundary graphs survived actual GC. */
	return 0;
}

/* Records one named ownership contract without hiding later independent observations. */
static void
range_check(
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
range_script(
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

	/* Release borrowed source storage after interpreter completion is checked. */
	wb_units_release(&units);

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Collects inside numeric conversion with all unrelated C stack retention explicitly disabled. */
static int
offset_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_cell *found;
	struct dom_node *target;
	struct dom_node *child;
	int status;

	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Actual current length changes before validation; the direct invocation has no outer VM receiver frame. */
	active_observer->conversions++;
	target = (struct dom_node *)active_observer->target;
	child = (struct dom_node *)dom_text_create(target->document, NULL, 0);
	if (child == NULL)
		return ENOMEM;
	dom_append_child(target, child);
	dom_remove(target);

	/* Temporary native roots alone preserve state, both previous boundaries, creator and conversion inputs. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, active_observer->state);
	range_check(found != NULL, "native range state survives conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->creator);
	range_check(found != NULL, "native creation XML Document survives conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->old);
	range_check(found != NULL, "previous native text endpoint survives conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->target);
	range_check(found != NULL, "incoming boundary node survives conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->argument);
	range_check(found != NULL, "numeric argument survives conversion GC");

	/* Comparison's second Range wrapper must retain its own native state through first-argument conversion. */
	if (active_observer->source_state != 0) {
		found = vm_heap_find_cell(realm->heap, active_observer->source_state);
		range_check(found != NULL, "source Range native state survives mode conversion GC");
	}

	/* Restore ordinary construction after verifying only explicit native roots under collector pressure. */
	vm_heap_set_stack_base(realm->heap, construction_stack);
	*result = vm_value_int32(1);

	/* Native host errors and genuine JavaScript exceptions must unwind every temporary slot. */
	if (active_observer->failure == 1)
		return EIO;

	/* Preserve a genuine engine exception rather than converting it to a successful offset. */
	if (active_observer->failure == 2) {
		status = vm_throw_type_error(realm, "Range conversion test exception.");
		if (status != VM_THROWN)
			return status;

		/* Succeeded: the requested genuine exception is installed. */
		return VM_THROWN;
	}

	/* Succeeded: the caller observes one child added during conversion. */
	return 0;
}

/* Exercises direct registry calls, detached graphs, current lengths and failure cleanup through actual GC. */
static int
range_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct range_observer observer;
	struct vm_object *wrapper;
	struct dom_node *previous;
	struct dom_node *target;
	struct vm_cell *state_root;
	struct vm_cell *found;
	vm_value receiver;
	vm_value arguments[2];
	vm_value comparison_args[2];
	vm_value other;
	vm_value expected_order;
	vm_value answer;
	unsigned kind;
	int status;
	int expected;
	int is_object;
	int is_cell;

	/* Install the production primary binding for creation only, without behavior switches. */
	status = vm_realm_create(heap, &realm);
	if (status != 0)
		return status;
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* The primary Document is independently bound; each tested XML creator is not. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* A real valueOf callback triggers actual collection during unsigned offset conversion. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* The host function is called by the actual numeric conversion, not directly by the fixture. */
	status = js_builtin_method(realm, realm->global, "collectOffset", 0, offset_collect);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return status;
	}

	/* Each independent attempt has an unrooted wrapper and no live JS call frame retaining its inputs. */
	for (kind = 0; kind < 3U; kind++) {
		/* The Range alone owns its otherwise detached XML creator and old text endpoints. */
		status = range_script(
		    realm,
		    "(function(){var d=document.implementation.createDocument(null,null,null);"
		    "var t=d.createTextNode('old');var r=d.createRange();r.selectNodeContents(t);return r;})()",
		    &receiver);
		if (status != 0)
			break;

		/* Observe native cells only by integer addresses after setup. */
		is_object = vm_value_is_object(receiver);
		if (!is_object) {
			status = EIO;
			break;
		}

		/* Read only the generated platform wrapper's actual private cell. */
		wrapper = (struct vm_object *)vm_value_as_cell(receiver);
		is_cell = vm_value_is_cell(wrapper->internal);
		if (wrapper->kind != VM_KIND_PLATFORM || !is_cell) {
			status = EIO;
			break;
		}

		/* Save only nonretaining observer addresses for this conversion. */
		memset(&observer, 0, sizeof(observer));
		observer.state = (uintptr_t)vm_value_as_cell(wrapper->internal);
		status = bind_abstract_range_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
		if (status != 0)
			break;
		previous = bind_node_of(answer);
		if (previous == NULL) {
			status = EIO;
			break;
		}

		/* Save addresses without adding a native retaining edge. */
		observer.creator = (uintptr_t)&previous->document->node;
		observer.old = (uintptr_t)previous;
		status = range_script(realm, "document.createElement('div')", &arguments[0]);
		if (status != 0)
			break;
		target = bind_node_of(arguments[0]);
		if (target == NULL) {
			status = EIO;
			break;
		}

		/* Observe the incoming native node without rooting it. */
		observer.target = (uintptr_t)target;
		status = range_script(realm, "({valueOf:collectOffset})", &arguments[1]);
		if (status != 0)
			break;
		observer.argument = (uintptr_t)vm_value_as_cell(arguments[1]);
		observer.failure = kind;

		/* The native implementation must retain every input during its own synchronous callback. */
		expected = 0;
		if (kind == 1)
			expected = EIO;
		if (kind == 2)
			expected = VM_THROWN;

		/* Publish C observer data only for the direct native invocation. */
		active_observer = &observer;
		status = bind_range_interface.operations[0].method(realm, receiver, arguments, 2, &answer);
		if (status != expected) {
			active_observer = NULL;
			break;
		}

		/* Release the synchronous observer only after its expected outcome is checked. */
		active_observer = NULL;
		range_check(status == expected, "exact numeric conversion outcome propagated");
		range_check(observer.conversions == 1U, "offset conversion runs once during real GC");

		/* The wrapper may be collected; an explicit post-call state root permits native endpoint inspection. */
		state_root = vm_heap_find_cell(heap, observer.state);
		if (state_root == NULL) {
			status = EIO;
			break;
		}

		/* Acquire the post-call root only for a verified live private cell. */
		status = vm_heap_add_root(heap, &state_root);
		if (status != 0)
			break;

		/* Only native state remains rooted; a successful point replacement must still retain its distinct creator. */
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.state);
		range_check(found != NULL, "post-call native state survives its sole explicit root");
		found = vm_heap_find_cell(heap, observer.creator);
		range_check(found != NULL, "original creator survives after endpoint ownership changes");

		/* Strong native fields retain current endpoints only; no weak token retains superseded inputs. */
		if (kind == 0) {
			found = vm_heap_find_cell(heap, observer.old);
			range_check(found == NULL, "superseded endpoint is not retained by original weak token");
		} else {
			found = vm_heap_find_cell(heap, observer.target);
			range_check(found == NULL, "failed conversion target is not retained by current token");
		}

		/* Restore ordinary construction before republishing only the already retained genuine state. */
		vm_heap_set_stack_base(heap, construction_stack);
		wrapper = vm_object_create(heap, window->prototypes[BIND_RANGE]);
		if (wrapper == NULL) {
			vm_heap_remove_root(heap, &state_root);
			status = ENOMEM;
			break;
		}

		/* Only the test repackages the existing genuine private cell for post-call accessor inspection. */
		wrapper->kind = VM_KIND_PLATFORM;
		wrapper->internal = vm_value_cell(state_root);
		receiver = vm_value_cell(wrapper);
		status = bind_abstract_range_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
		if (status != 0) {
			vm_heap_remove_root(heap, &state_root);
			break;
		}

		/* Observe the verified accessor without allocating another retaining wrapper. */
		previous = bind_node_of(answer);
		expected = 0;
		if (kind == 0 && (uintptr_t)previous == observer.target)
			expected = 1;
		if (kind != 0 && (uintptr_t)previous == observer.old)
			expected = 1;
		range_check(expected, "successful conversion commits target while failed conversion preserves boundary");

		/* The successful offset uses the callback's current child count rather than its original zero length. */
		if (kind == 0) {
			status = bind_abstract_range_interface.attributes[2].getter(realm, receiver, NULL, 0, &answer);
			if (status != 0) {
				vm_heap_remove_root(heap, &state_root);
				break;
			}

			/* The checked getter reports the current native offset. */
			range_check(answer == vm_value_int32(1), "offset validated against post-conversion current length");
		}

		/* The exception slot retains only the exception object; input cells must become independently collectible. */
		vm_heap_remove_root(heap, &state_root);
		receiver = VM_VALUE_UNDEFINED;
		arguments[0] = VM_VALUE_UNDEFINED;
		arguments[1] = VM_VALUE_UNDEFINED;
		answer = VM_VALUE_UNDEFINED;
		previous = NULL;
		target = NULL;
		wrapper = NULL;
		state_root = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.state);
		range_check(found == NULL, "range state reclaimed after native roots unwind");
		found = vm_heap_find_cell(heap, observer.creator);
		range_check(found == NULL, "XML creator reclaimed after native roots unwind");
		found = vm_heap_find_cell(heap, observer.target);
		range_check(found == NULL, "target reclaimed after native roots unwind");
		found = vm_heap_find_cell(heap, observer.old);
		range_check(found == NULL, "previous endpoint reclaimed after native roots unwind");
		found = vm_heap_find_cell(heap, observer.argument);
		range_check(found == NULL, "conversion argument reclaimed after native roots unwind");
		vm_heap_set_stack_base(heap, construction_stack);
		if (status != 0)
			break;
	}

	/* Numeric comparison protects both genuine ranges without an outer VM argument frame. */
	for (kind = 0; status == 0 && kind < 3U; kind++) {
		/* Both independent native states initially share one detached actual XML element interval. */
		status = range_script(
		    realm,
		    "(function(){var d=document.implementation.createDocument(null,null,null);"
		    "var box=d.createElement('box');box.appendChild(d.createTextNode('old'));"
		    "var r=d.createRange();r.selectNodeContents(box);return r;})()",
		    &receiver);
		if (status != 0)
			break;

		/* Actual source cloning has independent weak ownership and the same current native points. */
		status = bind_range_interface.operations[10].method(realm, receiver, NULL, 0, &other);
		if (status != 0)
			break;

		/* Observe integer addresses only, so explicit collector pressure tests the native invocation's roots. */
		memset(&observer, 0, sizeof(observer));
		is_object = vm_value_is_object(receiver);
		if (!is_object) {
			status = EIO;
			break;
		}

		/* Read only the generated platform wrapper's actual private cell. */
		wrapper = (struct vm_object *)vm_value_as_cell(receiver);
		is_cell = vm_value_is_cell(wrapper->internal);
		if (wrapper->kind != VM_KIND_PLATFORM || !is_cell) {
			status = EIO;
			break;
		}

		/* Observe the receiver state without retaining its wrapper. */
		observer.state = (uintptr_t)vm_value_as_cell(wrapper->internal);
		is_object = vm_value_is_object(other);
		if (!is_object) {
			status = EIO;
			break;
		}

		/* The cloned comparison source must own an actual private cell too. */
		wrapper = (struct vm_object *)vm_value_as_cell(other);
		is_cell = vm_value_is_cell(wrapper->internal);
		if (wrapper->kind != VM_KIND_PLATFORM || !is_cell) {
			status = EIO;
			break;
		}

		/* Observe the verified comparison state without a new trace edge. */
		observer.source_state = (uintptr_t)vm_value_as_cell(wrapper->internal);
		status = bind_abstract_range_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
		if (status != 0)
			break;
		target = bind_node_of(answer);
		if (target == NULL || target->first_child == NULL) {
			status = EIO;
			break;
		}

		/* Save the complete original interval as nonretaining addresses. */
		observer.target = (uintptr_t)target;
		observer.old = (uintptr_t)target->first_child;
		observer.creator = (uintptr_t)&target->document->node;
		status = range_script(realm, "({valueOf:collectOffset})", &comparison_args[0]);
		if (status != 0)
			break;
		observer.argument = (uintptr_t)vm_value_as_cell(comparison_args[0]);
		observer.failure = kind;
		comparison_args[1] = other;

		/* Mode one compares this actual end against the source start after conversion has collected. */
		expected = 0;
		if (kind == 1)
			expected = EIO;

		/* A genuine JavaScript exception preserves its native VM_THROWN outcome. */
		if (kind == 2)
			expected = VM_THROWN;

		/* Publish C observer data only for the direct native invocation. */
		active_observer = &observer;
		status = bind_range_interface.operations[11].method(realm, receiver, comparison_args, 2, &answer);
		if (status != expected) {
			active_observer = NULL;
			break;
		}

		/* Release the synchronous observer only after its expected outcome is checked. */
		active_observer = NULL;
		range_check(status == expected, "exact comparison numeric conversion outcome propagated");
		range_check(observer.conversions == 1U, "comparison mode converts once during actual GC");

		/* Historical START_TO_END mode observes end offset one versus source start offset zero. */
		if (status == 0) {
			expected_order = vm_value_int32(1);
			range_check(answer == expected_order, "native comparison order after collector pressure");
		}

		/* No receiver/source wrapper, C local, conversion value or result remains as a retained VM root. */
		receiver = VM_VALUE_UNDEFINED;
		other = VM_VALUE_UNDEFINED;
		comparison_args[0] = VM_VALUE_UNDEFINED;
		comparison_args[1] = VM_VALUE_UNDEFINED;
		answer = VM_VALUE_UNDEFINED;
		wrapper = NULL;
		target = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.state);
		range_check(found == NULL, "comparison receiver state reclaimed after roots unwind");
		found = vm_heap_find_cell(heap, observer.source_state);
		range_check(found == NULL, "comparison source state reclaimed after roots unwind");
		found = vm_heap_find_cell(heap, observer.creator);
		range_check(found == NULL, "comparison XML creator reclaimed after roots unwind");
		found = vm_heap_find_cell(heap, observer.target);
		range_check(found == NULL, "comparison endpoint graph reclaimed after roots unwind");
		found = vm_heap_find_cell(heap, observer.argument);
		range_check(found == NULL, "comparison numeric argument reclaimed after roots unwind");
		vm_heap_set_stack_base(heap, construction_stack);
		status = 0;
	}

	/* Primary teardown must not encounter a root pointing into an expired native invocation. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (status != 0)
		return status;

	/* Succeeded: successful, host-error and JavaScript-exception conversions obey native root ownership. */
	return 0;
}
