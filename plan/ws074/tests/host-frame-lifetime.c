/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercises actual iframe ownership through the production binding and GC. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Finalizer observations belong to this synchronous host test, not the engine. */
static unsigned released;
/* Assertion totals remain live until main reports the complete test outcome. */
static unsigned checks;
/* Failed assertions accumulate without concealing subsequent independent cases. */
static unsigned failures;
/* All observed children share this binding cleanup hook, preserved until teardown. */
static void (*original_release)(void *context);

static void frame_check(int condition, const char *name);
static void frame_release(void *context);
static int frame_source(struct vm_realm *realm, const char *source, vm_value *answer);
static int frame_sample(struct vm_heap *heap, unsigned kind, struct vm_cell **root);
static int frame_retention(unsigned kind);
static int frame_failure(void);

/*
 * Verifies saved iframe graphs after removal and explicit parent destruction.
 */
int
main(
	void)
{
	unsigned kind;
	int error;
	int printed;

	/* Each sample selects a different surviving edge into a real child context. */
	for (kind = 0; kind < 3; kind++) {
		error = frame_retention(kind);
		if (error != 0)
			return 2;
	}

	/* A whole-heap teardown must also finalize a child retired by its parent once. */
	error = frame_retention(3);
	if (error != 0)
		return 2;

	/* A saved nested Window retains both managed ancestors until its last root dies. */
	error = frame_retention(4);
	if (error != 0)
		return 2;

	/* A saved computed declaration retains its child without a permanent CSS root. */
	error = frame_retention(5);
	if (error != 0)
		return 2;

	/* A primary-created UI event retains a child only through its explicit view field. */
	error = frame_retention(6);
	if (error != 0)
		return 2;

	/* XML nodes and sole implementation objects each retain their managed child owner. */
	error = frame_retention(7);
	if (error != 0)
		return 2;
	error = frame_retention(8);
	if (error != 0)
		return 2;

	/* A saved TreeWalker keeps its own child callbacks and current/root node graph. */
	error = frame_retention(9);
	if (error != 0)
		return 2;

	/* A saved child NodeIterator retains its callback and weak removal subscription graph. */
	error = frame_retention(10);
	if (error != 0)
		return 2;

	/* A saved collection keeps child nodes and native methods alive after primary teardown. */
	error = frame_retention(11);
	if (error != 0)
		return 2;

	/* A forms collection alone keeps its managed child and relevant methods alive. */
	error = frame_retention(12);
	if (error != 0)
		return 2;

	/* A sole controls collection retains its form's managed child after primary teardown. */
	error = frame_retention(13);
	if (error != 0)
		return 2;

	/* A sole duplicate NodeList also retains its name filter and managed child owner. */
	error = frame_retention(14);
	if (error != 0)
		return 2;

	/* Dirty input values keep their managed child native getter after primary teardown. */
	error = frame_retention(15);
	if (error != 0)
		return 2;

	/* A sole RadioNodeList retains checkedness and a child value getter after primary teardown. */
	error = frame_retention(16);
	if (error != 0)
		return 2;

	/* A sole child input can perform script click after the primary Window is destroyed. */
	error = frame_retention(17);
	if (error != 0)
		return 2;

	/* A sole SubmitEvent retains its child submitter and native getter after primary teardown. */
	error = frame_retention(18);
	if (error != 0)
		return 2;

	/* Sole table rows and row cells each retain their actual managed child independently. */
	error = frame_retention(19);
	if (error != 0)
		return 2;
	error = frame_retention(20);
	if (error != 0)
		return 2;

	/* A saved table performs native child creation after its former primary realm is gone. */
	error = frame_retention(21);
	if (error != 0)
		return 2;

	/* Adoption retains both actual current and native prototype creator managed children. */
	error = frame_retention(22);
	if (error != 0)
		return 2;

	/* A saved native table inserts a row after its former primary realm has been destroyed. */
	error = frame_retention(23);
	if (error != 0)
		return 2;

	/* A sole options collection retains its actual child after manual primary teardown. */
	error = frame_retention(24);
	if (error != 0)
		return 2;

	/* A saved option retains its owned selectedness after manual primary destruction. */
	error = frame_retention(25);
	if (error != 0)
		return 2;

	/* A sole native Range retains its child creator and actual endpoints after primary teardown. */
	error = frame_retention(26);
	if (error != 0)
		return 2;

	/* A sole adopted Range keeps original creator and actual endpoint managed owners independently. */
	error = frame_retention(27);
	if (error != 0)
		return 2;

	/* A sole cloned native Range retains its original creator independently of its source. */
	error = frame_retention(28);
	if (error != 0)
		return 2;

	/* A real heap cap exercises partial child creation without fault switches. */
	error = frame_failure();
	if (error != 0)
		return 2;

	/* Reports behavioral failures separately from construction errors. */
	printed = printf("frame lifetime checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any failed lifetime contract makes the regression fail. */
	if (failures != 0)
		return 1;

	/* Succeeded: every selected lifecycle and GC path survived. */
	return 0;
}

/* Forces a bounded child-creation failure while keeping the primary host live. */
static int
frame_failure(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct dom_element *element;
	struct vm_string *name;
	struct bind_window *window;
	struct bind_host host;
	vm_value wrapper;
	vm_value answer;
	vm_value key;
	unsigned index;
	int error;
	int getter_failed;
	int same;

	/* This supported heap limit leaves room for the primary and several children. */
	error = vm_heap_create(&heap, 1024U * 1024U);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	error = vm_realm_create(heap, &realm);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Installs a real primary binding before exhausting child allocation capacity. */
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return error;
	}

	/* Allocates the primary Document before installing its bound owner. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* The empty host registers no external operation that might outlive the heap. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return error;
	}

	/* Names are allocated before the repeated child-construction observation. */
	name = vm_atom_from_ascii(heap, "iframe");
	if (name == NULL) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* The property key is stable throughout repeated child creation. */
	key = vm_key_from_ascii(heap, "contentWindow");
	if (key == VM_VALUE_EMPTY) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Tracks the final attempted element independently of successful child count. */
	element = NULL;
	error = 0;
	getter_failed = 0;

	/* Connected siblings retain successful children until the fixed cap is reached. */
	for (index = 0; index < 256; index++) {
		element = dom_element_create(document, DOM_NS_HTML, name, NULL);
		if (element == NULL) {
			error = ENOMEM;
			break;
		}

		/* The real getter remains responsible for creating and publishing the child. */
		dom_append_child(&document->node, &element->node);
		error = bind_wrap(window, &element->node, &wrapper);
		if (error != 0)
			break;
		error = vm_get(realm, wrapper, key, &answer);
		if (error != 0) {
			getter_failed = 1;
			break;
		}
	}

	/* A failed construction must leave no half-installed child edge. */
	same = 0;
	if (error == ENOMEM && getter_failed) {
		if (element->child_context == NULL)
			same = 1;
	}

	/* Records the failure boundary independently of subsequent collection safety. */
	frame_check(same, "bounded child ENOMEM leaves no published partial context");

	/* Partial cells are collected while the complete primary ownership remains. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Succeeded: capped construction and cleanup used the default production paths. */
	return 0;
}

