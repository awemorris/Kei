/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The nodes of the DOM: making them, linking them into a tree and the
 * collector's view of them (what each refers to, and what each owns).
 */

#include "dom/dom.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void node_trace(struct vm_heap *heap, struct vm_cell *cell);
static void element_trace(struct vm_heap *heap, struct vm_cell *cell);
static void element_finalize(struct vm_heap *heap, struct vm_cell *cell);
static void character_data_finalize(struct vm_heap *heap, struct vm_cell *cell);
static void doctype_trace(struct vm_heap *heap, struct vm_cell *cell);
static struct dom_node *node_alloc(struct dom_document *document, const struct vm_cell_type *type, size_t size, int node_type);
static struct dom_node *character_data_create(struct dom_document *document, int node_type, const uint16_t *units, size_t length);

/* A document, which refers to its children like any node. */
static const struct vm_cell_type document_type = { "document", node_trace, NULL };

/* A document fragment (a template's contents, a fragment being parsed). */
static const struct vm_cell_type fragment_type = { "document-fragment", node_trace, NULL };

/* An element, which also refers to its names, attributes and template contents. */
static const struct vm_cell_type element_type = { "element", element_trace, element_finalize };

/* A text or comment node, which owns its character buffer. */
static const struct vm_cell_type character_data_type = { "character-data", node_trace, character_data_finalize };

/* A DOCTYPE node, which also refers to its strings. */
static const struct vm_cell_type doctype_type = { "document-type", doctype_trace, NULL };

/*
 * Makes an empty document in a heap, in no-quirks mode.
 */
struct dom_document *
dom_document_create(
	struct vm_heap *heap)
{
	struct dom_document *document;

	/* Allocates the document's cell. */
	document = vm_heap_alloc(heap, &document_type, sizeof(*document));
	if (document == NULL)
		return NULL;

	/* The document is its own document. */
	document->node.type = DOM_DOCUMENT;
	document->node.document = document;
	document->heap = heap;
	document->quirks = DOM_NO_QUIRKS;

	/* Succeeded: the document is empty. */
	return document;
}

/*
 * Makes an element of a namespace with a local name (an atom) and, for
 * foreign content, a prefix.
 */
struct dom_element *
dom_element_create(
	struct dom_document *document,
	int ns,
	struct vm_string *local_name,
	struct vm_string *prefix)
{
	struct dom_element *element;
	struct wb_units lower;
	uint16_t unit;
	size_t index;
	int error;

	/* Allocates the element's cell. */
	element = (struct dom_element *)node_alloc(document, &element_type, sizeof(*element), DOM_ELEMENT);
	if (element == NULL)
		return NULL;

	/* Records the names, and when the element was made. */
	element->local_name = local_name;
	element->prefix = prefix;
	element->ns = (uint16_t)ns;
	element->created = document->generation;

	/* Finds the tag number from the name folded to lower case (SVG names keep their case). */
	wb_units_init(&lower);
	error = wb_units_reserve(&lower, local_name->length);
	if (error == 0) {
		for (index = 0; index < local_name->length; index++) {
			unit = vm_string_at(local_name, index);
			if (unit >= 'A' && unit <= 'Z')
				unit = (uint16_t)(unit + 0x20U);
			lower.data[index] = unit;
		}

		/* The folded name is complete; its tag number is looked up. */
		lower.length = local_name->length;
		element->tag = (uint16_t)dom_tag_lookup(lower.data, lower.length);
	}

	/* Frees the folded copy. */
	wb_units_release(&lower);

	/* Succeeded: the element is detached and has no attributes. */
	return element;
}

/*
 * Makes a text node holding length units.
 */
struct dom_node *
dom_text_create(
	struct dom_document *document,
	const uint16_t *units,
	size_t length)
{
	struct dom_node *node;

	/* Makes a character data node of the text kind. */
	node = character_data_create(document, DOM_TEXT, units, length);
	if (node == NULL)
		return NULL;

	/* Succeeded: the text is detached. */
	return node;
}

/*
 * Makes a comment node holding length units.
 */
struct dom_node *
dom_comment_create(
	struct dom_document *document,
	const uint16_t *units,
	size_t length)
{
	struct dom_node *node;

	/* Makes a character data node of the comment kind. */
	node = character_data_create(document, DOM_COMMENT, units, length);
	if (node == NULL)
		return NULL;

	/* Succeeded: the comment is detached. */
	return node;
}

