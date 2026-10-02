/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Verifies definite zero lengths and first empty versus positive native rectangle unions.
 */

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
	int error;
	int printed;

	/* Genuine DOM ownership supplies ordinary inclusive-ancestor relationships. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;

	/* Keeps construction cells visible until the fixture heap is released. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));

	/* The genuine Document owns all elements used by native ancestry queries. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Native layout observations use ordinary API input without a test implementation. */
	error = zero_case(document);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Reclaims all native fixture nodes after their layout observations finish. */
	vm_heap_destroy(heap);

	/* Reports the completed observations separately from construction failure. */
	printed = printf("native image zero layout: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any failed geometry observation rejects this regression. */
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

	/* Counts this independent sizing or rectangle observation in the final total. */
	checks++;

	/* Retains each named failure while later observations and cleanup continue. */
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: this observation and any lost failure report were counted. */
	return;
}

/* Distinguishes definite zero, indefinite height, absent boxes and actual empty rectangles. */
static int
zero_case(
	struct dom_document *document)
{
	struct dom_element *root;
	struct dom_element *image;
	struct vm_string *name;
	struct vm_string *value;
	struct layout_tree tree;
	struct layout_box container;
	struct layout_box boxes[3];
	struct layout_rect rect;
	unsigned index;
	int found;
	int error;
	int observed;

	/* Build real connected DOM nodes for the native layout's ancestry queries. */
	name = vm_atom_from_ascii(document->heap, "div");
	if (name == NULL)
		return 2;
	root = dom_element_create(document, DOM_NS_HTML, name, NULL);
	if (root == NULL)
		return 2;

	/* Builds the actual image node separately from its native parent. */
	name = vm_atom_from_ascii(document->heap, "img");
	if (name == NULL)
		return 2;
	image = dom_element_create(document, DOM_NS_HTML, name, NULL);
	if (image == NULL)
		return 2;

	/* Connects the genuine nodes before native inclusive-ancestor lookups. */
	dom_append_child(&document->node, &root->node);
	dom_append_child(&root->node, &image->node);

	/* Content attributes exercise the ordinary fallback only for truly unresolved dimensions. */
	name = vm_atom_from_ascii(document->heap, "width");
	if (name == NULL)
		return 2;

	/* Refuses fixture construction if the genuine width value cannot be allocated. */
	value = vm_atom_from_ascii(document->heap, "9");
	if (value == NULL)
		return 2;

	/* Installs the owned width atom only after both native arguments exist. */
	error = dom_element_set_attribute(image, name, value);
	if (error != 0)
		return error;

	/* The height attribute supplies the unresolved percentage fallback independently. */
	name = vm_atom_from_ascii(document->heap, "height");
	if (name == NULL)
		return 2;

	/* Refuses fixture construction if the genuine height value cannot be allocated. */
	value = vm_atom_from_ascii(document->heap, "8");
	if (value == NULL)
		return 2;

	/* Installs the owned height atom only after both native arguments exist. */
	error = dom_element_set_attribute(image, name, value);
	if (error != 0)
		return error;

	/* A replaced image's percentages resolve against definite zero dimensions. */
	memset(&boxes, 0, sizeof(boxes));
	css_initial_style(&boxes[0].style);
	boxes[0].node = &image->node;
	boxes[0].style.width.unit = CSS_UNIT_PERCENT;
	boxes[0].style.width.value = 50;
	boxes[0].style.height.unit = CSS_UNIT_PERCENT;
	boxes[0].style.height.value = 25;
	layout_replaced_size(&boxes[0], 0, 0, 1);

	/* Definite zero percentage lengths resolve to zero. */
	observed = 0;
	if (boxes[0].width == 0 && boxes[0].height == 0)
		observed = 1;
	zero_check(observed, "definite zero percentage lengths resolve to zero");

	/* A percentage offset remains valid even when its containing size is exactly zero. */
	boxes[0].style.width.offset = 5;
	boxes[0].style.height.offset = 7;
	layout_replaced_size(&boxes[0], 0, 0, 1);

