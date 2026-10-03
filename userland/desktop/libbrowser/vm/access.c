/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Properties of any value as the language reaches them: getting, setting
 * (with strict mode's errors), deleting, the in and instanceof operators,
 * an object literal's definitions, the global variables of scripts, and
 * the enumeration of for-in.
 *
 * The primitives other than strings get their prototypes (Number.prototype
 * and the rest) with the built-ins (ws074-p026); until then their
 * properties read undefined.
 */

#include "vm/internal.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/*
 * The state of one for-in loop: the keys it will visit, found when it
 * started, and how far it has come.
 *
 * The keys are a malloc'd array freed with the cell; a key is checked
 * again before it is visited, so one deleted meanwhile is skipped.  The
 * cell lives in a register of the loop's frame and is never a value the
 * script sees.
 */
struct access_for_in {
	struct vm_cell cell;
	vm_value object;
	vm_value *keys;
	uint32_t count;
	uint32_t index;
};

static void access_for_in_trace(struct vm_heap *heap, struct vm_cell *cell);
static void access_for_in_finalize(struct vm_heap *heap, struct vm_cell *cell);
static int access_string_property(struct vm_realm *realm, struct vm_string *string, vm_value key, vm_value *result, int *found);
static int access_set_length(struct vm_realm *realm, struct vm_object *array, vm_value value, int strict);
static int access_refuse(struct vm_realm *realm, int strict, const char *message);
static int access_collect(struct vm_heap *heap, struct vm_object *object, struct wb_vector *keys);
static int access_key_listed(const struct wb_vector *keys, size_t count, vm_value key);
static int access_is_symbol(vm_value value);
static struct vm_object *access_primitive_prototype(struct vm_realm *realm, vm_value value);
static int access_set_primitive(struct vm_realm *realm, vm_value base, vm_value key, vm_value value, int strict);
static int access_wrap_string(struct vm_realm *realm, struct vm_object *wrapper, struct vm_string *string);
static int access_descriptor_allowed(const struct vm_object *object, int found, const struct vm_descriptor *current, const struct vm_descriptor *descriptor);
static int access_define_length(struct vm_realm *realm, struct vm_object *array, const struct vm_descriptor *current, const struct vm_descriptor *descriptor, int *done);
static void access_get_length_attributes(struct vm_object *array, vm_value length_key, uint32_t *attributes);

/* The cell type of for-in states: they hold their object and keys. */
static const struct vm_cell_type access_for_in_type = {
	"for-in", access_for_in_trace, access_for_in_finalize
};

/*
 * Gets a property of any value: an object's through its chain (calling an
 * accessor's getter), a string's length and characters; reading from
 * undefined or null throws.
 */
int
vm_get(
	struct vm_realm *realm,
	vm_value base,
	vm_value key,
	vm_value *result)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	struct vm_object *holder;
	int is_object;
	int is_string;
	int found;
	int status;

	/* undefined and null have no properties. */
	*result = VM_VALUE_UNDEFINED;
	if (base == VM_VALUE_UNDEFINED || base == VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Cannot read properties of undefined or null");
		return status;
	}

	/* A string's own length and characters. */
	is_string = vm_value_is_string(base);
	if (is_string) {
		status = access_string_property(realm, (struct vm_string *)vm_value_as_cell(base), key, result, &found);
		if (status != 0)
			return status;
		if (found)
			return 0;
	}

	/* An object's chain, or a primitive's prototype's (the realm has none before its built-ins). */
	is_object = vm_value_is_object(base);
	holder = NULL;
	if (is_object) {
		holder = (struct vm_object *)vm_value_as_cell(base);
	} else {
		holder = access_primitive_prototype(realm, base);
	}

	/* Nothing to look in. */
	if (holder == NULL)
		return 0;

	/* The property, wherever on the chain. */
	found = vm_object_find(holder, key, &property);
	if (found < 0)
		return -found;

	/* A missing descriptor follows the absence path after errors have been excluded. */
	if (!found)
		return 0;

	/* A data property's value. */
	if ((property.attributes & VM_PROPERTY_ACCESSOR) == 0U) {
		*result = *property.value;
		return 0;
	}

	/* An accessor's getter, called with the base as this (no getter reads undefined). */
	accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
	if (accessor->getter == VM_VALUE_UNDEFINED)
		return 0;
	status = vm_call(realm, accessor->getter, base, NULL, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: the getter's result. */
	return 0;
}

/*
 * Reads a method of a value (GetMethod): undefined for undefined or null,
 * a TypeError for anything else that cannot be called.
 */
int
vm_get_method(
	struct vm_realm *realm,
	vm_value value,
	vm_value key,
	vm_value *method)
{
	int callable;
	int status;

	/* The property. */
	*method = VM_VALUE_UNDEFINED;
	status = vm_get(realm, value, key, method);
	if (status != 0)
		return status;

	/* Undefined and null say there is no method. */
	if (*method == VM_VALUE_UNDEFINED || *method == VM_VALUE_NULL) {
		*method = VM_VALUE_UNDEFINED;
		return 0;
	}

	/* Anything else must be callable. */
	callable = vm_value_is_callable(*method);
	if (!callable) {
		*method = VM_VALUE_UNDEFINED;
		status = vm_throw_type_error(realm, "method is not a function");
		return status;
	}

	/* Succeeded: the method. */
	return 0;
}

/*
 * Puts a property of any value as sloppy code does (a refused assignment
 * is ignored).
 */
int
vm_put(
	struct vm_realm *realm,
	vm_value base,
	vm_value key,
	vm_value value)
{
	int status;

	/* The assignment without strict mode's errors. */
	status = vm_set(realm, base, key, value, 0);
	if (status != 0)
		return status;

	/* Succeeded: the value is put (or refused in silence). */
	return 0;
}

/*
 * Puts a property of any value by assignment: an object's through an
 * accessor's setter or into a data property; undefined and null throw.  A
 * refused assignment (a read-only property, an accessor without a setter,
 * an object that takes no new property, a primitive) is ignored, or a
 * TypeError in strict code.
 */
int
vm_set(
	struct vm_realm *realm,
	vm_value base,
	vm_value key,
	vm_value value,
	int strict)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	struct vm_object *object;
	vm_value ignored;
	vm_value length_key;
	int is_object;
	int found;
	int done;
	int status;

	/* undefined and null have no properties. */
	if (base == VM_VALUE_UNDEFINED || base == VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Cannot set properties of undefined or null");
		return status;
	}

	/* A primitive takes no property, but its prototype's setter is called with it. */
	is_object = vm_value_is_object(base);
	if (!is_object) {
		status = access_set_primitive(realm, base, key, value, strict);
		return status;
	}

	/* An accessor on the chain: its setter is called with the value. */
	object = (struct vm_object *)vm_value_as_cell(base);
	found = vm_object_find(object, key, &property);
	if (found < 0)
		return -found;

	/* Present descriptors retain their ordinary accessor and attribute rules. */
	if (found && (property.attributes & VM_PROPERTY_ACCESSOR) != 0U) {
		accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
		if (accessor->setter == VM_VALUE_UNDEFINED) {
			status = access_refuse(realm, strict, "Cannot set property which has only a getter");
			return status;
		}

		/* The setter, called with the value. */
		status = vm_call(realm, accessor->setter, base, &value, 1, &ignored);
		if (status != 0)
			return status;
		return 0;
	}

	/* A read-only data property refuses. */
	if (found && (property.attributes & VM_PROPERTY_WRITABLE) == 0U) {
		status = access_refuse(realm, strict, "Cannot assign to read only property");
		return status;
	}

	/* An array's own length is set by its rules. */
	length_key = vm_key_from_ascii(realm->heap, "length");
	if (length_key == VM_VALUE_EMPTY)
		return ENOMEM;
	if (found && property.holder == object && key == length_key && (object->flags & VM_OBJECT_ARRAY) != 0U) {
		status = access_set_length(realm, object, value, strict);
		if (status != 0)
			return status;
		return 0;
	}

	/* Otherwise the ordinary assignment; an object that takes no new property refuses. */
	status = vm_object_set(realm->heap, object, key, value, &done);
	if (status != 0)
		return status;
	if (!done) {
		status = access_refuse(realm, strict, "Cannot add property, object is not extensible");
		return status;
	}

	/* Succeeded: the value is put. */
	return 0;
}

