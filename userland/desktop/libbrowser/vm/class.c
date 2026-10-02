/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Classes (ws074-p080): making a class's constructor and prototype from
 * its heritage, defining its methods and accessors, and reading a
 * property through super.
 *
 * A method's home object (the prototype, or the constructor for a static
 * one) is kept in its function's data: super.x reads x from the home
 * object's prototype with the method's this as the receiver.
 */

#include "vm/bytecode.h"
#include "vm/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int class_private_find(struct vm_realm *realm, vm_value object, vm_value key, struct vm_property *property, int *found);

/*
 * Sets up a class: its prototype object (whose prototype is the parent's
 * prototype, Object.prototype without a heritage, or null for extends
 * null), the constructor's prototype property and the prototype's
 * constructor, and the constructor's own prototype (the parent for
 * extends).  parent is the empty value without a heritage.
 */
int
vm_class_setup(
	struct vm_realm *realm,
	vm_value constructor,
	vm_value parent,
	vm_value *prototype)
{
	struct vm_function *function;
	struct vm_object *made;
	struct vm_object *proto_parent;
	struct vm_object *constructor_parent;
	vm_value parent_prototype;
	vm_value key;
	int is_constructor;
	int is_object;
	int status;

	/* Without a heritage: Object.prototype and Function.prototype. */
	proto_parent = realm->object_prototype;
	constructor_parent = realm->function_prototype;

	/* extends null: a prototype with no prototype, and an ordinary constructor. */
	if (parent == VM_VALUE_NULL)
		proto_parent = NULL;

	/* extends a constructor: its prototype property (an object or null) and itself. */
	if (parent != VM_VALUE_EMPTY && parent != VM_VALUE_NULL) {
		is_constructor = vm_value_is_constructor(parent);
		if (!is_constructor) {
			status = vm_throw_type_error(realm, "Class extends value is not a constructor or null");
			return status;
		}

		/* Its prototype property, an object or null. */
		key = vm_key_from_ascii(realm->heap, "prototype");
		if (key == VM_VALUE_EMPTY)
			return ENOMEM;
		status = vm_get(realm, parent, key, &parent_prototype);
		if (status != 0)
			return status;
		is_object = vm_value_is_object(parent_prototype);
		if (!is_object && parent_prototype != VM_VALUE_NULL) {
			status = vm_throw_type_error(realm, "Class extends value does not have valid prototype property");
			return status;
		}

		/* The chains. */
		proto_parent = NULL;
		if (is_object)
			proto_parent = (struct vm_object *)vm_value_as_cell(parent_prototype);
		constructor_parent = (struct vm_object *)vm_value_as_cell(parent);
	}

	/* The prototype object, the constructor's home object too (for super in it). */
	made = vm_object_create(realm->heap, proto_parent);
	if (made == NULL)
		return ENOMEM;
	function = (struct vm_function *)vm_value_as_cell(constructor);
	function->object.prototype = constructor_parent;
	function->data = vm_value_cell(made);

	/* constructor.prototype, fixed. */
	key = vm_key_from_ascii(realm->heap, "prototype");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_object_define(realm->heap, &function->object, key, vm_value_cell(made), 0);
	if (status != 0)
		return status;

	/* prototype.constructor, not enumerable. */
	key = vm_key_from_ascii(realm->heap, "constructor");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_object_define(realm->heap, made, key, constructor, VM_PROPERTY_WRITABLE | VM_PROPERTY_CONFIGURABLE);
	if (status != 0)
		return status;

	/* Succeeded: the prototype object. */
	*prototype = vm_value_cell(made);
	return 0;
}

/*
 * Defines a class's method (kind 0), getter (1) or setter (2) on its home
 * object, not enumerable, and makes that object the function's home
 * object.
 */
