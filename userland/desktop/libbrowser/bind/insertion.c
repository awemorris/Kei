/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Validates ordinary DOM insertion without moving nodes or notifying hosts. */

#include "bind/internal.h"

#include <errno.h>

static int insertion_document(struct vm_realm *realm, const struct dom_node *parent, const struct dom_node *node, const struct dom_node *reference);

/*
 * Checks whether an ordinary pre-insertion is permitted without changing its participants.
 *
 * The caller retains the nodes through possible DOM-exception allocation.
 * Existing Document children are not excluded, even when node is already a child.
 */
int
bind_validate_insert(
	struct vm_realm *realm,
	struct dom_node *parent,
	struct dom_node *node,
	struct dom_node *reference)
{
	int ancestor;
	int status;

	/* Invalid embedding inputs cannot be used to construct a script exception. */
	if (realm == NULL ||
	    parent == NULL ||
	    node == NULL)
		return EINVAL;

	/* Only the three container types can receive an ordinary child. */
	if (parent->type != DOM_DOCUMENT &&
	    parent->type != DOM_DOCUMENT_FRAGMENT &&
	    parent->type != DOM_ELEMENT) {
		status = bind_throw_dom(realm, "HierarchyRequestError", "The parent cannot have children.");
		return status;
	}

	/* Cycles are refused before testing whether the reference belongs to the parent. */
	ancestor = dom_is_inclusive_ancestor(node, parent);
	if (ancestor) {
		status = bind_throw_dom(realm, "HierarchyRequestError", "The new child contains the parent.");
		return status;
	}

	/* A foreign reference cannot identify an insertion position. */
	if (reference != NULL && reference->parent != parent) {
		status = bind_throw_dom(realm, "NotFoundError", "The insertion reference is not a child of the parent.");
		return status;
	}

	/* Only supported ordinary child types may enter a tree. */
	if (node->type != DOM_DOCUMENT_FRAGMENT &&
	    node->type != DOM_DOCUMENT_TYPE &&
	    node->type != DOM_ELEMENT &&
	    node->type != DOM_TEXT &&
	    node->type != DOM_CDATA_SECTION &&
	    node->type != DOM_PROCESSING_INSTRUCTION &&
	    node->type != DOM_COMMENT) {
		status = bind_throw_dom(realm, "HierarchyRequestError", "Nodes of this type cannot be inserted here.");
		return status;
	}

	/* A doctype belongs directly to a Document, never to an element or fragment. */
	if (node->type == DOM_DOCUMENT_TYPE && parent->type != DOM_DOCUMENT) {
		status = bind_throw_dom(realm, "HierarchyRequestError", "A doctype requires a Document parent.");
		return status;
	}

	/* Document child order and counts require validation of the whole incoming fragment. */
	if (parent->type == DOM_DOCUMENT) {
		status = insertion_document(realm, parent, node, reference);
		if (status != 0)
			return status;
	}

	/* Succeeded: the insertion is valid, and every node remains in its original position. */
	return 0;
}

/* Checks actual Document child types and order with an empty exclusion list. */
static int
insertion_document(
	struct vm_realm *realm,
	const struct dom_node *parent,
	const struct dom_node *node,
	const struct dom_node *reference)
{
	const struct dom_node *child;
	unsigned elements;
	int before_reference;
	int status;

	/* Even an empty Text node is forbidden directly beneath a Document. */
	if (node->type == DOM_TEXT || node->type == DOM_CDATA_SECTION) {
		status = bind_throw_dom(realm, "HierarchyRequestError", "A Document cannot contain Text children.");
		return status;
	}

	/* Comments have no effect on the Document's unique element and doctype constraints. */
	if (node->type == DOM_COMMENT || node->type == DOM_PROCESSING_INSTRUCTION)
		return 0;

	/* Refuse a complete fragment before its first child can be adopted or spliced. */
	if (node->type == DOM_DOCUMENT_FRAGMENT) {
		elements = 0;
		for (child = node->first_child; child != NULL; child = child->next) {
			/* Text is invalid regardless of its length or position among fragment children. */
			if (child->type == DOM_TEXT || child->type == DOM_CDATA_SECTION) {
				status = bind_throw_dom(realm, "HierarchyRequestError", "A Document fragment cannot insert Text here.");
				return status;
			}

			/* Stop at the second element so the bounded counter cannot overflow. */
			if (child->type == DOM_ELEMENT) {
				elements++;
				if (elements > 1U) {
					status = bind_throw_dom(realm, "HierarchyRequestError", "Only one Document element is allowed.");
					return status;
				}
			}
		}

		/* An empty or comment-only fragment introduces no constrained child. */
		if (elements == 0)
			return 0;
	}

	/* An element must be unique and must not precede an existing doctype. */
	if (node->type == DOM_ELEMENT || node->type == DOM_DOCUMENT_FRAGMENT) {
		before_reference = 1;
		for (child = parent->first_child; child != NULL; child = child->next) {
			/* The referenced child and all following children would follow the new element. */
			if (child == reference)
				before_reference = 0;

			/* No existing Document element is excluded from ordinary pre-insertion. */
			if (child->type == DOM_ELEMENT) {
				status = bind_throw_dom(realm, "HierarchyRequestError", "Only one Document element is allowed.");
				return status;
			}

			/* The existing doctype must remain before the new element. */
			if (!before_reference && child->type == DOM_DOCUMENT_TYPE) {
				status = bind_throw_dom(realm, "HierarchyRequestError", "The Document element must follow its doctype.");
				return status;
			}
		}

		/* Succeeded: this element or one-element fragment preserves Document structure. */
		return 0;
	}

	/* A doctype must be unique and must not follow an existing element. */
	before_reference = 1;
	for (child = parent->first_child; child != NULL; child = child->next) {
		/* Appending keeps every existing child before the insertion position. */
		if (child == reference)
			before_reference = 0;

		/* Ordinary insertion cannot exclude the existing doctype, including node itself. */
		if (child->type == DOM_DOCUMENT_TYPE) {
			status = bind_throw_dom(realm, "HierarchyRequestError", "Only one Document doctype is allowed.");
			return status;
		}

		/* An element preceding the reference would precede the new doctype. */
		if (before_reference && child->type == DOM_ELEMENT) {
			status = bind_throw_dom(realm, "HierarchyRequestError", "The Document doctype must precede its element.");
			return status;
		}
	}

	/* Succeeded: the doctype remains unique and before the Document element. */
	return 0;
}
