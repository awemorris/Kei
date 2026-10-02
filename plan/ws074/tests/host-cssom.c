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

static void cssom_check(int condition, const char *name);
static int cssom_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int cssom_case(struct page *page);


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
	if (status == 0)
		status = cssom_case(page);
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	page_destroy(page);
	if (status != 0)
		return 2;

	/* Publish complete observations after every production resource was released. */
	printed = printf("native CSSOM: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: actual layout, source ownership and collector lifetime were verified. */
	return 0;
}

/* Records an independent layout or lifetime observation. */
static void
cssom_check(
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
}

/* Executes ordinary scripts to construct production managed child contexts. */
static int
cssom_script(
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
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the script supplied actual native state. */
	return 0;
}

/* Checks genuine native model edits and actual public wrapper lifetime through threshold collection. */
static int
cssom_case(
	struct page *page)
{
	/* Ordinary inert cells approach the unchanged production collector threshold. */
	static const struct vm_cell_type pressure_type = { "cssom-pressure", NULL, NULL };
	struct dom_element *element;
	struct bind_style_sheet *source;
	struct bind_window *owner;
	struct vm_cell *root;
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	struct wb_units units;
	vm_value answer;
	vm_value receiver;
	uintptr_t model_address;
	uintptr_t document_address;
	uintptr_t realm_address;
	uintptr_t rules_address;
	double length;
	int truth;
	int status;

	/* Actual child rendering and saved JS rule counts both use the same native source model. */
	status = cssom_script(page->realm,
			      "var f=document.createElement('iframe');f.style.cssText='display:block;width:120px;height:80px';document.body.appendChild(f);"
			      "var d=f.contentDocument;d.open();d.write('<style>img{display:block;width:10px;height:10px}</style><img>');d.close();"
			      "var s=d.getElementsByTagName('style')[0];var old=s.sheet,rs=old.cssRules;s",
			      &answer);
	if (status != 0)
		return status;
	element = (struct dom_element *)bind_node_of(answer);
	status = bind_style_sheet_get(element, &source);
	if (status != 0)
		return status;
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)"img{height:20px}", 16U, &units);
	if (status == 0)
		status = css_rule_model_insert(source->model, units.data, units.length, 1);
	wb_units_release(&units);
	if (status != 0)
		return status;
	bind_style_sheet_changed(source);
	status = cssom_script(page->realm, "rs===old.cssRules && rs.length===2 && d.images[0].height===20 && s.firstChild.data==='img{display:block;width:10px;height:10px}'", &answer);
	if (status != 0)
		return status;
	truth = vm_to_boolean(answer);
	cssom_check(truth, "genuine model edit changes saved JS rule count and actual Page height without DOM Text writes");

	/* Native same-text replacement retires the edited model and yields a genuinely new public sheet. */
	status = cssom_script(page->realm, "s.firstChild.data=s.firstChild.data;rs.length===2 && old.ownerNode===null && s.sheet!==old && s.sheet.cssRules.length===1 && d.images[0].height===10", &answer);
	if (status != 0)
		return status;
	truth = vm_to_boolean(answer);
	cssom_check(truth, "old saved rule list preserves its model after actual same-text source replacement");

	/* A fresh source has no cached rule view before the direct allocating factory is challenged. */
	status = cssom_script(page->realm, "s.firstChild.data=s.firstChild.data;old=null;rs=null;s.sheet", &receiver);
	if (status != 0)
		return status;
	status = bind_style_sheet_get(element, &source);
	if (status != 0)
		return status;
	root = vm_value_as_cell(receiver);
	status = vm_heap_add_root(page->heap, &root);
	if (status != 0)
		return status;
	owner = element->node.document->view;
	model_address = (uintptr_t)&source->cell;
	document_address = (uintptr_t)&element->node.document->node.cell;
	realm_address = (uintptr_t)&owner->realm->cell;
	status = cssom_script(page->realm, "f.parentNode.removeChild(f);f=null;d=null;s=null", &answer);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		return status;
	}

	/* No permanent iframe or conservative native-stack edge retains this retired actual owner graph. */
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, model_address);
	cssom_check(found == &source->cell && owner->detached, "saved native sheet wrapper alone retains retired source and owner");
	pressure = vm_heap_alloc(page->heap, &pressure_type, 8U * 1024U * 1024U);
	if (pressure == NULL) {
		vm_heap_remove_root(page->heap, &root);
		return ENOMEM;
	}

	/* Remove the caller root before the getter roots its own genuine source through construction. */
	root = NULL;
	vm_heap_stats(page->heap, &before);
	status = bind_cssom_rules(page->realm, receiver, NULL, 0, &answer);
	vm_heap_stats(page->heap, &after);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		return status;
	}

	/* Keep only the returned public rule view during all subsequent actual collection observations. */
	root = vm_value_as_cell(answer);
	rules_address = (uintptr_t)root;
	cssom_check(after.collections > before.collections, "genuine threshold GC runs inside direct rule view construction without caller roots");
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, model_address);
	cssom_check(found == &source->cell, "saved public rule view alone traces actual source model");
	found = vm_heap_find_cell(page->heap, document_address);
	cssom_check(found != NULL, "saved public rule view retains retired actual Document");
	found = vm_heap_find_cell(page->heap, realm_address);
	cssom_check(found != NULL, "saved public rule view retains actual relevant realm");
	status = bind_cssom_rules_length(page->realm, answer, NULL, 0, &receiver);
	if (status == 0)
		status = vm_to_number(page->realm, receiver, &length);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		return status;
	}

	/* Reading the saved source count remains safe after its actual owner was retired and collected around. */
	cssom_check(length == 1, "saved public rule view remains readable after owner retirement GC");
	vm_heap_remove_root(page->heap, &root);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, rules_address);
	cssom_check(found == NULL, "released public rule view is collectible");
	found = vm_heap_find_cell(page->heap, model_address);
	cssom_check(found == NULL, "released public view leaves no permanent source model root");
	found = vm_heap_find_cell(page->heap, document_address);
	cssom_check(found == NULL, "released public view leaves no retained Document");
	found = vm_heap_find_cell(page->heap, realm_address);
	cssom_check(found == NULL, "released public view leaves no retained relevant realm");
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));

	/* Succeeded: actual source count, rendered effects and callee-owned construction were verified. */
	return 0;
}
