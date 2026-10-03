/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercises native SVG text identity, live addressable UTF16 counts and collectible actual owners. */

#include "page/page.h"
#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* One predicate inspects real DOM and binding behavior after the preceding ordinary mutations. */
struct svg_text_case {
	const char *name;
	const char *source;
};

/* Fixed ordinary source cases do not replace production functions, constructors or native data. */
static const struct svg_text_case svg_text_cases[] = {
    {"actual native hierarchy", "t instanceof SVGTextElement&&t instanceof SVGTextPositioningElement&&t instanceof SVGTextContentElement&&t instanceof SVGGraphicsElement&&t instanceof SVGElement&&t instanceof Element&&!(t instanceof HTMLElement)"},
    {"actual original three units", "t.getNumberOfChars()===3"},
    {"actual interface constants", "SVGTextContentElement.LENGTHADJUST_UNKNOWN===0&&SVGTextContentElement.LENGTHADJUST_SPACING===1&&SVGTextContentElement.LENGTHADJUST_SPACINGANDGLYPHS===2"},
    {"illegal native constructor", "(function(){try{new SVGTextElement();}catch(e){return e instanceof TypeError;}return false;})()"},
    {"actual live mutation", "t.firstChild.data='abcd';t.getNumberOfChars()===4"},
    {"supplementary UTF16 pair counts two", "t.textContent='A\\uD83D\\uDE00B';t.getNumberOfChars()===4"},
    {"combining characters independent of glyph", "t.textContent='a\\u0301';t.getNumberOfChars()===2"},
    {"lone surrogate native storage", "t.textContent='\\uD800';t.getNumberOfChars()===1"},
    {"normal collapse and trimmed flow edges", "t.textContent='  a \\t\\nb  ';t.getNumberOfChars()===3"},
    {"NBSP remains addressable", "t.textContent='a\\u00a0\\u00a0b';t.getNumberOfChars()===4"},
    {"nowrap collapse", "t.setAttribute('style','white-space:nowrap');t.textContent='  a   b  ';t.getNumberOfChars()===3"},
    {"pre preserves spaces and line breaks", "t.setAttribute('style','white-space:pre');t.textContent='  a \\n b  ';t.getNumberOfChars()===9"},
    {"pre wrap preserves code units", "t.setAttribute('style','white-space:pre-wrap');t.textContent='a  b';t.getNumberOfChars()===4"},
    {"native CRLF one preserved segment break", "t.setAttribute('style','white-space:pre');t.textContent='a\\r\\nb';t.getNumberOfChars()===3"},
    {"pre line keeps break and trims line spaces", "t.setAttribute('style','white-space:pre-line');t.textContent=' a  \\n  b ';t.getNumberOfChars()===3"},
    {"adjacent Text whitespace continuity", "t.removeAttribute('style');t.textContent='a ';t.appendChild(document.createTextNode('  b '));t.getNumberOfChars()===3"},
    {"native nested tspan identity and flow", "var span=document.createElementNS(ns,'tspan');span.textContent=' c ';t.appendChild(span);t.getNumberOfChars()===5&&span instanceof SVGTSpanElement&&span instanceof SVGTextPositioningElement&&span.getNumberOfChars()===1"},
    {"nested inherited preserved whitespace", "t.setAttribute('style','white-space:pre');t.getNumberOfChars()===9&&span.getNumberOfChars()===3"},
    {"nested actual display none skipped", "t.removeAttribute('style');span.setAttribute('style','display:none');t.getNumberOfChars()===3&&span.getNumberOfChars()===0"},
    {"visibility hidden keeps addressable units", "span.setAttribute('style','visibility:hidden');t.getNumberOfChars()===5&&span.getNumberOfChars()===1"},
    {"actual display none ancestor excludes", "t.parentNode.setAttribute('style','display:none');t.getNumberOfChars()===0&&span.getNumberOfChars()===0"},
    {"live display restoration", "t.parentNode.removeAttribute('style');t.getNumberOfChars()===5"},
    {"native comments excluded", "t.appendChild(document.createComment('not text'));t.getNumberOfChars()===5"},
    {"native textPath inherited operation", "var path=document.createElementNS(ns,'textPath');path.textContent=' q ';t.appendChild(path);path instanceof SVGTextPathElement&&path instanceof SVGTextContentElement&&!(path instanceof SVGTextPositioningElement)&&path.getNumberOfChars()===1&&t.getNumberOfChars()===7"},
    {"borrowed actual text subtype", "SVGTextContentElement.prototype.getNumberOfChars.call(span)===1"},
    {"wrong namespace brand", "(function(){var x=document.createElementNS(ns+'#','text');try{SVGTextContentElement.prototype.getNumberOfChars.call(x);}catch(e){return e instanceof TypeError&&!(x instanceof SVGElement);}return false;})()"},
    {"wrong local case brand", "(function(){var x=document.createElementNS(ns,'Text');try{SVGTextContentElement.prototype.getNumberOfChars.call(x);}catch(e){return e instanceof TypeError&&x instanceof SVGElement&&!(x instanceof SVGTextElement);}return false;})()"},
    {"unrelated native SVG brand", "(function(){var x=document.createElementNS(ns,'rect');try{SVGTextContentElement.prototype.getNumberOfChars.call(x);}catch(e){return e instanceof TypeError;}return false;})()"},
    {"forged prototype brand", "(function(){try{SVGTextContentElement.prototype.getNumberOfChars.call(Object.create(SVGTextElement.prototype));}catch(e){return e instanceof TypeError;}return false;})()"},
    {"public text expando cannot replace native data", "Object.defineProperty(t,'textContent',{value:'fake'});t.getNumberOfChars()===7"},
    {"detached node no rendered count", "var saved=t;document.getElementById('root').removeChild(t);saved.getNumberOfChars()===0"},
    {"reconnected live node", "document.getElementById('root').appendChild(saved);saved.getNumberOfChars()===7"}};

