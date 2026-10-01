/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Realms (plan/ws074/design.md §11.7): the global object and the
 * intrinsic objects every object of a script is made from.
 *
 * The first pass makes the skeleton: Object.prototype (the end of every
 * chain), Function.prototype and Array.prototype, and a global object
 * with globalThis.  The constructors and their methods arrive with the
 * built-ins (ws074-p026).  A realm also keeps the queue of microtasks its
 * embedder's checkpoints run (ws074-p030).
 */

#include "vm/internal.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* How many 64-bit slots the VM stack of a realm has (2 MiB). */
#define REALM_STACK_SLOTS	(256U * 1024U)

static int realm_fill(struct vm_realm *realm);
static void realm_trace(struct vm_heap *heap, void *context);
static int realm_empty_function(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int realm_define_value(struct vm_realm *realm, const char *name, vm_value value);
static void realm_report_rejections(struct vm_realm *realm, vm_job_report report, void *context);
static int realm_make_symbols(struct vm_realm *realm);

/* The descriptions of the well-known symbols, in the order of enum vm_well_known. */
static const char *const realm_symbol_names[VM_SYMBOLS] = {
	"Symbol.asyncIterator",
	"Symbol.hasInstance",
	"Symbol.isConcatSpreadable",
	"Symbol.iterator",
	"Symbol.match",
	"Symbol.matchAll",
	"Symbol.replace",
	"Symbol.search",
	"Symbol.species",
	"Symbol.split",
	"Symbol.toPrimitive",
	"Symbol.toStringTag",
	"Symbol.unscopables"
};

/*
 * Makes a realm in a heap with its intrinsic objects and global object.
 */
int
vm_realm_create(
	struct vm_heap *heap,
	struct vm_realm **realm)
{
	struct vm_realm *made;
	int error;

	/* The realm, kept by a tracer the heap calls in every collection. */
	*realm = NULL;
	made = calloc(1, sizeof(*made));
	if (made == NULL)
		return ENOMEM;
	made->heap = heap;
	made->exception = VM_VALUE_UNDEFINED;
	made->throw_value = VM_VALUE_UNDEFINED;
	made->callee = VM_VALUE_UNDEFINED;
	made->new_target = VM_VALUE_UNDEFINED;

	/* The VM stack its code runs on. */
	made->stack = calloc(REALM_STACK_SLOTS, sizeof(vm_value));
	if (made->stack == NULL) {
		free(made);
		return ENOMEM;
	}

	/* The stack's size, empty. */
	made->stack_capacity = REALM_STACK_SLOTS;
	wb_vector_init(&made->jobs, sizeof(struct vm_job));
	wb_vector_init(&made->rejections, sizeof(vm_value));

	/* The tracer. */
	error = vm_heap_add_tracer(heap, realm_trace, made);
	if (error != 0) {
		free(made->stack);
		free(made);
		return error;
	}

	/* Its objects. */
	error = realm_fill(made);
	if (error != 0) {
		vm_realm_destroy(made);
		return error;
	}

	/* Succeeded: the realm. */
	*realm = made;
	return 0;
}

/*
 * Lets a realm's objects go (they are freed by the next collection that
 * finds nothing else holding them) and frees the realm.
 */
void
vm_realm_destroy(
	struct vm_realm *realm)
{
	/* Nothing to destroy. */
	if (realm == NULL)
		return;

	/* The tracer, then the stack and the realm. */
	vm_heap_remove_tracer(realm->heap, realm_trace, realm);
	wb_vector_release(&realm->jobs);
	wb_vector_release(&realm->rejections);
	free(realm->stack);
	free(realm);
}

/*
 * Adds a microtask to the end of a realm's queue: a call of callback with
 * one argument at the next checkpoint.
 */
int
vm_enqueue_job(
	struct vm_realm *realm,
	vm_value callback,
	vm_value argument)
{
	struct vm_job job;
	int error;

	/* The job goes to the end of the queue, which the realm's tracer keeps alive. */
	job.callback = callback;
	job.argument = argument;
	error = wb_vector_push(&realm->jobs, &job);
	if (error != 0)
		return error;

	/* Succeeded: the job waits for the next checkpoint. */
	return 0;
}

/*
 * Runs a microtask checkpoint: every queued job in order, including the
 * jobs the jobs queue, until the queue is empty.
 *
 * A job that throws is reported to report (with context) and the rest
 * still run.  Returns 0, or ENOMEM when a job ran out of memory.
 */
int
vm_run_jobs(
	struct vm_realm *realm,
	vm_job_report report,
	void *context)
{
	struct vm_job *queued;
	struct vm_job job;
	vm_value ignored;
	struct vm_cell *cell;
	size_t next;
	int is_promise_job;
	int is_cell;
	int status;

	/* Takes the jobs from the front while there are any (a job may queue more at the end). */
	next = 0;
	while (next < realm->jobs.count) {
		queued = wb_vector_at(&realm->jobs, next);
		job = *queued;
		next++;

		/* A promise's job (ws074-p086) is a cell of its own type rather than a function. */
		is_promise_job = 0;
		is_cell = vm_value_is_cell(job.callback);
		if (is_cell) {
			cell = vm_value_as_cell(job.callback);
			if (cell->type == &vm_promise_job_type)
				is_promise_job = 1;
		}

		/* Runs the job: a promise's, or a function to call. */
		if (is_promise_job) {
			status = vm_promise_run_job(realm, job.callback, job.argument);
		} else {
			status = vm_call(realm, job.callback, VM_VALUE_UNDEFINED, &job.argument, 1, &ignored);
		}

		/* An exception is reported and cleared; any other failure ends the checkpoint. */
		if (status == VM_THROWN) {
			if (report != NULL)
				report(realm, realm->exception, context);
			realm->exception = VM_VALUE_UNDEFINED;
		} else if (status != 0) {
			wb_vector_clear(&realm->jobs);
			return status;
		}
	}

	/* The queue is empty; the rejections nothing handled are reported. */
	wb_vector_clear(&realm->jobs);
	realm_report_rejections(realm, report, context);

	/* Succeeded: the queue is empty. */
	return 0;
}

/*
 * Reports a well-known symbol of a realm as a property key.
 */
vm_value
vm_symbol_key(
	const struct vm_realm *realm,
	int which)
{
	/* The symbol's value. */
	return vm_value_cell(realm->symbols[which]);
}

/* Makes the intrinsic objects and the global object, each held by the realm as soon as it is made. */
static int
realm_fill(
	struct vm_realm *realm)
{
	struct vm_function *prototype_function;
	vm_value key;
	int error;

	/* Object.prototype: the end of every prototype chain. */
	realm->object_prototype = vm_object_create(realm->heap, NULL);
	if (realm->object_prototype == NULL)
		return ENOMEM;

	/*
	 * Function.prototype: itself a function (that returns undefined) whose
	 * prototype is Object.prototype.  It is made while the realm's
	 * function_prototype is still NULL, then given its prototype.
	 */
	prototype_function = vm_function_create_native(realm, "", 0, realm_empty_function);
	if (prototype_function == NULL)
		return ENOMEM;
	prototype_function->object.prototype = realm->object_prototype;
	realm->function_prototype = &prototype_function->object;

	/* Array.prototype: an array whose prototype is Object.prototype. */
	realm->array_prototype = vm_array_create(realm->heap, realm->object_prototype);
	if (realm->array_prototype == NULL)
		return ENOMEM;

	/* The record of the scripts' top-level let and const, which inherits nothing. */
	realm->lexicals = vm_object_create(realm->heap, NULL);
	if (realm->lexicals == NULL)
		return ENOMEM;

	/* The global object, and globalThis on it. */
	realm->global = vm_object_create(realm->heap, realm->object_prototype);
	if (realm->global == NULL)
		return ENOMEM;
	key = vm_key_from_ascii(realm->heap, "globalThis");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_object_define(realm->heap, realm->global, key, vm_value_cell(realm->global),
	    VM_PROPERTY_WRITABLE | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* The well-known symbols. */
	error = realm_make_symbols(realm);
	if (error != 0)
		return error;

	/* The global values: undefined, NaN and Infinity, which cannot be changed. */
	error = realm_define_value(realm, "undefined", VM_VALUE_UNDEFINED);
	if (error != 0)
		return error;
	error = realm_define_value(realm, "NaN", vm_value_double(NAN));
	if (error != 0)
		return error;
	error = realm_define_value(realm, "Infinity", vm_value_double(INFINITY));
	if (error != 0)
		return error;

	/* Succeeded: the realm has its objects. */
	return 0;
}

/* Marks a realm's objects and its exception. */
static void
realm_trace(
	struct vm_heap *heap,
	void *context)
{
	struct vm_realm *realm;
	struct vm_job *job;
	uint32_t slot;
	uint32_t index;

	/* The intrinsic objects, where made. */
	realm = context;
	if (realm->object_prototype != NULL)
		vm_heap_mark(heap, &realm->object_prototype->cell);
	if (realm->function_prototype != NULL)
		vm_heap_mark(heap, &realm->function_prototype->cell);
	if (realm->array_prototype != NULL)
		vm_heap_mark(heap, &realm->array_prototype->cell);

	/* The other intrinsic objects the built-ins made. */
	for (index = 0; index < VM_INTRINSICS; index++) {
		if (realm->intrinsics[index] != NULL)
			vm_heap_mark(heap, &realm->intrinsics[index]->cell);
	}

	/* The well-known symbols. */
	for (index = 0; index < VM_SYMBOLS; index++) {
		if (realm->symbols[index] != NULL)
			vm_heap_mark(heap, &realm->symbols[index]->cell);
	}

	/* The global object, the exception being thrown, and the native call's callee and new.target. */
	if (realm->global != NULL)
		vm_heap_mark(heap, &realm->global->cell);
	vm_heap_mark_value(heap, realm->exception);
	vm_heap_mark_value(heap, realm->throw_value);
	if (realm->lexicals != NULL)
		vm_heap_mark(heap, &realm->lexicals->cell);
	vm_heap_mark_value(heap, realm->callee);
	vm_heap_mark_value(heap, realm->new_target);

	/* The microtasks waiting for a checkpoint. */
	for (index = 0; index < realm->jobs.count; index++) {
		job = wb_vector_at(&realm->jobs, index);
		vm_heap_mark_value(heap, job->callback);
		vm_heap_mark_value(heap, job->argument);
	}

	/* The promises rejected while unhandled, waiting for the checkpoint's report. */
	vm_promise_trace_rejections(heap, realm);

	/* Every word of the used stack that could point at a cell (boxed and raw values share it). */
	for (slot = 0; slot < realm->stack_top; slot++)
		vm_heap_mark_word(heap, (uintptr_t)realm->stack[slot]);
}

/* Function.prototype's own behaviour: it takes anything and returns undefined. */
static int
realm_empty_function(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Succeeded: undefined. */
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Defines a global value that is neither writable, enumerable nor configurable. */
static int
realm_define_value(
	struct vm_realm *realm,
	const char *name,
	vm_value value)
{
	vm_value key;
	int error;

	/* The name's key. */
	key = vm_key_from_ascii(realm->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;

	/* The property. */
	error = vm_object_define(realm->heap, realm->global, key, value, 0);
	if (error != 0)
		return error;

	/* Succeeded: the value is defined. */
	return 0;
}

/*
 * Reports each promise of the realm's list of rejections that is still
 * unhandled (a rejection nothing handled by the end of the checkpoint),
 * with reporting_rejection set so the embedder writes it as a promise's,
 * and empties the list.
 */
static void
realm_report_rejections(
	struct vm_realm *realm,
	vm_job_report report,
	void *context)
{
	struct vm_promise *object;
	vm_value *promise;
	vm_value reason;
	size_t index;
	int state;

	/* Each promise, in the order it was rejected (a report runs no script, so the list does not change). */
	for (index = 0; index < realm->rejections.count; index++) {
		promise = wb_vector_at(&realm->rejections, index);
		object = (struct vm_promise *)vm_value_as_cell(*promise);
		if (object->handled)
			continue;

		/* The reason, reported as a promise's. */
		state = vm_promise_state(*promise, &reason);
		if (report == NULL || state != (int)VM_PROMISE_REJECTED)
			continue;
		realm->reporting_rejection = 1;
		report(realm, reason, context);
		realm->reporting_rejection = 0;
	}

	/* The list is spent. */
	wb_vector_clear(&realm->rejections);
}

/* Makes the well-known symbols of a realm, each with its description. */
static int
realm_make_symbols(
	struct vm_realm *realm)
{
	struct vm_string *description;
	int index;

	/* Each symbol, held by the realm as soon as it is made. */
	for (index = 0; index < (int)VM_SYMBOLS; index++) {
		description = vm_string_from_utf8(realm->heap, realm_symbol_names[index], strlen(realm_symbol_names[index]));
		if (description == NULL)
			return ENOMEM;
		realm->symbols[index] = vm_symbol_create(realm->heap, vm_value_cell(description));
		if (realm->symbols[index] == NULL)
			return ENOMEM;
	}

	/* Succeeded: the realm has its symbols. */
	return 0;
}
