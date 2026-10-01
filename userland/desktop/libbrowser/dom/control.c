/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Form controls (ws074-p032): which kind of control an element is, and the
 * state the user changes -- a text control's value and caret, a checkbox's
 * checkedness -- kept beside the element until the element dies.
 *
 * A control follows its content attributes (value, checked, a textarea's
 * text) until the user or the page changes it; from then on its own state
 * is what it shows and what a form submits, as the HTML Standard's dirty
 * value flag and dirty checkedness flag say.
 */

#include "dom/dom.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/*
 * One type attribute value and the kind of input it makes.  The types this
 * pass does not draw on their own (email, search, url, number and the
 * rest) are text fields, as a browser shows them without their widgets.
 */
struct control_type {
	const char *name;
	int kind;
};

/*
 * The input types by their names in lower case.  The table is constant for
 * the life of the program and ends with a NULL name.
 */
static const struct control_type control_types[] = {
	{ "text", DOM_CONTROL_TEXT },
	{ "search", DOM_CONTROL_TEXT },
	{ "email", DOM_CONTROL_TEXT },
	{ "url", DOM_CONTROL_TEXT },
	{ "tel", DOM_CONTROL_TEXT },
	{ "number", DOM_CONTROL_TEXT },
	{ "password", DOM_CONTROL_PASSWORD },
	{ "button", DOM_CONTROL_BUTTON },
	{ "submit", DOM_CONTROL_SUBMIT },
	{ "image", DOM_CONTROL_SUBMIT },
	{ "reset", DOM_CONTROL_RESET },
	{ "checkbox", DOM_CONTROL_CHECKBOX },
	{ "radio", DOM_CONTROL_RADIO },
	{ "hidden", DOM_CONTROL_HIDDEN },
	{ NULL, DOM_CONTROL_NONE }
};

static int control_equal_folded(const struct vm_string *string, const char *ascii);
static int control_text_of(const struct dom_node *node, struct wb_units *out);
static struct dom_node *control_next(struct dom_node *node, const struct dom_node *root);

/*
 * Tells which kind of form control an element is: an <input> by its type
 * (a missing or unknown type is a text field), a <button> by its type (a
 * submit button unless it says otherwise), a <textarea>; DOM_CONTROL_NONE
 * for any other element.
 */
int
dom_control_kind(
	const struct dom_element *element)
{
	struct vm_string *type;
	size_t index;
	int same;

	/* Only HTML elements are form controls. */
	if (element->ns != DOM_NS_HTML)
		return DOM_CONTROL_NONE;

	/* A textarea and a select are always one. */
	if (element->tag == DOM_TAG_TEXTAREA)
		return DOM_CONTROL_TEXTAREA;
	if (element->tag == DOM_TAG_SELECT)
		return DOM_CONTROL_SELECT;

	/* A <button> submits unless its type makes it a plain or a reset button. */
	type = dom_attribute_ascii(element, "type");
	if (element->tag == DOM_TAG_BUTTON) {
		if (type != NULL) {
			same = control_equal_folded(type, "button");
			if (same)
				return DOM_CONTROL_BUTTON;
			same = control_equal_folded(type, "reset");
			if (same)
				return DOM_CONTROL_RESET;
		}

		/* The default type. */
		return DOM_CONTROL_SUBMIT;
	}

	/* Anything else but an <input> is not a control. */
	if (element->tag != DOM_TAG_INPUT)
		return DOM_CONTROL_NONE;

	/* No type is a text field. */
	if (type == NULL)
		return DOM_CONTROL_TEXT;

	/* The type's kind, by its name in any case. */
	for (index = 0; control_types[index].name != NULL; index++) {
		same = control_equal_folded(type, control_types[index].name);
		if (same)
			return control_types[index].kind;
	}

	/* An unknown type is a text field. */
	return DOM_CONTROL_TEXT;
}

/*
 * Finds an element's control state, making an empty one the first time
 * (it follows the attributes until it is made dirty); NULL without memory.
 */
struct dom_control *
dom_control_of(
	struct dom_element *element)
{
	struct dom_control *control;

	/* The state already made. */
	if (element->control != NULL)
		return element->control;

	/* A new, clean state. */
	control = calloc(1, sizeof(*control));
	if (control == NULL)
		return NULL;
	wb_units_init(&control->value);

	/* The element owns it from now on. */
	element->control = control;
	return control;
}

/*
 * Writes a control's value into out (which it clears first): its own value
 * once dirty, else a textarea's text or the value attribute (empty when
 * there is none).
 */
