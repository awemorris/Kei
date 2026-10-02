/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies genuine Range content cloning through allocation-triggered GC without caller frames. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Ordinary inert allocation supplies collector pressure without any participant trace edge. */
static const struct vm_cell_type pressure_type = { "range-clone-pressure", NULL, NULL };
/* Each independent graph, interval and lifetime observation contributes to the outcome. */
static unsigned checks;
/* Failed ownership observations survive until the final process exit. */
static unsigned failures;

static void contents_check(int condition, const char *name);
static int contents_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int contents_case(struct vm_realm *realm, struct bind_window *window);

/*
 * Runs a genuine registered content operation without outer receiver or conservative stack roots.
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

	/* Ordinary fixture construction uses the production collector's real C stack convention. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Production builtins and a primary Window provide real native factory and prototype snapshots. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The primary Document supplies genuine binding installation ownership. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* No host mutation callback supplies an alternate cloning or rooting path. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The native case temporarily disables stack scanning only around the actual operation. */
	status = contents_case(realm, window);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Print all observations after the production embedding has released its participants. */
	printed = printf("native Range clone contents: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any incomplete graph, changed interval or retained released root rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: actual mid-clone collection and complete result lifetime are verified. */
	return 0;
}

/* Records one graph or lifetime observation while preserving later independent checks. */
static void
contents_check(
	int condition,
	const char *name)
{
	int printed;

	/* A failed observation remains part of the final process-wide failure count. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Constructs genuine native factory outputs through the ordinary production interpreter. */
static int
contents_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* The fixture source uses the same character conversion and parser as ordinary script execution. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* No alternate constructor or test-only brand supplies the XML Document. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the ordinary native factory completion is available to the embedding. */
	return 0;
}

/* Transfers construction ownership to the native callee, then observes complete graph reclamation. */
static int
contents_case(
	struct vm_realm *realm,
	struct bind_window *window)
{
	struct dom_document *document;
	struct dom_node *source;
	struct dom_node *copy;
	struct dom_node *child;
	struct dom_node *text;
	struct dom_element *element;
	struct vm_string *name;
	struct vm_cell *roots[3];
	struct vm_cell *state;
	struct vm_cell *found;
	struct vm_cell *pressure;
	struct vm_object *wrapper;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value creator;
	vm_value source_value;
	vm_value receiver;
	vm_value answer;
	vm_value arguments[2];
	const uint16_t units[2] = { 'X', 0xd800 };
	uintptr_t source_address;
	uintptr_t state_address;
	uintptr_t creator_address;
	uintptr_t copy_address;
	unsigned index;
	unsigned registered;
	unsigned copied;
	int intact;
	int status;

	/* Null result slots hold no participant before explicit construction ownership is installed. */
	roots[0] = NULL;
	roots[1] = NULL;
	roots[2] = NULL;
	registered = 0;
	for (index = 0; index < 3U; index++) {
		status = vm_heap_add_root(realm->heap, &roots[index]);
		if (status != 0)
			break;
		registered++;
	}

	/* A failed root registration must release only slots that actually registered. */
	if (status != 0) {
		while (registered != 0) {
			registered--;
			vm_heap_remove_root(realm->heap, &roots[registered]);
		}

		/* Report construction failure before any native graph was allocated. */
		return status;
	}

