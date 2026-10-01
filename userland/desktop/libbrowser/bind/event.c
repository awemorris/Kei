/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Events: EventTarget with its listeners, the Event interfaces (Event,
 * UIEvent, MouseEvent, CustomEvent; the keyboard's, the wheel's and the
 * focus's are in input.c), and dispatch along the tree.
 *
 * Dispatch follows the DOM standard's shape: the path from the target up
 * to the document and the window (not the window for a load event), the
 * capture phase down the path, the target, and the bubble phase up it for
 * an event that bubbles.  An event handler (onclick and the like,
 * handler.c) is one of its target's listeners, in the place it was first
 * set, and cancels the event when it returns false.  A microtask
 * checkpoint follows each listener when no script is running (an event the
 * page fires), and not inside a script's dispatchEvent.
 */

#include "bind/internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The most nodes an event's path holds (the parser caps nesting well below this). */
#define EVENT_PATH_MAX		4096U

/* The listener kinds a phase invokes. */
#define EVENT_INVOKE_CAPTURE	0x1U
#define EVENT_INVOKE_BUBBLE	0x2U

static void event_trace(struct vm_heap *heap, struct vm_cell *cell);
static void listeners_trace(struct vm_heap *heap, struct vm_cell *cell);
static void listeners_finalize(struct vm_heap *heap, struct vm_cell *cell);
static int event_this(struct vm_realm *realm, vm_value this_value, struct bind_event **event);
static int event_target_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_add_listener(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_remove_listener(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_dispatch_method(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_listener_options(struct vm_realm *realm, vm_value options, int *capture, int *once);
static void event_forget_listener(struct bind_listeners *listeners, size_t index);
static int event_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_construct_mouse(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_construct_custom(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_construct_message(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_type(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_target(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_current_target(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_phase(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_bubbles(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_cancelable(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_default_prevented(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_is_trusted(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_time_stamp(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_prevent_default(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_stop_propagation(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_stop_immediate(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_client_x(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_client_y(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_page_x(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_page_y(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_button(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_detail(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_message_origin(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_message_last_id(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_message_source(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_message_ports(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int event_build_path(struct bind_window *window, vm_value target, const struct bind_event *event, struct vm_object *path);
static int event_invoke(struct bind_window *window, vm_value current, vm_value event_value, int phase, unsigned kinds);
static int event_call_listener(struct bind_window *window, vm_value callback, vm_value current, vm_value event_value, vm_value *returned);
static int event_fire(struct bind_window *window, struct dom_node *target, int interface, const char *type, unsigned flags, const struct bind_mouse *mouse, int *canceled);

/* The state of an event, which refers to its type, targets and detail. */
static const struct vm_cell_type event_type_cell = { "event", event_trace, NULL };

/* The listeners of a target, which refer to their types and callbacks and own their array. */
static const struct vm_cell_type listeners_type = { "event-listeners", listeners_trace, listeners_finalize };

/*
 * The operations of EventTarget.  The table is constant for the life of
 * the program.
 */
static const struct bind_operation event_target_operations[] = {
	{ "addEventListener", 2, event_add_listener },
	{ "removeEventListener", 2, event_remove_listener },
	{ "dispatchEvent", 1, event_dispatch_method },
	{ NULL, 0, NULL }
};

/*
 * The EventTarget interface (new EventTarget() makes a target of its
 * own).
 */
const struct bind_interface bind_event_target_interface = {
	"EventTarget", BIND_NO_PARENT, 0, event_target_construct, NULL, event_target_operations, NULL
};

/*
 * The attributes of Event.  The table is constant for the life of the
 * program.
 */
static const struct bind_attribute event_attributes[] = {
	{ "type", event_type, NULL },
	{ "target", event_target, NULL },
	{ "srcElement", event_target, NULL },
	{ "currentTarget", event_current_target, NULL },
	{ "eventPhase", event_phase, NULL },
	{ "bubbles", event_bubbles, NULL },
	{ "cancelable", event_cancelable, NULL },
	{ "defaultPrevented", event_default_prevented, NULL },
	{ "isTrusted", event_is_trusted, NULL },
	{ "timeStamp", event_time_stamp, NULL },
	{ NULL, NULL, NULL }
};

/*
 * The operations of Event.  The table is constant for the life of the
 * program.
 */
static const struct bind_operation event_operations[] = {
	{ "preventDefault", 0, event_prevent_default },
	{ "stopPropagation", 0, event_stop_propagation },
	{ "stopImmediatePropagation", 0, event_stop_immediate },
	{ NULL, 0, NULL }
};

/*
 * The constants of Event: the phases.  The table is constant for the life
 * of the program.
 */
static const struct bind_constant event_constants[] = {
	{ "NONE", BIND_PHASE_NONE },
	{ "CAPTURING_PHASE", BIND_PHASE_CAPTURING },
	{ "AT_TARGET", BIND_PHASE_AT_TARGET },
	{ "BUBBLING_PHASE", BIND_PHASE_BUBBLING },
	{ NULL, 0 }
};

/*
 * The Event interface.
 */
const struct bind_interface bind_event_interface = {
	"Event", BIND_NO_PARENT, 1, event_construct, event_attributes, event_operations, event_constants
};

/*
 * The UIEvent interface (no members of its own in this pass).
 */
const struct bind_interface bind_ui_event_interface = {
	"UIEvent", BIND_EVENT, 1, NULL, NULL, NULL, NULL
};

/*
 * The attributes of MouseEvent.  The table is constant for the life of
 * the program.
 */
static const struct bind_attribute mouse_event_attributes[] = {
	{ "clientX", event_client_x, NULL },
	{ "clientY", event_client_y, NULL },
	{ "x", event_client_x, NULL },
	{ "y", event_client_y, NULL },
	{ "pageX", event_page_x, NULL },
	{ "pageY", event_page_y, NULL },
	{ "button", event_button, NULL },
	{ "shiftKey", bind_event_shift_key, NULL },
	{ "ctrlKey", bind_event_ctrl_key, NULL },
	{ "altKey", bind_event_alt_key, NULL },
	{ "metaKey", bind_event_meta_key, NULL },
	{ NULL, NULL, NULL }
};

/*
 * The MouseEvent interface.
 */
const struct bind_interface bind_mouse_event_interface = {
	"MouseEvent", BIND_UI_EVENT, 1, event_construct_mouse, mouse_event_attributes, NULL, NULL
};

/*
 * The attributes of CustomEvent.  The table is constant for the life of
 * the program.
 */
static const struct bind_attribute custom_event_attributes[] = {
	{ "detail", event_detail, NULL },
	{ NULL, NULL, NULL }
};

/*
 * The CustomEvent interface.
 */
const struct bind_interface bind_custom_event_interface = {
	"CustomEvent", BIND_EVENT, 1, event_construct_custom, custom_event_attributes, NULL, NULL
};

/*
 * The attributes of MessageEvent.  data shares the event state's generic
 * detail slot; the remaining values are kept separately because pages use
 * them to distinguish messages from different browsing contexts.
 */
static const struct bind_attribute message_event_attributes[] = {
	{ "data", event_detail, NULL },
	{ "origin", event_message_origin, NULL },
	{ "lastEventId", event_message_last_id, NULL },
	{ "source", event_message_source, NULL },
	{ "ports", event_message_ports, NULL },
	{ NULL, NULL, NULL }
};

/* The MessageEvent interface. */
const struct bind_interface bind_message_event_interface = {
	"MessageEvent", BIND_EVENT, 1, event_construct_message, message_event_attributes, NULL, NULL
};

/*
 * Reports the state of the event an Event object stands for, or NULL
 * when the value is not an Event.
 */
struct bind_event *
bind_event_of(
	vm_value value)
{
	struct vm_object *object;
	struct vm_cell *cell;
	int is_object;
	int is_cell;

	/* Only an object can be an event. */
	is_object = vm_value_is_object(value);
	if (!is_object)
		return NULL;

	/* A platform object whose cell is an event's state. */
	object = (struct vm_object *)vm_value_as_cell(value);
	if (object->kind != VM_KIND_PLATFORM)
		return NULL;
	is_cell = vm_value_is_cell(object->internal);
	if (!is_cell)
		return NULL;
	cell = vm_value_as_cell(object->internal);
	if (cell->type != &event_type_cell)
		return NULL;

	/* The state. */
	return (struct bind_event *)cell;
}

/*
 * Makes an event of an interface (BIND_EVENT or one derived from it) with
 * a type: its state, not dispatched, and its object.
 */
int
bind_event_create(
	struct bind_window *window,
	int interface,
	struct vm_string *type,
	vm_value *value,
	struct bind_event **event)
{
	struct bind_event *state;
	struct vm_object *object;

	/* The state, zeroed: no flags, no phase. */
	state = vm_heap_alloc(window->realm->heap, &event_type_cell, sizeof(*state));
	if (state == NULL)
		return ENOMEM;
	state->type = type;
	state->target = VM_VALUE_NULL;
	state->current_target = VM_VALUE_NULL;
	state->detail = VM_VALUE_NULL;
	state->source = VM_VALUE_NULL;
	state->ports = VM_VALUE_NULL;
	state->time_stamp = window->now;

	/* The object with the interface's prototype. */
	object = vm_object_create(window->realm->heap, window->prototypes[interface]);
	if (object == NULL)
		return ENOMEM;
	object->kind = VM_KIND_PLATFORM;
	object->internal = vm_value_cell(state);

	/* Succeeded: the event is made. */
	*value = vm_value_cell(object);
	*event = state;
	return 0;
}

/*
 * Dispatches an event at a target (a node's object, the window, or an
 * EventTarget of its own); *canceled says whether a listener canceled it.
 */
int
bind_dispatch(
	struct bind_window *window,
	vm_value target,
	vm_value event_value,
	int *canceled)
{
	struct bind_event *event;
	struct vm_object *path;
	vm_value current;
	uint32_t index;
	int status;

	/* The event is being dispatched at the target. */
	*canceled = 0;
	event = bind_event_of(event_value);
	event->dispatching = 1;
	event->target = target;
	if (event->target_override != VM_VALUE_EMPTY)
		event->target = event->target_override;
	event->stop = 0;
	event->stop_immediate = 0;

	/* The path from the target up. */
	status = bind_array_create(window->realm, &path);
	if (status != 0)
		return status;
	status = event_build_path(window, target, event, path);

	/* The capture phase, from the top down to the target's parent. */
	for (index = path->length; status == 0 && index > 1U; index--) {
		current = path->elements[index - 1U];
		status = event_invoke(window, current, event_value, BIND_PHASE_CAPTURING, EVENT_INVOKE_CAPTURE);
	}

	/* The target: its capture listeners, then its handler and the others. */
	if (status == 0)
		status = event_invoke(window, target, event_value, BIND_PHASE_AT_TARGET, EVENT_INVOKE_CAPTURE);
	if (status == 0)
		status = event_invoke(window, target, event_value, BIND_PHASE_AT_TARGET, EVENT_INVOKE_BUBBLE);

	/* The bubble phase, from the target's parent up, for an event that bubbles. */
	for (index = 1; status == 0 && event->bubbles && index < path->length; index++) {
		current = path->elements[index];
		status = event_invoke(window, current, event_value, BIND_PHASE_BUBBLING, EVENT_INVOKE_BUBBLE);
	}

	/* The dispatch is over: no phase, no current target. */
	event->dispatching = 0;
	event->phase = BIND_PHASE_NONE;
	event->current_target = VM_VALUE_NULL;
	event->stop = 0;
	event->stop_immediate = 0;
	if (status != 0)
		return status;

	/* Succeeded: whether it was canceled is reported. */
	*canceled = event->canceled;
	return 0;
}

/*
 * Finds the listeners of an event target, making the empty set when
 * create is set and there is none; *listeners is NULL for a target without
 * any.  Returns EINVAL for a value that is not an event target.
 */
int
bind_listeners_of(
	struct bind_window *window,
	vm_value target,
	int create,
	struct bind_listeners **listeners)
{
	struct dom_node *node;
	struct vm_object *object;
	struct vm_cell *cell;
	struct vm_cell **slot;
	vm_value global;
	int is_object;
	int is_cell;

	/* A node keeps its listeners, and so does the window. */
	*listeners = NULL;
	slot = NULL;
	node = bind_node_of(target);
	global = vm_value_cell(window->realm->global);
	if (node != NULL) {
		slot = &node->listeners;
	} else if (target == global) {
		slot = &window->listeners;
	}

	/* An EventTarget of its own is a platform object whose cell is the listeners. */
	is_object = vm_value_is_object(target);
	if (slot == NULL && is_object) {
		object = (struct vm_object *)vm_value_as_cell(target);
		is_cell = vm_value_is_cell(object->internal);
		if (object->kind == VM_KIND_PLATFORM && is_cell) {
			cell = vm_value_as_cell(object->internal);
			if (cell->type == &listeners_type) {
				*listeners = (struct bind_listeners *)cell;
				return 0;
			}
		}
	}

	/* Anything else is not an event target. */
	if (slot == NULL)
		return EINVAL;

	/* A target without listeners gets an empty set when one is wanted. */
	if (*slot == NULL && create) {
		*slot = vm_heap_alloc(window->realm->heap, &listeners_type, sizeof(struct bind_listeners));
		if (*slot == NULL)
			return ENOMEM;
	}

	/* Succeeded: the listeners (or NULL) are found. */
	*listeners = (struct bind_listeners *)*slot;
	return 0;
}

/*
 * Adds a listener to a target's listeners at a position (the count for
 * the end), the listeners from there on moving up one place.
 */
int
bind_listeners_add(
	struct bind_listeners *listeners,
	const struct bind_listener *listener,
	size_t position)
{
	struct bind_listener *items;
	size_t capacity;

	/* The array grows when it is full. */
	if (listeners->count == listeners->capacity) {
		capacity = listeners->capacity * 2U;
		if (capacity < 4U)
			capacity = 4U;
		items = realloc(listeners->items, capacity * sizeof(*items));
		if (items == NULL)
			return ENOMEM;
		listeners->items = items;
		listeners->capacity = capacity;
	}

	/* The listener at its position, the ones after it moved up one place. */
	if (position > listeners->count)
		position = listeners->count;
	memmove(&listeners->items[position + 1U], &listeners->items[position], (listeners->count - position) * sizeof(listeners->items[0]));
	listeners->items[position] = *listener;

	/* Succeeded: the target has one listener more. */
	listeners->count++;
	return 0;
}

/*
 * Finds a listener by its type, callback, capture flag and whether it is
 * an event handler; nonzero when found.
 */
int
bind_listeners_find(
	const struct bind_listeners *listeners,
	const struct vm_string *type,
	vm_value callback,
	int capture,
	int handler,
	size_t *index)
{
	const struct bind_listener *listener;
	size_t item;

	/* Compares each listener. */
	for (item = 0; item < listeners->count; item++) {
		listener = &listeners->items[item];
		if (listener->type != type)
			continue;
		if (listener->callback != callback)
			continue;
		if (listener->capture != capture)
			continue;
		if (listener->handler != handler)
			continue;
		*index = item;
		return 1;
	}

	/* No such listener. */
	return 0;
}

/*
 * Finds a target's event handler listener for a type; nonzero when
 * found.
 */
int
bind_listeners_find_handler(
	const struct bind_listeners *listeners,
	const struct vm_string *type,
	size_t *index)
{
	size_t item;

	/* The listener marked as the type's handler. */
	for (item = 0; item < listeners->count; item++) {
		if (!listeners->items[item].handler)
			continue;
		if (listeners->items[item].type != type)
			continue;
		*index = item;
		return 1;
	}

	/* The target has no handler for the type. */
	return 0;
}

/*
 * Fires a trusted event of a type at a node, or at the window for NULL
 * (the page's load events and the like).
 */
int
bind_fire_event(
	struct bind_window *window,
	struct dom_node *target,
	const char *type,
	unsigned flags,
	int *canceled)
{
	int status;

	/* A plain Event. */
	status = event_fire(window, target, BIND_EVENT, type, flags, NULL, canceled);
	if (status != 0)
		return status;

	/* Succeeded: the event is dispatched. */
	return 0;
}

/*
 * Fires a trusted mouse event (it bubbles and can be canceled) at a node.
 */
int
bind_fire_mouse_event(
	struct bind_window *window,
	struct dom_node *target,
	const char *type,
	const struct bind_mouse *mouse,
	int *canceled)
{
	int status;

	/* A MouseEvent at the pointer's place. */
	status = event_fire(window, target, BIND_MOUSE_EVENT, type, BIND_EVENT_BUBBLES | BIND_EVENT_CANCELABLE, mouse, canceled);
	if (status != 0)
		return status;

	/* Succeeded: the event is dispatched. */
	return 0;
}

/*
 * Makes an event of an interface from a constructor's type and init
 * (bubbles, cancelable), for the constructors of the Event interfaces.
 */
int
bind_event_construct(
	struct vm_realm *realm,
	int interface,
	const vm_value *args,
	unsigned count,
	vm_value *result,
	struct bind_event **event)
{
	struct bind_window *window;
	struct vm_string *type;
	vm_value value;
	int present;
	int status;

	/* The type is required. */
	window = bind_window_of(realm);
	if (count < 1U) {
		status = vm_throw_type_error(realm, "Failed to construct the event: 1 argument required, but only 0 present.");
		return status;
	}

	/* The type's atom. */
	status = bind_to_atom(realm, args[0], 0, &type);
	if (status != 0)
		return status;

	/* The event. */
	status = bind_event_create(window, interface, type, result, event);
	if (status != 0)
		return status;

	/* The init's flags. */
	status = bind_get_option(realm, js_argument(args, count, 1), "bubbles", &present, &value);
	if (status != 0)
		return status;
	(*event)->bubbles = vm_to_boolean(value);
	status = bind_get_option(realm, js_argument(args, count, 1), "cancelable", &present, &value);
	if (status != 0)
		return status;
	(*event)->cancelable = vm_to_boolean(value);

	/* Succeeded: the event is made. */
	return 0;
}

/*
 * Takes a MouseEvent's init (clientX, clientY, button and the modifiers)
 * into an event made by a constructor; the page coordinates copy the
 * client ones, as there is no scroll to add.
 */
int
bind_event_init_mouse(
	struct vm_realm *realm,
	vm_value init,
	struct bind_event *event)
{
	vm_value value;
	double number;
	int present;
	int status;

	/* The horizontal place. */
	status = bind_get_option(realm, init, "clientX", &present, &value);
	if (status != 0)
		return status;
	if (present) {
		status = vm_to_number(realm, value, &number);
		if (status != 0)
			return status;
		event->mouse.client_x = number;
		event->mouse.page_x = number;
	}

	/* The vertical place. */
	status = bind_get_option(realm, init, "clientY", &present, &value);
	if (status != 0)
		return status;
	if (present) {
		status = vm_to_number(realm, value, &number);
		if (status != 0)
			return status;
		event->mouse.client_y = number;
		event->mouse.page_y = number;
	}

	/* The button. */
	status = bind_get_option(realm, init, "button", &present, &value);
	if (status != 0)
		return status;
	if (present) {
		status = vm_to_number(realm, value, &number);
		if (status != 0)
			return status;
		event->mouse.button = (int)number;
	}

	/* The modifiers. */
	status = bind_event_init_modifiers(realm, init, event);
	if (status != 0)
		return status;

	/* Succeeded: the init is taken. */
	return 0;
}

/*
 * Makes a trusted event of an interface for the browser to fire at a
 * node, or at the window for NULL, with the flags asked for (bubbles,
 * cancelable, the document as the load's target): the event's object and
 * state, and the target's object, for the caller to fill in and pass to
 * bind_dispatch.
 */
int
bind_event_prepare(
	struct bind_window *window,
	struct dom_node *target,
	int interface,
	const char *type,
	unsigned flags,
	vm_value *event_value,
	vm_value *target_value,
	struct bind_event **event)
{
	struct vm_string *name;
	int status;

	/* The type's atom and the event. */
	name = vm_atom_from_ascii(window->realm->heap, type);
	if (name == NULL)
		return ENOMEM;
	status = bind_event_create(window, interface, name, event_value, event);
	if (status != 0)
		return status;

	/*
	 * The browser fires it: trusted, with the flags asked for.  bubbles and
	 * cancelable are what listeners see and what preventDefault honors.
	 */
	(*event)->trusted = 1;
	if ((flags & BIND_EVENT_BUBBLES) != 0)
		(*event)->bubbles = 1;
	if ((flags & BIND_EVENT_CANCELABLE) != 0)
		(*event)->cancelable = 1;

	/* The target: the node's object, or the window. */
	*target_value = vm_value_cell(window->realm->global);
	if (target != NULL) {
		status = bind_wrap(window, target, target_value);
		if (status != 0)
			return status;
	}

	/* The window's load reports the document as its target. */
	if ((flags & BIND_EVENT_DOCUMENT) != 0) {
		status = bind_wrap(window, &window->document->node, &(*event)->target_override);
		if (status != 0)
			return status;
	}

	/* Succeeded: the event is ready to dispatch. */
	return 0;
}

/* Marks what an event's state refers to. */
static void
event_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct bind_event *event;

	/* The type, the targets and the values carried by an event. */
	event = (struct bind_event *)cell;
	if (event->type != NULL)
		vm_heap_mark(heap, &event->type->cell);
	vm_heap_mark_value(heap, event->target);
	vm_heap_mark_value(heap, event->current_target);
	vm_heap_mark_value(heap, event->detail);
	vm_heap_mark_value(heap, event->source);
	vm_heap_mark_value(heap, event->ports);
	vm_heap_mark_value(heap, event->target_override);
	if (event->origin != NULL)
		vm_heap_mark(heap, &event->origin->cell);
	if (event->last_event_id != NULL)
		vm_heap_mark(heap, &event->last_event_id->cell);

	/* A key event's key and code. */
	if (event->key != NULL)
		vm_heap_mark(heap, &event->key->cell);
	if (event->code != NULL)
		vm_heap_mark(heap, &event->code->cell);
}

/* Marks the types and callbacks of a target's listeners. */
static void
listeners_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct bind_listeners *listeners;
	size_t index;

	/* Each listener's type and callback. */
	listeners = (struct bind_listeners *)cell;
	for (index = 0; index < listeners->count; index++) {
		vm_heap_mark(heap, &listeners->items[index].type->cell);
		vm_heap_mark_value(heap, listeners->items[index].callback);
	}
}

/* Frees the array of a dead target's listeners. */
static void
listeners_finalize(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct bind_listeners *listeners;

	UNUSED_PARAMETER(heap);

	/* The array is the only thing outside the heap. */
	listeners = (struct bind_listeners *)cell;
	free(listeners->items);
}

/* Finds the event state a method's this value stands for, throwing a TypeError otherwise. */
static int
event_this(
	struct vm_realm *realm,
	vm_value this_value,
	struct bind_event **event)
{
	int status;

	/* The state, or the error of a method on the wrong object. */
	*event = bind_event_of(this_value);
	if (*event == NULL) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* Succeeded: the event is found. */
	return 0;
}

/* Makes an event target of its own, with an empty set of listeners (new EventTarget()). */
static int
event_target_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct vm_object *object;
	struct vm_cell *listeners;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The empty set of listeners. */
	window = bind_window_of(realm);
	listeners = vm_heap_alloc(realm->heap, &listeners_type, sizeof(struct bind_listeners));
	if (listeners == NULL)
		return ENOMEM;

	/* The object that holds them. */
	object = vm_object_create(realm->heap, window->prototypes[BIND_EVENT_TARGET]);
	if (object == NULL)
		return ENOMEM;
	object->kind = VM_KIND_PLATFORM;
	object->internal = vm_value_cell(listeners);

	/* Succeeded: the target is made. */
	*result = vm_value_cell(object);
	return 0;
}

/* Adds a listener for a type, unless the same one is there (addEventListener). */
static int
event_add_listener(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_listeners *listeners;
	struct bind_listener listener;
	struct vm_string *type;
	vm_value target;
	vm_value callback;
	size_t index;
	int capture;
	int once;
	int found;
	int status;

	/* The target (a function called plainly is the window's). */
	*result = VM_VALUE_UNDEFINED;
	window = bind_window_of(realm);
	target = this_value;
	if (target == VM_VALUE_UNDEFINED || target == VM_VALUE_NULL)
		target = vm_value_cell(realm->global);

	/* The type, the callback and the options. */
	status = bind_to_atom(realm, js_argument(args, count, 0), 0, &type);
	if (status != 0)
		return status;
	callback = js_argument(args, count, 1);
	status = event_listener_options(realm, js_argument(args, count, 2), &capture, &once);
	if (status != 0)
		return status;

	/* A null callback adds nothing. */
	if (callback == VM_VALUE_NULL || callback == VM_VALUE_UNDEFINED)
		return 0;

	/* The target's listeners. */
	status = bind_listeners_of(window, target, 1, &listeners);
	if (status == EINVAL) {
		status = bind_throw_illegal(realm);
		return status;
	} else if (status != 0) {
		return status;
	}

	/* The same listener is added once. */
	found = bind_listeners_find(listeners, type, callback, capture, 0, &index);
	if (found)
		return 0;

	/* The listener at the end. */
	listener.type = type;
	listener.callback = callback;
	listener.capture = capture;
	listener.once = once;
	listener.handler = 0;
	listener.generation = window->document->generation;
	status = bind_listeners_add(listeners, &listener, listeners->count);
	if (status != 0)
		return status;

	/* Succeeded: the listener is added. */
	return 0;
}

/* Removes a listener for a type (removeEventListener). */
static int
event_remove_listener(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_listeners *listeners;
	struct vm_string *type;
	vm_value target;
	size_t index;
	int capture;
	int once;
	int found;
	int status;

	/* The target (a function called plainly is the window's). */
	*result = VM_VALUE_UNDEFINED;
	window = bind_window_of(realm);
	target = this_value;
	if (target == VM_VALUE_UNDEFINED || target == VM_VALUE_NULL)
		target = vm_value_cell(realm->global);

	/* The type and the options. */
	status = bind_to_atom(realm, js_argument(args, count, 0), 0, &type);
	if (status != 0)
		return status;
	status = event_listener_options(realm, js_argument(args, count, 2), &capture, &once);
	if (status != 0)
		return status;

	/* The target's listeners. */
	status = bind_listeners_of(window, target, 0, &listeners);
	if (status == EINVAL) {
		status = bind_throw_illegal(realm);
		return status;
	} else if (status != 0) {
		return status;
	}

	/* The listener, if the target has it. */
	if (listeners == NULL)
		return 0;
	found = bind_listeners_find(listeners, type, js_argument(args, count, 1), capture, 0, &index);
	if (found)
		event_forget_listener(listeners, index);

	/* Succeeded: the listener is gone. */
	return 0;
}

/* Dispatches an event a script made, and reports whether no listener canceled it (dispatchEvent). */
static int
event_dispatch_method(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_listeners *listeners;
	struct bind_event *event;
	vm_value target;
	int canceled;
	int status;

	/* The target (a function called plainly is the window's). */
	window = bind_window_of(realm);
	target = this_value;
	if (target == VM_VALUE_UNDEFINED || target == VM_VALUE_NULL)
		target = vm_value_cell(realm->global);
	status = bind_listeners_of(window, target, 0, &listeners);
	if (status == EINVAL) {
		status = bind_throw_illegal(realm);
		return status;
	} else if (status != 0) {
		return status;
	}

	/* The event, which must not be in a dispatch already. */
	event = bind_event_of(js_argument(args, count, 0));
	if (event == NULL) {
		status = vm_throw_type_error(realm, "Failed to execute 'dispatchEvent': parameter 1 is not of type 'Event'.");
		return status;
	} else if (event->dispatching) {
		status = bind_throw_dom(realm, "InvalidStateError", "The event is already being dispatched.");
		return status;
	}

	/* A script's event is not trusted. */
	event->trusted = 0;
	status = bind_dispatch(window, target, js_argument(args, count, 0), &canceled);
	if (status != 0)
		return status;

	/* True unless a listener canceled it. */
	*result = VM_VALUE_TRUE;
	if (canceled)
		*result = VM_VALUE_FALSE;

	/* Succeeded: the event is dispatched. */
	return 0;
}

/* Reads addEventListener's third argument: a Boolean capture, or a dictionary with capture and once. */
static int
event_listener_options(
	struct vm_realm *realm,
	vm_value options,
	int *capture,
	int *once)
{
	vm_value value;
	int present;
	int is_object;
	int status;

	/* A value that is not an object is the capture flag. */
	*capture = 0;
	*once = 0;
	is_object = vm_value_is_object(options);
	if (!is_object) {
		*capture = vm_to_boolean(options);
		return 0;
	}

	/* The dictionary's members. */
	status = bind_get_option(realm, options, "capture", &present, &value);
	if (status != 0)
		return status;
	*capture = vm_to_boolean(value);
	status = bind_get_option(realm, options, "once", &present, &value);
	if (status != 0)
		return status;
	*once = vm_to_boolean(value);

	/* Succeeded: the options are read. */
	return 0;
}

/* Removes the listener at an index, keeping the others in order. */
static void
event_forget_listener(
	struct bind_listeners *listeners,
	size_t index)
{
	size_t after;

	/* The listeners after it move down one place. */
	after = listeners->count - index - 1U;
	memmove(&listeners->items[index], &listeners->items[index + 1U], after * sizeof(listeners->items[0]));
	listeners->count--;
}

/* Makes an Event (new Event(type, init)). */
static int
event_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The event with its type and init. */
	status = bind_event_construct(realm, BIND_EVENT, args, count, result, &event);
	if (status != 0)
		return status;

	/* Succeeded: the event is made. */
	return 0;
}

/* Makes a MouseEvent (new MouseEvent(type, init)) with the init's place, button and modifiers. */
static int
event_construct_mouse(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The event with its type and init. */
	status = bind_event_construct(realm, BIND_MOUSE_EVENT, args, count, result, &event);
	if (status != 0)
		return status;

	/* The pointer's part of the init. */
	status = bind_event_init_mouse(realm, js_argument(args, count, 1), event);
	if (status != 0)
		return status;

	/* Succeeded: the event is made. */
	return 0;
}

/* Makes a CustomEvent (new CustomEvent(type, init)) with the init's detail. */
static int
event_construct_custom(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	vm_value value;
	int present;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The event with its type and init. */
	status = bind_event_construct(realm, BIND_CUSTOM_EVENT, args, count, result, &event);
	if (status != 0)
		return status;

	/* The detail. */
	status = bind_get_option(realm, js_argument(args, count, 1), "detail", &present, &value);
	if (status != 0)
		return status;
	if (present)
		event->detail = value;

	/* Succeeded: the event is made. */
	return 0;
}

/* Makes a MessageEvent (new MessageEvent(type, init)) with its message fields. */
static int
event_construct_message(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	struct vm_string *text;
	vm_value value;
	int present;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The event with its type and common init. */
	status = bind_event_construct(realm, BIND_MESSAGE_EVENT, args, count, result, &event);
	if (status != 0)
		return status;

	/* Its cloned value (the constructor keeps the value itself). */
	status = bind_get_option(realm, js_argument(args, count, 1), "data", &present, &value);
	if (status != 0)
		return status;
	if (present)
		event->detail = value;

	/* The origin and the last event id are strings. */
	status = bind_get_option(realm, js_argument(args, count, 1), "origin", &present, &value);
	if (status != 0)
		return status;
	if (present) {
		status = bind_to_string(realm, value, &text);
		if (status != 0)
			return status;
		event->origin = text;
	}

	/* The second string follows the same conversion. */
	status = bind_get_option(realm, js_argument(args, count, 1), "lastEventId", &present, &value);
	if (status != 0)
		return status;
	if (present) {
		status = bind_to_string(realm, value, &text);
		if (status != 0)
			return status;
		event->last_event_id = text;
	}

	/* The sending context and transferred ports. */
	status = bind_get_option(realm, js_argument(args, count, 1), "source", &present, &value);
	if (status != 0)
		return status;
	if (present)
		event->source = value;
	status = bind_get_option(realm, js_argument(args, count, 1), "ports", &present, &value);
	if (status != 0)
		return status;
	if (present) {
		event->ports = value;
	} else {
		status = js_builtin_array(realm, NULL, 0, &event->ports);
		if (status != 0)
			return status;
	}

	/* Succeeded: the event is made. */
	return 0;
}

/* Reports the event's type (type). */
static int
event_type(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the type is reported. */
	*result = vm_value_cell(event->type);
	return 0;
}

/* Reports the event's target (target, srcElement). */
static int
event_target(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the target is reported. */
	*result = event->target;
	return 0;
}

/* Reports the target whose listeners run now (currentTarget). */
static int
event_current_target(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the current target is reported. */
	*result = event->current_target;
	return 0;
}

/* Reports the dispatch's phase (eventPhase). */
static int
event_phase(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the phase is reported. */
	*result = vm_value_int32(event->phase);
	return 0;
}

/* Reports whether the event bubbles (bubbles). */
static int
event_bubbles(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the flag is reported. */
	*result = vm_value_boolean(event->bubbles);
	return 0;
}

/* Reports whether the event can be canceled (cancelable). */
static int
event_cancelable(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the flag is reported. */
	*result = vm_value_boolean(event->cancelable);
	return 0;
}

/* Reports whether a listener canceled the event (defaultPrevented). */
static int
event_default_prevented(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the flag is reported. */
	*result = vm_value_boolean(event->canceled);
	return 0;
}

/* Reports whether the browser fired the event rather than a script (isTrusted). */
static int
event_is_trusted(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the flag is reported. */
	*result = vm_value_boolean(event->trusted);
	return 0;
}

/* Reports when the event was made, in the window's milliseconds (timeStamp). */
static int
event_time_stamp(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the time is reported. */
	*result = vm_value_number(event->time_stamp);
	return 0;
}

/* Cancels the event, when it can be canceled (preventDefault). */
static int
event_prevent_default(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	*result = VM_VALUE_UNDEFINED;
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* An event that can be canceled is. */
	if (event->cancelable)
		event->canceled = 1;

	/* Succeeded: the default action will not happen. */
	return 0;
}

/* Stops the dispatch after the current target's listeners (stopPropagation). */
static int
event_stop_propagation(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	*result = VM_VALUE_UNDEFINED;
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: no further target gets the event. */
	event->stop = 1;
	return 0;
}

/* Stops the dispatch after the current listener (stopImmediatePropagation). */
static int
event_stop_immediate(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	*result = VM_VALUE_UNDEFINED;
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: no further listener gets the event. */
	event->stop = 1;
	event->stop_immediate = 1;
	return 0;
}

/* Reports the pointer's horizontal place in the viewport (clientX, x). */
static int
event_client_x(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the place is reported. */
	*result = vm_value_number(event->mouse.client_x);
	return 0;
}

/* Reports the pointer's vertical place in the viewport (clientY, y). */
static int
event_client_y(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the place is reported. */
	*result = vm_value_number(event->mouse.client_y);
	return 0;
}

/* Reports the pointer's horizontal place in the document (pageX). */
static int
event_page_x(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the place is reported. */
	*result = vm_value_number(event->mouse.page_x);
	return 0;
}

/* Reports the pointer's vertical place in the document (pageY). */
static int
event_page_y(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the place is reported. */
	*result = vm_value_number(event->mouse.page_y);
	return 0;
}

/* Reports the button that changed (button: 0 is the main one). */
static int
event_button(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the button is reported. */
	*result = vm_value_int32(event->mouse.button);
	return 0;
}

/* Reports a custom event's detail (detail). */
static int
event_detail(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The event. */
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;

	/* Succeeded: the detail is reported. */
	*result = event->detail;
	return 0;
}

/* Reports the origin of a message (origin). */
static int
event_message_origin(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;
	if (event->origin == NULL)
		return bind_string(realm, "", result);
	*result = vm_value_cell(event->origin);
	return 0;
}

/* Reports a message's last event id (lastEventId). */
static int
event_message_last_id(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;
	if (event->last_event_id == NULL)
		return bind_string(realm, "", result);
	*result = vm_value_cell(event->last_event_id);
	return 0;
}

/* Reports the context that sent a message (source). */
static int
event_message_source(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;
	*result = event->source;
	return 0;
}

/* Reports the ports transferred with a message (ports). */
static int
event_message_ports(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_event *event;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	status = event_this(realm, this_value, &event);
	if (status != 0)
		return status;
	if (event->ports == VM_VALUE_NULL)
		return js_builtin_array(realm, NULL, 0, result);
	*result = event->ports;
	return 0;
}

/* Builds an event's path: the target, its ancestors, and the window above a document (not for load). */
static int
event_build_path(
	struct bind_window *window,
	vm_value target,
	const struct bind_event *event,
	struct vm_object *path)
{
	struct dom_node *node;
	struct dom_node *walk;
	uint32_t length;
	int is_load;
	int status;

	/* The target is the path's start. */
	status = vm_object_define(window->realm->heap, path, vm_value_int32(0), target, VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* A target that is not a node has no ancestors. */
	node = bind_node_of(target);
	if (node == NULL)
		return 0;

	/* The ancestors, nearest first. */
	length = 1;
	for (walk = node->parent; walk != NULL && length < EVENT_PATH_MAX; walk = walk->parent) {
		status = bind_array_push_node(window, path, walk);
		if (status != 0)
			return status;
		node = walk;
		length++;
	}

	/* Above the document is the window, except for a load event. */
	is_load = vm_string_equal_ascii(event->type, "load");
	if (node->type != DOM_DOCUMENT || is_load)
		return 0;
	status = vm_object_define(window->realm->heap, path, vm_value_int32((int32_t)path->length), vm_value_cell(window->realm->global), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* Succeeded: the path is built. */
	return 0;
}

/* Runs the listeners of one target of the path, of the kinds a phase invokes (handlers are of the bubble kind). */
static int
event_invoke(
	struct bind_window *window,
	vm_value current,
	vm_value event_value,
	int phase,
	unsigned kinds)
{
	struct bind_event *event;
	struct bind_listeners *listeners;
	struct bind_listener *listener;
	struct vm_object *snapshot;
	vm_value callback;
	vm_value returned;
	size_t index;
	size_t found;
	uint32_t item;
	int capture;
	int handler;
	int present;
	int status;

	/* A stopped event reaches no further target. */
	event = bind_event_of(event_value);
	if (event->stop)
		return 0;
	event->current_target = current;
	event->phase = phase;

	/* A handler attribute (onclick="...") becomes the target's handler before the listeners run. */
	if ((kinds & EVENT_INVOKE_BUBBLE) != 0) {
		status = bind_handler_prepare(window, current, event->type);
		if (status != 0)
			return status;
	}

	/* The listeners of the event's type and the phase's kind as they are now (ones added while they run wait). */
	status = bind_listeners_of(window, current, 0, &listeners);
	if (status == EINVAL || listeners == NULL)
		return 0;
	if (status != 0)
		return status;
	status = bind_array_create(window->realm, &snapshot);
	for (index = 0; status == 0 && index < listeners->count; index++) {
		listener = &listeners->items[index];
		if (listener->type != event->type)
			continue;
		if (listener->capture && (kinds & EVENT_INVOKE_CAPTURE) == 0)
			continue;
		if (!listener->capture && (kinds & EVENT_INVOKE_BUBBLE) == 0)
			continue;
		if (listener->callback == VM_VALUE_NULL)
			continue;

		/* Each is kept as its callback and whether it is a handler. */
		status = vm_object_define(window->realm->heap, snapshot, vm_value_int32((int32_t)snapshot->length), listener->callback, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = vm_object_define(window->realm->heap, snapshot, vm_value_int32((int32_t)snapshot->length), vm_value_boolean(listener->handler), VM_PROPERTY_DEFAULT);
	}

	/* The copy may have run out of memory. */
	if (status != 0)
		return status;

	/* Each one that is still there, in order, until a listener stops the event at once. */
	capture = 0;
	if ((kinds & EVENT_INVOKE_CAPTURE) != 0)
		capture = 1;
	for (item = 0; item + 1U < snapshot->length && !event->stop_immediate; item += 2U) {
		callback = snapshot->elements[item];
		handler = 0;
		if (snapshot->elements[item + 1U] == VM_VALUE_TRUE)
			handler = 1;

		/* A listener removed by an earlier one does not run; a once listener is removed before it runs. */
		status = bind_listeners_of(window, current, 0, &listeners);
		if (status != 0)
			return status;
		present = bind_listeners_find(listeners, event->type, callback, capture, handler, &found);
		if (!present)
			continue;
		if (listeners->items[found].once)
			event_forget_listener(listeners, found);

		/* The call; a handler that returns false cancels the event. */
		status = event_call_listener(window, callback, current, event_value, &returned);
		if (status != 0)
			return status;
		if (handler && returned == VM_VALUE_FALSE && event->cancelable)
			event->canceled = 1;
	}

	/* Succeeded: the target's listeners have run. */
	return 0;
}

/* Calls a listener (a function, or an object's handleEvent) with the event; an exception is reported. */
static int
event_call_listener(
	struct bind_window *window,
	vm_value callback,
	vm_value current,
	vm_value event_value,
	vm_value *returned)
{
	struct vm_realm *realm;
	vm_value method;
	vm_value key;
	vm_value this_value;
	int callable;
	int status;

	/* A function is called with the current target as this. */
	*returned = VM_VALUE_UNDEFINED;
	realm = window->realm;
	method = callback;
	this_value = current;
	callable = vm_value_is_callable(callback);

	/* An object's handleEvent is called with the object as this. */
	status = 0;
	if (!callable) {
		key = vm_key_from_ascii(realm->heap, "handleEvent");
		if (key == VM_VALUE_EMPTY)
			return ENOMEM;
		status = vm_get(realm, callback, key, &method);
		this_value = callback;
		if (status == 0) {
			callable = vm_value_is_callable(method);
			if (!callable)
				status = vm_throw_type_error(realm, "The listener's handleEvent is not a function.");
		}
	}

	/* The call. */
	if (status == 0)
		status = vm_call(realm, method, this_value, &event_value, 1, returned);

	/* An exception is reported, and the dispatch goes on. */
	if (status == VM_THROWN) {
		bind_report_exception(window, realm->exception);
		realm->exception = VM_VALUE_UNDEFINED;
		status = 0;
	}

	/* Running out of memory stops the dispatch. */
	if (status != 0)
		return status;

	/* The microtasks run after each listener when no script is running. */
	if (realm->depth == 0) {
		status = bind_checkpoint(window);
		if (status != 0)
			return status;
	}

	/* Succeeded: the listener has run. */
	return 0;
}

/* Makes and dispatches a trusted event of an interface at a node, or at the window for NULL. */
static int
event_fire(
	struct bind_window *window,
	struct dom_node *target,
	int interface,
	const char *type,
	unsigned flags,
	const struct bind_mouse *mouse,
	int *canceled)
{
	struct bind_event *event;
	vm_value event_value;
	vm_value target_value;
	int status;

	/* The event and its target. */
	*canceled = 0;
	status = bind_event_prepare(window, target, interface, type, flags, &event_value, &target_value, &event);
	if (status != 0)
		return status;

	/* The pointer, for a mouse event. */
	if (mouse != NULL)
		event->mouse = *mouse;

	/* The dispatch. */
	status = bind_dispatch(window, target_value, event_value, canceled);
	if (status != 0)
		return status;

	/* Succeeded: the event is dispatched. */
	return 0;
}