int
dom_control_value(
	struct dom_element *element,
	struct wb_units *out)
{
	struct vm_string *attribute;
	size_t index;
	uint16_t unit;
	int error;

	/* A dirty value is the control's own. */
	wb_units_clear(out);
	if (element->control != NULL && element->control->dirty) {
		error = wb_units_append(out, element->control->value.data, element->control->value.length);
		return error;
	}

	/* A textarea's default value is its text. */
	if (element->tag == DOM_TAG_TEXTAREA) {
		error = control_text_of(&element->node, out);
		return error;
	}

	/* An input's default value is its value attribute. */
	attribute = dom_attribute_ascii(element, "value");
	if (attribute == NULL)
		return 0;

	/* The attribute's units. */
	for (index = 0; index < attribute->length; index++) {
		unit = vm_string_at(attribute, index);
		error = wb_units_append(out, &unit, 1);
		if (error != 0)
			return error;
	}

	/* Succeeded: the attribute's value is written. */
	return 0;
}

/*
 * Sets a control's value, making it dirty (the attribute no longer counts),
 * and puts the caret at its end.
 */
int
dom_control_set_value(
	struct dom_element *element,
	const uint16_t *units,
	size_t length)
{
	struct dom_control *control;
	int error;

	/* The state that holds the value. */
	control = dom_control_of(element);
	if (control == NULL)
		return ENOMEM;

	/* The new value replaces the old. */
	wb_units_clear(&control->value);
	error = wb_units_append(&control->value, units, length);
	if (error != 0)
		return error;

	/*
	 * dirty tells the submission and the painting that the value is the
	 * control's own now, whatever the value attribute says.
	 */
	control->dirty = 1;
	control->caret = length;

	/* Succeeded: the control has its new value. */
	return 0;
}

/*
 * Writes the text a button shows into out (which it clears first): its
 * value attribute, or else the label its kind has by default ("Submit",
 * "Reset", nothing for a plain button).
 */
int
dom_control_label(
	const struct dom_element *element,
	struct wb_units *out)
{
	struct vm_string *attribute;
	const char *fallback;
	size_t index;
	uint16_t unit;
	int kind;
	int error;

	/* A value attribute is the label. */
	wb_units_clear(out);
	attribute = dom_attribute_ascii(element, "value");
	if (attribute != NULL) {
		for (index = 0; index < attribute->length; index++) {
			unit = vm_string_at(attribute, index);
			error = wb_units_append(out, &unit, 1);
			if (error != 0)
				return error;
		}

		/* The attribute's text is the whole label. */
		return 0;
	}

	/* The kind's own label (a plain button has none). */
	kind = dom_control_kind(element);
	fallback = "";
	if (kind == DOM_CONTROL_SUBMIT)
		fallback = "Submit";
	if (kind == DOM_CONTROL_RESET)
		fallback = "Reset";

	/* Its characters, which are ASCII. */
	for (index = 0; fallback[index] != '\0'; index++) {
		unit = (uint16_t)(unsigned char)fallback[index];
		error = wb_units_append(out, &unit, 1);
		if (error != 0)
			return error;
	}

	/* Succeeded: the default label is written. */
	return 0;
}

/*
 * Finds a select's chosen option: its last option marked selected, or its
 * first option (NULL when it has none).  The user cannot change it yet.
 */
struct dom_element *
dom_select_chosen(
	struct dom_element *select)
{
	struct dom_node *node;
	struct dom_element *first;
	struct dom_element *chosen;
	struct vm_string *selected;
	int is_option;

	/* The options in tree order (through optgroups). */
	first = NULL;
	chosen = NULL;
	node = select->node.first_child;
	while (node != NULL) {
		is_option = dom_element_is(node, DOM_NS_HTML, DOM_TAG_OPTION);
		if (is_option) {
			/* The first option, and the last one marked selected. */
			if (first == NULL)
				first = (struct dom_element *)node;
			selected = dom_attribute_ascii((struct dom_element *)node, "selected");
			if (selected != NULL)
				chosen = (struct dom_element *)node;
		}

		/* The next node inside the select. */
		node = control_next(node, &select->node);
	}

	/* The one marked selected, else the first. */
	if (chosen != NULL)
		return chosen;
	return first;
}

/*
 * Writes an option's label into out (which it clears first): its text,
 * with runs of whitespace collapsed to one space and the ends trimmed, as
 * a select shows it.
 */
int
dom_option_text(
	const struct dom_element *option,
	struct wb_units *out)
{
	static const uint16_t space = 0x20U;
	struct wb_units text;
	size_t index;
	uint16_t unit;
	int pending;
	int is_space;
	int error;

