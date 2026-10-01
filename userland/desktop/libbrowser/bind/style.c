/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The inline style of elements for scripts (ws074-p031): the style
 * attribute as a CSSStyleDeclaration.
 *
 * The declaration reads and writes the element's style attribute each
 * time it is used: the attribute's text is cut into declarations (name,
 * value and !important), and a change writes them back in the form other
 * browsers serialize them ("name: value;" separated by spaces).  A value
 * is kept only when the style engine reads it (the declaration of a known
 * property with a value it takes, or a custom property), as other browsers
 * keep only what they parse.  The prototype has an accessor for each
 * property the style engine knows, by its CSS name and its camel-case
 * name (and "webkit" and "Webkit" ones for the -webkit- names).  A value
 * reads back as it was written: a shorthand's longhands are not split out
 * of it, and nothing is put in its canonical form.  A new declaration is
 * made each time style is read.
 */

#include "bind/internal.h"
#include "css/css.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The longest property name the accessors are made for, with its terminator. */
#define STYLE_NAME_MAX		64U

/*
 * The declarations of a style attribute while one is read or written:
 * three arrays of the same length, the names (lower case, but a custom
 * property's as it is), the values (trimmed, without !important) and the
 * priorities (true for !important).
 */
struct style_list {
	struct vm_object *names;
	struct vm_object *values;
	struct vm_object *priorities;
};

