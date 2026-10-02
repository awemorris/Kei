/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The HTML serialization of nodes (ws074-p081): the HTML Standard's
 * "serializing HTML fragments", which innerHTML and outerHTML report.
 *
 * An element is written as its tag with its attributes (a value escaped
 * with &amp;, &nbsp;, &quot;, &lt; and &gt;), its children and its end tag;
 * a void element has no end tag, and a template's children are its
 * contents'.  A text node is written escaped (&amp;, &nbsp;, &lt;, &gt;),
 * except inside the elements whose content is raw text (style, script,
 * xmp, iframe, noembed, noframes, plaintext, and noscript when scripting
 * is enabled).  A comment is <!--data-->, a DOCTYPE <!DOCTYPE name>.
 */

#include "dom/dom.h"

#include <errno.h>
#include <string.h>

/* The no-break space, which is written as &nbsp;. */
#define SERIALIZE_NBSP		0x00a0U

static int serialize_node(const struct dom_node *node, int scripting, struct wb_units *out);
static int serialize_children(const struct dom_node *node, int scripting, struct wb_units *out);
static int serialize_element(const struct dom_element *element, int scripting, struct wb_units *out);
static int serialize_tag_name(const struct dom_element *element, struct wb_units *out);
static int serialize_attribute_name(const struct dom_attribute *attribute, struct wb_units *out);
static int serialize_attribute(const struct dom_attribute *attribute, struct wb_units *out);
static int serialize_escaped(const struct vm_string *string, int attribute, struct wb_units *out);
static int serialize_units(const uint16_t *units, size_t length, int attribute, struct wb_units *out);
static int serialize_ascii(const char *ascii, struct wb_units *out);
static int serialize_string(const struct vm_string *string, struct wb_units *out);
static int serialize_is_void(const struct dom_element *element);
static int serialize_is_raw_text(const struct dom_node *parent, int scripting);

/*
 * Writes the HTML serialization of a node's children (innerHTML):
 * scripting says whether noscript's content is raw text.
 */
int
dom_serialize_children(
	const struct dom_node *node,
	int scripting,
	struct wb_units *out)
{
	int error;

	/* A template's children are its contents'. */
	if (node->type == DOM_ELEMENT && ((const struct dom_element *)node)->content != NULL) {
		error = serialize_children(((const struct dom_element *)node)->content, scripting, out);
		return error;
	}

	/* Each child. */
	error = serialize_children(node, scripting, out);
	if (error != 0)
		return error;

	/* Succeeded: the children are written. */
	return 0;
}

/*
 * Writes the HTML serialization of a node itself with its descendants
 * (outerHTML), as if it were the only child of a fragment.
 */
int
dom_serialize_node(
	const struct dom_node *node,
	int scripting,
	struct wb_units *out)
{
	int error;

	/* The node. */
	error = serialize_node(node, scripting, out);
	if (error != 0)
		return error;

	/* Succeeded: the node is written. */
	return 0;
}

/* Writes each child of a node. */
static int
serialize_children(
	const struct dom_node *node,
	int scripting,
	struct wb_units *out)
{
	const struct dom_node *child;
	int error;

	/* In order. */
	for (child = node->first_child; child != NULL; child = child->next) {
		error = serialize_node(child, scripting, out);
		if (error != 0)
			return error;
	}

	/* Succeeded: every child is written. */
	return 0;
}

/* Writes one node of any kind. */
static int
serialize_node(
	const struct dom_node *node,
	int scripting,
	struct wb_units *out)
{
	const struct dom_character_data *data;
	const struct dom_doctype *doctype;
	int raw;
	int error;

	/* Chooses by the node's kind. */
	switch (node->type) {
	case DOM_ELEMENT:
		error = serialize_element((const struct dom_element *)node, scripting, out);
		return error;
	case DOM_TEXT:
	case DOM_CDATA_SECTION:
		/* Text, raw inside the raw text elements and escaped anywhere else. */
		data = (const struct dom_character_data *)node;
		raw = 0;
		if (node->parent != NULL)
			raw = serialize_is_raw_text(node->parent, scripting);

		/* Written by the parent's kind. */
		if (raw) {
			error = wb_units_append(out, data->data.data, data->data.length);
		} else {
			error = serialize_units(data->data.data, data->data.length, 0, out);
		}

		/* Reports the writing. */
		return error;
	case DOM_PROCESSING_INSTRUCTION:
		/* HTML fragment serialization preserves the actual PI target and uninterpreted data. */
		data = (const struct dom_character_data *)node;
		error = serialize_ascii("<?", out);
		if (error == 0)
			error = serialize_string(data->target, out);
		if (error == 0)
			error = serialize_ascii(" ", out);
		if (error == 0)
			error = wb_units_append(out, data->data.data, data->data.length);
		if (error == 0)
			error = serialize_ascii("?>", out);

		/* Succeeded or failed: PI serialization never descends into nonexistent children. */
		return error;
	case DOM_COMMENT:
		/* A comment, its data as it is. */
		data = (const struct dom_character_data *)node;
		error = serialize_ascii("<!--", out);
		if (error != 0)
			return error;
		error = wb_units_append(out, data->data.data, data->data.length);
		if (error != 0)
			return error;
		error = serialize_ascii("-->", out);
		return error;
	case DOM_DOCUMENT_TYPE:
		/* A DOCTYPE, by its name. */
		doctype = (const struct dom_doctype *)node;
		error = serialize_ascii("<!DOCTYPE ", out);
		if (error != 0)
			return error;
		error = serialize_string(doctype->name, out);
		if (error != 0)
			return error;
		error = serialize_ascii(">", out);
		return error;
	default:
		break;
	}

