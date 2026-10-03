/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies native JS sheet wrappers, genuine model counts, rendering and source-only retirement GC. */

#include "bind/internal.h"
#include "page/page.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Count independent geometry and lifetime observations across the native fixture. */
static unsigned checks;
/* Preserve failed observations until the final result is printed. */
static unsigned failures;

/* A single conversion retires its actual frame and current Text-derived source; these are never VM roots. */
static struct dom_node *conversion_frame;
/* The native Text target is valid only while the tested method owns its source graph. */
static struct dom_node *conversion_text;
/* Capture actual source retirement generation before any later saved-model mutation. */
static uint32_t conversion_generation;
/* Count actual index conversions whose callbacks explicitly collect the heap. */
static unsigned conversions;

static int mutation_index(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static void mutation_check(int condition, const char *name);
static int mutation_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int mutation_case(struct page *page);

/*
 * Runs real Page layout, native inline source lifecycle and collector observations.
 */
int
main(
	void)
{
	struct page *page;
	struct text_font_paths paths;
	static const unsigned char html[] = "<!doctype html><html><body></body></html>";
	int status;
	int printed;

	/* Ordinary checked-in fonts supply real layout without a fixture implementation. */
	paths.sans = "userland/desktop/fonts/Inter.ttf";
	paths.mono = "userland/desktop/fonts/JetBrainsMono-Regular.ttf";
	paths.fallback = "userland/desktop/fonts/DroidSansFallbackFull.ttf";
	status = page_create(&page, __builtin_frame_address(0));
	if (status != 0)
		return 2;
	page_set_fonts(page, &paths);
	page_set_viewport(page, 800, 600);

	/* Load a genuine primary Page before constructing the binding-owned child. */
	status = page_load_html(page, html, sizeof(html) - 1U);
	if (status != 0) {
		page_destroy(page);
		return 2;
	}

	/* The complete Page now supports the actual mutation and retirement cases. */
	status = mutation_case(page);
	if (status != 0) {
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		page_destroy(page);
		return 2;
	}

	/* Restores the embedding boundary before ordinary Page teardown. */
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	page_destroy(page);

	/* Publish complete observations after every production resource was released. */
	printed = printf("native CSSOM mutation: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: actual layout, source ownership and collector lifetime were verified. */
	return 0;
}

/* Records an independent layout or lifetime observation. */
static void
mutation_check(
	int condition,
	const char *name)
{
	int printed;

	/* Retain all failures while later independent observations still run. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the geometry or lifetime observation is recorded. */
	return;
}

/* Executes ordinary scripts to construct production managed child contexts. */
static int
mutation_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Convert fixture input to the interpreter's ordinary source representation. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Execute genuine DOM operations without a production test switch. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* No converted source is borrowed after script execution finishes. */
	wb_units_release(&units);

	/* Succeeded: the script supplied actual native state. */
	return 0;
}

/* Retires actual DOM identity and embedding during an ordinary native index conversion callback. */
static int
mutation_index(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_character_data *text;
	int status;

	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Same-text replacement runs the real source lifecycle without changing original source units. */
	if (conversion_text == NULL || conversion_frame == NULL)
		return EIO;

	/* Only the still-live expected Text can provide converted source units. */
	text = (struct dom_character_data *)conversion_text;
	status = dom_text_set(conversion_text, text->data.data, text->data.length);
	if (status != 0)
		return status;
	conversion_generation = conversion_text->document->generation;
	dom_remove(conversion_frame);
	conversion_frame = NULL;
	conversion_text = NULL;
	conversions++;
	vm_heap_collect(realm->heap);

	/* Succeeded: conversion returns an actual insertion index after source and owner retirement GC. */
	*result = vm_value_int32(1);
	return 0;
}

/* Challenges real native methods with unrooted caller inputs, reentrancy and actual collector pressure. */
static int
mutation_case(
	struct page *page)
{
	/* An ordinary inert allocation reaches the unchanged production collection threshold. */
	static const struct vm_cell_type pressure_type = {"cssom-mutation-pressure", NULL, NULL};
	struct dom_element *element;
	struct dom_node *node;
	struct bind_style_sheet *source;
	struct bind_style_sheet *current;
	struct bind_window *owner;
	struct vm_cell *roots[4];
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_function *function;
	struct vm_object *index_object;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value receiver;
	vm_value args[2];
	vm_value answer;
	vm_value key;
	uintptr_t model_address;
	uintptr_t document_address;
	uintptr_t realm_address;
	size_t count;
	unsigned slot;
	unsigned registered;
	double index;
	int truth;
	int status;

	/* Genuine child streams and fonts supply ordinary rendered geometry before mutation pressure. */
	status = mutation_script(page->realm,
				 "var f=document.createElement('iframe');f.style.cssText='display:block;width:120px;height:80px';document.body.appendChild(f);"
				 "var d=f.contentDocument;d.open();d.write('<style>img{display:block;width:10px;height:10px}</style><img>');d.close();"
				 "var s=d.getElementsByTagName('style')[0];d.images[0].height===10",
				 &answer);
	if (status != 0)
		return status;
	truth = vm_to_boolean(answer);
	mutation_check(truth, "actual initial child cascade renders height10");
	status = mutation_script(page->realm, "s.sheet", &receiver);
	if (status != 0)
		return status;
	status = mutation_script(page->realm, "s", &answer);
	if (status != 0)
		return status;
	node = bind_node_of(answer);
	if (node == NULL || node->type != DOM_ELEMENT)
		return EIO;

	/* Uses only a validated native style element for source ownership. */
	element = (struct dom_element *)node;
	status = bind_style_sheet_get(element, &source);
	if (status != 0)
		return status;
	owner = element->node.document->view;
	if (source == NULL ||
	    owner == NULL ||
	    owner->frame == NULL ||
	    element->node.first_child == NULL ||
	    element->node.first_child->type != DOM_TEXT)
		return EIO;

	/* Captures actual model and child identities before explicit collection. */
	model_address = (uintptr_t)&source->cell;
	document_address = (uintptr_t)&element->node.document->node.cell;
	realm_address = (uintptr_t)&owner->realm->cell;

	/* Explicit construction roots are registered once and unwound for every fallible fixture step. */
	for (slot = 0; slot < 4U; slot++) {
		roots[slot] = NULL;
	}

	/* Tracks only the roots successfully registered for later cleanup. */
	registered = 0;
	for (slot = 0; slot < 4U; slot++) {
		status = vm_heap_add_root(page->heap, &roots[slot]);
		if (status != 0)
			goto cleanup;

		/* Counts only root slots successfully registered with the actual heap. */
		registered++;
	}

	/* Prepare ordinary reentrant input objects whose string result is allocated during conversion. */
	roots[0] = vm_value_as_cell(receiver);
	status = mutation_script(page->realm, "({toString:function(){return 'img{height:'+String(23)+'px}';}})", &args[0]);
	if (status != 0)
		goto cleanup;

	/* Preserves converted rule input while constructing the numeric callback. */
	roots[1] = vm_value_as_cell(args[0]);
	index_object = vm_object_create(page->heap, page->realm->object_prototype);
	if (index_object == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* The index object's ordinary valueOf invokes only this fixture's real DOM and GC operations. */
	roots[2] = &index_object->cell;
	args[1] = vm_value_cell(index_object);
	function = vm_function_create_native(page->realm, "valueOf", 0, mutation_index);
	if (function == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Preserve the new function while property shape construction allocates. */
	roots[3] = &function->object.cell;
	key = vm_key_from_ascii(page->heap, "valueOf");
	if (key == VM_VALUE_EMPTY) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Publish the complete native callback as an ordinary valueOf property. */
	status = vm_object_define(page->heap, index_object, key, vm_value_cell(function), VM_PROPERTY_DEFAULT);
	if (status != 0)
		goto cleanup;

	/* Callback context lasts until the one source-retirement conversion. */
	conversion_frame = &owner->frame->node;
	conversion_text = element->node.first_child;
	status = mutation_script(page->realm, "f=null;d=null;s=null", &answer);
	if (status != 0)
		goto cleanup;

	/* Heap pressure makes the direct method's first allocating conversion collect genuine cells. */
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_collect(page->heap);
	pressure = vm_heap_alloc(page->heap, &pressure_type, 8U * 1024U * 1024U);
	if (pressure == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* No caller source, argument or conservative-stack roots remain during either conversion. */
	for (slot = 0; slot < 4U; slot++) {
		roots[slot] = NULL;
	}

	/* Captures ordinary collector counters before the actual method call. */
	vm_heap_stats(page->heap, &before);
	status = bind_cssom_insert(page->realm, receiver, args, 2, &answer);
	if (status != 0)
		goto cleanup;

	/* Collector counters are meaningful after the actual insertion finishes. */
	vm_heap_stats(page->heap, &after);
	roots[0] = vm_value_as_cell(receiver);
	status = vm_to_number(page->realm, answer, &index);
	if (status != 0)
		goto cleanup;
	mutation_check(index == 1 && conversions == 1, "direct native insertion converts actual rule and reentrant index");
	mutation_check(after.collections >= before.collections + 2U, "actual threshold GC and index callback GC run inside native insertion");
	mutation_check(owner->detached && element->style_sheet == NULL, "index conversion retires actual source and managed owner");
	mutation_check(element->node.document->generation == conversion_generation, "saved obsolete model insertion does not invalidate replacement source Document");
	count = css_rule_model_count(source->model);
	mutation_check(count == 2U, "converted string survives callback GC and inserts into actual saved source");

	/* Saved receiver state retains the old model while a fresh actual current model can be rebuilt. */
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, model_address);
	mutation_check(found == &source->cell, "saved public receiver alone retains actual old source after conversion retirement");
	if (found != (struct vm_cell *)model_address) {
		status = EIO;
		goto cleanup;
	}

	/* Rebuilds current source only while the retained receiver graph is verified. */
	status = bind_style_sheet_get(element, &current);
	if (status != 0)
		goto cleanup;
	if (current == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Observes only the successfully rebuilt current source. */
	count = css_rule_model_count(current->model);
	mutation_check(current != source && count == 1U, "replacement current source remains original single rule after saved model insertion");
	args[0] = vm_value_int32(1);
	status = bind_cssom_delete(page->realm, receiver, args, 1, &answer);
	if (status != 0)
		goto cleanup;
	count = css_rule_model_count(source->model);
	mutation_check(answer == VM_VALUE_UNDEFINED && count == 1U, "direct native saved model deletion returns undefined and updates real count");

	/* The last actual saved JS receiver edge is released before collection of every retired owner. */
	roots[0] = NULL;
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, model_address);
	mutation_check(found == NULL, "released mutation receiver leaves no permanent model root");
	found = vm_heap_find_cell(page->heap, document_address);
	mutation_check(found == NULL, "released mutation receiver leaves no retained actual Document");
	found = vm_heap_find_cell(page->heap, realm_address);
	mutation_check(found == NULL, "released mutation receiver leaves no retained actual child realm");

	/* Clear fixture-only callback context and unwind every registered root on success or failure. */
cleanup:
	conversion_frame = NULL;
	conversion_text = NULL;
	for (slot = 0; slot < registered; slot++) {
		vm_heap_remove_root(page->heap, &roots[slot]);
	}

	/* Restores the embedding boundary before leaving any fixture outcome. */
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	if (status != 0)
		return status;

	/* Succeeded: both conversions, actual source retirement and final collector release were verified. */
	return 0;
}