/*
 * Deletes a property of any value (the delete operator) and stores whether
 * it is gone; a property that stays is a TypeError in strict code.
 */
int
vm_delete(
	struct vm_realm *realm,
	vm_value base,
	vm_value key,
	int strict,
	vm_value *result)
{
	struct vm_object *object;
	struct vm_string *string;
	vm_value ignored;
	int is_string;
	int is_object;
	int found;
	int deleted;
	int status;

	/* undefined and null have no properties. */
	*result = VM_VALUE_TRUE;
	if (base == VM_VALUE_UNDEFINED || base == VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Cannot convert undefined or null to object");
		return status;
	}

	/* A string's length and characters stay; any other property of a primitive is not there. */
	is_object = vm_value_is_object(base);
	if (!is_object) {
		is_string = vm_value_is_string(base);
		found = 0;
		if (is_string) {
			string = (struct vm_string *)vm_value_as_cell(base);
			status = access_string_property(realm, string, key, &ignored, &found);
			if (status != 0)
				return status;
		}

		/* A string keeps its own properties. */
		if (found) {
			*result = VM_VALUE_FALSE;
			status = access_refuse(realm, strict, "Cannot delete property of a string");
			return status;
		}

		/* Any other property of a primitive is not there. */
		return 0;
	}

	/* An object's own property, unless it is not configurable. */
	object = (struct vm_object *)vm_value_as_cell(base);
	status = vm_object_delete(realm->heap, object, key, &deleted);
	if (status != 0)
		return status;
	if (!deleted) {
		*result = VM_VALUE_FALSE;
		status = access_refuse(realm, strict, "Cannot delete property");
		return status;
	}

	/* Succeeded: the property is gone. */
	return 0;
}

/*
 * Tells whether an object has a property on its chain (the in operator);
 * the right side must be an object.
 */
int
vm_in(
	struct vm_realm *realm,
	vm_value key,
	vm_value object,
	vm_value *result)
{
	struct vm_property property;
	vm_value property_key;
	int is_object;
	int found;
	int status;

	/* Only an object can be searched. */
	*result = VM_VALUE_FALSE;
	is_object = vm_value_is_object(object);
	if (!is_object) {
		status = vm_throw_type_error(realm, "Cannot use 'in' operator to search for a key in a primitive");
		return status;
	}

	/* The key as a property key. */
	status = vm_to_key(realm, key, &property_key);
	if (status != 0)
		return status;

	/* Succeeded: whether the chain has it. */
	found = vm_object_find((struct vm_object *)vm_value_as_cell(object), property_key, &property);
	if (found < 0)
		return -found;

	/* Successful descriptor inspection now supplies the requested presence or flag result. */
	*result = vm_value_boolean(found);
	return 0;
}

/*
 * Tells whether a value is an instance of a constructor (the instanceof
 * operator): whether the constructor's prototype is on the value's chain.
 */
int
vm_instanceof(
	struct vm_realm *realm,
	vm_value value,
	vm_value constructor,
	vm_value *result)
{
	vm_value method;
	vm_value answer;
	vm_value builtin;
	int ordinary;
	int truth;
	int callable;
	int is_object;
	int status;

	/* An object's Symbol.hasInstance answers for it, unless it is Function.prototype's own (ws074-p087). */
	*result = VM_VALUE_FALSE;
	is_object = vm_value_is_object(constructor);
	if (is_object) {
		status = vm_get_method(realm, constructor, vm_symbol_key(realm, VM_SYMBOL_HAS_INSTANCE), &method);
		if (status != 0)
			return status;
		ordinary = 0;
		if (realm->intrinsics[VM_INTRINSIC_HAS_INSTANCE] != NULL) {
			builtin = vm_value_cell(realm->intrinsics[VM_INTRINSIC_HAS_INSTANCE]);
			if (method == builtin)
				ordinary = 1;
		}

		/* A method of its own is called, its answer made a boolean. */
		if (method != VM_VALUE_UNDEFINED && !ordinary) {
			status = vm_call(realm, method, constructor, &value, 1, &answer);
			if (status != 0)
				return status;
			truth = vm_to_boolean(answer);
			*result = vm_value_boolean(truth);
			return 0;
		}
	}

	/* Otherwise only a callable right side answers instanceof. */
	callable = vm_value_is_callable(constructor);
	if (!callable) {
		status = vm_throw_type_error(realm, "Right-hand side of 'instanceof' is not callable");
		return status;
	}

	/* The prototype chain answers. */
	status = vm_ordinary_has_instance(realm, constructor, value, result);
	if (status != 0)
		return status;

	/* Succeeded: the answer is stored. */
	return 0;
}