/*
 * Makes a DOCTYPE node.
 */
struct dom_node *
dom_doctype_create(
	struct dom_document *document,
	struct vm_string *name,
	struct vm_string *public_id,
	struct vm_string *system_id)
{
	struct dom_doctype *doctype;

	/* Allocates the node's cell. */
	doctype = (struct dom_doctype *)node_alloc(document, &doctype_type, sizeof(*doctype), DOM_DOCUMENT_TYPE);
	if (doctype == NULL)
		return NULL;

	/* Records the name and the identifiers. */
	doctype->name = name;
	doctype->public_id = public_id;
	doctype->system_id = system_id;

	/* Succeeded: the DOCTYPE is detached. */
	return &doctype->node;
}

/*
 * Makes an empty document fragment.
 */
struct dom_node *
dom_fragment_create(
	struct dom_document *document)
{
	struct dom_node *fragment;

	/* Allocates the fragment's cell. */
	fragment = node_alloc(document, &fragment_type, sizeof(*fragment), DOM_DOCUMENT_FRAGMENT);
	if (fragment == NULL)
		return NULL;

	/* Succeeded: the fragment is empty. */
	return fragment;
}

/*
 * Appends a node as the last child of parent, taking it from where it was.
 */
void
dom_append_child(
	struct dom_node *parent,
	struct dom_node *child)
{
	/* Inserting before nothing appends. */
	dom_insert_before(parent, child, NULL);
}

/*
 * Inserts a node into parent before reference (at the end when reference
 * is NULL), taking it from where it was.
 */
void
dom_insert_before(
	struct dom_node *parent,
	struct dom_node *child,
	struct dom_node *reference)
{
	/* A node that has a parent leaves it first. */
	if (child->parent != NULL)
		dom_remove(child);

	/* Links the node between reference's previous sibling and reference. */
	child->parent = parent;
	child->next = reference;
	if (reference != NULL) {
		child->previous = reference->previous;
		reference->previous = child;
	} else {
		child->previous = parent->last_child;
		parent->last_child = child;
	}

	/* The node before it now leads to it, or it is the first child. */
	if (child->previous != NULL) {
		child->previous->next = child;
	} else {
		parent->first_child = child;
	}

	/* The tree changed: the document's style and layout are out of date. */
	parent->document->generation++;
}

/*
 * Takes a node out of its parent (nothing happens when it has none).
 */
void
dom_remove(
	struct dom_node *child)
{
	struct dom_node *parent;

	/* A detached node stays as it is. */
	parent = child->parent;
	if (parent == NULL)
		return;

	/* Joins its neighbours, or moves the parent's ends past it. */
	if (child->previous != NULL) {
		child->previous->next = child->next;
	} else {
		parent->first_child = child->next;
	}

	/* The node after it now follows the node before it, or the node before it is the last child. */
	if (child->next != NULL) {
		child->next->previous = child->previous;
	} else {
		parent->last_child = child->previous;
	}

	/* The node is detached. */
	child->parent = NULL;
	child->previous = NULL;
	child->next = NULL;

	/* The tree changed: the document's style and layout are out of date. */
	parent->document->generation++;
}

/*
 * Appends characters to a text or comment node.
 */
int
dom_text_append(
	struct dom_node *node,
	const uint16_t *units,
	size_t length)
{
	struct dom_character_data *text;
	int error;

	/* Appends to the node's buffer. */
	text = (struct dom_character_data *)node;
	error = wb_units_append(&text->data, units, length);
	if (error != 0)
		return error;

	/* The text changed: the document's layout is out of date. */
	node->document->generation++;

	/* Succeeded: the node holds the characters. */
	return 0;
}

/*
 * Adds an attribute to an element (the caller checks it is not a
 * duplicate).
 */
int
dom_element_add_attribute(
	struct dom_element *element,
	int ns,
	struct vm_string *prefix,
	struct vm_string *name,
	struct vm_string *value)
{
	struct dom_attribute *attributes;
	struct dom_attribute *attribute;
	size_t capacity;

	/* Grows the array when it is full. */
	if (element->attribute_count == element->attribute_capacity) {
		capacity = element->attribute_capacity * 2U;
		if (capacity < 4U)
			capacity = 4U;

		/* Moves the attributes to larger storage. */
		attributes = realloc(element->attributes, capacity * sizeof(*attributes));
		if (attributes == NULL)
			return ENOMEM;

		/* Publishes the larger array. */
		element->attributes = attributes;
		element->attribute_capacity = capacity;
	}