int
vm_define_method(
	struct vm_realm *realm,
	vm_value home,
	vm_value key,
	vm_value function,
	uint32_t kind)
{
	struct vm_function *method;
	struct vm_object *object;
	struct vm_property property;
	struct vm_accessor *accessor;
	vm_value getter;
	vm_value setter;
	int found;
	int status;

	/* The function remembers where it lives (a bytecode method's data is its home object). */
	method = (struct vm_function *)vm_value_as_cell(function);
	if (method->code != NULL)
		method->data = home;

	/* A method: a writable, configurable data property. */
	object = (struct vm_object *)vm_value_as_cell(home);
	if (kind == 0U) {
		status = vm_object_define(realm->heap, object, key, function, VM_PROPERTY_WRITABLE | VM_PROPERTY_CONFIGURABLE);
		if (status != 0)
			return status;
		return 0;
	}

	/* An accessor's half joins the other half already there. */
	getter = VM_VALUE_UNDEFINED;
	setter = VM_VALUE_UNDEFINED;
	found = vm_object_get_own(object, key, &property);
	if (found < 0)
		return -found;

	/* Present descriptors retain their ordinary accessor and attribute rules. */
	if (found && (property.attributes & VM_PROPERTY_ACCESSOR) != 0U) {
		accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
		getter = accessor->getter;
		setter = accessor->setter;
	}

	/* The new half replaces its own. */
	if (kind == 1U) {
		getter = function;
	} else {
		setter = function;
	}

	/* The accessor property, configurable and not enumerable. */
	accessor = vm_accessor_create(realm->heap, getter, setter);
	if (accessor == NULL)
		return ENOMEM;
	status = vm_object_define(realm->heap, object, key, vm_value_cell(accessor), VM_PROPERTY_ACCESSOR | VM_PROPERTY_CONFIGURABLE);
	if (status != 0)
		return status;

	/* Succeeded: the accessor is defined. */
	return 0;
}

/*
 * Reads super.key in a method whose home object is home: the property of
 * the home object's prototype, a getter called with the method's this.
 */
