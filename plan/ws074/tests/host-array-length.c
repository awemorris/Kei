/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies that huge array metadata never implies a huge dense allocation. */

#include "js/js.h"

#include <stdio.h>

/*
 * Checks large logical lengths with the actual production sparse object storage.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_object *array;
	vm_value length_key;
	struct vm_property property;
	double number;
	int found;
	int error;
	int printed;

	/* A bounded VM heap holds the array while the normal C stack is scanned. */
	error = vm_heap_create(&heap, 1024U * 1024U);
	if (error != 0)
		return 2;

	/* Construction must not depend on a special test allocator switch. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	array = vm_array_create(heap, NULL);
	if (array == NULL) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Names the ordinary numeric length slot before observing metadata growth. */
	length_key = vm_key_from_ascii(heap, "length");
	if (length_key == VM_VALUE_EMPTY) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* A full uint32 length is stored as Number without allocating holes. */
	error = vm_object_define(heap, array, length_key, vm_value_number(4294967295.0), VM_PROPERTY_WRITABLE);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* The public length property keeps the full unsigned number rather than a signed wrap. */
	found = vm_object_get_own(array, length_key, &property);
	if (!found) {
		vm_heap_destroy(heap);
		return 1;
	}

	/* Numeric length metadata and its property must describe the same array. */
	number = vm_value_as_number(*property.value);
	if (number != 4294967295.0) {
		vm_heap_destroy(heap);
		return 1;
	}

	/* Logical growth must leave dense capacity bounded by actual stored elements. */
	if (array->length != 4294967295U || array->element_capacity > 1024U) {
		vm_heap_destroy(heap);
		return 1;
	}

	/* A supported int32 index far from existing storage belongs in a sparse shape slot. */
	error = vm_object_define(heap, array, vm_value_int32(2000000000), VM_VALUE_TRUE, VM_PROPERTY_DEFAULT);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Reads the sparse value without iteration over the array's logical holes. */
	found = vm_object_get_own(array, vm_value_int32(2000000000), &property);
	if (!found ||
	    *property.value != VM_VALUE_TRUE ||
	    array->element_capacity > 1024U) {
		vm_heap_destroy(heap);
		return 1;
	}

	/* Contracting metadata visits only existing dense capacity and sparse keys. */
	error = vm_array_set_length(heap, array, 0U);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* The far sparse property no longer survives the shortened array. */
	found = vm_object_get_own(array, vm_value_int32(2000000000), &property);
	if (found || array->length != 0U) {
		vm_heap_destroy(heap);
		return 1;
	}

	/* Reclaims the normal object graph and its C storage. */
	vm_heap_destroy(heap);
	printed = printf("array length capacity checks: 4/4 passed\n");
	if (printed < 0)
		return 2;

	/* Succeeded: unsigned metadata and sparse storage stayed bounded. */
	return 0;
}
