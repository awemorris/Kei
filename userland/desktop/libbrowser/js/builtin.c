/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The built-in objects of a realm (plan/ws074/design.md §12.3): the
 * installer that puts them on the global object, and the helpers the
 * groups share to make functions, constructors and properties.
 *
 * The Error objects come first, so a failure while the others are made is
 * already an Error; then Object, Function, Boolean, Number, Math and the
 * global functions.  Array, String and JSON arrive in ws074-p046.
 */

#include "js/builtin.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Installs the built-in objects on a realm's global object.
 */
int
js_install_builtins(
	struct vm_realm *realm)
{
	int error;

	/* The errors first, then the others in the order they build on each other. */
	error = js_builtin_install_error(realm);
	if (error == 0)
		error = js_builtin_install_object(realm);
	if (error == 0)
		error = js_builtin_install_function(realm);
	if (error == 0)
		error = js_builtin_install_symbol(realm);
	if (error == 0)
		error = js_builtin_install_array(realm);
	if (error == 0)
		error = js_builtin_install_string(realm);
	if (error == 0)
		error = js_builtin_install_regexp(realm);
	if (error == 0)
		error = js_builtin_install_date(realm);
	if (error == 0)
		error = js_builtin_install_json(realm);
	if (error == 0)
		error = js_builtin_install_boolean(realm);
	if (error == 0)
		error = js_builtin_install_number(realm);
	if (error == 0)
		error = js_builtin_install_math(realm);
	if (error == 0)
		error = js_builtin_install_global(realm);
	if (error == 0)
		error = js_builtin_install_uri(realm);
	if (error == 0)
		error = js_builtin_install_iterator(realm);
	if (error == 0)
		error = js_builtin_install_promise(realm);
	if (error == 0)
		error = js_builtin_install_generator(realm);
	if (error == 0)
		error = js_builtin_install_collection(realm);
	if (error == 0)
		error = js_builtin_install_tags(realm);
	if (error != 0)
		return error;

	/* Succeeded: the realm has its built-ins. */
	return 0;
}

/*
 * Makes a native function of a realm with its name and length (and, for a
 * constructor, what new runs).
 */
int
js_builtin_function(
	struct vm_realm *realm,
	const char *name,
	unsigned length,
	vm_native native,
	vm_native construct,
	struct vm_function **function)
{
	struct vm_function *made;

	/* The function. */
	made = vm_function_create_native(realm, name, length, native);
	if (made == NULL)
		return ENOMEM;
	made->construct = construct;

	/* Succeeded: the function. */
	*function = made;
	return 0;
}

/*
 * Defines a built-in method of an object: a native function, writable and
 * configurable but not enumerable.
 */
int
js_builtin_method(
	struct vm_realm *realm,
	struct vm_object *object,
	const char *name,
	unsigned length,
	vm_native native)
{
	struct vm_function *function;
	int error;

	/* The function. */
	error = js_builtin_function(realm, name, length, native, NULL, &function);
	if (error != 0)
		return error;

	/* The property. */
	error = js_builtin_value(realm, object, name, vm_value_cell(function), JS_BUILTIN_METHOD);
	if (error != 0)
		return error;

	/* Succeeded: the method is defined. */
	return 0;
}

/*
 * Defines a property of a built-in object by an ASCII name.
 */