int
vm_get_super(
	struct vm_realm *realm,
	vm_value home,
	vm_value key,
	vm_value this_value,
	vm_value *result)
{
	struct vm_object *object;
	struct vm_property property;
	struct vm_accessor *accessor;
	int is_object;
	int found;
	int status;

	/* A home object is needed (a method's), and its prototype may be null. */
	*result = VM_VALUE_UNDEFINED;
	is_object = vm_value_is_object(home);
	if (!is_object) {
		status = vm_throw_error(realm, VM_ERROR_SYNTAX, "'super' keyword unexpected here");
		return status;
	}

	/* The home object's prototype, where super looks. */
	object = ((struct vm_object *)vm_value_as_cell(home))->prototype;
	if (object == NULL) {
		status = vm_throw_type_error(realm, "Cannot read properties of null");
		return status;
	}

	/* The property on the prototype's chain. */
	found = vm_object_find(object, key, &property);
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

	/* A getter, called with the method's this (no getter reads undefined). */
	accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
	if (accessor->getter == VM_VALUE_UNDEFINED)
		return 0;
	status = vm_call(realm, accessor->getter, this_value, NULL, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: the getter's value. */
	return 0;
}

/* Throws the TypeError of a class's constructor called without new, naming the class. */
int
vm_throw_class_call(
	struct vm_realm *realm,
	struct vm_function *function)
{
	struct wb_buffer name;
	char text[200];
	int status;

	/* The class's name as UTF-8 (a constructor's code is named after its class). */
	wb_buffer_init(&name);
	if (function->code->name != NULL) {
		status = vm_string_to_utf8(function->code->name, &name);
		if (status != 0) {
			wb_buffer_release(&name);
			return status;
		}
	}

	/* The message, as Chromium words it. */
	snprintf(text, sizeof(text), "Class constructor %.120s cannot be invoked without 'new'", wb_buffer_string(&name));
	wb_buffer_release(&name);
	status = vm_throw_type_error(realm, text);

	/* Reports the throw. */
	return status;
}

/*
 * Reads a private member (obj.#x): only the object's own, a getter called
 * with the object; a TypeError when the object does not have it (its class
 * did not make it).
 */
int
vm_private_get(
	struct vm_realm *realm,
	vm_value object,
	vm_value key,
	vm_value *result)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	int found;
	int status;

	/* The object's own member. */
	*result = VM_VALUE_UNDEFINED;
	status = class_private_find(realm, object, key, &property, &found);
	if (status != 0)
		return status;
	if (!found) {
		status = vm_throw_type_error(realm, "Cannot read private member from an object whose class did not declare it");
		return status;
	}

	/* A field's or a method's value. */
	if ((property.attributes & VM_PROPERTY_ACCESSOR) == 0U) {
		*result = *property.value;
		return 0;
	}

	/* An accessor without a getter cannot be read. */
	accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
	if (accessor->getter == VM_VALUE_UNDEFINED) {
		status = vm_throw_type_error(realm, "'#' accessor was defined without a getter");
		return status;
	}

	/* The getter's value. */
	status = vm_call(realm, accessor->getter, object, NULL, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: the member's value. */
	return 0;
}

/*
 * Writes a private member (obj.#x = value): a field takes it, a setter is
 * called; a method, an accessor without a setter or a member the object
 * does not have is a TypeError.
 */
int
vm_private_set(
	struct vm_realm *realm,
	vm_value object,
	vm_value key,
	vm_value value)
{
	struct vm_property property;
	struct vm_accessor *accessor;
	vm_value ignored;
	int found;
	int status;

	/* The object's own member. */
	status = class_private_find(realm, object, key, &property, &found);
	if (status != 0)
		return status;
	if (!found) {
		status = vm_throw_type_error(realm, "Cannot write private member to an object whose class did not declare it");
		return status;
	}

	/* A field takes the value; a method cannot be assigned. */
	if ((property.attributes & VM_PROPERTY_ACCESSOR) == 0U) {
		if ((property.attributes & VM_PROPERTY_WRITABLE) == 0U) {
			status = vm_throw_type_error(realm, "Private method is not writable");
			return status;
		}

		/* A field takes the value. */
		*property.value = value;
		return 0;
	}

	/* An accessor without a setter cannot be written. */
	accessor = (struct vm_accessor *)vm_value_as_cell(*property.value);
	if (accessor->setter == VM_VALUE_UNDEFINED) {
		status = vm_throw_type_error(realm, "'#' accessor was defined without a setter");
		return status;
	}

	/* The setter. */
	status = vm_call(realm, accessor->setter, object, &value, 1, &ignored);
	if (status != 0)
		return status;

	/* Succeeded: the member is written. */
	return 0;
}

/* Adds a private field to an object (its class's field initializer); one it has already is a TypeError. */
int
vm_private_define(
	struct vm_realm *realm,
	vm_value object,
	vm_value key,
	vm_value value)
{
	struct vm_property property;
	int found;
	int status;

	/* An object that has it already. */
	status = class_private_find(realm, object, key, &property, &found);
	if (status != 0)
		return status;
	if (found) {
		status = vm_throw_type_error(realm, "Cannot initialize private field twice on the same object");
		return status;
	}

	/* A writable member, never enumerable. */
	status = vm_object_define(realm->heap, (struct vm_object *)vm_value_as_cell(object), key, value, VM_PROPERTY_WRITABLE);
	if (status != 0)
		return status;

	/* Succeeded: the field is added. */
	return 0;
}

/*
 * Gives an instance a private method or accessor its class keeps on the
 * prototype (the source), with the same attributes; an instance that has
 * it already is a TypeError.
 */
int
vm_private_copy(
	struct vm_realm *realm,
	vm_value target,
	vm_value source,
	vm_value key)
{
	struct vm_property property;
	struct vm_object *holder;
	int found;
	int status;

	/* An instance that has it already (constructed twice). */
	status = class_private_find(realm, target, key, &property, &found);
	if (status != 0)
		return status;
	if (found) {
		status = vm_throw_type_error(realm, "Cannot initialize private methods twice on the same object");
		return status;
	}

	/* The prototype's member. */
	holder = (struct vm_object *)vm_value_as_cell(source);
	found = vm_object_get_own_ordinary(holder, key, &property);
	if (!found)
		return EINVAL;

	/* The same value (a method, or the accessor pair) and attributes on the instance. */
	status = vm_object_define(realm->heap, (struct vm_object *)vm_value_as_cell(target), key, *property.value, property.attributes);
	if (status != 0)
		return status;

	/* Succeeded: the instance has the member. */
	return 0;
}

/* Tells whether an object has a private member (#x in object); anything but an object is a TypeError. */
int
vm_private_in(
	struct vm_realm *realm,
	vm_value key,
	vm_value object,
	vm_value *result)
{
	struct vm_property property;
	int is_object;
	int found;
	int status;

	/* Only an object can be asked. */
	is_object = vm_value_is_object(object);
	if (!is_object) {
		status = vm_throw_type_error(realm, "Cannot use 'in' operator to search for a private field in a value that is not an object");
		return status;
	}

	/* Its own member. */
	found = vm_object_get_own_ordinary((struct vm_object *)vm_value_as_cell(object), key, &property);
	*result = VM_VALUE_FALSE;
	if (found)
		*result = VM_VALUE_TRUE;

	/* Succeeded: whether it has it. */
	return 0;
}

/* Finds an object's own private member; a value that is not an object has none. */
static int
class_private_find(
	struct vm_realm *realm,
	vm_value object,
	vm_value key,
	struct vm_property *property,
	int *found)
{
	int is_object;

	UNUSED_PARAMETER(realm);

	/* A primitive has no private members. */
	*found = 0;
	is_object = vm_value_is_object(object);
	if (!is_object)
		return 0;

	/* Only the object's own (never its prototypes'). */
	*found = vm_object_get_own_ordinary((struct vm_object *)vm_value_as_cell(object), key, property);

	/* Succeeded: whether it has it. */
	return 0;
}