/* Records an independently named contract without hiding later observations. */
static void
frame_check(
	int condition,
	const char *name)
{
	/* Each observation contributes to the final failing exit status. */
	checks++;
	if (!condition) {
		failures++;
		fprintf(stderr, "FAIL %s\n", name);
	}

	/* Succeeded: the observation is recorded. */
	return;
}

/* Counts a child finalization and delegates all cleanup to its real owner hook. */
static void
frame_release(
	void *context)
{
	/* The original binding cleanup remains the only resource destructor. */
	released++;
	original_release(context);

	/* Succeeded: exactly one real owner cleanup was observed. */
	return;
}

/* Executes ordinary UTF-16 script through the production realm entry point. */
static int
frame_source(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int error;

	/* Converts the fixture independently of its parsing and execution. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* The script uses the same binding and collector as a browser tab. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the script completion is available. */
	return 0;
}

/* Returns only the selected saved cell after closing the explicit parent host. */
static int
frame_sample(
	struct vm_heap *heap,
	unsigned kind,
	struct vm_cell **root)
{
	struct vm_realm *realm;
	struct vm_realm *child;
	struct dom_document *document;
	struct dom_node *node;
	struct dom_element *element;
	struct bind_window *window;
	struct bind_host host;
	vm_value answer;
	int error;
	int same;