/*
 * Tells whether a value is an instance of a constructor by its prototype
 * chain (OrdinaryHasInstance, Function.prototype[Symbol.hasInstance]): a
 * value that cannot be called has no instances.
 */
int
vm_ordinary_has_instance(
	struct vm_realm *realm,
	vm_value constructor,
	vm_value value,
	vm_value *result)
{
	struct vm_object *object;
	vm_value key;
	vm_value prototype;
	vm_value link;
	int callable;
	int is_object;
	int status;

	/* Only a function has instances. */
	*result = VM_VALUE_FALSE;
	callable = vm_value_is_callable(constructor);
	if (!callable)
		return 0;

	/* A primitive is an instance of nothing. */
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* The constructor's prototype, which must be an object. */
	key = vm_key_from_ascii(realm->heap, "prototype");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_get(realm, constructor, key, &prototype);
	if (status != 0)
		return status;
	is_object = vm_value_is_object(prototype);
	if (!is_object) {
		status = vm_throw_type_error(realm, "Function has non-object prototype in instanceof check");
		return status;
	}

	/* Walks the value's chain looking for it. */
	object = ((struct vm_object *)vm_value_as_cell(value))->prototype;
	while (object != NULL) {
		link = vm_value_cell(object);
		if (link == prototype) {
			*result = VM_VALUE_TRUE;
			return 0;
		}

		/* The next object of the chain. */
		object = object->prototype;
	}

	/* Succeeded: not an instance. */
	return 0;
}

/*
 * Defines a data property of an object literal (not an assignment: a
 * setter on the chain is not called).
 */
int
vm_define_data(
	struct vm_realm *realm,
	vm_value object,
	vm_value key,
	vm_value value)
{
	struct vm_object *target;
	int status;

	/* The property, writable, enumerable and configurable. */
	target = (struct vm_object *)vm_value_as_cell(object);
	status = vm_object_define(realm->heap, target, key, value, VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* Succeeded: the property is defined. */
	return 0;
}

/*
 * Defines the getter (or, with setter, the setter) of an object literal's
 * accessor property, keeping the other half when the property is an
 * accessor already.
 */
int
vm_define_accessor(
	struct vm_realm *realm,
	vm_value object,
	vm_value key,
	vm_value function,
	int setter)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	struct vm_object *target;
	struct vm_cell *accessor_root;
	vm_value getter_function;
	vm_value setter_function;
	int found;
	int status;

	/* The other half, from an own accessor already there. */
	target = (struct vm_object *)vm_value_as_cell(object);
	getter_function = VM_VALUE_UNDEFINED;
	setter_function = VM_VALUE_UNDEFINED;
	found = vm_object_get_own(target, key, &property);
	if (found < 0)
		return -found;

	/* Present descriptors retain their ordinary accessor and attribute rules. */
	if (found && (property.attributes & VM_PROPERTY_ACCESSOR) != 0U) {
		accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
		getter_function = accessor->getter;
		setter_function = accessor->setter;
	}

	/* This half. */
	if (setter) {
		setter_function = function;
	} else {
		getter_function = function;
	}

	/* The pair, as an enumerable and configurable accessor property. */
	accessor = vm_accessor_create(realm->heap, getter_function, setter_function);
	if (accessor == NULL)
		return ENOMEM;

	/* The new pair is not owned by the literal until definition publishes it. */
	accessor_root = &accessor->cell;
	status = vm_heap_add_root(realm->heap, &accessor_root);
	if (status != 0)
		return status;
	status = vm_object_define(realm->heap, target, key, vm_value_cell(accessor),
	    VM_PROPERTY_ACCESSOR | VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	vm_heap_remove_root(realm->heap, &accessor_root);
	if (status != 0)
		return status;

	/* Succeeded: the accessor is defined. */
	return 0;
}

/*
 * Reads a global variable; a missing one is a ReferenceError, or
 * undefined for typeof.
 */
int
vm_get_global(
	struct vm_realm *realm,
	vm_value key,
	int for_typeof,
	vm_value *result)
{
	struct vm_property property;
	int found;
	int status;

	/* A script's top-level let or const comes first; before its declaration runs it cannot be read (not even by typeof). */
	*result = VM_VALUE_UNDEFINED;
	found = vm_object_get_own_ordinary(realm->lexicals, key, &property);
	if (found && *property.value == VM_VALUE_EMPTY) {
		status = vm_throw_uninitialized(realm, key);
		return status;
	}

	/* An initialized one is read from the record. */
	if (found) {
		*result = *property.value;
		return 0;
	}

	/* A name the global object does not have. */
	found = vm_object_find(realm->global, key, &property);
	if (found < 0)
		return -found;

	/* A missing descriptor follows the absence path after errors have been excluded. */
	if (!found) {
		if (for_typeof)
			return 0;
		status = vm_throw_not_defined(realm, key);
		return status;
	}

	/* The property's value (through a getter). */
	status = vm_get(realm, vm_value_cell(realm->global), key, result);
	if (status != 0)
		return status;

	/* Succeeded: the variable's value. */
	return 0;
}

/*
 * Assigns a global variable: sloppy code makes a missing one; strict code
 * may not (a ReferenceError).
 */
int
vm_put_global(
	struct vm_realm *realm,
	vm_value key,
	vm_value value,
	int strict)
{
	struct vm_property property;
	int found;
	int status;

	/* A script's top-level let or const: not before its declaration runs, and never a const. */
	found = vm_object_get_own_ordinary(realm->lexicals, key, &property);
	if (found && *property.value == VM_VALUE_EMPTY) {
		status = vm_throw_uninitialized(realm, key);
		return status;
	}

	/* A const keeps its value. */
	if (found && (property.attributes & VM_PROPERTY_WRITABLE) == 0U) {
		status = vm_throw_type_error(realm, "Assignment to constant variable.");
		return status;
	}

	/* A let takes the new one. */
	if (found) {
		*property.value = value;
		return 0;
	}

	/* Strict code assigns only a variable that exists. */
	if (strict) {
		found = vm_object_find(realm->global, key, &property);
		if (found < 0)
			return -found;

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found) {
			status = vm_throw_not_defined(realm, key);
			return status;
		}
	}

	/* The assignment to the global object. */
	status = vm_set(realm, vm_value_cell(realm->global), key, value, strict);
	if (status != 0)
		return status;

	/* Succeeded: the variable has the value. */
	return 0;
}

