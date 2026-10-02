/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * DOMImplementation associates creation with a Document without confusing its
 * platform brand with a Node. Script-created Documents keep traced prototype
 * snapshots rather than raw pointers to an explicitly retired Window.
 */

#include "bind/internal.h"

#include <errno.h>

/* One collectible implementation retains its associated Document and realm prototypes. */
struct implementation_state {
	struct vm_cell cell;
	struct dom_document *document;
	struct vm_object *prototypes;
};

static void implementation_trace(struct vm_heap *heap, struct vm_cell *cell);
static int implementation_this(struct vm_realm *realm, vm_value this_value, struct implementation_state **state);
static int implementation_snapshot(struct vm_realm *realm, struct dom_document *document, struct vm_object **snapshot);
static int implementation_document(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int implementation_doctype(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);

/* Implementation cells trace their associated graph until the last wrapper is dropped. */
static const struct vm_cell_type implementation_type = {
	"dom-implementation", implementation_trace, NULL
};

/* The immutable operation table describes only the implemented creation foundation. */
static const struct bind_operation implementation_operations[] = {
	{ "createDocument", 2, implementation_document },
	{ "createDocumentType", 3, implementation_doctype },
	{ NULL, 0, NULL }
};

/* The interface object has no public constructor and cannot impersonate a Node. */
const struct bind_interface bind_dom_implementation_interface = {
	"DOMImplementation", BIND_NO_PARENT, 0, NULL, NULL, implementation_operations, NULL
};

/*
 * Returns the actual Document's cached implementation with its own prototype graph.
 */
int
bind_document_implementation(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_document *document;
	struct implementation_state *state;
	struct vm_object *prototypes;
	struct vm_object *wrapper;
	vm_value prototype;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Branding precedes allocation and prevents borrowed non-Document access. */
	status = bind_this_node(realm, this_value, &node);
	if (status != 0)
		return status;

	/* A real Element or DocumentType still cannot stand for a Document. */
	if (node->type != DOM_DOCUMENT) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* SameObject caching preserves user expandos and repeated getter identity. */
	document = (struct dom_document *)node;
	if (document->implementation != NULL) {
		*result = vm_value_cell(document->implementation);
		return 0;
	}

	/* Builds a private prototype snapshot before publishing a cached wrapper. */
	status = implementation_snapshot(realm, document, &prototypes);
	if (status != 0)
		return status;

	/* Allocates and initializes the independent native brand. */
	state = vm_heap_alloc(document->heap, &implementation_type, sizeof(*state));
	if (state == NULL)
		return ENOMEM;
	state->document = document;
	state->prototypes = prototypes;

	/* Reads the receiver Document's own DOMImplementation prototype. */
	status = vm_object_get(prototypes, vm_value_int32(BIND_DOM_IMPLEMENTATION), &prototype);
	if (status != 0)
		return status;
	wrapper = vm_object_create(document->heap, (struct vm_object *)vm_value_as_cell(prototype));
	if (wrapper == NULL)
		return ENOMEM;

	/* Publishes the complete graph only after all allocations succeed. */
	wrapper->kind = VM_KIND_PLATFORM;
	wrapper->internal = vm_value_cell(state);
	document->implementation = wrapper;
	*result = vm_value_cell(wrapper);

	/* Succeeded: the Document's stable implementation object is available. */
	return 0;
}

/* Retains the associated Document and every prototype needed by its factories. */
static void
implementation_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct implementation_state *state;

	/* The brand's graph contains no manual Window pointer or permanent root. */
	state = (struct implementation_state *)cell;
	vm_heap_mark(heap, &state->document->node.cell);
	vm_heap_mark(heap, &state->prototypes->cell);
}

/* Requires the native implementation brand before converting caller arguments. */
static int
implementation_this(
	struct vm_realm *realm,
	vm_value this_value,
	struct implementation_state **state)
{
	struct vm_object *wrapper;
	struct vm_cell *cell;
	int valid;
	int status;

	/* Primitive values cannot carry the private platform brand. */
	valid = vm_value_is_object(this_value);
	if (!valid) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* Prototype inheritance alone supplies no native state. */
	wrapper = (struct vm_object *)vm_value_as_cell(this_value);
	valid = vm_value_is_cell(wrapper->internal);
	if (wrapper->kind != VM_KIND_PLATFORM || !valid) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* Node cells and other platform objects are distinct from implementations. */
	cell = vm_value_as_cell(wrapper->internal);
	if (cell->type != &implementation_type) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* Succeeded: the native implementation supplies its associated Document. */
	*state = (struct implementation_state *)cell;
	return 0;
}

/* Snapshots prototypes as private data properties, without mutable global lookups. */
static int
implementation_snapshot(
	struct vm_realm *realm,
	struct dom_document *document,
	struct vm_object **snapshot)
{
	struct bind_window *window;
	struct vm_object *made;
	unsigned index;
	int written;
	int status;

	/* A factory-created Document already has an immutable internal realm snapshot. */
	if (document->binding_prototypes != NULL) {
		*snapshot = document->binding_prototypes;
		return 0;
	}

	/* An ordinary Document uses its actual owning view, even through a borrowed getter. */
	window = document->view;
	if (window == NULL)
		window = bind_window_of(realm);
	made = vm_object_create(document->heap, NULL);
	if (made == NULL)
		return ENOMEM;

	/* Each independent data property retains an existing realm prototype. */
	for (index = 0; index < BIND_INTERFACES; index++) {
		status = vm_object_set(document->heap, made, vm_value_int32((int32_t)index), vm_value_cell(window->prototypes[index]), &written);
		if (status != 0)
			return status;

		/* A fresh private object must accept every prototype entry. */
		if (!written)
			return EINVAL;
	}

