/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies real native expanded attribute identity, clone preservation and precise collector ownership. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>

/* Independent semantic and lifetime observations contribute to one native fixture result. */
static unsigned checks;
/* Preserve every failed observation until all native resources have been released. */
static unsigned failures;

static void attribute_check(int condition, const char *name);
static int attribute_case(void);
static int attribute_run(struct vm_heap *heap, struct vm_realm *realm, struct vm_cell **source_root, struct vm_cell **copy_root);

/*
 * Runs native namespace identity checks without a synthetic script-side attribute map.
 */
int
main(
	void)
{
	int status;
	int printed;

	/* The actual DOM attribute array and collector supply every observation. */
	status = attribute_case();
	if (status != 0)
		return 2;

	/* Reports the independently counted observations. */
	printed = printf("native attribute namespaces: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Rejects any recorded semantic failure. */
	if (failures != 0)
		return 1;

	/* Succeeded: expanded names and their owner graphs survived actual lifetime transitions. */
	return 0;
}

/* Retains one independent expanded-name or collector observation. */
static void
attribute_check(
	int condition,
	const char *name)
{
	int printed;

	/* Continue independent observations while preserving all failures. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation contributes to the final result. */
	return;
}

/* Owns the actual heap and nullable root slots across every success or early failure. */
static int
attribute_case(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct vm_cell *source_root;
	struct vm_cell *copy_root;
	int status;

	/* Construction uses the real stack until the tested precise-root lifetime transitions. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return status;

	/* Protects initial construction through the actual caller stack. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return status;
	}

	/* Nullable registrations exist before any actual test graph is constructed. */
	source_root = NULL;
	copy_root = NULL;
	status = vm_heap_add_root(heap, &source_root);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return status;
	}

	/* A failed second registration releases the first before returning to its owner. */
	status = vm_heap_add_root(heap, &copy_root);
	if (status != 0) {
		vm_heap_remove_root(heap, &source_root);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return status;
	}

	/* Every early test return still passes through complete owner cleanup here. */
	status = attribute_run(heap, realm, &source_root, &copy_root);
	if (status != 0) {
		vm_heap_remove_root(heap, &copy_root);
		vm_heap_remove_root(heap, &source_root);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return status;
	}

	/* Unregisters live stack slots before releasing their heap. */
	vm_heap_remove_root(heap, &copy_root);
	vm_heap_remove_root(heap, &source_root);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Succeeded: no stack slot or native heap ownership outlives this invocation. */
	return 0;
}

/* Exercises exact native URI identity, ordered array moves and cloned ownership in one real heap. */
static int
attribute_run(
	struct vm_heap *heap,
	struct vm_realm *realm,
	struct vm_cell **source_root,
	struct vm_cell **copy_root)
{
	struct dom_document *document;
	struct dom_element *element;
	struct dom_element *copy_element;
	struct dom_node *copy;
	struct dom_attribute *attribute;
	struct vm_string *name;
	struct vm_string *prefix;
	struct vm_string *alias;
	struct vm_string *uri_a;
	struct vm_string *uri_b;
	struct vm_string *uri_equal;
	struct vm_string *name_equal;
	struct vm_string *empty;
	struct vm_string *xml;
	struct vm_string *value_a;
	struct vm_string *value_b;
	struct vm_cell *found;
	uintptr_t source_address;
	uintptr_t uri_address;
	uintptr_t document_address;
	size_t count;
	uint64_t generation;
	char buffer[32];
	struct vm_string *extra;
	int printed;
	unsigned index;
	int same;
	int status;

	/* Native test graph construction precedes precise-root collection. */
	document = dom_document_create(heap);
	if (document == NULL)
		return ENOMEM;

	/* Interns the local name shared by distinct expanded names. */
	name = vm_atom_from_ascii(heap, "a");
	if (name == NULL)
		return ENOMEM;

	/* Interns the first namespace prefix. */
	prefix = vm_atom_from_ascii(heap, "p");
	if (prefix == NULL)
		return ENOMEM;

	/* Interns an alternate prefix for the same expanded name. */
	alias = vm_atom_from_ascii(heap, "q");
	if (alias == NULL)
		return ENOMEM;

