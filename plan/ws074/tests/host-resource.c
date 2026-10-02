/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Observes actual owned responses and declared MIME selection without activating any DOM or Window. */

#include "page/page.h"
#include "net/net.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One matrix row names an independent MIME declaration and its actual processing outcome. */
struct resource_case {
	const char *mime;
	int status;
	enum dom_document_content content;
};

/* Count real native observations while retaining every failure. */
static unsigned checks;
/* Independent resource cleanup continues after failed observations. */
static unsigned failures;

static void resource_check(int condition, const char *name);
static void resource_mimes(void);
static void resource_data(void);
static int resource_files(const char *directory);
static int resource_file(const char *base, const char *href, const char *mime, enum dom_document_content content, int supported);
static void resource_http(const char *base);
static void resource_remote(const char *base, const char *href, const char *final_href, const char *mime, int response_status, const char *body, enum dom_document_content content);
static int resource_buffer(const struct wb_buffer *buffer, const char *expected);
static int resource_file_url(const char *base, const char *href, struct wb_buffer *out);

/*
 * Tests real local and loopback resource metadata through production URL and fetch code.
 */
int
main(
	int argc,
	char **argv)
{
	int status;
	int printed;

	/* The driver creates only ordinary files and an ephemeral HTTP server, never engine replacements. */
	if (argc != 3)
		return 2;
	resource_mimes();
	resource_data();
	status = resource_files(argv[1]);
	if (status != 0)
		return 2;
	resource_http(argv[2]);
	printed = printf("native resource metadata: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: actual response storage and declared MIME independently supplied every observation. */
	return 0;
}

/* Records a real resource observation without interrupting normal C response cleanup. */
static void
resource_check(
	int condition,
	const char *name)
{
	int printed;

	/* Earlier failures remain part of the final finite fixture outcome. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Uses actual valid and invalid declaration strings rather than URL or DOM assumptions. */
static void
resource_mimes(
	void)
{
	const struct resource_case cases[] = {
		{ "text/html", 0, DOM_CONTENT_HTML },
		{ "\t TEXT/HTML \r\n; charset=windows-1252", 0, DOM_CONTENT_HTML },
		{ "application/xhtml+xml", 0, DOM_CONTENT_XHTML },
		{ "IMAGE/SVG+XML; charset=utf-8", 0, DOM_CONTENT_SVG },
		{ "text/xml", 0, DOM_CONTENT_XML },
		{ "application/xml", 0, DOM_CONTENT_XML },
		{ "application/example+xml", 0, DOM_CONTENT_XML },
		{ "image/example+XML", 0, DOM_CONTENT_XML },
		{ "a/+xml", 0, DOM_CONTENT_XML },
		{ "text/html;", 0, DOM_CONTENT_HTML },
		{ "text/html;broken", 0, DOM_CONTENT_HTML },
		{ "text/html ; x=\"a;b\"", 0, DOM_CONTENT_HTML },
		{ "", EINVAL, DOM_CONTENT_SVG },
		{ " \t\r\n", EINVAL, DOM_CONTENT_SVG },
		{ "/html", EINVAL, DOM_CONTENT_SVG },
		{ "text/", EINVAL, DOM_CONTENT_SVG },
		{ "text/ ;p=1", EINVAL, DOM_CONTENT_SVG },
		{ "text html", EINVAL, DOM_CONTENT_SVG },
		{ "text /html", EINVAL, DOM_CONTENT_SVG },
		{ "text/ html", EINVAL, DOM_CONTENT_SVG },
		{ "text/html/other", EINVAL, DOM_CONTENT_SVG },
		{ "text/html, text/xml", EINVAL, DOM_CONTENT_SVG },
		{ "\ftext/html", EINVAL, DOM_CONTENT_SVG },
		{ "text/html\f", EINVAL, DOM_CONTENT_SVG },
		{ "text/ht\xc3\xa9ml", EINVAL, DOM_CONTENT_SVG },
		{ "text/plain", ENOTSUP, DOM_CONTENT_SVG },
		{ "application/octet-stream", ENOTSUP, DOM_CONTENT_SVG },
		{ "application/json", ENOTSUP, DOM_CONTENT_SVG },
		{ "text/html-extra", ENOTSUP, DOM_CONTENT_SVG },
		{ "image/svg+xml-extra", ENOTSUP, DOM_CONTENT_SVG }
	};
	const char embedded[] = "text/html\0extra";
	enum dom_document_content content;
	size_t index;
	size_t length;
	int status;

	/* Failed declarations retain an independent sentinel processing kind. */
	for (index = 0; index < sizeof(cases) / sizeof(cases[0]); index++) {
		content = DOM_CONTENT_SVG;
		length = strlen(cases[index].mime);
		status = page_document_content(cases[index].mime, length, &content);
		resource_check(status == cases[index].status && content == cases[index].content, cases[index].mime);
	}

	/* Explicit lengths cannot conceal an embedded NUL behind a C string terminator. */
	content = DOM_CONTENT_SVG;
	status = page_document_content(embedded, sizeof(embedded) - 1, &content);
	resource_check(status == EINVAL && content == DOM_CONTENT_SVG, "embedded NUL MIME rejected without output change");
	status = page_document_content(NULL, 0, &content);
	resource_check(status == EINVAL && content == DOM_CONTENT_SVG, "missing MIME storage rejected");
	status = page_document_content("text/html", 9, NULL);
	resource_check(status == EINVAL, "missing MIME output rejected");

	/* Succeeded: each declaration was classified through the actual native essence implementation. */
	return;
}

/* Uses real native data decoding and output failure cleanup without a network host. */
static void
resource_data(
	void)
{
	struct net_response response;
	enum dom_document_content content;
	int status;
	int same;

	/* A canonical data resource supplies all bytes and metadata, with its fragment excluded from the body. */
	status = page_fetch_response("https://example.invalid/base", "data:IMAGE/SVG+XML,%3Cr/%3E#mark", &response);
	resource_check(status == 0 && response.status == 200, "real data response successful status");
	same = resource_buffer(&response.body, "<r/>");
	resource_check(same, "data body percent decoded without URL fragment");
	same = resource_buffer(&response.content_type, "image/svg+xml");
	resource_check(same, "data actual MIME canonicalized by native data processor");
	same = resource_buffer(&response.url, "data:IMAGE/SVG+XML,%3Cr/%3E#mark");
	resource_check(same, "data actual resolved URL retained");
	status = page_document_content(wb_buffer_string(&response.content_type), response.content_type.length, &content);
	resource_check(status == 0 && content == DOM_CONTENT_SVG, "declared data MIME selects SVG independent of root r");
	net_response_release(&response);

	/* Existing forgiving base64 decoding still supplies ordinary owned resource bytes. */
	status = page_fetch_response("https://example.invalid/base", "data:application/xml;base64,PHIvPg==", &response);
	same = resource_buffer(&response.body, "<r/>");
	resource_check(status == 0 && same, "real base64 data response");
	same = resource_buffer(&response.content_type, "application/xml");
	resource_check(same, "base64 marker does not invent a MIME parameter");
	net_response_release(&response);

	/* Default data MIME remains plain text, rather than being promoted from apparent markup. */
	status = page_fetch_response("https://example.invalid/base", "data:,%3Cr/%3E", &response);
	same = resource_buffer(&response.content_type, "text/plain;charset=US-ASCII");
	resource_check(status == 0 && same, "default data MIME preserved");
	status = page_document_content(wb_buffer_string(&response.content_type), response.content_type.length, &content);
	resource_check(status == ENOTSUP, "plain text markup is not a declared XML document");
	net_response_release(&response);

	/* Failed real decoding and unsupported schemes publish no partial fields or successful status. */
	status = page_fetch_response("https://example.invalid/base", "data:application/xml;base64,!", &response);
	resource_check(status != 0 && response.status == 0, "invalid data decoding rejects successful response status");
	resource_check(response.body.length == 0 &&
	    response.url.length == 0 &&
	    response.content_type.length == 0, "invalid data leaves empty owned response");
	net_response_release(&response);
	status = page_fetch_response("https://example.invalid/base", "mailto:reader@example.invalid", &response);
	resource_check(status == EPROTONOSUPPORT &&
	    response.body.length == 0 &&
	    response.url.length == 0, "unsupported scheme leaves empty response");
	net_response_release(&response);
	status = page_fetch_response(NULL, "data:,x", &response);
	resource_check(status == EINVAL && response.status == 0, "invalid base leaves initialized empty response");
	net_response_release(&response);
	status = page_fetch_response("https://example.invalid/base", NULL, &response);
	resource_check(status == EINVAL && response.body.length == 0, "missing href leaves empty response");
	net_response_release(&response);
	status = page_fetch_response("https://example.invalid/base", "data:,x", NULL);
	resource_check(status == EINVAL, "missing response output rejected");

	/* Succeeded: every data result or error has followed ordinary response cleanup. */
	return;
}

/* Owns the ordinary local base path used by all independent file metadata cases. */
static int
resource_files(
	const char *directory)
{
	struct wb_buffer base;
	struct net_response response;
	int status;

	/* The driver supplies an actual absolute temporary directory, with no engine test switches. */
	wb_buffer_init(&base);
	status = wb_buffer_append_string(&base, directory);
	if (status == 0)
		status = wb_buffer_append_string(&base, "/base.html");
	if (status == 0)
		status = resource_file(wb_buffer_string(&base), "tree.xml?x=1#mark", "application/xml", DOM_CONTENT_XML, 1);
	if (status == 0)
		status = resource_file(wb_buffer_string(&base), "tree.SVG", "image/svg+xml", DOM_CONTENT_SVG, 1);
	if (status == 0)
		status = resource_file(wb_buffer_string(&base), "tree.xhtml", "application/xhtml+xml", DOM_CONTENT_XHTML, 1);
	if (status == 0)
		status = resource_file(wb_buffer_string(&base), "tree.htm", "text/html", DOM_CONTENT_HTML, 1);
	if (status == 0)
		status = resource_file(wb_buffer_string(&base), "tree.bin", "application/octet-stream", DOM_CONTENT_HTML, 0);
	if (status == 0)
		status = resource_file(wb_buffer_string(&base), "tree%20space.xml", "application/xml", DOM_CONTENT_XML, 1);
	if (status == 0) {
		status = page_fetch_response(wb_buffer_string(&base), "absent.xml", &response);
		resource_check(status != 0 && response.status == 0, "missing real local file rejects response");
		resource_check(response.body.length == 0 &&
		    response.url.length == 0 &&
		    response.content_type.length == 0, "missing local file leaves empty response fields");
		net_response_release(&response);
		status = 0;
	}

	/* The shared C base buffer has no lifetime beyond this independent file group. */
	wb_buffer_release(&base);
	if (status != 0)
		return status;

	/* Succeeded: all real file reads and ordinary metadata observations finished. */
	return 0;
}

/* Observes one ordinary file and its actual resolved URL independently of its markup root. */
static int
resource_file(
	const char *base,
	const char *href,
	const char *mime,
	enum dom_document_content expected,
	int supported)
{
	struct net_response response;
	struct wb_buffer target;
	enum dom_document_content content;
	int status;
	int same;

	/* Actual local bytes remain unchanged by document type selection. */
	status = page_fetch_response(base, href, &response);
	resource_check(status == 0 && response.status == 200, href);
	same = resource_buffer(&response.body, "<r/>");
	resource_check(same, "real local body copied exactly");
	same = resource_buffer(&response.content_type, mime);
	resource_check(same, "generic real path extension supplies local MIME metadata");
	content = DOM_CONTENT_HTML;
	status = page_document_content(wb_buffer_string(&response.content_type), response.content_type.length, &content);
	if (supported)
		resource_check(status == 0 && content == expected, "real local declared type selects expected processing kind");
	else
		resource_check(status == ENOTSUP && content == DOM_CONTENT_HTML, "unknown local extension does not promote markup");

	/* Independent native URL parsing supplies the complete expected file URL, including query and fragment. */
	wb_buffer_init(&target);
	status = resource_file_url(base, href, &target);
	if (status == 0) {
		same = resource_buffer(&response.url, wb_buffer_string(&target));
		resource_check(same, "actual complete final file URL preserves path query and fragment");
	}

	/* Every successful or failed local observation releases all independent owned C fields. */
	wb_buffer_release(&target);
	net_response_release(&response);
	if (status != 0)
		return status;

	/* Succeeded: the actual resource and independent URL observations agree. */
	return 0;
}

/* Uses a real loopback server for redirects, MIME headers and normal HTTP error responses. */
static void
resource_http(
	const char *base)
{
	/* Misleading names and root namespaces cannot replace actual response metadata. */
	resource_remote(base, "entry.bin", "odd.html", "IMAGE/SVG+XML; charset=utf-8", 200, "<r/>", DOM_CONTENT_SVG);
	resource_remote(base, "plain.svg", "plain.svg", "text/xml", 200, "<html/>", DOM_CONTENT_XML);
	resource_remote(base, "missing.xml", "missing.xml", "text/html", 404, "missing", DOM_CONTENT_HTML);

	/* Succeeded: actual network responses, including 404, were observed without DOM activation. */
	return;
}

/* Observes exact real HTTP output, then checks the unchanged legacy byte-only fetcher independently. */
static void
resource_remote(
	const char *base,
	const char *href,
	const char *final_href,
	const char *mime,
	int response_status,
	const char *body,
	enum dom_document_content expected)
{
	struct net_response response;
	struct wb_buffer final_url;
	struct wb_buffer bytes;
	struct wb_buffer legacy_url;
	enum dom_document_content content;
	int status;
	int same;

	/* Real HTTP status and headers remain data, including successful transport of a 404 response. */
	status = page_fetch_response(base, href, &response);
	resource_check(status == 0 && response.status == response_status, "actual HTTP response status preserved");
	same = resource_buffer(&response.content_type, mime);
	resource_check(same, "actual HTTP Content-Type retained exactly");
	same = resource_buffer(&response.body, body);
	resource_check(same, "actual HTTP response body copied exactly");
	content = DOM_CONTENT_XHTML;
	status = page_document_content(wb_buffer_string(&response.content_type), response.content_type.length, &content);
	resource_check(status == 0 && content == expected, "actual declared HTTP MIME wins over filename and root");

	/* Final response URL must follow a real redirect, not retain the requested alias. */
	wb_buffer_init(&final_url);
	status = page_resolve_location(base, final_href, &final_url);
	if (status == 0) {
		same = resource_buffer(&response.url, wb_buffer_string(&final_url));
		resource_check(same, "actual final redirect URL retained");
	}

	/* Existing byte-only consumers keep the same native body and final URL behavior. */
	wb_buffer_init(&bytes);
	wb_buffer_init(&legacy_url);
	status = page_fetch(base, href, &bytes, &legacy_url);
	same = resource_buffer(&bytes, body);
	resource_check(status == 0 && same, "unchanged legacy page_fetch body");
	same = resource_buffer(&legacy_url, wb_buffer_string(&final_url));
	resource_check(same, "unchanged legacy page_fetch final URL");
	wb_buffer_release(&final_url);
	wb_buffer_release(&bytes);
	wb_buffer_release(&legacy_url);
	net_response_release(&response);

	/* Succeeded: no real HTTP response or working buffer outlives this finite observation. */
	return;
}

/* Compares complete owned bytes without a terminator, interning or prefix-equality assumption. */
static int
resource_buffer(
	const struct wb_buffer *buffer,
	const char *expected)
{
	size_t length;
	int same;

	/* Complete native length must agree before any byte observation. */
	length = strlen(expected);
	if (buffer->length != length)
		return 0;
	if (length == 0)
		return 1;
	same = memcmp(buffer->data, expected, length);

	/* Succeeded: actual complete bytes determine the comparison. */
	return same == 0;
}

/* Resolves the expected file URL using only independent native URL operations. */
static int
resource_file_url(
	const char *base,
	const char *href,
	struct wb_buffer *out)
{
	struct wb_buffer text;
	struct net_url base_url;
	struct net_url target;
	int status;

	/* Absolute ordinary filesystem input becomes a real encoded base URL. */
	wb_buffer_init(&text);
	status = net_url_from_file_path(base, &text);
	if (status == 0)
		status = net_url_parse(wb_buffer_string(&text), text.length, NULL, &base_url);
	wb_buffer_release(&text);
	if (status != 0)
		return status;
	status = net_url_parse(href, strlen(href), &base_url, &target);
	net_url_release(&base_url);
	if (status != 0)
		return status;
	status = net_url_serialize(&target, 0, out);
	net_url_release(&target);
	if (status != 0)
		return status;

	/* Succeeded: query and fragment remain part of the actual resolved resource URL. */
	return 0;
}
