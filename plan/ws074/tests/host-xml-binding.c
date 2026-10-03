/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies genuine XML character wrappers, existing DOM operations and actual collector ownership. */

#include "bind/internal.h"
#include "page/page.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One independent observable contract executes against actual native nodes in the production VM. */
struct xml_binding_probe {
	const char *name;
	const char *source;
};

/* Ordinary allocating cells force collection inside the unchanged wrapper creation path. */
static const struct vm_cell_type pressure_type = { "xml-binding-pressure", NULL, NULL };
/* Sequential contracts preserve only the initial real XML nodes between independent function invocations. */
static const struct xml_binding_probe probes[] = {
	{ "native XML character prototypes", "cd instanceof CDATASection && cd instanceof Text && cd instanceof CharacterData && pi instanceof ProcessingInstruction && pi instanceof CharacterData && !(pi instanceof Text)" },
	{ "actual node type and name", "cd.nodeType===4 && cd.nodeName==='#cdata-section' && pi.nodeType===7 && pi.nodeName==='probe-target' && pi.target==='probe-target'" },
	{ "actual character data properties", "cd.data==='abcd' && cd.length===4 && cd.nodeValue==='abcd' && cd.textContent==='abcd' && pi.data==='wxyz' && pi.nodeValue==='wxyz' && pi.textContent==='wxyz'" },
	{ "immutable PI target native brand", "(function(){var get=Object.getOwnPropertyDescriptor(ProcessingInstruction.prototype,'target').get;try{get.call({target:'fake'});}catch(e){return e instanceof TypeError;}return false;})()"},
    {"CharacterData rejects prototype lookalike", "(function(){try{CharacterData.prototype.appendData.call(Object.create(CDATASection.prototype),'x');}catch(e){return e instanceof TypeError;}return false;})()"},
    {"native character clones", "(function(){var a=cd.cloneNode(),b=pi.cloneNode();return a.nodeType===4&&a.data==='abcd'&&b.nodeType===7&&b.target==='probe-target'&&b.data==='wxyz'&&a.ownerDocument===x&&b.ownerDocument===x;})()"},
    {"PI Document child and CDATA element child", "(function(){x.insertBefore(pi,x.documentElement);x.documentElement.appendChild(cd);return x.firstChild===pi&&x.documentElement.firstChild===cd&&x.documentElement.textContent==='abcd';})()"},
    {"CDATA Document hierarchy rejection", "(function(){try{x.appendChild(cd.cloneNode());}catch(e){return e.name==='HierarchyRequestError';}return false;})()"},
    {"CDATA fragment Document hierarchy rejection", "(function(){var f=x.createDocumentFragment();f.appendChild(cd.cloneNode());try{x.appendChild(f);}catch(e){return e.name==='HierarchyRequestError'&&f.firstChild.nodeType===4;}return false;})()"},
    {"PI omitted from descendant text", "(function(){var a=x.createElement('a');a.appendChild(pi.cloneNode());a.appendChild(cd.cloneNode());return a.textContent==='abcd';})()"},
    {"PI native Range length and clone slice", "(function(){var r=x.createRange();r.setStart(pi,1);r.setEnd(pi,3);var f=r.cloneContents();return f.firstChild.nodeType===7&&f.firstChild.target==='probe-target'&&f.firstChild.data==='xy'&&r.toString()==='';})()"},
    {"PI native Range bounds", "(function(){var r=x.createRange();try{r.setStart(pi,5);}catch(e){return e.name==='IndexSizeError';}return false;})()"},
    {"PI native Range extraction", "(function(){var p=pi.cloneNode(),r=x.createRange();r.setStart(p,1);r.setEnd(p,3);var f=r.extractContents();return p.data==='wz'&&f.firstChild.nodeType===7&&f.firstChild.target==='probe-target'&&f.firstChild.data==='xy'&&r.collapsed;})()"},
    {"PI native Range deletion", "(function(){var p=pi.cloneNode(),r=x.createRange();r.setStart(p,1);r.setEnd(p,3);r.deleteContents();return p.data==='wz'&&p.target==='probe-target'&&r.collapsed;})()"},
    {"PI Range insertion refusal", "(function(){var r=x.createRange();r.setStart(pi,1);r.collapse(true);try{r.insertNode(x.createElement('q'));}catch(e){return e.name==='HierarchyRequestError'&&pi.data==='wxyz';}return false;})()"},
    {"PI partial Range surround refusal", "(function(){var a=x.createElement('a'),p=pi.cloneNode();a.appendChild(p);a.appendChild(x.createTextNode('t'));var r=x.createRange();r.setStart(p,1);r.setEnd(a,2);try{r.surroundContents(x.createElement('q'));}catch(e){return e.name==='InvalidStateError'&&p.data==='wxyz';}return false;})()"},
    {"CDATA Range string and slice kind", "(function(){var r=x.createRange();r.setStart(cd,1);r.setEnd(cd,3);var f=r.cloneContents();return r.toString()==='bc'&&f.firstChild.nodeType===4&&f.firstChild.data==='bc';})()"},
    {"CDATA Text split original subtype and ordinary suffix", "(function(){var a=x.createElement('a'),c=cd.cloneNode(),r=x.createRange();a.appendChild(c);r.setStart(c,3);r.setEnd(c,4);var t=c.splitText(2);return c.nodeType===4&&c.data==='ab'&&t.nodeType===3&&t.data==='cd'&&t.previousSibling===c&&r.startContainer===t&&r.startOffset===1&&r.endOffset===2;})()"},
    {"PI rejected by Text split native brand", "(function(){try{Text.prototype.splitText.call(pi,2);}catch(e){return e instanceof TypeError;}return false;})()"},
    {"CDATA Range insertion uses Text split", "(function(){var a=x.createElement('a'),c=cd.cloneNode(),r=x.createRange();a.appendChild(c);r.setStart(c,2);r.collapse(true);var q=x.createElement('q');r.insertNode(q);return a.firstChild===c&&c.data==='ab'&&c.nextSibling===q&&q.nextSibling.nodeType===3&&q.nextSibling.data==='cd';})()"},
    {"CDATA partially contained Range surround", "(function(){var a=x.createElement('a'),c=cd.cloneNode(),r=x.createRange();a.appendChild(c);a.appendChild(x.createTextNode('tail'));r.setStart(c,1);r.setEnd(a,2);var q=x.createElement('q');r.surroundContents(q);return q.textContent==='bcdtail'&&a.textContent==='abcdtail';})()"},
    {"CDATA traversal mask and element collection", "(function(){var w=x.createTreeWalker(x.documentElement,NodeFilter.SHOW_CDATA_SECTION);return w.nextNode()===cd&&x.documentElement.children.length===0;})()"},
    {"PI traversal mask", "(function(){var w=x.createTreeWalker(x,NodeFilter.SHOW_PROCESSING_INSTRUCTION);return w.nextNode()===pi;})()"},
    {"CDATA and PI CharacterData append", "(function(){var c=cd.cloneNode(),p=pi.cloneNode();c.appendData('!');p.appendData('?');return c.data==='abcd!'&&p.data==='wxyz?'&&p.target==='probe-target';})()"},
    {"CDATA and PI Node value setters", "(function(){var c=cd.cloneNode(),p=pi.cloneNode();c.nodeValue='u';p.textContent=null;return c.data==='u'&&p.data===''&&p.target==='probe-target';})()"},
    {"HTML serialization actual XML character kinds", "(function(){var a=document.createElement('div'),c=cd.cloneNode(),p=pi.cloneNode();c.data='<&';a.appendChild(c);a.appendChild(p);return a.innerHTML==='&lt;&amp;<?probe-target wxyz?>';})()"},
    {"actual CDATA text layout", "(function(){var a=document.createElement('div'),c=cd.cloneNode();a.style.cssText='display:block;width:120px;font-size:16px';a.appendChild(c);document.body.appendChild(a);var h=a.getBoundingClientRect().height;a.remove();return h>0;})()"},
    {"actual CDATA inline CSS source", "(function(){var s=document.createElement('style'),c=cd.cloneNode();c.data='img{height:19px}';s.appendChild(c);document.body.appendChild(s);var a=s.sheet.cssRules.length;c.data='img{height:23px}';var b=s.sheet.cssRules.length;s.remove();return a===1&&b===1;})()"},
    {"actual CDATA inline script text", "(function(){var s=document.createElement('script'),c=cd.cloneNode();c.data='window.xmlInlineValue=23';s.appendChild(c);document.body.appendChild(s);s.remove();return window.xmlInlineValue===23;})()"}};
