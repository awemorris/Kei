/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks SameObject collection caches and root retention using actual production GC. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Assertions accumulate across callback-driven and embedding-driven collection. */
static unsigned checks;
/* Failed behavioral checks determine the final exit status. */
static unsigned failures;
/* The embedding restores this construction stack boundary after explicit collection. */
static const void *construction_stack;

static void collection_check(int condition, const char *name);
static int collection_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int collection_case(struct vm_heap *heap);

/*
 * Verifies collection caches and collectible DOM cycles through ordinary bindings.
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
	printed = printf("collection GC checks: %u/%u passed\n", checks - failures, checks);
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

/* Verifies cache cycles, collection-only roots and eventual reclamation with real GC. */
static int
collection_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct dom_node *node;
	struct dom_element *foreign;
	struct vm_string *href_name;
	struct vm_string *href_value;
	vm_value foreign_wrapper;
	struct vm_object *wrapper;
	struct vm_cell *found;
	struct bind_window *window;
	struct bind_host host;
	vm_value answer;
	uintptr_t children_address;
	uintptr_t links_address;
	uintptr_t forms_address;
	uintptr_t root_address;
	uintptr_t member_address;
	int same;
	int error;

	/* Ordinary embedding initialization establishes the real production graph. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The manually owned primary Document is distinct from the test's collectible XML root. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* No asynchronous host effect or test-only engine behavior participates. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Only the XML Document variable keeps the two observed SameObject caches. */
	error = collection_script(realm,
				  "var xml=document.implementation.createDocument(null,'Root');"
				  "var member=xml.createElementNS('http://www.w3.org/1999/xhtml','a');"
				  "member.setAttribute('href','');xml.documentElement.appendChild(member);"
				  "xml.documentElement.children;xml.links;xml.forms;member=null;xml.documentElement",
				  &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The C DOM boundary tests foreign attribute namespaces without requiring absent setAttributeNS. */
	foreign_wrapper = answer;
	node = bind_node_of(answer);
	foreign = (struct dom_element *)node->first_child;
	href_name = vm_atom_from_ascii(heap, "href");
	if (href_name == NULL) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* A nonempty value keeps namespace exclusion independent from empty-value filtering. */
	href_value = vm_atom_from_ascii(heap, "foreign");
	if (href_value == NULL) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Remove the ordinary href, then add a same-local-name attribute in a different namespace. */
	error = dom_element_remove_attribute(foreign, href_name);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The same local name in a foreign namespace cannot stand for an HTML href. */
	error = dom_element_add_attribute(foreign, DOM_NS_OTHER, NULL, href_name, href_value);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The saved accessor observes the foreign attribute through ordinary production script. */
	error = collection_script(realm, "xml.links.length===0", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* A foreign href attribute never supplies the HTML link-membership predicate. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "foreign namespace href does not establish a link");

	/* Restoring a no-namespace attribute updates the same live links collection immediately. */
	error = dom_element_add_attribute(foreign, DOM_NS_NONE, NULL, href_name, href_value);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* The second observation consumes the same collection with changed namespace membership. */
	error = collection_script(realm, "xml.links.length===1", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Namespace-aware membership changes without recreating the collection wrapper. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "no-namespace href establishes a link beside a foreign href");
	answer = foreign_wrapper;

	/* Record selected addresses without introducing any extra registered GC roots. */
	node = bind_node_of(answer);
	root_address = (uintptr_t)node;
	member_address = (uintptr_t)node->first_child;
	children_address = (uintptr_t)node->children_collection;
	links_address = (uintptr_t)node->document->links_collection;
	forms_address = (uintptr_t)node->document->forms_collection;

	/* Only registered roots and native traces may retain the observed addresses. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, children_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "node alone preserves SameObject children cache without stack roots");
	found = vm_heap_find_cell(heap, links_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "Document alone preserves SameObject links cache without stack roots");

	/* The same Document trace retains the forms accessor independently of link membership. */
	found = vm_heap_find_cell(heap, forms_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "Document alone preserves SameObject forms cache without stack roots");
	vm_heap_set_stack_base(heap, construction_stack);

	/* Both getters still return precisely the earlier cache rather than equivalent new objects. */
	error = collection_script(realm, "xml.documentElement.children", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Compare the observed wrapper address with the earlier cache identity. */
	wrapper = (struct vm_object *)vm_value_as_cell(answer);
	same = 0;
	if ((uintptr_t)wrapper == children_address)
		same = 1;
	collection_check(same, "children identity unchanged after cache-only GC");
	error = collection_script(realm, "xml.links", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Compare the observed wrapper address with the earlier cache identity. */
	wrapper = (struct vm_object *)vm_value_as_cell(answer);
	same = 0;
	if ((uintptr_t)wrapper == links_address)
		same = 1;
	collection_check(same, "links identity unchanged after cache-only GC");

	/* The forms getter must return the identical cached wrapper after a real collection. */
	error = collection_script(realm, "xml.forms", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Compare actual wrapper identity rather than an equivalent fresh result. */
	wrapper = (struct vm_object *)vm_value_as_cell(answer);
	same = 0;
	if ((uintptr_t)wrapper == forms_address)
		same = 1;
	collection_check(same, "forms identity unchanged after cache-only GC");

	/* The collection becomes the sole external edge into an otherwise unreachable tree. */
	error = collection_script(realm, "var saved=xml.documentElement.children;xml=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Only registered roots and native traces may retain the observed addresses. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, root_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "collection-only graph retains actual root");
	found = vm_heap_find_cell(heap, member_address);
	same = 0;
	if (found != NULL)
		same = 1;
	collection_check(same, "collection-only graph retains unwrapped member");
	vm_heap_set_stack_base(heap, construction_stack);

	/* The member can acquire its original XML-realm wrapper after explicit GC. */
	error = collection_script(realm,
				  "saved.length===1 && saved[0].localName==='a' && saved.item(0)===saved[0]", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Inspect the completed script result independently of execution status. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	collection_check(same, "unwrapped member remains usable after collection-only GC");

	/* Removing the final script root leaves only collectible node/Document/cache cycles. */
	error = collection_script(realm, "saved=null;true", &answer);
	if (error != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		return error;
	}

	/* Only registered roots and native traces may retain the observed addresses. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, root_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last collection root drop reclaims actual DOM root");
	found = vm_heap_find_cell(heap, member_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last collection root drop reclaims actual member");
	found = vm_heap_find_cell(heap, children_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last root drop reclaims children cache cycle");
	found = vm_heap_find_cell(heap, links_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last root drop reclaims links cache cycle");

	/* The forms cycle must disappear alongside the last otherwise unreachable Document. */
	found = vm_heap_find_cell(heap, forms_address);
	same = 0;
	if (found == NULL)
		same = 1;
	collection_check(same, "last root drop reclaims forms cache cycle");
	vm_heap_set_stack_base(heap, construction_stack);

	/* Manual primary ownership is released after all collectible-cycle observations. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);

	/* Succeeded: cache identity and root lifetime used ordinary production GC. */
	return 0;
}