	/* The embedding owns the primary realm explicitly, as a real tab does. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* An empty primary host has no external asynchronous lifetime. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Installs the primary binding only after its Document exists. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Creates a connected iframe and obtains its genuine managed child. */
	error = frame_source(realm,
		"document.appendChild(document.createElement('html'));"
		"document.documentElement.appendChild(document.createElement('body'));"
		"var f=document.createElement('iframe');document.body.appendChild(f);"
		"var w=f.contentWindow;var d=f.contentDocument;f", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Observes cleanup through the actual published iframe child owner. */
	node = bind_node_of(answer);
	element = (struct dom_element *)node;
	child = (struct vm_realm *)element->child_context;
	original_release = child->host_release;
	child->host_release = frame_release;

	/* Observes the second managed owner in an actual nested context graph. */
	if (kind == 4) {
		error = frame_source(realm,
			"var nested=d.createElement('iframe');d.body.appendChild(nested);"
			"var saved=nested.contentWindow;nested", &answer);
		if (error != 0) {
			bind_window_destroy(window);
			vm_realm_destroy(realm);
			return error;
		}

		/* The nested host uses the same real binding cleanup as its parent. */
		node = bind_node_of(answer);
		element = (struct dom_element *)node;
		child = (struct vm_realm *)element->child_context;
		child->host_release = frame_release;
	}

	/* A second independent child becomes the adopted subtree's current Document owner. */
	if (kind == 22 || kind == 27) {
		error = frame_source(realm,
			"var g=document.createElement('iframe');document.body.appendChild(g);"
			"var gd=g.contentDocument;g", &answer);
		if (error != 0) {
			bind_window_destroy(window);
			vm_realm_destroy(realm);
			return error;
		}

		/* Both managed owners use their real original binding cleanup callback. */
		node = bind_node_of(answer);
		element = (struct dom_element *)node;
		child = (struct vm_realm *)element->child_context;
		child->host_release = frame_release;
	}

	/* Builds real child CSS C caches before exercising collection or heap teardown. */
	error = frame_source(realm,
		"var sheet=d.createElement('style');sheet.textContent='body {color:red;}';"
		"d.head.appendChild(sheet);w.getComputedStyle(d.body).color", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Checks active child cache lifetime and viewport invalidation in one bounded case. */
	if (kind == 5) {
		/* Confirms the default production cascade before changing its viewport. */
		same = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(answer), "rgb(255, 0, 0)");
		frame_check(same, "active child CSS context computes before collection");

		/* A connected frame remains reachable through the primary Document tracer. */
		vm_heap_collect(heap);
		error = frame_source(realm,
			"w.getComputedStyle(d.body).color==='rgb(255, 0, 0)'", &answer);
		if (error != 0) {
			bind_window_destroy(window);
			vm_realm_destroy(realm);
			return error;
		}

		/* The C cache does not replace ordinary DOM and owner tracing. */
		same = 0;
		if (answer == VM_VALUE_TRUE)
			same = 1;
		frame_check(same, "active child CSS cache remains valid after GC");

		/* A fixed media rule exercises both sides of the inherited viewport contract. */
		bind_window_set_viewport(child->host, 400, 300);
		error = frame_source(realm,
			"sheet.textContent='body {z-index:1;} @media (min-width:500px) {body {z-index:2;}}';"
			"w.getComputedStyle(d.body).zIndex==='1'", &answer);
		if (error != 0) {
			bind_window_destroy(window);
			vm_realm_destroy(realm);
			return error;
		}

		/* The initial narrow viewport excludes the conditional rule. */
		same = 0;
		if (answer == VM_VALUE_TRUE)
			same = 1;
		frame_check(same, "child CSS media honors current narrow viewport");

		/* The same Document generation must invalidate styles when the width changes. */
		bind_window_set_viewport(child->host, 800, 300);
		error = frame_source(realm,
			"w.getComputedStyle(d.body).zIndex==='2'", &answer);
		if (error != 0) {
			bind_window_destroy(window);
			vm_realm_destroy(realm);
			return error;
		}

		/* The changed viewport enables the same cached media list. */
		same = 0;
		if (answer == VM_VALUE_TRUE)
			same = 1;
		frame_check(same, "child CSS viewport update invalidates cached styles");
	}

	/* Selects one graph while discarding the other script-held child references. */
	if (kind == 0) {
		error = frame_source(realm,
			"var saved=w.Function('return document.body.nodeName===\"BODY\" && document.defaultView===null;');"
			"document.body.removeChild(f);w=null;d=null;f=null;saved", &answer);
	} else if (kind == 1) {
		error = frame_source(realm,
			"var saved=d;document.body.removeChild(f);w=null;d=null;f=null;saved", &answer);
	} else if (kind == 5) {
		error = frame_source(realm,
			"var saved=w.getComputedStyle(d.body);"
			"document.body.removeChild(f);w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 6) {
		error = frame_source(realm,
			"var saved=document.createEvent('UIEvents');saved.initUIEvent('held',false,false,w,7);"
			"document.body.removeChild(f);w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 7) {
		error = frame_source(realm,
			"var xml=d.implementation.createDocument('urn:held','Root');"
			"var saved=xml.createElement('Held');xml.documentElement.appendChild(saved);"
			"document.body.removeChild(f);xml=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 22) {
		/* The iterator's creator realm and root's new owner are different actual children. */
		error = frame_source(realm,
			"var table=d.createElement('table');var row=d.createElement('tr');table.appendChild(row);"
			"var saved=d.createNodeIterator(table,1);saved.nextNode();saved.nextNode();"
			"gd.body.appendChild(table);document.body.removeChild(f);document.body.removeChild(g);"
			"table=null;row=null;w=null;d=null;f=null;g=null;gd=null;sheet=null;saved", &answer);
	} else if (kind == 28) {
		/* Only the independent clone survives; the original weak subscriber must become collectible. */
		error = frame_source(
			realm,
			"var original=d.createRange();var text=d.createTextNode('held');original.selectNodeContents(text);"
			"var saved=original.cloneRange();document.body.removeChild(f);"
			"original=null;text=null;w=null;d=null;f=null;sheet=null;saved",
			&answer);
	} else if (kind == 27) {
		/* A detached subtree adoption preserves the Range's endpoints while moving its weak token. */
		error = frame_source(
			realm,
			"var p=d.createElement('p');var text=d.createTextNode('held');p.appendChild(text);"
			"var saved=d.createRange();saved.selectNodeContents(text);gd.body.appendChild(p);"
			"document.body.removeChild(f);document.body.removeChild(g);"
			"p=null;text=null;w=null;d=null;f=null;g=null;gd=null;sheet=null;saved",
			&answer);
	} else if (kind == 26) {
		/* Only the native state, endpoints and child prototypes retain the original creator. */
		error = frame_source(
			realm,
			"var saved=d.createRange();var text=d.createTextNode('held');saved.selectNodeContents(text);"
			"document.body.removeChild(f);text=null;w=null;d=null;f=null;sheet=null;saved",
			&answer);
	} else if (kind == 25) {
		/* Only the actual native option and its creator prototypes retain the child owner. */
		error = frame_source(realm,
			"var saved=d.createElement('option');saved.selected=true;document.body.removeChild(f);"
			"w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 24) {
		/* Neither the select nor its child global remains an independent script root. */
		error = frame_source(realm,
			"var select=d.createElement('select');var option=d.createElement('option');option.id='held';"
			"select.add(option);var saved=select.options;document.body.removeChild(f);"
			"select=null;option=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 21 || kind == 23) {
		/* Only the native table retains its actual managed child. */
		error = frame_source(realm,
			"var saved=d.createElement('table');document.body.removeChild(f);"
			"w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 19 || kind == 20) {
		/* Neither the table nor frame remains as an independent script root. */
		error = frame_source(realm,
			"var table=d.createElement('table');d.body.appendChild(table);"
			"table.innerHTML='<tbody><tr><td id=held></td></tr></tbody>';"
			"var saved=table.rows;table=null;document.body.removeChild(f);"
			"w=null;d=null;f=null;sheet=null;saved", &answer);

		/* The second sample keeps only the child row's cells collection instead. */
		if (error == 0 && kind == 20)
			error = frame_source(realm, "saved=saved[0].cells;saved", &answer);
	} else if (kind == 18) {
		/* Only the native Event's submitter edge retains the detached child input. */
		error = frame_source(realm,
			"var saved=new w.SubmitEvent('submit',{submitter:d.createElement('input')});"
			"document.body.removeChild(f);w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 17) {
		/* The saved child input must select its retained actual child event owner. */
		error = frame_source(realm,
			"var saved=d.createElement('input');saved.type='checkbox';d.body.appendChild(saved);"
			"saved.onclick=w.Function('e','this.value=e.isTrusted ? \"bad\" : \"owned\";');"
			"document.body.removeChild(f);w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 16) {
		/* Only the native duplicate list survives with its actual group's current state. */
		error = frame_source(realm,
			"var form=d.createElement('form');d.body.appendChild(form);"
			"form.innerHTML='<input type=radio name=pair value=owned><input type=radio name=pair value=other>';"
			"form.elements[0].checked=true;var saved=form.elements.pair;"
			"document.body.removeChild(f);form=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 15) {
		/* Only the actual input remains after its frame and primary owner retire. */
		error = frame_source(realm,
			"var saved=d.createElement('input');saved.value='owned';d.body.appendChild(saved);"
			"document.body.removeChild(f);w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 13 || kind == 14) {
		/* Neither a form nor a separate child global survives as an independent script root. */
		error = frame_source(realm,
			"var form=d.createElement('form');d.body.appendChild(form);"
			"form.innerHTML='<input name=Next><input id=Next>';"
			"var saved=form.elements;", &answer);
		if (error == 0 && kind == 14)
			error = frame_source(realm, "saved=saved.Next", &answer);
		if (error == 0) {
			error = frame_source(realm,
				"document.body.removeChild(f);form=null;w=null;d=null;f=null;sheet=null;saved", &answer);
		}
	} else if (kind == 12) {
		/* The sole live forms collection has no independent child Window variable. */
		error = frame_source(realm,
			"var form=d.createElement('form');form.id='Next';d.body.appendChild(form);"
			"var saved=d.forms;document.body.removeChild(f);"
			"form=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 11) {
		/* A live child collection is the sole surviving DOM binding graph. */
		error = frame_source(realm,
			"var xml=d.implementation.createDocument(null,'Held');"
			"xml.documentElement.appendChild(xml.createElement('Next'));"
			"var saved=xml.documentElement.children;"
			"document.body.removeChild(f);xml=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 10) {
		/* A consumed root leaves the child iterator ready to traverse to Next. */
		error = frame_source(realm,
			"var xml=d.implementation.createDocument(null,'Held');"
			"xml.documentElement.appendChild(xml.createElement('Next'));"
			"var saved=xml.createNodeIterator(xml.documentElement,1,w.Function('return true;'));"
			"saved.nextNode();document.body.removeChild(f);"
			"xml=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 8) {
		error = frame_source(realm,
			"var saved=d.implementation;"
			"document.body.removeChild(f);w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 9) {
		error = frame_source(realm,
			"var xml=d.implementation.createDocument(null,'Held');"
			"xml.documentElement.appendChild(xml.createElement('Next'));"
			"var saved=xml.createTreeWalker(xml.documentElement,1,w.Function('return true;'));"
			"document.body.removeChild(f);xml=null;w=null;d=null;f=null;sheet=null;saved", &answer);
	} else if (kind == 4) {
		error = frame_source(realm, "w=null;d=null;f=null;nested=null;saved", &answer);
	} else {
		error = frame_source(realm,
			"var saved=w;w=null;d=null;f=null;saved", &answer);
	}

	/* A failed fixture must not publish an arbitrary cell as a saved reference. */
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Publishes the selected cell before the embedding destroys its parent. */
	*root = vm_value_as_cell(answer);

	/* Closing the primary retires still-connected children before freeing C hosts. */
	bind_window_destroy(window);
	same = 0;
	if (document->view == NULL && document->removed == NULL)
		same = 1;
	frame_check(same, "explicit parent releases Document hooks");
	vm_realm_destroy(realm);

	/* Succeeded: the caller receives a cell rather than an unrooted C child pointer. */
	return 0;
}

/* Collects using explicit embedding roots, excluding obsolete construction words. */
static int
frame_retention(
	unsigned kind)
{
	struct vm_heap *heap;
	struct vm_realm *caller;
	struct vm_cell *root;
	struct dom_node *node;
	struct dom_node *reference;
	struct bind_window *window;
	struct bind_event *event;
	const void *stack_base;
	vm_value answer;
	vm_value key;
	vm_value factory;
	vm_value arguments[2];
	vm_value saved;
	unsigned before;
	unsigned owners;
	int error;
	int same;

	/* Stack scanning protects partial construction and ordinary active execution. */
	stack_base = __builtin_frame_address(0);
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, stack_base);
	root = NULL;
	error = vm_heap_add_root(heap, &root);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Captures cleanup counts before constructing this sample's child. */
	before = released;
	owners = 1U;
	if (kind == 4 ||
	    kind == 22 ||
	    kind == 27)
		owners = 2U;
	error = frame_sample(heap, kind, &root);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		return error;
	}

	/* This branch verifies finalizer ordering without a preliminary collection. */
	if (kind == 3) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		same = 0;
		if (released == before + 1U)
			same = 1;
		frame_check(same, "whole heap releases child exactly once");
		return 0;
	}

	/* Only the selected saved graph should retain the detached child context. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_set_stack_base(heap, stack_base);
	same = 0;
	if (released == before)
		same = 1;
	frame_check(same, "saved child graph survives parent destruction and GC");

	/* An unrelated live caller can execute a surviving child-owned function. */
	error = vm_realm_create(heap, &caller);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		return error;
	}

	/* Dispatches through the surviving object graph selected by this sample. */
	saved = vm_value_cell(root);
	if (kind == 0) {
		error = vm_call(caller, saved, VM_VALUE_UNDEFINED, NULL, 0, &answer);
	} else if (kind == 1) {
		/* The raw Document edge preserves both its wrapper and actual bound owner. */
		node = bind_node_of(vm_value_cell(root));
		window = node->document->view;
		error = bind_wrap(window, node, &answer);
		if (error != 0) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return error;
		}

		/* The wrapper stays in its original child realm after removal. */
		same = 0;
		if (answer == saved && window->detached)
			same = 1;
		frame_check(same, "saved Document wrapper and detached owner identity");
		answer = VM_VALUE_TRUE;
	} else if (kind == 5) {
		/* A detached declaration remains safe while its values become empty. */
		key = vm_key_from_ascii(heap, "color");
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			same = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(answer), "");
			answer = vm_value_boolean(same);
		}
	} else if (kind == 6) {
		/* The primary prototype has no managed child owner; only UI view retains it. */
		event = bind_event_of(saved);
		key = vm_key_from_ascii(heap, "closed");
		if (event == NULL || key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return EINVAL;
		}

		/* Reads through the retained child's own live realm, not the retired parent prototype. */
		error = vm_get(caller, event->view, key, &answer);
	} else if (kind == 7) {
		/* The saved XML node retains its own realm while defaultView remains null. */
		node = bind_node_of(saved);
		key = vm_key_from_ascii(heap, "tagName");
		if (node == NULL || key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return EINVAL;
		}

		/* Calls the surviving child's Element accessor through an unrelated caller. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			same = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(answer), "Held");
			answer = vm_value_boolean(same);
		}
	} else if (kind == 8) {
		/* A sole implementation reference retains its associated detached Document. */
		key = vm_key_from_ascii(heap, "createDocument");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Resolves and invokes the real child's factory after the parent has been destroyed. */
		error = vm_get(caller, saved, key, &factory);
		if (error == 0) {
			arguments[0] = VM_VALUE_NULL;
			arguments[1] = VM_VALUE_NULL;
			error = vm_call(caller, factory, saved, arguments, 2, &answer);
		}

		/* The new result retains XML kind without acquiring a browsing-context view. */
		if (error == 0) {
			node = bind_node_of(answer);
			same = 0;
			if (node != NULL && node->type == DOM_DOCUMENT) {
				if (node->document->content == DOM_CONTENT_XML && node->document->view == NULL)
					same = 1;
			}

			/* The generic lifetime assertion consumes the independent factory observation. */
			answer = vm_value_boolean(same);
		}
	} else if (kind == 22) {
		/* The old native iterator getter retains its creator while its root retains the new owner. */
		key = vm_key_from_ascii(heap, "root");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Actual current-Document removal must repair the migrated native iterator after primary destruction. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			node = bind_node_of(answer);
			dom_remove(node->first_child);
			key = vm_key_from_ascii(heap, "referenceNode");
			if (key == VM_VALUE_EMPTY) {
				error = ENOMEM;
			} else {
				error = vm_get(caller, saved, key, &answer);
			}

			/* Cursor repair to the actual root proves that the migrated subscription remains live. */
			if (error == 0) {
				same = 0;
				reference = bind_node_of(answer);
				if (reference == node)
					same = 1;
				answer = vm_value_boolean(same);
			}
		}
	} else if (kind == 27) {
		/* The original creator getter resolves a current endpoint owned by the second retained child. */
		key = vm_key_from_ascii(heap, "startContainer");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Actual removal in the destination Document invokes the migrated native Range token. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			node = bind_node_of(answer);
			dom_remove(node->parent);
			key = vm_key_from_ascii(heap, "collapsed");
			if (key == VM_VALUE_EMPTY) {
				error = ENOMEM;
			} else {
				error = vm_get(caller, saved, key, &answer);
			}
		}
	} else if (kind == 26 || kind == 28) {
		/* Resolve the retained child's native readonly boundary after manual primary destruction. */
		key = vm_key_from_ascii(heap, "endOffset");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* The sole Range still owns the complete detached text interval. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			same = 0;
			factory = vm_value_int32(4);
			if (answer == factory)
				same = 1;
			answer = vm_value_boolean(same);
		}
	} else if (kind == 25) {
		/* Resolve the retained child's actual native getter after primary C ownership is gone. */
		key = vm_key_from_ascii(heap, "selected");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Owned selectedness remains true independently of a default attribute or live UI. */
		error = vm_get(caller, saved, key, &answer);
	} else if (kind == 21 || kind == 23) {
		/* The native method resolves its retained actual child after manual primary destruction. */
		key = vm_key_from_ascii(heap, "createCaption");
		if (kind == 23)
			key = vm_key_from_ascii(heap, "insertRow");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* The returned caption uses the retained child's exact native interface and Document. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0)
			error = vm_call(caller, answer, saved, NULL, 0, &answer);
		if (error == 0) {
			node = bind_node_of(answer);
			same = 0;
			if (node != NULL && node->type == DOM_ELEMENT)
				same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "caption");
			if (kind == 23 && node != NULL)
				same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "tr");
			answer = vm_value_boolean(same);
		}
	} else if (kind == 18) {
		/* The retained managed child's readonly getter returns the actual detached input. */
		key = vm_key_from_ascii(heap, "submitter");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Reading this field invokes the native child SubmitEvent accessor after primary destruction. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			node = bind_node_of(answer);
			same = 0;
			if (node != NULL && node->type == DOM_ELEMENT)
				same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "input");
			answer = vm_value_boolean(same);
		}
	} else if (kind == 17) {
		/* Calling click resolves a retained child native method without its former primary. */
		key = vm_key_from_ascii(heap, "click");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* The child-created handler outlives the manually retired primary callback realm. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0)
			error = vm_call(caller, answer, saved, NULL, 0, &answer);

		/* The handler value proves that actual untrusted event dispatch occurred after teardown. */
		if (error == 0) {
			key = vm_key_from_ascii(heap, "value");
			if (key == VM_VALUE_EMPTY) {
				error = ENOMEM;
			} else {
				error = vm_get(caller, saved, key, &answer);
			}
		}

		/* The shared lifetime assertion records both the invocation and child accessor result. */
		if (error == 0) {
			same = vm_value_is_string(answer);
			if (same)
				same = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(answer), "owned");
			answer = vm_value_boolean(same);
		}
	} else if (kind == 15 || kind == 16) {
		/* Reading value invokes the retained managed child's branded native accessor. */
		key = vm_key_from_ascii(heap, "value");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Resolve the current value through the surviving child native getter. */
		error = vm_get(caller, saved, key, &answer);
		if (error == 0) {
			/* The owned value remains available without a raw primary Window dependency. */
			same = vm_value_is_string(answer);
			if (same)
				same = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(answer), "owned");
			answer = vm_value_boolean(same);
		}
	} else if (kind == 11 ||
	           kind == 12 ||
	           kind == 13 ||
	           kind == 14 ||
	           kind == 19 ||
	           kind == 20 ||
	           kind == 24) {
		/* Resolve the child method after primary teardown, then invoke its live indexed getter. */
		key = vm_key_from_ascii(heap, "item");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* The saved wrapper retains the child native function owner and its XML root. */
		error = vm_get(caller, saved, key, &factory);
		if (error == 0) {
			arguments[0] = vm_value_int32(0);
			error = vm_call(caller, factory, saved, arguments, 1, &answer);
		}

		/* Its current member can acquire a wrapper after all primary C ownership is gone. */
		if (error == 0) {
			node = bind_node_of(answer);
			same = 0;
			if (node != NULL && node->type == DOM_ELEMENT) {
				if (kind == 11) {
					same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "Next");
				} else if (kind == 19) {
					/* A table rows collection returns a real current child tr. */
					same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "tr");
				} else if (kind == 20) {
					/* A sole cells collection returns its actual retained child td. */
					same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "td");
				} else if (kind == 24) {
					/* A sole options collection returns an actual retained child option. */
					same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "option");
				} else if (kind == 13 || kind == 14) {
					/* Both control families resolve members in the retained child realm. */
					same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "input");
				} else {
					/* HTML parser Documents wrap form nodes in their managed child realm. */
					same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "form");
				}
			}

			/* The generic assertion records this independent live collection observation. */
			answer = vm_value_boolean(same);
		}
	} else if (kind == 9 || kind == 10) {
		/* Child-created traversal retains both an XML root and its actual callback. */
		key = vm_key_from_ascii(heap, "nextNode");
		if (key == VM_VALUE_EMPTY) {
			vm_realm_destroy(caller);
			vm_heap_remove_root(heap, &root);
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* Invokes surviving child navigation and filtering after primary teardown. */
		error = vm_get(caller, saved, key, &factory);
		if (error == 0)
			error = vm_call(caller, factory, saved, NULL, 0, &answer);

		/* The accepted XML node preserves its original spelling and owner Document. */
		if (error == 0) {
			node = bind_node_of(answer);
			same = 0;
			if (node != NULL && node->type == DOM_ELEMENT) {
				same = vm_string_equal_ascii(((struct dom_element *)node)->local_name, "Next");
			}

			/* The generic lifetime observation consumes this independent navigation result. */
			answer = vm_value_boolean(same);
		}
	} else {
		/* A retained Window keeps its own closed getter and realm usable. */
		key = vm_key_from_ascii(heap, "closed");
		error = vm_get(caller, vm_value_cell(root), key, &answer);
	}

	/* The selected child API must complete successfully with its expected state. */
	same = 0;
	if (error == 0 && answer == VM_VALUE_TRUE)
		same = 1;
	frame_check(same, "saved child remains usable after parent teardown");
	vm_realm_destroy(caller);

	/* Releasing the last selected cell permits the managed host to be reclaimed. */
	root = NULL;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	same = 0;
	if (released == before + owners)
		same = 1;
	frame_check(same, "unreachable iframe releases real host once");
	vm_heap_collect(heap);
	same = 0;
	if (released == before + owners)
		same = 1;
	frame_check(same, "repeated collection does not release iframe twice");
	vm_heap_remove_root(heap, &root);
	vm_heap_destroy(heap);

	/* Succeeded: this selected saved-reference path was exercised completely. */
	return 0;
}