/* Count independent contracts and actual native collector observations. */
static unsigned checks;
/* Preserve every failure through normal resource teardown. */
static unsigned failures;

static void xml_binding_check(int condition, const char *name);
static int xml_binding_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int xml_binding_case(struct page *page, struct vm_cell **held);
static int xml_binding_publish(struct page *page, const char *name, vm_value value);

/*
 * Runs genuine XML native bindings over an ordinary loaded Page and its real fonts.
 */
int
main(
	void)
{
	struct page *page;
	struct text_font_paths paths;
	struct vm_cell *held;
	static const unsigned char html[] = "<!doctype html><html><body></body></html>";
	int status;
	int printed;

	/* Real layout metrics and Page initialization use the production embedding. */
	paths.sans = "userland/desktop/fonts/Inter.ttf";
	paths.mono = "userland/desktop/fonts/JetBrainsMono-Regular.ttf";
	paths.fallback = "userland/desktop/fonts/DroidSansFallbackFull.ttf";
	status = page_create(&page, __builtin_frame_address(0));
	if (status != 0)
		return 2;
	page_set_fonts(page, &paths);
	page_set_viewport(page, 800, 600);
	status = page_load_html(page, html, sizeof(html) - 1U);
	if (status != 0) {
		page_destroy(page);
		return 2;
	}

	/* The native caller retains only a nullable returned wrapper, never the unwrapped factory input. */
	held = NULL;
	status = vm_heap_add_root(page->heap, &held);
	if (status != 0) {
		page_destroy(page);
		return 2;
	}

	/* All early verification returns pass through root and Page teardown here. */
	status = xml_binding_case(page, &held);
	if (status != 0) {
		vm_heap_remove_root(page->heap, &held);
		vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
		page_destroy(page);
		return 2;
	}

	/* Release the completed fixture root before Page destruction. */
	vm_heap_remove_root(page->heap, &held);
	vm_heap_set_stack_base(page->heap, __builtin_frame_address(0));
	page_destroy(page);

	/* Publish every recorded XML contract after native teardown. */
	printed = printf("native XML bindings: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Reject every failed native observation independently of fixture setup. */
	if (failures != 0)
		return 1;

	/* Succeeded: real wrapper/category and collector contracts were verified. */
	return 0;
}

/* Records one independent native observable contract. */
static void
xml_binding_check(
	int condition,
	const char *name)
{
	int printed;

	/* Retain failures while later ordinary independent scripts still execute. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: this independent observation contributes to the final outcome. */
	return;
}

/* Executes ordinary script text through the production VM parser and interpreter. */
static int
xml_binding_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Source is fixture-owned C storage, converted before any actual script invocation. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Execute ordinary script only after conversion supplies complete input. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release the borrowed source after checked script execution. */
	wb_units_release(&units);

	/* Succeeded: the ordinary completion value is available. */
	return 0;
}

