/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The DOM's mixins, whose members several interfaces share: ParentNode
 * (children, the first and last element child, append and prepend),
 * ChildNode (remove), NonDocumentTypeChildNode (the element siblings),
 * and the getElementsBy* methods of Document and Element.
 */

#include "bind/internal.h"

#include <errno.h>
#include <string.h>

static int mixin_insert_values(struct vm_realm *realm, struct dom_node *parent, const vm_value *args, unsigned count, struct dom_node *reference);
static int mixin_element_link(struct vm_realm *realm, struct dom_node *node, vm_value *result);
static int mixin_name_matches(const struct dom_element *element, const struct vm_string *name, const struct vm_string *lower);

/*
 * Reports a parent's first element child (firstElementChild).
 */
int
bind_first_element_child_get(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_element *first;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The node. */
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* Its first element child, or null. */
	first = bind_first_element_child(node);
	status = mixin_element_link(realm, (struct dom_node *)first, result);
	if (status != 0)
		return status;

	/* Succeeded: the child is reported. */
	return 0;
}

/*
 * Reports a parent's last element child (lastElementChild).
 */
int
bind_last_element_child_get(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_node *child;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The node. */
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* The last child that is an element. */
	for (child = node->last_child; child != NULL; child = child->previous) {
		if (child->type == DOM_ELEMENT)
			break;
	}

	/* The child, or null. */
	status = mixin_element_link(realm, child, result);
	if (status != 0)
		return status;

	/* Succeeded: the child is reported. */
	return 0;
}

/*
 * Reports how many element children a parent has (childElementCount).
 */
int
bind_child_element_count(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_node *child;
	int32_t elements;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The node. */
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* Counts the element children. */
	elements = 0;
	for (child = node->first_child; child != NULL; child = child->next) {
		if (child->type == DOM_ELEMENT)
			elements++;
	}

	/* Succeeded: the count is reported. */
	*result = vm_value_int32(elements);
	return 0;
}

/*
 * Appends nodes and strings (as text) to a parent's children (append).
 */
int
bind_append(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	int status;

	/* The parent. */
	*result = VM_VALUE_UNDEFINED;
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* The arguments at the end, in order. */
	status = mixin_insert_values(realm, node, args, count, NULL);
	if (status != 0)
		return status;

	/* Succeeded: the nodes are appended. */
	return 0;
}

/*
 * Inserts nodes and strings (as text) before a parent's first child
 * (prepend).
 */
int
bind_prepend(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	int status;

	/* The parent. */
	*result = VM_VALUE_UNDEFINED;
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* The arguments before the first child, in order. */
	status = mixin_insert_values(realm, node, args, count, node->first_child);
	if (status != 0)
		return status;

	/* Succeeded: the nodes are prepended. */
	return 0;
}

/*
 * Removes a node from its parent (remove).
 */
int
bind_remove(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct dom_node *parent;
	struct dom_node *node;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The node. */
	*result = VM_VALUE_UNDEFINED;
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* Out of its parent, if it has one. */
	window = bind_window_of(realm);
	parent = node->parent;
	dom_remove(node);
	if (parent != NULL) {
		status = bind_environment_child_mutation(window, parent, NULL, node);
		if (status != 0)
			return status;
	}

	/* Succeeded: the node is detached. */
	return 0;
}

/*
 * Reports the element before a node among its siblings
 * (previousElementSibling).
 */
int
bind_previous_element_sibling(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_node *sibling;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The node. */
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* The nearest previous sibling that is an element. */
	for (sibling = node->previous; sibling != NULL; sibling = sibling->previous) {
		if (sibling->type == DOM_ELEMENT)
			break;
	}

	/* The sibling, or null. */
	status = mixin_element_link(realm, sibling, result);
	if (status != 0)
		return status;

	/* Succeeded: the sibling is reported. */
	return 0;
}

/*
 * Reports the element after a node among its siblings
 * (nextElementSibling).
 */
int
bind_next_element_sibling(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_node *sibling;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The node. */
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* The nearest next sibling that is an element. */
	for (sibling = node->next; sibling != NULL; sibling = sibling->next) {
		if (sibling->type == DOM_ELEMENT)
			break;
	}

	/* The sibling, or null. */
	status = mixin_element_link(realm, sibling, result);
	if (status != 0)
		return status;

	/* Succeeded: the sibling is reported. */
	return 0;
}

