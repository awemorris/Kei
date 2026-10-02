/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Native XML character nodes own exact data and target identity before any public binding exposure. */

#include "dom/dom.h"

#include <errno.h>

static int xml_character_create(struct dom_document *document, int type, struct vm_string *target, const uint16_t *units, size_t length, struct dom_node **created);

/*
 * Makes a native CDATA node from already validated caller-owned C data.
 */
int
dom_cdata_create(
	struct dom_document *document,
	const uint16_t *units,
	size_t length,
	struct dom_node **created)
{
	int status;

	/* The native factory preserves CDATA identity rather than converting it to ordinary Text. */
	status = xml_character_create(document, DOM_CDATA_SECTION, NULL, units, length, created);

	/* Succeeded or failed: the shared helper owns every temporary collector registration. */
	return status;
}

/*
 * Makes a native processing instruction with an actual traced target and copied C data.
 */
int
dom_pi_create(
	struct dom_document *document,
	struct vm_string *target,
	const uint16_t *units,
	size_t length,
	struct dom_node **created)
{
	int status;

	/* Native target validation belongs to the parser or later public factory caller. */
	if (created == NULL)
		return EINVAL;
	*created = NULL;
	if (target == NULL)
		return EINVAL;
	status = xml_character_create(document, DOM_PROCESSING_INSTRUCTION, target, units, length, created);

	/* Succeeded or failed: target ownership never depends on an unrooted caller local. */
	return status;
}

/*
 * Identifies the actual native character-data kinds without confusing them with child containers.
 */
int
dom_is_character_data(
	const struct dom_node *node)
{
	/* Missing nodes have no character buffer. */
	if (node == NULL)
		return 0;
	if (node->type == DOM_TEXT ||
	    node->type == DOM_CDATA_SECTION ||
	    node->type == DOM_PROCESSING_INSTRUCTION ||
	    node->type == DOM_COMMENT)
		return 1;

	/* Rejected: all other native kinds expose children rather than character storage. */
	return 0;
}

/*
 * Identifies Text and its CDATA subtype independently of other CharacterData nodes.
 */
int
dom_is_text(
	const struct dom_node *node)
{
	/* Only these two native kinds contribute text content. */
	if (node == NULL)
		return 0;
	if (node->type == DOM_TEXT || node->type == DOM_CDATA_SECTION)
		return 1;

	/* Rejected: comments and processing instructions are not Text. */
	return 0;
}

/* Completes native character identity while actual Document and target survive allocation collection. */
static int
xml_character_create(
	struct dom_document *document,
	int type,
	struct vm_string *target,
	const uint16_t *units,
	size_t length,
	struct dom_node **created)
{
	struct dom_node *node;
	struct dom_character_data *data;
	struct vm_heap *heap;
	struct vm_cell *document_root;
	struct vm_cell *target_root;
	int status;

	/* Invalid native inputs cannot publish a partial node or overflow the copied UTF16 buffer. */
	if (created == NULL)
		return EINVAL;
	*created = NULL;
	if (document == NULL)
		return EINVAL;
	if (length != 0 && units == NULL)
		return EINVAL;
	if (length > (size_t)-1 / sizeof(*units) - 1U)
		return EOVERFLOW;

	/* The source is ordinary caller-owned C storage; only Document and target are VM edges here. */
	heap = document->heap;
	document_root = &document->node.cell;
	target_root = NULL;
	if (target != NULL)
		target_root = &target->cell;
	status = vm_heap_add_root(heap, &document_root);
	if (status != 0)
		return status;
	status = vm_heap_add_root(heap, &target_root);
	if (status != 0) {
		vm_heap_remove_root(heap, &document_root);
		return status;
	}

	/* Base character storage allocation may collect; complete target/type initialization cannot allocate again. */
	node = dom_text_create(document, units, length);
	if (node == NULL) {
		vm_heap_remove_root(heap, &target_root);
		vm_heap_remove_root(heap, &document_root);
		return ENOMEM;
	}

	/* The collector cell already uses the actual character-data tracer and C buffer finalizer. */
	data = (struct dom_character_data *)node;
	data->target = target;
	node->type = (uint16_t)type;
	*created = node;
	vm_heap_remove_root(heap, &target_root);
	vm_heap_remove_root(heap, &document_root);

	/* Succeeded: the caller receives a complete actual owner graph with no intervening allocation. */
	return 0;
}