	/* All normal and failed construction paths share explicit root cleanup after this bounded attempt. */
	do {
		status = contents_script(realm, "document.implementation.createDocument(null,null,null)", &creator);
		if (status != 0)
			break;
		document = (struct dom_document *)bind_node_of(creator);
		roots[0] = &document->node.cell;
		source = dom_fragment_create(document);
		if (source == NULL) {
			status = ENOMEM;
			break;
		}

		/* The detached native graph traces its current creator before child factories run. */
		roots[0] = &source->cell;
		name = vm_atom_from_ascii(realm->heap, "content-node");
		if (name == NULL) {
			status = ENOMEM;
			break;
		}

		/* A bounded wide graph crosses the ordinary collector threshold while preserving fixture time. */
		for (index = 0; index < 6000U; index++) {
			element = dom_element_create(document, DOM_NS_NONE, name, NULL);
			if (element == NULL) {
				status = ENOMEM;
				break;
			}

			/* Native parent ownership protects the Element during its following Text allocation. */
			dom_append_child(source, &element->node);
			text = dom_text_create(document, units, 2);
			if (text == NULL) {
				status = ENOMEM;
				break;
			}

			/* Every copy later needs an independent exact character buffer and corresponding native link. */
			dom_append_child(&element->node, text);
		}

		/* A partial source construction cannot exercise the intended complete interval. */
		if (status != 0)
			break;

		/* Production factories and registered selection methods supply genuine private Range state. */
		status = bind_wrap(window, source, &source_value);
		if (status != 0)
			break;
		status = bind_create_range(realm, creator, NULL, 0, &receiver);
		if (status != 0)
			break;
		roots[1] = vm_value_as_cell(receiver);
		status = bind_range_interface.operations[7].method(realm, receiver, &source_value, 1, &answer);
		if (status != 0)
			break;
		/* Partial first and last Text boundaries require actual shell cloning around contained middle siblings. */
		status = bind_wrap(window, source->first_child->first_child, &arguments[0]);
		if (status != 0)
			break;
		arguments[1] = vm_value_number(1);
		status = bind_range_interface.operations[0].method(realm, receiver, arguments, 2, &answer);
		if (status != 0)
			break;
		status = bind_wrap(window, source->last_child->first_child, &arguments[0]);
		if (status != 0)
			break;
		status = bind_range_interface.operations[1].method(realm, receiver, arguments, 2, &answer);
		if (status != 0)
			break;

		/* Record genuine private state and integer-only lifetime observations before releasing construction roots. */
		wrapper = (struct vm_object *)vm_value_as_cell(receiver);
		state = vm_value_as_cell(wrapper->internal);
		source_address = (uintptr_t)source;
		state_address = (uintptr_t)state;
		creator_address = (uintptr_t)document;

		/* Only explicit construction slots participate in this known-threshold collection. */
		vm_heap_set_stack_base(realm->heap, NULL);
		vm_heap_collect(realm->heap);
		pressure = vm_heap_alloc(realm->heap, &pressure_type, 7U * 1024U * 1024U);
		if (pressure == NULL) {
			status = ENOMEM;
			break;
		}

		/* The native method alone holds genuine state and source across allocation-triggered collection. */
		vm_heap_stats(realm->heap, &before);
		roots[0] = NULL;
		roots[1] = NULL;
		status = bind_range_interface.operations[14].method(realm, receiver, NULL, 0, &answer);
		vm_heap_stats(realm->heap, &after);
		if (status != 0)
			break;

		/* Post-call ownership begins only after the registered operation has returned its complete result. */
		roots[1] = state;
		roots[2] = vm_value_as_cell(answer);
		copy = bind_node_of(answer);
		copy_address = (uintptr_t)copy;
		contents_check(after.collections > before.collections, "registered cloneContents allocation caused real GC without caller roots");
		contents_check(copy != NULL && copy != source && copy->type == DOM_DOCUMENT_FRAGMENT,
			"complete genuine native Fragment result published");
		contents_check(source->document == document && copy->document == document && source->parent == NULL,
			"source and clone retain exact actual owner and detached source identity");

		/* Observe every copied native link and unpaired code unit after mid-clone collection. */
		copied = 0;
		intact = 1;
		for (child = copy->first_child; child != NULL; child = child->next) {
			text = child->first_child;
			copied++;

			/* Copied links, names and buffer contents come from native fields alone. */
			if (child->parent != copy ||
			    child->document != document ||
			    ((struct dom_element *)child)->local_name != name ||
			    text == NULL ||
			    text->parent != child)
				intact = 0;

			/* First and last partial slices retain exact native character identity after collection. */
			if (text != NULL) {
				if (copied == 1U) {
					/* The first partially contained Element contains only the selected high surrogate. */
					if (((struct dom_character_data *)text)->data.length != 1 ||
					    ((struct dom_character_data *)text)->data.data[0] != 0xd800)
						intact = 0;
				} else if (copied == 6000U) {
					/* The final partially contained Element contains only its selected prefix. */
					if (((struct dom_character_data *)text)->data.length != 1 ||
					    ((struct dom_character_data *)text)->data.data[0] != 'X')
						intact = 0;
				} else {
					/* Fully contained middle siblings preserve their complete independent native buffers. */
					if (((struct dom_character_data *)text)->data.length != 2 ||
					    ((struct dom_character_data *)text)->data.data[0] != 'X' ||
					    ((struct dom_character_data *)text)->data.data[1] != 0xd800)
						intact = 0;
				}
			}
		}

		/* Every expected descendant must be present rather than a silently truncated successful result. */
		contents_check(intact && copied == 6000U, "all6000 native partial shells and contained copies survived actual collection");
		contents_check(source->first_child != copy->first_child && source->last_child != copy->last_child,
			"native content cloning does not move or alias original node identities");

		/* Every original character buffer and native link remains complete after cloning collection. */
		copied = 0;
		intact = 1;
		for (child = source->first_child; child != NULL; child = child->next) {
			text = child->first_child;
			copied++;

			/* Source nodes retain their original owner, parent and exact independent buffer. */
			if (child->parent != source || child->document != document || text == NULL)
				intact = 0;

			/* Native source units stay unchanged instead of being sliced or moved into the output. */
			if (text != NULL) {
				if (((struct dom_character_data *)text)->data.length != 2 ||
				    ((struct dom_character_data *)text)->data.data[0] != 'X' ||
				    ((struct dom_character_data *)text)->data.data[1] != 0xd800)
					intact = 0;
			}
		}

		/* Complete source size and data are independent of the copied graph's observations. */
		contents_check(intact && copied == 6000U, "all6000 original Element and Text pairs remain unchanged after cloning GC");

		/* Reconstruct only a temporary receiver around the surviving genuine private cell for getter inspection. */
		wrapper = vm_object_create(realm->heap, NULL);
		if (wrapper == NULL) {
			status = ENOMEM;
			break;
		}

		/* Private state branding remains real; no JS-visible prototype or property can synthesize this cell. */
		wrapper->kind = VM_KIND_PLATFORM;
		wrapper->internal = vm_value_cell(state);
		roots[0] = &wrapper->cell;
		receiver = vm_value_cell(wrapper);
		status = bind_abstract_range_interface.attributes[2].getter(realm, receiver, NULL, 0, &answer);
		if (status != 0)
			break;
		contents_check(answer == vm_value_number(1), "source private start offset unchanged after actual cloning GC");
		status = bind_abstract_range_interface.attributes[3].getter(realm, receiver, NULL, 0, &answer);
		if (status != 0)
			break;
		contents_check(answer == vm_value_number(1), "source private end offset unchanged after actual cloning GC");

		/* Only the complete result owns its actual creator after source and genuine state release. */
		roots[0] = NULL;
		roots[1] = NULL;
		vm_heap_collect(realm->heap);
		found = vm_heap_find_cell(realm->heap, source_address);
		contents_check(found == NULL, "callee source graph collectible after explicit post-call inspection roots release");
		found = vm_heap_find_cell(realm->heap, state_address);
		contents_check(found == NULL, "genuine native Range state collectible after callee and inspection roots release");
		found = vm_heap_find_cell(realm->heap, creator_address);
		contents_check(found != NULL, "complete result alone retains actual creator Document");
		found = vm_heap_find_cell(realm->heap, copy_address);
		contents_check(found != NULL, "explicit wrapped result retains complete native content graph");

		/* Release the final result root before testing independent graph and owner reclamation. */
		roots[2] = NULL;
		vm_heap_collect(realm->heap);
		found = vm_heap_find_cell(realm->heap, copy_address);
		contents_check(found == NULL, "complete native clone graph collectible after result release");
		found = vm_heap_find_cell(realm->heap, creator_address);
		contents_check(found == NULL, "actual creator Document collectible after result release");

		/* Wrong receivers cannot publish a partial result through the registered native operation. */
		answer = VM_VALUE_NULL;
		status = bind_range_interface.operations[14].method(realm, VM_VALUE_UNDEFINED, NULL, 0, &answer);
		contents_check(status == VM_THROWN && answer == VM_VALUE_NULL, "direct wrong-brand call rejects before result publication");
		status = 0;
	} while (0);

	/* Every temporary root registered by fixture construction is removed even after a failed attempt. */
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(realm->heap, &roots[registered]);
	}

	/* Report genuine embedding failures separately from already counted semantic observations. */
	if (status != 0)
		return status;

	/* Succeeded: collection, exact native state and independent result lifetime were inspected. */
	return 0;
}
