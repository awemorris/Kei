/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Native submission events retain their submitter and guard one current form dispatch. */

#include "bind/internal.h"

#include <errno.h>

static int submit_construct(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int submit_read_init(struct vm_realm *realm, vm_value init, struct vm_cell **roots, int *bubbles, int *cancelable, int *composed, vm_value *submitter);
static int submit_getter(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int submit_release(struct vm_heap *heap, struct vm_cell **roots, unsigned count, int status);

/* The readonly getter observes native state rather than a script-visible property on the input. */
static const struct bind_attribute submit_attributes[] = {
	{ "submitter", submit_getter, NULL },
	{ NULL, NULL, NULL }
};

/* SubmitEvent inherits the ordinary Event graph and has its own native constructor and brand. */
const struct bind_interface bind_submit_event_interface = {
	"SubmitEvent", BIND_EVENT, 1, submit_construct, submit_attributes, NULL, NULL
};

/*
 * Dispatches the native event stage without pretending to supply host navigation.
 * Same-form reentrancy cannot create another submission-event attempt during callbacks.
 */
int
bind_submit_event(
	struct bind_window *window,
	struct dom_element *form,
	struct dom_element *submitter,
	int *canceled)
{
	struct bind_event *event;
	struct vm_cell *roots[3];
	vm_value value;
	vm_value target;
	unsigned index;
	int status;

	/* Continuation is suppressed until an actual event returns uncanceled. */
	*canceled = 1;
	if (form->firing_submission_events)
		return 0;
	if (form->node.document->view != NULL)
		window = form->node.document->view;

	/* The form and input must survive removal while the original dispatch remains in progress. */
	roots[0] = &form->node.cell;
	roots[1] = &submitter->node.cell;
	roots[2] = NULL;
	for (index = 0; index < 3U; index++) {
		status = vm_heap_add_root(window->realm->heap, &roots[index]);
		if (status != 0)
			return submit_release(window->realm->heap, roots, index, status);
	}

	/* Preparing an event has no script callbacks, so the guard starts only after preparation succeeds. */
	status = bind_event_prepare(window, &form->node, BIND_SUBMIT_EVENT, "submit", BIND_EVENT_BUBBLES | BIND_EVENT_CANCELABLE, &value, &target, &event);
	if (status != 0)
		return submit_release(window->realm->heap, roots, 3U, status);
	roots[2] = vm_value_as_cell(value);
	status = bind_wrap(window, &submitter->node, &event->submitter);
	if (status != 0)
		return submit_release(window->realm->heap, roots, 3U, status);

	/* Native state carries the actual submitter while the form rejects nested event attempts. */
	form->firing_submission_events = 1;
	status = bind_dispatch(window, target, value, canceled);
	form->firing_submission_events = 0;
	return submit_release(window->realm->heap, roots, 3U, status);
}

/* Converts a SubmitEvent's native dictionary before publishing its newly allocated object. */
static int
submit_construct(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_event *event;
	struct vm_string *type;
	struct vm_cell *roots[3];
	vm_value init;
	vm_value submitter;
	unsigned index;
	int object;
	int bubbles;
	int cancelable;
	int composed;
	int status;

	UNUSED_PARAMETER(receiver);

	/* A required DOMString precedes the optional nullable dictionary conversion. */
	*result = VM_VALUE_UNDEFINED;
	if (count == 0)
		return vm_throw_type_error(realm, "SubmitEvent requires a type.");
	init = js_argument(args, count, 1);
	roots[0] = NULL;
	roots[1] = NULL;
	roots[2] = NULL;
	object = vm_value_is_object(init);
	if (object)
		roots[1] = vm_value_as_cell(init);

	/* Converted strings and later submitter values stay rooted through arbitrary dictionary getters. */
	for (index = 0; index < 3U; index++) {
		status = vm_heap_add_root(realm->heap, &roots[index]);
		if (status != 0)
			return submit_release(realm->heap, roots, index, status);
	}

	/* Type conversion may itself invoke user code and throw before dictionary conversion begins. */
	status = bind_to_atom(realm, args[0], 0, &type);
	if (status != 0)
		return submit_release(realm->heap, roots, 3U, status);
	roots[0] = &type->cell;

	/* Undefined/null are empty dictionaries; other nonobjects cannot stand for EventInit. */
	if (!object &&
	    init != VM_VALUE_NULL &&
	    init != VM_VALUE_UNDEFINED) {
		status = vm_throw_type_error(realm, "SubmitEvent init must be an object or null.");
		return submit_release(realm->heap, roots, 3U, status);
	}

	/* Conversion is complete before an Event object or observable subclass state is published. */
	status = submit_read_init(realm, init, roots, &bubbles, &cancelable, &composed, &submitter);
	if (status != 0)
		return submit_release(realm->heap, roots, 3U, status);
	window = bind_window_of(realm);
	status = bind_event_create(window, BIND_SUBMIT_EVENT, type, result, &event);
	if (status != 0)
		return submit_release(realm->heap, roots, 3U, status);

	/* Constructor events are untrusted and retain all successfully converted native members. */
	event->bubbles = bubbles;
	event->cancelable = cancelable;
	event->composed = composed;
	event->submitter = submitter;
	return submit_release(realm->heap, roots, 3U, 0);
}

/* Reads inherited EventInit members before the nullable native HTMLElement submitter. */
static int
submit_read_init(
	struct vm_realm *realm,
	vm_value init,
	struct vm_cell **roots,
	int *bubbles,
	int *cancelable,
	int *composed,
	vm_value *submitter)
{
	struct dom_node *node;
	struct dom_element *element;
	vm_value value;
	int present;
	int status;

	/* WebIDL booleans do not invoke valueOf or toString on a member's returned object. */
	status = bind_get_option(realm, init, "bubbles", &present, &value);
	if (status != 0)
		return status;
	*bubbles = vm_to_boolean(value);
	status = bind_get_option(realm, init, "cancelable", &present, &value);
	if (status != 0)
		return status;
	*cancelable = vm_to_boolean(value);
	status = bind_get_option(realm, init, "composed", &present, &value);
	if (status != 0)
		return status;
	*composed = vm_to_boolean(value);

	/* An absent, undefined or null submitter initializes the nullable native reference to null. */
	*submitter = VM_VALUE_NULL;
	status = bind_get_option(realm, init, "submitter", &present, &value);
	if (status != 0)
		return status;
	if (!present || value == VM_VALUE_NULL)
		return 0;

	/* Prototype forgery and foreign namespace elements cannot satisfy the HTMLElement contract. */
	node = bind_node_of(value);
	if (node == NULL || node->type != DOM_ELEMENT)
		return vm_throw_type_error(realm, "SubmitEvent submitter must be an HTMLElement.");
	element = (struct dom_element *)node;
	if (element->ns != DOM_NS_HTML)
		return vm_throw_type_error(realm, "SubmitEvent submitter must be an HTMLElement.");

	/* The final converted reference remains rooted while the actual event graph is allocated. */
	*submitter = value;
	roots[2] = vm_value_as_cell(value);
	return 0;
}

/* Reads the submitter only from actual native SubmitEvent state, including cross-realm instances. */
static int
submit_getter(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* An ordinary Event or an object inheriting SubmitEvent.prototype has no subclass state. */
	event = bind_event_of(receiver);
	if (event == NULL)
		return vm_throw_type_error(realm, "SubmitEvent receiver required.");
	if (event->interface != BIND_SUBMIT_EVENT)
		return vm_throw_type_error(realm, "SubmitEvent receiver required.");

	/* The getter returns the retained initialization value without inspecting script properties. */
	*result = event->submitter;
	return 0;
}

/* Removes each slot registered by one constructor or one native submission attempt. */
static int
submit_release(
	struct vm_heap *heap,
	struct vm_cell **roots,
	unsigned count,
	int status)
{
	unsigned index;

	/* No constructor or event attempt may leave a root pointing into its expired native stack. */
	for (index = 0; index < count; index++)
		vm_heap_remove_root(heap, &roots[index]);

	/* The caller receives the exact conversion, dispatch or allocation outcome. */
	return status;
}
