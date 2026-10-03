/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies strict native XML syntax, actual namespaces, pinned resources and bounded model lifetime. */

#include "xml/xml.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One independent input records whether this finite parser profile must accept, reject or defer it. */
struct xml_case {
	const char *input;
	int status;
};

/* Ordinary valid and invalid inputs exercise real XML syntax rather than implementation branches. */
static const struct xml_case xml_cases[] = {
    {"<r/>", 0},
    {" <r></r> \n", 0},
    {"<?xml version='1.0' encoding='uTf-8' standalone='yes'?><r/>", 0},
    {"\xef\xbb\xbf<?xml version='1.0'?><r/>", 0},
    {"<r a='&amp;&lt;&gt;&quot;&apos;'>&amp;&lt;&gt;&quot;&apos;&#65;&#x1f600;</r>", 0},
    {"<!--a--><?p data?><r><![CDATA[<&]]><!--x--></r><?after?>", 0},
    {"<r xmlns='urn:a' a='v'><q xmlns=''><z/></q></r>", 0},
    {"<r xmlns:p='urn:p'><p:q p:a='1' a='2'/></r>", 0},
    {"<r xml:lang='en' xml:space='preserve'/>", 0},
    {"<r xmlns:xml='http://www.w3.org/XML/1998/namespace'/>", 0},
    {"<\xce\xb1 \xce\xb2='x'/>", 0},
    {"<\xf0\x9f\x98\x80/>", 0},
    {"<r a='&#13;&#9;&#10;'/> ", 0},
    {"<r xmlns:p='urn:a'><p:q xmlns:p='urn:b'><p:z/></p:q><p:x/></r>", 0},
    {"<r xmlns:p='urn:&amp;p' p:a='x'/>", 0},
    {"", EINVAL},
    {"<!--a-->", EINVAL},
    {"<r>", EINVAL},
    {"<r", EINVAL},
    {"<r a='x'", EINVAL},
    {"<r><q/></Q></r>", EINVAL},
    {"<r></R>", EINVAL},
    {"<r><q></r></q>", EINVAL},
    {"<r/><q/>", EINVAL},
    {"x<r/>", EINVAL},
    {"<r/>x", EINVAL},
    {"&#32;<r/>", EINVAL},
    {"<r/>&#32;", EINVAL},
    {"<r a=x/>", EINVAL},
    {"<r a='x'b='y'/>", EINVAL},
    {"<r a='<'/>", EINVAL},
    {"<r a='x' a='y'/>", EINVAL},
    {"<p:r/>", EINVAL},
    {"<r p:a='x'/>", EINVAL},
    {"<r xmlns:p='u' xmlns:q='u' p:a='1' q:a='2'/>", EINVAL},
    {"<r xmlns:p=''/>", EINVAL},
    {"<r xmlns:xml='urn:wrong'/>", EINVAL},
    {"<r xmlns='http://www.w3.org/XML/1998/namespace'/>", EINVAL},
    {"<r xmlns:p='http://www.w3.org/XML/1998/namespace'/>", EINVAL},
    {"<r xmlns='http://www.w3.org/2000/xmlns/'/>", EINVAL},
    {"<r xmlns:xmlns='u'/>", EINVAL},
    {"<xmlns:r/>", EINVAL},
    {"<r a:b:c='1'/>", EINVAL},
    {"<:r/>", EINVAL},
    {"<r:/>", EINVAL},
    {"<r:1/>", EINVAL},
    {"<1r/>", EINVAL},
    {"<\xcc\x81/>", EINVAL},
    {"<r>]]></r>", EINVAL},
    {"<r><!--x--x--></r>", EINVAL},
    {"<r><!--x---></r>", EINVAL},
    {"<r><!--x</r>", EINVAL},
    {"<![CDATA[x]]><r/>", EINVAL},
    {"<r><![CDATA[x</r>", EINVAL},
    {"<?p data<r/>", EINVAL},
    {"<?p:data?><r/>", EINVAL},
    {"<?XML version='1.0'?><r/>", EINVAL},
    {" <?xml version='1.0'?><r/>", EINVAL},
    {"<?xml?><r/>", EINVAL},
    {"<?xml version='1.0' version='1.0'?><r/>", EINVAL},
    {"<?xml encoding='UTF-8' version='1.0'?><r/>", EINVAL},
    {"<?xml version='1.0' standalone='yes' encoding='UTF-8'?><r/>", EINVAL},
    {"<?xml version='1.0' standalone='maybe'?><r/>", EINVAL},
    {"<?xml version='1&#46;0'?><r/>", EINVAL},
    {"<?xml version='1.0' encoding='UTF 8'?><r/>", EINVAL},
    {"<?xml version='1.0'?><r><?xml version='1.0'?></r>", EINVAL},
    {"<?xml version='1.1'?><r/>", ENOTSUP},
    {"<?xml version='1.0' encoding='UTF-16'?><r/>", ENOTSUP},
    {"<!DOCTYPE r><r/>", ENOTSUP},
    {"<!DOCTYPE r [<!ENTITY x 'v'>]><r>&x;</r>", ENOTSUP},
    {"<r>&unknown;</r>", EINVAL},
    {"<r>&amp</r>", EINVAL},
    {"<r>&#;</r>", EINVAL},
    {"<r>&#x;</r>", EINVAL},
    {"<r>&#X41;</r>", EINVAL},
    {"<r>&#-1;</r>", EINVAL},
    {"<r>&#0;</r>", EINVAL},
    {"<r>&#xD800;</r>", EINVAL},
    {"<r>&#xFFFE;</r>", EINVAL},
    {"<r>&#x110000;</r>", EINVAL},
    {"<r>&#999999999999999999999999999;</r>", EINVAL},
    {"<r>\x01</r>", EINVAL},
    {"<r>\x0b</r>", EINVAL},
    {"<r>\x80</r>", EINVAL},
    {"<r>\xc0\xaf</r>", EINVAL},
    {"<r>\xe0\x80\x80</r>", EINVAL},
    {"<r>\xed\xa0\x80</r>", EINVAL},
    {"<r>\xf0\x80\x80\x80</r>", EINVAL},
    {"<r>\xf4\x90\x80\x80</r>", EINVAL},
    {"<r>\xe2\x82", EINVAL},
    {"\xff\xfe<r/>", ENOTSUP}};