/* Publishes an already rooted genuine wrapper to an ordinary script global. */
static int
xml_binding_publish(
	struct page *page,
	const char *name,
	vm_value value)
{
	vm_value key;
	int status;

	/* Key allocation cannot collect the caller's returned wrapper root. */
	key = vm_key_from_ascii(page->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;

	/* Store the genuine wrapper in the actual VM global object. */
	status = vm_set(page->realm, vm_value_cell(page->realm->global), key, value, 0);
	if (status != 0)
		return status;

	/* Succeeded: the ordinary global now retains the genuine wrapper. */
	return 0;
}

/* Verifies actual XML interfaces and a caller-root-free native wrapper allocation. */
static int
xml_binding_case(
	struct page *page,
	struct vm_cell **held)
{
	struct dom_node *node;
	struct dom_node *pi;
	struct dom_node *cdata;
	struct dom_document *document;
	struct vm_string *target;
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	struct wb_units units;
	vm_value answer;
	vm_value wrapper;
	uintptr_t document_address;
	uintptr_t target_address;
	size_t index;
	int status;
	int truth;

	/* Genuine DOMImplementation creation supplies an actual XMLDocument and owner prototype snapshot. */
	status = xml_binding_script(page->realm, "var x=document.implementation.createDocument('urn:fixture','root');x", &answer);
	if (status != 0)
		return status;
	node = bind_node_of(answer);
	if (node == NULL || node->type != DOM_DOCUMENT)
		return EINVAL;

	/* Resolve the genuine XML Document owner before native character allocation. */
	document = node->document;

	/* Allocate the non-atom target independently of character-data input. */
	target = vm_string_from_utf8(page->heap, "probe-target", 12);
	if (target == NULL)
		return ENOMEM;

	/* Convert the fixture-owned character-data input before native publication. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)"wxyz", 4, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Construct the native character node from the checked borrowed units. */
	status = dom_pi_create(document, target, units.data, units.length, &pi);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release temporary input after native construction was checked. */
	wb_units_release(&units);

	/* Integer addresses observe collection without retaining native owners. */
	document_address = (uintptr_t)document;
	target_address = (uintptr_t)target;

	/* The detached native PI and target have no caller roots when wrapper allocation crosses real GC. */
	vm_heap_set_stack_base(page->heap, NULL);
	pressure = vm_heap_alloc(page->heap, &pressure_type, 8U * 1024U * 1024U);
	if (pressure == NULL)
		return ENOMEM;
	vm_heap_stats(page->heap, &before);
	status = bind_wrap(NULL, pi, &wrapper);
	if (status != 0)
		return status;

	/* Observe completed native allocation before publishing the caller wrapper root. */
	vm_heap_stats(page->heap, &after);
	*held = vm_value_as_cell(wrapper);
	xml_binding_check(after.collections > before.collections, "callee wrapper allocation caused actual threshold collection");
	status = xml_binding_publish(page, "pi", wrapper);
	if (status != 0)
		return status;

	/* The second real native kind enters script through the same owner snapshot and native wrapper path. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)"abcd", 4, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Construct the native character node from the checked borrowed units. */
	status = dom_cdata_create(document, units.data, units.length, &cdata);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release temporary input after native construction was checked. */
	wb_units_release(&units);

	/* Publish only a successfully created native CDATA wrapper. */
	status = bind_wrap(NULL, cdata, &wrapper);
	if (status != 0)
		return status;
	*held = vm_value_as_cell(wrapper);
	status = xml_binding_publish(page, "cd", wrapper);
	if (status != 0)
		return status;
	*held = &pi->wrapper->cell;

	/* Every independent function invocation operates genuine initial nodes, creating disposable native copies. */
	for (index = 0; index < sizeof(probes) / sizeof(probes[0]); index++) {
		status = xml_binding_script(page->realm, probes[index].source, &answer);
		if (status != 0)
			return status;
		truth = vm_to_boolean(answer);
		xml_binding_check(truth, probes[index].name);
	}

	/* A saved native wrapper alone must retain the XML Document and target after all script globals clear. */
	status = xml_binding_script(page->realm, "x=null;pi=null;cd=null", &answer);
	if (status != 0)
		return status;
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, document_address);
	xml_binding_check(found != NULL, "saved wrapper alone retains actual XMLDocument");
	found = vm_heap_find_cell(page->heap, target_address);
	xml_binding_check(found != NULL, "saved wrapper alone retains non-atom PI target");

	/* Native node/Document cycles disappear after their last real wrapper root is removed. */
	*held = NULL;
	vm_heap_collect(page->heap);
	found = vm_heap_find_cell(page->heap, target_address);
	xml_binding_check(found == NULL, "last wrapper release collects native PI target");
	found = vm_heap_find_cell(page->heap, document_address);
	xml_binding_check(found == NULL, "last wrapper release collects XMLDocument");

	/* Succeeded: the owning caller now releases registration and ordinary Page resources. */
	return 0;
}
