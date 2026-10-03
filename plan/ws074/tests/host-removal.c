/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies weak C removal subscriptions under actual production GC and DOM mutations. */

#include "dom/dom.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* A synchronous callback observes original links and FIFO position without allocating. */
struct removal_observation {
	struct dom_node *expected;
	struct dom_node *parent;
	struct dom_node *previous;
	struct dom_node *next;
	unsigned *sequence;
	unsigned order;
	unsigned calls;
};

/* The token is owned by a collectible observer but does not trace its Document. */
struct removal_observer {
	struct vm_cell cell;
	struct dom_removal_subscription *subscription;
	unsigned *finalized;
	unsigned char padding[1024];
};

/* Assertion accounting lasts until all independent heap-order cases finish. */
static unsigned checks;
/* A failing check determines the final process status after later checks run. */
static unsigned failures;

static void removal_observer_finalize(struct vm_heap *heap, struct vm_cell *cell);

/* No trace edge turns a weak subscription into permanent Document or observer ownership. */
static const struct vm_cell_type removal_observer_type = {"removal-observer-test", NULL, removal_observer_finalize};

static void removal_check(int condition, const char *name);
static void removal_observe(void *context, struct dom_node *node);
static void removal_count(void *context, struct dom_node *node);
static void removal_weak_notify(void *context, struct dom_node *node);
static int removal_links(void);
static int removal_adoption(void);
static int removal_order(unsigned kind);

/*
 * Verifies notification links and independent Document/subscriber finalization.
 */
