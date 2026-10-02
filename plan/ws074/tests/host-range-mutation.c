/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies actual native mutation repairs and weak optional callback delivery. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Each weak C observer checks FIFO delivery without adding a DOM trace edge. */
struct mutation_observer {
	unsigned *sequence;
	unsigned order;
	unsigned inserted;
	unsigned removed;
	unsigned data;
	size_t offset;
	size_t count;
	size_t added;
	size_t committed_length;
};

/* Independent native observations accumulate until the embedding is destroyed. */
static unsigned checks;
/* A failed observation determines the exit status after later checks run. */
static unsigned failures;

static void mutation_check(int condition, const char *name);
static void mutation_inserted(void *context, struct dom_node *node);
static void mutation_data(void *context, struct dom_node *node, size_t offset, size_t removed, size_t added);
static void mutation_removed(void *context, struct dom_node *node);
static int mutation_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int mutation_case(struct vm_realm *realm, const void *stack_base);

/*
 * Exercises live native mutation repair and weak optional callback ownership.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	int status;
	int printed;

	/* Construction uses the real collector stack convention. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Production builtins and a genuine Document supply ordinary DOM exceptions. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The primary Document supplies the host while incoming XML owners remain distinct. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The production Window supplies real DOM bindings without alternate mutation paths. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Observe default native mutations independently of their script binding wrappers. */
	status = mutation_case(realm, __builtin_frame_address(0));
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Behavior failures are distinct from embedding allocation failures. */
	printed = printf("range-mutation native checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Any repair, delivery or ownership failure invalidates this prerequisite. */
	if (failures != 0)
		return 1;

	/* Succeeded: native mutation repair and weak ownership contracts hold. */
	return 0;
}

/* Records one independently observable graph or host contract. */
static void
mutation_check(
	int condition,
	const char *name)
{
	int printed;

	/* Preserve accounting even when an earlier native contract failed. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation contributes to the final count. */
	return;
}

/* Observes the complete native insertion and configured subscriber order. */
static void
mutation_inserted(
	void *context,
	struct dom_node *node)
{
	struct mutation_observer *observer;

	/* Each optional observer sees completed links before host or layout processing. */
	observer = context;
	observer->inserted++;
	(*observer->sequence)++;
	mutation_check(*observer->sequence == observer->order, "optional insertion callbacks retain FIFO order");
	mutation_check(node->parent != NULL, "insertion callback sees final native parent");
	mutation_check(node->previous == NULL, "first-child insertion callback sees final preceding link");
	mutation_check(node->parent->first_child == node, "insertion callback sees complete parent first-child link");

	/* Succeeded: this callback performed only native observations. */
	return;
}

/* Observes complete native data and exact replacement lengths. */
static void
mutation_data(
	void *context,
	struct dom_node *node,
	size_t offset,
	size_t removed,
	size_t added)
{
	struct mutation_observer *observer;

	/* Optional repairs preserve subscriber order and exact producer interval data. */
	observer = context;
	observer->data++;
	(*observer->sequence)++;
	mutation_check(*observer->sequence == observer->order, "optional data callbacks retain FIFO order");
	observer->offset = offset;
	observer->count = removed;
	observer->added = added;
	observer->committed_length = ((struct dom_character_data *)node)->data.length;
	mutation_check(node->type == DOM_TEXT, "data callback sees genuine CharacterData node");

	/* Succeeded: no allocation or registry mutation occurred inside notification. */
	return;
}

/* Counts original-Document removal deliveries before unlinking. */
static void
mutation_removed(
	void *context,
	struct dom_node *node)
{
	struct mutation_observer *observer;

	UNUSED_PARAMETER(node);

	/* Validation cannot start removal or adoption notifications. */
	observer = context;
	observer->removed++;

	/* Succeeded: the actual removal was observed. */
	return;
}