static int style_element(struct vm_realm *realm, vm_value this_value, struct dom_element **element);
static int style_read(struct vm_realm *realm, struct dom_element *element, struct style_list *list);
static int style_parse(struct vm_realm *realm, const uint16_t *units, size_t length, struct style_list *list);
static int style_parse_one(struct vm_realm *realm, const uint16_t *units, size_t start, size_t end, struct style_list *list);
static void style_trim(const uint16_t *units, size_t *start, size_t *end);
static int style_strip_important(const uint16_t *units, size_t start, size_t *end);
static int style_write(struct vm_realm *realm, struct dom_element *element, const struct style_list *list);
static int style_serialize(const struct style_list *list, struct wb_units *units);
static int style_find(const struct style_list *list, const struct vm_string *name, uint32_t *index);
static int style_set(struct vm_realm *realm, struct dom_element *element, struct vm_string *name, struct vm_string *value, int important);
static int style_remove(struct vm_realm *realm, struct dom_element *element, struct vm_string *name, vm_value *old_value);
static int style_list_create(struct vm_realm *realm, struct style_list *list);
static int style_list_append(struct vm_realm *realm, struct style_list *list, struct vm_string *name, struct vm_string *value, int important);
static int style_list_remove_at(struct vm_realm *realm, struct style_list *list, uint32_t index);
static int style_remove_from_list(struct vm_realm *realm, struct style_list *list, const struct vm_string *name);
static int style_valid(struct vm_realm *realm, const struct vm_string *name, const struct vm_string *value, int *valid);
static int style_property_name(struct vm_realm *realm, vm_value value, struct vm_string **name);
static int style_accessor(struct bind_window *window, const char *name, struct vm_string *property);
static void style_camel_case(const char *name, int capital, char out[STYLE_NAME_MAX]);
static int style_is_space(uint16_t unit);
static int style_css_text_get(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_css_text_set(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_length(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_item(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_get_property_value(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_get_property_priority(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_set_property(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_remove_property(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_member_get(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int style_member_set(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);

/*
 * The attributes of CSSStyleDeclaration; the properties' accessors are
 * added by bind_style_install.  The table is constant for the life of the
 * program.
 */
static const struct bind_attribute style_attributes[] = {
	{ "cssText", style_css_text_get, style_css_text_set },
	{ "length", style_length, NULL },
	{ NULL, NULL, NULL }
};

/*
 * The operations of CSSStyleDeclaration.  The table is constant for the
 * life of the program.
 */
static const struct bind_operation style_operations[] = {
	{ "item", 1, style_item },
	{ "getPropertyValue", 1, style_get_property_value },
	{ "getPropertyPriority", 1, style_get_property_priority },
	{ "setProperty", 2, style_set_property },
	{ "removeProperty", 1, style_remove_property },
	{ NULL, 0, NULL }
};

/*
 * The CSSStyleDeclaration interface (an element's style).
 */
const struct bind_interface bind_css_style_declaration_interface = {
	"CSSStyleDeclaration", BIND_NO_PARENT, 0, NULL, style_attributes, style_operations, NULL
};

/*
 * Adds an accessor to CSSStyleDeclaration's prototype for each property
 * the style engine knows: by its CSS name, its camel-case name, and for a
 * -webkit- name also the camel-case name that starts with "Webkit".
 */
int
bind_style_install(
	struct bind_window *window)
{
	struct vm_string *property;
	const char *name;
	char camel[STYLE_NAME_MAX];
	size_t index;
	size_t length;
	int webkit;
	int status;

	/* Each name the engine knows. */
	for (index = 0;; index++) {
		name = css_property_name(index);
		if (name == NULL)
			break;
		length = strlen(name);
		if (length + 1U > STYLE_NAME_MAX)
			continue;

		/* The property's name, which the accessors keep. */
		property = vm_atom_from_ascii(window->realm->heap, name);
		if (property == NULL)
			return ENOMEM;

		/* The CSS name, then the camel-case one. */
		status = style_accessor(window, name, property);
		if (status != 0)
			return status;
		style_camel_case(name, 0, camel);
		status = style_accessor(window, camel, property);
		if (status != 0)
			return status;

		/* A -webkit- name also has its "Webkit" form. */
		webkit = strncmp(name, "-webkit-", 8) == 0;
		if (!webkit)
			continue;
		style_camel_case(name, 1, camel);
		status = style_accessor(window, camel, property);
		if (status != 0)
			return status;
	}

	/* Succeeded: the prototype has the accessors. */
	return 0;
}

/*
 * Makes the CSSStyleDeclaration of an element's style attribute (style).
 */
int
bind_style(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct vm_object *declaration;
	struct dom_node *node;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The element. */
	window = bind_window_of(realm);
	node = bind_node_of(this_value);
	if (node == NULL || node->type != DOM_ELEMENT) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* The declaration, which keeps the element's object. */
	declaration = vm_object_create(realm->heap, window->prototypes[BIND_CSS_STYLE_DECLARATION]);
	if (declaration == NULL)
		return ENOMEM;
	declaration->kind = VM_KIND_PLATFORM;
	declaration->internal = this_value;

	/* Succeeded: the declaration. */
	*result = vm_value_cell(declaration);
	return 0;
}

/*
 * Sets an element's style from a text, as setting its cssText does
 * (element.style = "...").
 */
int
bind_style_set(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value declaration;
	int status;

	/* The element's declaration. */
	status = bind_style(realm, this_value, args, count, &declaration);
	if (status != 0)
		return status;

	/* Succeeded: its text is set. */
	status = style_css_text_set(realm, declaration, args, count, result);
	if (status != 0)
		return status;
	return 0;
}

/* Finds the element of the CSSStyleDeclaration a method's this value stands for. */
static int
style_element(
	struct vm_realm *realm,
	vm_value this_value,
	struct dom_element **element)
{
	struct vm_object *object;
	struct dom_node *node;
	int is_object;
	int computed;
	int status;

	/* A computed declaration (ws074-p082) reaches here only to be changed, which it cannot be. */
	computed = bind_computed_element(this_value, element);
	if (computed) {
		status = bind_throw_dom(realm, "NoModificationAllowedError", "The computed style cannot be modified.");
		return status;
	}

	/* A platform object. */
	is_object = vm_value_is_object(this_value);
	if (!is_object) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* One the binding made. */
	object = (struct vm_object *)vm_value_as_cell(this_value);
	if (object->kind != VM_KIND_PLATFORM) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* That keeps an element's object. */
	node = bind_node_of(object->internal);
	if (node == NULL || node->type != DOM_ELEMENT) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* Succeeded: the element. */
	*element = (struct dom_element *)node;
	return 0;
}

/* Reads the declarations of an element's style attribute (none without the attribute). */
static int
style_read(
	struct vm_realm *realm,
	struct dom_element *element,
	struct style_list *list)
{
	struct dom_attribute *attribute;
	struct vm_string *style_atom;
	struct wb_units units;
	int status;

	/* Empty lists. */
	status = style_list_create(realm, list);
	if (status != 0)
		return status;

	/* The attribute. */
	style_atom = vm_atom_from_ascii(realm->heap, "style");
	if (style_atom == NULL)
		return ENOMEM;
	attribute = dom_element_find_attribute(element, DOM_NS_NONE, style_atom);
	if (attribute == NULL)
		return 0;

	/* Its characters, cut into declarations. */
	wb_units_init(&units);
	status = vm_string_append_units(attribute->value, &units);
	if (status == 0)
		status = style_parse(realm, units.data, units.length, list);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the declarations. */
	return 0;
}

/*
 * Cuts a declaration block's text into declarations at the semicolons
 * outside parentheses and quotes.
 */
static int
style_parse(
	struct vm_realm *realm,
	const uint16_t *units,
	size_t length,
	struct style_list *list)
{
	size_t start;
	size_t at;
	uint16_t quote;
	uint16_t unit;
	int depth;
	int status;

	/* Each declaration up to its semicolon or the end. */
	start = 0;
	at = 0;
	depth = 0;
	quote = 0;
	while (at <= length) {
		/* The end ends the last declaration. */
		unit = ';';
		if (at < length)
			unit = units[at];

		/* Chooses what the character does. */
		if (quote != 0 && at < length) {
			/* Inside quotes only the closing quote and an escape matter. */
			if (unit == '\\') {
				at++;
			} else if (unit == quote) {
				quote = 0;
			}
		} else if (unit == '"' || unit == '\'') {
			quote = unit;
		} else if (unit == '(') {
			depth++;
		} else if (unit == ')' && depth > 0) {
			depth--;
		} else if (unit == ';' && (depth == 0 || at == length)) {
			/* A declaration ends. */
			status = style_parse_one(realm, units, start, at, list);
			if (status != 0)
				return status;
			start = at + 1U;
		}

		/* The next character. */
		at++;
	}

	/* Succeeded: every declaration is read. */
	return 0;
}

/*
 * Reads one declaration ("name: value" with an optional !important) and
 * appends it; one without a name or a value is dropped.
 */
static int
style_parse_one(
	struct vm_realm *realm,
	const uint16_t *units,
	size_t start,
	size_t end,
	struct style_list *list)
{
	struct vm_string *name;
	struct vm_string *value;
	struct wb_units lower;
	size_t colon;
	size_t name_start;
	size_t name_end;
	size_t value_start;
	size_t index;
	int important;
	int custom;
	int status;

	/* The colon between the name and the value. */
	colon = start;
	while (colon < end && units[colon] != ':')
		colon++;
	if (colon >= end)
		return 0;

	/* The name and the value, whitespace trimmed, the value without its !important. */
	name_start = start;
	name_end = colon;
	style_trim(units, &name_start, &name_end);
	value_start = colon + 1U;
	style_trim(units, &value_start, &end);
	important = style_strip_important(units, value_start, &end);

	/* Nothing on either side is no declaration. */
	if (name_end == name_start || end == value_start)
		return 0;

	/* The name's characters. */
	wb_units_init(&lower);
	status = wb_units_append(&lower, units + name_start, name_end - name_start);
	if (status != 0) {
		wb_units_release(&lower);
		return status;
	}

	/* In lower case, but a custom property's as it is. */
	custom = 0;
	if (lower.length >= 2U && lower.data[0] == '-' && lower.data[1] == '-')
		custom = 1;
	for (index = 0; !custom && index < lower.length; index++) {
		if (lower.data[index] >= 'A' && lower.data[index] <= 'Z')
			lower.data[index] = (uint16_t)(lower.data[index] + 0x20U);
	}

	/* The name's atom. */
	name = vm_atom_from_units(realm->heap, lower.data, lower.length);
	wb_units_release(&lower);
	if (name == NULL)
		return ENOMEM;

	/* The value as it is written. */
	value = vm_string_from_units(realm->heap, units + value_start, end - value_start);
	if (value == NULL)
		return ENOMEM;

	/* Succeeded: the declaration is in the list. */
	status = style_list_append(realm, list, name, value, important);
	if (status != 0)
		return status;
	return 0;
}

/* Moves the start of a run of characters past its leading whitespace and its end before its trailing whitespace. */
static void
style_trim(
	const uint16_t *units,
	size_t *start,
	size_t *end)
{
	int space;

	/* The leading whitespace. */
	while (*start < *end) {
		space = style_is_space(units[*start]);
		if (!space)
			break;
		(*start)++;
	}

	/* The trailing whitespace. */
	while (*end > *start) {
		space = style_is_space(units[*end - 1U]);
		if (!space)
			break;
		(*end)--;
	}
}

/*
 * Tells whether a trimmed value ends in "!important" (the word in any
 * case, whitespace allowed after the "!"), and if it does moves its end
 * before the "!" and the whitespace before it.
 */
static int
style_strip_important(
	const uint16_t *units,
	size_t start,
	size_t *end)
{
	static const char word[] = "important";
	size_t length;
	size_t bang;
	size_t index;
	uint16_t unit;

	/* The value must be long enough for the word and a "!". */
	length = sizeof(word) - 1U;
	if (*end - start < length + 1U)
		return 0;

	/* The word at the end, in any case. */
	bang = *end - length;
	for (index = 0; index < length; index++) {
		unit = units[bang + index];
		if (unit >= 'A' && unit <= 'Z')
			unit = (uint16_t)(unit + 0x20U);
		if (unit != (uint16_t)word[index])
			return 0;
	}

	/* The "!" before it, after optional whitespace. */
	style_trim(units, &start, &bang);
	if (bang == start || units[bang - 1U] != '!')
		return 0;

	/* Succeeded: the value ends before the "!" and the whitespace before it. */
	bang--;
	style_trim(units, &start, &bang);
	*end = bang;
	return 1;
}

/* Writes declarations to an element's style attribute as other browsers serialize them. */
static int
style_write(
	struct vm_realm *realm,
	struct dom_element *element,
	const struct style_list *list)
{
	struct vm_string *style_atom;
	struct vm_string *text;
	struct wb_units units;
	int status;

	/* The text. */
	wb_units_init(&units);
	status = style_serialize(list, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* As a string. */
	text = vm_string_from_units(realm->heap, units.data, units.length);
	wb_units_release(&units);
	if (text == NULL)
		return ENOMEM;

	/* The attribute takes it. */
	style_atom = vm_atom_from_ascii(realm->heap, "style");
	if (style_atom == NULL)
		return ENOMEM;
	status = dom_element_set_attribute(element, style_atom, text);
	if (status != 0)
		return status;

	/* Succeeded: the attribute holds the declarations. */
	return 0;
}

/* Writes declarations as "name: value;" (with " !important" when they are), separated by spaces. */
static int
style_serialize(
	const struct style_list *list,
	struct wb_units *units)
{
	static const uint16_t colon[] = { ':', ' ' };
	static const uint16_t important[] = { ' ', '!', 'i', 'm', 'p', 'o', 'r', 't', 'a', 'n', 't' };
	static const uint16_t semicolon = ';';
	static const uint16_t space = ' ';
	const struct vm_string *name;
	const struct vm_string *value;
	uint32_t index;
	int priority;
	int status;

	/* Each declaration. */
	status = 0;
	for (index = 0; index < list->names->length && status == 0; index++) {
		name = (const struct vm_string *)vm_value_as_cell(list->names->elements[index]);
		value = (const struct vm_string *)vm_value_as_cell(list->values->elements[index]);
		priority = vm_to_boolean(list->priorities->elements[index]);

		/* A space before every declaration but the first. */
		if (index > 0)
			status = wb_units_append(units, &space, 1);

		/* The name, the colon, the value, the priority and the semicolon. */
		if (status == 0)
			status = vm_string_append_units(name, units);
		if (status == 0)
			status = wb_units_append(units, colon, 2);
		if (status == 0)
			status = vm_string_append_units(value, units);
		if (status == 0 && priority)
			status = wb_units_append(units, important, sizeof(important) / sizeof(important[0]));
		if (status == 0)
			status = wb_units_append(units, &semicolon, 1);
	}

	/* A failure to grow the text. */
	if (status != 0)
		return status;

	/* Succeeded: the text. */
	return 0;
}

/* Tells whether declarations have a property, and where the first one is. */
static int
style_find(
	const struct style_list *list,
	const struct vm_string *name,
	uint32_t *index)
{
	const struct vm_string *other;
	uint32_t at;
	int same;

	/* Compares each name. */
	for (at = 0; at < list->names->length; at++) {
		other = (const struct vm_string *)vm_value_as_cell(list->names->elements[at]);
		same = vm_string_equal(other, name);
		if (same) {
			*index = at;
			return 1;
		}
	}

	/* The property is not there. */
	return 0;
}

/*
 * Sets a property of an element's style to a value (the value is kept
 * only when the style engine reads it): the first declaration of it takes
 * the value and the others go, or a new one is appended.
 */
static int
style_set(
	struct vm_realm *realm,
	struct dom_element *element,
	struct vm_string *name,
	struct vm_string *value,
	int important)
{
	struct style_list list;
	uint32_t index;
	uint32_t later;
	int present;
	int valid;
	int status;

	/* A value the engine does not read changes nothing. */
	status = style_valid(realm, name, value, &valid);
	if (status != 0)
		return status;
	if (!valid)
		return 0;

	/* The declarations as they are. */
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* A property not there is appended. */
	present = style_find(&list, name, &index);
	if (!present) {
		status = style_list_append(realm, &list, name, value, important);
		if (status != 0)
			return status;
	} else {
		/* Otherwise its first declaration takes the value and the later ones go. */
		list.values->elements[index] = vm_value_cell(value);
		list.priorities->elements[index] = vm_value_boolean(important);
		later = index + 1U;
		while (later < list.names->length) {
			present = vm_string_equal((const struct vm_string *)vm_value_as_cell(list.names->elements[later]), name);
			if (!present) {
				later++;
				continue;
			}

			/* A later declaration of it. */
			status = style_list_remove_at(realm, &list, later);
			if (status != 0)
				return status;
		}
	}

	/* Succeeded: the attribute holds the declarations. */
	status = style_write(realm, element, &list);
	if (status != 0)
		return status;
	return 0;
}

/*
 * Removes every declaration of a property from an element's style, and
 * reports the value it had (the empty string when it had none).
 */
static int
style_remove(
	struct vm_realm *realm,
	struct dom_element *element,
	struct vm_string *name,
	vm_value *old_value)
{
	struct style_list list;
	uint32_t index;
	int present;
	int removed;
	int status;

	/* The declarations as they are. */
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;
	status = bind_string(realm, "", old_value);
	if (status != 0)
		return status;

	/* Each declaration of the property, the last one's value reported. */
	removed = 0;
	for (;;) {
		present = style_find(&list, name, &index);
		if (!present)
			break;
		*old_value = list.values->elements[index];
		status = style_list_remove_at(realm, &list, index);
		if (status != 0)
			return status;
		removed = 1;
	}

	/* Nothing removed leaves the attribute as it is. */
	if (!removed)
		return 0;

	/* Succeeded: the attribute holds the rest. */
	status = style_write(realm, element, &list);
	if (status != 0)
		return status;
	return 0;
}

/* Makes three empty arrays for declarations. */
static int
style_list_create(
	struct vm_realm *realm,
	struct style_list *list)
{
	int status;

	/* The names. */
	status = bind_array_create(realm, &list->names);
	if (status != 0)
		return status;

	/* The values. */
	status = bind_array_create(realm, &list->values);
	if (status != 0)
		return status;

	/* Succeeded: and the priorities. */
	status = bind_array_create(realm, &list->priorities);
	if (status != 0)
		return status;
	return 0;
}

/* Appends one declaration. */
static int
style_list_append(
	struct vm_realm *realm,
	struct style_list *list,
	struct vm_string *name,
	struct vm_string *value,
	int important)
{
	vm_value at;
	int status;

	/* The place after the last. */
	at = vm_value_int32((int32_t)list->names->length);

	/* The name. */
	status = vm_object_define(realm->heap, list->names, at, vm_value_cell(name), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* The value. */
	status = vm_object_define(realm->heap, list->values, at, vm_value_cell(value), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* Succeeded: and the priority. */
	status = vm_object_define(realm->heap, list->priorities, at, vm_value_boolean(important), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;
	return 0;
}

/* Removes the declaration at a place, moving the later ones down. */
static int
style_list_remove_at(
	struct vm_realm *realm,
	struct style_list *list,
	uint32_t index)
{
	uint32_t at;
	uint32_t length;
	int status;

	/* Each later declaration one place down. */
	length = list->names->length;
	for (at = index; at + 1U < length; at++) {
		list->names->elements[at] = list->names->elements[at + 1U];
		list->values->elements[at] = list->values->elements[at + 1U];
		list->priorities->elements[at] = list->priorities->elements[at + 1U];
	}

	/* The names are one shorter. */
	status = vm_array_set_length(realm->heap, list->names, length - 1U);
	if (status != 0)
		return status;

	/* The values. */
	status = vm_array_set_length(realm->heap, list->values, length - 1U);
	if (status != 0)
		return status;

	/* Succeeded: and the priorities. */
	status = vm_array_set_length(realm->heap, list->priorities, length - 1U);
	if (status != 0)
		return status;
	return 0;
}

/* Removes every declaration of a property from declarations. */
static int
style_remove_from_list(
	struct vm_realm *realm,
	struct style_list *list,
	const struct vm_string *name)
{
	uint32_t index;
	int present;
	int status;

	/* Each declaration of it, until none is left. */
	for (;;) {
		present = style_find(list, name, &index);
		if (!present)
			break;
		status = style_list_remove_at(realm, list, index);
		if (status != 0)
			return status;
	}

	/* Succeeded: the property is not there. */
	return 0;
}

/* Tells whether the style engine reads a property with a value. */
static int
style_valid(
	struct vm_realm *realm,
	const struct vm_string *name,
	const struct vm_string *value,
	int *valid)
{
	static const uint16_t colon = ':';
	struct wb_units text;
	int status;

	/* The declaration's text. */
	wb_units_init(&text);
	status = vm_string_append_units(name, &text);
	if (status == 0)
		status = wb_units_append(&text, &colon, 1);
	if (status == 0)
		status = vm_string_append_units(value, &text);
	if (status != 0) {
		wb_units_release(&text);
		return status;
	}

	/* The engine's answer. */
	status = css_declaration_valid(realm->heap, text.data, text.length, valid);
	wb_units_release(&text);
	if (status != 0)
		return status;

	/* Succeeded: the answer is in *valid. */
	return 0;
}

/* Reads a property name argument: lower case, but a custom property's as it is. */
static int
style_property_name(
	struct vm_realm *realm,
	vm_value value,
	struct vm_string **name)
{
	struct vm_string *text;
	struct wb_units units;
	size_t index;
	int custom;
	int status;

	/* The argument's characters. */
	status = bind_to_string(realm, value, &text);
	if (status != 0)
		return status;
	wb_units_init(&units);
	status = vm_string_append_units(text, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* A custom property's name stays as it is; any other goes to lower case. */
	custom = 0;
	if (units.length >= 2U && units.data[0] == '-' && units.data[1] == '-')
		custom = 1;
	for (index = 0; !custom && index < units.length; index++) {
		if (units.data[index] >= 'A' && units.data[index] <= 'Z')
			units.data[index] = (uint16_t)(units.data[index] + 0x20U);
	}

	/* The name's atom. */
	*name = vm_atom_from_units(realm->heap, units.data, units.length);
	wb_units_release(&units);
	if (*name == NULL)
		return ENOMEM;

	/* Succeeded: the name. */
	return 0;
}

/* Defines one accessor of CSSStyleDeclaration's prototype for a property, under a name. */
static int
style_accessor(
	struct bind_window *window,
	const char *name,
	struct vm_string *property)
{
	struct vm_realm *realm;
	struct vm_function *getter;
	struct vm_function *setter;
	struct vm_accessor *accessor;
	int status;

	/* The getter and the setter, which keep the property's name. */
	realm = window->realm;
	status = js_builtin_function(realm, name, 0, style_member_get, NULL, &getter);
	if (status != 0)
		return status;
	getter->data = vm_value_cell(property);
	status = js_builtin_function(realm, name, 1, style_member_set, NULL, &setter);
	if (status != 0)
		return status;
	setter->data = vm_value_cell(property);

	/* The accessor on the prototype. */
	accessor = vm_accessor_create(realm->heap, vm_value_cell(getter), vm_value_cell(setter));
	if (accessor == NULL)
		return ENOMEM;
	status = js_builtin_value(realm, window->prototypes[BIND_CSS_STYLE_DECLARATION], name, vm_value_cell(accessor), VM_PROPERTY_ACCESSOR | VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	if (status != 0)
		return status;

	/* Succeeded: the accessor is defined. */
	return 0;
}

/*
 * Writes a CSS property name in camel case: each "-" before a letter goes
 * and the letter becomes upper case; a leading "-webkit-" becomes
 * "webkit" or, with capital, "Webkit".
 */
static void
style_camel_case(
	const char *name,
	int capital,
	char out[STYLE_NAME_MAX])
{
	size_t in;
	size_t made;
	char character;

	/* A leading hyphen (of a vendor prefix) goes; the prefix's letter is upper case only with capital. */
	in = 0;
	made = 0;
	if (name[0] == '-') {
		in = 1;
		out[made] = name[in];
		if (capital && name[in] >= 'a' && name[in] <= 'z')
			out[made] = (char)(name[in] - 0x20);
		made++;
		in++;
	}

	/* Each other character, a hyphen making the next one upper case. */
	while (name[in] != '\0' && made + 1U < STYLE_NAME_MAX) {
		character = name[in];
		if (character == '-' && name[in + 1U] != '\0') {
			in++;
			character = name[in];
			if (character >= 'a' && character <= 'z')
				character = (char)(character - 0x20);
		}

		/* The character. */
		out[made] = character;
		made++;
		in++;
	}

	/* The name ends. */
	out[made] = '\0';
}

/* Tells whether a character is CSS whitespace. */
static int
style_is_space(
	uint16_t unit)
{
	/* Space, tab, line feed, form feed, carriage return. */
	switch (unit) {
	case 0x20U:
	case 0x09U:
	case 0x0aU:
	case 0x0cU:
	case 0x0dU:
		return 1;
	default:
		break;
	}

	/* Anything else. */
	return 0;
}

/* Reports the declarations as other browsers serialize them (cssText). */
static int
style_css_text_get(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list list;
	struct vm_string *text;
	struct wb_units units;
	int computed;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* A computed declaration's text is empty, as in other browsers. */
	computed = bind_computed_element(this_value, &element);
	if (computed) {
		status = bind_string(realm, "", result);
		return status;
	}

	/* The element's declarations. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* Their text. */
	wb_units_init(&units);
	status = style_serialize(&list, &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* As a string. */
	text = vm_string_from_units(realm->heap, units.data, units.length);
	wb_units_release(&units);
	if (text == NULL)
		return ENOMEM;

	/* Succeeded: the text. */
	*result = vm_value_cell(text);
	return 0;
}

/* Replaces the declarations with the ones of a text the engine reads (cssText). */
static int
style_css_text_set(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list given;
	struct style_list kept;
	struct vm_string *text;
	struct vm_string *name;
	struct vm_string *value;
	struct wb_units units;
	uint32_t index;
	int important;
	int valid;
	int status;

	/* The element and the text's declarations. */
	*result = VM_VALUE_UNDEFINED;
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = bind_to_string(realm, js_argument(args, count, 0), &text);
	if (status != 0)
		return status;
	status = style_list_create(realm, &given);
	if (status != 0)
		return status;
	wb_units_init(&units);
	status = vm_string_append_units(text, &units);
	if (status == 0)
		status = style_parse(realm, units.data, units.length, &given);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* The ones the engine reads, each property once (the last one given). */
	status = style_list_create(realm, &kept);
	if (status != 0)
		return status;
	for (index = 0; index < given.names->length; index++) {
		name = (struct vm_string *)vm_value_as_cell(given.names->elements[index]);
		value = (struct vm_string *)vm_value_as_cell(given.values->elements[index]);
		important = vm_to_boolean(given.priorities->elements[index]);
		status = style_valid(realm, name, value, &valid);
		if (status != 0)
			return status;
		if (!valid)
			continue;

		/* A property given again replaces the earlier declaration. */
		status = style_remove_from_list(realm, &kept, name);
		if (status != 0)
			return status;
		status = style_list_append(realm, &kept, name, value, important);
		if (status != 0)
			return status;
	}

	/* Succeeded: the attribute holds them. */
	status = style_write(realm, element, &kept);
	if (status != 0)
		return status;
	return 0;
}

/* Reports how many declarations there are (length). */
static int
style_length(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list list;
	int computed;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* A computed declaration lists the properties it reports. */
	computed = bind_computed_element(this_value, &element);
	if (computed) {
		*result = vm_value_int32((int32_t)bind_computed_count());
		return 0;
	}

	/* The element's declarations. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* Succeeded: how many. */
	*result = vm_value_int32((int32_t)list.names->length);
	return 0;
}

/* Reports the name of the declaration at a place, or the empty string past the end (item). */
static int
style_item(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list list;
	const char *name;
	uint32_t index;
	int computed;
	int status;

	/* A computed declaration names its properties in its order. */
	computed = bind_computed_element(this_value, &element);
	if (computed) {
		status = vm_to_uint32(realm, js_argument(args, count, 0), &index);
		if (status != 0)
			return status;
		name = bind_computed_name(index);
		if (name == NULL)
			name = "";
		status = bind_string(realm, name, result);
		return status;
	}

	/* The element's declarations and the place. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = vm_to_uint32(realm, js_argument(args, count, 0), &index);
	if (status != 0)
		return status;
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* A place past the end has no name. */
	if (index >= list.names->length) {
		status = bind_string(realm, "", result);
		return status;
	}

	/* Succeeded: the name. */
	*result = list.names->elements[index];
	return 0;
}

/* Reports a property's value, or the empty string (getPropertyValue). */
static int
style_get_property_value(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list list;
	struct vm_string *name;
	uint32_t index;
	int present;
	int computed;
	int status;

	/* A computed declaration's value is the element's resolved one. */
	computed = bind_computed_element(this_value, &element);
	if (computed) {
		status = style_property_name(realm, js_argument(args, count, 0), &name);
		if (status != 0)
			return status;
		status = bind_computed_value(realm, element, name, result);
		return status;
	}

	/* The element, the name and the declarations. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_property_name(realm, js_argument(args, count, 0), &name);
	if (status != 0)
		return status;
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* A property not there is the empty string. */
	present = style_find(&list, name, &index);
	if (!present) {
		status = bind_string(realm, "", result);
		return status;
	}

	/* Succeeded: its value. */
	*result = list.values->elements[index];
	return 0;
}

/* Reports "important" for a property declared !important, or the empty string (getPropertyPriority). */
static int
style_get_property_priority(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list list;
	struct vm_string *name;
	uint32_t index;
	int present;
	int important;
	int computed;
	int status;

	/* A computed declaration has no priorities. */
	computed = bind_computed_element(this_value, &element);
	if (computed) {
		status = bind_string(realm, "", result);
		return status;
	}

	/* The element, the name and the declarations. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_property_name(realm, js_argument(args, count, 0), &name);
	if (status != 0)
		return status;
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* Whether the property is there and important. */
	important = 0;
	present = style_find(&list, name, &index);
	if (present)
		important = vm_to_boolean(list.priorities->elements[index]);

	/* The priority's word. */
	if (important) {
		status = bind_string(realm, "important", result);
	} else {
		status = bind_string(realm, "", result);
	}

	/* A string that could not be made. */
	if (status != 0)
		return status;

	/* Succeeded: the word. */
	return 0;
}

/*
 * Sets a property to a value with an optional "important" priority; an
 * empty value removes it (setProperty).
 */
static int
style_set_property(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct vm_string *name;
	struct vm_string *value;
	struct vm_string *priority;
	vm_value old_value;
	int important;
	int status;

	/* The element, the name, the value and the priority. */
	*result = VM_VALUE_UNDEFINED;
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_property_name(realm, js_argument(args, count, 0), &name);
	if (status != 0)
		return status;
	status = bind_to_string(realm, js_argument(args, count, 1), &value);
	if (status != 0)
		return status;
	priority = NULL;
	if (count > 2U && args[2] != VM_VALUE_UNDEFINED) {
		status = bind_to_string(realm, args[2], &priority);
		if (status != 0)
			return status;
	}

	/* An empty value removes the property. */
	if (value->length == 0) {
		status = style_remove(realm, element, name, &old_value);
		return status;
	}

	/* The priority must be "important" or nothing. */
	important = 0;
	if (priority != NULL && priority->length != 0) {
		important = vm_string_equal_ascii(priority, "important");
		if (!important)
			return 0;
	}

	/* Succeeded: the property is set. */
	status = style_set(realm, element, name, value, important);
	if (status != 0)
		return status;
	return 0;
}

/* Removes a property and reports the value it had (removeProperty). */
static int
style_remove_property(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct vm_string *name;
	int status;

	/* The element and the name. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_property_name(realm, js_argument(args, count, 0), &name);
	if (status != 0)
		return status;

	/* Succeeded: the property is gone. */
	status = style_remove(realm, element, name, result);
	if (status != 0)
		return status;
	return 0;
}

/* Reports the value of the property an accessor stands for, or the empty string. */
static int
style_member_get(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct style_list list;
	struct vm_function *callee;
	struct vm_string *name;
	uint32_t index;
	int present;
	int computed;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The property; a computed declaration's value is the element's resolved one. */
	callee = js_builtin_callee(realm);
	name = (struct vm_string *)vm_value_as_cell(callee->data);
	computed = bind_computed_element(this_value, &element);
	if (computed) {
		status = bind_computed_value(realm, element, name, result);
		return status;
	}

	/* The element and the declarations. */
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	status = style_read(realm, element, &list);
	if (status != 0)
		return status;

	/* A property not there is the empty string. */
	present = style_find(&list, name, &index);
	if (!present) {
		status = bind_string(realm, "", result);
		return status;
	}

	/* Succeeded: its value. */
	*result = list.values->elements[index];
	return 0;
}

/* Sets the property an accessor stands for; an empty value removes it. */
static int
style_member_set(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct vm_function *callee;
	struct vm_string *name;
	struct vm_string *value;
	vm_value old_value;
	vm_value given;
	int status;

	/* The element and the property. */
	*result = VM_VALUE_UNDEFINED;
	status = style_element(realm, this_value, &element);
	if (status != 0)
		return status;
	callee = js_builtin_callee(realm);
	name = (struct vm_string *)vm_value_as_cell(callee->data);

	/* Null removes the property, as the empty string does. */
	given = js_argument(args, count, 0);
	if (given == VM_VALUE_NULL) {
		status = style_remove(realm, element, name, &old_value);
		return status;
	}

	/* The value. */
	status = bind_to_string(realm, given, &value);
	if (status != 0)
		return status;

	/* An empty value removes the property. */
	if (value->length == 0) {
		status = style_remove(realm, element, name, &old_value);
		return status;
	}

	/* Succeeded: the property is set, without a priority. */
	status = style_set(realm, element, name, value, 0);
	if (status != 0)
		return status;
	return 0;
}
