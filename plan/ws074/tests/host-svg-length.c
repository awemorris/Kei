/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercises native SVG scalar width identity, live native attribute values and collectible actual owners. */

#include "page/page.h"
#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* One predicate inspects real DOM and binding behavior after the preceding ordinary mutations. */
struct svg_length_case {
	const char *name;
	const char *source;
};

/* Fixed ordinary source cases do not replace production functions, constructors or native data. */
static const struct svg_length_case svg_length_cases[] = {
    {"native rect inheritance", "r instanceof SVGRectElement&&r instanceof SVGGeometryElement&&r instanceof SVGGraphicsElement&&r instanceof SVGElement&&r instanceof Element&&!(r instanceof HTMLElement)"},
    {"actual stable animated and scalar wrappers", "var w=r.width;var base=w.baseVal;var anim=w.animVal;w===r.width&&base===w.baseVal&&anim===w.animVal&&base!==anim&&w instanceof SVGAnimatedLength&&base instanceof SVGLength&&anim instanceof SVGLength"},
    {"real original numeric width", "base.value===100&&base.valueInSpecifiedUnits===100&&base.unitType===1&&base.valueAsString==='100'&&anim.value===100&&r.getAttribute('width')==='100'"},
    {"actual unit constants", "SVGLength.SVG_LENGTHTYPE_UNKNOWN===0&&SVGLength.SVG_LENGTHTYPE_NUMBER===1&&SVGLength.SVG_LENGTHTYPE_PERCENTAGE===2&&SVGLength.SVG_LENGTHTYPE_EMS===3&&SVGLength.SVG_LENGTHTYPE_EXS===4&&SVGLength.SVG_LENGTHTYPE_PX===5&&SVGLength.SVG_LENGTHTYPE_CM===6&&SVGLength.SVG_LENGTHTYPE_MM===7&&SVGLength.SVG_LENGTHTYPE_IN===8&&SVGLength.SVG_LENGTHTYPE_PT===9&&SVGLength.SVG_LENGTHTYPE_PC===10"},
    {"illegal native length constructor", "(function(){try{new SVGLength();}catch(e){return e instanceof TypeError;}return false;})()"},
    {"illegal animated native constructor", "(function(){try{new SVGAnimatedLength();}catch(e){return e instanceof TypeError;}return false;})()"},
    {"native live external mutation", "r.setAttribute('width','200');base.value===200&&anim.value===200&&r.width===w"},
    {"native px scalar", "r.setAttribute('width','12px');base.value===12&&base.unitType===5&&base.valueAsString==='12px'"},
    {"absolute inches conversion", "r.setAttribute('width','1in');base.value===96&&base.valueInSpecifiedUnits===1&&base.unitType===8"},
    {"absolute centimeters conversion", "r.setAttribute('width','2.54cm');Math.abs(base.value-96)<0.0001&&base.unitType===6"},
    {"absolute millimeters conversion", "r.setAttribute('width','25.4mm');Math.abs(base.value-96)<0.0001&&base.unitType===7"},
    {"absolute points conversion", "r.setAttribute('width','72pt');base.value===96&&base.unitType===9"},
    {"absolute picas conversion", "r.setAttribute('width','6pc');base.value===96&&base.unitType===10"},
    {"actual finite exponent grammar", "r.setAttribute('width','  +1.25e2PX  ');base.value===125&&base.valueAsString==='125px'"},
    {"native signed fraction", "r.setAttribute('width','-.5px');base.value===-0.5&&base.valueAsString==='-0.5px'"},
    {"native finite positive exponent", "r.setAttribute('width','1e+2');base.value===100"},
    {"specified setter preserves current unit", "r.setAttribute('width','1in');base.valueInSpecifiedUnits=2;base.value===192&&anim.value===192&&r.getAttribute('width')==='2in'"},
    {"user unit setter resets to number", "base.value=12;base.value===12&&base.unitType===1&&r.getAttribute('width')==='12'"},
    {"string setter serializes native scalar", "base.valueAsString='  2.5e1PX  ';base.value===25&&r.getAttribute('width')==='25px'"},
    {"relative percentage parsed without guessed scale", "r.setAttribute('width','25%');base.unitType===2&&base.valueInSpecifiedUnits===25&&base.valueAsString==='25%'"},
    {"relative user conversion explicitly unsupported", "(function(){try{base.value;}catch(e){return e instanceof DOMException&&e.name==='NotSupportedError';}return false;})()"},
    {"relative em native reflection", "base.valueAsString='1em';base.unitType===3&&base.valueInSpecifiedUnits===1"},
    {"relative specified setter preserves actual unit", "base.valueInSpecifiedUnits=2;r.getAttribute('width')==='2em'&&anim.valueInSpecifiedUnits===2"},
    {"relative ex native reflection", "base.valueAsString='3ex';base.unitType===4&&base.valueInSpecifiedUnits===3"},
    {"invalid string refuses partial mutation", "(function(){try{base.valueAsString='12junk';}catch(e){return e.name==='SyntaxError'&&r.getAttribute('width')==='3ex';}return false;})()"},
    {"nonfinite numeric setter rejected", "(function(){try{base.value=Infinity;}catch(e){return e instanceof TypeError&&r.getAttribute('width')==='3ex';}return false;})()"},
    {"nonfinite parsed token invalid initial", "r.setAttribute('width','Infinity');base.value===0&&base.unitType===1&&r.getAttribute('width')==='Infinity'"},
    {"hex token invalid initial", "r.setAttribute('width','0x10');base.value===0"},
    {"incomplete exponent invalid initial", "r.setAttribute('width','1e+');base.value===0"},
    {"unit whitespace invalid initial", "r.setAttribute('width','1 px');base.value===0"},
    {"overflow scalar invalid initial", "r.setAttribute('width','1e999');base.value===0"},
    {"missing attribute initial scalar", "r.removeAttribute('width');base.value===0&&base.valueAsString==='0'"},
    {"readonly animated value", "base.value=7;(function(){try{anim.value=8;}catch(e){return e instanceof DOMException&&e.name==='NoModificationAllowedError'&&e.code===7&&base.value===7;}return false;})()"},
    {"readonly animated specified value", "(function(){try{anim.valueInSpecifiedUnits=8;}catch(e){return e.name==='NoModificationAllowedError'&&base.value===7;}return false;})()"},
    {"readonly animated invalid string precedes parsing", "(function(){try{anim.valueAsString='junk';}catch(e){return e.name==='NoModificationAllowedError'&&base.value===7;}return false;})()"},
    {"readonly attributes retain identity", "(function(){'use strict';try{w.baseVal=anim;}catch(e){return e instanceof TypeError&&w.baseVal===base;}return false;})()"},
    {"reentrant specified coercion reads current unit", "base.valueInSpecifiedUnits={valueOf:function(){r.setAttribute('width','2cm');return 3;}};r.getAttribute('width')==='3cm'&&base.unitType===6"},
    {"reentrant string conversion native mutation", "base.valueAsString={toString:function(){r.setAttribute('width','20px');return '4in';}};base.value===384&&r.getAttribute('width')==='4in'"},
    {"actual GC during scalar conversion", "base.value={valueOf:function(){forceCollection();return 11;}};base.value===11&&anim.value===11"},
    {"independent cloned native cache", "var clone=r.cloneNode(false);var cloneWidth=clone.width;cloneWidth!==w&&cloneWidth.baseVal!==base&&cloneWidth.baseVal.value===11&&cloneWidth.baseVal instanceof SVGLength"},
    {"clone mutation does not alter original", "cloneWidth.baseVal.value=13;base.value===11&&clone.getAttribute('width')==='13'"},
    {"wrong native namespace", "(function(){var x=document.createElementNS(ns+'#','rect');try{Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(x);}catch(e){return e instanceof TypeError&&!(x instanceof SVGRectElement);}return false;})()"},
    {"wrong local case", "(function(){var x=document.createElementNS(ns,'Rect');try{Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(x);}catch(e){return e instanceof TypeError&&!(x instanceof SVGRectElement);}return false;})()"},
    {"forged rect prototype rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(Object.create(SVGRectElement.prototype));}catch(e){return e instanceof TypeError;}return false;})()"},
    {"forged length prototype rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(Object.create(SVGLength.prototype));}catch(e){return e instanceof TypeError;}return false;})()"},
    {"wrong native private interface rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(w);}catch(e){return e instanceof TypeError;}return false;})()"},
    {"wrong animated role rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGAnimatedLength.prototype,'baseVal').get.call(base);}catch(e){return e instanceof TypeError;}return false;})()"},
    {"borrowed getter accepts genuine native scalar", "Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(base)===11"},
    {"public expando does not replace native value", "Object.defineProperty(cloneWidth.baseVal,'value',{value:99});clone.getAttribute('width')==='13'&&Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(cloneWidth.baseVal)===13"},
    {"detached live native width", "document.getElementById('root').removeChild(r);base.value===11&&anim.value===11"},
    {"reconnected identical native handles", "document.getElementById('root').appendChild(r);r.width===w&&w.baseVal===base&&base.value===11"}};

