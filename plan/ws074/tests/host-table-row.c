/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks native row mutation and conversion graphs during actual collector pressure. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One synchronous host invocation observes integer addresses without retaining DOM cells. */
struct table_observer {
	struct vm_heap *heap;
	uintptr_t owner;
	uintptr_t old;
	uintptr_t target;
	uintptr_t row;
	uintptr_t argument;
	unsigned conversions;
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
static int index_collect(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

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
	printed = printf("table row lifetime checks: %u/%u passed\n", checks - failures, checks);
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

	/* A newly built body must not hide an unrooted pending row through its original child edge. */
	if (node->first_child != NULL) {
		observer->row = (uintptr_t)node->first_child;
		dom_remove(node->first_child);
	}

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

	/* The operation target survives even when the current insertion parent is a removed header. */
	found = vm_heap_find_cell(observer->heap, observer->target);
	collection_check(found != NULL, "native operation target retained through callback GC");
	if (observer->row != 0) {
		found = vm_heap_find_cell(observer->heap, observer->row);
		collection_check(found != NULL, "new row retained after host removes it from new body");
	}

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

/* Collects inside numeric conversion before any row allocation or native membership observation. */
static int
index_collect(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_cell *found;
	struct dom_node *target;

	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The direct native invocation has no script argument frame independently retaining the receiver. */
	active_observer->conversions++;
	target = (struct dom_node *)active_observer->target;
	dom_remove(target);
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, active_observer->target);
	collection_check(found != NULL, "native target survives numeric conversion GC");
	found = vm_heap_find_cell(realm->heap, active_observer->argument);
	collection_check(found != NULL, "numeric argument survives conversion GC");
	vm_heap_set_stack_base(realm->heap, construction_stack);
	*result = vm_value_int32(0);

	/* Succeeded: membership observation can now begin using the converted zero index. */
	return 0;
}

/* Exercises pending new bodies/rows/parents/references, conversion and host-error cleanup. */
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

	/* Only actual managed child ownership supplies the borrowed mutation case. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* The ordinary host has no callbacks until each finite native invocation starts. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		return status;
	}

	/* A real native valueOf callback exercises collection during signed index conversion. */
	status = js_builtin_method(realm, realm->global, "collectIndex", 0, index_collect);
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return status;
	}

	/* Each fixture excludes unrelated script roots and every C stack during actual collection. */
	for (kind = 0; kind < 6U; kind++) {
		/* An empty table normally builds a new complete body containing the pending native row. */
		window->host.node_inserted = NULL;
		status = collection_script(realm, "document.createElement('table')", &receiver);
		if (status != 0)
			break;
		table = bind_node_of(receiver);
		actual = window;
		argument = vm_value_int32(0);
		memset(&observer, 0, sizeof(observer));
		observer.heap = heap;

		/* An indexed existing header supplies a separate insertion parent and separately removed anchor. */
		if (kind == 1) {
			status = bind_table_head_create(realm, receiver, NULL, 0, &answer);
			if (status != 0)
				break;
			status = bind_section_row_insert(realm, answer, NULL, 0, &answer);
			if (status != 0)
				break;
		}

		/* The actual child host must be selected through a borrowed primary row operation. */
		if (kind == 2) {
			status = collection_script(realm,
						   "document.appendChild(document.createElement('html'));"
						   "document.documentElement.appendChild(document.createElement('body'));"
						   "(function(){var f=document.createElement('iframe');document.body.appendChild(f);"
						   "var t=f.contentDocument.createElement('table');f.remove();return t;})()",
						   &receiver);
			if (status != 0)
				break;
			table = bind_node_of(receiver);
			actual = table->document->view;
		}

		/* Body creation anchors after the last current tbody and before the existing footer. */
		if (kind == 4) {
			status = collection_script(realm,
						   "(function(){var t=document.createElement('table');"
						   "t.innerHTML='<tbody></tbody><tfoot></tfoot>';return t;})()",
						   &receiver);
			if (status != 0)
				break;
			table = bind_node_of(receiver);
			observer.anchor = (uintptr_t)table->last_child;
		}

		/* A separate argument object survives its own valueOf collection before mutation allocation. */
		if (kind == 5) {
			status = collection_script(realm, "({valueOf:collectIndex})", &argument);
			if (status != 0)
				break;
			observer.argument = (uintptr_t)vm_value_as_cell(argument);
		}

		/* The host checks both the original target and the actual current insertion parent. */
		observer.target = (uintptr_t)table;
		observer.owner = observer.target;
		if (kind == 1) {
			observer.owner = (uintptr_t)table->first_child;
			observer.anchor = (uintptr_t)table->first_child->first_child;
		}

		/* A real host error follows already completed native insertion and must unwind every slot. */
		expected = 0;
		if (kind == 3) {
			observer.failure = 1;
			expected = EIO;
		}

		/* Only the synchronous callback uses the integer-only observer; actual host contexts stay intact. */
		active_observer = &observer;
		actual->host.node_inserted = table_collect;
		if (kind == 4) {
			status = bind_table_body_create(realm, receiver, NULL, 0, &answer);
		} else {
			status = bind_table_row_insert(realm, receiver, &argument, 1, &answer);
		}

		/* All native observations read complete invocation state before its final-root release. */
		collection_check(status == expected, "exact row host outcome propagated");
		collection_check(observer.calls == 1U, "complete subtree produces one actual owner notification");
		actual->host.node_inserted = NULL;
		active_observer = NULL;
		if (kind == 5)
			collection_check(observer.conversions == 1U, "native index converted exactly once under GC");
		if (status == 0) {
			created = bind_node_of(answer);
			matches = 0;
			if (created != NULL && created->document == table->document)
				matches = 1;
			collection_check(matches, "returned native element keeps actual current Document after callback removal");
		}

		/* No result, receiver, argument or construction stack retains the detached native mutation graph. */
		receiver = VM_VALUE_UNDEFINED;
		argument = VM_VALUE_UNDEFINED;
		answer = VM_VALUE_UNDEFINED;
		table = NULL;
		created = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		found = vm_heap_find_cell(heap, observer.target);
		collection_check(found == NULL, "target reclaimed after temporary native roots released");
		found = vm_heap_find_cell(heap, observer.owner);
		collection_check(found == NULL, "insertion parent reclaimed after temporary native roots released");
		found = vm_heap_find_cell(heap, observer.created);
		collection_check(found == NULL, "inserted subtree reclaimed after native result release");
		if (observer.row != 0) {
			found = vm_heap_find_cell(heap, observer.row);
			collection_check(found == NULL, "pending row reclaimed after temporary native roots released");
		}

		/* Independently removed anchors and conversion arguments have no expired slot or hidden root. */
		if (observer.anchor != 0) {
			found = vm_heap_find_cell(heap, observer.anchor);
			collection_check(found == NULL, "removed row/body anchor reclaimed after native return");
		}

		/* Numeric argument ownership ends with the same native invocation cleanup. */
		if (observer.argument != 0) {
			found = vm_heap_find_cell(heap, observer.argument);
			collection_check(found == NULL, "converted argument reclaimed after native return");
		}

		/* The next fixture resumes the ordinary conservative construction stack contract. */
		vm_heap_set_stack_base(heap, construction_stack);
		status = 0;
	}

	/* Primary teardown observes no root slot pointing into an expired native invocation. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	if (status != 0)
		return status;

	/* Succeeded: every finite row conversion/insertion/host-error lifetime contract was exercised. */
	return 0;
}
