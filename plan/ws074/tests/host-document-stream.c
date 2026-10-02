/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies native child Document stream ownership during inline script GC and retirement. */

#include "bind/internal.h"
#include "html/html.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Console callback state belongs to this synchronous native embedding. */
struct stream_fixture {
	struct vm_heap *heap;
	struct dom_node *frame;
	unsigned collections;
	unsigned exceptions;
	int retire;
};

/* Independent stream and ownership observations survive fixture teardown. */
static unsigned checks;
/* All failed observations remain visible in the final native result. */
static unsigned failures;

static void stream_check(int condition, const char *name);
static int stream_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int stream_case(struct vm_realm *realm, struct bind_window *window, struct stream_fixture *fixture);
static void stream_console(void *context, int level, const char *text, size_t length);

/*
 * Runs actual child Document stream operations against a genuine managed iframe context.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct stream_fixture fixture;
	int status;
	int printed;

	/* Ordinary fixture installation supplies genuine managed Window and iframe state. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Builtins and primary Document own the production binding graph. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* A primary Window is installed on an initially empty native Document. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The fixture uses ordinary child binding ownership without host callbacks. */
	memset(&fixture, 0, sizeof(fixture));
	fixture.heap = heap;
	memset(&host, 0, sizeof(host));
	host.context = &fixture;
	host.console = stream_console;
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Only the direct lifetime case disables conservative stack scanning. */
	status = stream_case(realm, window, &fixture);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Publish complete observations after the embedding has released its roots. */
	printed = printf("native document streams: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any parser lifetime loss or lingering child context rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native Document streams and context finalization were verified. */
	return 0;
}

/* Records one native observation while preserving later independent checks. */
static void
stream_check(
	int condition,
	const char *name)
{
	int printed;

	/* Keep every failed identity or lifetime check in the fixture's final result. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Constructs genuine DOM and managed contexts through the ordinary interpreter. */
static int
stream_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* The ordinary interpreter constructs the same iframe binding graph as a Page. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Ordinary script execution provides genuine managed iframe state. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: a normal interpreter value is available to the embedding. */
	return 0;
}

/* Collects during ordinary inline child script execution, optionally retiring its frame. */
static void
stream_console(
	void *context,
	int level,
	const char *text,
	size_t length)
{
	struct stream_fixture *fixture;
	int same;

	UNUSED_PARAMETER(level);

	/* Uncaught child exceptions remain a fixture failure rather than hidden success. */
	fixture = context;
	if (length >= 8U) {
		same = memcmp(text, "Uncaught", 8U);
		if (same == 0)
			fixture->exceptions++;
	}

	/* Only the normal source console call deliberately triggers collection. */
	if (length != 2U)
		return;
	same = memcmp(text, "gc", 2U);
	if (same != 0)
		return;
	if (fixture->retire)
		dom_remove(fixture->frame);
	fixture->collections++;
	vm_heap_collect(fixture->heap);

	/* Succeeded: the interpreter resumes under production write ownership. */
	return;
}

/* Exercises native Document operations without VM caller or conservative stack roots. */
static int
stream_case(
	struct vm_realm *realm,
	struct bind_window *window,
	struct stream_fixture *fixture)
{
	struct dom_node *frame;
	struct dom_document *factory;
	struct vm_realm *child_realm;
	struct bind_window *child;
	struct vm_object *wrapper;
	struct vm_string *source;
	struct vm_cell *found;
	vm_value receiver;
	vm_value argument;
	vm_value answer;
	uintptr_t child_address;
	size_t length;
	int status;
	int same;

	/* A genuine unattached native HTML Document must not acquire a fake replacement owner. */
	factory = dom_document_create(realm->heap);
	if (factory == NULL)
		return ENOMEM;
	status = bind_wrap(window, &factory->node, &receiver);
	if (status != 0)
		return status;
	status = bind_document_interface.operations[20].method(realm, receiver, NULL, 0, &answer);
	stream_check(status == VM_THROWN && factory->node.first_child == NULL, "factory HTML Document without owner refuses replacement stream");
	realm->exception = VM_VALUE_UNDEFINED;

	/* Ordinary DOM methods provide the genuine child Window and native Document wrapper. */
	status = stream_script(realm,
			       "var root=document.createElement('div');document.appendChild(root);"
			       "var f=document.createElement('iframe');root.appendChild(f);f.contentDocument;f",
			       &answer);
	if (status != 0)
		return status;
	frame = bind_node_of(answer);
	fixture->frame = frame;
	child_realm = (struct vm_realm *)((struct dom_element *)frame)->child_context;
	child = child_realm->host;
	child_address = (uintptr_t)&child_realm->cell;
	wrapper = child->document->node.wrapper;
	receiver = vm_value_cell(wrapper);
	status = stream_script(realm, "f=null", &answer);
	if (status != 0)
		return status;
	vm_heap_set_stack_base(realm->heap, NULL);

	/* Direct borrowed open resolves child ownership and preserves Document identity. */
	status = bind_document_interface.operations[20].method(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	stream_check(answer == receiver && child->document->node.first_child == NULL, "native open preserves wrapper and removes old actual tree");
	same = strcmp(child->ready_state, "loading");
	stream_check(same == 0 && child->document_parser != NULL, "native open publishes real owned parser and loading state");

	/* Parser hook performs actual GC and nested writes through the native child global. */
	length = strlen("<!doctype html><script>var nativeMark=5;console.log('gc');"
	    "document.write('<b id=nested>N</b>');document.close();</script><i id=tail>T</i>");
	source = vm_string_from_utf8(realm->heap,
	    "<!doctype html><script>var nativeMark=5;console.log('gc');"
	    "document.write('<b id=nested>N</b>');document.close();</script><i id=tail>T</i>",
	    length);
	if (source == NULL)
		return ENOMEM;
	argument = vm_value_cell(source);
	status = bind_document_interface.operations[0].method(realm, receiver, &argument, 1, &answer);
	if (status != 0)
		return status;
	stream_check(fixture->collections == 1U && fixture->exceptions == 0, "actual inline child callback collects without Uncaught");
	stream_check(child->document_parser == NULL && child->document_parser_depth == 0, "reentrant close releases parser only after outer native parse returns");
	status = stream_script(child_realm,
			       "nativeMark===5&&document.getElementById('nested').textContent==='N'"
			       "&&document.getElementById('tail').textContent==='T'",
			       &answer);
	if (status != 0)
		return status;
	stream_check(answer == VM_VALUE_TRUE, "actual child realm state and nested insertion survive callback GC");

	/* A second stream retires its last ordinary child edge during an executing parser hook. */
	status = bind_document_interface.operations[20].method(realm, receiver, NULL, 0, &answer);
	if (status != 0)
		return status;
	fixture->retire = 1;
	length = strlen("<script>console.log('gc')</script><p>after</p>");
	source = vm_string_from_utf8(realm->heap, "<script>console.log('gc')</script><p>after</p>", length);
	if (source == NULL)
		return ENOMEM;
	argument = vm_value_cell(source);
	status = bind_document_interface.operations[0].method(realm, receiver, &argument, 1, &answer);
	if (status != 0)
		return status;
	stream_check(fixture->collections == 2U && fixture->exceptions == 0, "real callback retires frame and collects during native write");
	stream_check(child->detached && child->document_parser != NULL && child->document_parser_depth == 0, "temporary actual owner survives retirement until native write unwinds");

	/* No script reference or parser standalone tracer may keep the retired context alive. */
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	stream_check(found == NULL, "forgotten retired child and unfinished written stream collectible");

	/* Succeeded: direct native stream calls, real callback GC and safe retirement were verified. */
	return 0;
}
