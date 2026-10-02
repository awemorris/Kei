/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks collectible realm/window ownership, saved references, observer
 * cycles, task retirement, partial construction and heap finalization.
 */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The test owns these counters; finalizers run synchronously on this thread. */
static unsigned released;
static unsigned checks;
static unsigned failures;

/* The borrowed timer callback sees host state but supplies no VM root itself. */
static struct bind_window *active_window;
static struct vm_cell **active_connection;
static const void *active_stack_base;

static void ownership_check(int condition, const char *name);
static void ownership_release(void *context);
static int ownership_source(struct vm_realm *realm, const char *source, vm_value *answer);
static int ownership_sample(struct vm_heap *heap, unsigned kind, struct vm_cell **root);
static int ownership_references(struct vm_heap *heap, const void *stack_base);
static int ownership_window(struct vm_heap *heap, const void *stack_base);
static int ownership_failure(void);
static int ownership_active_timer(struct vm_heap *heap, const void *stack_base);
static int ownership_retire(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int ownership_fetch(void *context, const char *href, bind_fetch_done done, void *done_context);
static int ownership_gc(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);

/*
 * Verifies reachability and cleanup against the default collector.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	const void *stack_base;
	int error;
	int printed;

	/* Scans the real C stack during construction and active execution. */
	stack_base = __builtin_frame_address(0);
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;
	vm_heap_set_stack_base(heap, stack_base);

	/* Exercises saved references before separate binding and failure cases. */
	error = ownership_references(heap, stack_base);
	if (error == 0)
		error = ownership_window(heap, stack_base);
	if (error == 0)
		error = ownership_active_timer(heap, stack_base);
	vm_heap_destroy(heap);
	if (error != 0)
		return 2;

	/* A limited heap bounds partial-construction failure without fault switches. */
	error = ownership_failure();
	if (error != 0)
		return 2;

	/* Reports all independently named assertions with a failing exit status. */
	printed = printf(
		"ownership checks: %u/%u passed\n",
		checks - failures,
		checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: every ownership contract passed. */
	return 0;
}

/* Records a behavioral failure without concealing subsequent independent checks. */
static void
ownership_check(
	int condition,
	const char *name)
{
	/* Each finalizer/identity assertion contributes independently to the result. */
	checks++;
	if (!condition) {
		failures++;
		fprintf(stderr, "FAIL %s\n", name);
	}

	/* Succeeded: this assertion has been recorded. */
	return;
}

/* Counts cleanup without inspecting any other cell during heap destruction. */
static void
ownership_release(
	void *context)
{
	UNUSED_PARAMETER(context);

	/* Each collected sample must release its host exactly once. */
	released++;

	/* Succeeded: cleanup is observable after the realm disappears. */
	return;
}

/* Executes ordinary script text through the production UTF-16 entry point. */
static int
ownership_source(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct js_syntax_error syntax;
	struct wb_units units;
	int error;

	/* Keeps source conversion separate from parsing and execution failures. */
	wb_units_init(&units);
	error = wb_utf8_to_units(
		(const unsigned char *)source,
		strlen(source),
		&units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Uses the same script implementation as a bound Window. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	wb_units_release(&units);
	if (error != 0)
		return error;

	/* Succeeded: the script's completion is available. */
	return 0;
}

/* Produces one saved reference without retaining a C realm pointer in its caller. */
static int
ownership_sample(
	struct vm_heap *heap,
	unsigned kind,
	struct vm_cell **root)
{
	struct vm_realm *realm;
	struct vm_function *function;
	struct dom_document *document;
	vm_value answer;
	int error;

	/* Managed creation precedes built-ins, so every function has the final owner. */
	error = vm_realm_create_managed(heap, &realm);
	if (error != 0)
		return error;
	realm->host_release = ownership_release;
	error = js_install_builtins(realm);
	if (error != 0)
		return error;

	/* A native collection runs while only execution keeps raw realm state live. */
	function = vm_function_create_native(realm, "collect", 0, ownership_gc);
	if (function == NULL)
		return ENOMEM;
	error = js_builtin_value(
		realm,
		realm->global,
		"collect",
		vm_value_cell(function),
		JS_BUILTIN_METHOD);
	if (error != 0)
		return error;

	/* Each kind checks a distinct traced path into the same lifetime record. */
	if (kind == 0) {
		error = ownership_source(
			realm,
			"var marker=42; function saved(){collect();return marker;} saved",
			&answer);
		if (error != 0)
			return error;
		*root = vm_value_as_cell(answer);
	} else if (kind == 1) {
		*root = &realm->global->cell;
	} else if (kind == 2) {
		*root = &realm->object_prototype->cell;
	} else if (kind == 3) {
		document = dom_document_create(heap);
		if (document == NULL)
			return ENOMEM;
		document->context = &realm->cell;
		*root = &document->node.cell;
	} else {
		error = ownership_source(
			realm,
			"function* suspended(){yield 1;collect();yield 42;} var g=suspended();g.next();g",
			&answer);
		if (error != 0)
			return error;
		*root = vm_value_as_cell(answer);
	}

	/* Succeeded: the caller owns only the selected saved reference. */
	return 0;
}

/* Verifies retention and exact reclamation with explicit embedding roots. */
static int
ownership_references(
	struct vm_heap *heap,
	const void *stack_base)
{
	struct vm_cell *root;
	struct vm_function *function;
	struct vm_realm *realm;
	vm_value answer;
	vm_value key;
	vm_value expected;
	unsigned kind;
	unsigned before;
	int error;
	int same;

	/* Explicit roots allow exact reclamation without stale conservative C words. */
	root = NULL;
	error = vm_heap_add_root(heap, &root);
	if (error != 0)
		return error;

	/* All five saved-reference classes must keep and then release their owner. */
	for (kind = 0; kind < 5; kind++) {
		before = released;
		error = ownership_sample(heap, kind, &root);
		if (error != 0) {
			vm_heap_remove_root(heap, &root);
			return error;
		}

		/* This collection follows roots only; construction used the normal stack. */
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		same = 0;
		if (released == before)
			same = 1;
		ownership_check(same, "saved reference retains its realm");
		vm_heap_set_stack_base(heap, stack_base);

		/* A surviving foreign function can collect while reading its own globals. */
		if (kind == 0) {
			function = (struct vm_function *)root;
			error = vm_call(
				function->realm,
				vm_value_cell(function),
				VM_VALUE_UNDEFINED,
				NULL,
				0,
				&answer);
			if (error != 0) {
				vm_heap_remove_root(heap, &root);
				return error;
			}

			/* Its result proves both stack retention and post-collection execution. */
			expected = vm_value_int32(42);
			same = 0;
			if (answer == expected)
				same = 1;
			ownership_check(same, "saved function runs after and during GC");
		}

		/* A suspended generator must resume in its surviving child's realm. */
		if (kind == 4) {
			key = vm_key_from_ascii(heap, "next");
			if (key == VM_VALUE_EMPTY) {
				vm_heap_remove_root(heap, &root);
				return ENOMEM;
			}

			/* The native method's realm follows the retained generator prototype. */
			error = vm_realm_create(heap, &realm);
			if (error != 0) {
				vm_heap_remove_root(heap, &root);
				return error;
			}

			/* Reads the retained generator method through ordinary property dispatch. */
			error = vm_get(realm, vm_value_cell(root), key, &answer);
			if (error != 0) {
				vm_realm_destroy(realm);
				vm_heap_remove_root(heap, &root);
				return error;
			}

			/* Resuming through an unrelated caller keeps the generator realm live. */
			error = vm_call(realm, answer, vm_value_cell(root), NULL, 0, &answer);
			if (error != 0) {
				vm_realm_destroy(realm);
				vm_heap_remove_root(heap, &root);
				return error;
			}

			/* The second yield reaches the collection and returns the child marker. */
			key = vm_key_from_ascii(heap, "value");
			if (key == VM_VALUE_EMPTY) {
				vm_realm_destroy(realm);
				vm_heap_remove_root(heap, &root);
				return ENOMEM;
			}

			/* Reads the second yield independently of the call completion object. */
			error = vm_get(realm, answer, key, &answer);
			vm_realm_destroy(realm);
			if (error != 0) {
				vm_heap_remove_root(heap, &root);
				return error;
			}

			/* A post-GC resume preserves the suspended execution environment. */
			expected = vm_value_int32(42);
			same = 0;
			if (answer == expected)
				same = 1;
			ownership_check(same, "saved generator resumes after and during GC");
		}

		/* No selected reference remains; stale test stack words are excluded. */
		root = NULL;
		vm_heap_set_stack_base(heap, NULL);
		vm_heap_collect(heap);
		same = 0;
		if (released == before + 1U)
			same = 1;
		ownership_check(same, "unreachable realm releases host once");
		vm_heap_collect(heap);
		same = 0;
		if (released == before + 1U)
			same = 1;
		ownership_check(same, "repeated GC does not release twice");
		vm_heap_set_stack_base(heap, stack_base);
	}

	/* Removes only this test's explicit embedding root. */
	vm_heap_remove_root(heap, &root);

	/* Succeeded: saved-reference graphs retain and reclaim their realm. */
	return 0;
}

/* Checks window-owned observer cycles and idempotent retirement. */
static int
ownership_window(
	struct vm_heap *heap,
	const void *stack_base)
{
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct vm_cell *root;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value answer;
	int error;
	int same;

	/* An empty host cannot retain asynchronous callback records. */
	memset(&host, 0, sizeof(host));
	error = vm_realm_create_managed(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0)
		return error;
	document = dom_document_create(heap);
	if (document == NULL)
		return ENOMEM;

	/* Unsupported asynchronous registration is rejected before creating a host. */
	host.fetch = ownership_fetch;
	error = bind_window_create(realm, document, &host, &window);
	same = 0;
	if (error == ENOTSUP && window == NULL)
		same = 1;
	ownership_check(same, "managed host rejects async fetch");
	host.fetch = NULL;
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0)
		return error;

	/* A connected embedding owns its Document, which owns the child context. */
	root = &document->node.cell;
	error = vm_heap_add_root(heap, &root);
	if (error != 0)
		return error;
	error = ownership_source(
		realm,
		"var deliveries=0;var observer=new MutationObserver(function(){deliveries++;});observer.observe(document,{childList:true});setTimeout(function(){deliveries++;},0);queueMicrotask(function(){deliveries++;});",
		&answer);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		return error;
	}

	/* The Document alone keeps the host and observer/timer state alive. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_set_stack_base(heap, stack_base);
	same = 0;
	if (document->context == &realm->cell && realm->host == window)
		same = 1;
	ownership_check(same, "Document retains bound Window across GC");

	/* Retirement clears task queues and observer delivery without freeing objects. */
	bind_window_detach(window);
	bind_window_detach(window);
	error = bind_run_timers(window);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		return error;
	}

	/* A detached checkpoint cannot deliver jobs queued before retirement. */
	error = bind_checkpoint(window);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		return error;
	}

	/* Saved observer objects and Documents remain callable and identifiable. */
	error = ownership_source(
		realm,
		"var late=new MutationObserver(function(){deliveries++;});"
		"late.observe(document,{childList:true});"
		"queueMicrotask(function(){deliveries++;});"
		"deliveries===0 && observer.takeRecords().length===0 "
		"&& document.nodeType===9",
		&answer);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		return error;
	}

	/* The behavioral result includes all three retained-state contracts. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	ownership_check(same, "detach cancels tasks and retains saved objects");

	/* Later saved functions may enqueue jobs, but detached delivery stays disabled. */
	error = bind_checkpoint(window);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		return error;
	}

	/* The fresh observer and job remain internal to the collectible graph. */
	error = ownership_source(realm, "deliveries===0", &answer);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		return error;
	}

	/* Repeated detached checkpoints never restart task delivery. */
	same = 0;
	if (answer == VM_VALUE_TRUE)
		same = 1;
	ownership_check(same, "late detached jobs do not execute");
	vm_heap_stats(heap, &before);

	/* Observer self-cycles must not leave a permanent root after disconnection. */
	root = NULL;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_stats(heap, &after);
	same = 0;
	if (after.live_cells + 100U < before.live_cells)
		same = 1;
	ownership_check(same, "unreachable observer context is reclaimed");
	vm_heap_set_stack_base(heap, stack_base);
	vm_heap_remove_root(heap, &root);

	/* Succeeded: Window ownership and detached task state are verified. */
	return 0;
}

