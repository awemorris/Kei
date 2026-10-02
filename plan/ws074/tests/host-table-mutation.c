/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks native table mutation graphs during actual collector pressure. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One synchronous host invocation observes integer addresses without retaining DOM cells. */
struct table_observer {
	struct vm_heap *heap;
	uintptr_t owner;
	uintptr_t old;
	uintptr_t anchor;
	uintptr_t created;
	unsigned calls;
	int failure;
};

/* Assertions accumulate across callback-driven and embedding-driven collection. */
static unsigned checks;
/* Failed behavioral checks determine the final exit status. */
static unsigned failures;
/* The embedding restores this construction stack boundary after explicit collection. */
static const void *construction_stack;

/* The synchronous callback sees integer-only observer data until its native invocation returns. */
static struct table_observer *active_observer;

static void collection_check(int condition, const char *name);
static int collection_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int collection_case(struct vm_heap *heap);
static int table_collect(void *context, struct dom_node *node);

/*
 * Verifies rooted table mutations and collectible DOM cycles through real host callbacks.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	int error;
	int printed;

	/* The collector uses real construction stacks until a test explicitly excludes them. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;
	construction_stack = __builtin_frame_address(0);
	vm_heap_set_stack_base(heap, construction_stack);
	error = collection_case(heap);
	vm_heap_destroy(heap);
	if (error != 0)
		return 2;

	/* Reports behavioral failures independently of fixture allocation failures. */
	printed = printf("table mutation lifetime checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Every independently observed GC contract must hold. */
	if (failures != 0)
		return 1;

	/* Succeeded: selected collection cache and root graphs survived actual GC. */
	return 0;
}

/* Records one named ownership contract without hiding later independent observations. */
static void
collection_check(
	int condition,
	const char *name)
{
	int printed;

	/* Each observation contributes to the final assertion accounting. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the named contract was recorded. */
	return;
}

/* Executes ordinary fixture script through the production parser and interpreter. */
static int
collection_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int error;

	/* Converts input before executing the production bindings. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* The actual script engine creates every node and collection wrapper. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the fixture completion is available. */
	return 0;
}

/* Collects after severing the links which otherwise hide missing native operation roots. */
static int
table_collect(
	void *context,
	struct dom_node *node)
{
	struct table_observer *observer;
	struct dom_node *owner;
	struct dom_node *anchor;
	struct vm_cell *found;
	uintptr_t created;
	int matches;

	UNUSED_PARAMETER(context);

	/* The actual host context stays intact for unrelated environment collaborators. */
	observer = active_observer;
	observer->calls++;
	owner = node->parent;
	anchor = NULL;
	if (observer->anchor != 0)
		anchor = (struct dom_node *)observer->anchor;
	created = (uintptr_t)node;
	observer->created = created;
	matches = 0;
	if ((uintptr_t)owner == observer->owner)
		matches = 1;
	collection_check(matches, "host sees actual table owner");

	/* The pending operation must retain both new and old nodes after all tree edges are severed. */
	dom_remove(node);
	dom_remove(owner);
	if (anchor != NULL)
		dom_remove(anchor);
	vm_heap_set_stack_base(observer->heap, NULL);
	vm_heap_collect(observer->heap);
	found = vm_heap_find_cell(observer->heap, observer->owner);
	collection_check(found != NULL, "table retained through callback GC");
	found = vm_heap_find_cell(observer->heap, created);
	collection_check(found != NULL, "created or incoming child retained through callback GC");

	/* A replacement also retains the already removed old child until its native invocation ends. */
	if (observer->old != 0) {
		found = vm_heap_find_cell(observer->heap, observer->old);
		collection_check(found != NULL, "detached replaced child retained through callback GC");
	}

	/* The insertion anchor remains independently retained if the host removes it too. */
	if (observer->anchor != 0) {
		found = vm_heap_find_cell(observer->heap, observer->anchor);
		collection_check(found != NULL, "removed insertion anchor retained through callback GC");
	}

	/* Restores the embedding's ordinary stack contract before returning its configured host outcome. */
	vm_heap_set_stack_base(observer->heap, construction_stack);
	if (observer->failure)
		return EIO;

	/* Succeeded: collection pressure exercised the complete default mutation path. */
	return 0;
}

