/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies native clone callee roots under actual allocation collection and heap exhaustion. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>

/* This inert cell contributes ordinary allocation pressure and has no trace edges. */
static const struct vm_cell_type pressure_type = { "clone-pressure", NULL, NULL };
/* Assertion totals include independent unlimited and heap-limited invocations. */
static unsigned checks;
/* Failed observations remain visible even when a later case succeeds. */
static unsigned failures;

static void clone_check(int condition, const char *name);
static int clone_case(size_t limit, unsigned children, int succeeds);

/*
 * Runs direct native cloning without a VM receiver frame or conservative stack roots.
 */
int
main(
	void)
{
	int status;
	int printed;

	/* Ordinary production allocation crosses the collector threshold during the copy. */
	status = clone_case(0, 6000, 1);
	if (status != 0)
		return status;

	/* Supported heap limits exercise a genuine partial-allocation failure. */
	status = clone_case(512U * 512U + 64U * 1024U, 512, 0);
	if (status != 0)
		return status;

	/* Report every semantic observation and preserve output failures. */
	printed = printf("native node cloning: %u checks, %u failures\n", checks, failures);
	if (printed < 0)
		return 2;

	/* Failed ownership or output observations reject the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: both native collection and allocation failure were inspected. */
	return 0;
}

/* Reports one ownership or native graph assertion without hiding other failures. */
static void
clone_check(
	int condition,
	const char *name)
{
	int printed;
	const char *outcome;

	/* Each assertion contributes independently to the final fixture outcome. */
	checks++;
	if (!condition)
		failures++;

	/* Print the exact condition name while preserving an output error. */
	outcome = "PASS";
	if (!condition)
		outcome = "FAIL";
	printed = printf("%s %s\n", outcome, name);
	if (printed < 0)
		failures++;

	/* Succeeded: every independent observation remains in the final fixture accounting. */
	return;
}