	/* Succeeded: no script can mutate the snapshot object through the public API. */
	*snapshot = made;
	return 0;
}

/* Creates a detached XML Document, adopting a supplied doctype only after validation. */
static int
implementation_document(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct implementation_state *state;
	struct dom_document *document;
	struct dom_element *element;
	struct dom_node *doctype;
	struct vm_string *uri;
	struct vm_string *qualified;
	vm_value given;
	int matches;
	int status;

	/* The receiver's native brand determines factory prototype identity. */
	status = implementation_this(realm, this_value, &state);
	if (status != 0)
		return status;

	/* Namespace and qualified name are required, even for an empty Document. */
	if (count < 2) {
		status = vm_throw_type_error(realm, "createDocument requires two arguments.");
		return status;
	}

	/* Converts a nullable namespace once, before the qualified name. */
	uri = NULL;
	given = args[0];
	if (given != VM_VALUE_NULL && given != VM_VALUE_UNDEFINED) {
		status = bind_to_string(realm, given, &uri);
		if (status != 0)
			return status;

		/* An empty namespace is normalized to no namespace. */
		if (uri->length == 0)
			uri = NULL;
	}

	/* LegacyNullToEmptyString preserves null's historical empty-root meaning. */
	given = args[1];
	if (given == VM_VALUE_NULL) {
		qualified = vm_atom_from_ascii(realm->heap, "");
		if (qualified == NULL)
			return ENOMEM;
	} else {
		status = bind_to_atom(realm, given, 0, &qualified);
		if (status != 0)
			return status;
	}

	/* Nullable DocumentType conversion rejects nodes of any other native kind. */
	doctype = NULL;
	given = js_argument(args, count, 2);
	if (given != VM_VALUE_NULL && given != VM_VALUE_UNDEFINED) {
		doctype = bind_node_of(given);
		if (doctype == NULL || doctype->type != DOM_DOCUMENT_TYPE) {
			status = vm_throw_type_error(realm, "The doctype is not a DocumentType.");
			return status;
		}
	}

	/* A detached Document has no view, but its prototypes retain its relevant realm. */
	document = dom_document_create(state->document->heap);
	if (document == NULL)
		return ENOMEM;
	document->binding_prototypes = state->prototypes;
	document->context = state->document->context;
	document->content = DOM_CONTENT_XML;

	/* Only the exact HTML/SVG namespace identities select their XML MIME types. */
	if (uri != NULL) {
		matches = vm_string_equal_ascii(uri, "http://www.w3.org/1999/xhtml");
		if (matches) {
			document->content = DOM_CONTENT_XHTML;
		} else {
			matches = vm_string_equal_ascii(uri, "http://www.w3.org/2000/svg");
			if (matches)
				document->content = DOM_CONTENT_SVG;
		}
	}

	/* Empty names leave the Document empty without running namespace extraction. */
	element = NULL;
	if (qualified->length != 0) {
		status = bind_create_element_ns(realm, document, uri, qualified, &element);
		if (status != 0)
			return status;
	}

	/* All fallible result allocation precedes detachment of an existing doctype. */
	status = bind_wrap(bind_window_of(realm), &document->node, result);
	if (status != 0)
		return status;

	/* Moving the only supplied doctype retains wrapper identity and updates ownership. */
	if (doctype != NULL) {
		dom_remove(doctype);
		doctype->document = document;
		dom_append_child(&document->node, doctype);
	}

	/* The already-created root follows the doctype in the new document tree. */
	if (element != NULL)
		dom_append_child(&document->node, &element->node);

	/* Succeeded: the detached XML Document is fully assembled. */
	return 0;
}

/* Creates a branded DocumentType with once-only ordered DOMString conversions. */
static int
implementation_doctype(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct implementation_state *state;
	struct dom_node *doctype;
	struct vm_string *name;
	struct vm_string *public_id;
	struct vm_string *system_id;
	size_t index;
	uint16_t unit;
	int status;

	/* Native branding takes precedence over argument conversion. */
	status = implementation_this(realm, this_value, &state);
	if (status != 0)
		return status;

	/* All three IDL strings are required. */
	if (count < 3) {
		status = vm_throw_type_error(realm, "createDocumentType requires three arguments.");
		return status;
	}

	/* Name, public identifier and system identifier convert in argument order. */
	status = bind_to_string(realm, args[0], &name);
	if (status != 0)
		return status;
	status = bind_to_string(realm, args[1], &public_id);
	if (status != 0)
		return status;
	status = bind_to_string(realm, args[2], &system_id);
	if (status != 0)
		return status;

	/* Current DOM doctype syntax allows empty names but forbids these code units. */
	for (index = 0; index < name->length; index++) {
		unit = vm_string_at(name, index);
		if (unit == 0 ||
		    unit == '\t' ||
		    unit == '\n' ||
		    unit == '\f' ||
		    unit == '\r' ||
		    unit == ' ' ||
		    unit == '>') {
			status = bind_throw_dom(realm, "InvalidCharacterError", "The doctype name is not valid.");
			return status;
		}
	}

	/* Native node allocation retains the associated Document and all converted strings. */
	doctype = dom_doctype_create(state->document, name, public_id, system_id);
	if (doctype == NULL)
		return ENOMEM;

	/* Publishes the receiver Document's ordinary DocumentType wrapper. */
	status = bind_wrap(bind_window_of(realm), doctype, result);
	if (status != 0)
		return status;

	/* Succeeded: the new detached DocumentType is available. */
	return 0;
}