	/* Fills the next place. */
	attribute = &element->attributes[element->attribute_count];
	attribute->name = name;
	attribute->prefix = prefix;
	attribute->value = value;
	attribute->ns = ns;
	element->attribute_count++;

	/* The attributes changed: the document's style is out of date. */
	element->node.document->generation++;

	/* Succeeded: the attribute is the element's last. */
	return 0;
}

/*
 * Finds an element's attribute by namespace and local name (an atom).
 */
struct dom_attribute *
dom_element_find_attribute(
	const struct dom_element *element,
	int ns,
	const struct vm_string *name)
{
	size_t index;

	/* Compares each attribute's name and namespace. */
	for (index = 0; index < element->attribute_count; index++) {
		/* Atoms compare by pointer. */
		if (element->attributes[index].name == name && element->attributes[index].ns == ns)
			return &element->attributes[index];
	}

	/* The element has no such attribute. */
	return NULL;
}

/*
 * Tells whether a node is an element of a namespace with a tag.
 */
int
dom_element_is(
	const struct dom_node *node,
	int ns,
	int tag)
{
	const struct dom_element *element;

	/* Only elements have tags. */
	if (node == NULL || node->type != DOM_ELEMENT)
		return 0;

	/* Compares the namespace and the tag. */
	element = (const struct dom_element *)node;
	if (element->ns != ns || element->tag != tag)
		return 0;

	/* The element matches. */
	return 1;
}


/*
 * Tells whether a cell is a node of the DOM (of any kind).
 */
int
dom_is_node(
	const struct vm_cell *cell)
{
	/* The node types are the cell types of this file. */
	if (cell->type == &document_type)
		return 1;
	if (cell->type == &fragment_type)
		return 1;
	if (cell->type == &element_type)
		return 1;
	if (cell->type == &character_data_type)
		return 1;
	if (cell->type == &doctype_type)
		return 1;

	/* Another kind of cell. */
	return 0;
}

/*
 * Sets an element's attribute of no namespace (name is an atom), adding
 * it when the element does not have it.
 */
int
dom_element_set_attribute(
	struct dom_element *element,
	struct vm_string *name,
	struct vm_string *value)
{
	struct dom_attribute *attribute;
	int error;

	/* An attribute the element has takes the new value, which puts the style out of date. */
	attribute = dom_element_find_attribute(element, DOM_NS_NONE, name);
	if (attribute != NULL) {
		attribute->value = value;
		element->node.document->generation++;
		return 0;
	}

	/* Otherwise the attribute is added at the end. */
	error = dom_element_add_attribute(element, DOM_NS_NONE, NULL, name, value);
	if (error != 0)
		return error;

	/* Succeeded: the element has the attribute with the value. */
	return 0;
}

/*
 * Removes an element's attribute of no namespace (name is an atom);
 * nothing happens when it has none.
 */
int
dom_element_remove_attribute(
	struct dom_element *element,
	struct vm_string *name)
{
	struct dom_attribute *attribute;
	size_t index;
	size_t after;

	/* An element without the attribute stays as it is. */
	attribute = dom_element_find_attribute(element, DOM_NS_NONE, name);
	if (attribute == NULL)
		return 0;

	/* The attributes after it move down one place, keeping their order. */
	index = (size_t)(attribute - element->attributes);
	after = element->attribute_count - index - 1U;
	memmove(&element->attributes[index], &element->attributes[index + 1U], after * sizeof(*attribute));
	element->attribute_count--;

	/* The attributes changed: the document's style is out of date. */
	element->node.document->generation++;

	/* Succeeded: the attribute is gone. */
	return 0;
}

/*
 * Replaces the characters of a text or comment node.
 */
int
dom_text_set(
	struct dom_node *node,
	const uint16_t *units,
	size_t length)
{
	struct dom_character_data *text;
	int error;

	/* The old characters go, and the new ones take their place. */
	text = (struct dom_character_data *)node;
	wb_units_clear(&text->data);
	error = wb_units_append(&text->data, units, length);
	if (error != 0)
		return error;

	/* The text changed: the document's layout is out of date. */
	node->document->generation++;

	/* Succeeded: the node holds the new text. */
	return 0;
}

/*
 * Tells whether ancestor is node or one of node's ancestors.
 */
