/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies callee ownership through actual split conversion and insertion-host collection. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Integer addresses observe collectible participants without creating a native trace edge. */
struct split_observer {
	uintptr_t original;
	uintptr_t creator;
	uintptr_t argument;
	uintptr_t state;
	uintptr_t parent;
	uintptr_t incoming;
	uintptr_t foreign_creator;
	uintptr_t reference;
	unsigned mode;
	unsigned failure;
	unsigned calls;
};

/* Assertion counts cover every independent success and error invocation. */
static unsigned checks;
/* Failed observations determine the final process status after all attempts run. */
static unsigned failures;
/* One direct native invocation exposes integer-only observations to synchronous callbacks. */
static struct split_observer *active_observer;

static void split_check(int condition, const char *name);
static void split_alive(struct vm_heap *heap, uintptr_t address, int expected, const char *name);
static int split_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int split_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int split_host(void *context, struct dom_node *node);
static int split_cases(struct vm_realm *realm, struct bind_window *window, const void *stack_base);

/*
 * Verifies split and insertion ownership without an outer VM receiver or argument frame.
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

	/* Fixture construction uses the ordinary real-stack collector convention. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Production interfaces provide genuine native input and result wrappers. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* A primary host Document supplies bindings while each tested XML owner remains detached. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* A default host callback collects only during an explicitly active direct invocation. */
	memset(&host, 0, sizeof(host));
	host.context = realm;
	host.node_inserted = split_host;
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Actual ToUint32 invokes this production native function through valueOf. */
	status = js_builtin_method(realm, realm->global, "collectSplit", 0, split_collect);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Invoke real native methods only after callback installation succeeded. */
	status = split_cases(realm, window, __builtin_frame_address(0));
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Destroy the completed embedding after every native invocation was checked. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Independent lifetime observations determine this fixture's acceptance. */
	printed = printf("range insertion lifetime checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any missing root or incorrect callback outcome fails the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: actual conversion and host collection preserved every required native graph. */
	return 0;
}

/* Records one native lifetime or mutation contract. */
static void
split_check(
	int condition,
	const char *name)
{
	int printed;

	/* Keep later independent contracts visible after an earlier failure. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation contributes to final accounting. */
	return;
}

/* Observes a heap cell by integer address with no additional retention. */
static void
split_alive(
	struct vm_heap *heap,
	uintptr_t address,
	int expected,
	const char *name)
{
	struct vm_cell *found;
	int present;

	/* Native graph survival is measured independently of conservative stack roots. */
	found = vm_heap_find_cell(heap, address);
	present = 0;
	if (found != NULL)
		present = 1;
	split_check(present == expected, name);

	/* Succeeded: no observation retained the inspected cell. */
	return;
}

/* Creates fixture inputs through the actual production script engine. */
static int
split_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Prepare the source without alternate DOM allocation or binding paths. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* The real engine supplies native platform objects and relevant prototypes. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release source storage after checking interpreter completion. */
	wb_units_release(&units);

	/* Succeeded: the genuine completion is available. */
	return 0;
}

/* Collects during the public Text method's actual numeric conversion. */
static int
split_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	uint16_t unit;
	int status;

	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Current data changes before validation and suffix copying, without any alternate split path. */
	active_observer->calls++;
	node = (struct dom_node *)active_observer->original;
	unit = 'G';
	status = dom_text_append(node, &unit, 1U);
	if (status != 0)
		return status;

	/* Only the callee's explicit node roots and active conversion machinery retain the input graph. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	split_alive(realm->heap, active_observer->original, 1, "original Text survives actual conversion collection");
	split_alive(realm->heap, active_observer->creator, 1, "current Text creator survives actual conversion collection");
	split_alive(realm->heap, active_observer->argument, 1, "numeric argument survives actual conversion collection");

	/* An ordinary host error is propagated without splitting the original data. */
	if (active_observer->failure == 1U)
		return EIO;

	/* A real VM exception is preserved by the public unsigned conversion. */
	if (active_observer->failure == 2U) {
		status = vm_throw_type_error(realm, "split conversion failure");
		if (status != VM_THROWN)
			return status;

		/* Succeeded: the requested genuine VM exception is installed. */
		return VM_THROWN;
	}

	/* Succeeded: the native helper must use this post-conversion offset and updated data. */
	*result = vm_value_number(2);
	return 0;
}

