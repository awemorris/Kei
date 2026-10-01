/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Shapes (hidden classes): the tree of property layouts objects share.
 *
 * A shape is the shape before it plus one property: its key, its
 * attributes and the slot its value takes (the count of properties
 * before it).  Objects made the same way walk the same path from the
 * heap's root shape and share every shape on it; the transitions from a
 * shape to the shapes made from it are kept so the path is found again.
 * Keys are values: an atom, a symbol, or an int32 index.
 */

#include "vm/internal.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The fewest transitions a shape's table is made for. */
#define SHAPE_TRANSITIONS_MIN	4U

/*
 * One transition: the key and attributes added, and the shape they lead to.
 */
struct vm_shape_transition {
	vm_value key;
	uint32_t attributes;
	struct vm_shape *shape;
};

/*
 * A shape.
 *
 * parent is NULL for the root, whose key is VM_VALUE_EMPTY.  count is the
 * number of properties on the path to this shape (this one's slot plus
 * one).  The transitions are malloc'd and freed with the shape; the shape
 * holds the shapes they lead to, so a path lives as long as its root.
 */
struct vm_shape {
	struct vm_cell cell;
	struct vm_shape *parent;
	vm_value key;
	uint32_t attributes;
	uint32_t slot;
	uint32_t count;
	uint32_t transition_count;
	uint32_t transition_capacity;
	uint32_t reserved;
	struct vm_shape_transition *transitions;
};

static void shape_trace(struct vm_heap *heap, struct vm_cell *cell);
static void shape_finalize(struct vm_heap *heap, struct vm_cell *cell);

/* The cell type of shapes: they hold their parent, their key and the shapes made from them. */
static const struct vm_cell_type shape_type = {
	"shape", shape_trace, shape_finalize
};

/*
 * Reports the heap's root shape (no properties), making it the first time.
 */
struct vm_shape *
vm_shape_root(
	struct vm_heap *heap)
{
	struct vm_shape *root;
	int error;

	/* The root made before. */
	if (heap->root_shape != NULL)
		return heap->root_shape;

	/* The root: no parent, no key, no properties. */
	root = vm_heap_alloc(heap, &shape_type, sizeof(*root));
	if (root == NULL)
		return NULL;
	root->key = VM_VALUE_EMPTY;

	/* The heap keeps it (and through it every shape) as a root. */
	heap->root_shape = root;
	error = vm_heap_add_root(heap, (struct vm_cell **)&heap->root_shape);
	if (error != 0) {
		heap->root_shape = NULL;
		return NULL;
	}

	/* Succeeded: the root shape. */
	return root;
}

/*
 * Reports the shape of a shape plus a property, following the transition
 * made before or making it; NULL when out of memory.
 */
struct vm_shape *
vm_shape_add(
	struct vm_heap *heap,
	struct vm_shape *shape,
	vm_value key,
	uint32_t attributes)
{
	struct vm_shape_transition *transitions;
	struct vm_shape *added;
	uint32_t capacity;
	uint32_t index;

	/* The transition made before, when there is one. */
	for (index = 0; index < shape->transition_count; index++) {
		if (shape->transitions[index].key != key)
			continue;
		if (shape->transitions[index].attributes != attributes)
			continue;
		return shape->transitions[index].shape;
	}

	/* The new shape (the parent is on this frame, so a collection keeps it). */
	added = vm_heap_alloc(heap, &shape_type, sizeof(*added));
	if (added == NULL)
		return NULL;
	added->parent = shape;
	added->key = key;
	added->attributes = attributes;
	added->slot = shape->count;
	added->count = shape->count + 1U;

	/* Room for the transition. */
	if (shape->transition_count == shape->transition_capacity) {
		capacity = shape->transition_capacity * 2U;
		if (capacity < SHAPE_TRANSITIONS_MIN)
			capacity = SHAPE_TRANSITIONS_MIN;
		transitions = realloc(shape->transitions, capacity * sizeof(*transitions));
		if (transitions == NULL)
			return NULL;
		shape->transitions = transitions;
		shape->transition_capacity = capacity;
	}

	/* Records the transition. */
	shape->transitions[shape->transition_count].key = key;
	shape->transitions[shape->transition_count].attributes = attributes;
	shape->transitions[shape->transition_count].shape = added;
	shape->transition_count++;

	/* Succeeded: the new shape. */
	return added;
}

/*
 * Finds a key's slot and attributes in a shape; zero when the shape does
 * not have the key.
 */
int
vm_shape_find(
	const struct vm_shape *shape,
	vm_value key,
	uint32_t *slot,
	uint32_t *attributes)
{
	/* Walks from the last property added to the first. */
	while (shape != NULL && shape->parent != NULL) {
		/* The key's own shape. */
		if (shape->key == key) {
			*slot = shape->slot;
			*attributes = shape->attributes;
			return 1;
		}

		/* The property before it. */
		shape = shape->parent;
	}

	/* Not among the shape's properties. */
	return 0;
}

/*
 * Reports how many properties a shape has.
 */
uint32_t
vm_shape_count(
	const struct vm_shape *shape)
{
	/* The count kept on the shape. */
	return shape->count;
}

/*
 * Writes a shape's keys, slots and attributes in the order they were
 * added (each array as long as the shape's count).
 */
int
vm_shape_keys(
	const struct vm_shape *shape,
	vm_value *keys,
	uint32_t *slots,
	uint32_t *attributes)
{
	uint32_t index;

	/* From the last property to the first, each at its own place. */
	index = shape->count;
	while (shape->parent != NULL) {
		index--;
		keys[index] = shape->key;
		slots[index] = shape->slot;
		attributes[index] = shape->attributes;
		shape = shape->parent;
	}

	/* Succeeded: the arrays are filled. */
	return 0;
}

/* Marks a shape's parent, its key and the shapes its transitions lead to. */
static void
shape_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct vm_shape *shape;
	uint32_t index;

	/* The path before it, and the key it adds. */
	shape = (struct vm_shape *)cell;
	if (shape->parent != NULL)
		vm_heap_mark(heap, &shape->parent->cell);
	vm_heap_mark_value(heap, shape->key);

	/* The shapes made from it. */
	for (index = 0; index < shape->transition_count; index++)
		vm_heap_mark(heap, &shape->transitions[index].shape->cell);
}

/* Frees a dead shape's transition table. */
static void
shape_finalize(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct vm_shape *shape;

	UNUSED_PARAMETER(heap);

	/* The table the shape owns. */
	shape = (struct vm_shape *)cell;
	free(shape->transitions);
	shape->transitions = NULL;
}
