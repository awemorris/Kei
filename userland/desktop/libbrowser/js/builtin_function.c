/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Function: the constructor (a function made from source text in the
 * global scope) and Function.prototype's call, apply, bind and toString.
 *
 * A bound function is a native function whose data is an internal array:
 * its target, the bound this and the bound arguments.  toString writes
 * every function as native code for now (a script function's source text
 * is not kept yet).
 */

#include "js/builtin.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The slots of a bound function's data array before its bound arguments. */
#define FUNCTION_BOUND_TARGET	0U
#define FUNCTION_BOUND_THIS	1U
#define FUNCTION_BOUND_ARGS	2U

static int function_has_instance(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_call_constructor(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_call(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_apply(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_bind(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_to_string(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_bound_call(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_bound_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int function_bound_arguments(struct vm_function *bound, const vm_value *args, unsigned count, vm_value **all, unsigned *all_count);
static int function_bound_length(struct vm_realm *realm, vm_value target, unsigned bound_count, vm_value *length);
static int function_bound_name(struct vm_realm *realm, vm_value target, vm_value *name);
static int function_append_text(struct wb_units *units, const char *text);

/*
 * Installs Function and Function.prototype's methods.
 */
int
js_builtin_install_function(
	struct vm_realm *realm)
{
	struct vm_function *constructor;
	struct vm_function *has_instance;
	struct vm_object *prototype;
	struct vm_accessor *accessor;
	vm_value thrower;
	int error;

	/* The constructor over the realm's Function.prototype (called or constructed alike). */
	prototype = realm->function_prototype;
	error = js_builtin_constructor(realm, "Function", 1, function_call_constructor, function_call_constructor, prototype, &constructor);
	if (error != 0)
		return error;

	/* The prototype's methods. */
	error = js_builtin_method(realm, prototype, "apply", 2, function_apply);
	if (error == 0)
		error = js_builtin_method(realm, prototype, "bind", 1, function_bind);
	if (error == 0)
		error = js_builtin_method(realm, prototype, "call", 1, function_call);
	if (error == 0)
		error = js_builtin_method(realm, prototype, "toString", 0, function_to_string);
	if (error != 0)
		return error;

	/* Symbol.hasInstance (neither writable, enumerable nor configurable), which instanceof recognizes (ws074-p087). */
	error = js_builtin_function(realm, "[Symbol.hasInstance]", 1, function_has_instance, NULL, &has_instance);
	if (error != 0)
		return error;
	error = js_builtin_symbol_value(realm, prototype, VM_SYMBOL_HAS_INSTANCE, vm_value_cell(has_instance), 0);
	if (error != 0)
		return error;
	realm->intrinsics[VM_INTRINSIC_HAS_INSTANCE] = &has_instance->object;

	/* caller and arguments: accessors that throw (AddRestrictedFunctionProperties). */
	thrower = vm_value_cell(realm->intrinsics[VM_INTRINSIC_THROW_TYPE_ERROR]);
	accessor = vm_accessor_create(realm->heap, thrower, thrower);
	if (accessor == NULL)
		return ENOMEM;
	error = js_builtin_value(realm, prototype, "caller", vm_value_cell(accessor), VM_PROPERTY_ACCESSOR | VM_PROPERTY_CONFIGURABLE);
	if (error == 0)
		error = js_builtin_value(realm, prototype, "arguments", vm_value_cell(accessor), VM_PROPERTY_ACCESSOR | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* Succeeded: Function is installed. */
	return 0;
}

/* Function(p1, ..., pn, body): a function made from the parameters and the body, in the global scope. */
static int
function_call_constructor(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_units source;
	struct vm_string *part;
	struct vm_string *text;
	unsigned index;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The source: (function anonymous(p1,...,pn\n) {\nbody\n}). */
	wb_units_init(&source);
	status = function_append_text(&source, "(function anonymous(");
	for (index = 0; status == 0 && index + 1U < count; index++) {
		if (index > 0)
			status = function_append_text(&source, ",");
		if (status == 0)
			status = vm_to_string(realm, args[index], &part);
		if (status == 0)
			status = vm_string_append_units(part, &source);
	}

	/* The end of the parameters, the body and the end. */
	if (status == 0)
		status = function_append_text(&source, "\n) {\n");
	if (status == 0 && count > 0) {
		status = vm_to_string(realm, args[count - 1U], &part);
		if (status == 0)
			status = vm_string_append_units(part, &source);
	}

	/* The end of the function expression. */
	if (status == 0)
		status = function_append_text(&source, "\n})");
	if (status != 0) {
		wb_units_release(&source);
		return status;
	}

	/* The source as a string, evaluated in the global scope. */
	text = vm_string_from_units(realm->heap, source.data, source.length);
	wb_units_release(&source);
	if (text == NULL)
		return ENOMEM;
	status = js_builtin_evaluate(realm, text, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: the function. */
	return 0;
}

/* Function.prototype.call(thisArg, ...args). */
static int
function_call(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int callable;
	int status;

	/* Only a function can be called. */
	callable = vm_value_is_callable(this_value);
	if (!callable) {
		status = vm_throw_type_error(realm, "Function.prototype.call called on a value that is not a function");
		return status;
	}

	/* The call with the rest of the arguments. */
	if (count == 0) {
		status = vm_call(realm, this_value, VM_VALUE_UNDEFINED, NULL, 0, result);
	} else {
		status = vm_call(realm, this_value, args[0], args + 1, count - 1U, result);
	}

	/* Reports a failed call. */
	if (status != 0)
		return status;

	/* Succeeded: the function's result. */
	return 0;
}

/* Function.prototype.apply(thisArg, argArray): the arguments from an array-like object. */
static int
function_apply(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value *list;
	vm_value array;
	uint32_t length;
	uint32_t index;
	int callable;
	int is_object;
	int status;

	/* Only a function can be called. */
	callable = vm_value_is_callable(this_value);
	if (!callable) {
		status = vm_throw_type_error(realm, "Function.prototype.apply was called on a value that is not a function");
		return status;
	}

	/* No array: no arguments. */
	array = js_argument(args, count, 1);
	if (array == VM_VALUE_UNDEFINED || array == VM_VALUE_NULL) {
		status = vm_call(realm, this_value, js_argument(args, count, 0), NULL, 0, result);
		return status;
	}

	/* An array-like object's elements (CreateListFromArrayLike). */
	is_object = vm_value_is_object(array);
	if (!is_object) {
		status = vm_throw_type_error(realm, "CreateListFromArrayLike called on non-object");
		return status;
	}

	/* The list's length. */
	status = js_builtin_length(realm, array, &length);
	if (status != 0)
		return status;
	list = calloc((size_t)length + 1U, sizeof(vm_value));
	if (list == NULL)
		return ENOMEM;

	/* Each element (the list is not seen by the collector, but every element is also held by the array). */
	status = 0;
	for (index = 0; status == 0 && index < length; index++)
		status = vm_get(realm, array, vm_value_int32((int32_t)index), &list[index]);

	/* The call. */
	if (status == 0)
		status = vm_call(realm, this_value, js_argument(args, count, 0), list, length, result);
	free(list);
	if (status != 0)
		return status;

	/* Succeeded: the function's result. */
	return 0;
}

/* Function.prototype.bind(thisArg, ...args): a bound function. */
static int
function_bind(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *target;
	struct vm_function *bound;
	vm_native construct;
	vm_value *data;
	vm_value data_array;
	vm_value length;
	vm_value name;
	unsigned bound_count;
	unsigned index;
	int callable;
	int constructor;
	int status;

	/* Only a function can be bound. */
	callable = vm_value_is_callable(this_value);
	if (!callable) {
		status = vm_throw_type_error(realm, "Bind must be called on a function");
		return status;
	}

	/* The target. */
	target = (struct vm_function *)vm_value_as_cell(this_value);

	/* The data: the target, the bound this and the bound arguments. */
	bound_count = 0;
	if (count > 1U)
		bound_count = count - 1U;
	data = calloc(FUNCTION_BOUND_ARGS + (size_t)bound_count, sizeof(vm_value));
	if (data == NULL)
		return ENOMEM;
	data[FUNCTION_BOUND_TARGET] = this_value;
	data[FUNCTION_BOUND_THIS] = js_argument(args, count, 0);
	for (index = 0; index < bound_count; index++)
		data[FUNCTION_BOUND_ARGS + index] = args[index + 1U];
	status = js_builtin_array(realm, data, FUNCTION_BOUND_ARGS + bound_count, &data_array);
	free(data);
	if (status != 0)
		return status;

	/* The bound function: a constructor when its target is one, with the target's prototype. */
	constructor = vm_value_is_constructor(this_value);
	construct = NULL;
	if (constructor)
		construct = function_bound_construct;
	status = js_builtin_function(realm, "", 0, function_bound_call, construct, &bound);
	if (status != 0)
		return status;
	bound->data = data_array;
	bound->object.prototype = target->object.prototype;

	/* Its length and name from the target's. */
	status = function_bound_length(realm, this_value, bound_count, &length);
	if (status == 0)
		status = js_builtin_value(realm, &bound->object, "length", length, VM_PROPERTY_CONFIGURABLE);
	if (status == 0)
		status = function_bound_name(realm, this_value, &name);
	if (status == 0)
		status = js_builtin_value(realm, &bound->object, "name", name, VM_PROPERTY_CONFIGURABLE);
	if (status != 0)
		return status;

	/* Succeeded: the bound function. */
	*result = vm_value_cell(bound);
	return 0;
}

/* Function.prototype.toString(): native code for every function for now. */
static int
function_to_string(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_buffer text;
	struct vm_string *name;
	vm_value key;
	vm_value value;
	int callable;
	int is_string;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Only a function has a source text. */
	callable = vm_value_is_callable(this_value);
	if (!callable) {
		status = vm_throw_type_error(realm, "Function.prototype.toString requires that 'this' be a Function");
		return status;
	}

	/* function NAME() { [native code] }, the name when it is a string. */
	wb_buffer_init(&text);
	status = wb_buffer_append_string(&text, "function ");
	key = vm_key_from_ascii(realm->heap, "name");
	if (key == VM_VALUE_EMPTY)
		status = ENOMEM;
	if (status == 0) {
		status = vm_get(realm, this_value, key, &value);
		is_string = vm_value_is_string(value);
		if (status == 0 && is_string) {
			name = (struct vm_string *)vm_value_as_cell(value);
			status = vm_string_to_utf8(name, &text);
		}
	}

	/* The rest of the text, as a string. */
	if (status == 0)
		status = wb_buffer_append_string(&text, "() { [native code] }");
	if (status == 0)
		status = js_builtin_string(realm, wb_buffer_string(&text), result);
	wb_buffer_release(&text);
	if (status != 0)
		return status;

	/* Succeeded: the text. */
	return 0;
}

/* Calls a bound function: its target with the bound this and the bound arguments before the given ones. */
static int
function_bound_call(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *bound;
	struct vm_object *data;
	vm_value *all;
	unsigned all_count;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The bound function and its data. */
	bound = js_builtin_callee(realm);
	data = (struct vm_object *)vm_value_as_cell(bound->data);

	/* All the arguments, then the target's call. */
	status = function_bound_arguments(bound, args, count, &all, &all_count);
	if (status != 0)
		return status;
	status = vm_call(realm, data->elements[FUNCTION_BOUND_TARGET], data->elements[FUNCTION_BOUND_THIS], all, all_count, result);
	free(all);
	if (status != 0)
		return status;

	/* Succeeded: the target's result. */
	return 0;
}

/* Constructs with a bound function: the target, new.target the target when it was the bound function. */
static int
function_bound_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *bound;
	struct vm_object *data;
	vm_value new_target;
	vm_value self;
	vm_value *all;
	unsigned all_count;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The bound function, its data and new.target. */
	bound = js_builtin_callee(realm);
	data = (struct vm_object *)vm_value_as_cell(bound->data);
	new_target = realm->new_target;
	self = vm_value_cell(bound);
	if (new_target == self)
		new_target = data->elements[FUNCTION_BOUND_TARGET];

	/* All the arguments, then the target's construction. */
	status = function_bound_arguments(bound, args, count, &all, &all_count);
	if (status != 0)
		return status;
	status = vm_construct(realm, data->elements[FUNCTION_BOUND_TARGET], all, all_count, new_target, result);
	free(all);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* Makes the argument list of a bound function's call: the bound arguments, then the given ones (malloc'd). */
static int
function_bound_arguments(
	struct vm_function *bound,
	const vm_value *args,
	unsigned count,
	vm_value **all,
	unsigned *all_count)
{
	struct vm_object *data;
	unsigned bound_count;
	unsigned index;

	/* The list (the collector need not see it: the bound arguments are held by the data, the rest by the caller). */
	data = (struct vm_object *)vm_value_as_cell(bound->data);
	bound_count = data->length - FUNCTION_BOUND_ARGS;
	*all_count = bound_count + count;
	*all = calloc((size_t)*all_count + 1U, sizeof(vm_value));
	if (*all == NULL)
		return ENOMEM;

	/* The bound ones, then the given ones. */
	for (index = 0; index < bound_count; index++)
		(*all)[index] = data->elements[FUNCTION_BOUND_ARGS + index];
	for (index = 0; index < count; index++)
		(*all)[bound_count + index] = args[index];

	/* Succeeded: the list. */
	return 0;
}

/* Finds a bound function's length: the target's own number length less the bound arguments, at least 0. */
static int
function_bound_length(
	struct vm_realm *realm,
	vm_value target,
	unsigned bound_count,
	vm_value *length)
{
	struct vm_descriptor descriptor;
	vm_value key;
	vm_value value;
	double number;
	int found;
	int is_number;
	int status;

	/* 0 unless the target has its own length. */
	*length = vm_value_int32(0);
	key = vm_key_from_ascii(realm->heap, "length");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(target), key, &descriptor);
	if (!found)
		return 0;

	/* The target's length, when a number. */
	status = vm_get(realm, target, key, &value);
	if (status != 0)
		return status;
	is_number = vm_value_is_number(value);
	if (!is_number)
		return 0;

	/* Infinity stays; -Infinity is 0; otherwise the integer less the bound arguments. */
	number = vm_value_as_number(value);
	if (number == INFINITY) {
		*length = value;
		return 0;
	}

	/* The integer less the bound arguments. */
	status = js_builtin_integer(realm, value, &number);
	if (status != 0)
		return status;
	number -= (double)bound_count;
	if (number < 0.0)
		number = 0.0;

	/* Succeeded: the length. */
	*length = vm_value_number(number);
	return 0;
}

/* Makes a bound function's name: "bound " and the target's name when it is a string. */
static int
function_bound_name(
	struct vm_realm *realm,
	vm_value target,
	vm_value *name)
{
	struct vm_string *prefix;
	struct vm_string *joined;
	vm_value key;
	vm_value value;
	int is_string;
	int status;

	/* The target's name, or the empty string. */
	key = vm_key_from_ascii(realm->heap, "name");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_get(realm, target, key, &value);
	if (status != 0)
		return status;
	is_string = vm_value_is_string(value);
	if (!is_string) {
		status = js_builtin_string(realm, "", &value);
		if (status != 0)
			return status;
	}

	/* With the prefix. */
	prefix = vm_string_from_utf8(realm->heap, "bound ", 6);
	if (prefix == NULL)
		return ENOMEM;
	joined = vm_string_concat(realm->heap, prefix, (struct vm_string *)vm_value_as_cell(value));
	if (joined == NULL)
		return ENOMEM;

	/* Succeeded: the name. */
	*name = vm_value_cell(joined);
	return 0;
}

/* Appends ASCII text to a list of UTF-16 units. */
static int
function_append_text(
	struct wb_units *units,
	const char *text)
{
	uint16_t unit;
	int error;

	/* Each character as a unit. */
	error = 0;
	for (; error == 0 && *text != '\0'; text++) {
		unit = (uint16_t)(unsigned char)*text;
		error = wb_units_append(units, &unit, 1);
	}

	/* Reports a list that could not grow. */
	if (error != 0)
		return error;

	/* Succeeded: the text is appended. */
	return 0;
}

/* Function.prototype[Symbol.hasInstance](value): whether the value is an instance of this by its prototype chain. */
static int
function_has_instance(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	/* OrdinaryHasInstance. */
	status = vm_ordinary_has_instance(realm, this_value, js_argument(args, count, 0), result);
	if (status != 0)
		return status;

	/* Succeeded: the answer. */
	return 0;
}