	/* A document or a fragment is its children. */
	error = serialize_children(node, scripting, out);
	if (error != 0)
		return error;

	/* Succeeded: the node is written. */
	return 0;
}

/* Writes an element: its start tag, its children or contents, and its end tag unless it is void. */
static int
serialize_element(
	const struct dom_element *element,
	int scripting,
	struct wb_units *out)
{
	size_t index;
	int is_void;
	int error;

	/* The start tag's name. */
	error = serialize_ascii("<", out);
	if (error != 0)
		return error;
	error = serialize_tag_name(element, out);
	if (error != 0)
		return error;

	/* Each attribute. */
	for (index = 0; index < element->attribute_count; index++) {
		error = serialize_attribute(&element->attributes[index], out);
		if (error != 0)
			return error;
	}

	/* The start tag ends. */
	error = serialize_ascii(">", out);
	if (error != 0)
		return error;

	/* A void element ends there. */
	is_void = serialize_is_void(element);
	if (is_void)
		return 0;

	/* The children, or a template's contents. */
	error = dom_serialize_children(&element->node, scripting, out);
	if (error != 0)
		return error;

	/* The end tag. */
	error = serialize_ascii("</", out);
	if (error != 0)
		return error;
	error = serialize_tag_name(element, out);
	if (error != 0)
		return error;
	error = serialize_ascii(">", out);
	if (error != 0)
		return error;

	/* Succeeded: the element is written. */
	return 0;
}

/*
 * Writes an element's name in a tag: its local name, after its prefix
 * for an element of a namespace other than HTML's, SVG's and MathML's.
 */
static int
serialize_tag_name(
	const struct dom_element *element,
	struct wb_units *out)
{
	int prefixed;
	int error;

	/* Only an element of another namespace keeps its prefix. */
	prefixed = 0;
	if (element->prefix != NULL &&
	    element->ns != DOM_NS_HTML &&
	    element->ns != DOM_NS_SVG &&
	    element->ns != DOM_NS_MATHML)
		prefixed = 1;

	/* The prefix and its colon. */
	if (prefixed) {
		error = serialize_string(element->prefix, out);
		if (error != 0)
			return error;
		error = serialize_ascii(":", out);
		if (error != 0)
			return error;
	}

	/* Succeeded: the local name. */
	error = serialize_string(element->local_name, out);
	if (error != 0)
		return error;
	return 0;
}

/* Writes one attribute: a space, its serialized name, and its escaped value in double quotes. */
static int
serialize_attribute(
	const struct dom_attribute *attribute,
	struct wb_units *out)
{
	int error;

	/* The space and the name. */
	error = serialize_ascii(" ", out);
	if (error != 0)
		return error;
	error = serialize_attribute_name(attribute, out);
	if (error != 0)
		return error;

	/* The value in double quotes. */
	error = serialize_ascii("=\"", out);
	if (error != 0)
		return error;
	error = serialize_escaped(attribute->value, 1, out);
	if (error != 0)
		return error;
	error = serialize_ascii("\"", out);
	if (error != 0)
		return error;

	/* Succeeded: the attribute is written. */
	return 0;
}

/*
 * Writes an attribute's serialized name: its local name, after xml:,
 * xlink: or xmlns: for those namespaces (not for xmlns itself), or after
 * its own prefix for another namespace.
 */
static int
serialize_attribute_name(
	const struct dom_attribute *attribute,
	struct wb_units *out)
{
	int xmlns;
	int error;

	/* Whether the name is xmlns itself, which takes no prefix. */
	xmlns = vm_string_equal_ascii(attribute->name, "xmlns");

	/* The prefix by the namespace. */
	error = 0;
	if (attribute->ns == DOM_NS_XML) {
		error = serialize_ascii("xml:", out);
	} else if (attribute->ns == DOM_NS_XLINK) {
		error = serialize_ascii("xlink:", out);
	} else if (attribute->ns == DOM_NS_XMLNS && !xmlns) {
		error = serialize_ascii("xmlns:", out);
	} else if (attribute->ns != DOM_NS_NONE &&
		   attribute->ns != DOM_NS_XMLNS &&
		   attribute->prefix != NULL) {
		/* Another namespace's own prefix and its colon. */
		error = serialize_string(attribute->prefix, out);
		if (error == 0)
			error = serialize_ascii(":", out);
	}