/* Predicate outcomes remain available until the complete fixture has been reported. */
static unsigned checks;
/* Preserve every failed predicate while cleanup and independent cases continue. */
static unsigned failures;
/* Genuine child or primary interpreter errors must remain visible. */
static unsigned exceptions;
/* The single fixture restores its original native stack boundary after explicit collection callbacks. */
static const void *svg_stack_base;

static void svg_check(int condition, const char *name);
static void svg_console(void *context, int level, const char *text, size_t length);
static int svg_script(struct page *page, const char *source, vm_value *answer);
static int svg_assert(struct page *page, const char *source, const char *name);
static int svg_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int svg_child(struct page *page);
static int svg_retention(struct page *page);

/*
 * Tests actual SVG scalar classes and live width through a genuine loaded Page.
 */
int
main(
	void)
{
	struct page *page;
	struct vm_function *collect;
	vm_value collect_key;
	struct text_font_paths fonts;
	struct wb_buffer path;
	FILE *file;
	char directory[] = "/tmp/ws074-svg-length-XXXXXX";
	char *created;
	const char *xml;
	const char *html;
	size_t index;
	size_t written;
	size_t xml_length;
	int error;
	int closed;
	int printed;
	int removed;
	int file_created;

	/* Ordinary files supply actual SVG MIME and resource parsing without fixture server hooks. */
	created = mkdtemp(directory);
	if (created == NULL)
		return 2;

	/* Optional native and file owners are published only after successful creation. */
	page = NULL;
	file = NULL;
	file_created = 0;
	wb_buffer_init(&path);
	error = wb_buffer_append_string(&path, directory);
	if (error != 0)
		goto cleanup;

	/* Complete the local SVG source path before opening its ordinary file. */
	error = wb_buffer_append_string(&path, "/text.svg");
	if (error != 0)
		goto cleanup;

	/* Ordinary filesystem creation supplies genuine resource metadata. */
	file = fopen(wb_buffer_string(&path), "wb");
	if (file == NULL) {
		error = EIO;
		goto cleanup;
	}

	/* Cleanup removes this source only after its successful creation. */
	file_created = 1;
	xml = "<svg xmlns='http://www.w3.org/2000/svg'><rect id='child' width='50'/></svg>";
	xml_length = strlen(xml);
	written = fwrite(xml, 1U, xml_length, file);
	if (written != xml_length) {
		error = EIO;
		goto cleanup;
	}

	/* Check the completed source write before the Page can read it. */
	closed = fclose(file);
	file = NULL;
	if (closed != 0) {
		error = EIO;
		goto cleanup;
	}

	/* Create the actual parent before referencing its body or SVG nodes. */
	svg_stack_base = __builtin_frame_address(0);
	error = page_create(&page, svg_stack_base);
	if (error != 0)
		goto cleanup;

	/* Install ordinary host display resources on the complete parent. */
	fonts.sans = "userland/desktop/fonts/Inter.ttf";
	fonts.mono = "userland/desktop/fonts/JetBrainsMono-Regular.ttf";
	fonts.fallback = "userland/desktop/fonts/DroidSansFallbackFull.ttf";
	page_set_console(page, svg_console, NULL);
	page_set_viewport(page, 800, 600);
	page_set_fonts(page, &fonts);
	error = page_open_fonts(page, &fonts);
	if (error != 0)
		goto cleanup;

	/* The parent contains a genuine parser-created canonical SVG rect subtree. */
	html = "<!doctype html><body><svg id='root'><rect id='subject' width='100'/></svg><script>var ns='http://www.w3.org/2000/svg';var r=document.getElementById('subject');</script></body>";
	error = page_load_bytes(page, (const unsigned char *)html, strlen(html), wb_buffer_string(&path));
	if (error != 0)
		goto cleanup;

	/* Intern the ordinary callback name before allocating its native function. */
	collect_key = vm_key_from_ascii(page->heap, "forceCollection");
	if (collect_key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* A fixture host callback requests actual precise-only collection during argument coercion. */
	collect = vm_function_create_native(page->realm, "forceCollection", 0, svg_collect);
	if (collect == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Publish the complete native function through the unchanged ordinary global property. */
	error = vm_object_define(page->heap, page->realm->global, collect_key, vm_value_cell(collect), VM_PROPERTY_DEFAULT);
	if (error != 0)
		goto cleanup;

	/* Record each independent native scalar predicate after the preceding live mutations. */
	for (index = 0; index < sizeof(svg_length_cases) / sizeof(svg_length_cases[0]); index++) {
		/* Record each actual native predicate before later live tree edits. */
		error = svg_assert(page, svg_length_cases[index].source, svg_length_cases[index].name);
		if (error != 0)
			goto cleanup;
	}

	/* A real SVG child also exercises independently owned scalar wrappers and prototype identity. */
	error = svg_child(page);
	if (error != 0)
		goto cleanup;

	/* Retire the actual child and verify its sole saved native owner. */
	error = svg_retention(page);
	if (error != 0)
		goto cleanup;

	/* Actual console exceptions remain an independent final failure criterion. */
	svg_check(exceptions == 0, "zero actual interpreter console exceptions");

cleanup:
	/* Teardown owns all native contexts before deleting ordinary temporary source files. */
	page_destroy(page);

	/* Close only a source stream still owned after failed setup. */
	if (file != NULL) {
		closed = fclose(file);
		if (closed != 0 && error == 0)
			error = EIO;
	}

	/* Remove only the source file actually created by this fixture. */
	if (file_created) {
		removed = unlink(wb_buffer_string(&path));
		if (removed != 0 && error == 0)
			error = EIO;
	}

	/* Every exit releases its successfully created ordinary source directory. */
	removed = rmdir(directory);
	if (removed != 0 && error == 0)
		error = EIO;
	wb_buffer_release(&path);

	/* Reports behavioral totals and the first failed fixture operation independently. */
	printed = printf("native SVG length: %u/%u passed; error=%d\n", checks - failures, checks, error);
	if (printed < 0 || error != 0)
		return 2;

	/* Refuse a fixture with any failed native observation. */
	if (failures != 0)
		return 1;

	/* Succeeded: every native scalar and actual ownership observation passed. */
	return 0;
}

/* Records one genuine native predicate without suppressing subsequent independent checks. */
static void
svg_check(
	int condition,
	const char *name)
{
	int printed;

	/* Retain the original failing observation at its actual source. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation belongs to the final fixture totals. */
	return;
}

/* Reports actual interpreter exceptions independently of ordinary predicate results. */
static void
svg_console(
	void *context,
	int level,
	const char *text,
	size_t length)
{
	int printed;

	UNUSED_PARAMETER(context);

	/* Error output remains visible under both sanitizer and ordinary execution. */
	if (level == BIND_CONSOLE_ERROR) {
		exceptions++;
		printed = fprintf(stderr, "console: %.*s\n", (int)length, text);
		if (printed < 0)
			exceptions++;
	}

	/* Succeeded: the fixture does not alter production interpreter behavior. */
	return;
}

/* Runs ordinary script source through the actual Page interpreter. */
static int
svg_script(
	struct page *page,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct wb_buffer message;
	struct js_syntax_error syntax;
	int error;
	int rendered;
	int printed;

	/* Convert fixture-owned source before invoking the native binding. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Execute only completely converted ordinary script source. */
	error = js_run_script(page->realm, units.data, units.length, 0, answer, &syntax);
	if (error != 0) {
		/* Genuine exceptions remain diagnostic evidence rather than false predicate success. */
		wb_buffer_init(&message);
		rendered = js_exception_text(page->realm, page->realm->exception, &message);
		if (rendered != 0) {
			wb_buffer_release(&message);
			wb_units_release(&units);
			return error;
		}

		/* Report the actual exception without hiding the original script error. */
		printed = fprintf(stderr, "exception: %s\n", wb_buffer_string(&message));
		if (printed < 0)
			exceptions++;
		wb_buffer_release(&message);
		wb_units_release(&units);
		return error;
	}

	/* Completed execution no longer borrows converted fixture storage. */
	wb_units_release(&units);

	/* Succeeded: the actual native script completion is available. */
	return 0;
}

/* Observes a real interpreter predicate and retains its independent result. */
static int
svg_assert(
	struct page *page,
	const char *source,
	const char *name)
{
	vm_value answer;
	int correct;
	int error;
	int printed;

	/* Ordinary script inspects actual native identity and current DOM data. */
	error = svg_script(page, source, &answer);
	if (error != 0) {
		printed = fprintf(stderr, "script failure %d: %s\n", error, name);
		if (printed < 0)
			exceptions++;
		return error;
	}

	/* The predicate's real completion determines success, independently of console formatting. */
	correct = vm_to_boolean(answer);
	svg_check(correct, name);

	/* Succeeded: this native predicate has been recorded. */
	return 0;
}

/* Forces genuine collector execution while the native scalar setter protects its actual owner graph. */
static int
svg_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Eliminate conservative native-stack rescue during the actual coercion callback. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, svg_stack_base);

	/* Succeeded: the setter continues after a real precise-root collection. */
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Activates a real local SVG response and verifies independently owned native scalar prototypes. */
static int
svg_child(
	struct page *page)
{
	int error;

	/* Ordinary resource metadata and strict XML parsing create the actual child rect. */
	error = svg_assert(page, "var childFrame=document.createElement('iframe');childFrame.src='text.svg';document.body.appendChild(childFrame);true;", "actual SVG child source connected");
	if (error != 0)
		return error;

	/* Activate the connected child through the ordinary frame checkpoint. */
	error = page_frames_checkpoint(page);
	if (error != 0)
		return error;

	/* Observe independently owned child classes and their actual native data. */
	error = svg_assert(page, "var cd=childFrame.contentDocument;var cw=childFrame.contentWindow;var cRect=cd.getElementById('child');var ct=cRect.width.baseVal;cRect instanceof cw.SVGRectElement&&ct instanceof cw.SVGLength&&!(ct instanceof SVGLength)&&ct.value===50;", "actual child native owner prototypes and live scalar");
	if (error != 0)
		return error;

	/* Observe the child native interface through the original borrowed primary entry point. */
	error = svg_assert(page, "Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(cRect)===cRect.width&&Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(ct)===50;", "borrowed primary getters preserve actual child graph");
	if (error != 0)
		return error;

	/* Succeeded: the actual child owns distinct native scalar prototypes. */
	return 0;
}

/* Retains a sole native SVG scalar after retirement and verifies its final graph release. */
static int
svg_retention(
	struct page *page)
{
	struct dom_node *node;
	struct dom_document *document;
	struct bind_window *window;
	struct vm_cell *held;
	struct vm_cell *found;
	vm_value answer;
	vm_value wrapper;
	vm_value key;
	uintptr_t address;
	int error;
	int numeric;

	/* Extract the real native child rather than copying its public interface or Document. */
	error = svg_script(page, "cd;", &answer);
	if (error != 0)
		return error;
	node = bind_node_of(answer);
	if (node == NULL)
		return EINVAL;

	/* Inspect the genuine node owner only after successful native branding. */
	document = node->document;
	window = document->view;
	if (window == NULL || window->realm == NULL)
		return EINVAL;

	/* The sole saved scalar wrapper, rather than its public Document alias, owns the retained graph. */
	error = svg_script(page, "ct;", &wrapper);
	if (error != 0)
		return error;
	numeric = vm_value_is_object(wrapper);
	if (!numeric)
		return EINVAL;

	/* The successfully branded scalar receives the original sole saved root. */
	held = vm_value_as_cell(wrapper);
	address = (uintptr_t)&document->node.cell;
	error = vm_heap_add_root(page->heap, &held);
	if (error != 0)
		return error;

	/* Remove all public aliases before precise-only collection of the saved native node. */
	error = svg_assert(page, "document.body.removeChild(childFrame);childFrame=null;cd=null;cw=null;ct=null;cRect=null;true;", "actual loaded SVG child retired before sole native root");
	if (error != 0)
		goto cleanup;

	/* Observe genuine collection after all public child aliases have been removed. */
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, address);
	svg_check(found != NULL, "sole native SVG length retains actual owning Document and creator realm");
	if (found == NULL) {
		error = EINVAL;
		goto cleanup;
	}

	/* The retained wrapper resolves the actual getter after collection without a primary prototype alias. */
	key = vm_key_from_ascii(page->heap, "value");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Resolve the saved native interface through its actual retained realm. */
	error = vm_get(window->realm, wrapper, key, &answer);
	if (error != 0)
		goto cleanup;

	/* Observe the successful saved native numeric completion. */
	numeric = vm_value_is_number(answer);
	svg_check(numeric, "saved-only actual native SVG length getter survives collection");
	if (numeric)
		svg_check(vm_value_as_number(answer) == 50, "retired actual SVG scalar still reflects the real owner attribute");

	/* No temporary operation or native interface table permanently roots the retained child graph. */
	held = NULL;
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, address);
	svg_check(found == NULL, "last native SVG length release collects actual child Document and managed realm");

cleanup:
	/* Release the acquired saved slot even when a native lookup or script fails. */
	vm_heap_remove_root(page->heap, &held);
	vm_heap_set_stack_base(page->heap, svg_stack_base);
	if (error != 0)
		return error;

	/* Succeeded: saved SVG length accessors and their owning native graph have collectible precise lifetimes. */
	return 0;
}
