/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Functions (plan/ws074/design.md §11.6): the function cell, native
 * functions, bytecode functions and the closures a script makes with their
 * environments, calling one, and throwing.
 *
 * A function is an object with its realm and what runs when it is called:
 * a native function, or a code unit that the interpreter runs.  A closure
 * also holds the environment of the code that made it, where the variables
 * it shares with that code live.
 */

#include "vm/bytecode.h"
#include "vm/internal.h"

#include <errno.h>
#include <string.h>

static void function_trace(struct vm_heap *heap, struct vm_cell *cell);
static void function_env_trace(struct vm_heap *heap, struct vm_cell *cell);
static int function_add_prototype(struct vm_realm *realm, struct vm_function *function);
static int function_add_generator_prototype(struct vm_realm *realm, struct vm_function *function);

/* The cell type of functions: objects that also hold their realm's objects through the realm's tracer. */
const struct vm_cell_type vm_function_type = {
	"function", function_trace, vm_object_finalize
};

/* The cell type of environments: they hold their parent and their slots. */
const struct vm_cell_type vm_env_type = {
	"environment", function_env_trace, NULL
};

/*
 * Makes a native function of a realm with its name and length properties
 * (neither writable nor enumerable, but configurable, as for built-ins);
 * NULL when out of memory.
 */