/*
 * Declares a var of a script: a global property that cannot be deleted,
 * undefined unless the global object has the name already.
 */
int
vm_define_global_var(
	struct vm_realm *realm,
	vm_value key)
{
	struct vm_property property;
	int found;
	int status;

	/* A name the global object has keeps its value. */
	found = vm_object_get_own(realm->global, key, &property);
	if (found < 0)
		return -found;

	/* Present descriptors retain their ordinary accessor and attribute rules. */
	if (found)
		return 0;

	/* A new variable: writable and enumerable, not configurable. */
	status = vm_object_define(realm->heap, realm->global, key, VM_VALUE_UNDEFINED,
	    VM_PROPERTY_WRITABLE | VM_PROPERTY_ENUMERABLE);
	if (status == EPERM) {
		status = vm_throw_type_error(realm, "Cannot declare a global variable");
		return status;
	}

	/* Any other failure of the definition. */
	if (status != 0)
		return status;

	/* Succeeded: the variable exists. */
	return 0;
}

/*
 * Declares a function of a script: the global property takes the
 * function; a property there already must be configurable, or a writable
 * and enumerable data property.
 */
int
vm_define_global_function(
	struct vm_realm *realm,
	vm_value key,
	vm_value function)
{
	struct vm_property property;
	uint32_t attributes;
	int found;
	int replaceable;
	int status;

	/* A property there already decides whether the declaration may replace it. */
	attributes = VM_PROPERTY_WRITABLE | VM_PROPERTY_ENUMERABLE;
	found = vm_object_get_own(realm->global, key, &property);
	if (found < 0)
		return -found;

	/* Present descriptors retain their ordinary accessor and attribute rules. */
	if (found && (property.attributes & VM_PROPERTY_CONFIGURABLE) == 0U) {
		replaceable = 0;
		if ((property.attributes & VM_PROPERTY_ACCESSOR) == 0U &&
		    (property.attributes & VM_PROPERTY_WRITABLE) != 0U &&
		    (property.attributes & VM_PROPERTY_ENUMERABLE) != 0U)
			replaceable = 1;
		if (!replaceable) {
			status = vm_throw_type_error(realm, "Cannot redefine a global function");
			return status;
		}

		/* A variable of the script takes the function as its value. */
		attributes = property.attributes;
	}

	/* The property. */
	status = vm_object_define(realm->heap, realm->global, key, function, attributes);
	if (status == EPERM) {
		status = vm_throw_type_error(realm, "Cannot declare a global function");
		return status;
	}

	/* Any other failure of the definition. */
	if (status != 0)
		return status;

	/* Succeeded: the function is declared. */
	return 0;
}

/*
 * Deletes a global variable (delete of a bare name in sloppy code) and
 * stores whether it is gone.
 */
