/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies definite zero lengths and first empty versus positive native rectangle unions. */

#include "layout/layout.h"

#include <stdio.h>
#include <string.h>

/* Count independent native layout observations until the fixture returns. */
static unsigned checks;
/* Preserve every failed geometry observation while later cases still execute. */
static unsigned failures;

static void zero_check(int condition, const char *name);
static int zero_case(struct dom_document *document);

/*
 * Runs actual replaced-sizing and native rectangle APIs against genuine DOM nodes.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct dom_document *document;
	int status;
	int printed;

	/* Genuine DOM ownership supplies ordinary inclusive-ancestor relationships. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Native layout observations use ordinary API input without a test implementation. */
	status = zero_case(document);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;
	printed = printf("native image zero layout: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: definite zero lengths and first-empty rectangle rules were verified. */
	return 0;
}

/* Records one actual sizing or rectangle observation. */
static void
zero_check(
	int condition,
	const char *name)
{
	int printed;

	/* Preserve named failures while all independent native observations run. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Distinguishes definite zero, indefinite height, absent boxes and actual empty rectangles. */
static int
zero_case(
	struct dom_document *document)
{
	struct dom_element *root;
	struct dom_element *image;
	struct vm_string *name;
	struct layout_tree tree;
	struct layout_box container;
	struct layout_box boxes[3];
	struct layout_rect rect;
	unsigned index;
	int found;
	int status;

	/* Build real connected DOM nodes for the native layout's ancestry queries. */
	name = vm_atom_from_ascii(document->heap, "div");
	if (name == NULL)
		return 2;
	root = dom_element_create(document, DOM_NS_HTML, name, NULL);
	if (root == NULL)
		return 2;
	name = vm_atom_from_ascii(document->heap, "img");
	if (name == NULL)
		return 2;
	image = dom_element_create(document, DOM_NS_HTML, name, NULL);
	if (image == NULL)
		return 2;
	dom_append_child(&document->node, &root->node);
	dom_append_child(&root->node, &image->node);

	/* Content attributes exercise the ordinary fallback only for truly unresolved dimensions. */
	name = vm_atom_from_ascii(document->heap, "width");
	if (name == NULL)
		return 2;
	status = dom_element_set_attribute(image, name, vm_atom_from_ascii(document->heap, "9"));
	if (status != 0)
		return status;
	name = vm_atom_from_ascii(document->heap, "height");
	if (name == NULL)
		return 2;
	status = dom_element_set_attribute(image, name, vm_atom_from_ascii(document->heap, "8"));
	if (status != 0)
		return status;

	/* A replaced image's percentages resolve against definite zero dimensions. */
	memset(&boxes, 0, sizeof(boxes));
	css_initial_style(&boxes[0].style);
	boxes[0].node = &image->node;
	boxes[0].style.width.unit = CSS_UNIT_PERCENT;
	boxes[0].style.width.value = 50;
	boxes[0].style.height.unit = CSS_UNIT_PERCENT;
	boxes[0].style.height.value = 25;
	layout_replaced_size(&boxes[0], 0, 0, 1);
	zero_check(boxes[0].width == 0 && boxes[0].height == 0, "definite zero percentage lengths resolve to zero");

	/* A percentage offset remains valid even when its containing size is exactly zero. */
	boxes[0].style.width.offset = 5;
	boxes[0].style.height.offset = 7;
	layout_replaced_size(&boxes[0], 0, 0, 1);
	zero_check(boxes[0].width == 5 * LAYOUT_UNIT && boxes[0].height == 7 * LAYOUT_UNIT, "zero percentage bases preserve finite offsets");
	layout_replaced_size(&boxes[0], 0, 0, 0);
	zero_check(boxes[0].width == 5 * LAYOUT_UNIT && boxes[0].height == 8 * LAYOUT_UNIT, "indefinite height still refuses percentage resolution");
	layout_replaced_size(&boxes[0], -1, -1, 1);
	zero_check(boxes[0].width == 9 * LAYOUT_UNIT && boxes[0].height == 8 * LAYOUT_UNIT, "negative containing sentinel remains unresolved");

	/* A real DOM node without any native box has no geometry. */
	memset(&tree, 0, sizeof(tree));
	found = layout_node_bounds(&tree, &image->node, &rect);
	zero_check(found == 0, "absent box differs from a native empty rectangle");
	memset(&container, 0, sizeof(container));
	memset(&boxes, 0, sizeof(boxes));
	container.kind = LAYOUT_ANONYMOUS_BLOCK;
	container.node = &root->node;
	container.first_child = &boxes[0];
	tree.root = &container;

	/* Multiple fragments of one genuine image retain their actual native rectangle inputs. */
	for (index = 0; index < 3U; index++) {
		boxes[index].kind = LAYOUT_BLOCK;
		boxes[index].node = &image->node;
		boxes[index].parent = &container;
	}

	/* An empty-width first fragment retains its location and nonzero height. */
	boxes[0].x = 20 * LAYOUT_UNIT;
	boxes[0].y = 30 * LAYOUT_UNIT;
	boxes[0].height = 10 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);
	zero_check(found == 1 && rect.x == 20 * LAYOUT_UNIT && rect.y == 30 * LAYOUT_UNIT && rect.width == 0 && rect.height == 10 * LAYOUT_UNIT, "first empty-width rectangle retains actual coordinates and height");

	/* Later empty rectangles never enlarge an all-empty first-fragment result. */
	boxes[0].next = &boxes[1];
	boxes[1].x = 100 * LAYOUT_UNIT;
	boxes[1].y = 200 * LAYOUT_UNIT;
	boxes[1].width = 30 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);
	zero_check(found == 1 && rect.x == 20 * LAYOUT_UNIT && rect.width == 0 && rect.height == 10 * LAYOUT_UNIT, "all empty fragments retain only first actual rectangle");

	/* A first positive rectangle replaces provisional empty geometry without including it. */
	boxes[1].height = 40 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);
	zero_check(found == 1 && rect.x == 100 * LAYOUT_UNIT && rect.y == 200 * LAYOUT_UNIT && rect.width == 30 * LAYOUT_UNIT && rect.height == 40 * LAYOUT_UNIT, "positive rectangle replaces first empty fallback");

	/* Far-away later empty fragments cannot alter an already positive union. */
	boxes[1].next = &boxes[2];
	boxes[2].x = -100 * LAYOUT_UNIT;
	boxes[2].y = -200 * LAYOUT_UNIT;
	boxes[2].height = 10 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);
	zero_check(found == 1 && rect.x == 100 * LAYOUT_UNIT && rect.y == 200 * LAYOUT_UNIT && rect.width == 30 * LAYOUT_UNIT && rect.height == 40 * LAYOUT_UNIT, "later empty fragment is excluded from positive union");

	/* Nonempty fragments retain the original smallest enclosing rectangle behavior. */
	boxes[2].width = 10 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);
	zero_check(found == 1 && rect.x == -100 * LAYOUT_UNIT && rect.y == -200 * LAYOUT_UNIT && rect.width == 230 * LAYOUT_UNIT && rect.height == 440 * LAYOUT_UNIT, "positive fragments retain exact enclosing union");

	/* Succeeded: actual native sizing and first-empty rectangle observations ran. */
	return 0;
}