	/* Allocates the first independently traced custom URI. */
	uri_a = vm_string_from_utf8(heap, "urn:custom:a", 12);
	if (uri_a == NULL)
		return ENOMEM;

	/* Allocates a distinct custom URI with the same local name. */
	uri_b = vm_string_from_utf8(heap, "urn:custom:b", 12);
	if (uri_b == NULL)
		return ENOMEM;

	/* Allocates equal URI text without relying on pointer identity. */
	uri_equal = vm_string_from_utf8(heap, "urn:custom:a", 12);
	if (uri_equal == NULL)
		return ENOMEM;

	/* Allocates equal local-name text outside the atom registry. */
	name_equal = vm_string_from_utf8(heap, "a", 1);
	if (name_equal == NULL)
		return ENOMEM;

	/* Represents an empty namespace URI explicitly. */
	empty = vm_string_from_utf8(heap, "", 0);
	if (empty == NULL)
		return ENOMEM;

	/* Represents the canonical XML namespace by its full URI. */
	xml = vm_string_from_utf8(heap, "http://www.w3.org/XML/1998/namespace", 36);
	if (xml == NULL)
		return ENOMEM;

	/* Allocates the value used to verify first-record identity. */
	value_a = vm_string_from_utf8(heap, "one", 3);
	if (value_a == NULL)
		return ENOMEM;

	/* Allocates the independent second-record value. */
	value_b = vm_string_from_utf8(heap, "two", 3);
	if (value_b == NULL)
		return ENOMEM;

	/* The detached source has no Document child edge that could substitute for its own root. */
	element = dom_element_create(document, DOM_NS_NONE, name, NULL);
	if (element == NULL)
		return ENOMEM;

	/* All URI helpers work directly on already validated strings without a VM allocation. */
	status = dom_element_add_attribute_uri(element, uri_a, prefix, name, value_a);
	if (status != 0)
		return status;

	/* Adds the independently named attribute through its native namespace path. */
	status = dom_element_add_attribute_uri(element, uri_b, prefix, name, value_b);
	if (status != 0)
		return status;
	attribute_check(element->attribute_count == 2, "distinct custom namespaces coexist with the same local name");
	attribute = dom_element_find_attribute_uri(element, uri_equal, name_equal);
	attribute_check(attribute != NULL, "independent equal URI and local-name strings identify the actual record");
	if (attribute != NULL) {
		attribute_check(attribute->value == value_a, "exact URI selects the first value");
		attribute_check(attribute->ns == DOM_NS_OTHER && attribute->namespace_uri == uri_a, "custom class retains exact native URI edge");
	}

	/* Enum classification alone must never pick an arbitrary custom namespace. */
	attribute = dom_element_find_attribute_uri(element, uri_b, name);
	attribute_check(attribute != NULL && attribute->value == value_b, "second custom URI selects the second value");
	attribute = dom_element_find_attribute(element, DOM_NS_OTHER, name);
	attribute_check(attribute == NULL, "enum-only custom lookup refuses ambiguous URI identity");
	count = element->attribute_count;
	generation = document->generation;
	status = dom_element_add_attribute_uri(element, uri_equal, alias, name_equal, value_b);
	attribute_check(status == EEXIST, "prefix alias cannot duplicate an expanded attribute name");
	attribute_check(element->attribute_count == count && document->generation == generation, "duplicate rejection leaves count and generation unchanged");
	attribute = dom_element_find_attribute_uri(element, uri_a, name);
	attribute_check(attribute != NULL && attribute->value == value_a, "duplicate rejection preserves the existing value");

	/* Absent and canonical XML namespaces remain distinct from custom namespaces with the same local name. */
	status = dom_element_add_attribute_uri(element, NULL, NULL, name, value_a);
	if (status != 0)
		return status;