/* Constructs native input, transfers all ownership to the callee, then inspects cleanup. */
static int
clone_case(
	size_t limit,
	unsigned children,
	int succeeds)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct dom_node *source;
	struct dom_node *copy;
	struct dom_node *child;
	struct dom_node *text;
	struct dom_element *element;
	struct vm_string *name;
	struct vm_cell *construction;
	struct vm_cell *copy_root;
	struct vm_cell *found;
	struct vm_cell *pressure;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	const uint16_t units[2] = { 'X', 0xd800 };
	uintptr_t source_address;
	uintptr_t document_address;
	uintptr_t copy_address;
	unsigned index;
	unsigned copied;
	int construction_registered;
	int copy_registered;
	int intact;
	int status;

	/* Cleanup owns no resources before real production construction begins. */
	heap = NULL;
	realm = NULL;
	construction_registered = 0;
	copy_registered = 0;

	/* A fresh production heap makes the configured limit independent of other cases. */
	status = vm_heap_create(&heap, limit);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		status = 2;
		goto cleanup;
	}

	/* Construction retains a detached graph whose Document does not own the source root. */
	document = dom_document_create(heap);
	if (document == NULL) {
		status = 2;
		goto cleanup;
	}

	/* Retain the empty creator before constructing its detached graph. */
	construction = &document->node.cell;
	status = vm_heap_add_root(heap, &construction);
	if (status != 0) {
		status = 2;
		goto cleanup;
	}

	/* Successful construction registration is the sole owner until the tested callee begins. */
	construction_registered = 1;

	/* The detached source Fragment traces all participants before child allocation. */
	source = dom_fragment_create(document);
	if (source == NULL) {
		status = 2;
		goto cleanup;
	}

	/* The source fragment now carries creator and descendant ownership. */
	construction = &source->cell;
	name = vm_atom_from_ascii(heap, "native-copy");
	if (name == NULL) {
		status = 2;
		goto cleanup;
	}

	/* Wide input bounds fixture time while exercising every new node and character buffer. */
	for (index = 0; index < children; index++) {
		element = dom_element_create(document, DOM_NS_NONE, name, name);
		if (element == NULL) {
			status = 2;
			goto cleanup;
		}

		/* Namespaced attributes and arbitrary URI identity are copied from the actual native model. */
		element->namespace_uri = name;
		status = dom_element_add_attribute(element, DOM_NS_XLINK, name, name, name);
		if (status != 0) {
			status = 2;
			goto cleanup;
		}

		/* Linking first gives the native element a traced owner before its Text allocation. */
		dom_append_child(source, &element->node);
		text = dom_text_create(document, units, 2);
		if (text == NULL) {
			status = 2;
			goto cleanup;
		}

		/* Exact source UTF16 remains independent of the destination buffers. */
		dom_append_child(&element->node, text);
	}

	/* Unsupported kinds and missing output slots cannot publish or mutate native input. */
	copy = source;
	status = bind_clone_node(realm, &document->node, 1, &copy);
	clone_check(status == EINVAL && copy == source, "unsupported native Document kind preserves output");
	status = bind_clone_node(realm, source, 1, NULL);
	clone_check(status == EINVAL, "missing embedding output slot rejected");

	/* A null post-result slot supplies no source ownership during the direct helper call. */
	copy_root = NULL;
	status = vm_heap_add_root(heap, &copy_root);
	if (status != 0) {
		status = 2;
		goto cleanup;
	}

	/* The null result registration carries no input edge until complete publication. */
	copy_registered = 1;

	/* Disable conservative discovery and establish a known ordinary collection threshold. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	source_address = (uintptr_t)source;
	document_address = (uintptr_t)document;

	/* Disposable ordinary VM allocation forces collection partway through unlimited cloning. */
	if (succeeds) {
		pressure = vm_heap_alloc(heap, &pressure_type, 7U * 1024U * 1024U);
		if (pressure == NULL) {
			status = 2;
			goto cleanup;
		}
	}

	/* The callee alone retains input and intermediate copies across every allocation. */
	vm_heap_stats(heap, &before);
	vm_heap_remove_root(heap, &construction);
	construction_registered = 0;
	copy = source;
	status = bind_clone_node(realm, source, 1, &copy);
	if (status != 0 && status != ENOMEM) {
		status = 1;
		goto cleanup;
	}

	/* A missing input cannot be dereferenced after the callee's actual collector has returned. */
	found = vm_heap_find_cell(heap, source_address);
	if (found == NULL) {
		status = 1;
		goto cleanup;
	}

	/* Observe collection only after the actual success or bounded-allocation refusal was classified. */
	vm_heap_stats(heap, &after);
	clone_check(after.collections > before.collections, "clone allocation caused actual collection without stack roots");
	clone_check(source->parent == NULL && source->document == document, "callee retained source identity and current owner");

	/* Inspect all original native buffers and links after collection or partial failure. */
	intact = 1;
	copied = 0;
	for (child = source->first_child;
	     child != NULL;
	     child = child->next) {
		text = child->first_child;
		copied++;

		/* Exact native contents and ownership must survive either outcome. */
		if (child->parent != source ||
		    child->document != document ||
		    text == NULL ||
		    text->type != DOM_TEXT ||
		    text->parent != child)
			intact = 0;

		/* Both code units remain in their original independently owned source buffers. */
		if (text != NULL && text->type == DOM_TEXT) {
			if (((struct dom_character_data *)text)->data.length != 2 ||
			    ((struct dom_character_data *)text)->data.data == NULL ||
			    ((struct dom_character_data *)text)->data.data[0] != 'X' ||
			    ((struct dom_character_data *)text)->data.data[1] != 0xd800)
				intact = 0;
		}
	}

	/* Compare complete source size and contents after the direct invocation. */
	clone_check(intact && copied == children, "source graph and every UTF16 buffer preserved");

	/* Successful publication keeps the complete native copy through an explicit later collection. */
	copy_address = 0;
	if (succeeds) {
		clone_check(
			status == 0 &&
			copy != NULL &&
			copy != source,
			"complete copy published after successful allocation");

		/* A failed helper cannot be inspected as a successfully initialized output graph. */
		if (status != 0 ||
		    copy == NULL ||
		    copy == source) {
			status = 1;
			goto cleanup;
		}

		/* Post-call ownership starts only after the helper has returned. */
		copy_root = &copy->cell;
		copy_address = (uintptr_t)copy;
		intact = 1;
		copied = 0;
		for (child = copy->first_child;
		     child != NULL;
		     child = child->next) {
			text = child->first_child;
			copied++;

			/* Copied children are distinct and retain actual native ownership and UTF16. */
			if (child->parent != copy ||
			    child->document != document ||
			    text == NULL ||
			    text->type != DOM_TEXT ||
			    text->parent != child)
				intact = 0;

			/* Character data must match without any script wrapper allocation. */
			if (text != NULL && text->type == DOM_TEXT) {
				if (((struct dom_character_data *)text)->data.length != 2 ||
				    ((struct dom_character_data *)text)->data.data == NULL ||
				    ((struct dom_character_data *)text)->data.data[1] != 0xd800)
					intact = 0;
			}
		}

		/* Native attribute namespace, prefix, local name and value survive actual collection. */
		child = copy->first_child;
		if (child == NULL || child->type != DOM_ELEMENT) {
			status = 1;
			goto cleanup;
		}

		/* The typed native copy must have actual attribute storage before any attribute dereference. */
		element = (struct dom_element *)child;
		if (element->attribute_count != 1 || element->attributes == NULL) {
			status = 1;
			goto cleanup;
		}

		/* The genuine copied Element exposes its exact original prefix and namespace URI. */
		clone_check(
			element->prefix == name &&
			element->namespace_uri == name,
			"copied element retains native prefix and arbitrary namespace URI");
		clone_check(
			element->attribute_count == 1 &&
			element->attributes[0].ns == DOM_NS_XLINK,
			"copied native attribute retains namespaced identity");
		clone_check(
			element->attributes[0].prefix == name &&
			element->attributes[0].name == name &&
			element->attributes[0].value == name,
			"copied native attribute retains prefix local name and value");

		/* Every expected child must be present after real allocation-triggered collection. */
		clone_check(intact && copied == children, "every copied child survived allocation collection");
	} else {
		/* Heap exhaustion cannot publish an apparently successful truncated graph. */
		clone_check(status == ENOMEM && copy == source, "real ENOMEM preserves unpublished output sentinel");
	}

	/* Released callee roots must not retain the source or unpublished partial result. */
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, source_address);
	clone_check(found == NULL, "source graph collectible after callee roots released");
	found = vm_heap_find_cell(heap, document_address);
	if (succeeds) {
		clone_check(found != NULL, "complete copy retains creator Document");
		found = vm_heap_find_cell(heap, copy_address);
		clone_check(found != NULL, "post-call root retains copied graph");
	} else {
		clone_check(found == NULL, "failed partial graph and Document collectible");
	}

	/* The last explicit output root is the only remaining owner of the detached copy. */
	vm_heap_remove_root(heap, &copy_root);
	copy_registered = 0;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, document_address);
	clone_check(found == NULL, "creator collectible after all output roots released");

	/* A successful native copy must also release its entire cell graph. */
	if (succeeds) {
		found = vm_heap_find_cell(heap, copy_address);
		clone_check(found == NULL, "complete copy collectible after output release");
	}

	/* Invalid helper inputs preserve output without dereferencing native garbage. */
	copy = NULL;
	status = bind_clone_node(NULL, NULL, 1, &copy);
	clone_check(status == EINVAL && copy == NULL, "invalid embedding arguments rejected without publication");

	/* Negative helper observations affect the fixture's counters, not infrastructure cleanup. */
	status = 0;

cleanup:
	/* Only acquired construction/result slots are removed; no roots survive any outcome. */
	if (copy_registered)
		vm_heap_remove_root(heap, &copy_root);

	/* Construction ownership ended before the tested helper when that invocation was reached. */
	if (construction_registered)
		vm_heap_remove_root(heap, &construction);

	/* The fixture's actual primary owner precedes terminal heap finalization. */
	if (realm != NULL)
		vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return status;

	/* Succeeded: all observations remain in the process-wide assertion totals. */
	return 0;
}