int
main(
	void)
{
	unsigned kind;
	int error;
	int printed;

	/* Ordinary DOM moves and unlinks deliver exactly the current subscriptions. */
	error = removal_links();
	if (error != 0)
		return 2;

	/* Adoption must preserve rooted token identity and original generic observers. */
	error = removal_adoption();
	if (error != 0)
		return 2;

	/* Cases explicitly order collection and also exercise small/large heap teardown. */
	for (kind = 0; kind < 4; kind++) {
		error = removal_order(kind);
		if (error != 0)
			return 2;
	}

	/* The independent observed contracts determine the process outcome. */
	printed = printf("removal subscription checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any weak ownership or delivery failure invalidates this foundation. */
	if (failures != 0)
		return 1;

	/* Succeeded: delivery and every tested finalization order preserved ownership. */
	return 0;
}

/* Records one actual ownership or callback observation. */
static void
removal_check(
	int condition,
	const char *name)
{
	int printed;

	/* Later cases remain observable even if a preceding contract failed. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the contract contributes to the final check accounting. */
	return;
}

/* Observes exact old links and callback registration order before unlinking. */
static void
removal_observe(
	void *context,
	struct dom_node *node)
{
	struct removal_observation *observation;
	int same;

	/* Every callback sees the same removed Node while its original relatives are intact. */
	observation = context;
	observation->calls++;
	(*observation->sequence)++;
	same = 0;
	if (node == observation->expected &&
	    node->parent == observation->parent &&
	    node->previous == observation->previous &&
	    node->next == observation->next)
		same = 1;
	removal_check(same, "notification preserves exact pre-unlink links");

	/* Registration order is independent of the node's eventual tree position. */
	same = 0;
	if (*observation->sequence == observation->order)
		same = 1;
	removal_check(same, "multiple subscribers deliver FIFO");

	/* Succeeded: this callback only observed links and C-owned counters. */
	return;
}

/* Accepts notifications without introducing an allocation or trace edge. */
static void
removal_weak_notify(
	void *context,
	struct dom_node *node)
{
	UNUSED_PARAMETER(context);
	UNUSED_PARAMETER(node);

	/* Succeeded: this lifetime fixture needs no mutation repair. */
	return;
}

/* Unlinks the separately allocated token without touching potentially freed GC neighbours. */
static void
removal_observer_finalize(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct removal_observer *observer;

	UNUSED_PARAMETER(heap);

	/* Only C allocations and this still valid observer are read during finalization. */
	observer = (struct removal_observer *)cell;
	dom_removal_unsubscribe(observer->subscription);
	(*observer->finalized)++;

	/* Succeeded: no future weak callback can reach this observer's reclaimed storage. */
	return;
}

/* Exercises original links, detached silence, token release and move notifications. */
static int
removal_links(
	void)
{
	struct vm_heap *heap;
	struct dom_document *document;
	struct dom_document *other;
	struct dom_node *root;
	struct dom_node *first;
	struct dom_node *middle;
	struct dom_node *last;
	struct dom_removal_subscription *one;
	struct dom_removal_subscription *two;
	struct removal_observation a;
	struct removal_observation b;
	unsigned sequence;
	int same;
	int error;

	/* Construction uses actual DOM allocation and the default conservative stack collector. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* A separate Document provides an unrelated notification control. */
	other = dom_document_create(heap);
	if (other == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* A fragment gives three distinct siblings without name allocation. */
	root = dom_fragment_create(document);
	if (root == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Every node is allocated and checked before constructing the next. */
	first = dom_fragment_create(document);
	if (first == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* The middle node has both neighbours during its first removal. */
	middle = dom_fragment_create(document);
	if (middle == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* The last node provides a stable following sibling. */
	last = dom_fragment_create(document);
	if (last == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Production low-level append creates the observed live link structure. */
	dom_append_child(root, first);
	dom_append_child(root, middle);
	dom_append_child(root, last);

	/* Both independent callbacks observe the same original links in registration order. */
	memset(&a, 0, sizeof(a));
	a.expected = middle;
	a.parent = root;
	a.previous = first;
	a.next = last;
	a.sequence = &sequence;
	a.order = 1;

	/* The second callback has its own call counter and FIFO expectation. */
	b = a;
	b.order = 2;
	sequence = 0;
	one = NULL;
	two = NULL;
	error = dom_removal_subscribe(document, &a, removal_observe, &one);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Failure cleanup never leaves the first token pointing at expired stack context. */
	error = dom_removal_subscribe(document, &b, removal_observe, &two);
	if (error != 0) {
		dom_removal_unsubscribe(one);
		vm_heap_destroy(heap);
		return error;
	}

	/* Removal delivers both callbacks before linking the surviving siblings together. */
	dom_remove(middle);
	same = 0;
	if (a.calls == 1 && b.calls == 1)
		same = 1;
	removal_check(same, "one removal reaches both registered contexts exactly once");

	/* Detached removal and another Document's registry cannot invoke these subscribers. */
	dom_remove(middle);
	dom_removal_notify(other, middle);
	same = 0;
	if (a.calls == 1 && b.calls == 1)
		same = 1;
	removal_check(same, "detached nodes and unrelated Documents are silent");

	/* Releasing the second token preserves the first FIFO member. */
	dom_removal_unsubscribe(two);
	dom_removal_unsubscribe(NULL);
	dom_append_child(root, middle);
	a.previous = last;
	a.next = NULL;
	sequence = 0;
	dom_append_child(root, middle);
	same = 0;
	if (a.calls == 2 && b.calls == 1)
		same = 1;
	removal_check(same, "append move notifies only unreleased subscriber");

	/* No token remains before stack-owned callback contexts are retired. */
	dom_removal_unsubscribe(one);
	dom_remove(middle);
	same = 0;
	if (a.calls == 2 && b.calls == 1)
		same = 1;
	removal_check(same, "final token release removes every weak callback");

	/* Invalid requests leave their output empty and create no obligation. */
	one = NULL;
	error = dom_removal_subscribe(NULL, &a, removal_observe, &one);
	same = 0;
	if (error == EINVAL && one == NULL)
		same = 1;
	removal_check(same, "invalid registration does not publish a token");
	vm_heap_destroy(heap);

	/* Succeeded: callbacks observed default production mutation paths. */
	return 0;
}

/* Checks native same-heap adoption and weak root migration without scripting or test switches. */
static int
removal_adoption(
	void)
{
	struct vm_heap *heap;
	struct vm_heap *foreign;
	struct dom_document *original;
	struct dom_document *destination;
	struct dom_document *unrelated;
	struct dom_element *root;
	struct dom_element *child;
	struct vm_string *name;
	struct dom_removal_subscription *generic;
	struct dom_removal_subscription *rooted;
	unsigned original_calls;
	unsigned rooted_calls;
	unsigned iteration;
	int error;
	int same;

	/* Ordinary heap creation supplies every cell and allocation used by checked adoption. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	error = vm_heap_create(&foreign, 0);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Each allocation is checked before constructing the next native component. */
	original = dom_document_create(heap);
	if (original == NULL) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Allocates the destination independently of the original Document. */
	destination = dom_document_create(heap);
	if (destination == NULL) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Allocates a genuinely separate-heap Document for preflight rejection. */
	unrelated = dom_document_create(foreign);
	if (unrelated == NULL) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Prepares the actual native element name. */
	name = vm_atom_from_ascii(heap, "div");
	if (name == NULL) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Allocates the adoption root before constructing its descendant. */
	root = dom_element_create(original, DOM_NS_HTML, name, NULL);
	if (root == NULL) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Allocates an independently observed descendant. */
	child = dom_element_create(original, DOM_NS_HTML, name, NULL);
	if (child == NULL) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* The original Document has one generic observer and one actual-root subscriber. */
	dom_append_child(&original->node, &root->node);
	dom_append_child(&root->node, &child->node);
	original_calls = 0;
	rooted_calls = 0;
	error = dom_removal_subscribe(original, &original_calls, removal_count, &generic);
	if (error != 0) {
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return error;
	}

	/* Registers the rooted subscriber independently of the generic observer. */
	error = dom_removal_subscribe(original, &rooted_calls, removal_count, &rooted);
	if (error != 0) {
		dom_removal_unsubscribe(generic);
		vm_heap_destroy(foreign);
		vm_heap_destroy(heap);
		return error;
	}

	/* Associates only the rooted subscriber with its actual native subtree. */
	dom_removal_set_root(rooted, &root->node);

	/* Cross-heap rejection cannot unlink the source or change descendant owner Documents. */
	error = dom_adopt(unrelated, &root->node);
	same = 0;
	if (error == EINVAL &&
	    root->node.parent == &original->node &&
	    root->node.document == original &&
	    child->node.document == original)
		same = 1;
	removal_check(same, "cross-heap adoption rejects before unlink or owner change");
	error = dom_adopt(destination, &original->node);
	removal_check(error == EINVAL, "Document native adoption rejected");

	/* Current registry movement preserves token handles across several back-and-forth transfers. */
	for (iteration = 0; iteration < 4U; iteration++) {
		error = dom_adopt(destination, &root->node);
		if (error != 0) {
			dom_removal_unsubscribe(rooted);
			dom_removal_unsubscribe(generic);
			vm_heap_destroy(foreign);
			vm_heap_destroy(heap);
			return error;
		}

		/* Successful adoption updates both observed current owner Documents. */
		removal_check(root->node.document == destination, "root current Document changed");
		removal_check(child->node.document == destination, "descendant current Document changed");
		original_calls = 0;
		rooted_calls = 0;
		dom_remove(&child->node);
		same = 0;
		if (rooted_calls == 1U && original_calls == 0U)
			same = 1;
		removal_check(same, "migrated root receives destination removal without original observer");
		dom_append_child(&root->node, &child->node);
		rooted_calls = 0;
		dom_removal_notify(original, &child->node);
		removal_check(rooted_calls == 0U, "original registry no longer retains migrated token");
		error = dom_adopt(original, &root->node);
		if (error != 0) {
			dom_removal_unsubscribe(rooted);
			dom_removal_unsubscribe(generic);
			vm_heap_destroy(foreign);
			vm_heap_destroy(heap);
			return error;
		}
	}

	/* Both original handles unlink their current lists without accessing a former Document. */
	dom_removal_unsubscribe(rooted);
	dom_removal_unsubscribe(generic);
	vm_heap_destroy(foreign);
	vm_heap_destroy(heap);

	/* Succeeded: default checked adoption preserved exact weak registration ownership. */
	return 0;
}

/* Counts deliveries without reading possibly reclaimed weak test roots. */
static void
removal_count(
	void *context,
	struct dom_node *node)
{
	unsigned *calls;

	UNUSED_PARAMETER(node);

	/* Stack accounting outlives each finite native subscription and its heap. */
	calls = context;
	(*calls)++;

	/* Succeeded: this registry's actual delivery was observed. */
	return;
}

/* Forces each finalization order without conservative stack roots hiding a weak edge. */
static int
removal_order(
	unsigned kind)
{
	struct vm_heap *heap;
	struct dom_document *document;
	struct removal_observer *observer;
	struct removal_observer *neighbour;
	struct vm_cell *root;
	struct vm_cell *found;
	uintptr_t document_address;
	uintptr_t observer_address;
	size_t size;
	unsigned finalized;
	int error;
	int same;
	struct dom_document *other;
	struct dom_element *element;
	struct vm_string *name;

	/* Explicit roots replace stack scanning for each collection-order assertion. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Different cell sizes also change block/large allocation teardown order. */
	size = offsetof(struct removal_observer, padding);
	if (kind == 3)
		size = sizeof(*observer);

	/* The selected cell size affects only the real finalization order. */
	observer = vm_heap_alloc(heap, &removal_observer_type, size);
	if (observer == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* The stack counter outlives the whole heap and every observer finalizer. */
	finalized = 0;
	observer->finalized = &finalized;
	error = dom_removal_subscribe(document, observer, removal_weak_notify, &observer->subscription);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* The same token must remain safe after its root and registry move before finalization. */
	other = dom_document_create(heap);
	if (other == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Prepares an actual root for the token migration before finalization. */
	name = vm_atom_from_ascii(heap, "span");
	if (name == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Allocates the node named weakly by the migrated token. */
	element = dom_element_create(document, DOM_NS_HTML, name, NULL);
	if (element == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Associates the synthetic weak token without adding a GC ownership edge. */
	dom_removal_set_root(observer->subscription, &element->node);
	error = dom_adopt(other, &element->node);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Subsequent finalization assertions observe the destination registry ownership. */
	document = other;

	/* Whole-heap cases exercise the actual collector teardown, not manual token destruction. */
	if (kind >= 2) {
		/* Neighbour tokens remain valid even when several GC observers finalize together. */
		neighbour = vm_heap_alloc(heap, &removal_observer_type, size);
		if (neighbour == NULL) {
			vm_heap_destroy(heap);
			return ENOMEM;
		}

		/* The second finalizer shares a counter whose stack storage outlives teardown. */
		neighbour->finalized = &finalized;
		error = dom_removal_subscribe(document, neighbour, removal_weak_notify, &neighbour->subscription);
		if (error != 0) {
			vm_heap_destroy(heap);
			return error;
		}

		/* Neither observer finalizer reads a possibly reclaimed neighbour GC cell. */
		vm_heap_destroy(heap);
		same = 0;
		if (finalized == 2)
			same = 1;
		removal_check(same, "whole heap teardown finalizes each subscribed observer once");

		/* Succeeded: heap teardown released both neighbouring tokens exactly once. */
		return 0;
	}

	/* Exactly one side remains rooted to force the other side to finalize first. */
	document_address = (uintptr_t)document;
	observer_address = (uintptr_t)observer;
	root = &document->node.cell;
	if (kind == 1)
		root = &observer->cell;
	error = vm_heap_add_root(heap, &root);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Collection excludes all test C locals so only the explicit selected root survives. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	if (kind == 0) {
		found = vm_heap_find_cell(heap, observer_address);
		same = 0;
		if (found == NULL && finalized == 1)
			same = 1;
		removal_check(same, "live Document does not retain a weak subscriber");
	} else {
		found = vm_heap_find_cell(heap, document_address);
		same = 0;
		if (found == NULL && finalized == 0)
			same = 1;
		removal_check(same, "token safely outlives a finalized Document");
	}

	/* Dropping the remaining side releases the final C registry ownership. */
	root = NULL;
	vm_heap_collect(heap);
	same = 0;
	if (finalized == 1)
		same = 1;
	removal_check(same, "later observer cleanup is safe in either finalization order");

	/* Repeated collection cannot invoke a reclaimed subscriber's finalizer twice. */
	vm_heap_collect(heap);
	same = 0;
	if (finalized == 1)
		same = 1;
	removal_check(same, "repeated collection does not double finalize a subscriber");
	vm_heap_remove_root(heap, &root);
	vm_heap_destroy(heap);

	/* Succeeded: the selected collection order used actual production finalizers. */
	return 0;
}
