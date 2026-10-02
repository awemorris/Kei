/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks fatal parser-script errors at the initial and nested write boundaries.
 * Ordinary JavaScript exceptions are separate; ENOMEM must stop the parse.
 */

#include "html/html.h"
#include "dom/dom.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * One stack-owned callback attempt borrows its live parser and owns inserted units.
 * calls counts the observed boundaries; fail_at chooses which reports ENOMEM.
 */
struct write_probe {
	struct html_parser *parser;
	struct wb_units text;
	int calls;
	int fail_at;
};

static int probe_script(void *context, struct dom_element *script);
static void probe_failure(struct vm_heap *heap, int fail_at);

/*
 * Verifies that fatal script callbacks cannot produce a successful parse.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	int error;

	/* Keeps the test documents visible to the conservative collector. */
	error = vm_heap_create(&heap, 0);
	assert(error == 0);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));

	/* Exercises both an initial failure and one inside a reentrant write. */
	probe_failure(heap, 1);
	probe_failure(heap, 2);

	/* Reclaims the shared heap after both parser attempts have released their roots. */
	vm_heap_destroy(heap);

	/* Publishes the verified cases, refusing a lost test report. */
	error = puts("write fatal errors: 2 cases passed");
	if (error == EOF)
		return 1;

	/* Succeeded: fatal failures stayed visible through parser cleanup. */
	return 0;
}

/* Makes the selected script callback fail without a production fault switch. */
static int
probe_script(
	void *context,
	struct dom_element *script)
{
	struct write_probe *probe;
	int error;

	UNUSED_PARAMETER(script);

	/* The callback count identifies the boundary that fails in this attempt. */
	probe = context;
	probe->calls++;
	if (probe->calls == probe->fail_at)
		return ENOMEM;

	/* The outer callback inserts a script that will hit the fatal boundary. */
	error = html_parser_write(probe->parser, probe->text.data, probe->text.length);
	if (error != 0)
		return error;

	/* Succeeded: no callback in this inserted prefix failed. */
	return 0;
}

/* Confirms failure survives restoration of the unread source and parser finish. */
static void
probe_failure(
	struct vm_heap *heap,
	int fail_at)
{
	struct write_probe probe;
	struct dom_document *document;
	struct wb_units source;
	const char *markup;
	size_t length;
	int error;

	/* Gives the callback one complete parser-inserted script. */
	memset(&probe, 0, sizeof(probe));
	probe.fail_at = fail_at;
	markup = "<script></script>";
	length = strlen(markup);
	wb_units_init(&probe.text);
	error = wb_utf8_to_units((const unsigned char *)markup, length, &probe.text);
	assert(error == 0);

	/* The source has a suffix that must not conceal a failed write. */
	markup = "<script></script><p>unread source</p>";
	length = strlen(markup);
	wb_units_init(&source);
	error = wb_utf8_to_units((const unsigned char *)markup, length, &source);
	assert(error == 0);

	/* Owns the document and parser until all terminal outcomes are checked. */
	document = dom_document_create(heap);
	assert(document != NULL);

	/* The parser roots the live document until the test destroys it. */
	error = html_parser_create(&probe.parser, document, 1);
	assert(error == 0);

	/* The callback borrows this stack-owned attempt for the parser lifetime. */
	html_parser_set_script_hook(probe.parser, probe_script, &probe);

	/* A fatal callback must fail feed, finish and any subsequent insertion. */
	error = html_parser_feed(probe.parser, source.data, source.length);
	assert(error == ENOMEM);
	assert(probe.calls == fail_at);
	error = html_parser_finish(probe.parser);
	assert(error == ENOMEM);
	error = html_parser_write(probe.parser, NULL, 0);
	assert(error == ENOMEM);

	/* Releases every parser-owned and test-owned buffer after the failure. */
	html_parser_destroy(probe.parser);
	wb_units_release(&source);
	wb_units_release(&probe.text);

	/* Succeeded: all failed parser outcomes were observed and owned buffers released. */
	return;
}
