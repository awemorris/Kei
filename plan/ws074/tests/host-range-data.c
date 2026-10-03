/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies native partial UTF16 replacement, allocation-free deletion and actual live Range repair. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Each weak C observer checks FIFO delivery without adding a DOM trace edge. */
struct partial_observer {
	unsigned *sequence;
	unsigned order;
	unsigned removed;
	unsigned data;
	size_t offset;
	size_t count;
	size_t added;
	size_t committed_length;
	uint16_t first;
	uint16_t last;
};

/* Independent native observations accumulate until the embedding is destroyed. */
static unsigned checks;
/* A failed observation determines the exit status after later checks run. */
static unsigned failures;

static void partial_check(int condition, const char *name);
static void partial_data(void *context, struct dom_node *node, size_t offset, size_t removed, size_t added);
static void partial_removed(void *context, struct dom_node *node);
static int partial_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int partial_case(struct vm_realm *realm, const void *stack_base);
static int partial_expect(struct vm_realm *realm, const char *expression, const char *name);

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
	status = partial_case(realm, __builtin_frame_address(0));
	if (status != 0) {
		bind_window_destroy(window);
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Tear down the embedding only after its native checks completed. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Behavior failures are distinct from embedding allocation failures. */
	printed = printf("range-data native checks: %u/%u passed\n", checks - failures, checks);
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
partial_check(
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

/* Observes complete native data and exact replacement lengths. */
static void
partial_data(
	void *context,
	struct dom_node *node,
	size_t offset,
	size_t removed,
	size_t added)
{
	struct partial_observer *observer;

	/* Optional repairs preserve subscriber order and exact producer interval data. */
	observer = context;
	observer->data++;
	(*observer->sequence)++;
	partial_check(*observer->sequence == observer->order, "optional data callbacks retain FIFO order");
	observer->offset = offset;
	observer->count = removed;
	observer->added = added;
	observer->committed_length = ((struct dom_character_data *)node)->data.length;

	/* Empty data has no first or last unit for the pure observer to dereference. */
	observer->first = 0;
	observer->last = 0;
	if (observer->committed_length != 0) {
		observer->first = ((struct dom_character_data *)node)->data.data[0];
		observer->last = ((struct dom_character_data *)node)->data.data[observer->committed_length - 1U];
	}

	/* Only genuine supported native CharacterData reaches the configured observer. */
	partial_check(node->type == DOM_TEXT || node->type == DOM_COMMENT, "data callback sees genuine CharacterData node");

	/* Succeeded: no allocation or registry mutation occurred inside notification. */
	return;
}

/* Counts original-Document removal deliveries before unlinking. */
static void
partial_removed(
	void *context,
	struct dom_node *node)
{
	struct partial_observer *observer;

	UNUSED_PARAMETER(node);

	/* Validation cannot start removal or adoption notifications. */
	observer = context;
	observer->removed++;

	/* Succeeded: the actual removal was observed. */
	return;
}

/* Constructs genuine nodes through the default production script bindings. */
static int
partial_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;
	int printed;

	/* Convert the fixture without invoking any alternate binding path. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* The actual script engine supplies native wrappers and original owner graphs. */
	status = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		printed = fprintf(stderr, "FAIL fixture script status%d: %s\n", status, source);
		if (printed < 0)
			return EIO;

		/* Preserve the actual interpreter failure after exposing the failing fixture operation. */
		return status;
	}

	/* Release source storage after checking successful execution. */
	wb_units_release(&units);

	/* Succeeded: the genuine completion is available to the embedding. */
	return 0;
}