	/* Adds the independently named attribute through its native namespace path. */
	status = dom_element_add_attribute(element, DOM_NS_XML, prefix, name, value_b);
	if (status != 0)
		return status;
	attribute = dom_element_find_attribute_uri(element, empty, name);
	attribute_check(attribute != NULL && attribute->ns == DOM_NS_NONE, "empty URI and absent namespace are identical");
	attribute = dom_element_find_attribute_uri(element, xml, name_equal);
	attribute_check(attribute != NULL && attribute->value == value_b, "canonical URI finds an implicit legacy built-in attribute");
	status = dom_element_add_attribute_uri(element, xml, alias, name, value_a);
	attribute_check(status == EEXIST, "explicit URI cannot duplicate a legacy built-in expanded name");

	/* Ordinary array growth preserves exact URI edges in earlier records. */
	for (index = 0; index < 16; index++) {
		printed = snprintf(buffer, sizeof(buffer), "extra%u", index);
		if (printed < 0)
			return EIO;

		/* Interns the next bounded attribute name. */
		extra = vm_atom_from_ascii(heap, buffer);
		if (extra == NULL)
			return ENOMEM;

		/* Grows the actual native attribute array. */
		status = dom_element_add_attribute(element, DOM_NS_NONE, NULL, extra, value_b);
		if (status != 0)
			return status;
	}

	/* Removal shifts complete attribute records instead of losing their URI identity. */
	attribute = dom_element_find_attribute_uri(element, uri_a, name);
	attribute_check(attribute != NULL && attribute->namespace_uri == uri_a, "attribute array growth preserves exact URI");
	status = dom_element_remove_attribute(element, name);
	if (status != 0)
		return status;
	attribute = dom_element_find_attribute_uri(element, xml, name);
	attribute_check(attribute != NULL && attribute->ns == DOM_NS_XML, "ordinary removal preserves shifted built-in identity");
	attribute = dom_element_find_attribute_uri(element, uri_b, name);
	attribute_check(attribute != NULL && attribute->namespace_uri == uri_b, "ordinary removal preserves custom namespace identity");

	/* Source alone must retain both non-atom URI strings and its actual Document through collection. */
	*source_root = &element->node.cell;
	source_address = (uintptr_t)element;
	uri_address = (uintptr_t)uri_a;
	document_address = (uintptr_t)document;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, uri_address);
	attribute_check(found == &uri_a->cell, "source-only collection retains non-atom attribute URI");
	found = vm_heap_find_cell(heap, document_address);
	attribute_check(found == &document->node.cell, "source-only collection retains actual owner Document");

	/* Native cloning copies exact URI edges while the source and pending destination are rooted by its callee. */
	status = bind_clone_node(realm, &element->node, 0, &copy);
	if (status != 0)
		return status;

	/* Retains the independently cloned native graph. */
	*copy_root = &copy->cell;
	copy_element = (struct dom_element *)copy;
	attribute = dom_element_find_attribute_uri(copy_element, uri_a, name);
	attribute_check(attribute != NULL && attribute->namespace_uri == uri_a, "native clone preserves exact custom URI identity");
	attribute_check(copy_element->attribute_count == element->attribute_count, "native clone preserves the complete ordered attribute array");
	attribute_check(copy->parent == NULL && copy->document == document, "clone has independent detached identity and actual owner");

	/* A clone-only root must retain the shared URI while the original detached element becomes unreachable. */
	*source_root = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, source_address);
	attribute_check(found == NULL, "clone-only collection releases the original detached source");
	found = vm_heap_find_cell(heap, uri_address);
	attribute_check(found != NULL, "clone-only collection retains the copied non-atom URI");
	if (found == NULL)
		return EINVAL;

	/* Queries the copied record only through a verified surviving URI cell. */
	attribute = dom_element_find_attribute_uri(copy_element, (struct vm_string *)found, name);
	same = attribute != NULL;
	if (same)
		same = vm_string_equal_ascii(attribute->value, "one");
	attribute_check(same, "clone-only surviving URI still selects the original value");

	/* No global or permanent root may keep dead namespace strings or their owner graph alive. */
	*copy_root = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, uri_address);
	attribute_check(found == NULL, "last-root release collects the non-atom attribute URI");
	found = vm_heap_find_cell(heap, document_address);
	attribute_check(found == NULL, "last-root release collects the owner Document");

	/* Succeeded: the owner wrapper releases registered slots and native heap resources. */
	return 0;
}