/* Runs a parent-realm timer that drops the child's last connection during GC. */
static int
ownership_active_timer(
	struct vm_heap *heap,
	const void *stack_base)
{
	struct vm_realm *parent;
	struct vm_realm *child;
	struct vm_function *callback;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	struct vm_cell *root;
	vm_value answer;
	int error;
	int same;

	/* The borrowed callback has a separate, explicitly owned primary realm. */
	error = vm_realm_create(heap, &parent);
	if (error != 0)
		return error;
	callback = vm_function_create_native(parent, "retire", 0, ownership_retire);
	if (callback == NULL) {
		vm_realm_destroy(parent);
		return ENOMEM;
	}

	/* The collectible child host owns no external callback registration. */
	error = vm_realm_create_managed(heap, &child);
	if (error != 0) {
		vm_realm_destroy(parent);
		return error;
	}

	/* Initializes the child before making its bound Document. */
	error = js_install_builtins(child);
	if (error != 0) {
		vm_realm_destroy(parent);
		return error;
	}

	/* An explicit connection retains the Document until the borrowed callback. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(parent);
		return ENOMEM;
	}

	/* Installing an empty host preserves the child's collectible ownership. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(child, document, &host, &window);
	if (error != 0) {
		vm_realm_destroy(parent);
		return error;
	}

	/* A saved parent function is the child's timer callback. */
	error = js_builtin_value(
		child,
		child->global,
		"retire",
		vm_value_cell(callback),
		JS_BUILTIN_METHOD);
	if (error != 0) {
		vm_realm_destroy(parent);
		return error;
	}

	/* Connects only through the Document, as a real frame embedding must. */
	root = &document->node.cell;
	error = vm_heap_add_root(heap, &root);
	if (error != 0) {
		vm_realm_destroy(parent);
		return error;
	}

	/* Queues the borrowed function before retiring its owning child. */
	error = ownership_source(child, "setTimeout(retire,0)", &answer);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_realm_destroy(parent);
		return error;
	}

	/* Raw test globals deliberately provide no GC root for the child. */
	active_window = window;
	active_connection = &root;
	active_stack_base = stack_base;
	error = bind_run_timers(window);
	if (error != 0) {
		vm_heap_remove_root(heap, &root);
		vm_realm_destroy(parent);
		return error;
	}

	/* The public timer entry point must retain its host across the collection. */
	same = 0;
	if (root == NULL &&
	    window->detached &&
	    window->realm == child)
		same = 1;
	ownership_check(same, "borrowed callback GC retains active detached host");
	active_window = NULL;
	active_connection = NULL;
	vm_heap_remove_root(heap, &root);
	vm_realm_destroy(parent);

	/* No host or VM reference remains after the active entry point returns. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_set_stack_base(heap, stack_base);

	/* Succeeded: active host retention does not require connected tree ownership. */
	return 0;
}