int
vm_delete_global(
	struct vm_realm *realm,
	vm_value key,
	vm_value *result)
{
	struct vm_property property;
	int found;
	int status;

	/* A script's top-level let or const cannot be deleted. */
	found = vm_object_get_own_ordinary(realm->lexicals, key, &property);
	if (found) {
		*result = VM_VALUE_FALSE;
		return 0;
	}

	/* The global object's property (a var of a script is not configurable and stays). */
	status = vm_delete(realm, vm_value_cell(realm->global), key, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: whether it is gone. */
	return 0;
}

/*
 * Declares a script's top-level let or const in the realm's record of
 * them, holding the empty value until its declaration runs.  A name the
 * record has already (from this or an earlier script) is a SyntaxError.
 */
int
vm_define_global_lexical(
	struct vm_realm *realm,
	vm_value key,
	int is_const)
{
	struct vm_property property;
	uint32_t attributes;
	int found;
	int status;

	/* A name declared already. */
	found = vm_object_get_own_ordinary(realm->lexicals, key, &property);
	if (found) {
		status = vm_throw_redeclared(realm, key);
		return status;
	}

	/* A const is a property that cannot be written; a let one that can. */
	attributes = VM_PROPERTY_ENUMERABLE;
	if (!is_const)
		attributes |= VM_PROPERTY_WRITABLE;
	status = vm_object_define(realm->heap, realm->lexicals, key, VM_VALUE_EMPTY, attributes);
	if (status != 0)
		return status;

	/* Succeeded: the name is declared, not yet initialized. */
	return 0;
}

/*
 * Runs the declaration of a script's top-level let or const: the binding
 * takes its first value (a const too).
 */
int
vm_init_global_lexical(
	struct vm_realm *realm,
	vm_value key,
	vm_value value)
{
	struct vm_property property;
	int found;

	/* The record has the name, which the script's prologue declared. */
	found = vm_object_get_own_ordinary(realm->lexicals, key, &property);
	if (!found)
		return EINVAL;

	/* Succeeded: the binding has its value. */
	*property.value = value;
	return 0;
}

/*
 * Converts a value to an object (ToObject): an object is itself; a
 * boolean, a number, a string or a symbol gets a wrapper object from its
 * prototype; undefined and null throw.
 */
int
vm_to_object(
	struct vm_realm *realm,
	vm_value value,
	vm_value *object)
{
	struct vm_object *wrapper;
	struct vm_object *prototype;
	struct vm_cell *wrapper_root;
	int is_object;
	int is_string;
	int is_boolean;
	int is_number;
	int status;

	/* An object is itself. */
	is_object = vm_value_is_object(value);
	if (is_object) {
		*object = value;
		return 0;
	}

	/* undefined and null have no object. */
	if (value == VM_VALUE_UNDEFINED || value == VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Cannot convert undefined or null to object");
		return status;
	}

	/* The wrapper, from the primitive's prototype (Object.prototype before the built-ins). */
	prototype = access_primitive_prototype(realm, value);
	if (prototype == NULL)
		prototype = realm->object_prototype;
	wrapper = vm_object_create(realm->heap, prototype);
	if (wrapper == NULL)
		return ENOMEM;

	/* A primitive wrapper is detached while its indexed characters are built. */
	wrapper_root = &wrapper->cell;
	status = vm_heap_add_root(realm->heap, &wrapper_root);
	if (status != 0)
		return status;
	wrapper->internal = value;

	/* Its kind by the primitive's type; a string's characters and length are its own properties. */
	is_string = vm_value_is_string(value);
	is_boolean = vm_value_is_boolean(value);
	is_number = vm_value_is_number(value);
	if (is_boolean) {
		wrapper->kind = VM_KIND_BOOLEAN;
	} else if (is_number) {
		wrapper->kind = VM_KIND_NUMBER;
	} else if (is_string) {
		wrapper->kind = VM_KIND_STRING;
		status = access_wrap_string(realm, wrapper, (struct vm_string *)vm_value_as_cell(value));
		if (status != 0) {
			vm_heap_remove_root(realm->heap, &wrapper_root);
			return status;
		}
	} else {
		wrapper->kind = VM_KIND_SYMBOL;
	}

	/* Succeeded: the wrapper. */
	*object = vm_value_cell(wrapper);
	vm_heap_remove_root(realm->heap, &wrapper_root);
	return 0;
}

/*
 * Reads an object's own property as a descriptor; reports whether it has
 * one.
 */
int
vm_get_own_descriptor(
	struct vm_object *object,
	vm_value key,
	struct vm_descriptor *descriptor)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	int found;

	/* The own property. */
	memset(descriptor, 0, sizeof(*descriptor));
	found = vm_object_get_own(object, key, &property);
	if (found < 0)
		return found;

	/* A missing descriptor follows the absence path after errors have been excluded. */
	if (!found)
		return 0;

	/* An accessor's getter and setter, or a data property's value and writability. */
	descriptor->attributes = property.attributes & (VM_PROPERTY_WRITABLE | VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	descriptor->has = VM_HAS_ENUMERABLE | VM_HAS_CONFIGURABLE;
	if ((property.attributes & VM_PROPERTY_ACCESSOR) != 0U) {
		accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
		descriptor->getter = accessor->getter;
		descriptor->setter = accessor->setter;
		descriptor->has |= VM_HAS_GET | VM_HAS_SET;
		descriptor->attributes &= ~VM_PROPERTY_WRITABLE;
	} else {
		descriptor->value = *property.value;
		descriptor->has |= VM_HAS_VALUE | VM_HAS_WRITABLE;
	}

	/* It has the property. */
	return 1;
}

/*
 * Defines an object's own property from a descriptor as the language
 * validates it (ValidateAndApplyPropertyDescriptor, and an array's length
 * and indices); done says whether it was allowed.
 */
int
vm_define_own_property(
	struct vm_realm *realm,
	struct vm_object *object,
	vm_value key,
	const struct vm_descriptor *descriptor,
	int *done)
{
	struct vm_descriptor current;
	struct vm_accessor *accessor;
	struct vm_cell *accessor_root;
	vm_value length_key;
	vm_value getter;
	vm_value setter;
	vm_value value;
	uint32_t attributes;
	uint32_t index;
	int found;
	int allowed;
	int is_index;
	int is_accessor;
	int is_array;
	int status;
	int handled;

	/* Native policy may reject or handle a descriptor before ordinary validation. */
	*done = 0;
	if (object->native_operations != NULL && object->native_operations->define != NULL) {
		handled = 0;
		status = object->native_operations->define(realm, object, key, descriptor, &handled, done);
		if (status != 0)
			return status;
		if (handled)
			return 0;
	}

	/* The property there now. */
	*done = 0;
	found = vm_get_own_descriptor(object, key, &current);
	if (found < 0)
		return -found;

	/* Array descriptor handling resolves the separate length metadata key. */
	length_key = vm_key_from_ascii(realm->heap, "length");
	if (length_key == VM_VALUE_EMPTY)
		return ENOMEM;

	/* An array's length has rules of its own. */
	is_array = 0;
	if ((object->flags & VM_OBJECT_ARRAY) != 0U)
		is_array = 1;
	if (is_array && key == length_key) {
		status = access_define_length(realm, object, &current, descriptor, done);
		if (status != 0)
			return status;
		return 0;
	}

	/* An array takes no index past a length that cannot change. */
	is_index = vm_value_is_array_index(key, &index);
	if (is_array && is_index && !found && index >= object->length) {
		access_get_length_attributes(object, length_key, &attributes);
		if ((attributes & VM_PROPERTY_WRITABLE) == 0U)
			return 0;
	}

	/* Whether the change is allowed from what is there. */
	allowed = access_descriptor_allowed(object, found, &current, descriptor);
	if (!allowed)
		return 0;

	/* The kind after the change: the descriptor's, or (for a generic one) what is there. */
	is_accessor = 0;
	if ((descriptor->has & (VM_HAS_GET | VM_HAS_SET)) != 0U)
		is_accessor = 1;
	if ((descriptor->has & (VM_HAS_VALUE | VM_HAS_WRITABLE | VM_HAS_GET | VM_HAS_SET)) == 0U && found && (current.has & VM_HAS_GET) != 0U)
		is_accessor = 1;

	/* The enumerability and configurability: given, or kept, or false. */
	attributes = 0;
	if (found)
		attributes = current.attributes & (VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	if ((descriptor->has & VM_HAS_ENUMERABLE) != 0U)
		attributes = (attributes & ~VM_PROPERTY_ENUMERABLE) | (descriptor->attributes & VM_PROPERTY_ENUMERABLE);
	if ((descriptor->has & VM_HAS_CONFIGURABLE) != 0U)
		attributes = (attributes & ~VM_PROPERTY_CONFIGURABLE) | (descriptor->attributes & VM_PROPERTY_CONFIGURABLE);

	/* An accessor: the getter and setter given, or those it had. */
	if (is_accessor) {
		getter = VM_VALUE_UNDEFINED;
		setter = VM_VALUE_UNDEFINED;
		if (found && (current.has & VM_HAS_GET) != 0U) {
			getter = current.getter;
			setter = current.setter;
		}

		/* The ones given. */
		if ((descriptor->has & VM_HAS_GET) != 0U)
			getter = descriptor->getter;
		if ((descriptor->has & VM_HAS_SET) != 0U)
			setter = descriptor->setter;
		accessor = vm_accessor_create(realm->heap, getter, setter);
		if (accessor == NULL)
			return ENOMEM;

		/* The descriptor owns the pair only after the property is published. */
		accessor_root = &accessor->cell;
		status = vm_heap_add_root(realm->heap, &accessor_root);
		if (status != 0)
			return status;
		status = vm_object_define(realm->heap, object, key, vm_value_cell(accessor), attributes | VM_PROPERTY_ACCESSOR);
		vm_heap_remove_root(realm->heap, &accessor_root);
		if (status != 0)
			return status;
		*done = 1;
		return 0;
	}

	/* A data property: the value and writability given, or those it had. */
	value = VM_VALUE_UNDEFINED;
	if (found && (current.has & VM_HAS_VALUE) != 0U) {
		value = current.value;
		attributes |= current.attributes & VM_PROPERTY_WRITABLE;
	}

	/* The ones given. */
	if ((descriptor->has & VM_HAS_VALUE) != 0U)
		value = descriptor->value;
	if ((descriptor->has & VM_HAS_WRITABLE) != 0U)
		attributes = (attributes & ~VM_PROPERTY_WRITABLE) | (descriptor->attributes & VM_PROPERTY_WRITABLE);
	status = vm_object_define(realm->heap, object, key, value, attributes);
	if (status != 0)
		return status;

	/* Succeeded: the property is defined. */
	*done = 1;
	return 0;
}

/*
 * Tells whether two values are the same value (SameValue: NaN is itself,
 * +0 and -0 differ).
 */
int
vm_same_value(
	vm_value left,
	vm_value right)
{
	double left_number;
	double right_number;
	int left_is_number;
	int right_is_number;
	int left_negative;
	int right_negative;
	int equal;

	/* Numbers: NaN equals NaN, and the signs of zeros count. */
	left_is_number = vm_value_is_number(left);
	right_is_number = vm_value_is_number(right);
	if (left_is_number && right_is_number) {
		left_number = vm_value_as_number(left);
		right_number = vm_value_as_number(right);
		if (left_number != left_number && right_number != right_number)
			return 1;
		if (left_number == 0.0 && right_number == 0.0) {
			left_negative = signbit(left_number) != 0;
			right_negative = signbit(right_number) != 0;
			if (left_negative == right_negative)
				return 1;
			return 0;
		}

		/* Other numbers by value. */
		if (left_number == right_number)
			return 1;
		return 0;
	}

	/* Anything else as ===. */
	equal = vm_strict_equals(left, right);
	if (equal)
		return 1;

	/* Different values. */
	return 0;
}

/*
 * Starts a for-in loop over a value: lists the enumerable string keys of
 * an object and its chain (a key shadowed by a nearer property is listed
 * once, by the nearer one), or a string's indices; undefined, null and the
 * other primitives list nothing.
 */
int
vm_for_in_start(
	struct vm_realm *realm,
	vm_value value,
	vm_value *iterator)
{
	struct access_for_in *state;
	struct wb_vector keys;
	struct vm_string *string;
	vm_value key;
	uint32_t index;
	int is_object;
	int is_string;
	int status;

	/* The keys of the object's chain, or of the string. */
	wb_vector_init(&keys, sizeof(vm_value));
	is_object = vm_value_is_object(value);
	is_string = vm_value_is_string(value);
	status = 0;
	if (is_object)
		status = access_collect(realm->heap, (struct vm_object *)vm_value_as_cell(value), &keys);
	if (is_string) {
		string = (struct vm_string *)vm_value_as_cell(value);
		for (index = 0; status == 0 && index < string->length; index++) {
			key = vm_value_int32((int32_t)index);
			status = wb_vector_push(&keys, &key);
		}
	}

	/* A failure leaves nothing behind. */
	if (status != 0) {
		wb_vector_release(&keys);
		return status;
	}

	/* The loop's state, which takes the keys over. */
	state = vm_heap_alloc(realm->heap, &access_for_in_type, sizeof(*state));
	if (state == NULL) {
		wb_vector_release(&keys);
		return ENOMEM;
	}

	/* The keys, from the first. */
	state->object = value;
	state->keys = keys.items;
	state->count = (uint32_t)keys.count;
	state->index = 0;

	/* Succeeded: the state as the loop's iterator. */
	*iterator = vm_value_cell(state);
	return 0;
}

/*
 * Moves a for-in loop to its next key (as a string), skipping the keys
 * deleted since it started; done when there is none left.
 */
int
vm_for_in_next(
	struct vm_realm *realm,
	vm_value iterator,
	vm_value *key,
	int *done)
{
	struct access_for_in *state;
	struct vm_property property;
	struct vm_string *string;
	vm_value candidate;
	int is_object;
	int found;
	int status;

	/* The state; anything else in the register is a fault of the code. */
	*done = 1;
	state = (struct access_for_in *)vm_value_as_cell(iterator);
	if (state->cell.type != &access_for_in_type)
		return EINVAL;

	/* The next key the object still has. */
	is_object = vm_value_is_object(state->object);
	while (state->index < state->count) {
		candidate = state->keys[state->index];
		state->index++;
		if (is_object) {
			found = vm_object_find((struct vm_object *)vm_value_as_cell(state->object), candidate, &property);
			if (found < 0)
				return -found;

			/* A missing descriptor follows the absence path after errors have been excluded. */
			if (!found)
				continue;
		}

		/* The key as a string. */
		status = vm_to_string(realm, candidate, &string);
		if (status != 0)
			return status;

		/* Succeeded: the key. */
		*key = vm_value_cell(string);
		*done = 0;
		return 0;
	}

	/* Succeeded: the loop is over. */
	return 0;
}

/* Marks what a for-in state holds: its object and its keys. */
static void
access_for_in_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct access_for_in *state;
	uint32_t index;

	/* The object, then each key. */
	state = (struct access_for_in *)cell;
	vm_heap_mark_value(heap, state->object);
	for (index = 0; state->keys != NULL && index < state->count; index++)
		vm_heap_mark_value(heap, state->keys[index]);
}

/* Frees a dead for-in state's keys. */
static void
access_for_in_finalize(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct access_for_in *state;

	UNUSED_PARAMETER(heap);

	/* The array of keys. */
	state = (struct access_for_in *)cell;
	free(state->keys);
	state->keys = NULL;
}

/* Reads a string's own property: its length, or a character by index; found says whether it has the key. */
static int
access_string_property(
	struct vm_realm *realm,
	struct vm_string *string,
	vm_value key,
	vm_value *result,
	int *found)
{
	struct vm_string *character;
	vm_value length_key;
	uint32_t index;
	uint16_t unit;
	int is_index;

	/* The length. */
	*found = 0;
	*result = VM_VALUE_UNDEFINED;
	length_key = vm_key_from_ascii(realm->heap, "length");
	if (length_key == VM_VALUE_EMPTY)
		return ENOMEM;
	if (key == length_key) {
		*result = vm_value_int32((int32_t)string->length);
		*found = 1;
		return 0;
	}

	/* A character is a one-unit string. */
	is_index = vm_value_is_array_index(key, &index);
	if (is_index && index < string->length) {
		unit = vm_string_at(string, index);
		character = vm_string_from_units(realm->heap, &unit, 1);
		if (character == NULL)
			return ENOMEM;
		*result = vm_value_cell(character);
		*found = 1;
	}

	/* Succeeded: the property, or nothing. */
	return 0;
}

/* Sets an array's length by assignment: a number that is a valid length, or a RangeError. */
static int
access_set_length(
	struct vm_realm *realm,
	struct vm_object *array,
	vm_value value,
	int strict)
{
	double number;
	uint32_t length;
	int status;

	UNUSED_PARAMETER(strict);

	/* The new length must be a whole number in uint32's range. */
	status = vm_to_uint32(realm, value, &length);
	if (status != 0)
		return status;
	status = vm_to_number(realm, value, &number);
	if (status != 0)
		return status;
	if ((double)length != number) {
		status = vm_throw_range_error(realm, "Invalid array length");
		return status;
	}

	/* The array grows or loses its elements past the length. */
	status = vm_array_set_length(realm->heap, array, length);
	if (status != 0)
		return status;

	/* Succeeded: the length is set. */
	return 0;
}

/* Refuses an assignment or a deletion: nothing in sloppy code, a TypeError in strict code. */
static int
access_refuse(
	struct vm_realm *realm,
	int strict,
	const char *message)
{
	int status;

	/* Sloppy code ignores it. */
	if (!strict)
		return 0;

	/* Strict code throws. */
	status = vm_throw_type_error(realm, message);
	return status;
}

/* Lists the enumerable string keys of an object and its chain, each once, the nearest first. */
static int
access_collect(
	struct vm_heap *heap,
	struct vm_object *object,
	struct wb_vector *keys)
{
	struct wb_vector seen;
	struct wb_vector own;
	struct vm_property property;
	vm_value key;
	size_t index;
	size_t seen_before;
	int is_symbol;
	int listed;
	int found;
	int status;

	/* Every key met so far (enumerable or not, since a non-enumerable one still shadows). */
	wb_vector_init(&seen, sizeof(vm_value));
	wb_vector_init(&own, sizeof(vm_value));
	status = 0;

	/* Each object of the chain, the nearest first. */
	for (;
	     status == 0 && object != NULL;
	     object = object->prototype) {
		wb_vector_clear(&own);
		status = vm_object_own_keys(heap, object, &own);
		if (status != 0)
			break;

		/* Each own key: symbols are never listed, and one met on a nearer object is shadowed. */
		seen_before = seen.count;
		for (index = 0; status == 0 && index < own.count; index++) {
			key = *(vm_value *)wb_vector_at(&own, index);
			is_symbol = access_is_symbol(key);
			if (is_symbol)
				continue;
			listed = access_key_listed(&seen, seen_before, key);
			if (listed)
				continue;

			/* The key is met; it is listed when enumerable. */
			status = wb_vector_push(&seen, &key);
			if (status != 0)
				break;
			found = vm_object_get_own(object, key, &property);
			if (found < 0) {
				status = -found;
				break;
			}

			/* Present descriptors retain their ordinary accessor and attribute rules. */
			if (found && (property.attributes & VM_PROPERTY_ENUMERABLE) != 0U)
				status = wb_vector_push(keys, &key);
		}
	}

	/* The working lists are no longer needed. */
	wb_vector_release(&seen);
	wb_vector_release(&own);
	if (status != 0)
		return status;

	/* Succeeded: the keys are listed. */
	return 0;
}

/* Tells whether a key is among the first count keys of a list. */
static int
access_key_listed(
	const struct wb_vector *keys,
	size_t count,
	vm_value key)
{
	vm_value listed;
	size_t index;

	/* Keys are indices, atoms or symbols, which compare by value. */
	for (index = 0; index < count; index++) {
		listed = *(vm_value *)wb_vector_at(keys, index);
		if (listed == key)
			return 1;
	}

	/* Not listed. */
	return 0;
}

/* Tells whether a key is a symbol. */
static int
access_is_symbol(
	vm_value value)
{
	struct vm_cell *cell;
	int is_cell;

	/* Only cells are symbols. */
	is_cell = vm_value_is_cell(value);
	if (!is_cell)
		return 0;

	/* A symbol cell. */
	cell = vm_value_as_cell(value);
	if (cell->type == &vm_symbol_type)
		return 1;

	/* Another cell. */
	return 0;
}

/* Finds the prototype a primitive's properties come from (NULL for undefined and null, and before the built-ins). */
static struct vm_object *
access_primitive_prototype(
	struct vm_realm *realm,
	vm_value value)
{
	int is_boolean;
	int is_number;
	int is_string;
	int is_symbol;

	/* A boolean. */
	is_boolean = vm_value_is_boolean(value);
	if (is_boolean)
		return realm->intrinsics[VM_INTRINSIC_BOOLEAN_PROTOTYPE];

	/* A number. */
	is_number = vm_value_is_number(value);
	if (is_number)
		return realm->intrinsics[VM_INTRINSIC_NUMBER_PROTOTYPE];

	/* A string. */
	is_string = vm_value_is_string(value);
	if (is_string)
		return realm->intrinsics[VM_INTRINSIC_STRING_PROTOTYPE];

	/* A symbol. */
	is_symbol = access_is_symbol(value);
	if (is_symbol)
		return realm->intrinsics[VM_INTRINSIC_SYMBOL_PROTOTYPE];

	/* undefined, null. */
	return NULL;
}

/* Assigns a property of a primitive: a setter on its prototype's chain is called with it; anything else is refused. */
static int
access_set_primitive(
	struct vm_realm *realm,
	vm_value base,
	vm_value key,
	vm_value value,
	int strict)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	struct vm_object *prototype;
	vm_value ignored;
	int found;
	int status;

	/* A setter on the prototype's chain. */
	prototype = access_primitive_prototype(realm, base);
	found = 0;
	if (prototype != NULL) {
		found = vm_object_find(prototype, key, &property);
		if (found < 0)
			return -found;
	}

	/* Present descriptors retain their ordinary accessor and attribute rules. */
	if (found && (property.attributes & VM_PROPERTY_ACCESSOR) != 0U) {
		accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
		if (accessor->setter != VM_VALUE_UNDEFINED) {
			status = vm_call(realm, accessor->setter, base, &value, 1, &ignored);
			return status;
		}
	}

	/* Anything else: the primitive takes no property. */
	status = access_refuse(realm, strict, "Cannot create property on primitive value");
	return status;
}

