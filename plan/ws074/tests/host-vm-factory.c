/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies VM object and Array factory ownership across actual allocation-triggered GC. */

#include "js/builtin.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Ordinary inert cells approach the unchanged production collection threshold. */
static const struct vm_cell_type pressure_type = { "vm-factory-pressure", NULL, NULL };
/* Independent identity and lifetime observations contribute to the result. */
static unsigned checks;
/* Failed native factory observations survive until process exit. */
static unsigned failures;

static void factory_check(int condition, const char *name);
static int factory_case(struct vm_heap *heap, struct vm_realm *realm);
static int factory_cold_failure(void);

/*
 * Exercises direct public factories without relying on conservative caller stack roots.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	int status;
	int printed;

	/* Production realm setup establishes the actual Array prototype and heap atom table. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Ordinary builtins supply the shared Array helper's actual realm prototype. */
	status = js_install_builtins(realm);
	if (status == 0)
		status = factory_case(heap, realm);
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return 2;

	/* Cold-heap failure uses the same public factory and actual live-byte cap. */
	status = factory_cold_failure();
	if (status != 0)
		return 2;

	/* Print all independent observations after releasing factory-owned temporary roots. */
	printed = printf("native VM factory GC: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* An incomplete factory result, missing collection or retained input rejects this case. */
	if (failures != 0)
		return 1;

	/* Succeeded: direct ownership and later collection passed under both host modes. */
	return 0;
}

