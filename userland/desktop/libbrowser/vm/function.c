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
#include <stdlib.h>
#include <string.h>

/* Temporary invocation ownership includes direct arguments and collectible realms. */
struct function_roots {
	struct vm_heap *heap;
	struct vm_cell **cells;
	size_t registered;
};

/* Trace declarations are required by the cell-type initializers below. */
static void function_trace(struct vm_heap *heap, struct vm_cell *cell);
static void function_env_trace(struct vm_heap *heap, struct vm_cell *cell);

/* The cell type of functions: objects that also hold their realm's objects through the realm's tracer. */
const struct vm_cell_type vm_function_type = {
	"function", function_trace, vm_object_finalize
};

/* The cell type of environments: they hold their parent and their slots. */
const struct vm_cell_type vm_env_type = {
	"environment", function_env_trace, NULL
};

static int function_retain(struct vm_realm *realm, const vm_value *values, unsigned value_count, const vm_value *args, unsigned count, struct function_roots *roots);
static void function_release(struct function_roots *roots);
static int function_add_prototype(struct vm_realm *realm, struct vm_function *function);
static int function_add_generator_prototype(struct vm_realm *realm, struct vm_function *function);
static struct vm_object *function_default_prototype(struct vm_realm *realm, vm_value target, struct vm_object *fallback);
static int function_run_native(struct vm_function *function, vm_native native, vm_value this_value, const vm_value *args, unsigned count, vm_value new_target, vm_value *result);
static void function_transfer_exception(struct vm_realm *caller, struct vm_realm *callee, int status);

/*
 * Makes a native function with its realm, name and length properties.
 *
 * The configurable properties are neither writable nor enumerable.
 * Returns NULL when allocation or root registration fails.
 */