/* Collects directly from actual insertion hooks without a VM receiver frame retaining participants. */
static int
split_host(
	void *context,
	struct dom_node *node)
{
	struct vm_realm *realm;
	int status;

	/* Setup insertion uses the normal host path without artificial collection. */
	if (active_observer == NULL)
		return 0;

	/* The first delivery is the Text suffix; later delivery inserts the incoming Fragment's child. */
	realm = context;
	active_observer->calls++;
	if (active_observer->calls == 1U)
		active_observer->reference = (uintptr_t)node;

	/* Callee roots and genuine native traces alone retain every participant during host collection. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	split_alive(realm->heap, active_observer->state, 1, "native Range state survives actual insertion host collection");
	split_alive(realm->heap, active_observer->creator, 1, "original Range creator survives insertion host collection");
	split_alive(realm->heap, active_observer->original, 1, "original Text snapshot survives insertion host collection");
	split_alive(realm->heap, active_observer->parent, 1, "actual insertion parent survives host collection");
	split_alive(realm->heap, active_observer->incoming, 1, "incoming Fragment survives host collection even after emptying");
	split_alive(realm->heap, active_observer->reference, 1, "new Text reference survives actual host collection");

	/* Original incoming ownership is retained before adoption and collectible after complete transfer. */
	if (active_observer->calls == 1U) {
		split_alive(realm->heap, active_observer->foreign_creator, 1, "foreign incoming creator survives before checked adoption");
	} else {
		split_alive(realm->heap, active_observer->foreign_creator, 0, "superseded foreign creator is collectible after actual adoption");
	}

	/* Already committed split data and Range repair survive an ordinary host failure. */
	if (active_observer->failure == 1U)
		return EIO;

	/* VM host exceptions retain their exact status and already committed native mutation. */
	if (active_observer->failure == 2U) {
		status = vm_throw_type_error(realm, "split host failure");
		if (status != VM_THROWN)
			return status;

		/* Succeeded: the requested genuine VM exception is installed. */
		return VM_THROWN;
	}

	/* Succeeded: ordinary insertion continues with all current participants still alive. */
	return 0;
}