/*
 * Reports the descendant elements with a qualified name, or all of them
 * for "*", in tree order (getElementsByTagName).
 */
int
bind_get_elements_by_tag_name(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct dom_node *root;
	struct dom_node *walk;
	struct vm_string *name;
	struct vm_string *lower;
	struct vm_object *array;
	int matches;
	int status;

	/* The root, the name as it is and folded, and an empty list. */
	window = bind_window_of(realm);
	status = bind_this_node(realm, this_value, &root);
	if (status != 0)
		return status;
	status = bind_to_atom(realm, js_argument(args, count, 0), 0, &name);
	if (status != 0)
		return status;
	status = bind_to_atom(realm, js_argument(args, count, 0), 1, &lower);
	if (status != 0)
		return status;
	status = bind_array_create(realm, &array);
	if (status != 0)
		return status;

	/* Each descendant element whose name matches. */
	for (walk = bind_following(root, root); walk != NULL; walk = bind_following(walk, root)) {
		if (walk->type != DOM_ELEMENT)
			continue;
		matches = mixin_name_matches((const struct dom_element *)walk, name, lower);
		if (!matches)
			continue;
		status = bind_array_push_node(window, array, walk);
		if (status != 0)
			return status;
	}

	/* Succeeded: the list is reported. */
	*result = vm_value_cell(array);
	return 0;
}

/*
 * Reports the descendant elements that have every class of a
 * space-separated list, in tree order (getElementsByClassName).
 */
int
bind_get_elements_by_class_name(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct dom_node *root;
	struct dom_node *walk;
	struct vm_string *classes;
	struct vm_string *name;
	struct vm_object *names;
	struct vm_object *array;
	uint32_t index;
	int has;
	int status;

	/* The root, the class names, and an empty list. */
	window = bind_window_of(realm);
	status = bind_this_node(realm, this_value, &root);
	if (status != 0)
		return status;
	status = bind_to_string(realm, js_argument(args, count, 0), &classes);
	if (status != 0)
		return status;
	status = bind_array_create(realm, &names);
	if (status != 0)
		return status;
	status = bind_split_classes(realm, classes, names);
	if (status != 0)
		return status;
	status = bind_array_create(realm, &array);
	if (status != 0)
		return status;

	/* No class names match nothing. */
	if (names->length == 0) {
		*result = vm_value_cell(array);
		return 0;
	}

	/* Each descendant element that has every class. */
	for (walk = bind_following(root, root); walk != NULL; walk = bind_following(walk, root)) {
		if (walk->type != DOM_ELEMENT)
			continue;

		/* The element must have each name. */
		has = 1;
		for (index = 0; has && index < names->length; index++) {
			name = (struct vm_string *)vm_value_as_cell(names->elements[index]);
			has = bind_element_has_class((const struct dom_element *)walk, name);
		}

		/* An element with all of them is listed. */
		if (!has)
			continue;
		status = bind_array_push_node(window, array, walk);
		if (status != 0)
			return status;
	}

	/* Succeeded: the list is reported. */
	*result = vm_value_cell(array);
	return 0;
}

/*
 * Adds the atoms of the space-separated class names in a string to an
 * array (getElementsByClassName's names, and classList's tokens).
 */
int
bind_split_classes(
	struct vm_realm *realm,
	const struct vm_string *classes,
	struct vm_object *names)
{
	struct vm_string *atom;
	struct wb_units units;
	size_t start;
	size_t index;
	uint16_t unit;
	int space;
	int status;