/* Verifies partial native data mutation and real live Range offsets through direct calls. */
static int
partial_case(
	struct vm_realm *realm,
	const void *stack_base)
{
	struct partial_observer first;
	struct partial_observer second;
	struct partial_observer generic;
	struct dom_removal_subscription *first_token;
	struct dom_removal_subscription *second_token;
	struct dom_removal_subscription *generic_token;
	struct dom_node *text;
	struct dom_node *comment;
	struct dom_character_data *characters;
	struct dom_document *owner;
	struct vm_cell *found;
	vm_value answer;
	uintptr_t creator_address;
	uintptr_t text_address;
	uint16_t *storage;
	const uint16_t units[2] = { 'X', 'Y' };
	const uint16_t unicode[2] = { 0xd800, 0xd800 };
	unsigned sequence;
	unsigned notifications;
	uint32_t generation;
	int status;

	/* Weak observer contexts live in C storage without owning native nodes. */
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

	/* Genuine globals own source data and every independent native boundary tuple. */
	status = partial_script(realm,
				"var partialTarget=document.createElement('div');document.appendChild(partialTarget);"
				"var partialDoc=document.implementation.createDocument(null,'root',null);"
				"var partialText=partialDoc.createTextNode('ABCDEFGH');"
				"partialDoc.documentElement.appendChild(partialText);"
				"var partialRanges=[],partialOffsets=[0,2,3,5,6,8];"
				"for(var j=0;j<6;j++)partialRanges.push(partialDoc.createRange());"
				"var partialClone=partialDoc.createRange();partialClone.detach();"
				"function partialReset(s){partialText.data=s;for(var j=0;j<6;j++){"
				"partialRanges[j].setStart(partialText,partialOffsets[j]);"
				"partialRanges[j].collapse(true);}partialClone.setStart(partialText,5);"
				"partialClone.collapse(true);}partialReset('ABCDEFGH');"
				"partialClone=partialRanges[3].cloneRange();partialClone.detach();"
				"function partialPoints(){return partialRanges.map(function(r){"
				"return r.startOffset+':'+r.endOffset;}).join(',')+','+partialClone.startOffset;}"
				"partialText;",
				&answer);
	if (status != 0)
		goto cleanup;
	text = bind_node_of(answer);
	if (text == NULL || text->type != DOM_TEXT) {
		status = EIO;
		goto cleanup;
	}

	/* Read only verified native CharacterData and its original owner. */
	characters = (struct dom_character_data *)text;
	owner = text->document;
	text_address = (uintptr_t)text;
	creator_address = (uintptr_t)owner;

	/* Two optional native observers and one removal-only observer remain weak. */
	status = dom_removal_subscribe(owner, &first, partial_removed, &first_token);
	if (status != 0)
		goto cleanup;
	dom_removal_set_updates(first_token, NULL, partial_data);
	dom_removal_rebind_root(first_token, text);
	status = dom_removal_subscribe(owner, &second, partial_removed, &second_token);
	if (status != 0)
		goto cleanup;
	dom_removal_set_updates(second_token, NULL, partial_data);
	dom_removal_rebind_root(second_token, text);
	status = dom_removal_subscribe(owner, &generic, partial_removed, &generic_token);
	if (status != 0)
		goto cleanup;

	/* Only script globals and native graph traces retain input during actual collection. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, text_address);
	partial_check(found != NULL, "script globals retain native input with stack scanning disabled");

	/* Interior replacement publishes complete independent data and exact clamped intervals. */
	sequence = 0;
	status = dom_text_replace(text, 2, 3, units, 2);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.length == 7 &&
		characters->data.data[2] == 'X' &&
		characters->data.data[3] == 'Y' &&
		characters->data.data[4] == 'F',
		"native interior replacement retains prefix and current suffix");
	partial_check(
		first.offset == 2 &&
		first.count == 3 &&
		first.added == 2 &&
		first.committed_length == 7 &&
		first.first == 'A' &&
		first.last == 'H',
		"optional observer sees complete committed characters and exact interval");
	status = partial_expect(realm,
				"partialPoints()==='0:0,2:2,2:2,2:2,5:5,7:7,2'",
				"replacement repairs before equal interior removed-end and following native points");
	if (status != 0)
		goto cleanup;

	/* Pure deletion retains buffer identity while shifting the native suffix. */
	sequence = 0;
	status = partial_script(realm, "partialReset('ABCDEFGH');", &answer);
	if (status != 0)
		goto cleanup;
	storage = characters->data.data;
	sequence = 0;
	status = dom_text_replace(text, 2, 3, NULL, 0);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.data == storage &&
		characters->data.length == 5 &&
		characters->data.data[2] == 'F' &&
		characters->data.data[4] == 'H',
		"interior deletion is allocation-free and keeps shifted suffix");
	status = partial_expect(realm,
				"partialPoints()==='0:0,2:2,2:2,2:2,3:3,5:5,2'",
				"deletion clamps removed interval and shifts strictly following live points");
	if (status != 0)
		goto cleanup;

	/* Prefix deletion supplies the end-container primitive required by Range extraction. */
	sequence = 0;
	status = partial_script(realm, "partialReset('ABCDEFGH');", &answer);
	if (status != 0)
		goto cleanup;
	storage = characters->data.data;
	sequence = 0;
	status = dom_text_replace(text, 0, 3, NULL, 0);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.data == storage &&
		characters->data.length == 5 &&
		characters->data.data[0] == 'D' &&
		characters->data.data[4] == 'H',
		"prefix deletion keeps exact retained data and allocation-free storage");
	status = partial_expect(realm,
				"partialPoints()==='0:0,0:0,0:0,2:2,3:3,5:5,2'",
				"prefix deletion repairs actual independent and detached-clone endpoints");
	if (status != 0)
		goto cleanup;

	/* Oversized counts clamp before addition and delete only the remaining suffix. */
	sequence = 0;
	status = partial_script(realm, "partialReset('ABCDEFGH');", &answer);
	if (status != 0)
		goto cleanup;
	sequence = 0;
	status = dom_text_replace(text, 5, SIZE_MAX, NULL, 0);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.length == 5 &&
		first.count == 3 &&
		first.offset == 5,
		"SIZE_MAX count clamps safely to actual suffix without arithmetic overflow");
	status = partial_expect(realm,
				"partialPoints()==='0:0,2:2,3:3,5:5,5:5,5:5,5'",
				"clamped suffix deletion retains equal point and clamps following ones");
	if (status != 0)
		goto cleanup;

	/* Zero-count insertion shifts only strictly following native boundary positions. */
	sequence = 0;
	status = partial_script(realm, "partialReset('ABCDEFGH');", &answer);
	if (status != 0)
		goto cleanup;
	sequence = 0;
	status = dom_text_replace(text, 2, 0, units, 2);
	if (status != 0)
		goto cleanup;
	status = partial_expect(realm,
				"partialText.data==='ABXYCDEFGH'&&partialPoints()==='0:0,2:2,5:5,7:7,8:8,10:10,7'",
				"native insertion leaves equal offset unchanged and shifts all following points");
	if (status != 0)
		goto cleanup;

	/* Aliased replacement input stays readable until complete independent data publication. */
	sequence = 0;
	status = partial_script(realm, "partialReset('ABCDEFGH');", &answer);
	if (status != 0)
		goto cleanup;
	sequence = 0;
	status = dom_text_replace(text, 2, 3, characters->data.data + 1, 4);
	if (status != 0)
		goto cleanup;
	status = partial_expect(realm,
				"partialText.data==='ABBCDEFGH'&&partialPoints()==='0:0,2:2,2:2,2:2,7:7,9:9,2'",
				"aliased replacement copies exact original units and repairs native offsets");
	if (status != 0)
		goto cleanup;

	/* Invalid inputs and checked extreme sizes cannot change an already nonzero interval. */
	generation = owner->generation;
	notifications = first.data;
	storage = characters->data.data;
	status = dom_text_replace(text, 10, 0, units, 2);
	partial_check(status == EINVAL, "invalid native offset fails before mutation");
	status = dom_text_replace(text, 2, 0, NULL, 1);
	partial_check(status == EINVAL, "missing nonempty native units rejected");
	status = dom_text_replace(NULL, 0, 0, NULL, 0);
	partial_check(status == EINVAL, "null native node rejected");
	status = dom_text_replace(text->parent, 0, 0, units, 2);
	partial_check(status == EINVAL, "non CharacterData native kind rejected");
	status = dom_text_replace(text, 2, 0, units, SIZE_MAX);
	partial_check(status == ENOMEM, "final unit addition overflow rejected before input read");
	status = dom_text_replace(text, 0, SIZE_MAX, units, SIZE_MAX / sizeof(uint16_t) + 1U);
	partial_check(status == ENOMEM, "exact native byte multiplication overflow rejected before input read");
	partial_check(
		owner->generation == generation &&
		first.data == notifications &&
		characters->data.data == storage &&
		characters->data.length == 9,
		"all rejected operations preserve storage length generation and notifications");
	status = partial_expect(realm,
				"partialText.data==='ABBCDEFGH'&&partialPoints()==='0:0,2:2,2:2,2:2,7:7,9:9,2'",
				"rejected operations preserve exact data and nonzero actual endpoints");
	if (status != 0)
		goto cleanup;

	/* Full deletion keeps allocated storage and clamps every actual point to zero. */
	sequence = 0;
	status = dom_text_replace(text, 0, SIZE_MAX, NULL, 0);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.data == storage &&
		characters->data.length == 0,
		"full deletion retains allocation while committing empty data");
	status = partial_expect(realm,
				"partialPoints()==='0:0,0:0,0:0,0:0,0:0,0:0,0'",
				"full native deletion clamps every independent range to zero");
	if (status != 0)
		goto cleanup;

	/* Empty deletion is valid and still reports its exact zero interval. */
	sequence = 0;
	status = dom_text_replace(text, 0, SIZE_MAX, NULL, 0);
	if (status != 0)
		goto cleanup;
	partial_check(
		first.offset == 0 &&
		first.count == 0 &&
		first.added == 0 &&
		characters->data.length == 0,
		"empty native deletion has no NULL buffer arithmetic");

	/* Exact surrogate units are preserved by insertion into an empty retained buffer. */
	sequence = 0;
	status = dom_text_replace(text, 0, 0, unicode, 2);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.length == 2 &&
		characters->data.data[0] == 0xd800 &&
		characters->data.data[1] == 0xd800,
		"native replacement preserves exact unpaired UTF16 units");
	sequence = 0;
	status = dom_text_replace(text, 2, 0, units, 2);
	if (status != 0)
		goto cleanup;
	partial_check(
		characters->data.length == 4 &&
		characters->data.data[1] == 0xd800 &&
		characters->data.data[3] == 'Y',
		"end insertion retains complete existing UTF16 suffix identity");

	/* Actual adoption migrates data subscriptions according to their associated native root. */
	sequence = 0;
	status = partial_script(realm,
				"partialTarget.appendChild(partialText);partialReset('ABCDEFGH');",
				&answer);
	if (status != 0)
		goto cleanup;
	partial_check(text->document != owner, "real adoption changes current native data owner");
	sequence = 0;
	status = dom_text_replace(text, 2, 3, units, 2);
	if (status != 0)
		goto cleanup;
	status = partial_expect(realm,
				"partialText.ownerDocument===document&&partialPoints()==='0:0,2:2,2:2,2:2,5:5,7:7,2'",
				"adopted current-owner registry repairs all genuine Range and clone points");
	if (status != 0)
		goto cleanup;
	partial_check(
		first.committed_length == 7 &&
		second.committed_length == 7,
		"weak optional observers migrate and see adopted complete data");

	/* Comments use the same partial data algorithm and native live boundary repair. */
	status = partial_script(realm,
				"var partialComment=partialDoc.createComment('comment');"
				"partialDoc.documentElement.appendChild(partialComment);"
				"var partialCommentRange=partialDoc.createRange();"
				"partialCommentRange.setStart(partialComment,1);"
				"partialCommentRange.setEnd(partialComment,6);partialComment;",
				&answer);
	if (status != 0)
		goto cleanup;
	comment = bind_node_of(answer);
	if (comment == NULL || comment->type != DOM_COMMENT) {
		status = EIO;
		goto cleanup;
	}

	/* Mutate the actual native Comment through its ordinary data helper. */
	status = dom_text_replace(comment, 0, 3, NULL, 0);
	if (status != 0)
		goto cleanup;
	status = partial_expect(realm,
				"partialComment.data==='ment'&&partialCommentRange.startOffset===0&&partialCommentRange.endOffset===3",
				"native Comment prefix deletion retains actual data and live endpoints");
	if (status != 0)
		goto cleanup;
	partial_check(generic.data == 0, "removal-only native observer receives no partial data delivery");

	/* Remove external script ownership before inspecting weak callback retention. */
	status = partial_script(realm,
				"partialText.remove();partialRanges=null;partialClone=null;partialText=null;"
				"partialDoc=null;partialComment=null;partialCommentRange=null;",
				&answer);
	if (status != 0)
		goto cleanup;
	vm_heap_collect(realm->heap);
	found = vm_heap_find_cell(realm->heap, creator_address);
	partial_check(found == NULL, "weak native data observers do not retain original creator");
	found = vm_heap_find_cell(realm->heap, text_address);
	partial_check(found == NULL, "weak native data observers do not retain released endpoint graph");

	/* All native mutation observations completed before token cleanup. */
	status = 0;

cleanup:
	/* Token cleanup remains valid even when its original Document has already finalized. */
	vm_heap_set_stack_base(realm->heap, stack_base);
	dom_removal_unsubscribe(generic_token);
	dom_removal_unsubscribe(second_token);
	dom_removal_unsubscribe(first_token);
	if (status != 0)
		return status;

	/* Succeeded: checked native partial mutation and weak ownership are verified. */
	return 0;
}

/* Compares genuine script observations after the native helper has repaired private state. */
static int
partial_expect(
	struct vm_realm *realm,
	const char *expression,
	const char *name)
{
	vm_value answer;
	int status;

	/* The regular interpreter observes native getters without a production test branch. */
	status = partial_script(realm, expression, &answer);
	if (status != 0)
		return status;
	partial_check(answer == VM_VALUE_TRUE, name);

	/* Succeeded: the exact observation contributes to the fixture's failure count. */
	return 0;
}