int
js_builtin_value(
	struct vm_realm *realm,
	struct vm_object *object,
	const char *name,
	vm_value value,
	uint32_t attributes)
{
	vm_value key;
	int error;

	/* The name's key. */
	key = vm_key_from_ascii(realm->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;

	/* The property. */
	error = vm_object_define(realm->heap, object, key, value, attributes);
	if (error != 0)
		return error;

	/* Succeeded: the property is defined. */
	return 0;
}

/*
 * Defines a built-in accessor property (configurable, not enumerable) with
 * a native getter and, when given, a native setter.
 */
int
js_builtin_accessor(
	struct vm_realm *realm,
	struct vm_object *object,
	const char *name,
	vm_native getter,
	vm_native setter)
{
	struct vm_function *getter_function;
	struct vm_function *setter_function;
	struct vm_accessor *accessor;
	char accessor_name[80];
	vm_value setter_value;
	int error;

	/* The getter, named "get NAME". */
	snprintf(accessor_name, sizeof(accessor_name), "get %s", name);
	error = js_builtin_function(realm, accessor_name, 0, getter, NULL, &getter_function);
	if (error != 0)
		return error;

	/* The setter, named "set NAME", when there is one. */
	setter_value = VM_VALUE_UNDEFINED;
	if (setter != NULL) {
		snprintf(accessor_name, sizeof(accessor_name), "set %s", name);
		error = js_builtin_function(realm, accessor_name, 1, setter, NULL, &setter_function);
		if (error != 0)
			return error;
		setter_value = vm_value_cell(setter_function);
	}

	/* The pair as the property. */
	accessor = vm_accessor_create(realm->heap, vm_value_cell(getter_function), setter_value);
	if (accessor == NULL)
		return ENOMEM;
	error = js_builtin_value(realm, object, name, vm_value_cell(accessor), VM_PROPERTY_ACCESSOR | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* Succeeded: the accessor is defined. */
	return 0;
}

/*
 * Makes a built-in constructor with its prototype object (the
 * constructor's prototype property, which cannot change, and the
 * prototype's constructor), and defines it on the global object.
 */
int
js_builtin_constructor(
	struct vm_realm *realm,
	const char *name,
	unsigned length,
	vm_native native,
	vm_native construct,
	struct vm_object *prototype,
	struct vm_function **function)
{
	struct vm_function *made;
	int error;

	/* The function. */
	error = js_builtin_function(realm, name, length, native, construct, &made);
	if (error != 0)
		return error;

	/* Its prototype, and the prototype's constructor. */
	error = js_builtin_value(realm, &made->object, "prototype", vm_value_cell(prototype), 0);
	if (error == 0)
		error = js_builtin_value(realm, prototype, "constructor", vm_value_cell(made), JS_BUILTIN_METHOD);
	if (error != 0)
		return error;

	/* The global. */
	error = js_builtin_value(realm, realm->global, name, vm_value_cell(made), JS_BUILTIN_METHOD);
	if (error != 0)
		return error;

	/* Succeeded: the constructor. */
	*function = made;
	return 0;
}

/*
 * Reports the native function the realm is running (its callee).
 */
struct vm_function *
js_builtin_callee(
	const struct vm_realm *realm)
{
	/* The caller set it before the call. */
	return (struct vm_function *)vm_value_as_cell(realm->callee);
}

/*
 * Makes a string value from UTF-8 text.
 */
int
js_builtin_string(
	struct vm_realm *realm,
	const char *text,
	vm_value *value)
{
	struct vm_string *string;

	/* The string. */
	string = vm_string_from_utf8(realm->heap, text, strlen(text));
	if (string == NULL)
		return ENOMEM;

	/* Succeeded: its value. */
	*value = vm_value_cell(string);
	return 0;
}

/*
 * Makes an array of values while retaining every input and the unpublished Array.
 */
int
js_builtin_array(
	struct vm_realm *realm,
	const vm_value *values,
	uint32_t count,
	vm_value *array)
{
	struct vm_object *made;
	struct vm_cell **roots;
	size_t root_count;
	size_t root_index;
	size_t registered;
	uint32_t index;
	int error;
	int is_cell;

	/* Reject missing elements or root-array size overflow before any allocation. */
	if (count != 0 && values == NULL)
		return EINVAL;
	root_count = (size_t)count;
	if (root_count == SIZE_MAX)
		return EOVERFLOW;
	root_count++;
	if (root_count > SIZE_MAX / sizeof(*roots))
		return EOVERFLOW;

	/* Every input cell survives Array creation before its element can be installed. */
	roots = calloc(root_count, sizeof(*roots));
	if (roots == NULL)
		return ENOMEM;
	for (index = 0; index < count; index++) {
		is_cell = vm_value_is_cell(values[index]);
		if (is_cell)
			roots[index] = vm_value_as_cell(values[index]);
	}

	/* Reserve one last slot for the Array before any VM allocation. */
	registered = 0;
	error = 0;
	for (root_index = 0; root_index < root_count; root_index++) {
		error = vm_heap_add_root(realm->heap, &roots[root_index]);
		if (error != 0)
			break;
		registered++;
	}

	/* The factory's own roots cover construction; this caller then retains its result. */
	if (error == 0) {
		made = vm_array_create(realm->heap, realm->array_prototype);
		if (made == NULL) {
			error = ENOMEM;
		} else {
			roots[count] = &made->cell;

			/* Assign every element before the Array can escape to its caller. */
			for (index = 0; index < count; index++) {
				error = vm_object_define(realm->heap, made, vm_value_int32((int32_t)index), values[index], VM_PROPERTY_DEFAULT);
				if (error != 0)
					break;
			}

			/* Publish only a fully populated Array. */
			if (error == 0)
				*array = vm_value_cell(made);
		}
	}

	/* Every registered temporary slot is released, including failed property creation. */
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(realm->heap, &roots[registered]);
	}

	/* The C-side slot table no longer carries any collector roots. */
	free(roots);

	/* Propagate the original allocation or element-definition failure. */
	if (error != 0)
		return error;

	/* Succeeded: only the returned Array owns its input values. */
	return 0;
}

/*
 * Converts a value to an integer or an infinity (ToIntegerOrInfinity):
 * NaN is 0, the rest truncated towards zero.
 */
int
js_builtin_integer(
	struct vm_realm *realm,
	vm_value value,
	double *integer)
{
	double number;
	int status;

	/* The number. */
	status = vm_to_number(realm, value, &number);
	if (status != 0)
		return status;

	/* NaN is zero; infinities stay; the rest lose their fraction (and -0 its sign). */
	if (number != number) {
		*integer = 0.0;
		return 0;
	}

	/* The whole part (a -0 is 0). */
	*integer = trunc(number);
	if (*integer == 0.0)
		*integer = 0.0;

	/* Succeeded: the integer. */
	return 0;
}

/*
 * Reads an array-like object's length (LengthOfArrayLike: ToLength of its
 * length property, capped at what an array can hold).
 */
int
js_builtin_length(
	struct vm_realm *realm,
	vm_value object,
	uint32_t *length)
{
	vm_value key;
	vm_value value;
	double number;
	int status;

	/* The length property. */
	*length = 0;
	key = vm_key_from_ascii(realm->heap, "length");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_get(realm, object, key, &value);
	if (status != 0)
		return status;

	/* As an integer from 0 up (the engine's lists stop at 2^31). */
	status = js_builtin_integer(realm, value, &number);
	if (status != 0)
		return status;
	if (number <= 0.0)
		return 0;
	if (number > 2147483647.0) {
		status = vm_throw_range_error(realm, "Invalid array length");
		return status;
	}

	/* Succeeded: the length. */
	*length = (uint32_t)number;
	return 0;
}