	/* The string's units. */
	wb_units_init(&units);
	status = vm_string_append_units(classes, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Cuts the units at ASCII whitespace; each piece is a name. */
	start = 0;
	for (index = 0; index <= units.length; index++) {
		/* The end of the string ends the last piece. */
		space = 1;
		if (index < units.length) {
			unit = units.data[index];
			space = 0;
			if (unit == 0x20U || unit == 0x09U || unit == 0x0aU || unit == 0x0cU || unit == 0x0dU)
				space = 1;
		}

		/* A character of a name goes on to the next. */
		if (!space)
			continue;

		/* A piece between two spaces is a name. */
		if (index > start) {
			atom = vm_atom_from_units(realm->heap, &units.data[start], index - start);
			if (atom == NULL) {
				wb_units_release(&units);
				return ENOMEM;
			}

			/* The name at the array's end. */
			status = vm_object_define(realm->heap, names, vm_value_int32((int32_t)names->length), vm_value_cell(atom), VM_PROPERTY_DEFAULT);
			if (status != 0) {
				wb_units_release(&units);
				return status;
			}
		}

		/* The next name starts after the space. */
		start = index + 1U;
	}

	/* Succeeded: the names are in the array. */
	wb_units_release(&units);
	return 0;
}

/* Inserts append's or prepend's arguments before reference: nodes as they are, other values as text. */
static int
mixin_insert_values(
	struct vm_realm *realm,
	struct dom_node *parent,
	const vm_value *args,
	unsigned count,
	struct dom_node *reference)
{
	struct dom_node *node;
	struct vm_string *string;
	struct wb_units units;
	unsigned index;
	int status;

	/* Each argument in order. */
	for (index = 0; index < count; index++) {
		/* A node is inserted as it is. */
		node = bind_node_of(args[index]);

		/* Any other value becomes a text node of its string. */
		if (node == NULL) {
			status = bind_to_string(realm, args[index], &string);
			if (status != 0)
				return status;
			wb_units_init(&units);
			status = vm_string_append_units(string, &units);
			if (status == 0) {
				node = dom_text_create(parent->document, units.data, units.length);
				if (node == NULL)
					status = ENOMEM;
			}

			/* The characters are in the node now. */
			wb_units_release(&units);
			if (status != 0)
				return status;
		}

		/* Before the reference (a reference that is being moved stays the place). */
		if (reference == node)
			reference = node->next;
		status = bind_insert(realm, parent, node, reference);
		if (status != 0)
			return status;
	}

	/* Succeeded: every argument is inserted. */
	return 0;
}

/* Reports an element that an attribute leads to, or null. */
static int
mixin_element_link(
	struct vm_realm *realm,
	struct dom_node *node,
	vm_value *result)
{
	struct bind_window *window;
	int status;

	/* The node's object, or null. */
	window = bind_window_of(realm);
	status = bind_wrap_or_null(window, node, result);
	if (status != 0)
		return status;

	/* Succeeded: the element is reported. */
	return 0;
}

/* Tells whether a qualified name matches using the owning Document's case policy. */
static int
mixin_name_matches(
	const struct dom_element *element,
	const struct vm_string *name,
	const struct vm_string *lower)
{
	const struct vm_string *requested;
	size_t index;
	size_t offset;
	uint16_t actual;
	uint16_t expected;
	int star;

	/* A wildcard includes every descendant Element irrespective of namespace. */
	star = vm_string_equal_ascii(name, "*");
	if (star)
		return 1;

	/* Only HTML elements within HTML Documents use the folded query spelling. */
	requested = name;
	if (element->ns == DOM_NS_HTML && element->node.document->content == DOM_CONTENT_HTML)
		requested = lower;

	/* The qualified name includes an optional prefix and its colon separator. */
	offset = 0;
	if (element->prefix != NULL) {
		offset = element->prefix->length + 1U;

		/* A different qualified-name length cannot match this element. */
		if (requested->length != offset + element->local_name->length)
			return 0;

		/* Prefixes participate in exact comparison rather than disappearing from queries. */
		for (index = 0; index < element->prefix->length; index++) {
			actual = vm_string_at(element->prefix, index);
			expected = vm_string_at(requested, index);
			if (actual != expected)
				return 0;
		}

		/* A prefix always occupies the slice before one literal namespace separator. */
		expected = vm_string_at(requested, offset - 1U);
		if (expected != ':')
			return 0;
	}

	/* Unprefixed and prefixed local slices must have exactly the same remaining length. */
	if (requested->length != offset + element->local_name->length)
		return 0;

	/* Compares UTF-16 code units without allocation or repeating user conversion. */
	for (index = 0; index < element->local_name->length; index++) {
		actual = vm_string_at(element->local_name, index);
		expected = vm_string_at(requested, offset + index);
		if (actual != expected)
			return 0;
	}

	/* Succeeded: every qualified-name code unit matches the selected spelling. */
	return 1;
}