/* Gives a String wrapper its characters (enumerable, read-only) and its length (read-only). */
static int
access_wrap_string(
	struct vm_realm *realm,
	struct vm_object *wrapper,
	struct vm_string *string)
{
	struct vm_string *character;
	vm_value key;
	uint32_t index;
	uint16_t unit;
	int status;

	/* Each character as an index property. */
	for (index = 0; index < string->length; index++) {
		unit = vm_string_at(string, index);
		character = vm_string_from_units(realm->heap, &unit, 1);
		if (character == NULL)
			return ENOMEM;
		status = vm_object_define(realm->heap, wrapper, vm_value_int32((int32_t)index), vm_value_cell(character),
		    VM_PROPERTY_ENUMERABLE);
		if (status != 0)
			return status;
	}

	/* The length. */
	key = vm_key_from_ascii(realm->heap, "length");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_object_define(realm->heap, wrapper, key, vm_value_int32((int32_t)string->length), 0);
	if (status != 0)
		return status;

	/* Succeeded: the wrapper has the string's properties. */
	return 0;
}

/* Tells whether a descriptor may be applied over what an object has (ValidateAndApplyPropertyDescriptor's refusals). */
static int
access_descriptor_allowed(
	const struct vm_object *object,
	int found,
	const struct vm_descriptor *current,
	const struct vm_descriptor *descriptor)
{
	int is_accessor;
	int was_accessor;
	int generic;
	int same;

	/* A new property needs an object that takes new ones. */
	if (!found) {
		if ((object->flags & VM_OBJECT_NOT_EXTENSIBLE) != 0U)
			return 0;
		return 1;
	}

	/* A configurable property may become anything. */
	if ((current->attributes & VM_PROPERTY_CONFIGURABLE) != 0U)
		return 1;

	/* A property that is not configurable stays so and keeps its enumerability. */
	if ((descriptor->has & VM_HAS_CONFIGURABLE) != 0U && (descriptor->attributes & VM_PROPERTY_CONFIGURABLE) != 0U)
		return 0;
	if ((descriptor->has & VM_HAS_ENUMERABLE) != 0U &&
	    (descriptor->attributes & VM_PROPERTY_ENUMERABLE) != (current->attributes & VM_PROPERTY_ENUMERABLE))
		return 0;

	/* It keeps its kind. */
	is_accessor = 0;
	if ((descriptor->has & (VM_HAS_GET | VM_HAS_SET)) != 0U)
		is_accessor = 1;
	generic = 0;
	if ((descriptor->has & (VM_HAS_VALUE | VM_HAS_WRITABLE | VM_HAS_GET | VM_HAS_SET)) == 0U)
		generic = 1;
	was_accessor = 0;
	if ((current->has & VM_HAS_GET) != 0U)
		was_accessor = 1;
	if (!generic && is_accessor != was_accessor)
		return 0;

	/* An accessor keeps its getter and setter. */
	if (was_accessor) {
		if ((descriptor->has & VM_HAS_GET) != 0U) {
			same = vm_same_value(descriptor->getter, current->getter);
			if (!same)
				return 0;
		}

		/* The setter too. */
		if ((descriptor->has & VM_HAS_SET) != 0U) {
			same = vm_same_value(descriptor->setter, current->setter);
			if (!same)
				return 0;
		}

		/* It keeps them. */
		return 1;
	}

	/* A read-only data property stays read-only with its value. */
	if ((current->attributes & VM_PROPERTY_WRITABLE) == 0U) {
		if ((descriptor->has & VM_HAS_WRITABLE) != 0U && (descriptor->attributes & VM_PROPERTY_WRITABLE) != 0U)
			return 0;
		if ((descriptor->has & VM_HAS_VALUE) != 0U) {
			same = vm_same_value(descriptor->value, current->value);
			if (!same)
				return 0;
		}
	}

	/* Allowed. */
	return 1;
}