int
dom_is_inclusive_ancestor(
	const struct dom_node *ancestor,
	const struct dom_node *node)
{
	const struct dom_node *walk;

	/* Walks up from the node to its root. */
	for (walk = node; walk != NULL; walk = walk->parent) {
		if (walk == ancestor)
			return 1;
	}

	/* The ancestor is not on the way up. */
	return 0;
}

/* Marks the nodes a node is linked to. */
static void
node_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct dom_node *node;

	/* Marks the script's object for the node and its listeners. */
	node = (struct dom_node *)cell;
	if (node->wrapper != NULL)
		vm_heap_mark(heap, &node->wrapper->cell);
	if (node->listeners != NULL)
		vm_heap_mark(heap, node->listeners);

	/* Marks the document, the parent, the neighbours and the children's ends. */
	if (node->document != NULL)
		vm_heap_mark(heap, &node->document->node.cell);
	if (node->parent != NULL)
		vm_heap_mark(heap, &node->parent->cell);
	if (node->previous != NULL)
		vm_heap_mark(heap, &node->previous->cell);
	if (node->next != NULL)
		vm_heap_mark(heap, &node->next->cell);
	if (node->first_child != NULL)
		vm_heap_mark(heap, &node->first_child->cell);
	if (node->last_child != NULL)
		vm_heap_mark(heap, &node->last_child->cell);
}

/* Marks what an element refers to: its links, names, attributes and template contents. */
static void
element_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct dom_element *element;
	size_t index;

	/* Marks the links every node has. */
	node_trace(heap, cell);

	/* Marks the names and the template contents. */
	element = (struct dom_element *)cell;
	vm_heap_mark(heap, &element->local_name->cell);
	if (element->prefix != NULL)
		vm_heap_mark(heap, &element->prefix->cell);
	if (element->content != NULL)
		vm_heap_mark(heap, &element->content->cell);

	/* Marks each attribute's strings. */
	for (index = 0; index < element->attribute_count; index++) {
		vm_heap_mark(heap, &element->attributes[index].name->cell);
		vm_heap_mark(heap, &element->attributes[index].value->cell);
		if (element->attributes[index].prefix != NULL)
			vm_heap_mark(heap, &element->attributes[index].prefix->cell);
	}
}

/* Frees the attribute array of a dead element. */
static void
element_finalize(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct dom_element *element;

	UNUSED_PARAMETER(heap);

	/* The strings are cells of their own; only the array and a control's state are the element's. */
	element = (struct dom_element *)cell;
	free(element->attributes);
	dom_control_free(element);
}

/* Frees the character buffer of a dead text or comment node. */
static void
character_data_finalize(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct dom_character_data *text;

	UNUSED_PARAMETER(heap);

	/* Frees the buffer. */
	text = (struct dom_character_data *)cell;
	wb_units_release(&text->data);
}

/* Marks what a DOCTYPE refers to: its links and strings. */
static void
doctype_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct dom_doctype *doctype;

	/* Marks the links every node has. */
	node_trace(heap, cell);

	/* Marks the strings. */
	doctype = (struct dom_doctype *)cell;
	vm_heap_mark(heap, &doctype->name->cell);
	vm_heap_mark(heap, &doctype->public_id->cell);
	vm_heap_mark(heap, &doctype->system_id->cell);
}

/* Allocates a node of a kind in its document's heap. */
static struct dom_node *
node_alloc(
	struct dom_document *document,
	const struct vm_cell_type *type,
	size_t size,
	int node_type)
{
	struct dom_node *node;

	/* Allocates the cell, zeroed. */
	node = vm_heap_alloc(document->heap, type, size);
	if (node == NULL)
		return NULL;

	/* Records the kind and the document. */
	node->type = (uint16_t)node_type;
	node->document = document;

	/* Succeeded: the node is detached. */
	return node;
}

/* Makes a text or comment node holding length units. */
static struct dom_node *
character_data_create(
	struct dom_document *document,
	int node_type,
	const uint16_t *units,
	size_t length)
{
	struct dom_character_data *text;
	int error;

	/* Allocates the node's cell. */
	text = (struct dom_character_data *)node_alloc(document, &character_data_type, sizeof(*text), node_type);
	if (text == NULL)
		return NULL;

	/* Copies the characters into the node's buffer (an empty node owns no buffer). */
	wb_units_init(&text->data);
	error = wb_units_append(&text->data, units, length);
	if (error != 0)
		return NULL;

	/* Succeeded: the node is detached. */
	return &text->node;
}