/* Exercises create, replace and host-error cleanup through native public operations. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_window *actual;
	struct bind_host host;
	struct table_observer observer;
	struct dom_node *table;
	struct dom_node *created;
	struct vm_cell *found;
	uintptr_t created_address;
	vm_value receiver;
	vm_value argument;
	vm_value answer;
	unsigned kind;
	int status;
	int expected;
	int matches;

	/* Installs the real primary binding without a synthetic engine behavior switch. */
	status = vm_realm_create(heap, &realm);
	if (status != 0)
		return status;
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* The primary host is manual; child hosts are managed by their actual iframe owners. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* No embedding callback participates until the finite mutation case starts. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* Each finite case leaves its receiver unrooted outside the native operation itself. */
	for (kind = 0; kind < 4U; kind++) {
		/* Fixture construction cannot invoke the subsequent mutation observer prematurely. */
		window->host.node_inserted = NULL;
		status = collection_script(realm,
			"(function(){var t=document.createElement('table');"
			"t.appendChild(document.createElement('tbody'));return t;})()", &receiver);
		if (status != 0)
			break;
		table = bind_node_of(receiver);
		actual = window;
		argument = VM_VALUE_NULL;
		memset(&observer, 0, sizeof(observer));
		observer.heap = heap;
		observer.owner = (uintptr_t)table;
		observer.anchor = (uintptr_t)table->first_child;

		/* Replacement needs an old member and a separate incoming native caption. */
		if (kind == 1) {
			status = bind_table_caption_create(realm, receiver, NULL, 0, &answer);
			if (status != 0)
				break;
			observer.old = (uintptr_t)bind_node_of(answer);
			status = collection_script(realm, "document.createElement('caption')", &argument);
			if (status != 0)
				break;
		}

		/* A borrowed primary operation must call the retained actual child's host instead. */
		if (kind == 2) {
			status = collection_script(realm,
				"document.appendChild(document.createElement('html'));"
				"document.documentElement.appendChild(document.createElement('body'));"
				"(function(){var f=document.createElement('iframe');document.body.appendChild(f);"
				"var t=f.contentDocument.createElement('table');"
				"t.appendChild(f.contentDocument.createElement('tbody'));f.remove();return t;})()", &receiver);
			if (status != 0)
				break;
			table = bind_node_of(receiver);
			actual = table->document->view;
			observer.owner = (uintptr_t)table;
			observer.anchor = (uintptr_t)table->first_child;
		}

		/* A real host error tests root cleanup after insertion already changed the DOM. */
		expected = 0;
		if (kind == 3) {
			observer.failure = 1;
			expected = EIO;
		}

		/* Only the synchronous insertion hook uses this integer-only observer. */
		active_observer = &observer;
		actual->host.node_inserted = table_collect;
		if (kind == 1) {
			status = bind_table_caption_set(realm, receiver, &argument, 1, &answer);
		} else {
			status = bind_table_caption_create(realm, receiver, NULL, 0, &answer);
		}

		/* All observations before the next collection read only completed native invocation state. */
		collection_check(status == expected, "exact host outcome propagated");
		collection_check(observer.calls == 1U, "actual owner insertion callback called once");
		actual->host.node_inserted = NULL;
		active_observer = NULL;
		created_address = 0;
		if (kind != 1 && status == 0) {
			created = bind_node_of(answer);
			created_address = (uintptr_t)created;
			matches = 0;
			if (created != NULL && created->document == table->document)
				matches = 1;
			collection_check(matches, "created wrapper uses actual table Document after callback removal");
		}

		/* No receiver, result or C stack keeps these detached operation roots alive afterward. */
		receiver = VM_VALUE_UNDEFINED;
		argument = VM_VALUE_UNDEFINED;
		answer = VM_VALUE_UNDEFINED;
		table = NULL;
		created = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.owner);
		collection_check(found == NULL, "owner reclaimed after native temporary roots released");
		found = vm_heap_find_cell(heap, observer.created);
		collection_check(found == NULL, "incoming child reclaimed after every native exit");
		found = vm_heap_find_cell(heap, observer.anchor);
		collection_check(found == NULL, "anchor reclaimed after native temporary roots released");

		/* Replacement must release its detached old child on the same cleanup path. */
		if (observer.old != 0) {
			found = vm_heap_find_cell(heap, observer.old);
			collection_check(found == NULL, "replaced old child reclaimed after native return");
		}

		/* A created wrapper has no persistent publication root after the embedding drops the result. */
		if (created_address != 0) {
			found = vm_heap_find_cell(heap, created_address);
			collection_check(found == NULL, "created node reclaimed after final result release");
		}

		/* The next fixture resumes ordinary stack tracing after all temporary roots are gone. */
		vm_heap_set_stack_base(heap, construction_stack);
		status = 0;
	}

	/* The primary remains valid while every pending root slot is already removed. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (status != 0)
		return status;

	/* Succeeded: every finite default-path mutation and failure case was exercised. */
	return 0;
}