	/* Zero percentage bases preserve finite offsets. */
	observed = 0;
	if (boxes[0].width == 5 * LAYOUT_UNIT && boxes[0].height == 7 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "zero percentage bases preserve finite offsets");

	/* An indefinite height must use its real content-attribute fallback. */
	layout_replaced_size(&boxes[0], 0, 0, 0);

	/* Indefinite height still refuses percentage resolution. */
	observed = 0;
	if (boxes[0].width == 5 * LAYOUT_UNIT && boxes[0].height == 8 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "indefinite height still refuses percentage resolution");

	/* Negative containing-size sentinels leave both dimensions unresolved. */
	layout_replaced_size(&boxes[0], -1, -1, 1);

	/* Negative containing sentinel remains unresolved. */
	observed = 0;
	if (boxes[0].width == 9 * LAYOUT_UNIT && boxes[0].height == 8 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "negative containing sentinel remains unresolved");

	/* A real DOM node without any native box has no geometry. */
	memset(&tree, 0, sizeof(tree));
	found = layout_node_bounds(&tree, &image->node, &rect);

	/* Absent box differs from a native empty rectangle. */
	observed = 0;
	if (found == 0)
		observed = 1;
	zero_check(observed, "absent box differs from a native empty rectangle");

	/* The native rectangle algorithm receives stack-owned boxes over genuine DOM nodes. */
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

	/* First empty-width rectangle retains actual coordinates and height. */
	observed = 0;
	if (found == 1 &&
	    rect.x == 20 * LAYOUT_UNIT &&
	    rect.y == 30 * LAYOUT_UNIT &&
	    rect.width == 0 &&
	    rect.height == 10 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "first empty-width rectangle retains actual coordinates and height");

	/* Later empty rectangles never enlarge an all-empty first-fragment result. */
	boxes[0].next = &boxes[1];
	boxes[1].x = 100 * LAYOUT_UNIT;
	boxes[1].y = 200 * LAYOUT_UNIT;
	boxes[1].width = 30 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);

	/* All empty fragments retain only first actual rectangle. */
	observed = 0;
	if (found == 1 &&
	    rect.x == 20 * LAYOUT_UNIT &&
	    rect.width == 0 &&
	    rect.height == 10 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "all empty fragments retain only first actual rectangle");

	/* A first positive rectangle replaces provisional empty geometry without including it. */
	boxes[1].height = 40 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);

	/* Positive rectangle replaces first empty fallback. */
	observed = 0;
	if (found == 1 &&
	    rect.x == 100 * LAYOUT_UNIT &&
	    rect.y == 200 * LAYOUT_UNIT &&
	    rect.width == 30 * LAYOUT_UNIT &&
	    rect.height == 40 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "positive rectangle replaces first empty fallback");

	/* Far-away later empty fragments cannot alter an already positive union. */
	boxes[1].next = &boxes[2];
	boxes[2].x = -100 * LAYOUT_UNIT;
	boxes[2].y = -200 * LAYOUT_UNIT;
	boxes[2].height = 10 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);

	/* Later empty fragment is excluded from positive union. */
	observed = 0;
	if (found == 1 &&
	    rect.x == 100 * LAYOUT_UNIT &&
	    rect.y == 200 * LAYOUT_UNIT &&
	    rect.width == 30 * LAYOUT_UNIT &&
	    rect.height == 40 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "later empty fragment is excluded from positive union");

	/* Nonempty fragments retain the original smallest enclosing rectangle behavior. */
	boxes[2].width = 10 * LAYOUT_UNIT;
	found = layout_node_bounds(&tree, &image->node, &rect);

	/* Positive fragments retain exact enclosing union. */
	observed = 0;
	if (found == 1 &&
	    rect.x == -100 * LAYOUT_UNIT &&
	    rect.y == -200 * LAYOUT_UNIT &&
	    rect.width == 230 * LAYOUT_UNIT &&
	    rect.height == 440 * LAYOUT_UNIT)
		observed = 1;
	zero_check(observed, "positive fragments retain exact enclosing union");

	/* Succeeded: actual native sizing and first-empty rectangle observations ran. */
	return 0;
}