/* Count all independent model acceptance and semantic observations in this fixture. */
static unsigned checks;
/* Retain every failed observation until normal native resources have been released. */
static unsigned failures;

static void xml_check(int condition, const char *name);
static int xml_matches(struct xml_units units, const char *text);
static int xml_inputs(void);
static int xml_semantics(void);
static int xml_resources(void);
static int xml_bounds(void);

/*
 * Verifies real native XML records and rejected inputs without a Page or VM implementation shortcut.
 */
int
main(
	void)
{
	int status;
	int printed;

	/* Check independent syntax cases before inspecting expanded model fields. */
	status = xml_inputs();
	if (status != 0)
		return 2;

	/* Verify copied model semantics after successful syntax coverage. */
	status = xml_semantics();
	if (status != 0)
		return 2;

	/* Parse the original pinned resources without activation. */
	status = xml_resources();
	if (status != 0)
		return 2;

	/* Verify the unchanged finite input, depth and record boundaries. */
	status = xml_bounds();
	if (status != 0)
		return 2;

	/* Report all original parser observations independently of setup errors. */
	printed = printf("native XML model: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Refuse any failed syntax or native model observation. */
	if (failures != 0)
		return 1;

	/* Succeeded: real syntax, expanded names, storage ownership and finite bounds were verified. */
	return 0;
}

/* Records an independent actual parser or model observation. */
static void
xml_check(
	int condition,
	const char *name)
{
	int printed;

	/* Preserve every failed observation while later independent cases still run. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the independent observation remains in the final fixture totals. */
	return;
}

/* Compares actual immutable model output with independent expected valid UTF8 text. */
static int
xml_matches(
	struct xml_units units,
	const char *text)
{
	struct wb_units expected;
	int status;
	int same;
	int compared;

	/* The expected literal is valid UTF8; the production XML decoder is not used to create it. */
	wb_units_init(&expected);
	status = wb_utf8_to_units((const unsigned char *)text, strlen(text), &expected);
	if (status != 0) {
		wb_units_release(&expected);
		return 0;
	}

	/* Compare only equally sized, completely converted independent expected buffers. */
	same = 0;
	if (expected.length == units.length) {
		same = 1;
		if (units.length != 0) {
			compared = memcmp(expected.data, units.data, units.length * sizeof(*units.data));
			same = 0;
			if (compared == 0)
				same = 1;
		}
	}

	/* Release the independent expected buffer after the actual comparison. */
	wb_units_release(&expected);

	/* Succeeded: the comparison does not borrow storage past its independent expected buffer lifetime. */
	return same;
}

/* Runs actual independent valid, malformed and explicitly unsupported profile inputs. */
static int
xml_inputs(
	void)
{
	struct xml_document *document;
	struct xml_error error;
	size_t index;
	int status;
	int accepted;

	/* Every rejection must leave the output pointer null rather than publishing partial normal structure. */
	for (index = 0; index < sizeof(xml_cases) / sizeof(xml_cases[0]); index++) {
		status = xml_document_parse(&document, (const unsigned char *)xml_cases[index].input, strlen(xml_cases[index].input), &error);
		accepted = 0;
		if (status == xml_cases[index].status) {
			if (status == 0) {
				if (document != NULL && error.status == 0)
					accepted = 1;
			} else {
				if (document == NULL && error.status == status)
					accepted = 1;
			}
		}

		/* Record acceptance and release any successfully published owning model. */
		xml_check(accepted, xml_cases[index].input);
		xml_document_destroy(document);
	}

	/* Succeeded: all models and partial parse allocations have been released. */
	return 0;
}

/* Verifies expanded names, literal normalization and separate native character records. */
static int
xml_semantics(
	void)
{
	struct xml_document *document;
	struct xml_error error;
	struct xml_node *root;
	struct xml_node *child;
	struct xml_node *node;
	struct xml_attribute *attribute;
	char mutable_input[32];
	const char *input;
	unsigned cycle;
	int status;
	int same;

	/* Nullable ownership covers each successive independent native model. */
	document = NULL;

	/* Default namespaces affect elements while ordinary unprefixed attributes remain namespace free. */
	input = "<r xmlns='urn:a' a='v'><q xmlns=''><z/></q><p:x xmlns:p='urn:b' p:a='1' a='2'/></r>";
	status = xml_document_parse(&document, (const unsigned char *)input, strlen(input), &error);
	if (status != 0)
		goto cleanup;

	/* Refuse a successfully reported model with an incomplete native shape. */
	if (document == NULL || document->node.first_child == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* The complete model supplies its original root record. */
	root = document->node.first_child;
	same = xml_matches(root->uri, "urn:a");
	xml_check(same, "default element namespace");

	/* Expanded attribute reads require their actual original chain. */
	if (root->attributes == NULL || root->attributes->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Expanded root attributes retain their original namespace declaration order. */
	attribute = root->attributes;
	same = xml_matches(attribute->uri, "http://www.w3.org/2000/xmlns/");
	xml_check(same, "namespace declaration expanded name");
	attribute = attribute->next;
	xml_check(attribute->uri.length == 0, "unprefixed attribute has no namespace");

	/* Namespace scope reads require complete original child and sibling records. */
	if (root->first_child == NULL ||
	    root->first_child->first_child == NULL ||
	    root->first_child->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* The real child and sibling supply the original namespace-scope observations. */
	child = root->first_child;
	xml_check(child->uri.length == 0 && child->first_child->uri.length == 0, "default namespace undeclaration inherited");
	child = child->next;
	same = xml_matches(child->name, "p:x");
	xml_check(same, "qualified name preserved");
	same = xml_matches(child->prefix, "p");
	xml_check(same, "prefix preserved");
	same = xml_matches(child->local, "x");
	xml_check(same, "local name preserved");
	same = xml_matches(child->uri, "urn:b");
	xml_check(same, "prefixed element expanded namespace");

	/* Attribute scope reads require every original qualified and ordinary field. */
	if (child->attributes == NULL ||
	    child->attributes->next == NULL ||
	    child->attributes->next->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Expanded child attributes have their complete original native chain. */
	attribute = child->attributes->next;
	same = xml_matches(attribute->uri, "urn:b");
	xml_check(same, "prefixed attribute expanded namespace");
	xml_check(attribute->next->uri.length == 0, "default namespace does not affect ordinary attributes");
	xml_check(child->parent == root && root->last_child == child, "native parent and last child edges");
	xml_document_destroy(document);
	document = NULL;

	/* A locally rebound prefix returns to the ancestor binding at a following sibling. */
	input = "<a xmlns:p='u'><p:b xmlns:p='v'><p:c/></p:b><p:d/></a>";
	status = xml_document_parse(&document, (const unsigned char *)input, strlen(input), &error);
	if (status != 0)
		goto cleanup;

	/* Refuse a successfully reported model with an incomplete native shape. */
	if (document == NULL ||
	    document->node.first_child == NULL ||
	    document->node.first_child->first_child == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Follow the complete actual prefix-shadow child graph. */
	child = document->node.first_child->first_child;
	if (child->first_child == NULL || child->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Observe the valid original shadow binding and its descendants. */
	same = xml_matches(child->uri, "v");
	xml_check(same, "prefix shadow applies to current element");
	same = xml_matches(child->first_child->uri, "v");
	xml_check(same, "prefix shadow inherited by descendants");
	same = xml_matches(child->next->uri, "u");
	xml_check(same, "following sibling recovers ancestor prefix binding");
	xml_document_destroy(document);
	document = NULL;

	/* Literal line endings normalize before parsing; character references retain their actual scalars. */
	input = "<r a='a\r\nb\rc\td&#13;&#9;&#10;'>a\r\nb\rc&#13;&#9;&#10;&amp;&lt;&gt;&quot;&apos;&#x1f600;<![CDATA[<&]]><!--raw--><?p data?></r>";
	status = xml_document_parse(&document, (const unsigned char *)input, strlen(input), &error);
	if (status != 0)
		goto cleanup;

	/* Refuse a successfully reported model with an incomplete native shape. */
	if (document == NULL || document->node.first_child == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* The complete model supplies its original root record. */
	root = document->node.first_child;
	if (root->attributes == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Normalize the complete original root attribute. */
	same = xml_matches(root->attributes->value, "a b c d\r\t\n");
	xml_check(same, "literal attribute whitespace differs from reference whitespace");

	/* Mixed-content reads require their real initial native record. */
	if (root->first_child == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Each mixed-content record remains independently represented. */
	node = root->first_child;
	same = xml_matches(node->value, "a\nb\nc\r\t\n&<>\"'\xf0\x9f\x98\x80");
	xml_check(node->type == XML_TEXT && same, "text expansion and supplementary scalar");

	/* Continue only through the next actual mixed-content record. */
	if (node->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Advance only to an existing next native mixed-content record. */
	node = node->next;
	same = xml_matches(node->value, "<&");
	xml_check(node->type == XML_CDATA && same, "CDATA separate and uninterpreted");

	/* Continue only through the next actual mixed-content record. */
	if (node->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Advance only to an existing next native mixed-content record. */
	node = node->next;
	same = xml_matches(node->value, "raw");
	xml_check(node->type == XML_COMMENT && same, "comment separate and uninterpreted");

	/* Continue only through the next actual mixed-content record. */
	if (node->next == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Advance only to an existing next native mixed-content record. */
	node = node->next;
	same = xml_matches(node->name, "p");
	xml_check(node->type == XML_PI && same, "processing instruction target");
	same = xml_matches(node->value, "data");
	xml_check(same && node->next == NULL, "processing instruction data and tree end");
	xml_document_destroy(document);
	document = NULL;

	/* Declaration metadata is retained separately from ordinary processing instructions. */
	input = "<?xml version='1.0' encoding='UTF-8' standalone='no'?><r/>";
	status = xml_document_parse(&document, (const unsigned char *)input, strlen(input), &error);
	if (status != 0)
		goto cleanup;

	/* Declaration observations require a complete successfully published model. */
	if (document == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Read declaration metadata independently of ordinary node records. */
	same = xml_matches(document->version, "1.0");
	xml_check(same, "XML declaration version");
	same = xml_matches(document->encoding, "UTF-8");
	xml_check(same, "XML declaration encoding");
	same = xml_matches(document->standalone, "no");
	xml_check(same, "XML declaration standalone");

	/* The declaration must preserve the real first ordinary root. */
	if (document->node.first_child == NULL) {
		status = EINVAL;
		goto cleanup;
	}

	/* Declaration metadata must not occupy an ordinary node slot. */
	xml_check(document->node.first_child->type == XML_ELEMENT, "XML declaration is not a PI node");
	xml_document_destroy(document);
	document = NULL;

	/* Repeated owning models never borrow the mutable input buffer past successful parsing. */
	for (cycle = 0; cycle < 100; cycle++) {
		memcpy(mutable_input, "<r>&amp;</r>", 13);
		status = xml_document_parse(&document, (const unsigned char *)mutable_input, 12, &error);
		if (status != 0)
			goto cleanup;

		/* Destroy the caller input only after successful independent model publication. */
		memset(mutable_input, 0, sizeof(mutable_input));
		if (document == NULL ||
		    document->node.first_child == NULL ||
		    document->node.first_child->first_child == NULL) {
			status = EINVAL;
			goto cleanup;
		}

		/* The independent model survives destruction of its mutable source. */
		same = xml_matches(document->node.first_child->first_child->value, "&");
		xml_check(same, "repeated owning model lifetime");
		xml_document_destroy(document);
		document = NULL;
	}

cleanup:
	/* Release only the current independent model after any failed native observation. */
	xml_document_destroy(document);
	if (status != 0)
		return status;

	/* Succeeded: every semantic model was released after its final native observation. */
	return 0;
}

/* Parses the real pinned XML resources without synthesizing their tree or executing their text. */
static int
xml_resources(
	void)
{
	struct wb_buffer bytes;
	struct xml_document *document;
	struct xml_error error;
	struct xml_node *node;
	const char *paths[5];
	struct xml_node *body;
	const char *uris[5];
	size_t index;
	int status;
	int same;

	/* Actual resource bytes include both well-formed documents and deliberately malformed inputs. */
	paths[0] = "build/ws074-suites/wpt/acid/acid3/svg.xml";
	paths[1] = "build/ws074-suites/wpt/acid/acid3/xhtml.1";
	paths[2] = "build/ws074-suites/wpt/acid/acid3/xhtml.3";
	paths[3] = "build/ws074-suites/wpt/acid/acid3/xhtml.2";
	paths[4] = "build/ws074-suites/wpt/acid/acid3/empty.xml";
	uris[0] = "http://www.w3.org/2000/svg";
	uris[1] = "http://www.w3.org/1999/xhtml";
	uris[2] = "http://www.w3.org/1999/xhtml#";
	uris[3] = NULL;
	uris[4] = NULL;

	/* The model must retain its own storage after the native resource buffer is released. */
	for (index = 0; index < 5; index++) {
		wb_buffer_init(&bytes);
		status = wb_file_read(paths[index], &bytes);
		if (status != 0) {
			wb_buffer_release(&bytes);
			return status;
		}

		/* Parsing owns a separate complete input before the source buffer is released. */
		status = xml_document_parse(&document, bytes.data, bytes.length, &error);
		if (status != 0 && uris[index] != NULL) {
			wb_buffer_release(&bytes);
			xml_document_destroy(document);
			return status;
		}

		/* Expected malformed resources and successful owning models no longer borrow input. */
		wb_buffer_release(&bytes);
		if (uris[index] == NULL) {
			same = 0;
			if (status == EINVAL && document == NULL)
				same = 1;
			xml_check(same, paths[index]);
		} else {
			xml_check(status == 0 && document != NULL, paths[index]);
			if (status == 0) {
				if (document == NULL || document->node.first_child == NULL) {
					xml_document_destroy(document);
					return EINVAL;
				}

				/* The complete actual resource supplies the root and optional prolog. */
				node = document->node.first_child;
				if (index == 0) {
					same = xml_matches(node->name, "xml-stylesheet");
					xml_check(node->type == XML_PI && same, "actual SVG stylesheet PI");
					if (node->next == NULL) {
						xml_document_destroy(document);
						return EINVAL;
					}

					/* Skip only the genuine prolog PI before reading its actual SVG root. */
					node = node->next;
				}

				/* Check the exact actual root URI, including the deliberately different XHTML URI. */
				same = xml_matches(node->uri, uris[index]);
				xml_check(same, "actual resource root namespace preserved exactly");
				if (index == 1 || index == 2) {
					body = node->first_child;
					while (body != NULL) {
						same = xml_matches(body->local, "body");
						if (body->type == XML_ELEMENT && same)
							break;
						body = body->next;
					}

					/* Locate the real script child inside the actual body rather than assuming child order. */
					node = NULL;
					if (body != NULL)
						node = body->first_child;
					while (node != NULL) {
						same = xml_matches(node->local, "script");
						if (node->type == XML_ELEMENT && same)
							break;
						node = node->next;
					}

					/* Script text is preserved as data for later separately authorized activation. */
					same = 0;
					if (node != NULL)
						same = 1;
					xml_check(same, "actual XHTML resource script element retained");
					if (node != NULL) {
						same = xml_matches(node->local, "script");
						xml_check(same, "actual XHTML script local name");
						xml_check(node->first_child != NULL && node->first_child->type == XML_TEXT, "actual XHTML script text retained");
					}
				}
			}
		}

		/* No resource model survives its final observation. */
		xml_document_destroy(document);
	}

	/* Succeeded: malformed resources never published a normal partially parsed tree. */
	return 0;
}

/* Checks finite input, depth and shared node/attribute record limits at their actual boundaries. */
static int
xml_bounds(
	void)
{
	struct wb_buffer bytes;
	struct xml_document *document;
	struct xml_error error;
	size_t index;
	size_t count;
	int status;
	int expected;

	/* Oversized input is refused before decoding or dereferencing its byte pointer. */
	status = xml_document_parse(&document, NULL, 16U * 1024U * 1024U + 1U, &error);
	xml_check(status == EOVERFLOW && document == NULL, "input length bound before decoding");

	/* The iterative parser accepts its maximum depth and refuses the next nested element. */
	for (count = 256; count <= 257; count++) {
		wb_buffer_init(&bytes);
		status = 0;

		/* Append each original opening element before the matching closers. */
		for (index = 0; index < count; index++) {
			status = wb_buffer_append(&bytes, "<r>", 3);
			if (status != 0) {
				wb_buffer_release(&bytes);
				return status;
			}
		}

		/* Close all checked opening elements in the original nested source. */
		for (index = 0; index < count; index++) {
			status = wb_buffer_append(&bytes, "</r>", 4);
			if (status != 0) {
				wb_buffer_release(&bytes);
				return status;
			}
		}

		/* Parsing owns a separate complete input before the source buffer is released. */
		expected = 0;
		if (count == 257)
			expected = EOVERFLOW;
		status = xml_document_parse(&document, bytes.data, bytes.length, &error);
		xml_check(status == expected, "exact native nesting boundary");
		wb_buffer_release(&bytes);
		xml_document_destroy(document);
	}

	/* Document and root consume two records; each actual empty child consumes one more. */
	for (count = 65534; count <= 65535; count++) {
		wb_buffer_init(&bytes);
		status = wb_buffer_append(&bytes, "<r>", 3);
		if (status != 0) {
			wb_buffer_release(&bytes);
			return status;
		}

		/* Append each original child before completing the actual wide document. */
		for (index = 0; index < count; index++) {
			status = wb_buffer_append(&bytes, "<c/>", 4);
			if (status != 0) {
				wb_buffer_release(&bytes);
				return status;
			}
		}

		/* Complete the original wide source only after all checked child writes. */
		status = wb_buffer_append(&bytes, "</r>", 4);
		if (status != 0) {
			wb_buffer_release(&bytes);
			return status;
		}

		/* Parsing owns a separate complete input before the source buffer is released. */
		expected = 0;
		if (count == 65535)
			expected = EOVERFLOW;
		status = xml_document_parse(&document, bytes.data, bytes.length, &error);
		xml_check(status == expected, "exact shared native record boundary");
		wb_buffer_release(&bytes);
		if (status == 0 && document != NULL)
			xml_check(document->records == 65536, "maximum records counted without truncation");
		xml_document_destroy(document);
	}

	/* Succeeded: rejected bounds release all partial records just like ordinary syntax failures. */
	return 0;
}
