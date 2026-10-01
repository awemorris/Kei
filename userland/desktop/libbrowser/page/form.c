/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Forms (ws074-p032): typing into the focused text control, the
 * activation of buttons, checkboxes and radio buttons, and a form's
 * submission.
 *
 * A text control takes the characters a key types, and Backspace, Delete,
 * the arrows, Home and End move and edit at its caret; each change fires
 * an input event.  A submission gathers the form's entries in tree order
 * (the HTML Standard's "constructing the entry list": named controls that
 * are not disabled, the submitter among the buttons, checked checkboxes
 * and radio buttons, a select's chosen option), encodes them as
 * application/x-www-form-urlencoded in the form's character encoding
 * (UTF-8, or windows-1252 with the characters it lacks written as
 * "&#N;"), and gives the view the action with that query to follow.  Only
 * GET is submitted in this pass.
 */

#include "page/page.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The deepest element nesting walked (the parser caps nesting too). */
#define FORM_DEPTH		512

/* The most characters a field takes when maxlength says nothing. */
#define FORM_LENGTH_MAX		524288U

/* The encodings a form is submitted in. */
enum form_encoding {
	FORM_UTF8,
	FORM_WINDOWS_1252
};

/* The editing a key asks of a text control. */
enum form_edit {
	FORM_EDIT_NONE,
	FORM_EDIT_BACKSPACE,
	FORM_EDIT_DELETE,
	FORM_EDIT_LEFT,
	FORM_EDIT_RIGHT,
	FORM_EDIT_HOME,
	FORM_EDIT_END,
	FORM_EDIT_NEWLINE
};

/* A DOM key name that edits, and how. */
struct form_key {
	const char *name;
	int edit;
};

/*
 * The keys a text control acts on.  The table is constant for the life of
 * the program and ends with a NULL name.
 */
static const struct form_key form_keys[] = {
	{ "Backspace", FORM_EDIT_BACKSPACE },
	{ "Delete", FORM_EDIT_DELETE },
	{ "ArrowLeft", FORM_EDIT_LEFT },
	{ "ArrowRight", FORM_EDIT_RIGHT },
	{ "Home", FORM_EDIT_HOME },
	{ "End", FORM_EDIT_END },
	{ "Enter", FORM_EDIT_NEWLINE },
	{ NULL, FORM_EDIT_NONE }
};

static struct dom_element *form_editable(struct page *page);
static int form_is_text(int kind);
static int form_edit_of(const char *key);
static int form_insert(struct page *page, struct dom_element *element, const char *text);
static int form_edit(struct page *page, struct dom_element *element, int edit);
static int form_changed(struct page *page, struct dom_element *element);
static size_t form_step_back(const struct wb_units *units, size_t offset);
static size_t form_step_on(const struct wb_units *units, size_t offset);
static size_t form_limit(const struct dom_element *element);
static struct dom_element *form_owner(struct dom_element *element);
static struct dom_element *form_default_button(struct dom_element *form);
static int form_disabled(const struct dom_element *element);
static int form_toggle(struct page *page, struct dom_element *element);
static int form_clear_group(struct page *page, struct dom_element *element);
static int form_entries(struct dom_element *form, struct dom_element *submitter, int encoding, struct wb_buffer *query);
static int form_entry_value(struct dom_element *element, int kind, const struct vm_string *name, const struct dom_element *submitter, int encoding, struct wb_units *value, int *included);
static int form_add_entry(struct wb_buffer *query, const struct vm_string *name, const uint16_t *value, size_t length, int encoding);
static int form_encode(struct wb_buffer *query, const uint16_t *units, size_t length, int encoding);
static int form_encode_byte(struct wb_buffer *query, unsigned char byte);
static int form_encoding(struct page *page, const struct dom_element *form);
static int form_charset_named(const struct vm_string *label, size_t start, size_t end, int *encoding);
static int form_action(const struct dom_element *form, struct wb_buffer *href);
static int form_option_value(const struct dom_element *option, struct wb_units *out);
static const struct dom_node *form_next(const struct dom_node *node, const struct dom_node *root);
static int form_equal_folded(const struct vm_string *string, const char *ascii);

/*
 * Tells whether the focused element takes typing: a text field, a
 * password field or a textarea that is neither disabled nor read-only.
 */
int
page_is_editing(
	struct page *page)
{
	struct dom_element *element;

	/* The focused control that takes typing, if any. */
	element = form_editable(page);
	if (element == NULL)
		return 0;

	/* The focus is in a text control. */
	return 1;
}

/*
 * Does what a key does in the focused text control: types its text, or
 * moves the caret or deletes (Backspace, Delete, the arrows, Home, End;
 * Enter in a textarea starts a new line).  *handled says whether the key
 * was the control's; the others keep their default action.
 */
int
page_edit_key(
	struct page *page,
	const char *key,
	const char *text,
	unsigned modifiers,
	int *handled)
{
	struct dom_element *element;
	int edit;
	int kind;
	int error;

	/* Only a text control with the focus takes keys. */
	*handled = 0;
	element = form_editable(page);
	if (element == NULL)
		return 0;

	/* A key held with Control, Alt or Meta is a command, not typing. */
	if ((modifiers & (BIND_MOD_CTRL | BIND_MOD_ALT | BIND_MOD_META)) != 0U)
		return 0;

	/* Text the key types goes in at the caret. */
	if (text != NULL && text[0] != '\0') {
		*handled = 1;
		error = form_insert(page, element, text);
		return error;
	}

	/* The editing keys; Enter only in a textarea (in a field it submits). */
	edit = form_edit_of(key);
	if (edit == FORM_EDIT_NONE)
		return 0;
	kind = dom_control_kind(element);
	if (edit == FORM_EDIT_NEWLINE && kind != DOM_CONTROL_TEXTAREA)
		return 0;

	/* The edit. */
	*handled = 1;
	error = form_edit(page, element, edit);
	if (error != 0)
		return error;

	/* Succeeded: the key was the control's. */
	return 0;
}

/*
 * Carries out the activation of a form control (a click on it, or Enter or
 * Space on it): a checkbox or radio button changes, a submit button (or a
 * text field, by implicit submission) submits its form.  *handled says the
 * element is a control with an activation; *found says a submission was
 * made, with the location to go to in href.
 */
int
page_activate_control(
	struct page *page,
	struct dom_element *element,
	int implicit,
	struct wb_buffer *href,
	int *found,
	int *handled)
{
	struct dom_element *form;
	struct dom_element *submitter;
	struct bind_mouse mouse;
	int kind;
	int disabled;
	int canceled;
	int error;

	/* Nothing is done until the element is known to be a control. */
	*found = 0;
	*handled = 0;
	kind = dom_control_kind(element);
	if (kind == DOM_CONTROL_NONE)
		return 0;

	/* A disabled control does nothing. */
	disabled = form_disabled(element);
	if (disabled)
		return 0;

	/* A checkbox or a radio button changes. */
	if (kind == DOM_CONTROL_CHECKBOX || kind == DOM_CONTROL_RADIO) {
		*handled = 1;
		error = form_toggle(page, element);
		return error;
	}

	/* Only a submit button, or a field by implicit submission, submits. */
	submitter = NULL;
	if (kind == DOM_CONTROL_SUBMIT)
		submitter = element;
	if (implicit && (kind == DOM_CONTROL_TEXT || kind == DOM_CONTROL_PASSWORD))
		submitter = element;
	if (submitter == NULL)
		return 0;

	/* The form it belongs to; a control outside a form submits nothing. */
	*handled = 1;
	form = form_owner(element);
	if (form == NULL)
		return 0;

	/*
	 * A field submits through its form's default button: that button's click
	 * is fired, and a form with no such button submits without one.
	 */
	if (submitter != element || kind != DOM_CONTROL_SUBMIT) {
		submitter = form_default_button(form);
		if (submitter != NULL) {
			memset(&mouse, 0, sizeof(mouse));
			error = bind_fire_mouse_event(page->window, &submitter->node, "click", &mouse, &canceled);
			if (error != 0)
				return error;
			if (canceled)
				return 0;
		}
	}

	/* The submission. */
	error = page_submit_form(page, form, submitter, href, found);
	if (error != 0)
		return error;

	/* Succeeded: the control is activated. */
	return 0;
}

/*
 * Submits a form with a submitter (NULL for none): fires submit at the
 * form, and unless it is canceled writes the location its entries go to
 * (the action with the entries as its query) and sets *found.  A form
 * whose method is not GET is not submitted in this pass (the console says
 * so).
 */
int
page_submit_form(
	struct page *page,
	struct dom_element *form,
	struct dom_element *submitter,
	struct wb_buffer *href,
	int *found)
{
	struct vm_string *method;
	struct wb_buffer query;
	int is_get;
	int canceled;
	int encoding;
	int error;

	/* Nothing is found until the submission is made. */
	*found = 0;

	/* The method: GET unless the form (or its submitter) says otherwise. */
	method = NULL;
	if (submitter != NULL)
		method = dom_attribute_ascii(submitter, "formmethod");
	if (method == NULL)
		method = dom_attribute_ascii(form, "method");
	is_get = 1;
	if (method != NULL)
		is_get = form_equal_folded(method, "get");
	if (!is_get) {
		bind_console(page->window, 2, "form: only GET forms are submitted yet");
		return 0;
	}

	/* The form hears of it first, and may cancel it. */
	error = bind_fire_event(page->window, &form->node, "submit", BIND_EVENT_BUBBLES | BIND_EVENT_CANCELABLE, &canceled);
	if (error != 0)
		return error;
	if (canceled)
		return 0;

	/* The entries, encoded. */
	encoding = form_encoding(page, form);
	wb_buffer_init(&query);
	error = form_entries(form, submitter, encoding, &query);

	/* The action, without its query and fragment, and the new query. */
	wb_buffer_clear(href);
	if (error == 0)
		error = form_action(form, href);
	if (error == 0)
		error = wb_buffer_append_byte(href, '?');
	if (error == 0)
		error = wb_buffer_append(href, query.data, query.length);
	wb_buffer_release(&query);
	if (error != 0)
		return error;

	/* Succeeded: the location to go to is written. */
	*found = 1;
	return 0;
}

/*
 * Moves the focused text control's caret to the character boundary
 * nearest a point of the document (a press in it), using the geometry the
 * display list recorded.
 */
int
page_place_caret(
	struct page *page,
	int x)
{
	struct dom_element *element;
	struct dom_control *control;
	const struct layout_box *box;
	struct text_font font;
	struct wb_units value;
	layout_unit target;
	layout_unit pen;
	layout_unit advance;
	size_t index;
	size_t best;
	uint16_t bullet;
	int kind;
	int error;

	/* Only a single-line field drawn on the page has a caret to place from a point. */
	element = form_editable(page);
	if (element == NULL)
		return 0;
	kind = dom_control_kind(element);
	if (kind != DOM_CONTROL_TEXT && kind != DOM_CONTROL_PASSWORD)
		return 0;
	control = dom_control_of(element);
	if (control == NULL)
		return ENOMEM;
	if (!control->drawn || !page->laid_out)
		return 0;
	box = layout_box_of(&page->layout, &element->node);
	if (box == NULL)
		return 0;

	/* The point from the text's start, as the text was scrolled. */
	target = (layout_unit)x * LAYOUT_UNIT - control->content_x + control->scroll_x;
	layout_font_of(&page->layout, &box->style, &font);

	/* The value, whose characters are measured one by one (a password's as bullets). */
	wb_units_init(&value);
	error = dom_control_value(element, &value);
	if (error != 0) {
		wb_units_release(&value);
		return error;
	}

	/* The boundary nearest the point: past a character when the point is beyond its middle. */
	pen = 0;
	best = 0;
	bullet = 0x2022U;
	for (index = 0; index < value.length; index++) {
		if (kind == DOM_CONTROL_PASSWORD)
			error = layout_units_width(page->layout.text, &font, &bullet, 1, &advance);
		else
			error = layout_units_width(page->layout.text, &font, &value.data[index], 1, &advance);
		if (error != 0)
			break;
		if (target < pen + advance / 2)
			break;
		pen += advance;
		best = index + 1U;
	}

	/* The value is no longer needed. */
	wb_units_release(&value);
	if (error != 0)
		return error;

	/* The caret moves there, which the page paints again. */
	control->caret = best;
	page->focus_generation++;

	/* Succeeded: the caret is placed. */
	return 0;
}

/*
 * Puts the focused text control's caret after its text (the keyboard moved
 * the focus there; the browser selects nothing, so typing goes at the end).
 */
int
page_caret_to_end(
	struct page *page)
{
	struct dom_element *element;
	struct dom_control *control;
	struct wb_units value;
	int error;

	/* Only a text control with the focus has a caret. */
	element = form_editable(page);
	if (element == NULL)
		return 0;
	control = dom_control_of(element);
	if (control == NULL)
		return ENOMEM;

	/* The value's length is where the caret goes. */
	wb_units_init(&value);
	error = dom_control_value(element, &value);
	if (error == 0)
		control->caret = value.length;
	wb_units_release(&value);
	if (error != 0)
		return error;

	/* Succeeded: the caret is at the end, which the page paints again. */
	page->focus_generation++;
	return 0;
}

/*
 * Adds the caret of the focused text control to the display list just made,
 * while the view has its program's focus: a one pixel line where the
 * display list placed it.
 */
int
page_paint_caret(
	struct page *page)
{
	struct dom_element *element;
	struct dom_control *control;
	int error;

	/* A caret only in a focused text control, while the view has the focus. */
	if (!page->window_focused)
		return 0;
	element = form_editable(page);
	if (element == NULL || element->control == NULL)
		return 0;
	control = element->control;
	if (!control->drawn)
		return 0;

	/* The caret, a pixel wide, in the text's color. */
	error = paint_add_rect(&page->paint, control->caret_x, control->caret_top, LAYOUT_UNIT, control->caret_height, control->caret_color);
	if (error != 0)
		return error;

	/* Succeeded: the caret is drawn. */
	return 0;
}

/* Finds the focused element when it is a text control that takes typing (NULL otherwise). */
static struct dom_element *
form_editable(
	struct page *page)
{
	struct dom_element *element;
	struct vm_string *read_only;
	int connected;
	int kind;
	int text;
	int disabled;

	/* The focused element, still in the document. */
	element = page->focused;
	if (element == NULL)
		return NULL;
	connected = dom_is_inclusive_ancestor(&page->document->node, &element->node);
	if (!connected)
		return NULL;

	/* A text control. */
	kind = dom_control_kind(element);
	text = form_is_text(kind);
	if (!text)
		return NULL;

	/* Neither disabled nor read-only. */
	disabled = form_disabled(element);
	if (disabled)
		return NULL;
	read_only = dom_attribute_ascii(element, "readonly");
	if (read_only != NULL)
		return NULL;

	/* The control takes typing. */
	return element;
}

/* Tells whether a kind of control is edited as text. */
static int
form_is_text(
	int kind)
{
	/* A field, a password field, a textarea. */
	if (kind == DOM_CONTROL_TEXT)
		return 1;
	if (kind == DOM_CONTROL_PASSWORD)
		return 1;
	if (kind == DOM_CONTROL_TEXTAREA)
		return 1;

	/* The other kinds are not. */
	return 0;
}

/* Finds how a DOM key name edits (FORM_EDIT_NONE for a key that does not). */
static int
form_edit_of(
	const char *key)
{
	size_t index;
	int differs;

	/* The table's names. */
	if (key == NULL)
		return FORM_EDIT_NONE;
	for (index = 0; form_keys[index].name != NULL; index++) {
		differs = strcmp(key, form_keys[index].name);
		if (differs == 0)
			return form_keys[index].edit;
	}

	/* A key that does not edit. */
	return FORM_EDIT_NONE;
}

/*
 * Inserts UTF-8 text at a control's caret (a single-line field drops line
 * breaks), up to its maxlength, and fires input.
 */
static int
form_insert(
	struct page *page,
	struct dom_element *element,
	const char *text)
{
	struct dom_control *control;
	struct wb_units value;
	struct wb_units typed;
	size_t limit;
	size_t room;
	size_t index;
	size_t kept;
	size_t caret;
	size_t inserted;
	int single_line;
	int kind;
	int error;

	/* The typed text as UTF-16, without line breaks in a single-line field. */
	wb_units_init(&typed);
	error = wb_utf8_to_units((const unsigned char *)text, strlen(text), &typed);
	if (error != 0) {
		wb_units_release(&typed);
		return error;
	}

	/* A textarea keeps the line breaks; another field drops them. */
	single_line = 1;
	kind = dom_control_kind(element);
	if (kind == DOM_CONTROL_TEXTAREA)
		single_line = 0;
	kept = 0;
	for (index = 0; index < typed.length; index++) {
		if (single_line && typed.data[index] == 0x0aU)
			continue;
		if (single_line && typed.data[index] == 0x0dU)
			continue;
		typed.data[kept] = typed.data[index];
		kept++;
	}

	/* The text is as long as what was kept. */
	typed.length = kept;

	/* The value as it is, with its caret. */
	control = dom_control_of(element);
	wb_units_init(&value);
	error = ENOMEM;
	if (control != NULL)
		error = dom_control_value(element, &value);
	if (error != 0) {
		wb_units_release(&typed);
		wb_units_release(&value);
		return error;
	}

	/* A caret past the value's end comes back to it. */
	if (control->caret > value.length)
		control->caret = value.length;

	/* No more than maxlength leaves room for. */
	limit = form_limit(element);
	room = 0;
	if (value.length < limit)
		room = limit - value.length;
	if (typed.length > room)
		typed.length = room;

	/* The new value: the part before the caret, the text, the part after. */
	caret = control->caret;
	inserted = typed.length;
	error = wb_units_reserve(&value, inserted);
	if (error == 0) {
		memmove(value.data + caret + inserted, value.data + caret, (value.length - caret) * sizeof(uint16_t));
		memcpy(value.data + caret, typed.data, inserted * sizeof(uint16_t));
		value.length += inserted;
	}

	/* The typed text is in the value now. */
	wb_units_release(&typed);

	/* The control takes it (which puts the caret at the end), and the caret goes after the text. */
	if (error == 0)
		error = dom_control_set_value(element, value.data, value.length);
	if (error == 0)
		control->caret = caret + inserted;
	wb_units_release(&value);
	if (error != 0)
		return error;

	/* The page hears of the change. */
	error = form_changed(page, element);
	if (error != 0)
		return error;

	/* Succeeded: the text is in. */
	return 0;
}

/* Carries out an editing key at a control's caret. */
static int
form_edit(
	struct page *page,
	struct dom_element *element,
	int edit)
{
	struct dom_control *control;
	struct wb_units value;
	size_t caret;
	size_t start;
	size_t end;
	int changed;
	int error;

	/* The value and the caret. */
	control = dom_control_of(element);
	if (control == NULL)
		return ENOMEM;
	wb_units_init(&value);
	error = dom_control_value(element, &value);
	if (error != 0) {
		wb_units_release(&value);
		return error;
	}

	/* A caret past the value's end comes back to it. */
	caret = control->caret;
	if (caret > value.length)
		caret = value.length;

	/* The range an edit removes (none for a move), and where the caret goes. */
	start = caret;
	end = caret;
	switch (edit) {
	case FORM_EDIT_BACKSPACE:
		/* The character before the caret. */
		start = form_step_back(&value, start);
		break;
	case FORM_EDIT_DELETE:
		/* The character after the caret. */
		end = form_step_on(&value, end);
		break;
	case FORM_EDIT_LEFT:
		/* One character back. */
		caret = form_step_back(&value, caret);
		break;
	case FORM_EDIT_RIGHT:
		/* One character on. */
		caret = form_step_on(&value, caret);
		break;
	case FORM_EDIT_HOME:
		caret = 0;
		break;
	case FORM_EDIT_END:
		caret = value.length;
		break;
	default:
		break;
	}

	/* A new line in a textarea is typed like text. */
	if (edit == FORM_EDIT_NEWLINE) {
		wb_units_release(&value);
		error = form_insert(page, element, "\n");
		return error;
	}

	/* A removal takes the range out, and the caret stays at its start. */
	changed = 0;
	if (end > start) {
		memmove(value.data + start, value.data + end, (value.length - end) * sizeof(uint16_t));
		value.length -= end - start;
		error = dom_control_set_value(element, value.data, value.length);
		caret = start;
		changed = 1;
	}

	/* The value is the control's own again. */
	wb_units_release(&value);
	if (error != 0)
		return error;

	/* The caret's new place, which the page paints again. */
	control->caret = caret;
	page->focus_generation++;

	/* A removal is a change the page hears of. */
	if (changed) {
		error = form_changed(page, element);
		if (error != 0)
			return error;
	}

	/* Succeeded: the edit is done. */
	return 0;
}

/* Finds the offset one character before another in UTF-16 text (both halves of a surrogate pair). */
static size_t
form_step_back(
	const struct wb_units *units,
	size_t offset)
{
	uint16_t unit;

	/* The start has nothing before it. */
	if (offset == 0)
		return 0;

	/* One unit back, and one more when it is the second half of a pair. */
	offset--;
	if (offset == 0)
		return 0;
	unit = units->data[offset];
	if (unit >= 0xdc00U && unit <= 0xdfffU)
		offset--;

	/* The character's start. */
	return offset;
}

/* Finds the offset one character after another in UTF-16 text (both halves of a surrogate pair). */
static size_t
form_step_on(
	const struct wb_units *units,
	size_t offset)
{
	uint16_t unit;

	/* The end has nothing after it. */
	if (offset >= units->length)
		return units->length;

	/* One unit on, and one more when the next is the second half of a pair. */
	offset++;
	if (offset >= units->length)
		return units->length;
	unit = units->data[offset];
	if (unit >= 0xdc00U && unit <= 0xdfffU)
		offset++;

	/* The next character's start. */
	return offset;
}

/* Tells the page that a control's value changed: it is painted again, and it gets input. */
static int
form_changed(
	struct page *page,
	struct dom_element *element)
{
	int canceled;
	int error;

	/* The control is painted again with its new value. */
	page->focus_generation++;

	/* input, which bubbles and cannot be canceled. */
	error = bind_fire_event(page->window, &element->node, "input", BIND_EVENT_BUBBLES, &canceled);
	if (error != 0)
		return error;

	/* Succeeded: the page heard of it. */
	return 0;
}

/* Reads a control's maxlength (the most characters it takes; a large number when it has none). */
static size_t
form_limit(
	const struct dom_element *element)
{
	struct vm_string *attribute;
	size_t index;
	size_t number;
	uint16_t unit;

	/* No maxlength is no limit worth the name. */
	attribute = dom_attribute_ascii(element, "maxlength");
	if (attribute == NULL || attribute->length == 0)
		return FORM_LENGTH_MAX;

	/* The digits (a value that is not a number is no limit). */
	number = 0;
	for (index = 0; index < attribute->length; index++) {
		unit = vm_string_at(attribute, index);
		if (unit < '0' || unit > '9')
			return FORM_LENGTH_MAX;
		if (number < FORM_LENGTH_MAX)
			number = number * 10U + (size_t)(unit - '0');
	}

	/* The limit. */
	return number;
}

/* Finds the form a control belongs to: its nearest <form> ancestor (NULL outside one). */
static struct dom_element *
form_owner(
	struct dom_element *element)
{
	struct dom_node *node;
	int depth;
	int is_form;

	/* The ancestors, nearest first. */
	node = element->node.parent;
	for (depth = 0; node != NULL && depth < FORM_DEPTH; depth++) {
		is_form = dom_element_is(node, DOM_NS_HTML, DOM_TAG_FORM);
		if (is_form)
			return (struct dom_element *)node;
		node = node->parent;
	}

	/* No form around it. */
	return NULL;
}

/* Finds a form's default button: its first submit button in tree order (NULL when it has none). */
static struct dom_element *
form_default_button(
	struct dom_element *form)
{
	const struct dom_node *node;
	struct dom_element *element;
	int kind;

	/* The form's descendants in tree order. */
	for (node = form->node.first_child; node != NULL; node = form_next(node, &form->node)) {
		if (node->type != DOM_ELEMENT)
			continue;

		/* The first submit button is the default one. */
		element = (struct dom_element *)node;
		kind = dom_control_kind(element);
		if (kind == DOM_CONTROL_SUBMIT)
			return element;
	}

	/* The form has no submit button. */
	return NULL;
}

/* Tells whether a control is disabled: its own disabled attribute (fieldsets are not looked at in this pass). */
static int
form_disabled(
	const struct dom_element *element)
{
	struct vm_string *disabled;

	/* The disabled attribute's presence. */
	disabled = dom_attribute_ascii(element, "disabled");
	if (disabled != NULL)
		return 1;

	/* The control is enabled. */
	return 0;
}

/*
 * Changes a checkbox (checked or not) or checks a radio button and clears
 * the others of its group (the same name in the same form), then fires
 * input and change.
 */
static int
form_toggle(
	struct page *page,
	struct dom_element *element)
{
	struct dom_control *control;
	int canceled;
	int checked;
	int kind;
	int error;

	/* The state that keeps the checkedness. */
	control = dom_control_of(element);
	if (control == NULL)
		return ENOMEM;
	checked = dom_control_checked(element);
	kind = dom_control_kind(element);

	/*
	 * checked_dirty tells the painting and the submission that the control's
	 * own checkedness counts from now on, not the checked attribute.  A
	 * checkbox flips; a checked radio button stays checked.
	 */
	if (kind == DOM_CONTROL_CHECKBOX) {
		control->checked = !checked;
		control->checked_dirty = 1;
	} else if (!checked) {
		control->checked = 1;
		control->checked_dirty = 1;

		/* The other radio buttons of its group are cleared. */
		error = form_clear_group(page, element);
		if (error != 0)
			return error;
	}

	/* The control is painted again. */
	page->focus_generation++;

	/* input and change, which bubble. */
	error = bind_fire_event(page->window, &element->node, "input", BIND_EVENT_BUBBLES, &canceled);
	if (error != 0)
		return error;
	error = bind_fire_event(page->window, &element->node, "change", BIND_EVENT_BUBBLES, &canceled);
	if (error != 0)
		return error;

	/* Succeeded: the control changed. */
	return 0;
}

/*
 * Clears the other radio buttons of a radio button's group: the ones with
 * the same name in the same form (or in the document, outside a form).
 */
static int
form_clear_group(
	struct page *page,
	struct dom_element *element)
{
	struct dom_control *other_control;
	struct dom_element *other;
	struct dom_element *form;
	struct vm_string *name;
	struct vm_string *other_name;
	const struct dom_node *node;
	const struct dom_node *root;
	int same;
	int kind;

	/* A button without a name has no group. */
	name = dom_attribute_ascii(element, "name");
	if (name == NULL)
		return 0;

	/* The group is searched in the form, or the document. */
	form = form_owner(element);
	root = &page->document->node;
	if (form != NULL)
		root = &form->node;

	/* Each other radio button of the same name. */
	for (node = root->first_child; node != NULL; node = form_next(node, root)) {
		if (node->type != DOM_ELEMENT)
			continue;
		if (node == &element->node)
			continue;
		other = (struct dom_element *)node;
		kind = dom_control_kind(other);
		if (kind != DOM_CONTROL_RADIO)
			continue;
		other_name = dom_attribute_ascii(other, "name");
		if (other_name == NULL)
			continue;
		same = vm_string_equal(other_name, name);
		if (!same)
			continue;

		/* The other button of the group is no longer checked, whatever its attribute says. */
		other_control = dom_control_of(other);
		if (other_control == NULL)
			return ENOMEM;
		other_control->checked = 0;
		other_control->checked_dirty = 1;
	}

	/* Succeeded: the group has one checked button. */
	return 0;
}

/*
 * Writes a form's entries, encoded and joined with "&", in tree order:
 * each named control that is not disabled, a button only when it is the
 * submitter, a checkbox or radio button only when checked ("on" without a
 * value), a select's chosen option's value, a field's or textarea's value
 * (a textarea's line breaks as CR LF), and a hidden control named
 * _charset_ as the encoding's name.
 */
static int
form_entries(
	struct dom_element *form,
	struct dom_element *submitter,
	int encoding,
	struct wb_buffer *query)
{
	const struct dom_node *node;
	struct dom_element *element;
	struct vm_string *name;
	struct wb_units value;
	int included;
	int disabled;
	int kind;
	int error;

	/* The form's controls in tree order. */
	error = 0;
	wb_units_init(&value);
	for (node = form->node.first_child; node != NULL && error == 0; node = form_next(node, &form->node)) {
		if (node->type != DOM_ELEMENT)
			continue;
		element = (struct dom_element *)node;
		kind = dom_control_kind(element);
		if (kind == DOM_CONTROL_NONE)
			continue;

		/* A control without a name, or a disabled one, is not an entry. */
		name = dom_attribute_ascii(element, "name");
		if (name == NULL || name->length == 0)
			continue;
		disabled = form_disabled(element);
		if (disabled)
			continue;

		/* Whether the control gives an entry, and its value; then the entry. */
		error = form_entry_value(element, kind, name, submitter, encoding, &value, &included);
		if (error == 0 && included)
			error = form_add_entry(query, name, value.data, value.length, encoding);
	}

	/* The value's buffer is no longer needed. */
	wb_units_release(&value);
	if (error != 0)
		return error;

	/* Succeeded: the entries are written. */
	return 0;
}

/*
 * Works out one control's entry: whether it gives one (*included) and its
 * value -- only the submitter among the buttons, a checked checkbox or
 * radio button with its value or "on", a select's chosen option's value,
 * the encoding's name for a hidden control named _charset_, a textarea's
 * value with CR LF line breaks, any other control's value.
 */
static int
form_entry_value(
	struct dom_element *element,
	int kind,
	const struct vm_string *name,
	const struct dom_element *submitter,
	int encoding,
	struct wb_units *value,
	int *included)
{
	static const uint16_t on[2] = { 'o', 'n' };
	static const char utf8_name[] = "UTF-8";
	static const char latin_name[] = "windows-1252";
	struct dom_element *option;
	struct vm_string *given;
	struct wb_units lines;
	size_t index;
	int checked;
	int charset;
	int error;

	/* Nothing is included until the kind says so. */
	*included = 0;
	wb_units_clear(value);

	/* The kinds, each with its own rule. */
	switch (kind) {
	case DOM_CONTROL_SUBMIT:
	case DOM_CONTROL_BUTTON:
	case DOM_CONTROL_RESET:
		/* Only the submitter among the buttons. */
		if (element != submitter || kind != DOM_CONTROL_SUBMIT)
			return 0;
		break;
	case DOM_CONTROL_CHECKBOX:
	case DOM_CONTROL_RADIO:
		/* Only a checked one, with its value or "on". */
		checked = dom_control_checked(element);
		if (!checked)
			return 0;
		given = dom_attribute_ascii(element, "value");
		if (given == NULL) {
			*included = 1;
			error = wb_units_append(value, on, 2);
			return error;
		}

		/* The value attribute. */
		break;
	case DOM_CONTROL_SELECT:
		/* The chosen option's value, when there is an option. */
		option = dom_select_chosen(element);
		if (option == NULL)
			return 0;
		*included = 1;
		error = form_option_value(option, value);
		return error;
	case DOM_CONTROL_HIDDEN:
		/* _charset_ is the encoding's name. */
		charset = form_equal_folded(name, "_charset_");
		if (charset) {
			*included = 1;
			if (encoding == FORM_UTF8)
				error = wb_utf8_to_units((const unsigned char *)utf8_name, sizeof(utf8_name) - 1U, value);
			else
				error = wb_utf8_to_units((const unsigned char *)latin_name, sizeof(latin_name) - 1U, value);
			return error;
		}

		/* Any other hidden control gives its value. */
		break;
	case DOM_CONTROL_TEXTAREA:
		/* A textarea's line breaks go as CR LF. */
		*included = 1;
		wb_units_init(&lines);
		error = dom_control_value(element, &lines);
		for (index = 0; index < lines.length && error == 0; index++) {
			if (lines.data[index] == 0x0aU)
				error = wb_units_append_code_point(value, 0x0dU);
			if (error == 0)
				error = wb_units_append(value, &lines.data[index], 1);
		}

		/* The value as written is no longer needed. */
		wb_units_release(&lines);
		return error;
	default:
		/* A field gives its value. */
		break;
	}

	/* The control's value. */
	*included = 1;
	error = dom_control_value(element, value);
	if (error != 0)
		return error;

	/* Succeeded: the entry's value. */
	return 0;
}

/* Appends one entry, name=value encoded, after an "&" when it is not the first. */
static int
form_add_entry(
	struct wb_buffer *query,
	const struct vm_string *name,
	const uint16_t *value,
	size_t length,
	int encoding)
{
	struct wb_units units;
	size_t index;
	uint16_t unit;
	int error;

	/* The separator. */
	error = 0;
	if (query->length != 0)
		error = wb_buffer_append_byte(query, '&');

	/* The name, as UTF-16. */
	wb_units_init(&units);
	for (index = 0; index < name->length && error == 0; index++) {
		unit = vm_string_at(name, index);
		error = wb_units_append(&units, &unit, 1);
	}

	/* name=value. */
	if (error == 0)
		error = form_encode(query, units.data, units.length, encoding);
	if (error == 0)
		error = wb_buffer_append_byte(query, '=');
	if (error == 0)
		error = form_encode(query, value, length, encoding);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the entry is appended. */
	return 0;
}

/*
 * Encodes text as application/x-www-form-urlencoded: into the encoding's
 * bytes (a character windows-1252 lacks as "&#N;"), then the bytes that
 * are not letters, digits or *-._ percent-encoded and spaces as "+".
 */
static int
form_encode(
	struct wb_buffer *query,
	const uint16_t *units,
	size_t length,
	int encoding)
{
	unsigned char bytes[8];
	char reference[16];
	uint32_t code_point;
	size_t position;
	size_t used;
	size_t count;
	size_t index;
	int error;

	/* Each character in turn. */
	position = 0;
	error = 0;
	while (position < length && error == 0) {
		used = wb_utf16_decode(units + position, length - position, &code_point);
		position += used;

		/* Its bytes: UTF-8, or a byte of windows-1252 (only its Latin-1 part), or a character reference. */
		if (encoding == FORM_UTF8) {
			count = wb_utf8_encode(code_point, bytes);
		} else if (code_point < 0x100U) {
			bytes[0] = (unsigned char)code_point;
			count = 1;
		} else {
			count = (size_t)snprintf(reference, sizeof(reference), "&#%lu;", (unsigned long)code_point);
			memcpy(bytes, reference, count);
		}

		/* Each byte, escaped as the form encoding asks. */
		for (index = 0; index < count && error == 0; index++)
			error = form_encode_byte(query, bytes[index]);
	}

	/* Reports a failed append. */
	if (error != 0)
		return error;

	/* Succeeded: the text is encoded. */
	return 0;
}

/* Appends one byte of an entry: kept, a space as "+", or percent-encoded. */
static int
form_encode_byte(
	struct wb_buffer *query,
	unsigned char byte)
{
	static const char hex[] = "0123456789ABCDEF";
	int kept;
	int error;

	/* Letters, digits and *-._ stay as they are. */
	kept = 0;
	if (byte >= 'a' && byte <= 'z')
		kept = 1;
	if (byte >= 'A' && byte <= 'Z')
		kept = 1;
	if (byte >= '0' && byte <= '9')
		kept = 1;
	if (byte == '*' || byte == '-' || byte == '.' || byte == '_')
		kept = 1;
	if (kept) {
		error = wb_buffer_append_byte(query, byte);
		return error;
	}

	/* A space is a plus sign. */
	if (byte == ' ') {
		error = wb_buffer_append_byte(query, '+');
		return error;
	}

	/* Anything else is %XX. */
	error = wb_buffer_append_byte(query, '%');
	if (error == 0)
		error = wb_buffer_append_byte(query, (unsigned char)hex[byte >> 4]);
	if (error == 0)
		error = wb_buffer_append_byte(query, (unsigned char)hex[byte & 0x0fU]);
	if (error != 0)
		return error;

	/* Succeeded: the byte is escaped. */
	return 0;
}

/*
 * Picks the encoding a form is submitted in: the first label of its
 * accept-charset this pass knows, else the document's <meta charset> (or
 * http-equiv Content-Type) when it names one, else UTF-8.
 */
static int
form_encoding(
	struct page *page,
	const struct dom_element *form)
{
	const struct dom_node *node;
	const struct dom_element *meta;
	struct vm_string *label;
	struct vm_string *equiv;
	size_t start;
	size_t end;
	size_t index;
	uint16_t unit;
	int encoding;
	int known;
	int is_meta;

	/* accept-charset: its labels, separated by spaces. */
	label = dom_attribute_ascii(form, "accept-charset");
	start = 0;
	while (label != NULL && start < label->length) {
		end = start;
		while (end < label->length) {
			unit = vm_string_at(label, end);
			if (unit == ' ' || unit == '\t' || unit == ',')
				break;
			end++;
		}

		/* The first label known decides. */
		known = form_charset_named(label, start, end, &encoding);
		if (known)
			return encoding;
		start = end + 1U;
	}

	/* The document's <meta charset>, or the charset= of an http-equiv Content-Type. */
	for (node = page->document->node.first_child; node != NULL; node = form_next(node, &page->document->node)) {
		is_meta = dom_element_is(node, DOM_NS_HTML, DOM_TAG_META);
		if (!is_meta)
			continue;
		meta = (const struct dom_element *)node;
		label = dom_attribute_ascii(meta, "charset");
		if (label != NULL) {
			known = form_charset_named(label, 0, label->length, &encoding);
			if (known)
				return encoding;
			continue;
		}

		/* An http-equiv Content-Type: the text after the "=" of "charset=". */
		label = dom_attribute_ascii(meta, "content");
		equiv = dom_attribute_ascii(meta, "http-equiv");
		if (label == NULL || equiv == NULL)
			continue;
		for (index = 0; index < label->length; index++) {
			unit = vm_string_at(label, index);
			if (unit != '=')
				continue;
			known = form_charset_named(label, index + 1U, label->length, &encoding);
			if (known)
				return encoding;
		}
	}

	/* UTF-8 by default. */
	return FORM_UTF8;
}

/*
 * Tells whether the part [start, end) of a string names an encoding this
 * pass submits in: UTF-8, or one of the labels of windows-1252 (which
 * includes ISO-8859-1 and US-ASCII).
 */
static int
form_charset_named(
	const struct vm_string *label,
	size_t start,
	size_t end,
	int *encoding)
{
	static const char *const latin[] = {
		"windows-1252", "iso-8859-1", "iso8859-1", "latin1", "l1", "us-ascii", "ascii", "cp1252", NULL
	};
	char name[32];
	size_t length;
	size_t index;
	uint16_t unit;
	int differs;

	/* The label in lower case, up to a ";" or a quote (a long one is no label this pass knows). */
	length = 0;
	for (index = start; index < end; index++) {
		unit = vm_string_at(label, index);
		if (unit == ';' || unit == '"' || unit == '\'')
			break;
		if (length + 1U >= sizeof(name))
			return 0;
		if (unit > 0x7fU)
			return 0;
		if (unit >= 'A' && unit <= 'Z')
			unit = (uint16_t)(unit - 'A' + 'a');
		name[length] = (char)unit;
		length++;
	}

	/* The label ends there. */
	name[length] = '\0';

	/* UTF-8 under its two labels. */
	differs = strcmp(name, "utf-8");
	if (differs != 0)
		differs = strcmp(name, "utf8");
	if (differs == 0) {
		*encoding = FORM_UTF8;
		return 1;
	}

	/* The labels of windows-1252. */
	for (index = 0; latin[index] != NULL; index++) {
		differs = strcmp(name, latin[index]);
		if (differs == 0) {
			*encoding = FORM_WINDOWS_1252;
			return 1;
		}
	}

	/* A label this pass does not know. */
	return 0;
}

/* Writes a form's action as UTF-8 without its query and fragment (empty for the document's own location). */
static int
form_action(
	const struct dom_element *form,
	struct wb_buffer *href)
{
	struct vm_string *action;
	struct wb_units units;
	size_t start;
	size_t end;
	uint16_t unit;
	int error;

	/* No action is the document's own location. */
	action = dom_attribute_ascii(form, "action");
	if (action == NULL)
		return 0;

	/* The action's characters up to its query or fragment. */
	end = 0;
	while (end < action->length) {
		unit = vm_string_at(action, end);
		if (unit == '?' || unit == '#')
			break;
		end++;
	}

	/* Without the spaces at its start and its end. */
	start = 0;
	while (start < end) {
		unit = vm_string_at(action, start);
		if (unit != ' ')
			break;
		start++;
	}
	while (end > start) {
		unit = vm_string_at(action, end - 1U);
		if (unit != ' ')
			break;
		end--;
	}

	/* Those characters, as UTF-8. */
	wb_units_init(&units);
	error = 0;
	while (start < end && error == 0) {
		unit = vm_string_at(action, start);
		error = wb_units_append(&units, &unit, 1);
		start++;
	}

	/* The UTF-16 characters written out as UTF-8. */
	if (error == 0)
		error = wb_units_to_utf8(units.data, units.length, href);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the action is written. */
	return 0;
}

/* Writes an option's value: its value attribute, or its text with spaces collapsed. */
static int
form_option_value(
	const struct dom_element *option,
	struct wb_units *out)
{
	struct vm_string *value;
	int error;

	/* The value attribute, as the control's value reads it. */
	wb_units_clear(out);
	value = dom_attribute_ascii(option, "value");
	if (value != NULL) {
		error = dom_control_value((struct dom_element *)option, out);
		return error;
	}

	/* Otherwise the option's text. */
	error = dom_option_text(option, out);
	if (error != 0)
		return error;

	/* Succeeded: the option's text. */
	return 0;
}

/* Finds the node after another in tree order within a root (NULL at the end). */
static const struct dom_node *
form_next(
	const struct dom_node *node,
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

/* Tells whether a string is an ASCII word in any case (the word is in lower case). */
static int
form_equal_folded(
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
