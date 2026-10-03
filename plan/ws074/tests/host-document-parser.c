/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies owned HTML parser stacks and child finalization during actual heap collection. */

#include "bind/internal.h"
#include "html/html.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Independent parser lifetime observations survive fixture teardown. */
static unsigned checks;
/* All failed observations remain visible in the final native result. */
static unsigned failures;

static void parser_check(int condition, const char *name);
static int parser_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int parser_case(struct vm_realm *realm);
static int parser_feed(struct html_parser *parser, const char *source);

/*
 * Runs parser lifetime observations against a genuine managed iframe context.
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
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Only the direct lifetime case disables conservative stack scanning. */
	status = parser_case(realm);
	if (status != 0) {
		vm_heap_set_stack_base(heap, __builtin_frame_address(0));
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Restore conservative construction before completed native teardown. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Publish complete observations after the embedding has released its roots. */
	printed = printf("native document parser lifetime: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any hidden stack loss or lingering child context rejects the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native parser reachability and context finalization were verified. */
	return 0;
}

/* Records one native observation while preserving later independent checks. */
static void
parser_check(
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

	/* Succeeded: this independent observation contributes to the final outcome. */
	return;
}

/* Constructs genuine DOM and managed contexts through the ordinary interpreter. */
static int
parser_script(
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
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release the borrowed source after interpreter execution was checked. */
	wb_units_release(&units);

	/* Succeeded: a normal interpreter value is available to the embedding. */
	return 0;
}

/* Feeds production HTML input without script-side parser substitution. */
static int
parser_feed(
	struct html_parser *parser,
	const char *source)
{
	struct wb_units units;
	int status;

	/* The same UTF-16 parser accepts split HTML input in ordinary browser execution. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Feed only successfully converted ordinary input to the native parser. */
	status = html_parser_feed(parser, units.data, units.length);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release borrowed units after native parsing was checked. */
	wb_units_release(&units);

	/* Succeeded: complete and partial tokens now belong to the parser. */
	return 0;
}

/* Verifies hidden open-element roots without conservatively scanning native locals. */
static int
parser_case(
	struct vm_realm *realm)
{
	struct vm_realm *child_realm;
	struct bind_window *child;
	struct dom_node *frame;
	struct dom_node *open_node;
	struct dom_node *walk;
	struct html_parser *parser;
	struct html_parser *standalone;
	struct vm_cell *owner_root;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value answer;
	uintptr_t child_address;
	uintptr_t open_address;
	int rooted;
	int status;

	/* No unregistered root or uncreated standalone parser participates in teardown. */
	standalone = NULL;
	owner_root = NULL;
	rooted = 0;

	/* Ordinary DOM and contentWindow access install a genuine managed child. */
	status = parser_script(realm,
			       "var root=document.createElement('div');document.appendChild(root);"
			       "var f=document.createElement('iframe');root.appendChild(f);f.contentWindow;f",
			       &answer);
	if (status != 0)
		goto cleanup;
	frame = bind_node_of(answer);
	if (frame == NULL || frame->type != DOM_ELEMENT) {
		status = EIO;
		goto cleanup;
	}

	/* Resolve the checked element's actual managed child realm. */
	child_realm = (struct vm_realm *)((struct dom_element *)frame)->child_context;
	if (child_realm == NULL || child_realm->host == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* The genuine native host supplies the parser-owning child Document. */
	child = child_realm->host;
	child_address = (uintptr_t)&child_realm->cell;
	status = parser_script(realm, "f=null", &answer);
	if (status != 0)
		goto cleanup;

	/* The native Window owns the actual parser, after publishing its tracing edge. */
	while (child->document->node.first_child != NULL)
		dom_remove(child->document->node.first_child);
	status = html_parser_create(&parser, child->document, 1);
	if (status != 0)
		goto cleanup;
	child->document_parser = parser;
	html_parser_transfer_ownership(parser);
	status = parser_feed(parser, "<!doctype html><html><head></head><body><div><span>");
	if (status != 0)
		goto cleanup;

	/* Remove an open subtree so only the parser's native stacks retain its elements. */
	walk = child->document->node.first_child;
	if (walk == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Skip the generated doctype to inspect the document element. */
	walk = walk->next;
	if (walk == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Follow the checked native element into its first generated child. */
	walk = walk->first_child;
	if (walk == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Skip the generated doctype to inspect the document element. */
	walk = walk->next;
	if (walk == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Follow the checked native element into its first generated child. */
	walk = walk->first_child;
	if (walk == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Save an unrooted address only after following the generated open subtree. */
	open_node = walk;
	open_address = (uintptr_t)&open_node->cell;
	dom_remove(open_node);
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_stats(realm->heap, &before);
	vm_heap_collect(realm->heap);
	vm_heap_stats(realm->heap, &after);
	parser_check(after.collections > before.collections, "actual collection with conservative stack disabled");
	found = vm_heap_find_cell(realm->heap, open_address);
	parser_check(found != NULL, "owned parser retains detached open element absent from document graph");
	if (found == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Resume the actual retained parser stack with the remaining input. */
	status = parser_feed(parser, "text</span></div></body></html>");
	if (status != 0)
		goto cleanup;
	status = html_parser_finish(parser);
	if (status != 0)
		goto cleanup;
	if (open_node->first_child == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Observe the text inserted into the surviving detached open subtree. */
	parser_check(open_node->first_child->first_child != NULL, "native parser resumes into retained detached stack after GC");

	/* Destroyed parser stacks no longer retain a detached subtree. */
	child->document_parser = NULL;
	html_parser_destroy(parser);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, open_address);
	parser_check(found == NULL, "destroying completed owned parser releases hidden stack cells");

	/* Leave a second stream incomplete while a saved child realm owns its native parser. */
	status = html_parser_create(&parser, child->document, 1);
	if (status != 0)
		goto cleanup;
	child->document_parser = parser;
	html_parser_transfer_ownership(parser);
	status = parser_feed(parser, "<html><head><title>unfinished");
	if (status != 0)
		goto cleanup;
	owner_root = &child_realm->cell;
	status = vm_heap_add_root(realm->heap, &owner_root);
	if (status != 0)
		goto cleanup;

	/* The explicit child owner root is registered only for this retirement observation. */
	rooted = 1;
	dom_remove(frame);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	parser_check(found != NULL && child->detached, "saved retired child retains its unfinished owned stream");
	vm_heap_remove_root(realm->heap, &owner_root);
	rooted = 0;
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	parser_check(found == NULL, "unfinished parser does not permanently root forgotten child realm");

	/* A standalone parser still independently traces its open subtree until destruction. */
	status = parser_script(realm,
			       "var g=document.createElement('iframe');root.appendChild(g);g.contentWindow;g", &answer);
	if (status != 0)
		goto cleanup;
	frame = bind_node_of(answer);
	if (frame == NULL || frame->type != DOM_ELEMENT) {
		status = EIO;
		goto cleanup;
	}

	/* Resolve the checked element's actual managed child realm. */
	child_realm = (struct vm_realm *)((struct dom_element *)frame)->child_context;
	if (child_realm == NULL || child_realm->host == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* The genuine native host supplies the parser-owning child Document. */
	child = child_realm->host;
	child_address = (uintptr_t)&child_realm->cell;
	status = html_parser_create(&parser, child->document, 1);
	if (status != 0)
		goto cleanup;

	/* Track the independently rooted parser until explicit destruction. */
	standalone = parser;
	status = parser_feed(parser, "<html><body><p>standalone");
	if (status != 0)
		goto cleanup;
	status = parser_script(realm, "g=null", &answer);
	if (status != 0)
		goto cleanup;
	dom_remove(frame);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	parser_check(found != NULL, "ordinary standalone parser preserves independent tracing contract");
	html_parser_destroy(standalone);
	standalone = NULL;
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, child_address);
	parser_check(found == NULL, "standalone parser teardown releases retired document context");

	/* Every ownership sample completed before shared fixture teardown. */
	status = 0;

cleanup:
	/* Remove only acquired roots and a parser that still has independent ownership. */
	if (rooted)
		vm_heap_remove_root(realm->heap, &owner_root);
	html_parser_destroy(standalone);
	if (status != 0)
		return status;

	/* Succeeded: both ownership modes and unfinished stream finalization were verified. */
	return 0;
}