/* Defines an array's length (ArraySetLength): a valid length, the elements past it removed, and its writability. */
static int
access_define_length(
	struct vm_realm *realm,
	struct vm_object *array,
	const struct vm_descriptor *current,
	const struct vm_descriptor *descriptor,
	int *done)
{
	struct vm_descriptor rest;
	vm_value length_key;
	double number;
	uint32_t length;
	int allowed;
	int status;

	/* The new length, when given: a whole number in uint32's range. */
	*done = 0;
	length_key = vm_key_from_ascii(realm->heap, "length");
	if (length_key == VM_VALUE_EMPTY)
		return ENOMEM;
	rest = *descriptor;
	length = array->length;
	if ((descriptor->has & VM_HAS_VALUE) != 0U) {
		status = vm_to_uint32(realm, descriptor->value, &length);
		if (status != 0)
			return status;
		status = vm_to_number(realm, descriptor->value, &number);
		if (status != 0)
			return status;
		if ((double)length != number) {
			status = vm_throw_range_error(realm, "Invalid array length");
			return status;
		}

		/* The value as the number it is. */
		rest.value = vm_value_number((double)length);
	}

	/* The change must be allowed as for any property, and a read-only length cannot change. */
	allowed = access_descriptor_allowed(array, 1, current, &rest);
	if (!allowed)
		return 0;
	if ((current->attributes & VM_PROPERTY_WRITABLE) == 0U && length != array->length)
		return 0;

	/* The new length (the elements past it go). */
	if (length != array->length) {
		status = vm_array_set_length(realm->heap, array, length);
		if (status != 0)
			return status;
	}

	/* A length made read-only stays so. */
	if ((descriptor->has & VM_HAS_WRITABLE) != 0U && (descriptor->attributes & VM_PROPERTY_WRITABLE) == 0U) {
		status = vm_object_define(realm->heap, array, length_key, vm_value_number((double)array->length), 0);
		if (status != 0)
			return status;
	}

	/* Succeeded: the length is defined. */
	*done = 1;
	return 0;
}

/* Reports the attributes of an array's length property. */
static void
access_get_length_attributes(
	struct vm_object *array,
	vm_value length_key,
	uint32_t *attributes)
{
	struct vm_property property;
	int found;

	/* The property (an array always has it). */
	*attributes = VM_PROPERTY_WRITABLE;
	found = vm_object_get_own_ordinary(array, length_key, &property);
	if (found)
		*attributes = property.attributes;
}
