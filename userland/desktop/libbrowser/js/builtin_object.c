/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Object: the constructor, its functions over properties, prototypes and
 * integrity (defineProperty, create, keys, freeze and the rest), and
 * Object.prototype's methods (hasOwnProperty, toString, the __proto__
 * accessor and Annex B's __defineGetter__ family).
 *
 * Object.fromEntries and Object.groupBy need iteration and arrive with the
 * iterators (ws074-p028).
 */

#include "js/builtin.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The attribute masks an integrity level changes. */
#define OBJECT_SEALED		1
#define OBJECT_FROZEN		2

/*
 * One built-in function of this group: its name, its length and what it
 * does.  The tables are constant for the life of the program.
 */
struct object_entry {
	const char *name;
	unsigned length;
	vm_native native;
};

static int object_call(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_assign(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_create(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_define_properties(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_define_property(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_entries(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_freeze(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_get_own_property_descriptor(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_get_own_property_descriptors(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_get_own_property_names(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_get_own_property_symbols(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_get_prototype_of(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_has_own(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_is(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_is_extensible(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_is_frozen(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_is_sealed(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_keys(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_prevent_extensions(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_seal(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_set_prototype_of(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_values(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_has_own_property(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_is_prototype_of(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_property_is_enumerable(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_to_locale_string(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_to_string(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_value_of(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_proto_get(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_proto_set(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_define_getter(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_define_setter(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_lookup_getter(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_lookup_setter(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int object_require(struct vm_realm *realm, vm_value value, const char *message, struct vm_object **object);
static int object_to_descriptor(struct vm_realm *realm, vm_value value, struct vm_descriptor *descriptor);
static int object_descriptor_field(struct vm_realm *realm, vm_value object, const char *name, uint32_t flag, struct vm_descriptor *descriptor, vm_value *value);
static int object_from_descriptor(struct vm_realm *realm, const struct vm_descriptor *descriptor, vm_value *result);
static int object_define_or_throw(struct vm_realm *realm, struct vm_object *object, vm_value key, const struct vm_descriptor *descriptor);
static int object_own_keys(struct vm_realm *realm, struct vm_object *object, int symbols, struct wb_vector *keys);
static int object_key_value(struct vm_realm *realm, vm_value key, vm_value *value);
static int object_list(struct vm_realm *realm, vm_value value, int what, vm_value *result);
static int object_set_integrity(struct vm_realm *realm, struct vm_object *object, int level);
static int object_test_integrity(struct vm_realm *realm, struct vm_object *object, int level);
static int object_set_prototype(struct vm_object *object, struct vm_object *prototype);
static int object_accessor_half(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, int setter, vm_value *result);
static int object_lookup_half(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, int setter, vm_value *result);
static const char *object_tag(vm_value value, const struct vm_object *object);

/*
 * The functions of the Object constructor.
 */
static const struct object_entry object_statics[] = {
	{ "assign", 2, object_assign },
	{ "create", 2, object_create },
	{ "defineProperties", 2, object_define_properties },
	{ "defineProperty", 3, object_define_property },
	{ "entries", 1, object_entries },
	{ "freeze", 1, object_freeze },
	{ "getOwnPropertyDescriptor", 2, object_get_own_property_descriptor },
	{ "getOwnPropertyDescriptors", 1, object_get_own_property_descriptors },
	{ "getOwnPropertyNames", 1, object_get_own_property_names },
	{ "getOwnPropertySymbols", 1, object_get_own_property_symbols },
	{ "getPrototypeOf", 1, object_get_prototype_of },
	{ "hasOwn", 2, object_has_own },
	{ "is", 2, object_is },
	{ "isExtensible", 1, object_is_extensible },
	{ "isFrozen", 1, object_is_frozen },
	{ "isSealed", 1, object_is_sealed },
	{ "keys", 1, object_keys },
	{ "preventExtensions", 1, object_prevent_extensions },
	{ "seal", 1, object_seal },
	{ "setPrototypeOf", 2, object_set_prototype_of },
	{ "values", 1, object_values },
	{ NULL, 0, NULL }
};

/*
 * The methods of Object.prototype.
 */
static const struct object_entry object_methods[] = {
	{ "hasOwnProperty", 1, object_has_own_property },
	{ "isPrototypeOf", 1, object_is_prototype_of },
	{ "propertyIsEnumerable", 1, object_property_is_enumerable },
	{ "toLocaleString", 0, object_to_locale_string },
	{ "toString", 0, object_to_string },
	{ "valueOf", 0, object_value_of },
	{ "__defineGetter__", 2, object_define_getter },
	{ "__defineSetter__", 2, object_define_setter },
	{ "__lookupGetter__", 1, object_lookup_getter },
	{ "__lookupSetter__", 1, object_lookup_setter },
	{ NULL, 0, NULL }
};

/* What object_list lists: keys, values or [key, value] entries. */
#define OBJECT_LIST_KEYS	0
#define OBJECT_LIST_VALUES	1
#define OBJECT_LIST_ENTRIES	2

/*
 * Installs Object and Object.prototype's methods.
 */
int
js_builtin_install_object(
	struct vm_realm *realm)
{
	const struct object_entry *entry;
	struct vm_function *constructor;
	int error;

	/* The constructor over the realm's Object.prototype. */
	error = js_builtin_constructor(realm, "Object", 1, object_call, object_construct, realm->object_prototype, &constructor);
	if (error != 0)
		return error;

	/* Its functions. */
	for (entry = object_statics; entry->name != NULL; entry++) {
		error = js_builtin_method(realm, &constructor->object, entry->name, entry->length, entry->native);
		if (error != 0)
			return error;
	}

	/* The prototype's methods and the __proto__ accessor. */
	for (entry = object_methods; entry->name != NULL; entry++) {
		error = js_builtin_method(realm, realm->object_prototype, entry->name, entry->length, entry->native);
		if (error != 0)
			return error;
	}

	/* The __proto__ accessor. */
	error = js_builtin_accessor(realm, realm->object_prototype, "__proto__", object_proto_get, object_proto_set);
	if (error != 0)
		return error;

	/* Succeeded: Object is installed. */
	return 0;
}

/* Object(value) called: a new object for undefined and null, the value as an object otherwise. */
static int
object_call(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *made;
	vm_value value;
	int status;

	UNUSED_PARAMETER(this_value);

	/* undefined and null make an empty object. */
	value = js_argument(args, count, 0);
	if (value == VM_VALUE_UNDEFINED || value == VM_VALUE_NULL) {
		made = vm_object_create(realm->heap, realm->object_prototype);
		if (made == NULL)
			return ENOMEM;
		*result = vm_value_cell(made);
		return 0;
	}

	/* Anything else as an object. */
	status = vm_to_object(realm, value, result);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* new Object(value): as called, unless new.target is a subclass (an object from its prototype). */
static int
object_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *constructor;
	struct vm_object *prototype;
	struct vm_object *made;
	vm_value self;
	int status;

	/* new.target other than Object: an ordinary object from its prototype. */
	constructor = js_builtin_callee(realm);
	self = vm_value_cell(constructor);
	if (realm->new_target != self) {
		status = vm_construct_prototype(realm, realm->new_target, realm->object_prototype, &prototype);
		if (status != 0)
			return status;
		made = vm_object_create(realm->heap, prototype);
		if (made == NULL)
			return ENOMEM;
		*result = vm_value_cell(made);
		return 0;
	}

	/* Otherwise as a call. */
	status = object_call(realm, this_value, args, count, result);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* Object.assign(target, ...sources): each source's own enumerable properties, set on the target. */
static int
object_assign(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_vector keys;
	struct vm_descriptor descriptor;
	vm_value target;
	vm_value source;
	vm_value key;
	vm_value value;
	unsigned index;
	size_t item;
	int found;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The target as an object. */
	status = vm_to_object(realm, js_argument(args, count, 0), &target);
	if (status != 0)
		return status;

	/* Each source that is not undefined or null. */
	wb_vector_init(&keys, sizeof(vm_value));
	for (index = 1; index < count; index++) {
		if (args[index] == VM_VALUE_UNDEFINED || args[index] == VM_VALUE_NULL)
			continue;
		status = vm_to_object(realm, args[index], &source);
		if (status == 0) {
			wb_vector_clear(&keys);
			status = object_own_keys(realm, (struct vm_object *)vm_value_as_cell(source), 1, &keys);
		}

		/* Each own enumerable property, read and set (strictly: a refusal throws). */
		for (item = 0; status == 0 && item < keys.count; item++) {
			key = *(vm_value *)wb_vector_at(&keys, item);
			found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(source), key, &descriptor);
			if (found < 0) {
				status = -found;
				break;
			}

			/* A missing descriptor follows the absence path after errors have been excluded. */
			if (!found || (descriptor.attributes & VM_PROPERTY_ENUMERABLE) == 0U)
				continue;
			status = vm_get(realm, source, key, &value);
			if (status == 0)
				status = vm_set(realm, target, key, value, 1);
		}

		/* A failure leaves the rest. */
		if (status != 0) {
			wb_vector_release(&keys);
			return status;
		}
	}

	/* The list is no longer needed. */
	wb_vector_release(&keys);

	/* Succeeded: the target. */
	*result = target;
	return 0;
}

/* Object.create(proto, properties): an object with that prototype (an object or null), and the properties defined. */
static int
object_create(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *made;
	struct vm_object *prototype;
	vm_value proto;
	vm_value properties;
	vm_value define_args[2];
	vm_value ignored;
	int is_object;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The prototype: an object or null. */
	proto = js_argument(args, count, 0);
	is_object = vm_value_is_object(proto);
	if (!is_object && proto != VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Object prototype may only be an Object or null");
		return status;
	}

	/* An object is the prototype; null none. */
	prototype = NULL;
	if (is_object)
		prototype = (struct vm_object *)vm_value_as_cell(proto);

	/* The object. */
	made = vm_object_create(realm->heap, prototype);
	if (made == NULL)
		return ENOMEM;
	*result = vm_value_cell(made);

	/* The properties, as defineProperties defines them. */
	properties = js_argument(args, count, 1);
	if (properties == VM_VALUE_UNDEFINED)
		return 0;
	define_args[0] = *result;
	define_args[1] = args[1];
	status = object_define_properties(realm, VM_VALUE_UNDEFINED, define_args, 2, &ignored);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* Object.defineProperties(object, properties): every own enumerable property of properties is a descriptor to define. */
static int
object_define_properties(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_vector keys;
	struct wb_vector descriptors;
	struct vm_descriptor descriptor;
	struct vm_descriptor own;
	struct vm_object *object;
	vm_value properties;
	vm_value key;
	vm_value value;
	size_t item;
	int found;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object, and the properties as an object. */
	status = object_require(realm, js_argument(args, count, 0), "Object.defineProperties called on non-object", &object);
	if (status != 0)
		return status;
	status = vm_to_object(realm, js_argument(args, count, 1), &properties);
	if (status != 0)
		return status;

	/* All the descriptors first (a bad one defines nothing), then each definition. */
	wb_vector_init(&keys, sizeof(vm_value));
	wb_vector_init(&descriptors, sizeof(struct vm_descriptor));
	status = object_own_keys(realm, (struct vm_object *)vm_value_as_cell(properties), 1, &keys);
	for (item = 0; status == 0 && item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(properties), key, &own);
		if (found < 0) {
			status = -found;
			break;
		}

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found || (own.attributes & VM_PROPERTY_ENUMERABLE) == 0U) {
			memset(&descriptor, 0, sizeof(descriptor));
			descriptor.has = 0xffffffffU;
			status = wb_vector_push(&descriptors, &descriptor);
			continue;
		}

		/* An enumerable one's descriptor. */
		status = vm_get(realm, properties, key, &value);
		if (status == 0)
			status = object_to_descriptor(realm, value, &descriptor);
		if (status == 0)
			status = wb_vector_push(&descriptors, &descriptor);
	}

	/* Then each definition. */
	for (item = 0; status == 0 && item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		descriptor = *(struct vm_descriptor *)wb_vector_at(&descriptors, item);
		if (descriptor.has == 0xffffffffU)
			continue;
		status = object_define_or_throw(realm, object, key, &descriptor);
	}

	/* The lists are no longer needed. */
	wb_vector_release(&keys);
	wb_vector_release(&descriptors);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	*result = vm_value_cell(object);
	return 0;
}

/* Object.defineProperty(object, key, attributes). */
static int
object_define_property(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	struct vm_object *object;
	vm_value key;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object, the key and the descriptor, in that order. */
	status = object_require(realm, js_argument(args, count, 0), "Object.defineProperty called on non-object", &object);
	if (status != 0)
		return status;
	status = vm_to_key(realm, js_argument(args, count, 1), &key);
	if (status != 0)
		return status;
	status = object_to_descriptor(realm, js_argument(args, count, 2), &descriptor);
	if (status != 0)
		return status;

	/* The definition, which must be allowed. */
	status = object_define_or_throw(realm, object, key, &descriptor);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	*result = vm_value_cell(object);
	return 0;
}

/* Object.entries(object). */
static int
object_entries(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(this_value);

	/* The [key, value] pairs. */
	status = object_list(realm, js_argument(args, count, 0), OBJECT_LIST_ENTRIES, result);
	if (status != 0)
		return status;

	/* Succeeded: the array. */
	return 0;
}

/* Object.freeze(object). */
static int
object_freeze(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value value;
	int is_object;
	int status;

	UNUSED_PARAMETER(this_value);

	/* A primitive is returned as it is. */
	value = js_argument(args, count, 0);
	*result = value;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* Every property read-only and fixed, and no new ones. */
	status = object_set_integrity(realm, (struct vm_object *)vm_value_as_cell(value), OBJECT_FROZEN);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* Object.getOwnPropertyDescriptor(object, key). */
static int
object_get_own_property_descriptor(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	vm_value object;
	vm_value key;
	int found;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object, then the key. */
	status = vm_to_object(realm, js_argument(args, count, 0), &object);
	if (status != 0)
		return status;
	status = vm_to_key(realm, js_argument(args, count, 1), &key);
	if (status != 0)
		return status;

	/* The descriptor as an object, or undefined. */
	*result = VM_VALUE_UNDEFINED;
	found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(object), key, &descriptor);
	if (found < 0)
		return -found;

	/* A missing descriptor follows the absence path after errors have been excluded. */
	if (!found)
		return 0;
	status = object_from_descriptor(realm, &descriptor, result);
	if (status != 0)
		return status;

	/* Succeeded: the descriptor. */
	return 0;
}

/* Object.getOwnPropertyDescriptors(object). */
static int
object_get_own_property_descriptors(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_vector keys;
	struct vm_descriptor descriptor;
	struct vm_object *made;
	vm_value object;
	vm_value key;
	vm_value value;
	size_t item;
	int found;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object and the result. */
	status = vm_to_object(realm, js_argument(args, count, 0), &object);
	if (status != 0)
		return status;
	made = vm_object_create(realm->heap, realm->object_prototype);
	if (made == NULL)
		return ENOMEM;

	/* Each own property's descriptor. */
	wb_vector_init(&keys, sizeof(vm_value));
	status = object_own_keys(realm, (struct vm_object *)vm_value_as_cell(object), 1, &keys);
	for (item = 0; status == 0 && item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(object), key, &descriptor);
		if (found < 0) {
			status = -found;
			break;
		}

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found)
			continue;
		status = object_from_descriptor(realm, &descriptor, &value);
		if (status == 0)
			status = vm_object_define(realm->heap, made, key, value, VM_PROPERTY_DEFAULT);
	}

	/* The list is no longer needed. */
	wb_vector_release(&keys);
	if (status != 0)
		return status;

	/* Succeeded: the descriptors. */
	*result = vm_value_cell(made);
	return 0;
}

/* Object.getOwnPropertyNames(object): the own string keys, enumerable or not. */
static int
object_get_own_property_names(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_vector keys;
	struct wb_vector names;
	vm_value object;
	vm_value name;
	size_t item;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object's own string keys. */
	status = vm_to_object(realm, js_argument(args, count, 0), &object);
	if (status != 0)
		return status;
	wb_vector_init(&keys, sizeof(vm_value));
	wb_vector_init(&names, sizeof(vm_value));
	status = object_own_keys(realm, (struct vm_object *)vm_value_as_cell(object), 0, &keys);

	/* Each as a string, in an array. */
	for (item = 0; status == 0 && item < keys.count; item++) {
		status = object_key_value(realm, *(vm_value *)wb_vector_at(&keys, item), &name);
		if (status == 0)
			status = wb_vector_push(&names, &name);
	}

	/* The array. */
	if (status == 0)
		status = js_builtin_array(realm, names.items, (uint32_t)names.count, result);
	wb_vector_release(&keys);
	wb_vector_release(&names);
	if (status != 0)
		return status;

	/* Succeeded: the names. */
	return 0;
}

/* Object.getOwnPropertySymbols(object): the own symbol keys. */
static int
object_get_own_property_symbols(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct wb_vector keys;
	struct wb_vector symbols;
	vm_value object;
	vm_value key;
	size_t item;
	int is_string;
	int is_int32;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object's own keys, strings and symbols. */
	status = vm_to_object(realm, js_argument(args, count, 0), &object);
	if (status != 0)
		return status;
	wb_vector_init(&keys, sizeof(vm_value));
	wb_vector_init(&symbols, sizeof(vm_value));
	status = object_own_keys(realm, (struct vm_object *)vm_value_as_cell(object), 1, &keys);

	/* Only the symbols. */
	for (item = 0; status == 0 && item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		is_string = vm_value_is_string(key);
		is_int32 = vm_value_is_int32(key);
		if (is_string || is_int32)
			continue;
		status = wb_vector_push(&symbols, &key);
	}

	/* The array. */
	if (status == 0)
		status = js_builtin_array(realm, symbols.items, (uint32_t)symbols.count, result);
	wb_vector_release(&keys);
	wb_vector_release(&symbols);
	if (status != 0)
		return status;

	/* Succeeded: the symbols. */
	return 0;
}

/* Object.getPrototypeOf(object). */
static int
object_get_prototype_of(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *object;
	vm_value value;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object's prototype, or null. */
	status = vm_to_object(realm, js_argument(args, count, 0), &value);
	if (status != 0)
		return status;
	object = (struct vm_object *)vm_value_as_cell(value);
	*result = VM_VALUE_NULL;
	if (object->prototype != NULL)
		*result = vm_value_cell(object->prototype);

	/* Succeeded: the prototype. */
	return 0;
}

/* Object.hasOwn(object, key). */
static int
object_has_own(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	vm_value object;
	vm_value key;
	int found;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object, then the key. */
	status = vm_to_object(realm, js_argument(args, count, 0), &object);
	if (status != 0)
		return status;
	status = vm_to_key(realm, js_argument(args, count, 1), &key);
	if (status != 0)
		return status;

	/* Succeeded: whether it has the property. */
	found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(object), key, &descriptor);
	if (found < 0)
		return -found;

	/* Successful descriptor inspection now supplies the requested presence or flag result. */
	*result = vm_value_boolean(found);
	return 0;
}

/* Object.is(a, b): SameValue. */
static int
object_is(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int same;

	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);

	/* Succeeded: the comparison. */
	same = vm_same_value(js_argument(args, count, 0), js_argument(args, count, 1));
	*result = vm_value_boolean(same);
	return 0;
}

/* Object.isExtensible(object). */
static int
object_is_extensible(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *object;
	vm_value value;
	int is_object;

	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);

	/* A primitive is not. */
	value = js_argument(args, count, 0);
	*result = VM_VALUE_FALSE;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* Succeeded: whether it takes new properties. */
	object = (struct vm_object *)vm_value_as_cell(value);
	*result = vm_value_boolean((object->flags & VM_OBJECT_NOT_EXTENSIBLE) == 0U);
	return 0;
}

/* Object.isFrozen(object). */
static int
object_is_frozen(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value value;
	int is_object;
	int frozen;

	UNUSED_PARAMETER(this_value);

	/* A primitive is. */
	value = js_argument(args, count, 0);
	*result = VM_VALUE_TRUE;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* Succeeded: whether every property is read-only and fixed. */
	frozen = object_test_integrity(realm, (struct vm_object *)vm_value_as_cell(value), OBJECT_FROZEN);
	if (frozen < 0)
		return -frozen;

	/* Successful integrity inspection reports the requested boolean. */
	*result = vm_value_boolean(frozen);
	return 0;
}

/* Object.isSealed(object). */
static int
object_is_sealed(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value value;
	int is_object;
	int sealed;

	UNUSED_PARAMETER(this_value);

	/* A primitive is. */
	value = js_argument(args, count, 0);
	*result = VM_VALUE_TRUE;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* Succeeded: whether every property is fixed. */
	sealed = object_test_integrity(realm, (struct vm_object *)vm_value_as_cell(value), OBJECT_SEALED);
	if (sealed < 0)
		return -sealed;

	/* Successful integrity inspection reports the requested boolean. */
	*result = vm_value_boolean(sealed);
	return 0;
}

/* Object.keys(object). */
static int
object_keys(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(this_value);

	/* The keys. */
	status = object_list(realm, js_argument(args, count, 0), OBJECT_LIST_KEYS, result);
	if (status != 0)
		return status;

	/* Succeeded: the array. */
	return 0;
}

/* Object.preventExtensions(object). */
static int
object_prevent_extensions(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *object;
	vm_value value;
	int is_object;
	int done;
	int status;

	UNUSED_PARAMETER(this_value);

	/* A primitive is returned as it is. */
	value = js_argument(args, count, 0);
	*result = value;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* Native policy must permit the transition before any extensibility state changes. */
	object = (struct vm_object *)vm_value_as_cell(value);
	status = vm_object_prevent_extensions(object, &done);
	if (status != 0)
		return status;

	/* Object.preventExtensions throws when a native object refuses the transition. */
	if (!done) {
		status = vm_throw_type_error(realm, "The object refuses preventing extensions.");
		return status;
	}

	/* Succeeded: the object now refuses every new own property. */
	return 0;
}

/* Object.seal(object). */
static int
object_seal(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value value;
	int is_object;
	int status;

	UNUSED_PARAMETER(this_value);

	/* A primitive is returned as it is. */
	value = js_argument(args, count, 0);
	*result = value;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* Every property fixed, and no new ones. */
	status = object_set_integrity(realm, (struct vm_object *)vm_value_as_cell(value), OBJECT_SEALED);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* Object.setPrototypeOf(object, proto). */
static int
object_set_prototype_of(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *prototype;
	vm_value value;
	vm_value proto;
	int is_object;
	int done;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The object must not be undefined or null; the prototype must be an object or null. */
	value = js_argument(args, count, 0);
	proto = js_argument(args, count, 1);
	if (value == VM_VALUE_UNDEFINED || value == VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Object.setPrototypeOf called on null or undefined");
		return status;
	}

	/* The prototype must be an object or null. */
	is_object = vm_value_is_object(proto);
	if (!is_object && proto != VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Object prototype may only be an Object or null");
		return status;
	}

	/* A primitive is returned as it is. */
	*result = value;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* The change must be allowed. */
	prototype = NULL;
	if (proto != VM_VALUE_NULL)
		prototype = (struct vm_object *)vm_value_as_cell(proto);
	done = object_set_prototype((struct vm_object *)vm_value_as_cell(value), prototype);
	if (!done) {
		status = vm_throw_type_error(realm, "Cannot set the prototype");
		return status;
	}

	/* Succeeded: the object. */
	return 0;
}

/* Object.values(object). */
static int
object_values(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(this_value);

	/* The values. */
	status = object_list(realm, js_argument(args, count, 0), OBJECT_LIST_VALUES, result);
	if (status != 0)
		return status;

	/* Succeeded: the array. */
	return 0;
}

/* Object.prototype.hasOwnProperty(key): the key first, then this as an object. */
static int
object_has_own_property(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	vm_value object;
	vm_value key;
	int found;
	int status;

	/* The key, then the object. */
	status = vm_to_key(realm, js_argument(args, count, 0), &key);
	if (status != 0)
		return status;
	status = vm_to_object(realm, this_value, &object);
	if (status != 0)
		return status;

	/* Succeeded: whether it has the property. */
	found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(object), key, &descriptor);
	if (found < 0)
		return -found;

	/* Successful descriptor inspection now supplies the requested presence or flag result. */
	*result = vm_value_boolean(found);
	return 0;
}

/* Object.prototype.isPrototypeOf(value). */
static int
object_is_prototype_of(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *walk;
	struct vm_cell *target;
	vm_value value;
	vm_value object;
	int is_object;
	int status;

	/* A primitive has no prototypes. */
	value = js_argument(args, count, 0);
	*result = VM_VALUE_FALSE;
	is_object = vm_value_is_object(value);
	if (!is_object)
		return 0;

	/* This as an object. */
	status = vm_to_object(realm, this_value, &object);
	if (status != 0)
		return status;

	/* Up the value's chain. */
	target = vm_value_as_cell(object);
	for (walk = ((struct vm_object *)vm_value_as_cell(value))->prototype; walk != NULL; walk = walk->prototype) {
		if (&walk->cell == target) {
			*result = VM_VALUE_TRUE;
			return 0;
		}
	}

	/* Succeeded: not on the chain. */
	return 0;
}

/* Object.prototype.propertyIsEnumerable(key). */
static int
object_property_is_enumerable(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	vm_value object;
	vm_value key;
	int found;
	int enumerable;
	int status;

	/* The key, then the object. */
	status = vm_to_key(realm, js_argument(args, count, 0), &key);
	if (status != 0)
		return status;
	status = vm_to_object(realm, this_value, &object);
	if (status != 0)
		return status;

	/* Succeeded: whether it has the property and it is enumerable. */
	found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(object), key, &descriptor);
	if (found < 0)
		return -found;

	/* Successful descriptor inspection now supplies the requested presence or flag result. */
	enumerable = 0;
	if (found && (descriptor.attributes & VM_PROPERTY_ENUMERABLE) != 0U)
		enumerable = 1;
	*result = vm_value_boolean(enumerable);
	return 0;
}

/* Object.prototype.toLocaleString(): this.toString(). */
static int
object_to_locale_string(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value key;
	vm_value method;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The toString method, called on this. */
	key = vm_key_from_ascii(realm->heap, "toString");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_get(realm, this_value, key, &method);
	if (status != 0)
		return status;
	status = vm_call(realm, method, this_value, NULL, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: its string. */
	return 0;
}

/* Object.prototype.toString(): "[object Tag]". */
static int
object_to_string(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *object;
	struct vm_string *text_string;
	vm_value value;
	vm_value named;
	vm_value prefix;
	char text[64];
	const char *tag;
	int is_string;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* undefined and null have their own tags. */
	object = NULL;
	if (this_value == VM_VALUE_UNDEFINED) {
		tag = "Undefined";
	} else if (this_value == VM_VALUE_NULL) {
		tag = "Null";
	} else {
		status = vm_to_object(realm, this_value, &value);
		if (status != 0)
			return status;
		object = (struct vm_object *)vm_value_as_cell(value);
		tag = object_tag(value, object);

		/* A string Symbol.toStringTag names it instead (ws074-p087). */
		status = vm_get(realm, value, vm_symbol_key(realm, VM_SYMBOL_TO_STRING_TAG), &named);
		if (status != 0)
			return status;
		is_string = vm_value_is_string(named);
		if (is_string) {
			status = js_builtin_string(realm, "[object ", &prefix);
			if (status != 0)
				return status;
			text_string = vm_string_concat(realm->heap, (struct vm_string *)vm_value_as_cell(prefix), (struct vm_string *)vm_value_as_cell(named));
			if (text_string == NULL)
				return ENOMEM;
			status = js_builtin_string(realm, "]", &prefix);
			if (status != 0)
				return status;
			text_string = vm_string_concat(realm->heap, text_string, (struct vm_string *)vm_value_as_cell(prefix));
			if (text_string == NULL)
				return ENOMEM;
			*result = vm_value_cell(text_string);
			return 0;
		}
	}

	/* Succeeded: the text. */
	snprintf(text, sizeof(text), "[object %s]", tag);
	status = js_builtin_string(realm, text, result);
	if (status != 0)
		return status;
	return 0;
}

/* Object.prototype.valueOf(): this as an object. */
static int
object_value_of(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The object. */
	status = vm_to_object(realm, this_value, result);
	if (status != 0)
		return status;

	/* Succeeded: the object. */
	return 0;
}

/* get Object.prototype.__proto__: this object's prototype. */
static int
object_proto_get(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *object;
	vm_value value;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* This as an object. */
	status = vm_to_object(realm, this_value, &value);
	if (status != 0)
		return status;

	/* Succeeded: its prototype, or null. */
	object = (struct vm_object *)vm_value_as_cell(value);
	*result = VM_VALUE_NULL;
	if (object->prototype != NULL)
		*result = vm_value_cell(object->prototype);
	return 0;
}

/* set Object.prototype.__proto__: a new prototype (an object or null; anything else is ignored). */
static int
object_proto_set(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_object *prototype;
	vm_value proto;
	int is_object;
	int done;
	int status;

	/* undefined and null cannot have one. */
	*result = VM_VALUE_UNDEFINED;
	if (this_value == VM_VALUE_UNDEFINED || this_value == VM_VALUE_NULL) {
		status = vm_throw_type_error(realm, "Object.prototype.__proto__ called on null or undefined");
		return status;
	}

	/* Only an object or null is taken, and only by an object. */
	proto = js_argument(args, count, 0);
	is_object = vm_value_is_object(proto);
	if (!is_object && proto != VM_VALUE_NULL)
		return 0;
	is_object = vm_value_is_object(this_value);
	if (!is_object)
		return 0;

	/* The change must be allowed. */
	prototype = NULL;
	if (proto != VM_VALUE_NULL)
		prototype = (struct vm_object *)vm_value_as_cell(proto);
	done = object_set_prototype((struct vm_object *)vm_value_as_cell(this_value), prototype);
	if (!done) {
		status = vm_throw_type_error(realm, "Cannot set the prototype");
		return status;
	}

	/* Succeeded: the prototype is set. */
	return 0;
}

/* Object.prototype.__defineGetter__(key, getter). */
static int
object_define_getter(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	/* The getter half. */
	status = object_accessor_half(realm, this_value, args, count, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: undefined. */
	return 0;
}

/* Object.prototype.__defineSetter__(key, setter). */
static int
object_define_setter(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	/* The setter half. */
	status = object_accessor_half(realm, this_value, args, count, 1, result);
	if (status != 0)
		return status;

	/* Succeeded: undefined. */
	return 0;
}

/* Object.prototype.__lookupGetter__(key). */
static int
object_lookup_getter(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	/* The getter on the chain. */
	status = object_lookup_half(realm, this_value, args, count, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: the getter or undefined. */
	return 0;
}

/* Object.prototype.__lookupSetter__(key). */
static int
object_lookup_setter(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	/* The setter on the chain. */
	status = object_lookup_half(realm, this_value, args, count, 1, result);
	if (status != 0)
		return status;

	/* Succeeded: the setter or undefined. */
	return 0;
}

/* Requires a value to be an object, throwing a TypeError with a message otherwise. */
static int
object_require(
	struct vm_realm *realm,
	vm_value value,
	const char *message,
	struct vm_object **object)
{
	int is_object;
	int status;

	/* A primitive is refused. */
	*object = NULL;
	is_object = vm_value_is_object(value);
	if (!is_object) {
		status = vm_throw_type_error(realm, message);
		return status;
	}

	/* Succeeded: the object. */
	*object = (struct vm_object *)vm_value_as_cell(value);
	return 0;
}

/* Reads a property descriptor from an object (ToPropertyDescriptor). */
static int
object_to_descriptor(
	struct vm_realm *realm,
	vm_value value,
	struct vm_descriptor *descriptor)
{
	struct vm_object *object;
	vm_value field;
	int callable;
	int truth;
	int status;

	/* A descriptor is an object. */
	memset(descriptor, 0, sizeof(*descriptor));
	descriptor->value = VM_VALUE_UNDEFINED;
	descriptor->getter = VM_VALUE_UNDEFINED;
	descriptor->setter = VM_VALUE_UNDEFINED;
	status = object_require(realm, value, "Property description must be an object", &object);
	if (status != 0)
		return status;

	/* enumerable, configurable, value, writable, get and set, in that order. */
	field = VM_VALUE_FALSE;
	status = object_descriptor_field(realm, value, "enumerable", VM_HAS_ENUMERABLE, descriptor, &field);
	truth = vm_to_boolean(field);
	if (status == 0 && truth)
		descriptor->attributes |= VM_PROPERTY_ENUMERABLE;
	field = VM_VALUE_FALSE;
	if (status == 0)
		status = object_descriptor_field(realm, value, "configurable", VM_HAS_CONFIGURABLE, descriptor, &field);
	truth = vm_to_boolean(field);
	if (status == 0 && truth)
		descriptor->attributes |= VM_PROPERTY_CONFIGURABLE;
	if (status == 0)
		status = object_descriptor_field(realm, value, "value", VM_HAS_VALUE, descriptor, &descriptor->value);
	field = VM_VALUE_FALSE;
	if (status == 0)
		status = object_descriptor_field(realm, value, "writable", VM_HAS_WRITABLE, descriptor, &field);
	truth = vm_to_boolean(field);
	if (status == 0 && truth)
		descriptor->attributes |= VM_PROPERTY_WRITABLE;
	if (status == 0)
		status = object_descriptor_field(realm, value, "get", VM_HAS_GET, descriptor, &descriptor->getter);
	if (status == 0)
		status = object_descriptor_field(realm, value, "set", VM_HAS_SET, descriptor, &descriptor->setter);
	if (status != 0)
		return status;

	/* A getter and a setter must be functions or undefined. */
	callable = vm_value_is_callable(descriptor->getter);
	if ((descriptor->has & VM_HAS_GET) != 0U && !callable && descriptor->getter != VM_VALUE_UNDEFINED) {
		status = vm_throw_type_error(realm, "Getter must be a function");
		return status;
	}

	/* The same for the setter. */
	callable = vm_value_is_callable(descriptor->setter);
	if ((descriptor->has & VM_HAS_SET) != 0U && !callable && descriptor->setter != VM_VALUE_UNDEFINED) {
		status = vm_throw_type_error(realm, "Setter must be a function");
		return status;
	}

	/* A descriptor is an accessor or a data property, not both. */
	if ((descriptor->has & (VM_HAS_GET | VM_HAS_SET)) != 0U && (descriptor->has & (VM_HAS_VALUE | VM_HAS_WRITABLE)) != 0U) {
		status = vm_throw_type_error(realm, "Invalid property descriptor. Cannot both specify accessors and a value or writable attribute");
		return status;
	}

	/* Succeeded: the descriptor. */
	return 0;
}

/* Reads one field of a descriptor object when it has it (on its chain). */
static int
object_descriptor_field(
	struct vm_realm *realm,
	vm_value object,
	const char *name,
	uint32_t flag,
	struct vm_descriptor *descriptor,
	vm_value *value)
{
	vm_value key;
	vm_value has;
	int status;

	/* Whether it has the field. */
	key = vm_key_from_ascii(realm->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_in(realm, key, object, &has);
	if (status != 0)
		return status;
	if (has != VM_VALUE_TRUE)
		return 0;

	/* The field's value. */
	status = vm_get(realm, object, key, value);
	if (status != 0)
		return status;

	/* Succeeded: the descriptor has it. */
	descriptor->has |= flag;
	return 0;
}

/* Makes an object of a descriptor (FromPropertyDescriptor). */
static int
object_from_descriptor(
	struct vm_realm *realm,
	const struct vm_descriptor *descriptor,
	vm_value *result)
{
	struct vm_object *made;
	int error;

	/* The object. */
	made = vm_object_create(realm->heap, realm->object_prototype);
	if (made == NULL)
		return ENOMEM;

	/* value and writable, or get and set; then enumerable and configurable. */
	error = 0;
	if ((descriptor->has & VM_HAS_VALUE) != 0U)
		error = js_builtin_value(realm, made, "value", descriptor->value, VM_PROPERTY_DEFAULT);
	if (error == 0 && (descriptor->has & VM_HAS_WRITABLE) != 0U)
		error = js_builtin_value(realm, made, "writable", vm_value_boolean((descriptor->attributes & VM_PROPERTY_WRITABLE) != 0U), VM_PROPERTY_DEFAULT);
	if (error == 0 && (descriptor->has & VM_HAS_GET) != 0U)
		error = js_builtin_value(realm, made, "get", descriptor->getter, VM_PROPERTY_DEFAULT);
	if (error == 0 && (descriptor->has & VM_HAS_SET) != 0U)
		error = js_builtin_value(realm, made, "set", descriptor->setter, VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, made, "enumerable", vm_value_boolean((descriptor->attributes & VM_PROPERTY_ENUMERABLE) != 0U), VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, made, "configurable", vm_value_boolean((descriptor->attributes & VM_PROPERTY_CONFIGURABLE) != 0U), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Succeeded: the object. */
	*result = vm_value_cell(made);
	return 0;
}

/* Defines a property, throwing a TypeError when it is not allowed. */
static int
object_define_or_throw(
	struct vm_realm *realm,
	struct vm_object *object,
	vm_value key,
	const struct vm_descriptor *descriptor)
{
	int done;
	int status;

	/* The definition. */
	status = vm_define_own_property(realm, object, key, descriptor, &done);
	if (status != 0)
		return status;
	if (!done) {
		status = vm_throw_type_error(realm, "Cannot redefine property");
		return status;
	}

	/* Succeeded: the property is defined. */
	return 0;
}

/* Lists an object's own keys: the string ones, and the symbols too when asked. */
static int
object_own_keys(
	struct vm_realm *realm,
	struct vm_object *object,
	int symbols,
	struct wb_vector *keys)
{
	struct wb_vector all;
	vm_value key;
	size_t item;
	int is_int32;
	int is_string;
	int status;

	/* Every own key in order (indices, strings, symbols). */
	wb_vector_init(&all, sizeof(vm_value));
	status = vm_object_own_keys(realm->heap, object, &all);

	/* Those wanted. */
	for (item = 0; status == 0 && item < all.count; item++) {
		key = *(vm_value *)wb_vector_at(&all, item);
		is_int32 = vm_value_is_int32(key);
		is_string = vm_value_is_string(key);
		if (!is_int32 && !is_string && !symbols)
			continue;
		status = wb_vector_push(keys, &key);
	}

	/* The full list is no longer needed. */
	wb_vector_release(&all);
	if (status != 0)
		return status;

	/* Succeeded: the keys. */
	return 0;
}

/* Makes the value of a key as the language shows it (an index as its string). */
static int
object_key_value(
	struct vm_realm *realm,
	vm_value key,
	vm_value *value)
{
	struct vm_string *string;
	int is_int32;
	int status;

	/* A string or a symbol is itself. */
	is_int32 = vm_value_is_int32(key);
	if (!is_int32) {
		*value = key;
		return 0;
	}

	/* An index's numeral. */
	status = vm_to_string(realm, key, &string);
	if (status != 0)
		return status;

	/* Succeeded: the string. */
	*value = vm_value_cell(string);
	return 0;
}

/* Lists an object's own enumerable string keys, their values or [key, value] pairs in an array. */
static int
object_list(
	struct vm_realm *realm,
	vm_value value,
	int what,
	vm_value *result)
{
	struct wb_vector keys;
	struct wb_vector items;
	struct vm_descriptor descriptor;
	vm_value object;
	vm_value key;
	vm_value name;
	vm_value property;
	vm_value pair[2];
	vm_value item;
	size_t index;
	int found;
	int status;

	/* The object and its own string keys. */
	status = vm_to_object(realm, value, &object);
	if (status != 0)
		return status;
	wb_vector_init(&keys, sizeof(vm_value));
	wb_vector_init(&items, sizeof(vm_value));
	status = object_own_keys(realm, (struct vm_object *)vm_value_as_cell(object), 0, &keys);

	/* Each one still there and enumerable. */
	for (index = 0; status == 0 && index < keys.count; index++) {
		key = *(vm_value *)wb_vector_at(&keys, index);
		found = vm_get_own_descriptor((struct vm_object *)vm_value_as_cell(object), key, &descriptor);
		if (found < 0) {
			status = -found;
			break;
		}

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found || (descriptor.attributes & VM_PROPERTY_ENUMERABLE) == 0U)
			continue;
		status = object_key_value(realm, key, &name);
		if (status != 0)
			break;

		/* The key, the value, or both. */
		item = name;
		if (what != OBJECT_LIST_KEYS) {
			status = vm_get(realm, object, key, &property);
			if (status != 0)
				break;
			item = property;
		}

		/* An entry is a pair. */
		if (what == OBJECT_LIST_ENTRIES) {
			pair[0] = name;
			pair[1] = property;
			status = js_builtin_array(realm, pair, 2, &item);
			if (status != 0)
				break;
		}

		/* It joins the list. */
		status = wb_vector_push(&items, &item);
	}

	/* The array. */
	if (status == 0)
		status = js_builtin_array(realm, items.items, (uint32_t)items.count, result);
	wb_vector_release(&keys);
	wb_vector_release(&items);
	if (status != 0)
		return status;

	/* Succeeded: the array. */
	return 0;
}

/* Seals or freezes an object: every own property made fixed (and read-only), and no new ones. */
static int
object_set_integrity(
	struct vm_realm *realm,
	struct vm_object *object,
	int level)
{
	struct wb_vector keys;
	struct vm_descriptor descriptor;
	struct vm_descriptor change;
	vm_value key;
	size_t item;
	int found;
	int done;
	int status;

	/* Native refusal must leave both flags and descriptors unchanged. */
	status = vm_object_prevent_extensions(object, &done);
	if (status != 0)
		return status;

	/* Sealing and freezing cannot proceed when the object stays extensible. */
	if (!done) {
		status = vm_throw_type_error(realm, "The object refuses changing its integrity.");
		return status;
	}

	/* Each own property. */
	wb_vector_init(&keys, sizeof(vm_value));
	status = object_own_keys(realm, object, 1, &keys);
	for (item = 0; status == 0 && item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		found = vm_get_own_descriptor(object, key, &descriptor);
		if (found < 0) {
			status = -found;
			break;
		}

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found)
			continue;

		/* Not configurable; for a frozen data property, not writable either. */
		memset(&change, 0, sizeof(change));
		change.has = VM_HAS_CONFIGURABLE;
		if (level == OBJECT_FROZEN && (descriptor.has & VM_HAS_VALUE) != 0U)
			change.has |= VM_HAS_WRITABLE;
		status = vm_define_own_property(realm, object, key, &change, &done);
		if (status == 0 && !done) {
			status = vm_throw_type_error(realm, "A property refuses changing its integrity.");
			break;
		}
	}

	/* The list is no longer needed. */
	wb_vector_release(&keys);
	if (status != 0)
		return status;

	/* Succeeded: the object is sealed or frozen. */
	return 0;
}

/* Tells whether an object is sealed or frozen. */
static int
object_test_integrity(
	struct vm_realm *realm,
	struct vm_object *object,
	int level)
{
	struct wb_vector keys;
	struct vm_descriptor descriptor;
	vm_value key;
	size_t item;
	int found;
	int holds;
	int status;

	/* An object that takes new properties is neither. */
	if ((object->flags & VM_OBJECT_NOT_EXTENSIBLE) == 0U)
		return 0;

	/* Every own property fixed (and read-only). */
	holds = 1;
	wb_vector_init(&keys, sizeof(vm_value));
	status = object_own_keys(realm, object, 1, &keys);
	for (item = 0; status == 0 && holds && item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		found = vm_get_own_descriptor(object, key, &descriptor);
		if (found < 0) {
			status = -found;
			break;
		}

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found)
			continue;
		if ((descriptor.attributes & VM_PROPERTY_CONFIGURABLE) != 0U)
			holds = 0;
		if (level == OBJECT_FROZEN && (descriptor.has & VM_HAS_VALUE) != 0U && (descriptor.attributes & VM_PROPERTY_WRITABLE) != 0U)
			holds = 0;
	}

	/* The list is no longer needed. */
	wb_vector_release(&keys);

	/* Lookup and enumeration failures retain their distinct negative error outcome. */
	if (status != 0)
		return -status;

	/* Succeeded: reports whether every own descriptor has the requested integrity. */
	return holds;
}

/* Sets an object's prototype when allowed (OrdinarySetPrototypeOf): not for a non-extensible object, and never into a cycle. */
static int
object_set_prototype(
	struct vm_object *object,
	struct vm_object *prototype)
{
	struct vm_object *walk;

	/* The same prototype is no change. */
	if (object->prototype == prototype)
		return 1;

	/* A non-extensible object keeps its prototype. */
	if ((object->flags & VM_OBJECT_NOT_EXTENSIBLE) != 0U)
		return 0;

	/* The object must not be on the new prototype's chain. */
	for (walk = prototype; walk != NULL; walk = walk->prototype) {
		if (walk == object)
			return 0;
	}

	/* Allowed: the new prototype. */
	object->prototype = prototype;
	return 1;
}

/* Defines one half of an accessor (Annex B's __defineGetter__ and __defineSetter__). */
static int
object_accessor_half(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	int setter,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	vm_value object;
	vm_value function;
	vm_value key;
	int callable;
	int status;

	/* This as an object, and a function. */
	*result = VM_VALUE_UNDEFINED;
	status = vm_to_object(realm, this_value, &object);
	if (status != 0)
		return status;
	function = js_argument(args, count, 1);
	callable = vm_value_is_callable(function);
	if (!callable) {
		status = vm_throw_type_error(realm, "Object.prototype.__defineGetter__: Expecting function");
		return status;
	}

	/* The half, enumerable and configurable. */
	status = vm_to_key(realm, js_argument(args, count, 0), &key);
	if (status != 0)
		return status;
	memset(&descriptor, 0, sizeof(descriptor));
	descriptor.has = VM_HAS_ENUMERABLE | VM_HAS_CONFIGURABLE;
	descriptor.attributes = VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE;
	if (setter) {
		descriptor.has |= VM_HAS_SET;
		descriptor.setter = function;
	} else {
		descriptor.has |= VM_HAS_GET;
		descriptor.getter = function;
	}

	/* The definition. */
	status = object_define_or_throw(realm, (struct vm_object *)vm_value_as_cell(object), key, &descriptor);
	if (status != 0)
		return status;

	/* Succeeded: undefined. */
	return 0;
}

/* Finds one half of an accessor on an object's chain (Annex B's __lookupGetter__ and __lookupSetter__). */
static int
object_lookup_half(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	int setter,
	vm_value *result)
{
	struct vm_descriptor descriptor;
	struct vm_object *walk;
	vm_value object;
	vm_value key;
	int found;
	int status;

	/* This as an object, and the key. */
	*result = VM_VALUE_UNDEFINED;
	status = vm_to_object(realm, this_value, &object);
	if (status != 0)
		return status;
	status = vm_to_key(realm, js_argument(args, count, 0), &key);
	if (status != 0)
		return status;

	/* The first object of the chain with the property decides. */
	for (walk = (struct vm_object *)vm_value_as_cell(object); walk != NULL; walk = walk->prototype) {
		found = vm_get_own_descriptor(walk, key, &descriptor);
		if (found < 0)
			return -found;

		/* A missing descriptor follows the absence path after errors have been excluded. */
		if (!found)
			continue;
		if ((descriptor.has & VM_HAS_GET) != 0U) {
			*result = descriptor.getter;
			if (setter)
				*result = descriptor.setter;
		}

		/* The nearest property decides. */
		return 0;
	}

	/* Succeeded: none on the chain. */
	return 0;
}

/* Reports Object.prototype.toString's tag of an object by its kind. */
static const char *
object_tag(
	vm_value value,
	const struct vm_object *object)
{
	int callable;

	/* A function by being callable. */
	callable = vm_value_is_callable(value);
	if (callable)
		return "Function";

	/* The others by their kind. */
	switch (object->kind) {
	case VM_KIND_ARRAY:
		return "Array";
	case VM_KIND_ARGUMENTS:
		return "Arguments";
	case VM_KIND_ERROR:
		return "Error";
	case VM_KIND_BOOLEAN:
		return "Boolean";
	case VM_KIND_NUMBER:
		return "Number";
	case VM_KIND_STRING:
		return "String";
	case VM_KIND_DATE:
		return "Date";
	case VM_KIND_REGEXP:
		return "RegExp";
	default:
		break;
	}

	/* An ordinary object. */
	return "Object";
}