struct vm_function *
vm_function_create_native(
	struct vm_realm *realm,
	const char *name,
	unsigned length,
	vm_native native)
{
	struct vm_function *function;
	struct vm_string *text;
	vm_value key;
	int error;

	/* The cell, an object whose prototype is Function.prototype. */
	function = vm_heap_alloc(realm->heap, &vm_function_type, sizeof(*function));
	if (function == NULL)
		return NULL;
	error = vm_object_init(realm->heap, &function->object, realm->function_prototype);
	if (error != 0)
		return NULL;
	function->realm = realm;
	function->native = native;
	function->object.kind = VM_KIND_FUNCTION;
	function->data = VM_VALUE_UNDEFINED;

	/* Its length: how many arguments it expects. */
	key = vm_key_from_ascii(realm->heap, "length");
	if (key == VM_VALUE_EMPTY)
		return NULL;
	error = vm_object_define(realm->heap, &function->object, key, vm_value_int32((int32_t)length), VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return NULL;

	/* Its name. */
	text = vm_string_from_utf8(realm->heap, name, strlen(name));
	if (text == NULL)
		return NULL;
	key = vm_key_from_ascii(realm->heap, "name");
	if (key == VM_VALUE_EMPTY)
		return NULL;
	error = vm_object_define(realm->heap, &function->object, key, vm_value_cell(text), VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return NULL;

	/* Succeeded: the function. */
	return function;
}

/*
 * Makes a bytecode function of a realm from a checked code unit, with its
 * name (the code's) and length (its parameter count); NULL when out of
 * memory.
 */
struct vm_function *
vm_function_create(
	struct vm_realm *realm,
	struct vm_code *code)
{
	struct vm_function *function;
	vm_value key;
	vm_value name;
	uint32_t length;
	int error;

	/* The cell, an object whose prototype is Function.prototype, running the code. */
	function = vm_heap_alloc(realm->heap, &vm_function_type, sizeof(*function));
	if (function == NULL)
		return NULL;
	error = vm_object_init(realm->heap, &function->object, realm->function_prototype);
	if (error != 0)
		return NULL;
	function->realm = realm;
	function->code = code;
	function->object.kind = VM_KIND_FUNCTION;
	function->data = VM_VALUE_UNDEFINED;

	/* Its length: how many parameters it declares (those before a default or a rest when the code says so). */
	length = code->parameter_count;
	if ((code->flags & VM_CODE_LENGTH) != 0U)
		length = code->length;
	key = vm_key_from_ascii(realm->heap, "length");
	if (key == VM_VALUE_EMPTY)
		return NULL;
	error = vm_object_define(realm->heap, &function->object, key, vm_value_int32((int32_t)length), VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return NULL;

	/* Its name: the code's, or the empty string. */
	name = VM_VALUE_EMPTY;
	if (code->name != NULL)
		name = vm_value_cell(code->name);
	if (name == VM_VALUE_EMPTY) {
		code->name = vm_string_from_utf8(realm->heap, "", 0);
		if (code->name == NULL)
			return NULL;
		name = vm_value_cell(code->name);
	}

	/* The name property. */
	key = vm_key_from_ascii(realm->heap, "name");
	if (key == VM_VALUE_EMPTY)
		return NULL;
	error = vm_object_define(realm->heap, &function->object, key, name, VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return NULL;

	/* Succeeded: the function. */
	return function;
}

/*
 * Makes a closure: a bytecode function of a realm that runs a code unit in
 * an environment (NULL for none).  A code unit that can construct gives
 * its closure a prototype object.  NULL when out of memory.
 */
struct vm_function *
vm_closure_create(
	struct vm_realm *realm,
	struct vm_code *code,
	struct vm_env *env)
{
	struct vm_function *function;
	int error;

	/* The function with its length and name. */
	function = vm_function_create(realm, code);
	if (function == NULL)
		return NULL;
	function->env = env;

	/* A constructor's prototype object (a class's comes from class_setup). */
	if ((code->flags & VM_CODE_CONSTRUCTOR) != 0U && (code->flags & VM_CODE_CLASS) == 0U) {
		error = function_add_prototype(realm, function);
		if (error != 0)
			return NULL;
	}

	/* A generator function: its prototype object (its generators' prototype), and it inherits from %GeneratorFunction.prototype%. */
	if ((code->flags & VM_CODE_GENERATOR) != 0U) {
		error = function_add_generator_prototype(realm, function);
		if (error != 0)
			return NULL;
		if (realm->intrinsics[VM_INTRINSIC_GENERATOR_FUNCTION_PROTOTYPE] != NULL)
			function->object.prototype = realm->intrinsics[VM_INTRINSIC_GENERATOR_FUNCTION_PROTOTYPE];
	}

	/* An async function inherits from %AsyncFunction.prototype%. */
	if ((code->flags & VM_CODE_ASYNC) != 0U && realm->intrinsics[VM_INTRINSIC_ASYNC_FUNCTION_PROTOTYPE] != NULL)
		function->object.prototype = realm->intrinsics[VM_INTRINSIC_ASYNC_FUNCTION_PROTOTYPE];

	/* Succeeded: the closure. */
	return function;
}

/*
 * Makes an environment with a parent (NULL for none) and count slots, each
 * undefined; NULL when out of memory.
 */
struct vm_env *
vm_env_create(
	struct vm_heap *heap,
	struct vm_env *parent,
	uint32_t count)
{
	struct vm_env *env;
	vm_value *slots;
	uint32_t index;

	/* The cell, its slots after the header. */
	env = vm_heap_alloc(heap, &vm_env_type, sizeof(*env) + (size_t)count * sizeof(vm_value));
	if (env == NULL)
		return NULL;
	env->parent = parent;
	env->count = count;

	/* Every slot starts undefined. */
	slots = vm_env_slots(env);
	for (index = 0; index < count; index++)
		slots[index] = VM_VALUE_UNDEFINED;

	/* Succeeded: the environment. */
	return env;
}

/*
 * Finds the slots of an environment.
 */
vm_value *
vm_env_slots(
	struct vm_env *env)
{
	/* They follow the header. */
	return (vm_value *)(env + 1);
}

/*
 * Tells whether a value can be called.
 */
int
vm_value_is_callable(
	vm_value value)
{
	struct vm_cell *cell;
	int is_cell;

	/* Only cells are functions. */
	is_cell = vm_value_is_cell(value);
	if (!is_cell)
		return 0;

	/* A function cell. */
	cell = vm_value_as_cell(value);
	if (cell->type == &vm_function_type)
		return 1;

	/* Any other cell. */
	return 0;
}

/*
 * Tells whether a value can be called with new (a bytecode function whose
 * code can construct, or a native function with a construct behaviour).
 */
int
vm_value_is_constructor(
	vm_value value)
{
	struct vm_function *function;
	int callable;

	/* Only functions construct. */
	callable = vm_value_is_callable(value);
	if (!callable)
		return 0;

	/* A code unit that can, or a native constructor. */
	function = (struct vm_function *)vm_value_as_cell(value);
	if (function->code != NULL && (function->code->flags & VM_CODE_CONSTRUCTOR) != 0U)
		return 1;
	if (function->construct != NULL)
		return 1;

	/* Anything else. */
	return 0;
}

/*
 * Finds the prototype of an object new makes (GetPrototypeFromConstructor):
 * new.target's prototype property, or a fallback when that is not an
 * object.
 */
int
vm_construct_prototype(
	struct vm_realm *realm,
	vm_value new_target,
	struct vm_object *fallback,
	struct vm_object **prototype)
{
	vm_value key;
	vm_value value;
	int is_object;
	int status;

	/* new.target's prototype property. */
	*prototype = fallback;
	key = vm_key_from_ascii(realm->heap, "prototype");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_get(realm, new_target, key, &value);
	if (status != 0)
		return status;

	/* An object is the prototype; anything else leaves the fallback. */
	is_object = vm_value_is_object(value);
	if (is_object)
		*prototype = (struct vm_object *)vm_value_as_cell(value);

	/* Succeeded: the prototype. */
	return 0;
}

/*
 * Makes the object new makes for a constructor: its prototype is the
 * constructor's prototype property, or Object.prototype when that is not
 * an object.
 */
int
vm_construct_this(
	struct vm_realm *realm,
	vm_value constructor,
	vm_value *object)
{
	struct vm_object *prototype;
	struct vm_object *made;
	int status;

	/* The prototype. */
	status = vm_construct_prototype(realm, constructor, realm->object_prototype, &prototype);
	if (status != 0)
		return status;

	/* The object. */
	made = vm_object_create(realm->heap, prototype);
	if (made == NULL)
		return ENOMEM;

	/* Succeeded: the new object. */
	*object = vm_value_cell(made);
	return 0;
}

/*
 * Calls a function with a this value and arguments and stores its result;
 * returns 0, VM_THROWN with the realm's exception set, or an errno value.
 */
int
vm_call(
	struct vm_realm *realm,
	vm_value callee,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *function;
	int callable;
	int status;

	/* Only a function can be called (a TypeError when the realm has its constructors). */
	*result = VM_VALUE_UNDEFINED;
	callable = vm_value_is_callable(callee);
	if (!callable) {
		status = vm_throw(realm, VM_VALUE_UNDEFINED);
		return status;
	}

	/* A bytecode function runs in the interpreter; a class's constructor only with new. */
	function = (struct vm_function *)vm_value_as_cell(callee);
	if (function->code != NULL && (function->code->flags & VM_CODE_CLASS) != 0U) {
		status = vm_throw_class_call(realm, function);
		return status;
	}

	/* Any other bytecode function runs. */
	if (function->code != NULL) {
		status = vm_interpret(function->realm, function, this_value, args, count, result);
		return status;
	}

	/* A function with neither has nothing to run. */
	if (function->native == NULL)
		return ENOSYS;

	/* Runs the native code in the function's own realm (which tells it which function it is). */
	function->realm->callee = callee;
	function->realm->new_target = VM_VALUE_UNDEFINED;
	status = function->native(function->realm, this_value, args, count, result);
	if (status == VM_THROWN)
		return VM_THROWN;
	if (status != 0)
		return status;

	/* Succeeded: the result is stored. */
	return 0;
}

/*
 * Constructs with a constructor and a new.target (new, from C): a bytecode
 * constructor runs with a new object from new.target's prototype; a native
 * one makes its own.  Returns 0, VM_THROWN or an errno value.
 */
int
vm_construct(
	struct vm_realm *realm,
	vm_value constructor,
	const vm_value *args,
	unsigned count,
	vm_value new_target,
	vm_value *result)
{
	struct vm_function *function;
	struct vm_object *prototype;
	struct vm_object *made;
	int is_constructor;
	int status;

	/* Only a constructor. */
	*result = VM_VALUE_UNDEFINED;
	is_constructor = vm_value_is_constructor(constructor);
	if (!is_constructor) {
		status = vm_throw_type_error(realm, "value is not a constructor");
		return status;
	}

	/* A native constructor makes its object itself. */
	function = (struct vm_function *)vm_value_as_cell(constructor);
	if (function->code == NULL) {
		function->realm->callee = constructor;
		function->realm->new_target = new_target;
		status = function->construct(function->realm, VM_VALUE_UNDEFINED, args, count, result);
		if (status != 0)
			return status;
		return 0;
	}

	/* A derived class's constructor starts without this (its super call makes it). */
	if ((function->code->flags & VM_CODE_DERIVED) != 0U) {
		status = vm_interpret_construct(function->realm, function, VM_VALUE_EMPTY, args, count, new_target, result);
		if (status != 0)
			return status;
		return 0;
	}

	/* Any other bytecode one runs on a new object. */
	status = vm_construct_prototype(realm, new_target, realm->object_prototype, &prototype);
	if (status != 0)
		return status;
	made = vm_object_create(realm->heap, prototype);
	if (made == NULL)
		return ENOMEM;
	status = vm_interpret_construct(function->realm, function, vm_value_cell(made), args, count, new_target, result);
	if (status != 0)
		return status;

	/* Succeeded: the object made. */
	return 0;
}

/*
 * Throws a value: it becomes the realm's exception.  Returns VM_THROWN, so
 * a native function can end with "return vm_throw(...)".
 */
int
vm_throw(
	struct vm_realm *realm,
	vm_value exception)
{
	/* The exception waits in the realm for whoever catches it. */
	realm->exception = exception;

	/* Reports that a value was thrown. */
	return VM_THROWN;
}

/*
 * Finds where an exception left the bytecode it was thrown in.
 *
 * Returns 1 with the source line and column of the instruction that threw
 * it (or that called the native function that did), or 0 when the realm
 * has no place for that exception.
 */
int
vm_throw_site(
	const struct vm_realm *realm,
	vm_value exception,
	uint32_t *line,
	uint32_t *column)
{
	/* The place recorded is another exception's, or not known. */
	if (realm->throw_value != exception)
		return 0;
	if (realm->throw_line == 0)
		return 0;

	/* Succeeded: the place the exception was thrown from. */
	*line = realm->throw_line;
	*column = realm->throw_column;
	return 1;
}

/* Marks what a function refers to: what every object refers to, and its code (its realm's objects are the realm's tracer's). */
static void
function_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct vm_function *function;

	/* The object's shape, prototype, slots and elements. */
	vm_object_trace(heap, cell);

	/* The code unit it runs, the environment it runs in, and its data. */
	function = (struct vm_function *)cell;
	if (function->code != NULL)
		vm_heap_mark(heap, (struct vm_cell *)function->code);
	if (function->env != NULL)
		vm_heap_mark(heap, &function->env->cell);
	vm_heap_mark_value(heap, function->data);
}

/* Marks what an environment refers to: its parent and the values in its slots. */
static void
function_env_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct vm_env *env;
	vm_value *slots;
	uint32_t index;

	/* The parent. */
	env = (struct vm_env *)cell;
	if (env->parent != NULL)
		vm_heap_mark(heap, &env->parent->cell);

	/* Each slot's value. */
	slots = vm_env_slots(env);
	for (index = 0; index < env->count; index++)
		vm_heap_mark_value(heap, slots[index]);
}

/* Gives a constructor its prototype object, whose constructor property points back. */
static int
function_add_prototype(
	struct vm_realm *realm,
	struct vm_function *function)
{
	struct vm_object *prototype;
	vm_value key;
	int error;

	/* The object, from Object.prototype. */
	prototype = vm_object_create(realm->heap, realm->object_prototype);
	if (prototype == NULL)
		return ENOMEM;

	/* Its constructor: writable and configurable, not enumerable. */
	key = vm_key_from_ascii(realm->heap, "constructor");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_object_define(realm->heap, prototype, key, vm_value_cell(function),
	    VM_PROPERTY_WRITABLE | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* The function's prototype property: writable only. */
	key = vm_key_from_ascii(realm->heap, "prototype");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_object_define(realm->heap, &function->object, key, vm_value_cell(prototype), VM_PROPERTY_WRITABLE);
	if (error != 0)
		return error;

	/* Succeeded: the constructor has its prototype. */
	return 0;
}

/*
 * Gives a generator function its prototype object: the prototype of the
 * generators it makes, which inherits from %GeneratorPrototype% and has no
 * constructor property.
 */
static int
function_add_generator_prototype(
	struct vm_realm *realm,
	struct vm_function *function)
{
	struct vm_object *parent;
	struct vm_object *prototype;
	vm_value key;
	int error;

	/* The object, from %GeneratorPrototype% (Object.prototype before the built-ins). */
	parent = realm->intrinsics[VM_INTRINSIC_GENERATOR_PROTOTYPE];
	if (parent == NULL)
		parent = realm->object_prototype;
	prototype = vm_object_create(realm->heap, parent);
	if (prototype == NULL)
		return ENOMEM;

	/* The function's prototype property: writable only. */
	key = vm_key_from_ascii(realm->heap, "prototype");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_object_define(realm->heap, &function->object, key, vm_value_cell(prototype), VM_PROPERTY_WRITABLE);
	if (error != 0)
		return error;

	/* Succeeded: the generator function has its prototype. */
	return 0;
}