/* Predicate outcomes remain available until the complete fixture has been reported. */
static unsigned checks;
/* Preserve every failed predicate while cleanup and independent cases continue. */
static unsigned failures;
/* Genuine child or primary interpreter errors must remain visible. */
static unsigned exceptions;
/* The fixture restores its live embedding frame after explicit root-only observations. */
static const void *svg_stack_base;

static void svg_check(int condition, const char *name);
static void svg_console(void *context, int level, const char *text, size_t length);
static int svg_script(struct page *page, const char *source, vm_value *answer);
static int svg_assert(struct page *page, const char *source, const char *name);
static int svg_child(struct page *page);
static int svg_retention(struct page *page);

/*
 * Tests actual SVG text classes and addressable counts through a genuine loaded Page.
 */
int
main(
	void)
{
	struct page *page;
	struct text_font_paths fonts;
	struct wb_buffer path;
	FILE *file;
	char directory[] = "/tmp/ws074-svg-text-XXXXXX";
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
	xml = "<svg xmlns='http://www.w3.org/2000/svg'><text id='child'>a<![CDATA[  b]]><?ignored no?><!--excluded--><tspan> c</tspan></text></svg>";
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

	/* The parent contains a genuine parser-created canonical SVG text subtree. */
	html = "<!doctype html><body><svg id='root'><text id='subject'>abc</text></svg><script>var ns='http://www.w3.org/2000/svg';var t=document.getElementById('subject');</script></body>";
	error = page_load_bytes(page, (const unsigned char *)html, strlen(html), wb_buffer_string(&path));
	if (error != 0)
		goto cleanup;

	/* Record each native text predicate after its preceding live mutation. */
	for (index = 0; index < sizeof(svg_text_cases) / sizeof(svg_text_cases[0]); index++) {
		/* Record each actual native predicate before later live tree edits. */
		error = svg_assert(page, svg_text_cases[index].source, svg_text_cases[index].name);
		if (error != 0)
			goto cleanup;
	}

	/* A real SVG child also exercises CDATA, PI exclusion and its independent actual CSS owner. */
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
	printed = printf("native SVG text: %u/%u passed; error=%d\n", checks - failures, checks, error);
	if (printed < 0 || error != 0)
		return 2;

	/* Refuse a fixture with any failed native observation. */
	if (failures != 0)
		return 1;

	/* Succeeded: every native text and actual ownership observation passed. */
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

/* Activates a real local SVG response and exercises its actual native Text and CDATA graph. */
static int
svg_child(
	struct page *page)
{
	int error;

	/* The source is an ordinary local SVG file with actual response metadata and strict XML parsing. */
	error = svg_assert(page, "var childFrame=document.createElement('iframe');childFrame.src='text.svg';document.body.appendChild(childFrame);true;", "actual SVG child source connected");
	if (error != 0)
		return error;

	/* Activate the connected child through the ordinary frame checkpoint. */
	error = page_frames_checkpoint(page);
	if (error != 0)
		return error;

	/* Observe independently owned child classes and their actual native data. */
	error = svg_assert(page, "var cd=childFrame.contentDocument;var cw=childFrame.contentWindow;var ct=cd.getElementById('child');ct instanceof cw.SVGTextElement&&ct instanceof cw.SVGTextContentElement&&!(ct instanceof SVGTextElement)&&ct.getNumberOfChars()===5;", "actual independently owned SVG child classes and Text CDATA flow");
	if (error != 0)
		return error;

	/* Observe preserved child whitespace, CDATA identity and excluded processing instructions. */
	error = svg_assert(page, "ct.setAttribute('style','white-space:pre');ct.getNumberOfChars()===6&&ct.childNodes[1].nodeType===4&&ct.childNodes[2].nodeType===7;", "actual child preserved whitespace CDATA and PI exclusion");
	if (error != 0)
		return error;

	/* Observe the child native interface through the original borrowed primary entry point. */
	error = svg_assert(page, "SVGTextContentElement.prototype.getNumberOfChars.call(ct)===6;", "borrowed primary method uses actual child's CSS context");
	if (error != 0)
		return error;

	/* Succeeded: XML child data was counted through its actual owning managed realm. */
	return 0;
}

/* Retains a sole native SVG text node after retirement and verifies its final graph release. */
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
	vm_value method;
	vm_value key;
	uintptr_t address;
	int error;
	int numeric;

	/* Extract the real native child rather than copying its public interface or Document. */
	error = svg_script(page, "ct;", &answer);
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

	/* The genuine native node receives the original sole saved root. */
	held = &node->cell;
	address = (uintptr_t)&document->node.cell;
	error = vm_heap_add_root(page->heap, &held);
	if (error != 0)
		return error;

	/* Remove all public aliases before precise-only collection of the saved native node. */
	error = svg_assert(page, "document.body.removeChild(childFrame);childFrame=null;cd=null;cw=null;ct=null;true;", "actual loaded SVG child retired before sole native root");
	if (error != 0)
		goto cleanup;

	/* Observe genuine collection after all public child aliases have been removed. */
	vm_heap_set_stack_base(page->heap, NULL);
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, address);
	svg_check(found != NULL, "sole native SVG text retains actual owning Document and creator realm");
	if (found == NULL) {
		error = EINVAL;
		goto cleanup;
	}

	/* The retained wrapper resolves the actual method after collection without a primary prototype alias. */
	key = vm_key_from_ascii(page->heap, "getNumberOfChars");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Resolve the saved native interface through its actual retained realm. */
	error = vm_get(window->realm, vm_value_cell(node->wrapper), key, &method);
	if (error != 0)
		goto cleanup;

	/* Invoke the genuine saved native method after its checked property lookup. */
	error = vm_call(window->realm, method, vm_value_cell(node->wrapper), NULL, 0, &answer);
	if (error != 0)
		goto cleanup;

	/* Observe the successful saved native numeric completion. */
	numeric = vm_value_is_number(answer);
	svg_check(numeric, "saved-only actual native SVG text method survives collection");
	if (numeric)
		svg_check(vm_value_as_number(answer) == 0, "retired actual SVG owner has no rendered addressable count");

	/* No temporary operation or native interface table permanently roots the retained child graph. */
	held = NULL;
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, address);
	svg_check(found == NULL, "last native SVG text release collects actual child Document and managed realm");

cleanup:
	/* Release the acquired saved slot even when a native lookup or script fails. */
	vm_heap_remove_root(page->heap, &held);
	vm_heap_set_stack_base(page->heap, svg_stack_base);
	if (error != 0)
		return error;

	/* Succeeded: saved SVG methods and their owning native graph have collectible precise lifetimes. */
	return 0;
}
