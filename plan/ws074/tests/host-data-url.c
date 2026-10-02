/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Observes real body-decoding errors independently of unrelated native MIME fallback. */

#include "page/page.h"
#include "net/net.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* One ordinary URL supplies real decoded bytes, MIME and an expected native error outcome. */
struct data_case {
	const char *url;
	int error;
	const char *body;
	const char *mime;
};

/* Count independent body, MIME and response observations. */
static unsigned checks;
/* Preserve every failed observation while ordinary C cleanup continues. */
static unsigned failures;

static void data_check(int condition, const char *name);
static int data_cases(void);
static int data_case_run(const struct data_case *test);
static int data_bytes(const struct wb_buffer *bytes, const char *expected);
static void data_consumers(void);

/*
 * Tests production data URL failure routing without replacing its native decoder or MIME parser.
 */
int
main(
	void)
{
	int status;
	int printed;

	/* The same real native parser supplies valid and invalid-body observations. */
	status = data_cases();
	if (status != 0)
		return 2;
	data_consumers();
	printed = printf("native data URL error routing: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: invalid body errors and MIME-only fallback remain separate native outcomes. */
	return 0;
}

/* Records a semantic observation while retaining normal native cleanup on failure. */
static void
data_check(
	int condition,
	const char *name)
{
	int printed;

	/* Earlier failures remain in the final bounded fixture result. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Covers actual forbidden characters, padding and partial decode alongside unchanged valid defaults. */
static int
data_cases(
	void)
{
	const struct data_case tests[] = {
		{ "data:application/xml;base64,!", EINVAL, "", "" },
		{ "data:application/xml;base64,A", EINVAL, "", "" },
		{ "data:application/xml;base64,!!!!", EINVAL, "", "" },
		{ "data:application/xml;base64,AA=A", EINVAL, "", "" },
		{ "data:application/xml;base64,YQ===", EINVAL, "", "" },
		{ "data:application/xml;base64,PHIvPg=Q", EINVAL, "", "" },
		{ "data:application/xml;base64,PHIvPg==!", EINVAL, "", "" },
		{ "data:application/xml;base64,%00", EINVAL, "", "" },
		{ "data:bad%20type;base64,!", EINVAL, "", "" },
		{ "data:application/xml", EINVAL, "", "" },
		{ "data:application/xml;base64,YQ==", 0, "a", "application/xml" },
		{ "data:application/xml;base64,YQ", 0, "a", "application/xml" },
		{ "data:application/xml;base64,YQ%0A==", 0, "a", "application/xml" },
		{ "data:application/xml;base64,", 0, "", "application/xml" },
		{ "data:bad%20type;base64,YQ==", 0, "a", "text/plain;charset=US-ASCII" },
		{ "data:,markup", 0, "markup", "text/plain;charset=US-ASCII" },
		{ "data:;charset=UTF-8,a", 0, "a", "text/plain;charset=UTF-8" },
		{ "data:TEXT/HTML,%3Cp%3E#fragment", 0, "<p>", "text/html" }
	};
	size_t index;
	int status;

	/* Each independent case owns and releases its genuine URL and native decoded result. */
	for (index = 0; index < sizeof(tests) / sizeof(tests[0]); index++) {
		status = data_case_run(&tests[index]);
		if (status != 0)
			return status;
	}

	/* Succeeded: no earlier case supplied parser state or accidental output to another case. */
	return 0;
}

/* Parses actual URL text and observes all native body and MIME cleanup fields. */
static int
data_case_run(
	const struct data_case *test)
{
	struct net_url url;
	struct net_data decoded;
	size_t length;
	int status;
	int same;

	/* The native URL parser and genuine data processor receive ordinary independent input. */
	length = strlen(test->url);
	status = net_url_parse(test->url, length, NULL, &url);
	if (status != 0)
		return status;
	status = net_data_parse(&url, &decoded);
	data_check(status == test->error, test->url);
	same = data_bytes(&decoded.body, test->body);
	data_check(same, "actual native body bytes or empty failed decode");
	same = data_bytes(&decoded.mime, test->mime);
	data_check(same, "actual native MIME or empty failed body outcome");
	if (status != 0) {
		data_check(decoded.body.data == NULL && decoded.mime.data == NULL, "failed decode releases partial native C allocations");
	}

	/* Both failed and successful native outputs support ordinary idempotent release. */
	net_data_release(&decoded);
	net_url_release(&url);

	/* Succeeded: this independent semantic case has no remaining parser allocations. */
	return 0;
}

/* Compares actual owned bytes without relying on terminators or partial-prefix equality. */
static int
data_bytes(
	const struct wb_buffer *bytes,
	const char *expected)
{
	size_t length;
	int same;

	/* Complete lengths agree before any actual byte observation. */
	length = strlen(expected);
	if (bytes->length != length)
		return 0;
	if (length == 0)
		return 1;
	same = memcmp(bytes->data, expected, length);

	/* Succeeded: complete native bytes determine the comparison. */
	return same == 0;
}

/* Confirms both actual metadata and existing byte-only resource consumers reject a malformed body. */
static void
data_consumers(
	void)
{
	struct net_response response;
	struct wb_buffer bytes;
	int status;

	/* New metadata ownership cannot publish an invented status200 for a genuine decoding failure. */
	status = page_fetch_response("https://example.invalid/base", "data:application/xml;base64,!", &response);
	data_check(status == EINVAL && response.status == 0, "metadata consumer preserves body EINVAL");
	data_check(response.body.length == 0 && response.content_type.length == 0, "metadata consumer has no partial body or MIME");
	data_check(response.url.length == 0 && response.body.data == NULL, "metadata consumer releases failed response allocations");
	net_response_release(&response);

	/* Existing consumers receive the same real decoder failure without a synthetic successful empty body. */
	wb_buffer_init(&bytes);
	status = page_fetch("https://example.invalid/base", "data:application/xml;base64,!", &bytes, NULL);
	data_check(status == EINVAL && bytes.length == 0, "legacy byte-only consumer preserves body EINVAL");
	wb_buffer_release(&bytes);

	/* Succeeded: ordinary actual consumers share the repaired native failure semantics. */
	return;
}