/* Constructs genuine nodes through the default production script bindings. */
static int
mutation_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Convert the fixture without invoking any alternate binding path. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* The actual script engine supplies native wrappers and original owner graphs. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the genuine completion is available to the embedding. */
	return 0;
}

/* Verifies native repairs and rejected allocation with conservative C-stack retention disabled. */
static int
mutation_case(
	struct vm_realm *realm,
	const void *stack_base)
{
	struct mutation_observer first;
	struct mutation_observer second;
	struct mutation_observer generic;
	struct dom_removal_subscription *first_token;
	struct dom_removal_subscription *second_token;
	struct dom_removal_subscription *generic_token;
	struct dom_node *text;
	struct dom_node *incoming;
	struct dom_character_data *characters;
	struct dom_document *owner;
	struct vm_cell *found;
	vm_value range;
	vm_value answer;
	uintptr_t creator_address;
	uintptr_t text_address;
	uint16_t suffix[2];
	unsigned sequence;
	uint32_t generation;
	int status;

	/* Observer contexts are C storage and token allocations do not retain any VM cell. */
	memset(&first, 0, sizeof(first));
	memset(&second, 0, sizeof(second));
	memset(&generic, 0, sizeof(generic));
	sequence = 0;
	first.sequence = &sequence;
	first.order = 1;
	second.sequence = &sequence;
	second.order = 2;
	first_token = NULL;
	second_token = NULL;
	generic_token = NULL;

	/* Globals supply genuine script ownership while direct C calls have no receiver frame. */
	do {
		status = mutation_script(realm,
					 "var mutationDoc=document.implementation.createDocument(null,'root',null);"
					 "var mutationText=mutationDoc.createTextNode('ABCDE');"
					 "mutationDoc.documentElement.appendChild(mutationText);"
					 "var mutationRange=mutationDoc.createRange();"
					 "mutationRange.setStart(mutationText,1);mutationRange.setEnd(mutationText,4);"
					 "var mutationIncoming=mutationDoc.createComment('incoming');mutationText;",
					 &answer);
		if (status != 0)
			break;
		text = bind_node_of(answer);
		characters = (struct dom_character_data *)text;
		owner = text->document;
		text_address = (uintptr_t)text;
		creator_address = (uintptr_t)owner;
		status = mutation_script(realm, "mutationRange", &range);
		if (status != 0)
			break;
		status = mutation_script(realm, "mutationIncoming", &answer);
		if (status != 0)
			break;
		incoming = bind_node_of(answer);

		/* Three real weak subscriptions distinguish optional delivery from removal-only behavior. */
		status = dom_removal_subscribe(owner, &first, mutation_removed, &first_token);
		if (status != 0)
			break;
		dom_removal_set_updates(first_token, mutation_inserted, mutation_data);
		status = dom_removal_subscribe(owner, &second, mutation_removed, &second_token);
		if (status != 0)
			break;
		dom_removal_set_updates(second_token, mutation_inserted, mutation_data);
		status = dom_removal_subscribe(owner, &generic, mutation_removed, &generic_token);
		if (status != 0)
			break;

		/* Real GC sees script globals and native traces, never these C graph pointers. */
		vm_heap_set_stack_base(realm->heap, NULL);
		vm_heap_collect(realm->heap);
		found = vm_heap_find_cell(realm->heap, text_address);
		mutation_check(found != NULL, "genuine script roots retain native data graph during collection");

		/* Default DOM insertion delivers optional callbacks after complete first-child links. */
		dom_insert_before(text->parent, incoming, text);
		mutation_check(first.inserted == 1U, "first optional observer receives native insertion");
		mutation_check(second.inserted == 1U, "second optional observer receives native insertion");
		mutation_check(generic.inserted == 0U && generic.removed == 0U, "generic removal observer receives no insertion delivery");

		/* Full native replacement accepts aliased old input and resets live Range points. */
		sequence = 0;
		status = dom_text_set(text, characters->data.data + 1U, 3U);
		if (status != 0)
			break;
		mutation_check(characters->data.length == 3U, "aliased replacement commits independent native buffer length");
		mutation_check(characters->data.data[0] == 'B' && characters->data.data[2] == 'D', "aliased replacement preserves exact UTF16 units");
		mutation_check(
			first.offset == 0U &&
			first.count == 5U &&
			first.added == 3U &&
			first.committed_length == 3U,
			"full replacement notifies actual previous interval");
		status = bind_abstract_range_interface.attributes[2].getter(realm, range, NULL, 0, &answer);
		if (status != 0)
			break;
		mutation_check(answer == vm_value_number(0), "direct native full replacement resets actual start");
		status = bind_abstract_range_interface.attributes[3].getter(realm, range, NULL, 0, &answer);
		if (status != 0)
			break;
		mutation_check(answer == vm_value_number(0), "direct native full replacement resets actual end");

		/* Nonzero current endpoints make failed preparation's preservation independently observable. */
		status = mutation_script(realm,
					 "mutationRange.setStart(mutationText,1);mutationRange.setEnd(mutationText,2);",
					 &answer);
		if (status != 0)
			break;

		/* Oversized preparation fails before copying input, changing generation or notifying observers. */
		generation = owner->generation;
		status = dom_text_set(text, characters->data.data, SIZE_MAX);
		mutation_check(status == ENOMEM, "oversized full replacement propagates checked allocation failure");
		mutation_check(owner->generation == generation, "failed replacement preserves native generation");
		mutation_check(characters->data.length == 3U && characters->data.data[0] == 'B', "failed replacement preserves complete old data");
		mutation_check(first.data == 1U && second.data == 1U, "failed replacement delivers no optional notification");
		status = bind_abstract_range_interface.attributes[3].getter(realm, range, NULL, 0, &answer);
		if (status != 0)
			break;
		mutation_check(answer == vm_value_number(2), "failed replacement preserves actual native endpoint");

		/* Efficient native append reports a zero-removal interval and preserves old equal endpoints. */
		sequence = 0;
		suffix[0] = 0xD83DU;
		suffix[1] = 0xDE00U;
		status = dom_text_append(text, suffix, 2U);
		if (status != 0)
			break;
		mutation_check(
			first.offset == 3U &&
			first.count == 0U &&
			first.added == 2U &&
			first.committed_length == 5U,
			"native append notifies exact UTF16 insertion interval");
		mutation_check(characters->data.length == 5U && characters->data.data[4] == 0xDE00U, "native append commits exact surrogate units");
		status = bind_abstract_range_interface.attributes[3].getter(realm, range, NULL, 0, &answer);
		if (status != 0)
			break;
		mutation_check(answer == vm_value_number(2), "native append preserves earlier live endpoint");
		mutation_check(generic.data == 0U && generic.removed == 0U, "generic removal observer receives no data delivery");

		/* With no script graph roots, configured weak callbacks cannot retain their creator Document. */
		status = mutation_script(realm,
					 "mutationRange=null;mutationIncoming=null;mutationText=null;mutationDoc=null;", &answer);
		if (status != 0)
			break;
		vm_heap_collect(realm->heap);
		found = vm_heap_find_cell(realm->heap, creator_address);
		mutation_check(found == NULL, "configured optional weak token does not retain creator Document");
		found = vm_heap_find_cell(realm->heap, text_address);
		mutation_check(found == NULL, "configured optional weak token does not retain endpoint graph");
		status = 0;
	} while (0);

	/* Token cleanup remains valid after its Document and native Range have already finalized. */
	vm_heap_set_stack_base(realm->heap, stack_base);
	dom_removal_unsubscribe(generic_token);
	dom_removal_unsubscribe(second_token);
	dom_removal_unsubscribe(first_token);
	if (status != 0)
		return status;

	/* Succeeded: optional repairs preserve native data, checked failure and weak ownership. */
	return 0;
}