struct vm_function *
vm_function_create_native(
	struct vm_realm *realm,
	const char *name,
	unsigned length,
	vm_native native)
{
	struct vm_heap *heap;
	struct vm_function *function;
	struct vm_string *text;
	struct vm_cell *roots[3];
	vm_value key;
	unsigned index;
	unsigned registered;
	int error;

	/* Retains a collectible realm before the first allocation and both unpublished cells. */
	heap = realm->heap;
	function = NULL;
	roots[0] = NULL;
	roots[1] = NULL;
	roots[2] = NULL;

	/* Collectible realms need an explicit input edge before the function can own them. */
	if (realm->managed)
		roots[0] = &realm->cell;

	/* The acquisition count owns only slots successfully inserted into the heap registry. */
	registered = 0;
	for (index = 0; index < 3U; index++) {
		error = vm_heap_add_root(heap, &roots[index]);
		if (error != 0)
			goto cleanup;

		/* Cleanup owns this slot only after the heap registry has acquired it. */
		registered++;
	}

	/* Allocates the function before publishing its shape or native state. */
	function = vm_heap_alloc(heap, &vm_function_type, sizeof(*function));
	if (function == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Protects the newborn while initialization may allocate a cold root shape. */
	roots[1] = &function->object.cell;
	error = vm_object_init(heap, &function->object, realm->function_prototype);
	if (error != 0)
		goto cleanup;

	/* Retains only collectible realms without consulting retired manual owners. */
	function->realm = realm;

	/* The completed function supplies the durable edge only for collectible realms. */
	if (realm->managed)
		function->realm_owner = &realm->cell;

	/* Initializes the native entry point and its ordinary function state. */
	function->native = native;
	function->object.kind = VM_KIND_FUNCTION;
	function->data = VM_VALUE_UNDEFINED;

	/* Acquires the heap-owned length key before defining its configurable property. */
	key = vm_key_from_ascii(heap, "length");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Records the native argument count without writable or enumerable attributes. */
	error = vm_object_define(heap, &function->object, key, vm_value_int32((int32_t)length), VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		goto cleanup;

	/* Allocates the name before protecting it across subsequent key and shape allocations. */
	text = vm_string_from_utf8(heap, name, strlen(name));
	if (text == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Keeps the private name alive until its completed property owns it. */
	roots[2] = &text->cell;
	key = vm_key_from_ascii(heap, "name");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Publishes the name only after its key is available. */
	error = vm_object_define(heap, &function->object, key, vm_value_cell(text), VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		goto cleanup;

cleanup:
	/* Releases only registrations acquired by this invocation on every outcome. */
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(heap, &roots[registered]);
	}

	/* Failed construction publishes no partially initialized result. */
	if (error != 0)
		return NULL;

	/* Succeeded: the complete native function owns its realm and name. */
	return function;
}

/*
 * Makes a bytecode function from a realm and a checked code unit.
 *
 * Its name and length come from the code. Returns NULL on allocation failure.
 */
struct vm_function *
vm_function_create(
	struct vm_realm *realm,
	struct vm_code *code)
{
	struct vm_heap *heap;
	struct vm_function *function;
	struct vm_cell *roots[3];
	vm_value key;
	vm_value name;
	uint32_t length;
	unsigned index;
	unsigned registered;
	int error;

	/* Retains the input code, collectible realm and unpublished function for the whole factory. */
	heap = realm->heap;
	function = NULL;
	roots[0] = &code->cell;
	roots[1] = NULL;
	roots[2] = NULL;

	/* A collectible input realm cannot rely on an unpublished function's future edge. */
	if (realm->managed)
		roots[1] = &realm->cell;

	/* Only completed registry insertions belong to the factory's cleanup path. */
	registered = 0;
	for (index = 0; index < 3U; index++) {
		error = vm_heap_add_root(heap, &roots[index]);
		if (error != 0)
			goto cleanup;

		/* Cleanup owns this slot only after the heap registry has acquired it. */
		registered++;
	}

	/* Allocates a function without borrowing conservative caller retention. */
	function = vm_heap_alloc(heap, &vm_function_type, sizeof(*function));
	if (function == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Protects the partially initialized cell before any shape allocation. */
	roots[2] = &function->object.cell;
	error = vm_object_init(heap, &function->object, realm->function_prototype);
	if (error != 0)
		goto cleanup;

	/* Retains only collectible realms without consulting retired manual owners. */
	function->realm = realm;

	/* The completed function supplies the durable edge only for collectible realms. */
	if (realm->managed)
		function->realm_owner = &realm->cell;

	/* Initializes the bytecode entry point and its ordinary function state. */
	function->code = code;
	function->object.kind = VM_KIND_FUNCTION;
	function->data = VM_VALUE_UNDEFINED;

	/* Uses the parameter count before a default or rest parameter when the code records it. */
	length = code->parameter_count;

	/* Explicit length metadata excludes parameters after the first default or rest. */
	if ((code->flags & VM_CODE_LENGTH) != 0U)
		length = code->length;
	key = vm_key_from_ascii(heap, "length");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Defines the ordinary configurable length after acquiring its heap-owned key. */
	error = vm_object_define(heap, &function->object, key, vm_value_int32((int32_t)length), VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		goto cleanup;

	/* Uses the code-owned name, creating an empty string only when necessary. */
	name = VM_VALUE_EMPTY;
	if (code->name != NULL)
		name = vm_value_cell(code->name);
	if (name == VM_VALUE_EMPTY) {
		code->name = vm_string_from_utf8(heap, "", 0);
		if (code->name == NULL) {
			error = ENOMEM;
			goto cleanup;
		}

		/* The already rooted code now owns the generated name. */
		name = vm_value_cell(code->name);
	}

	/* Acquires the property key before publishing the protected name. */
	key = vm_key_from_ascii(heap, "name");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Completes the bytecode function's name property. */
	error = vm_object_define(heap, &function->object, key, name, VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		goto cleanup;

cleanup:
	/* No input or partially initialized function remains pinned after return. */
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(heap, &roots[registered]);
	}

	/* Failed construction publishes no partially initialized result. */
	if (error != 0)
		return NULL;

	/* Succeeded: the function owns its checked code unit and name. */
	return function;
}

/*
 * Makes a bytecode closure with its realm, code unit and captured environment.
 *
 * The environment may be NULL. Constructible code gives its closure a
 * prototype object. Returns NULL on allocation failure.
 */
struct vm_function *
vm_closure_create(
	struct vm_realm *realm,
	struct vm_code *code,
	struct vm_env *env)
{
	struct vm_heap *heap;
	struct vm_function *function;
	struct vm_cell *roots[2];
	unsigned index;
	unsigned registered;
	int error;

	/* Retains the captured environment while the bytecode factory owns code and realm inputs. */
	heap = realm->heap;
	function = NULL;
	roots[0] = NULL;
	roots[1] = NULL;

	/* A captured environment needs ownership before the inner bytecode factory allocates. */
	if (env != NULL)
		roots[0] = &env->cell;

	/* Only completed environment/output registrations belong to cleanup. */
	registered = 0;
	for (index = 0; index < 2U; index++) {
		error = vm_heap_add_root(heap, &roots[index]);
		if (error != 0)
			goto cleanup;

		/* Cleanup owns this slot only after the heap registry has acquired it. */
		registered++;
	}

	/* Creates the function before allocating any private constructor prototype. */
	function = vm_function_create(realm, code);
	if (function == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Publishes the captured environment under a root retained until all properties are complete. */
	roots[1] = &function->object.cell;
	function->env = env;

	/* Ordinary constructors own a private prototype; class setup provides its own. */
	if ((code->flags & VM_CODE_CONSTRUCTOR) != 0U && (code->flags & VM_CODE_CLASS) == 0U) {
		error = function_add_prototype(realm, function);
		if (error != 0)
			goto cleanup;
	}

	/* A generator keeps its private prototype and inherits from the realm's generator function prototype. */
	if ((code->flags & VM_CODE_GENERATOR) != 0U) {
		error = function_add_generator_prototype(realm, function);
		if (error != 0)
			goto cleanup;

		/* The completed generator properties permit publishing its intrinsic parent. */
		if (realm->intrinsics[VM_INTRINSIC_GENERATOR_FUNCTION_PROTOTYPE] != NULL)
			function->object.prototype = realm->intrinsics[VM_INTRINSIC_GENERATOR_FUNCTION_PROTOTYPE];
	}

	/* An async function inherits from the realm's async function prototype. */
	if ((code->flags & VM_CODE_ASYNC) != 0U && realm->intrinsics[VM_INTRINSIC_ASYNC_FUNCTION_PROTOTYPE] != NULL)
		function->object.prototype = realm->intrinsics[VM_INTRINSIC_ASYNC_FUNCTION_PROTOTYPE];

cleanup:
	/* Removes only this factory's successful root registrations. */
	while (registered != 0) {
		registered--;
		vm_heap_remove_root(heap, &roots[registered]);
	}

	/* Failed construction publishes no partially initialized result. */
	if (error != 0)
		return NULL;

	/* Succeeded: the complete closure owns its code, environment and prototype. */
	return function;
}

/*
 * Makes an environment with a parent and initialized undefined slots.
 *
 * The parent may be NULL. Returns NULL on allocation or size overflow.
 */
struct vm_env *
vm_env_create(
	struct vm_heap *heap,
	struct vm_env *parent,
	uint32_t count)
{
	struct vm_env *env;
	struct vm_cell *root;
	vm_value *slots;
	size_t slot_count;
	size_t capacity;
	size_t size;
	uint32_t index;
	int error;

	/* Rejects a slot count whose complete allocation would overflow the host's size type. */
	slot_count = count;
	capacity = (SIZE_MAX - sizeof(*env)) / sizeof(vm_value);
	if (slot_count > capacity)
		return NULL;

	/* Retains an otherwise unowned parent before the allocation can trigger collection. */
	root = NULL;

	/* A non-null parent is the factory's only allocating input. */
	if (parent != NULL)
		root = &parent->cell;

	/* Acquire parent ownership before requesting the complete child allocation. */
	error = vm_heap_add_root(heap, &root);
	if (error != 0)
		return NULL;

	/* Allocates the header and slots before releasing temporary parent ownership. */
	size = sizeof(*env) + slot_count * sizeof(vm_value);
	env = vm_heap_alloc(heap, &vm_env_type, size);
	if (env == NULL) {
		vm_heap_remove_root(heap, &root);
		return NULL;
	}

	/* The checked child can retain its parent without any intervening allocation. */
	vm_heap_remove_root(heap, &root);

	/* Publishes the parent and slot count without a further allocation. */
	env->parent = parent;
	env->count = count;
	slots = vm_env_slots(env);
	for (index = 0; index < count; index++)
		slots[index] = VM_VALUE_UNDEFINED;

	/* Succeeded: the environment retains its parent and initialized slots. */
	return env;
}

/*
 * Finds the slots of an environment.
 */
vm_value *
vm_env_slots(
	struct vm_env *env)
{
	/* Succeeded: the slots follow the environment header. */
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
	if (cell->type != &vm_function_type)
		return 0;

	/* Succeeded: a function cell can be called. */
	return 1;
}

/*
 * Tells whether a value can construct with new.
 *
 * Bytecode must permit construction; natives need construction behavior.
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
	if (function->code != NULL && (function->code->flags & VM_CODE_CONSTRUCTOR) != 0U) {
		/* Succeeded: the bytecode explicitly permits construction. */
		return 1;
	}

	/* A native without construction behavior cannot be used with new. */
	if (function->construct == NULL)
		return 0;

	/* Succeeded: the native supplies its construction behavior. */
	return 1;
}

/*
 * Finds a new object's prototype through GetPrototypeFromConstructor.
 *
 * Uses new.target's prototype property or a realm-specific fallback.
 */
int
vm_construct_prototype(
	struct vm_realm *realm,
	vm_value new_target,
	struct vm_object *fallback,
	struct vm_object **prototype)
{
	struct function_roots roots;
	vm_value inputs[2];
	vm_value key;
	vm_value value;
	int is_object;
	int status;

	/* Direct GetPrototypeFromConstructor callers may have no VM execution frame. */
	inputs[0] = new_target;
	inputs[1] = VM_VALUE_UNDEFINED;
	if (fallback != NULL)
		inputs[1] = vm_value_cell(fallback);
	status = function_retain(realm, inputs, 2, NULL, 0, &roots);
	if (status != 0)
		return status;

	/* Acquires the heap-owned key while preserving the target and fallback. */
	*prototype = fallback;
	key = vm_key_from_ascii(realm->heap, "prototype");
	if (key == VM_VALUE_EMPTY) {
		status = ENOMEM;
		goto cleanup;
	}

	/* The getter may invoke script, reenter native code or collect the heap. */
	status = vm_get(realm, new_target, key, &value);
	if (status != 0)
		goto cleanup;

	/* Uses an explicit object prototype or the target realm's default intrinsic. */
	is_object = vm_value_is_object(value);
	if (is_object) {
		*prototype = (struct vm_object *)vm_value_as_cell(value);
	} else {
		*prototype = function_default_prototype(realm, new_target, fallback);
	}

cleanup:
	/* No temporary target, fallback or realm root remains after the lookup. */
	function_release(&roots);
	if (status != 0)
		return status;

	/* Succeeded: the prototype is available before the caller's next allocation. */
	return 0;
}

/*
 * Makes a constructor's receiver from its prototype property.
 *
 * Uses Object.prototype when the constructor's property is not an object.
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
 * Calls a function with its receiver and arguments and stores its result.
 *
 * Returns 0, VM_THROWN with the realm's exception set, or an errno value.
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
	struct function_roots roots;
	vm_value inputs[2];
	int callable;
	int status;

	/* Rejects a non-callable value in the caller's realm. */
	*result = VM_VALUE_UNDEFINED;
	callable = vm_value_is_callable(callee);
	if (!callable) {
		status = vm_throw(realm, VM_VALUE_UNDEFINED);
		if (status != 0)
			return status;

		/* Succeeded: the refusal follows its existing native throw contract. */
		return 0;
	}

	/* Retains the direct callee, receiver, arguments and caller realm across reentrant execution. */
	inputs[0] = callee;
	inputs[1] = this_value;
	status = function_retain(realm, inputs, 2, args, count, &roots);
	if (status != 0)
		return status;

	/* Rejects a class constructor invoked without new. */
	function = (struct vm_function *)vm_value_as_cell(callee);
	if (function->code != NULL &&
	    (function->code->flags & VM_CODE_CLASS) != 0U) {
		status = vm_throw_class_call(realm, function);
		if (status != 0)
			goto cleanup;

		/* Successful refusal still completes through the same ownership cleanup. */
		goto cleanup;
	}

	/* Runs script code in the callee's realm, including suspended functions. */
	if (function->code != NULL) {
		status = vm_interpret(
		    function->realm,
		    function,
		    this_value,
		    args,
		    count,
		    result);
		if (status != 0) {
			function_transfer_exception(realm, function->realm, status);
			goto cleanup;
		}
	} else {
		/* A native-less function cannot be executed. */
		if (function->native == NULL) {
			status = ENOSYS;
			goto cleanup;
		}

		/* Preserves the native caller's callee and construction state. */
		status = function_run_native(
		    function,
		    function->native,
		    this_value,
		    args,
		    count,
		    VM_VALUE_UNDEFINED,
		    result);
		if (status != 0) {
			function_transfer_exception(realm, function->realm, status);
			goto cleanup;
		}
	}

cleanup:
	/* Unwinds direct input ownership after any foreign exception has reached its caller. */
	function_release(&roots);
	if (status != 0)
		return status;

	/* Succeeded: the callee's result is available in the caller. */
	return 0;
}

/*
 * Constructs an object with a constructor and new.target from C.
 *
 * Bytecode runs with a new receiver from new.target's prototype; a native
 * makes its own. Returns 0, VM_THROWN or an errno value.
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
	struct function_roots roots;
	vm_value inputs[2];
	struct vm_object *prototype;
	struct vm_object *made;
	int is_constructor;
	int status;

	/* Rejects a non-constructor in the caller's realm. */
	*result = VM_VALUE_UNDEFINED;
	is_constructor = vm_value_is_constructor(constructor);
	if (!is_constructor) {
		/* Error construction may collect an otherwise unowned managed caller realm. */
		status = function_retain(realm, NULL, 0, NULL, 0, &roots);
		if (status != 0)
			return status;

		/* Preserve the existing TypeError refusal before validating direct arguments. */
		status = vm_throw_type_error(realm, "value is not a constructor");
		if (status != 0)
			goto cleanup;

		/* Successful refusal uses the same temporary realm ownership cleanup. */
		goto cleanup;
	}

	/* Constructor input ownership begins before any prototype getter or receiver allocation. */
	inputs[0] = constructor;
	inputs[1] = new_target;
	status = function_retain(realm, inputs, 2, args, count, &roots);
	if (status != 0)
		return status;

	/* A native constructor creates its object in its own realm. */
	function = (struct vm_function *)vm_value_as_cell(constructor);
	if (function->code == NULL) {
		status = function_run_native(
		    function,
		    function->construct,
		    VM_VALUE_UNDEFINED,
		    args,
		    count,
		    new_target,
		    result);
		if (status != 0) {
			function_transfer_exception(realm, function->realm, status);
			goto cleanup;
		}
	} else if ((function->code->flags & VM_CODE_DERIVED) != 0U) {
		/* A derived constructor's super call supplies this. */
		status = vm_interpret_construct(
		    function->realm,
		    function,
		    VM_VALUE_EMPTY,
		    args,
		    count,
		    new_target,
		    result);
		if (status != 0) {
			function_transfer_exception(realm, function->realm, status);
			goto cleanup;
		}
	} else {
		/* Allocates this using new.target's prototype and the callee's fallback. */
		status = vm_construct_prototype(
		    function->realm,
		    new_target,
		    function->realm->object_prototype,
		    &prototype);
		if (status != 0) {
			function_transfer_exception(realm, function->realm, status);
			goto cleanup;
		}

		/* Makes the ordinary constructor's receiver in the shared heap. */
		made = vm_object_create(function->realm->heap, prototype);
		if (made == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Runs the constructor without losing its realm or new.target. */
		status = vm_interpret_construct(
		    function->realm,
		    function,
		    vm_value_cell(made),
		    args,
		    count,
		    new_target,
		    result);
		if (status != 0) {
			function_transfer_exception(realm, function->realm, status);
			goto cleanup;
		}
	}

cleanup:
	/* Releases direct constructor inputs only after execution and exception transfer finish. */
	function_release(&roots);
	if (status != 0)
		return status;

	/* Succeeded: the constructor's object is available to the caller. */
	return 0;
}

/*
 * Stores a thrown value in the realm's pending exception.
 *
 * Returns VM_THROWN for the caller to propagate through its failure path.
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

/* Owns direct invocation values without depending on conservative C-stack scanning. */
static int
function_retain(
	struct vm_realm *realm,
	const vm_value *values,
	unsigned value_count,
	const vm_value *args,
	unsigned count,
	struct function_roots *roots)
{
	vm_value value;
	size_t total;
	size_t capacity;
	size_t index;
	int is_cell;
	int status;

	/* Initializes resumable cleanup before validating or allocating the temporary slot array. */
	roots->heap = realm->heap;
	roots->cells = NULL;
	roots->registered = 0;

	/* Nonempty direct argument lists must have actual storage to snapshot into roots. */
	if (count != 0 && args == NULL)
		return EINVAL;

	/* Bounds the complete realm, fixed-input and argument root array on this host. */
	capacity = SIZE_MAX / sizeof(*roots->cells);
	total = value_count;
	if (total >= capacity)
		return ENOMEM;
	total++;

	/* The remaining root-array capacity must hold every direct argument. */
	if ((size_t)count > capacity - total)
		return ENOMEM;
	total += count;
	roots->cells = calloc(total, sizeof(*roots->cells));
	if (roots->cells == NULL)
		return ENOMEM;

	/* Manual realms have their permanent tracer; collectible realms require an invocation edge. */
	if (realm->managed)
		roots->cells[0] = &realm->cell;

	/* Snapshot fixed values and arguments before registration can fail partway. */
	for (index = 1; index < total; index++) {
		if (index <= value_count) {
			value = values[index - 1];
		} else {
			value = args[index - value_count - 1];
		}

		/* Only actual cells enter the collector's pointer-root registry. */
		is_cell = vm_value_is_cell(value);
		if (is_cell)
			roots->cells[index] = vm_value_as_cell(value);
	}

	/* Registration uses ordinary C allocation, so no VM participant can collect during acquisition. */
	for (index = 0; index < total; index++) {
		status = vm_heap_add_root(roots->heap, &roots->cells[index]);
		if (status != 0) {
			function_release(roots);
			return status;
		}

		/* Only successful registrations belong to subsequent cleanup. */
		roots->registered++;
	}

	/* Succeeded: all direct cell inputs and the collectible realm have invocation ownership. */
	return 0;
}

/* Releases only roots acquired by the matching invocation's successful registrations. */
static void
function_release(
	struct function_roots *roots)
{
	/* Remove registry pointers before releasing the C storage that contains them. */
	while (roots->registered != 0) {
		roots->registered--;
		vm_heap_remove_root(roots->heap, &roots->cells[roots->registered]);
	}

	/* Root storage is no longer referenced by the collector registry. */
	free(roots->cells);
	roots->cells = NULL;

	/* Succeeded: invocation ownership no longer keeps any VM cell alive. */
	return;
}

/* Resolves a default intrinsic in a direct function target's own realm. */
static struct vm_object *
function_default_prototype(
	struct vm_realm *realm,
	vm_value target,
	struct vm_object *fallback)
{
	struct vm_function *function;
	struct vm_realm *target_realm;
	unsigned index;
	int callable;

	/* A host-provided non-function target retains its explicit fallback. */
	callable = vm_value_is_callable(target);
	if (!callable)
		return fallback;

	/* The common same-realm case requires no intrinsic translation. */
	function = (struct vm_function *)vm_value_as_cell(target);
	target_realm = function->realm;
	if (target_realm == realm)
		return fallback;

	/* Translates the three foundational prototypes stored outside intrinsics. */
	if (fallback == realm->object_prototype)
		return target_realm->object_prototype;

	/* Function.prototype is independently initialized in each realm. */
	if (fallback == realm->function_prototype)
		return target_realm->function_prototype;

	/* Array.prototype is likewise a realm-specific foundational object. */
	if (fallback == realm->array_prototype)
		return target_realm->array_prototype;

	/* Maps any registered default intrinsic to its counterpart in the target. */
	for (index = 0; index < VM_INTRINSICS; index++) {
		/* Only a constructed intrinsic can match the provided fallback. */
		if (realm->intrinsics[index] == NULL)
			continue;

		/* Keeps fallback identity within the target function's realm. */
		if (fallback == realm->intrinsics[index])
			return target_realm->intrinsics[index];
	}

	/* Succeeded: a host-specific fallback retains its explicit ownership. */
	return fallback;
}

/* Runs a native with scoped callee/new.target state, including reentrant calls. */
static int
function_run_native(
	struct vm_function *function,
	vm_native native,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value new_target,
	vm_value *result)
{
	struct vm_realm *realm;
	struct function_roots roots;
	vm_value saved[2];
	int status;

	/* Retains suspended callee and new.target edges before temporarily replacing them. */
	realm = function->realm;
	saved[0] = realm->callee;
	saved[1] = realm->new_target;
	status = function_retain(realm, saved, 2, NULL, 0, &roots);
	if (status != 0)
		return status;

	/* The public dispatcher already owns direct function, receiver and argument inputs. */
	realm->callee = vm_value_cell(function);
	realm->new_target = new_target;
	status = native(realm, this_value, args, count, result);
	if (status != 0) {
		realm->callee = saved[0];
		realm->new_target = saved[1];
		goto cleanup;
	}

	/* Lets the suspended outer native observe its original invocation again. */
	realm->callee = saved[0];
	realm->new_target = saved[1];

cleanup:
	/* Suspended state returns to the realm before temporary roots are released. */
	function_release(&roots);
	if (status != 0)
		return status;

	/* Succeeded: the native produced its result with the outer state intact. */
	return 0;
}

/* Moves a foreign thrown value and its source metadata to the catching realm. */
static void
function_transfer_exception(
	struct vm_realm *caller,
	struct vm_realm *callee,
	int status)
{
	/* Ordinary failures carry no JavaScript exception. */
	if (status != VM_THROWN)
		return;

	/* A same-realm call already published its exception in the right place. */
	if (caller == callee)
		return;

	/* Transfers the actual object rather than constructing a caller-realm copy. */
	caller->exception = callee->exception;
	caller->throw_value = callee->throw_value;
	caller->throw_line = callee->throw_line;
	caller->throw_column = callee->throw_column;

	/* The caller now owns the pending throw; the callee no longer retains it. */
	callee->exception = VM_VALUE_UNDEFINED;
	callee->throw_value = VM_VALUE_UNDEFINED;
	callee->throw_line = 0;
	callee->throw_column = 0;

	/* Succeeded: the foreign exception is pending only in its caller. */
	return;
}

/* Marks a function's environment, code, data and collectible child realm. */
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
	vm_heap_mark(heap, function->realm_owner);
	if (function->code != NULL)
		vm_heap_mark(heap, (struct vm_cell *)function->code);
	if (function->env != NULL)
		vm_heap_mark(heap, &function->env->cell);
	vm_heap_mark_value(heap, function->data);

	/* Succeeded: the function's owned graph was marked. */
	return;
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

	/* Succeeded: the parent and initialized environment slots were marked. */
	return;
}

/* Gives a constructor its prototype object, whose constructor property points back. */
static int
function_add_prototype(
	struct vm_realm *realm,
	struct vm_function *function)
{
	struct vm_heap *heap;
	struct vm_object *parent;
	struct vm_object *prototype;
	struct vm_cell *root;
	vm_value key;
	int error;

	/* The closure factory owns its function while this helper protects the unpublished prototype. */
	heap = realm->heap;
	root = NULL;
	error = vm_heap_add_root(heap, &root);
	if (error != 0)
		return error;

	/* Chooses the realm's ordinary prototype before allocating the private child. */
	parent = realm->object_prototype;
	prototype = vm_object_create(heap, parent);
	if (prototype == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* The completed object remains protected until the function owns its property. */
	root = &prototype->cell;
	key = vm_key_from_ascii(heap, "constructor");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Defines the reciprocal writable and configurable constructor property. */
	error = vm_object_define(heap, prototype, key, vm_value_cell(function), VM_PROPERTY_WRITABLE | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		goto cleanup;

	/* Acquires the heap-owned key before linking the private prototype to its function. */
	key = vm_key_from_ascii(heap, "prototype");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Publishes the private prototype with the ordinary writable-only descriptor. */
	error = vm_object_define(heap, &function->object, key, vm_value_cell(prototype), VM_PROPERTY_WRITABLE);
	if (error != 0)
		goto cleanup;

cleanup:
	/* Releases the temporary prototype root on both failure and successful publication. */
	vm_heap_remove_root(heap, &root);
	if (error != 0)
		return error;

	/* Succeeded: the function owns its completed prototype. */
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
	struct vm_heap *heap;
	struct vm_object *parent;
	struct vm_object *prototype;
	struct vm_cell *root;
	vm_value key;
	int error;

	/* The closure factory owns its function while this helper protects the unpublished prototype. */
	heap = realm->heap;
	root = NULL;
	error = vm_heap_add_root(heap, &root);
	if (error != 0)
		return error;

	/* Chooses the realm's ordinary prototype before allocating the private child. */
	parent = realm->object_prototype;
	if (realm->intrinsics[VM_INTRINSIC_GENERATOR_PROTOTYPE] != NULL)
		parent = realm->intrinsics[VM_INTRINSIC_GENERATOR_PROTOTYPE];
	prototype = vm_object_create(heap, parent);
	if (prototype == NULL) {
		error = ENOMEM;
		goto cleanup;
	}

	/* The completed object remains protected until the function owns its property. */
	root = &prototype->cell;
	key = vm_key_from_ascii(heap, "prototype");
	if (key == VM_VALUE_EMPTY) {
		error = ENOMEM;
		goto cleanup;
	}

	/* Publishes the private prototype with the ordinary writable-only descriptor. */
	error = vm_object_define(heap, &function->object, key, vm_value_cell(prototype), VM_PROPERTY_WRITABLE);
	if (error != 0)
		goto cleanup;

cleanup:
	/* Releases the temporary prototype root on both failure and successful publication. */
	vm_heap_remove_root(heap, &root);
	if (error != 0)
		return error;

	/* Succeeded: the function owns its completed prototype. */
	return 0;
}
