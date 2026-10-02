/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies actual native XML character-node identity, copied data, roots and clone lifetime. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>

/* Disposable allocation reaches the unchanged production collector threshold. */
static const struct vm_cell_type pressure_type = { "xml-node-pressure", NULL, NULL };
/* Count independent native identity, data and collector observations. */
static unsigned checks;
/* Preserve all failures while ordinary owned native resources are released. */
static unsigned failures;

static void xml_node_check(int condition, const char *name);
static int xml_node_case(void);
static int xml_node_run(struct vm_heap *heap, struct vm_realm *realm, struct vm_cell **roots);

/*
 * Exercises native PI and CDATA nodes without public XML loading or synthetic wrappers.
 */
int
main(
	void)
{
	int status;
	int printed;

	/* The real node factories, buffer operations and collector supply every observation. */
	status = xml_node_case();
	if (status != 0)
		return 2;
	printed = printf("native XML nodes: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: exact native kinds and their owner graphs survived actual collection. */
	return 0;
}

/* Records one real native semantic or ownership observation. */
static void
xml_node_check(
	int condition,
	const char *name)
{
	int printed;

	/* Preserve failed observations while later independent native checks continue. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Owns all nullable root slots and heap resources across every early native return. */
static int
xml_node_case(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct vm_cell *roots[4];
	unsigned index;
	unsigned registered;
	int status;

	/* Only initial construction may use the real stack; tested lifetime uses explicit roots. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return status;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return status;
	}

	/* The owning wrapper keeps every successful registration removable on failure. */
	registered = 0;
	for (index = 0; index < 4U; index++) {
		roots[index] = NULL;
		status = vm_heap_add_root(heap, &roots[index]);
		if (status != 0)
			break;
		registered++;
	}

	/* Independent native data and collector checks always return through complete owner cleanup. */
	if (status == 0)
		status = xml_node_run(heap, realm, roots);
	for (index = 0; index < registered; index++)
		vm_heap_remove_root(heap, &roots[index]);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Succeeded or failed: no registered stack slot or native heap escapes. */
	return status;
}

/* Observes native allocation collection and exact character graphs without conservative caller roots. */
static int
xml_node_run(
	struct vm_heap *heap,
	struct vm_realm *realm,
	struct vm_cell **roots)
{
	struct dom_document *document;
	struct dom_node *pi;
	struct dom_node *cdata;
	struct dom_node *fragment;
	struct dom_node *copy_pi;
	struct dom_node *copy_cdata;
	struct dom_node *rejected;
	struct dom_character_data *data;
	struct vm_string *target;
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	uint16_t units[3];
	uint16_t replacement;
	uintptr_t pi_address;
	uintptr_t cdata_address;
	uintptr_t document_address;
	uintptr_t target_address;
	uint64_t generation;
	int status;
	int classified;

	/* The PI target is a non-atom VM string, so only genuine node tracing can retain it later. */
	document = dom_document_create(heap);
	target = vm_string_from_utf8(heap, "processing-target", 17);
	if (document == NULL || target == NULL)
		return ENOMEM;
	units[0] = 'A';
	units[1] = 0xd800U;
	units[2] = 'Z';
	document_address = (uintptr_t)document;
	target_address = (uintptr_t)target;

	/* Caller Document and target have no roots or conservative stack when the callee allocation collects. */
	vm_heap_set_stack_base(heap, NULL);
	pressure = vm_heap_alloc(heap, &pressure_type, 8U * 1024U * 1024U);
	if (pressure == NULL)
		return ENOMEM;
	vm_heap_stats(heap, &before);
	status = dom_pi_create(document, target, units, 3, &pi);
	vm_heap_stats(heap, &after);
	if (status != 0)
		return status;
	roots[0] = &pi->cell;
	pi_address = (uintptr_t)pi;
	xml_node_check(after.collections > before.collections, "PI callee allocation caused actual threshold collection");
	xml_node_check(pi->type == DOM_PROCESSING_INSTRUCTION && pi->document == document, "PI has native type seven and actual owner");
	data = (struct dom_character_data *)pi;
	xml_node_check(data->target == target, "PI retains actual non-atom target identity");
	classified = dom_is_character_data(pi);
	xml_node_check(classified, "PI is genuine CharacterData");
	classified = dom_is_text(pi);
	xml_node_check(!classified, "PI is not Text");

	/* Native CDATA owns a distinct buffer and does not share the caller's mutable C data. */
	status = dom_cdata_create(document, units, 3, &cdata);
	if (status != 0)
		return status;
	roots[1] = &cdata->cell;
	cdata_address = (uintptr_t)cdata;
	units[0] = 'B';
	data = (struct dom_character_data *)cdata;
	xml_node_check(cdata->type == DOM_CDATA_SECTION && cdata->document == document, "CDATA has native type four and actual owner");
	xml_node_check(data->data.length == 3 && data->data.data[0] == 'A', "CDATA copied caller input before mutation");
	xml_node_check(data->data.data[1] == 0xd800U && data->target == NULL, "native exact UTF16 retains lone surrogate without a PI target");
	classified = dom_is_text(cdata);
	xml_node_check(classified, "CDATA is the Text subtype");
	classified = dom_is_character_data(cdata);
	xml_node_check(classified, "CDATA is genuine CharacterData");

	/* Actual native tree links preserve character identity and clear cleanly on removal. */
	fragment = dom_fragment_create(document);
	if (fragment == NULL)
		return ENOMEM;
	dom_append_child(fragment, cdata);
	xml_node_check(cdata->parent == fragment && fragment->first_child == cdata, "CDATA joins a real native tree");
	dom_remove(cdata);
	xml_node_check(cdata->parent == NULL && fragment->first_child == NULL, "CDATA removal restores detached native identity");

	/* All real CharacterData kinds accept the existing atomic native interval operations. */
	replacement = 'Q';
	status = dom_text_append(pi, &replacement, 1);
	if (status != 0)
		return status;
	data = (struct dom_character_data *)pi;
	xml_node_check(data->data.length == 4 && data->data.data[3] == 'Q', "PI append updates its own character buffer");
	status = dom_text_set(pi, units, 3);
	if (status != 0)
		return status;
	xml_node_check(data->data.length == 3 && data->data.data[0] == 'B', "PI set copies replacement data");
	status = dom_text_replace(pi, 1, 1, &replacement, 1);
	if (status != 0)
		return status;
	xml_node_check(data->data.data[1] == 'Q' && data->target == target, "PI interval replacement preserves immutable target");
	status = dom_text_truncate(pi, 2);
	if (status != 0)
		return status;
	xml_node_check(data->data.length == 2, "PI truncate retains its actual prefix");
	generation = document->generation;
	status = dom_text_replace(pi, 3, 0, NULL, 0);
	xml_node_check(status == EINVAL && document->generation == generation, "invalid PI interval leaves generation unchanged");

	/* CDATA replacement and empty truncation keep its actual subtype rather than retagging it. */
	status = dom_text_replace(cdata, 0, 1, &replacement, 1);
	if (status != 0)
		return status;
	data = (struct dom_character_data *)cdata;
	xml_node_check(data->data.data[0] == 'Q' && cdata->type == DOM_CDATA_SECTION, "CDATA interval replacement preserves native type");
	status = dom_text_truncate(cdata, 0);
	if (status != 0)
		return status;
	xml_node_check(data->data.length == 0, "CDATA empty truncation remains a genuine node");

	/* Rejected native inputs leave no published node and cannot mutate the existing owner graph. */
	rejected = pi;
	status = dom_pi_create(document, NULL, units, 3, &rejected);
	xml_node_check(status == EINVAL && rejected == NULL, "missing PI target rejects without publication");
	rejected = pi;
	status = dom_cdata_create(document, NULL, 1, &rejected);
	xml_node_check(status == EINVAL && rejected == NULL, "missing nonempty C data rejects without publication");
	status = dom_cdata_create(document, units, (size_t)-1, &rejected);
	xml_node_check(status == EOVERFLOW && rejected == NULL, "oversized native data rejects without publication");

	/* Result-only roots retain the exact target and Document after the callee registrations are gone. */
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, target_address);
	xml_node_check(found == (struct vm_cell *)target_address, "result-only PI root retains the non-atom target");
	found = vm_heap_find_cell(heap, document_address);
	xml_node_check(found == (struct vm_cell *)document_address, "result-only character roots retain actual owner");
	status = bind_clone_node(realm, pi, 0, &copy_pi);
	if (status != 0)
		return status;
	roots[2] = &copy_pi->cell;
	status = bind_clone_node(realm, cdata, 0, &copy_cdata);
	if (status != 0)
		return status;
	roots[3] = &copy_cdata->cell;
	data = (struct dom_character_data *)copy_pi;
	xml_node_check(copy_pi->type == DOM_PROCESSING_INSTRUCTION && data->target == target, "native clone preserves PI type and target");
	xml_node_check(data->data.length == 2 && data->data.data[1] == 'Q', "native PI clone copies exact current character data");
	xml_node_check(copy_cdata->type == DOM_CDATA_SECTION && copy_cdata->document == document, "native clone preserves CDATA subtype and owner");

	/* Detached originals become unreachable while the copies retain their target and actual owner. */
	roots[0] = NULL;
	roots[1] = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, pi_address);
	xml_node_check(found == NULL, "clone-only collection releases original PI");
	found = vm_heap_find_cell(heap, cdata_address);
	xml_node_check(found == NULL, "clone-only collection releases original CDATA");
	found = vm_heap_find_cell(heap, target_address);
	xml_node_check(found != NULL, "clone-only collection retains copied PI target");

	/* Last release must collect target and owner; no permanent registry substitutes for native tracing. */
	roots[2] = NULL;
	roots[3] = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, target_address);
	xml_node_check(found == NULL, "last-root release collects non-atom PI target");
	found = vm_heap_find_cell(heap, document_address);
	xml_node_check(found == NULL, "last-root release collects actual owner Document");

	/* Succeeded: owner wrapper now releases nullable registrations, realm and native heap. */
	return 0;
}
