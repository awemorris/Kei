/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks namespace cells through cloning, primary teardown and real collection.
 */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Failed independent identity checks determine this test's exit status. */
static unsigned failures;

/* Every field assertion contributes to the final test total. */
static unsigned checks;

static void namespace_check(int condition, const char *name);
static int namespace_sample(struct vm_heap *heap, unsigned kind, struct vm_cell **root);
static int namespace_case(unsigned kind);

/*
 * Verifies original and cloned namespace identities after their embedder dies.
 */
int
main(
	void)
{
	unsigned kind;
	int error;
	int printed;

	/* Retains the original raw node, a cloned wrapper and a cloned raw node. */
	for (kind = 0; kind < 3; kind++) {
		error = namespace_case(kind);
		if (error != 0)
			return 2;
	}

	/* Reports identity failures separately from fixture construction errors. */
	printed = printf("namespace GC checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any lost namespace component makes this regression fail. */
	if (failures != 0)
		return 1;

	/* Succeeded: every selected graph retained its complete namespace identity. */
	return 0;
}

/* Records field identity without suppressing other independently useful checks. */
static void
namespace_check(
	int condition,
	const char *name)
{
	int printed;

	/* Counts this independent namespace field observation until main reports. */
	checks++;

	/* The existing failure count rejects the case even if its report is lost. */
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			return;
	}

	/* Succeeded: this assertion has been recorded. */
	return;
}

/* Constructs a runtime URI, selecting one original or cloned graph as its root. */
static int
namespace_sample(
	struct vm_heap *heap,
	unsigned kind,
	struct vm_cell **root)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct dom_node *node;
	struct bind_window *window;
	struct bind_host host;
	struct wb_units units;
	struct js_syntax_error syntax;
	const char *source;
	vm_value answer;
	int error;

	/* Creates an ordinary explicitly owned primary realm and its intrinsics. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;

	/* Installs ordinary intrinsics before namespaced node creation runs. */
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The primary Document has no retained asynchronous host operations. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Installs the ordinary primary binding after its Document exists. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The full URI is a runtime concatenation, not a constant atom in code. */
	source = "document.createElementNS(['urn','runtime','not-an-atom'].join(':'),'prefix:Local')";
	if (kind != 0)
		source = "document.createElementNS(['urn','runtime','not-an-atom'].join(':'),'prefix:Local').cloneNode(true)";

	/* Converts fixture input independently of parsing and execution errors. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Executes the production namespaced creation and clone operations. */
	error = js_run_script(realm, units.data, units.length, 0, &answer, &syntax);
	wb_units_release(&units);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* A raw DOM node must trace its URI even without an explicit wrapper root. */
	node = bind_node_of(answer);
	*root = &node->cell;
	if (kind == 1)
		*root = vm_value_as_cell(answer);

	/* Removes all primary C roots before the selected graph is collected. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: only the caller's selected embedding root retains the graph. */
	return 0;
}

/* Collects with explicit roots and checks each retained namespace component. */
static int
namespace_case(
	unsigned kind)
{
	struct vm_heap *heap;
	struct vm_cell *root;
	struct dom_node *node;
	struct dom_element *element;
	int error;
	int same;

	/* Real-stack scanning protects construction until an explicit root exists. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;

	/* Registers the selected graph's persistent root after conservative construction setup. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	root = NULL;
	error = vm_heap_add_root(heap, &root);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Constructs this case only after the caller owns a registered root slot. */
	error = namespace_sample(heap, kind, &root);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_heap_destroy(heap);
		return error;
	}

	/* Excludes obsolete construction words so only the selected graph retains cells. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_collect(heap);
	node = (struct dom_node *)root;
	if (kind == 1)
		node = bind_node_of(vm_value_cell(root));

	/* Checks exact runtime URI data after all parent embedding roots are gone. */
	element = (struct dom_element *)node;
	same = vm_string_equal_ascii(element->namespace_uri, "urn:runtime:not-an-atom");
	namespace_check(same, "runtime namespace URI survives GC");

	/* Prefix and local-name atoms retain their original case independently. */
	same = vm_string_equal_ascii(element->prefix, "prefix");
	namespace_check(same, "namespace prefix survives GC");

	/* The cloned or original graph keeps its case-sensitive local identity too. */
	same = vm_string_equal_ascii(element->local_name, "Local");
	namespace_check(same, "case-preserving local name survives GC");

	/* An arbitrary URI must never acquire a built-in HTML or SVG classification. */
	same = 0;
	if (element->ns == DOM_NS_OTHER)
		same = 1;
	namespace_check(same, "custom namespace keeps its own classification");

	/* Releasing the selected root permits normal collection and complete teardown. */
	root = NULL;
	vm_heap_collect(heap);
	vm_heap_remove_root(heap, &root);
	vm_heap_destroy(heap);

	/* Succeeded: this selected graph survived and was safely released. */
	return 0;
}