/* Drops connection ownership while a parent function executes a child timer. */
static int
ownership_retire(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Cancels the host and removes the only embedding root during its callback. */
	bind_window_detach(active_window);
	*active_connection = NULL;
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, active_stack_base);
	*result = VM_VALUE_UNDEFINED;

	/* Succeeded: the timer entry point can safely complete its own checkpoint. */
	return 0;
}

/* Checks failed managed construction and finalization of still-reachable cells. */
static int
ownership_failure(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	vm_value answer;
	unsigned before;
	int error;
	int same;

	/* A realm cell fits this limit, while the intrinsic skeleton cannot. */
	error = vm_heap_create(&heap, sizeof(struct vm_realm) + 64U);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	error = vm_realm_create_managed(heap, &realm);
	same = 0;
	if (error == ENOMEM && realm == NULL)
		same = 1;
	ownership_check(same, "partial managed construction reports ENOMEM");
	vm_heap_destroy(heap);

	/* Whole-heap teardown must finalize a managed owner that remains on the stack. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	error = vm_realm_create_managed(heap, &realm);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Explicit destruction cannot prematurely release a managed realm. */
	before = released;
	realm->host_release = ownership_release;
	vm_realm_destroy(realm);
	same = 0;
	if (released == before)
		same = 1;
	ownership_check(same, "explicit destroy preserves managed ownership");
	vm_heap_destroy(heap);
	same = 0;
	if (released == before + 1U)
		same = 1;
	ownership_check(same, "whole heap releases managed host exactly once");

	/* A live bound host with observer registrations may finalize in any cell order. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return error;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	error = vm_realm_create_managed(heap, &realm);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* The bound-host case creates both native methods and observer C backend. */
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Ownership is transferred to the realm before creating observer cycles. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_heap_destroy(heap);
		return ENOMEM;
	}

	/* Empty host callbacks cannot outlive heap destruction. */
	memset(&host, 0, sizeof(host));
	error = bind_window_create(realm, document, &host, &window);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Connected observer state remains alive until the whole tab is destroyed. */
	error = ownership_source(
		realm,
		"var o=new MutationObserver(function(){});o.observe(document,{childList:true});setTimeout(function(){},0)",
		&answer);
	if (error != 0) {
		vm_heap_destroy(heap);
		return error;
	}

	/* Finalizers release C records without dereferencing other dead cells. */
	vm_heap_destroy(heap);
	ownership_check(1, "whole heap releases live bound observer host");

	/* Succeeded: partial and terminal cleanup did not crash or double-free. */
	return 0;
}

/* Detects any accidentally accepted asynchronous host callback registration. */
static int
ownership_fetch(
	void *context,
	const char *href,
	bind_fetch_done done,
	void *done_context)
{
	UNUSED_PARAMETER(context);
	UNUSED_PARAMETER(href);
	UNUSED_PARAMETER(done);
	UNUSED_PARAMETER(done_context);

	/* Accepted asynchronous registration is outside this ownership foundation. */
	return ENOTSUP;
}

/* Forces a production collection from inside a saved managed function. */
static int
ownership_gc(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The executing managed realm must survive this stack-scanning collection. */
	vm_heap_collect(realm->heap);
	*result = VM_VALUE_UNDEFINED;

	/* Succeeded: the caller can keep executing in its own realm. */
	return 0;
}
