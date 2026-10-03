/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Verifies VM factory and reentrant invocation ownership across actual production GC. */

#include "js/builtin.h"
#include "vm/bytecode.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Ordinary inert cells approach the unchanged production collection threshold. */
static const struct vm_cell_type pressure_type = { "vm-factory-pressure", NULL, NULL };
/* Independent identity and lifetime observations contribute to the result. */
static unsigned checks;
/* Failed native factory observations survive until process exit. */
static unsigned failures;
/* Integer-only native observations never keep a VM participant alive. */
static uintptr_t native_observed[5];
/* The inner collector records lost cells without reading freed state. */
static unsigned native_lost;

static void factory_check(int condition, const char *name);
static int factory_case(struct vm_heap *heap, struct vm_realm *realm);
static int factory_cold_failure(void);
static int factory_function_native(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int factory_function_script(struct vm_realm *realm, const char *source, vm_value *result);
static int factory_function_case(unsigned mode);
static int factory_native_case(void);
static int factory_native_inner(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int factory_native_outer(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

/*
 * Exercises direct public factories without relying on conservative caller stack roots.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	unsigned mode;
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
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The actual object and Array tests complete before their owning realm is retired. */
	status = factory_case(heap, realm);
	if (status != 0) {
		vm_heap_set_stack_base(heap, __builtin_frame_address(0));
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Restore the ordinary embedding after checking the completed factory result. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);

	/* Cold-heap failure uses the same public factory and actual live-byte cap. */
	status = factory_cold_failure();
	if (status != 0)
		return 2;

	/* Four independent heaps exercise parent, unpublished native, bytecode and closure ownership. */
	for (mode = 0; mode < 4U; mode++) {
		status = factory_function_case(mode);
		if (status != 0)
			return 2;
	}

	/* A real nested native collector also exercises suspended realm state and direct inputs. */
	status = factory_native_case();
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

	/* Succeeded: each failed observation remains visible to the fixture. */
	return;
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
	struct vm_cell *observed;
	int is_object;
	int same;

	/* Construction slots hold participants only until each direct callee begins. */
	memset(roots, 0, sizeof(roots));
	registered = 0;
	status = 0;
	for (index = 0; index < 3U; index++) {
		status = vm_heap_add_root(heap, &roots[index]);
		if (status != 0)
			goto cleanup;

		/* Cleanup owns only this successfully acquired construction registration. */
		registered++;
	}

	/* An isolated prototype has no realm edge when its caller root is dropped. */
	prototype = vm_object_create(heap, NULL);
	if (prototype == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Construction ownership now protects the next allocation. */
	roots[0] = &prototype->cell;
	prototype_address = (uintptr_t)prototype;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	pressure = vm_heap_alloc(heap, &pressure_type, 8U * 1024U * 1024U);
	if (pressure == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* The plain-object callee takes sole ownership before its first allocating step. */
	roots[0] = NULL;
	vm_heap_stats(heap, &before);
	created = vm_object_create(heap, prototype);
	if (created == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Observe collection only after a successful factory result has been checked. */
	vm_heap_stats(heap, &after);

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
		goto cleanup;
	}

	/* Construction ownership now protects the next allocation. */
	roots[0] = &prototype->cell;
	prototype_address = (uintptr_t)prototype;
	vm_heap_collect(heap);
	pressure = vm_heap_alloc(heap, &pressure_type, 8U * 1024U * 1024U - 64U);
	if (pressure == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Array construction may collect after allocating its unpublished cell. */
	roots[0] = NULL;
	vm_heap_stats(heap, &before);
	array = vm_array_create(heap, prototype);
	if (array == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Observe collection only after a successful factory result has been checked. */
	vm_heap_stats(heap, &after);

	/* The returned result is kept for post-call lifetime checks. */
	roots[1] = &array->cell;
	object_address = (uintptr_t)array;
	factory_check(after.collections > before.collections, "raw Array factory collected during unpublished construction");
	factory_check(
		array->prototype == prototype &&
		array->length == 0 &&
		array->kind == VM_KIND_ARRAY,
		"raw Array retains prototype and initialized empty length");
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
		goto cleanup;
	}

	/* Construction ownership now protects the next allocation. */
	roots[0] = &created->cell;
	first_address = (uintptr_t)created;
	prototype = vm_object_create(heap, NULL);
	if (prototype == NULL) {
		status = ENOMEM;
		goto cleanup;
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
		goto cleanup;
	}

	/* Only js_builtin_array's own slots retain both inputs and its newborn result. */
	result = VM_VALUE_EMPTY;
	roots[0] = NULL;
	roots[1] = NULL;
	vm_heap_stats(heap, &before);
	status = js_builtin_array(realm, input, 2, &result);
	if (status != 0)
		goto cleanup;

	/* Brand the published result before dereferencing its native Array fields. */
	is_object = vm_value_is_object(result);
	if (!is_object) {
		status = EIO;
		goto cleanup;
	}

	/* Only the successful branded result contributes to the collection observation. */
	vm_heap_stats(heap, &after);
	array = (struct vm_object *)vm_value_as_cell(result);
	roots[2] = &array->cell;
	factory_check(after.collections > before.collections, "builtin Array collected with unowned input cells");
	factory_check(array->prototype == realm->array_prototype && array->length == 2, "builtin Array retains ordinary prototype and exact length");
	status = vm_object_get(array, vm_value_int32(0), &element);
	if (status != 0)
		goto cleanup;
	observed = vm_value_as_cell(element);
	same = 0;
	if (observed == (struct vm_cell *)first_address)
		same = 1;
	factory_check(same, "first builtin element keeps exact input cell identity");
	status = vm_object_get(array, vm_value_int32(1), &element);
	if (status != 0)
		goto cleanup;
	observed = vm_value_as_cell(element);
	same = 0;
	if (observed == (struct vm_cell *)second_address)
		same = 1;
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
cleanup:
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

/* Supplies a real native entry without allocation or host effects. */
static int
factory_function_native(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Succeeded: the native remains callable after complete factory publication. */
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Builds bytecode and captured environments through the actual production compiler. */
static int
factory_function_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *result)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int status;

	/* Decode ordinary script before invoking the real compiler/interpreter. */
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Generated functions provide real production code and closure objects. */
	status = js_run_script(realm, units.data, units.length, 0, result, &syntax);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* Release source storage after successful execution has been checked. */
	wb_units_release(&units);

	/* Succeeded: the real function completion is available. */
	return 0;
}

/* Validates factory ownership through real threshold collection, calls and root retirement. */
static int
factory_function_case(
	unsigned mode)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct vm_function *original;
	struct vm_function *function;
	struct vm_env *parent;
	struct vm_env *created;
	struct vm_code *code;
	struct vm_cell *roots[3];
	struct vm_cell *pressure;
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value answer;
	vm_value argument;
	uintptr_t input_address;
	uintptr_t parent_address;
	vm_value expected;
	vm_value property;
	vm_value key;
	vm_value *slots;
	struct vm_string *text;
	int equal;
	uintptr_t output_address;
	size_t pressure_size;
	unsigned index;
	unsigned registered;
	int callable;
	int status;

	/* Ordinary heap and realm setup owns only builtins, not the isolated factory participants. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return status;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return status;
	}

	/* Populate the normal intrinsic prototypes before any pressure is introduced. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return status;
	}

	/* Construction slots stop retaining inputs before each actual native factory begins. */
	memset(roots, 0, sizeof(roots));
	registered = 0;
	for (index = 0; index < 3U; index++) {
		status = vm_heap_add_root(heap, &roots[index]);
		if (status != 0)
			goto cleanup;

		/* Cleanup owns only this successfully acquired construction registration. */
		registered++;
	}

	/* Standalone environment parents have no realm or function owner. */
	function = NULL;
	created = NULL;
	parent = NULL;
	code = NULL;
	input_address = 0;
	parent_address = 0;
	if (mode == 0U) {
		parent = vm_env_create(heap, NULL, 1);
		if (parent == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Keep the actual parent only during subsequent fixture construction. */
		roots[0] = &parent->cell;
		input_address = (uintptr_t)parent;
		parent_address = (uintptr_t)parent;
	} else if (mode >= 2U) {
		/* Real bytecode has a distinct captured environment only in the closure case. */
		if (mode == 2U) {
			status = factory_function_script(realm, "(function(n){return n+17;})", &answer);
			if (status != 0)
				goto cleanup;
		} else {
			status = factory_function_script(realm, "(function(){var secret=17;return function(n){return secret+n;};})()", &answer);
			if (status != 0)
				goto cleanup;
		}

		/* Inspect only an actual generated callable before capturing its private inputs. */
		callable = vm_value_is_callable(answer);
		if (!callable) {
			status = EIO;
			goto cleanup;
		}

		/* Generated code and environment are owned only through construction slots. */
		original = (struct vm_function *)vm_value_as_cell(answer);
		code = original->code;
		parent = original->env;
		parent_address = (uintptr_t)parent;
		roots[0] = &code->cell;
		if (parent != NULL)
			roots[2] = &parent->cell;
		input_address = (uintptr_t)code;
	}

	/* Reset accounting with no conservative root while preserving only temporary construction slots. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	pressure_size = 8U * 1024U * 1024U;
	if (mode == 1U)
		pressure_size -= 16U;
	pressure = vm_heap_alloc(heap, &pressure_type, pressure_size);
	if (pressure == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* The real callee must take ownership before the next default allocation collects. */
	roots[0] = NULL;
	roots[2] = NULL;
	vm_heap_stats(heap, &before);
	if (mode == 0U) {
		created = vm_env_create(heap, parent, 4096U);
		if (created == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Only a checked returned cell contributes its publication address. */
		output_address = (uintptr_t)created;
	} else if (mode == 1U) {
		function = vm_function_create_native(realm, "owned-native", 1, factory_function_native);
		if (function == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Only a checked returned cell contributes its publication address. */
		output_address = (uintptr_t)function;
	} else if (mode == 2U) {
		function = vm_function_create(realm, code);
		if (function == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Only a checked returned cell contributes its publication address. */
		output_address = (uintptr_t)function;
	} else {
		function = vm_closure_create(realm, code, parent);
		if (function == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Only a checked returned cell contributes its publication address. */
		output_address = (uintptr_t)function;
	}

	/* Integer-only observations prove collection occurred and the complete graph still exists. */
	vm_heap_stats(heap, &after);
	factory_check(after.collections > before.collections, "direct function/environment factory performs actual default GC");
	if (after.collections <= before.collections) {
		status = EIO;
		goto cleanup;
	}

	/* Check the actual input graph before reading any input-derived native field. */
	if (input_address != 0) {
		found = vm_heap_find_cell(heap, input_address);
		factory_check(found != NULL, "callee factory retains its unowned parent or bytecode input");
		if (found == NULL) {
			status = EIO;
			goto cleanup;
		}
	}

	/* Only surviving complete output may receive a root after the entire factory has returned. */
	found = vm_heap_find_cell(heap, output_address);
	factory_check(found != NULL, "factory publishes a surviving complete output cell");
	if (found == NULL) {
		status = EIO;
		goto cleanup;
	}

	/* The caller begins ownership only after complete factory publication was verified. */
	roots[1] = found;

	/* Environment output owns the precise parent and all requested undefined slots. */
	if (mode == 0U) {
		factory_check(created->parent == parent && created->count == 4096U, "new environment retains exact parent and slot count");
		slots = vm_env_slots(created);
		factory_check(slots[0] == VM_VALUE_UNDEFINED && slots[4095] == VM_VALUE_UNDEFINED, "new environment initializes first and final slot");
	}

	/* The published function executes real native or bytecode semantics. */
	if (mode != 0U) {
		if (found->type != &vm_function_type) {
			status = EIO;
			goto cleanup;
		}

		/* Execute only an actual surviving function cell. */
		argument = vm_value_number(5);
		status = vm_call(realm, vm_value_cell(function), VM_VALUE_UNDEFINED, &argument, 1, &answer);
		if (status != 0)
			goto cleanup;

		/* Expected completion depends only on the actual factory's native or bytecode kind. */
		expected = VM_VALUE_UNDEFINED;
		if (mode >= 2U)
			expected = vm_value_number(22);
		factory_check(answer == expected, "published native/bytecode/closure executes actual expected result");
		if (answer != expected) {
			status = EIO;
			goto cleanup;
		}

		/* Native names and lengths survive the same unpublished-object collection. */
		if (mode == 1U) {
			key = vm_key_from_ascii(heap, "length");
			if (key == VM_VALUE_EMPTY) {
				status = ENOMEM;
				goto cleanup;
			}

			/* Inspect the successful property lookup before comparing its value. */
			status = vm_object_get(&function->object, key, &property);
			if (status != 0)
				goto cleanup;

			/* The checked native length must equal the constructor's declared argument count. */
			expected = vm_value_int32(1);
			factory_check(property == expected, "published native retains configurable length property");
			key = vm_key_from_ascii(heap, "name");
			if (key == VM_VALUE_EMPTY) {
				status = ENOMEM;
				goto cleanup;
			}

			/* Read and brand the actual private name before inspecting its native text. */
			status = vm_object_get(&function->object, key, &property);
			if (status != 0)
				goto cleanup;

			/* The successful name result must have the actual string cell brand. */
			equal = vm_value_is_string(property);
			if (!equal) {
				status = EIO;
				goto cleanup;
			}

			/* The completed name belongs to the function after temporary factory roots are gone. */
			text = (struct vm_string *)vm_value_as_cell(property);
			equal = vm_string_equal_ascii(text, "owned-native");
			factory_check(equal, "published native retains exact private name property");
		}
	}

	/* No constructor slot may keep its result or isolated inputs after the caller drops output. */
	roots[1] = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, output_address);
	factory_check(found == NULL, "released function/environment output is collectible");
	if (found != NULL) {
		status = EIO;
		goto cleanup;
	}

	/* Input code and captured private environments lose all factory roots with the output. */
	if (input_address != 0) {
		found = vm_heap_find_cell(heap, input_address);
		factory_check(found == NULL, "released factory does not permanently retain input code or parent");
	}

	/* The captured private environment has no realm owner after the closure is released. */
	if (mode == 3U) {
		found = vm_heap_find_cell(heap, parent_address);
		factory_check(found == NULL, "released closure does not retain captured private environment");
	}

cleanup:
	/* Unwind only acquired construction slots, then restore ordinary embedding teardown. */
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(heap, &roots[registered]);
	}

	/* Restore normal embedding teardown after all construction slots are unregistered. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return status;

	/* Succeeded: this direct factory returned a complete collectible graph. */
	return 0;
}

/* Exercises nested native state with no caller roots or conservative stack retention. */
static int
factory_native_case(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct vm_function *function;
	struct vm_object *object;
	struct vm_cell *roots[5];
	struct vm_cell *found;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	vm_value argument;
	vm_value receiver;
	vm_value answer;
	vm_value saved_callee;
	vm_value saved_target;
	unsigned index;
	unsigned registered;
	int status;

	/* Real primary-realm initialization supplies ordinary native function prototypes. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return status;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return status;
	}

	/* Every setup result is checked before constructing isolated inputs. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return status;
	}

	/* Only construction owns the direct callable, receiver, argument and previous realm state. */
	memset(roots, 0, sizeof(roots));
	registered = 0;
	native_lost = 0;
	for (index = 0; index < 5U; index++) {
		status = vm_heap_add_root(heap, &roots[index]);
		if (status != 0)
			goto cleanup;

		/* Cleanup owns only this successfully acquired construction registration. */
		registered++;
	}

	/* The outer callable is otherwise unowned when inner native GC runs. */
	function = vm_function_create_native(realm, "outer", 1, factory_native_outer);
	if (function == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* Record only integer addresses for the callback's collection observations. */
	roots[0] = &function->object.cell;
	native_observed[0] = (uintptr_t)function;
	for (index = 1; index < 5U; index++) {
		object = vm_object_create(heap, NULL);
		if (object == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Each private participant remains protected solely during fixture construction. */
		roots[index] = &object->cell;
		native_observed[index] = (uintptr_t)object;
	}

	/* Realm state is overwritten by native entry, so its suspended edges need callee ownership. */
	receiver = vm_value_cell(roots[1]);
	argument = vm_value_cell(roots[2]);
	saved_callee = vm_value_cell(roots[3]);
	saved_target = vm_value_cell(roots[4]);
	realm->callee = saved_callee;
	realm->new_target = saved_target;
	vm_heap_set_stack_base(heap, NULL);
	for (index = 0; index < 5U; index++)
		roots[index] = NULL;
	vm_heap_stats(heap, &before);
	status = vm_call(realm, vm_value_cell(function), receiver, &argument, 1, &answer);
	if (status != 0)
		goto cleanup;

	/* The inner callback records actual surviving cells without dereferencing missing state. */
	vm_heap_stats(heap, &after);
	factory_check(after.collections > before.collections, "nested native callback performs actual caller-free GC");
	factory_check(native_lost == 0, "nested native retains direct inputs and suspended callee/new.target");
	factory_check(realm->callee == saved_callee && realm->new_target == saved_target, "outer dispatch restores exact previous realm state");
	factory_check(answer == VM_VALUE_UNDEFINED, "nested native returns its actual completion");

	/* Realm state is the final owner after direct invocation roots have been released. */
	realm->callee = VM_VALUE_UNDEFINED;
	realm->new_target = VM_VALUE_UNDEFINED;
	vm_heap_collect(heap);
	for (index = 0; index < 5U; index++) {
		found = vm_heap_find_cell(heap, native_observed[index]);
		factory_check(found == NULL, "released nested invocation does not retain isolated participant");
	}

cleanup:
	/* Clear overwritten state and unwind only acquired construction registrations. */
	realm->callee = VM_VALUE_UNDEFINED;
	realm->new_target = VM_VALUE_UNDEFINED;
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(heap, &roots[registered]);
	}

	/* Restore normal embedding teardown after releasing all temporary collector slots. */
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0)
		return status;

	/* Succeeded: reentrant native ownership and later root retirement were observed. */
	return 0;
}

/* Collects while inner native entry temporarily replaces its suspended caller's realm state. */
static int
factory_native_inner(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_cell *found;
	unsigned index;

	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Actual production GC must retain every direct or suspended invocation participant. */
	vm_heap_collect(realm->heap);
	for (index = 0; index < 5U; index++) {
		found = vm_heap_find_cell(realm->heap, native_observed[index]);
		if (found == NULL)
			native_lost++;
	}

	/* Succeeded: missing cells contribute failure without a freed-state dereference. */
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Reenters the actual dispatcher before inspecting the restored outer native state. */
static int
factory_native_outer(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *function;
	vm_value expected;
	int status;

	/* A real inner function has no caller root when its own native collector runs. */
	function = vm_function_create_native(realm, "inner", 1, factory_native_inner);
	if (function == NULL)
		return ENOMEM;

	/* Invoke the real reentrant path and check execution before inspecting restored state. */
	status = vm_call(realm, vm_value_cell(function), receiver, args, count, result);
	if (status != 0)
		return status;

	/* Boxing the observed integer address compares state without dereferencing the participant. */
	expected = vm_value_cell((struct vm_cell *)native_observed[0]);
	factory_check(realm->callee == expected && realm->new_target == VM_VALUE_UNDEFINED, "inner dispatch restores actual outer callee and call target");

	/* Succeeded: the nested dispatcher returned with its exact suspended invocation restored. */
	return 0;
}
