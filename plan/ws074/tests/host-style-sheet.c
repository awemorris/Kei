/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies actual inline source ownership, Page rendering and retired managed-child collection. */

#include "bind/internal.h"
#include "page/page.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Count independent geometry and lifetime observations across the native fixture. */
static unsigned checks;
/* Preserve failed observations until the final result is printed. */
static unsigned failures;

static void sheet_check(int condition, const char *name);
static int sheet_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int sheet_case(struct page *page);
static int sheet_insert(struct bind_style_sheet *sheet, const char *source, size_t index);
static int sheet_height(struct page *page, struct vm_realm *realm, const char *source, int height);

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
		status = sheet_case(page);
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	page_destroy(page);
	if (status != 0)
		return 2;

	/* Publish complete observations after every production resource was released. */
	printed = printf("native style source: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: actual layout, source ownership and collector lifetime were verified. */
	return 0;
}

/* Records an independent layout or lifetime observation. */
static void
sheet_check(
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
sheet_script(
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

/* Edits genuine native source storage and signals only successful current-model changes. */
static int
sheet_insert(
	struct bind_style_sheet *sheet,
	const char *source,
	size_t index)
{
	struct wb_units units;
	int status;

	/* Native CSS insertion owns its source and never changes the owner's DOM Text. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status == 0)
		status = css_rule_model_insert(sheet->model, units.data, units.length, index);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Invalidation is separate from pure model storage and runs only after successful insertion. */
	bind_style_sheet_changed(sheet);

	/* Succeeded: the native model now supplies the current source. */
	return 0;
}

/* Queries actual image content geometry through the production binding getter. */
static int
sheet_height(
	struct page *page,
	struct vm_realm *realm,
	const char *source,
	int height)
{
	vm_value receiver;
	vm_value answer;
	int status;

	/* Evaluate an ordinary DOM image expression before using its real native getter. */
	status = sheet_script(realm, source, &receiver);
	if (status != 0)
		return status;
	status = bind_html_image_element_interface.attributes[3].getter(page->realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	sheet_check(answer == vm_value_int32(height), source);

	/* Succeeded: the actual Page or managed-child cascade supplied the queried height. */
	return 0;
}

/* Exercises source identity, genuine model edits, DOM lifecycle and actual VM collection. */
static int
sheet_case(
	struct page *page)
{
	struct dom_element *element;
	struct dom_node *text;
	struct dom_node *empty;
	struct bind_style_sheet *sheet;
	struct bind_style_sheet *current;
	struct bind_window *owner;
	struct vm_cell *root;
	struct vm_cell *found;
	struct wb_units units;
	vm_value answer;
	uintptr_t state_address;
	uintptr_t document_address;
	uintptr_t realm_address;
	uint64_t generation;
	size_t count;
	int status;
	int same;

	/* Primary and child styles start with different actual native cascade results. */
	status = sheet_script(page->realm,
			      "var ps=document.createElement('style');ps.appendChild(document.createTextNode('img{display:block;width:10px;height:7px}'));document.head.appendChild(ps);"
			      "document.body.appendChild(document.createElement('img'));"
			      "var f=document.createElement('iframe');f.style.cssText='display:block;width:120px;height:80px';document.body.appendChild(f);"
			      "var d=f.contentDocument;d.open();d.write('<style>img{display:block;width:10px;height:10px}</style><img>');d.close();"
			      "var s=d.getElementsByTagName('style')[0];var mo=new MutationObserver(function(){});mo.observe(s,{childList:true,characterData:true,subtree:true});s",
			      &answer);
	if (status != 0)
		return status;
	element = (struct dom_element *)bind_node_of(answer);
	text = element->node.first_child;
	status = bind_style_sheet_get(element, &sheet);
	if (status != 0)
		return status;
	status = sheet_height(page, page->realm, "d.images[0]", 10);
	if (status != 0)
		return status;
	status = sheet_insert(sheet, "img{height:20px}", 1);
	if (status != 0)
		return status;
	status = sheet_height(page, page->realm, "d.images[0]", 20);
	if (status != 0)
		return status;
	status = sheet_insert(sheet, "img{height:40px}", 2);
	if (status != 0)
		return status;
	status = sheet_height(page, page->realm, "d.images[0]", 40);
	if (status != 0)
		return status;
	status = css_rule_model_delete(sheet->model, 2);
	if (status != 0)
		return status;
	bind_style_sheet_changed(sheet);
	status = sheet_height(page, page->realm, "d.images[0]", 20);
	if (status != 0)
		return status;
	status = sheet_height(page, page->realm, "document.images[0]", 7);
	if (status != 0)
		return status;

	/* Native rule edits are independent of DOM Text and MutationObserver record creation. */
	status = sheet_script(page->realm, "s.firstChild.data==='img{display:block;width:10px;height:10px}' && mo.takeRecords().length===0", &answer);
	if (status != 0)
		return status;
	sheet_check(answer == vm_value_boolean(1), "model edits preserve actual DOM Text and create no mutation records");
	status = sheet_script(page->realm, "d.body.appendChild(d.createElement('span'))", &answer);
	if (status != 0)
		return status;
	status = bind_style_sheet_get(element, &current);
	if (status != 0)
		return status;
	sheet_check(current == sheet, "unrelated DOM edits preserve current native sheet identity");

	/* Save only the old native state while same-text replacement discards its current association. */
	root = &sheet->cell;
	status = vm_heap_add_root(page->heap, &root);
	if (status != 0)
		return status;
	status = sheet_script(page->realm, "s.firstChild.data=s.firstChild.data", &answer);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Continue only after the previous native operation succeeded. */
	status = bind_style_sheet_get(element, &current);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Observe the completed native operation before proceeding. */
	count = css_rule_model_count(current->model);
	sheet_check(current != sheet && count == 1U, "same-text replacement creates a new original model");
	generation = element->node.document->generation;
	status = sheet_insert(sheet, "img{height:99px}", 2);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Observe the completed native operation before proceeding. */
	sheet_check(element->node.document->generation == generation, "obsolete model edit does not invalidate current Document");
	status = sheet_height(page, page->realm, "d.images[0]", 10);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Appending even empty Text retires the current identity rather than comparing source hashes. */
	empty = dom_text_create(element->node.document, NULL, 0);
	if (empty == NULL) {
		vm_heap_remove_root(page->heap, &root);
		return ENOMEM;
	}

	/* The real child-list operation retires the previous source identity. */
	dom_append_child(&element->node, empty);
	sheet_check(element->style_sheet == NULL, "empty Text child insertion retires current source identity");
	status = bind_style_sheet_get(element, &current);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Removing the actual child must retire the newly current model. */
	dom_remove(empty);
	sheet_check(element->style_sheet == NULL, "Text child removal retires current source identity");
	status = bind_style_sheet_get(element, &current);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Continue only after the previous native operation succeeded. */
	status = dom_text_append(text, NULL, 0);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Observe the completed native operation before proceeding. */
	sheet_check(element->style_sheet == NULL, "empty Text data append runs source retirement lifecycle");
	status = sheet_script(page->realm, "d.head.removeChild(s);d.head.appendChild(s);s.firstChild.data='img{display:block;width:10px;height:30px}'", &answer);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Continue only after the previous native operation succeeded. */
	status = sheet_height(page, page->realm, "d.images[0]", 30);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* A genuine owner-only root retains its current state with conservative stack discovery disabled. */
	vm_heap_remove_root(page->heap, &root);
	root = &element->node.cell;
	status = vm_heap_add_root(page->heap, &root);
	if (status != 0)
		return status;
	status = bind_style_sheet_get(element, &current);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Record identity before disabling conservative stack discovery. */
	state_address = (uintptr_t)&current->cell;
	status = sheet_script(page->realm, "mo.disconnect();mo=null;s=null;d=null;f.parentNode.removeChild(f);f=null", &answer);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Collect only after clearing script-held child references. */
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, state_address);
	sheet_check(found == &current->cell && element->style_sheet == found, "retired actual owner alone retains current model across real GC");

	/* A saved source state alone retains its retired actual child Document and realm. */
	vm_heap_remove_root(page->heap, &root);
	root = &current->cell;
	status = vm_heap_add_root(page->heap, &root);
	if (status != 0)
		return status;
	owner = element->node.document->view;
	document_address = (uintptr_t)&element->node.document->node.cell;
	realm_address = (uintptr_t)&owner->realm->cell;

	/* Only the saved state now roots the already retired child graph. */
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, document_address);
	sheet_check(found != NULL && owner->detached, "saved state alone retains retired actual child Document");
	found = vm_heap_find_cell(page->heap, realm_address);
	sheet_check(found != NULL, "saved state traces actual binding realm after child retirement");
	wb_units_init(&units);
	status = css_rule_model_text(current->model, &units);
	same = units.length == current->original.length + 1U;
	if (same)
		same = memcmp(units.data, current->original.data, current->original.length * sizeof(*units.data)) == 0;
	wb_units_release(&units);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &root);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		return status;
	}

	/* Observe the completed native operation before proceeding. */
	sheet_check(same, "saved retired model remains readable after actual GC");

	/* Releasing the last saved model root leaves no permanent owner or resource edge. */
	vm_heap_remove_root(page->heap, &root);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, state_address);
	sheet_check(found == NULL, "released saved model is collectible");
	found = vm_heap_find_cell(page->heap, document_address);
	sheet_check(found == NULL, "released saved model leaves no retained Document");
	found = vm_heap_find_cell(page->heap, realm_address);
	sheet_check(found == NULL, "released saved model leaves no retained child realm");
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));

	/* Succeeded: all genuine model, lifecycle, rendering and lifetime observations completed. */
	return 0;

}