/* Retains every failed native observation without skipping later independent checks. */
static void
factory_check(
	int condition,
	const char *name)
{
	int printed;

	/* Keep each failed result visible in the final fixture status. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}
}

/* Drives three direct factories through genuine threshold collections and result release. */
static int
factory_case(
	struct vm_heap *heap,
	struct vm_realm *realm)
{
	struct vm_object *prototype;
	struct vm_object *created;
	struct vm_object *array;
	struct vm_cell *roots[3];
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value input[2];
	vm_value result;
	vm_value element;
	uintptr_t prototype_address;
	uintptr_t object_address;
	uintptr_t first_address;
	uintptr_t second_address;
	unsigned index;
	unsigned registered;
	int status;
	int same;

	/* Construction slots hold participants only until each direct callee begins. */
	memset(roots, 0, sizeof(roots));
	registered = 0;
	status = 0;
	for (index = 0; index < 3U; index++) {
		status = vm_heap_add_root(heap, &roots[index]);
		if (status != 0)
			break;
		registered++;
	}

	/* Every factory operation uses the same bounded cleanup path. */
	if (status == 0) {
		do {
			/* An isolated prototype has no realm edge when its caller root is dropped. */
			prototype = vm_object_create(heap, NULL);
			if (prototype == NULL) {
				status = ENOMEM;
				break;
			}

			/* Construction ownership now protects the next allocation. */
			roots[0] = &prototype->cell;
			prototype_address = (uintptr_t)prototype;
			vm_heap_set_stack_base(heap, NULL);
			vm_heap_collect(heap);
			pressure = vm_heap_alloc(heap, &pressure_type, 8U * 1024U * 1024U);
			if (pressure == NULL) {
				status = ENOMEM;
				break;
			}

			/* The plain-object callee takes sole ownership before its first allocating step. */
			roots[0] = NULL;
			vm_heap_stats(heap, &before);
			created = vm_object_create(heap, prototype);
			vm_heap_stats(heap, &after);
			if (created == NULL) {
				status = ENOMEM;
				break;
			}

			/* The returned result is kept for post-call lifetime checks. */
			roots[1] = &created->cell;
			object_address = (uintptr_t)created;
			factory_check(after.collections > before.collections, "plain object factory collected without caller roots");
			factory_check(created->prototype == prototype, "new plain object retains exact callee-owned prototype");
			vm_heap_collect(heap);
			found = vm_heap_find_cell(heap, prototype_address);
			factory_check(found == &prototype->cell, "result retains prototype after factory roots release");
			roots[1] = NULL;
			vm_heap_collect(heap);
			found = vm_heap_find_cell(heap, object_address);
			factory_check(found == NULL, "released plain object is collectible");
			found = vm_heap_find_cell(heap, prototype_address);
			factory_check(found == NULL, "released private prototype is collectible");

			/* A second isolated prototype exercises key allocation after Array creation. */
			prototype = vm_object_create(heap, NULL);
			if (prototype == NULL) {
				status = ENOMEM;
				break;
			}

			/* Construction ownership now protects the next allocation. */
			roots[0] = &prototype->cell;
			prototype_address = (uintptr_t)prototype;
			vm_heap_collect(heap);
			pressure = vm_heap_alloc(heap, &pressure_type, 8U * 1024U * 1024U - 64U);
			if (pressure == NULL) {
				status = ENOMEM;
				break;
			}

			/* Array construction may collect after allocating its unpublished cell. */
			roots[0] = NULL;
			vm_heap_stats(heap, &before);
			array = vm_array_create(heap, prototype);
			vm_heap_stats(heap, &after);
			if (array == NULL) {
				status = ENOMEM;
				break;
			}

			/* The returned result is kept for post-call lifetime checks. */
			roots[1] = &array->cell;
			object_address = (uintptr_t)array;
			factory_check(after.collections > before.collections, "raw Array factory collected during unpublished construction");
			factory_check(array->prototype == prototype && array->length == 0 && array->kind == VM_KIND_ARRAY, "raw Array retains prototype and initialized empty length");
			vm_heap_collect(heap);
			found = vm_heap_find_cell(heap, prototype_address);
			factory_check(found == &prototype->cell, "complete raw Array owns its private prototype");
			roots[1] = NULL;
			vm_heap_collect(heap);
			found = vm_heap_find_cell(heap, object_address);
			factory_check(found == NULL, "released raw Array is collectible");
			found = vm_heap_find_cell(heap, prototype_address);
			factory_check(found == NULL, "released raw Array prototype is collectible");

			/* Two otherwise unowned cells become inputs to the shared builtin helper. */
			created = vm_object_create(heap, NULL);
			if (created == NULL) {
				status = ENOMEM;
				break;
			}

			/* Construction ownership now protects the next allocation. */
			roots[0] = &created->cell;
			first_address = (uintptr_t)created;
			prototype = vm_object_create(heap, NULL);
			if (prototype == NULL) {
				status = ENOMEM;
				break;
			}

			/* The returned result is kept for post-call lifetime checks. */
			roots[1] = &prototype->cell;
			second_address = (uintptr_t)prototype;
			input[0] = vm_value_cell(created);
			input[1] = vm_value_cell(prototype);
			vm_heap_collect(heap);
			pressure = vm_heap_alloc(heap, &pressure_type, 8U * 1024U * 1024U - 64U);
			if (pressure == NULL) {
				status = ENOMEM;
				break;
			}

			/* Only js_builtin_array's own slots retain both inputs and its newborn result. */
			result = VM_VALUE_EMPTY;
			roots[0] = NULL;
			roots[1] = NULL;
			vm_heap_stats(heap, &before);
			status = js_builtin_array(realm, input, 2, &result);
			vm_heap_stats(heap, &after);
			if (status != 0)
				break;
			array = (struct vm_object *)vm_value_as_cell(result);
			roots[2] = &array->cell;
			factory_check(after.collections > before.collections, "builtin Array collected with unowned input cells");
			factory_check(array->prototype == realm->array_prototype && array->length == 2, "builtin Array retains ordinary prototype and exact length");
			status = vm_object_get(array, vm_value_int32(0), &element);
			if (status != 0)
				break;
			same = vm_value_as_cell(element) == (struct vm_cell *)first_address;
			factory_check(same, "first builtin element keeps exact input cell identity");
			status = vm_object_get(array, vm_value_int32(1), &element);
			if (status != 0)
				break;
			same = vm_value_as_cell(element) == (struct vm_cell *)second_address;
			factory_check(same, "second builtin element keeps exact input cell identity");

			/* The Array alone owns inputs after the temporary roots have returned. */
			vm_heap_collect(heap);
			found = vm_heap_find_cell(heap, first_address);
			factory_check(found != NULL, "published builtin Array retains first input");
			found = vm_heap_find_cell(heap, second_address);
			factory_check(found != NULL, "published builtin Array retains second input");
			roots[2] = NULL;
			vm_heap_collect(heap);
			found = vm_heap_find_cell(heap, first_address);
			factory_check(found == NULL, "released builtin Array does not retain first input");
			found = vm_heap_find_cell(heap, second_address);
			factory_check(found == NULL, "released builtin Array does not retain second input");

			/* Invalid shared input never overwrites the caller's result sentinel. */
			result = VM_VALUE_EMPTY;
			status = js_builtin_array(realm, NULL, 1, &result);
			factory_check(status == EINVAL && result == VM_VALUE_EMPTY, "missing nonempty builtin input fails before result publication");
			status = 0;
		} while (0);
	}

	/* Restore ordinary fixture ownership and release only registered construction slots. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(heap, &roots[registered]);
	}

	/* Return a genuine factory or element failure to the fixture. */
	if (status != 0)
		return status;

	/* Succeeded: direct caller-free collection and result lifetime were inspected. */
	return 0;
}

/* Checks a cold heap's failing root-shape allocation without changing collector policy. */
static int
factory_cold_failure(
	void)
{
	struct vm_heap *heap;
	struct vm_object *array;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	int status;

	/* A bounded live-byte limit forces shape creation to collect the unpublished Array. */
	status = vm_heap_create(&heap, 128U);
	if (status != 0)
		return status;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_stats(heap, &before);
	array = vm_array_create(heap, NULL);
	vm_heap_stats(heap, &after);
	factory_check(array == NULL && after.collections > before.collections, "cold Array shape failure collects safely and returns NULL");

	/* Heap destruction finalizes every partially initialized cell after root cleanup. */
	vm_heap_destroy(heap);

	/* Succeeded: the bounded cold failure did not retain an unpublished object. */
	return 0;
}