/* Runs direct registered native methods with integer-only callback observation and bounded GC. */
static int
split_cases(
	struct vm_realm *realm,
	struct bind_window *window,
	const void *stack_base)
{
	struct split_observer observer;
	struct vm_cell *post_roots[2];
	struct vm_object *wrapper;
	struct dom_node *node;
	struct dom_character_data *data;
	vm_value receiver;
	vm_value argument;
	vm_value answer;
	unsigned mode;
	unsigned kind;
	unsigned expected_calls;
	int status;
	int invocation;
	int expected;
	int is_object;
	int is_cell;

	/* Empty post-call slots retain no input during direct native execution. */
	post_roots[0] = NULL;
	post_roots[1] = NULL;
	status = vm_heap_add_root(realm->heap, &post_roots[0]);
	if (status != 0)
		return status;
	status = vm_heap_add_root(realm->heap, &post_roots[1]);
	if (status != 0) {
		vm_heap_remove_root(realm->heap, &post_roots[0]);
		return status;
	}

	/* Each operation is tested through success, ordinary error and actual VM exception. */
	for (mode = 0; mode < 2U; mode++) {
		/* Fresh detached XML owners expose accidental dependence on global or Window Document roots. */
		for (kind = 0; kind < 3U; kind++) {
			/* No observer address is itself a VM retention edge. */
			memset(&observer, 0, sizeof(observer));
			observer.mode = mode;
			observer.failure = kind;
			if (mode == 0) {
				status = split_script(realm,
						      "(function(){var d=document.implementation.createDocument(null,null,null);"
						      "return d.createTextNode('abcdef');})()",
						      &receiver);
				if (status != 0)
					break;
				node = bind_node_of(receiver);
				if (node == NULL || node->type != DOM_TEXT) {
					status = EIO;
					break;
				}

				/* Save nonretaining observations of the checked native input. */
				observer.original = (uintptr_t)node;
				observer.creator = (uintptr_t)node->document;
				status = split_script(realm, "({valueOf:collectSplit})", &argument);
				if (status != 0)
					break;
				observer.argument = (uintptr_t)vm_value_as_cell(argument);
			} else {
				/* Native Range endpoints own an otherwise detached XML parent and creator. */
				status = split_script(realm,
						      "(function(){var d=document.implementation.createDocument(null,null,null);"
						      "var p=d.createElement('parent'),t=d.createTextNode('12345');p.appendChild(t);"
						      "var r=d.createRange();r.setStart(t,2);r.setEnd(t,3);return r;})()",
						      &receiver);
				if (status != 0)
					break;
				is_object = vm_value_is_object(receiver);
				if (!is_object) {
					status = EIO;
					break;
				}

				/* Inspect only the generated platform wrapper's actual private state. */
				wrapper = (struct vm_object *)vm_value_as_cell(receiver);
				is_cell = vm_value_is_cell(wrapper->internal);
				if (wrapper->kind != VM_KIND_PLATFORM || !is_cell) {
					status = EIO;
					break;
				}

				/* Record the checked state before reading its genuine native endpoint. */
				observer.state = (uintptr_t)vm_value_as_cell(wrapper->internal);
				status = bind_abstract_range_interface.attributes[0].getter(realm, receiver, NULL, 0, &answer);
				if (status != 0)
					break;
				node = bind_node_of(answer);
				if (node == NULL || node->type != DOM_TEXT) {
					status = EIO;
					break;
				}

				/* Save nonretaining observations of the checked native input. */
				observer.original = (uintptr_t)node;
				observer.creator = (uintptr_t)node->document;
				observer.parent = (uintptr_t)node->parent;
				status = split_script(realm,
						      "(function(){var d=document.implementation.createDocument(null,null,null);"
						      "var f=d.createDocumentFragment();f.appendChild(d.createTextNode('ABCDE'));return f;})()",
						      &argument);
				if (status != 0)
					break;
				node = bind_node_of(argument);
				if (node == NULL || node->type != DOM_DOCUMENT_FRAGMENT) {
					status = EIO;
					break;
				}

				/* Save nonretaining observations of the checked native input. */
				observer.incoming = (uintptr_t)node;
				observer.foreign_creator = (uintptr_t)node->document;
			}

			/* Compute each expected ordinary or throwing outcome before the native call. */
			expected = 0;
			if (kind == 1U)
				expected = EIO;
			if (kind == 2U)
				expected = VM_THROWN;

			/* Direct registration-table invocation supplies no outer VM receiver or Node argument frame. */
			active_observer = &observer;
			if (mode == 0) {
				invocation = bind_text_interface.operations[0].method(realm, receiver, &argument, 1, &answer);
				if (invocation != expected) {
					active_observer = NULL;
					status = EIO;
					break;
				}
			} else {
				invocation = bind_range_interface.operations[13].method(realm, receiver, &argument, 1, &answer);
				if (invocation != expected) {
					active_observer = NULL;
					status = EIO;
					break;
				}
			}

			/* Callee completion ends synchronous callback observation before independent inspection. */
			active_observer = NULL;
			split_check(invocation == expected, "exact direct native success or callback error status");
			expected_calls = 1;
			if (mode == 1U && kind == 0U)
				expected_calls = 2;
			split_check(observer.calls == expected_calls, "exact conversion or split-plus-incoming host delivery count");

			/* Post-call inspection acquires its own roots only after callee root cleanup has finished. */
			if (mode == 0) {
				if (kind == 0U) {
					is_object = vm_value_is_object(answer);
					if (!is_object) {
						status = EIO;
						break;
					}

					/* Root the verified suffix result only after direct native return. */
					post_roots[0] = vm_value_as_cell(answer);
					node = bind_node_of(answer);
					if (node == NULL || node->type != DOM_TEXT) {
						status = EIO;
						break;
					}

					/* Read only checked native CharacterData from the suffix result. */
					data = (struct dom_character_data *)node;
					split_check(data->data.length == 5U && data->data.data[4] == 'G', "suffix copies actual post-conversion data");
					observer.reference = (uintptr_t)node;
				}
			} else {
				/* Even a rejected host callback leaves a completely committed and inspectable native split. */
				post_roots[0] = vm_heap_find_cell(realm->heap, observer.state);
				if (post_roots[0] == NULL) {
					status = EIO;
					break;
				}

				/* The incoming Fragment must remain real before its inspection root is published. */
				post_roots[1] = vm_heap_find_cell(realm->heap, observer.incoming);
				if (post_roots[1] == NULL) {
					status = EIO;
					break;
				}

				/* Allocate inspection storage only after both actual participants are rooted. */
				wrapper = vm_object_create(realm->heap, window->prototypes[BIND_RANGE]);
				if (wrapper == NULL) {
					status = ENOMEM;
					break;
				}

				/* Republish only the already rooted genuine state for primitive native getter inspection. */
				wrapper->kind = VM_KIND_PLATFORM;
				wrapper->internal = vm_value_cell(post_roots[0]);
				receiver = vm_value_cell(wrapper);
				status = bind_abstract_range_interface.attributes[2].getter(realm, receiver, NULL, 0, &answer);
				if (status != 0)
					break;
				split_check(answer == vm_value_number(2), "committed split preserves original native start offset");
				status = bind_abstract_range_interface.attributes[3].getter(realm, receiver, NULL, 0, &answer);
				if (status != 0)
					break;
				split_check(answer == vm_value_number(1), "committed split transfers original native end offset");
				data = (struct dom_character_data *)observer.original;
				split_check(data->data.length == 2U, "complete original prefix remains after host success or failure");
			}

			/* A successful detached suffix owns its creator but never keeps the original Text or numeric object. */
			vm_heap_collect(realm->heap);
			if (mode == 0) {
				split_alive(realm->heap, observer.original, 0, "original detached Text collectible after conversion roots unwind");
				split_alive(realm->heap, observer.argument, 0, "numeric argument collectible after conversion roots unwind");
				expected = 0;
				if (kind == 0U)
					expected = 1;
				split_alive(realm->heap, observer.creator, expected, "detached suffix alone determines current creator retention");
			}

			/* Optional weak tokens and host integer observations cannot retain any released operation graph. */
			post_roots[0] = NULL;
			post_roots[1] = NULL;
			vm_heap_collect(realm->heap);
			split_alive(realm->heap, observer.creator, 0, "creator collectible after final operation roots unwind");
			split_alive(realm->heap, observer.original, 0, "original Text collectible after all operation roots unwind");
			if (mode == 1U) {
				split_alive(realm->heap, observer.state, 0, "native Range state collectible after explicit inspection root release");
				split_alive(realm->heap, observer.incoming, 0, "incoming Fragment collectible after operation root release");
				split_alive(realm->heap, observer.reference, 0, "new Text reference collectible after operation root release");
				split_alive(realm->heap, observer.foreign_creator, 0, "foreign creator collectible after all operation roots unwind");
			}

			/* Restore ordinary construction before the next independent direct native invocation. */
			vm_heap_set_stack_base(realm->heap, stack_base);
		}

		/* An embedding or fixture setup failure ends this bounded attempt. */
		if (status != 0)
			break;
	}

	/* Registered slots cannot outlive this returned C stack even after an embedding failure. */
	vm_heap_set_stack_base(realm->heap, stack_base);
	vm_heap_remove_root(realm->heap, &post_roots[1]);
	vm_heap_remove_root(realm->heap, &post_roots[0]);
	if (status != 0)
		return status;

	/* Succeeded: every bounded success/error case retained and then released its actual native graph. */
	return 0;
}
