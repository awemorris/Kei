/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * A retained function must remain safe to trace after its explicitly owned
 * primary realm is destroyed, even though calling it is no longer supported.
 */

#include "vm/vm.h"

#include <stdio.h>

static int retirement_native(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);

/*
 * Verifies GC tracing of cells retained after primary realm destruction.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct vm_function *function;
	struct vm_cell *root;
	int error;
	int printed;

	/* Constructs the function under ordinary conservative stack protection. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;

	/* The ordinary stack keeps construction values alive until the explicit root exists. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));

	/* The explicitly owned realm supplies the native function's creator during setup. */
	error = vm_realm_create(heap, &realm);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* The heap root retains a function cell, not the explicitly owned C realm. */
	function = vm_function_create_native(realm, "retained", 0, retirement_native);
	if (function == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Registration precedes destruction and contains no dangling slot pointer. */
	root = &function->object.cell;
	error = vm_heap_add_root(heap, &root);
	if (error != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Collection may trace retained cells but must not read the freed C realm. */
	vm_realm_destroy(realm);
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	vm_heap_remove_root(heap, &root);
	vm_heap_destroy(heap);

	/* A sanitizer failure terminates before this successful outcome is printed. */
	printed = printf("retired primary realm tracing: PASS\n");
	if (printed < 0)
		return 2;

	/* Succeeded: tracing and heap cleanup did not access the retired realm. */
	return 0;
}

/* Supplies a native function that is retained but never invoked after retirement. */
static int
retirement_native(
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

	/* The test verifies reachability rather than post-retirement execution. */
	*result = VM_VALUE_UNDEFINED;

	/* Succeeded: an ordinary native completion is available during setup. */
	return 0;
}