	/* The option's text. */
	wb_units_clear(out);
	wb_units_init(&text);
	error = control_text_of(&option->node, &text);

	/* Each character, a run of spaces kept as one space between words. */
	pending = 0;
	for (index = 0; index < text.length && error == 0; index++) {
		unit = text.data[index];
		is_space = 0;

		/* The HTML Standard's ASCII whitespace. */
		switch (unit) {
		case ' ':
		case '\t':
		case '\n':
		case '\r':
		case '\f':
			is_space = 1;
			break;
		default:
			break;
		}

		/* Whitespace waits to become one space before the next word. */
		if (is_space) {
			pending = 1;
			continue;
		}

		/* A space goes before a word that follows another. */
		if (pending && out->length != 0)
			error = wb_units_append(out, &space, 1);
		pending = 0;
		if (error == 0)
			error = wb_units_append(out, &unit, 1);
	}

	/* The text is no longer needed. */
	wb_units_release(&text);
	if (error != 0)
		return error;

	/* Succeeded: the label is written. */
	return 0;
}

/*
 * Tells whether a checkbox or a radio button is checked: its own
 * checkedness once the user changed it, else its checked attribute.
 */
int
dom_control_checked(
	const struct dom_element *element)
{
	struct vm_string *attribute;

	/* Checkedness the user set. */
	if (element->control != NULL && element->control->checked_dirty)
		return element->control->checked;

	/* Otherwise the attribute's presence. */
	attribute = dom_attribute_ascii(element, "checked");
	if (attribute == NULL)
		return 0;

	/* The attribute is there. */
	return 1;
}

/*
 * Frees an element's control state (its finalizer's part; the element keeps
 * no pointer to it afterwards).
 */
void
dom_control_free(
	struct dom_element *element)
{
	/* An element without a state has nothing to free. */
	if (element->control == NULL)
		return;

	/* The value's units, then the state. */
	wb_units_release(&element->control->value);
	free(element->control);
	element->control = NULL;
}

/*
 * Finds the value of an element's attribute in no namespace by its ASCII
 * name in lower case (the parser folds HTML attribute names); NULL when the
 * element has none.
 */
struct vm_string *
dom_attribute_ascii(
	const struct dom_element *element,
	const char *name)
{
	const struct dom_attribute *attribute;
	size_t index;
	int same;

	/* Each attribute in no namespace, by its name. */
	for (index = 0; index < element->attribute_count; index++) {
		attribute = &element->attributes[index];
		if (attribute->ns != DOM_NS_NONE)
			continue;
		same = vm_string_equal_ascii(attribute->name, name);
		if (same)
			return attribute->value;
	}

	/* The element has no such attribute. */
	return NULL;
}

/* Tells whether a string is an ASCII word in any case (the word is in lower case). */
static int
control_equal_folded(
	const struct vm_string *string,
	const char *ascii)
{
	size_t length;
	size_t index;
	uint16_t unit;

	/* The lengths must agree. */
	length = strlen(ascii);
	if (string->length != length)
		return 0;

	/* Each unit, folded to lower case. */
	for (index = 0; index < length; index++) {
		unit = vm_string_at(string, index);
		if (unit >= 'A' && unit <= 'Z')
			unit = (uint16_t)(unit - 'A' + 'a');
		if (unit != (uint16_t)(unsigned char)ascii[index])
			return 0;
	}

	/* The same word. */
	return 1;
}

/* Finds the node after another in tree order within a root (NULL at the end). */
static struct dom_node *
control_next(
	struct dom_node *node,
	const struct dom_node *root)
{
	/* The first child, when there is one. */
	if (node->first_child != NULL)
		return node->first_child;

	/* Otherwise the next sibling of the node or of the nearest ancestor that has one. */
	while (node != NULL && node != root) {
		if (node->next != NULL)
			return node->next;
		node = node->parent;
	}

	/* The end of the root. */
	return NULL;
}

/* Appends the text of a node's text children (a textarea's default value). */
static int
control_text_of(
	const struct dom_node *node,
	struct wb_units *out)
{
	const struct dom_character_data *text;
	const struct dom_node *child;
	int error;

	/* Each text child in order (a textarea holds only text). */
	for (child = node->first_child; child != NULL; child = child->next) {
		if (child->type != DOM_TEXT)
			continue;
		text = (const struct dom_character_data *)child;
		error = wb_units_append(out, text->data.data, text->data.length);
		if (error != 0)
			return error;
	}

	/* Succeeded: the text is appended. */
	return 0;
}