	/* A failed prefix. */
	if (error != 0)
		return error;

	/* Succeeded: the local name. */
	error = serialize_string(attribute->name, out);
	if (error != 0)
		return error;
	return 0;
}

/* Writes a string escaped for text or for an attribute's value. */
static int
serialize_escaped(
	const struct vm_string *string,
	int attribute,
	struct wb_units *out)
{
	struct wb_units units;
	int error;

	/* The string's units. */
	wb_units_init(&units);
	error = vm_string_append_units(string, &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Escaped, and the units are freed. */
	error = serialize_units(units.data, units.length, attribute, out);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the string is written. */
	return 0;
}

/*
 * Writes units escaped: & as &amp;, the no-break space as &nbsp;, < as
 * &lt; and > as &gt;, and in an attribute's value " as &quot;.
 */
static int
serialize_units(
	const uint16_t *units,
	size_t length,
	int attribute,
	struct wb_units *out)
{
	size_t index;
	size_t start;
	const char *entity;
	int error;

	/* Each run of characters that need nothing, then the entity of the one that ends it. */
	start = 0;
	for (index = 0; index < length; index++) {
		entity = NULL;
		if (units[index] == '&') {
			entity = "&amp;";
		} else if (units[index] == SERIALIZE_NBSP) {
			entity = "&nbsp;";
		} else if (units[index] == '<') {
			entity = "&lt;";
		} else if (units[index] == '>') {
			entity = "&gt;";
		} else if (units[index] == '"' && attribute) {
			entity = "&quot;";
		}

		/* A character that needs nothing continues the run. */
		if (entity == NULL)
			continue;

		/* The run before it, and the entity. */
		error = wb_units_append(out, units + start, index - start);
		if (error != 0)
			return error;
		error = serialize_ascii(entity, out);
		if (error != 0)
			return error;
		start = index + 1U;
	}

	/* Succeeded: the last run. */
	error = wb_units_append(out, units + start, length - start);
	if (error != 0)
		return error;
	return 0;
}

/* Appends ASCII characters. */
static int
serialize_ascii(
	const char *ascii,
	struct wb_units *out)
{
	uint16_t unit;
	size_t index;
	int error;

	/* Each character. */
	for (index = 0; ascii[index] != '\0'; index++) {
		unit = (uint16_t)(unsigned char)ascii[index];
		error = wb_units_append(out, &unit, 1);
		if (error != 0)
			return error;
	}

	/* Succeeded: the characters are appended. */
	return 0;
}

/* Appends a string's characters as they are. */
static int
serialize_string(
	const struct vm_string *string,
	struct wb_units *out)
{
	int error;

	/* The string's units. */
	error = vm_string_append_units(string, out);
	if (error != 0)
		return error;

	/* Succeeded: the string is appended. */
	return 0;
}

/* Tells whether an element is one of HTML's void elements, which have no end tag. */
static int
serialize_is_void(
	const struct dom_element *element)
{
	/* Only HTML elements are void. */
	if (element->ns != DOM_NS_HTML)
		return 0;

	/* Chooses by the tag. */
	switch (element->tag) {
	case DOM_TAG_AREA:
	case DOM_TAG_BASE:
	case DOM_TAG_BASEFONT:
	case DOM_TAG_BGSOUND:
	case DOM_TAG_BR:
	case DOM_TAG_COL:
	case DOM_TAG_EMBED:
	case DOM_TAG_FRAME:
	case DOM_TAG_HR:
	case DOM_TAG_IMG:
	case DOM_TAG_INPUT:
	case DOM_TAG_KEYGEN:
	case DOM_TAG_LINK:
	case DOM_TAG_META:
	case DOM_TAG_PARAM:
	case DOM_TAG_SOURCE:
	case DOM_TAG_TRACK:
	case DOM_TAG_WBR:
		return 1;
	default:
		break;
	}

	/* Any other element has an end tag. */
	return 0;
}

/* Tells whether a node's text children are written raw (the node is an element whose content is raw text). */
static int
serialize_is_raw_text(
	const struct dom_node *parent,
	int scripting)
{
	const struct dom_element *element;

	/* Only HTML elements have raw text. */
	if (parent->type != DOM_ELEMENT)
		return 0;
	element = (const struct dom_element *)parent;
	if (element->ns != DOM_NS_HTML)
		return 0;

	/* Chooses by the tag. */
	switch (element->tag) {
	case DOM_TAG_STYLE:
	case DOM_TAG_SCRIPT:
	case DOM_TAG_XMP:
	case DOM_TAG_IFRAME:
	case DOM_TAG_NOEMBED:
	case DOM_TAG_NOFRAMES:
	case DOM_TAG_PLAINTEXT:
		return 1;
	case DOM_TAG_NOSCRIPT:
		return scripting;
	default:
		break;
	}

	/* Any other element's text is escaped. */
	return 0;
}
