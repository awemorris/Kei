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
	{ "native rect inheritance", "r instanceof SVGRectElement&&r instanceof SVGGeometryElement&&r instanceof SVGGraphicsElement&&r instanceof SVGElement&&r instanceof Element&&!(r instanceof HTMLElement)" },
	{ "actual stable animated and scalar wrappers", "var w=r.width;var base=w.baseVal;var anim=w.animVal;w===r.width&&base===w.baseVal&&anim===w.animVal&&base!==anim&&w instanceof SVGAnimatedLength&&base instanceof SVGLength&&anim instanceof SVGLength" },
	{ "real original numeric width", "base.value===100&&base.valueInSpecifiedUnits===100&&base.unitType===1&&base.valueAsString==='100'&&anim.value===100&&r.getAttribute('width')==='100'" },
	{ "actual unit constants", "SVGLength.SVG_LENGTHTYPE_UNKNOWN===0&&SVGLength.SVG_LENGTHTYPE_NUMBER===1&&SVGLength.SVG_LENGTHTYPE_PERCENTAGE===2&&SVGLength.SVG_LENGTHTYPE_EMS===3&&SVGLength.SVG_LENGTHTYPE_EXS===4&&SVGLength.SVG_LENGTHTYPE_PX===5&&SVGLength.SVG_LENGTHTYPE_CM===6&&SVGLength.SVG_LENGTHTYPE_MM===7&&SVGLength.SVG_LENGTHTYPE_IN===8&&SVGLength.SVG_LENGTHTYPE_PT===9&&SVGLength.SVG_LENGTHTYPE_PC===10" },
	{ "illegal native length constructor", "(function(){try{new SVGLength();}catch(e){return e instanceof TypeError;}return false;})()" },
	{ "illegal animated native constructor", "(function(){try{new SVGAnimatedLength();}catch(e){return e instanceof TypeError;}return false;})()" },
	{ "native live external mutation", "r.setAttribute('width','200');base.value===200&&anim.value===200&&r.width===w" },
	{ "native px scalar", "r.setAttribute('width','12px');base.value===12&&base.unitType===5&&base.valueAsString==='12px'" },
	{ "absolute inches conversion", "r.setAttribute('width','1in');base.value===96&&base.valueInSpecifiedUnits===1&&base.unitType===8" },
	{ "absolute centimeters conversion", "r.setAttribute('width','2.54cm');Math.abs(base.value-96)<0.0001&&base.unitType===6" },
	{ "absolute millimeters conversion", "r.setAttribute('width','25.4mm');Math.abs(base.value-96)<0.0001&&base.unitType===7" },
	{ "absolute points conversion", "r.setAttribute('width','72pt');base.value===96&&base.unitType===9" },
	{ "absolute picas conversion", "r.setAttribute('width','6pc');base.value===96&&base.unitType===10" },
	{ "actual finite exponent grammar", "r.setAttribute('width','  +1.25e2PX  ');base.value===125&&base.valueAsString==='125px'" },
	{ "native signed fraction", "r.setAttribute('width','-.5px');base.value===-0.5&&base.valueAsString==='-0.5px'" },
	{ "native finite positive exponent", "r.setAttribute('width','1e+2');base.value===100" },
	{ "specified setter preserves current unit", "r.setAttribute('width','1in');base.valueInSpecifiedUnits=2;base.value===192&&anim.value===192&&r.getAttribute('width')==='2in'" },
	{ "user unit setter resets to number", "base.value=12;base.value===12&&base.unitType===1&&r.getAttribute('width')==='12'" },
	{ "string setter serializes native scalar", "base.valueAsString='  2.5e1PX  ';base.value===25&&r.getAttribute('width')==='25px'" },
	{ "relative percentage parsed without guessed scale", "r.setAttribute('width','25%');base.unitType===2&&base.valueInSpecifiedUnits===25&&base.valueAsString==='25%'" },
	{ "relative user conversion explicitly unsupported", "(function(){try{base.value;}catch(e){return e instanceof DOMException&&e.name==='NotSupportedError';}return false;})()" },
	{ "relative em native reflection", "base.valueAsString='1em';base.unitType===3&&base.valueInSpecifiedUnits===1" },
	{ "relative specified setter preserves actual unit", "base.valueInSpecifiedUnits=2;r.getAttribute('width')==='2em'&&anim.valueInSpecifiedUnits===2" },
	{ "relative ex native reflection", "base.valueAsString='3ex';base.unitType===4&&base.valueInSpecifiedUnits===3" },
	{ "invalid string refuses partial mutation", "(function(){try{base.valueAsString='12junk';}catch(e){return e.name==='SyntaxError'&&r.getAttribute('width')==='3ex';}return false;})()" },
	{ "nonfinite numeric setter rejected", "(function(){try{base.value=Infinity;}catch(e){return e instanceof TypeError&&r.getAttribute('width')==='3ex';}return false;})()" },
	{ "nonfinite parsed token invalid initial", "r.setAttribute('width','Infinity');base.value===0&&base.unitType===1&&r.getAttribute('width')==='Infinity'" },
	{ "hex token invalid initial", "r.setAttribute('width','0x10');base.value===0" },
	{ "incomplete exponent invalid initial", "r.setAttribute('width','1e+');base.value===0" },
	{ "unit whitespace invalid initial", "r.setAttribute('width','1 px');base.value===0" },
	{ "overflow scalar invalid initial", "r.setAttribute('width','1e999');base.value===0" },
	{ "missing attribute initial scalar", "r.removeAttribute('width');base.value===0&&base.valueAsString==='0'" },
	{ "readonly animated value", "base.value=7;(function(){try{anim.value=8;}catch(e){return e instanceof DOMException&&e.name==='NoModificationAllowedError'&&e.code===7&&base.value===7;}return false;})()" },
	{ "readonly animated specified value", "(function(){try{anim.valueInSpecifiedUnits=8;}catch(e){return e.name==='NoModificationAllowedError'&&base.value===7;}return false;})()" },
	{ "readonly animated invalid string precedes parsing", "(function(){try{anim.valueAsString='junk';}catch(e){return e.name==='NoModificationAllowedError'&&base.value===7;}return false;})()" },
	{ "readonly attributes retain identity", "(function(){'use strict';try{w.baseVal=anim;}catch(e){return e instanceof TypeError&&w.baseVal===base;}return false;})()" },
	{ "reentrant specified coercion reads current unit", "base.valueInSpecifiedUnits={valueOf:function(){r.setAttribute('width','2cm');return 3;}};r.getAttribute('width')==='3cm'&&base.unitType===6" },
	{ "reentrant string conversion native mutation", "base.valueAsString={toString:function(){r.setAttribute('width','20px');return '4in';}};base.value===384&&r.getAttribute('width')==='4in'" },
	{ "actual GC during scalar conversion", "base.value={valueOf:function(){forceCollection();return 11;}};base.value===11&&anim.value===11" },
	{ "independent cloned native cache", "var clone=r.cloneNode(false);var cloneWidth=clone.width;cloneWidth!==w&&cloneWidth.baseVal!==base&&cloneWidth.baseVal.value===11&&cloneWidth.baseVal instanceof SVGLength" },
	{ "clone mutation does not alter original", "cloneWidth.baseVal.value=13;base.value===11&&clone.getAttribute('width')==='13'" },
	{ "wrong native namespace", "(function(){var x=document.createElementNS(ns+'#','rect');try{Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(x);}catch(e){return e instanceof TypeError&&!(x instanceof SVGRectElement);}return false;})()" },
	{ "wrong local case", "(function(){var x=document.createElementNS(ns,'Rect');try{Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(x);}catch(e){return e instanceof TypeError&&!(x instanceof SVGRectElement);}return false;})()" },
	{ "forged rect prototype rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGRectElement.prototype,'width').get.call(Object.create(SVGRectElement.prototype));}catch(e){return e instanceof TypeError;}return false;})()" },
	{ "forged length prototype rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(Object.create(SVGLength.prototype));}catch(e){return e instanceof TypeError;}return false;})()" },
	{ "wrong native private interface rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(w);}catch(e){return e instanceof TypeError;}return false;})()" },
	{ "wrong animated role rejected", "(function(){try{Object.getOwnPropertyDescriptor(SVGAnimatedLength.prototype,'baseVal').get.call(base);}catch(e){return e instanceof TypeError;}return false;})()" },
	{ "borrowed getter accepts genuine native scalar", "Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(base)===11" },
	{ "public expando does not replace native value", "Object.defineProperty(cloneWidth.baseVal,'value',{value:99});clone.getAttribute('width')==='13'&&Object.getOwnPropertyDescriptor(SVGLength.prototype,'value').get.call(cloneWidth.baseVal)===13" },
	{ "detached live native width", "document.getElementById('root').removeChild(r);base.value===11&&anim.value===11" },
	{ "reconnected identical native handles", "document.getElementById('root').appendChild(r);r.width===w&&w.baseVal===base&&base.value===11" }
};

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

	/* Ordinary files supply actual SVG MIME and resource parsing without fixture server hooks. */
	created = mkdtemp(directory);
	if (created == NULL)
		return 2;
	wb_buffer_init(&path);
	error = wb_buffer_append_string(&path, directory);
	if (error == 0)
		error = wb_buffer_append_string(&path, "/text.svg");
	if (error != 0)
		return 2;
	file = fopen(wb_buffer_string(&path), "wb");
	if (file == NULL)
		return 2;
	xml = "<svg xmlns='http://www.w3.org/2000/svg'><rect id='child' width='50'/></svg>";
	xml_length = strlen(xml);
	written = fwrite(xml, 1U, xml_length, file);
	closed = fclose(file);
	if (written != xml_length || closed != 0)
		return 2;

	/* Create and load the actual parent before referencing its body or SVG nodes. */
	page = NULL;
	svg_stack_base = __builtin_frame_address(0);
	error = page_create(&page, svg_stack_base);
	if (error == 0) {
		fonts.sans = "userland/desktop/fonts/Inter.ttf";
		fonts.mono = "userland/desktop/fonts/JetBrainsMono-Regular.ttf";
		fonts.fallback = "userland/desktop/fonts/DroidSansFallbackFull.ttf";
		page_set_console(page, svg_console, NULL);
		page_set_viewport(page, 800, 600);
		page_set_fonts(page, &fonts);
		error = page_open_fonts(page, &fonts);
	}

	/* The parent contains a genuine parser-created canonical SVG rect subtree. */
	html = "<!doctype html><body><svg id='root'><rect id='subject' width='100'/></svg><script>var ns='http://www.w3.org/2000/svg';var r=document.getElementById('subject');</script></body>";
	if (error == 0)
		error = page_load_bytes(page, (const unsigned char *)html, strlen(html), wb_buffer_string(&path));

	/* A fixture host operation requests real precise-only collection during ordinary argument coercion. */
	if (error == 0) {
		collect_key = vm_key_from_ascii(page->heap, "forceCollection");
		collect = vm_function_create_native(page->realm, "forceCollection", 0, svg_collect);
		if (collect == NULL) {
			error = ENOMEM;
		} else {
			error = vm_object_define(page->heap, page->realm->global, collect_key, vm_value_cell(collect), VM_PROPERTY_DEFAULT);
		}
	}

	/* Record each independent native scalar predicate after the preceding live mutations. */
	for (index = 0; index < sizeof(svg_length_cases) / sizeof(svg_length_cases[0]); index++) {
		/* Record each actual native predicate before later live tree edits. */
		if (error != 0)
			break;
		error = svg_assert(page, svg_length_cases[index].source, svg_length_cases[index].name);
	}

	/* A real SVG child also exercises independently owned scalar wrappers and prototype identity. */
	if (error == 0)
		error = svg_child(page);
	if (error == 0)
		error = svg_retention(page);
	svg_check(exceptions == 0, "zero actual interpreter console exceptions");

	/* Teardown owns all native contexts before deleting ordinary temporary source files. */
	page_destroy(page);
	unlink(wb_buffer_string(&path));
	rmdir(directory);
	wb_buffer_release(&path);
	printed = printf("native SVG length: %u/%u passed; error=%d\n", checks - failures, checks, error);
	if (printed < 0 || error != 0)
		return 2;
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
	/* Retain the original failing observation at its actual source. */
	checks++;
	if (!condition) {
		failures++;
		fprintf(stderr, "FAIL %s\n", name);
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
	UNUSED_PARAMETER(context);

	/* Error output remains visible under both sanitizer and ordinary execution. */
	if (level == BIND_CONSOLE_ERROR) {
		exceptions++;
		fprintf(stderr, "console: %.*s\n", (int)length, text);
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

	/* Convert fixture-owned source before invoking the native binding. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error == 0)
		error = js_run_script(page->realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (error != 0) {
		/* Genuine exceptions remain diagnostic evidence rather than false predicate success. */
		wb_buffer_init(&message);
		rendered = js_exception_text(page->realm, page->realm->exception, &message);
		if (rendered == 0)
			fprintf(stderr, "exception: %s\n", wb_buffer_string(&message));
		wb_buffer_release(&message);
		return error;
	}

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

	/* Ordinary script inspects actual native identity and current DOM data. */
	error = svg_script(page, source, &answer);
	if (error != 0) {
		fprintf(stderr, "script failure %d: %s\n", error, name);
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
	if (error == 0)
		error = page_frames_checkpoint(page);
	if (error == 0)
		error = svg_assert(page, "var cd=childFrame.contentDocument;var cw=childFrame.contentWindow;var cRect=cd.getElementById('child');var ct=cRect.width.baseVal;cRect instanceof cw.SVGRectElement&&ct instanceof cw.SVGLength&&!(ct instanceof SVGLength)&&ct.value===50;", "actual child native owner prototypes and live scalar");
	if (error == 0)
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
	document = node->document;
	window = document->view;

	/* The sole saved scalar wrapper, rather than its public Document alias, owns the retained graph. */
	error = svg_script(page, "ct;", &wrapper);
	if (error != 0)
		return error;
	held = vm_value_as_cell(wrapper);
	address = (uintptr_t)&document->node.cell;
	error = vm_heap_add_root(page->heap, &held);
	if (error != 0)
		return error;

	/* Remove all public aliases before precise-only collection of the saved native node. */
	error = svg_assert(page, "document.body.removeChild(childFrame);childFrame=null;cd=null;cw=null;ct=null;cRect=null;true;", "actual loaded SVG child retired before sole native root");
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, address);
	svg_check(found != NULL, "sole native SVG length retains actual owning Document and creator realm");
	if (error == 0) {
		/* The retained wrapper resolves the actual getter after collection without a primary prototype alias. */
		key = vm_key_from_ascii(page->heap, "value");
		error = vm_get(window->realm, wrapper, key, &answer);
		if (error == 0) {
			numeric = vm_value_is_number(answer);
			svg_check(numeric, "saved-only actual native SVG length getter survives collection");
			if (numeric)
				svg_check(vm_value_as_number(answer) == 50, "retired actual SVG scalar still reflects the real owner attribute");
		}
	}

	/* No temporary operation or native interface table permanently roots the retained child graph. */
	held = NULL;
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, address);
	svg_check(found == NULL, "last native SVG length release collects actual child Document and managed realm");
	vm_heap_remove_root(page->heap, &held);
	vm_heap_set_stack_base(page->heap, svg_stack_base);
	if (error != 0)
		return error;

	/* Succeeded: saved SVG length accessors and their owning native graph have collectible precise lifetimes. */
	return 0;
}
