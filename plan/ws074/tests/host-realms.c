/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Verifies bidirectional calls, construction, unwinding and native reentry
 * between two live realms sharing one heap. The original eight-check q510
 * investigation remains a separate fixture.
 */

#include "js/js.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Counts independent checks without hiding failures behind a final script. */
struct realm_probe {
	unsigned checks;
	unsigned failures;
};

static int probe_realm(struct vm_heap *heap, struct vm_realm **realm);
static int probe_run(struct realm_probe *probe, struct vm_realm *parent, struct vm_realm *child);
static int probe_script(struct vm_realm *realm, const char *source, vm_value *answer);
static int probe_check(struct realm_probe *probe, struct vm_realm *realm, const char *label, const char *source);
static int probe_construct_target(struct vm_realm *parent, struct vm_realm *child, const char *name);
static int probe_api(struct realm_probe *probe, struct vm_realm *parent, struct vm_realm *child);
static int probe_native_reentry(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int probe_nomem(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int probe_throw(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);

/*
 * Checks calls and exception transport between two realms in one heap.
 */
int
main(
	void)
{
	struct realm_probe probe;
	struct vm_heap *heap;
	struct vm_realm *parent;
	struct vm_realm *child;
	int error;
	int printed;

	/* Leaves partially constructed resources safe to unwind. */
	memset(&probe, 0, sizeof(probe));
	heap = NULL;
	parent = NULL;
	child = NULL;

	/* Shares only the heap, as documents in one tab must. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));

	/* Installs each realm, unwinding only resources already acquired. */
	error = probe_realm(heap, &parent);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Keeps the parent's tracer installed while constructing the child. */
	error = probe_realm(heap, &child);
	if (error != 0) {
		vm_realm_destroy(parent);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Executes the bounded inventory before dismantling either realm. */
	error = probe_run(&probe, parent, child);
	if (error != 0) {
		vm_realm_destroy(child);
		vm_realm_destroy(parent);
		vm_heap_destroy(heap);
		printed = fprintf(
		    stderr,
		    "realm probe infrastructure error: %d\n",
		    error);
		if (printed < 0)
			return 2;
		return 2;
	}

	/* Successful execution is checked before releasing the two explicit realm tracers. */
	vm_realm_destroy(child);
	vm_realm_destroy(parent);
	vm_heap_destroy(heap);

	/* Preserves failing outcomes instead of treating reproduction as a pass. */
	printed = printf(
	    "realm checks: %u/%u passed\n",
	    probe.checks - probe.failures,
	    probe.checks);
	if (printed < 0)
		return 2;

	/* Every semantic assertion must pass after successful infrastructure teardown. */
	if (probe.failures != 0)
		return 1;

	/* Succeeded: both realms retained their execution and exception identity. */
	return 0;
}

/* Makes one fully initialized realm or releases its partial construction. */
static int
probe_realm(
	struct vm_heap *heap,
	struct vm_realm **realm)
{
	int error;

	/* Installs the realm's skeleton before allocating built-ins. */
	error = vm_realm_create(heap, realm);
	if (error != 0)
		return error;

	/* Keeps a failed built-in installation from leaving a live tracer. */
	error = js_install_builtins(*realm);
	if (error != 0) {
		vm_realm_destroy(*realm);
		*realm = NULL;
		return error;
	}

	/* Succeeded: scripts can use the initialized realm. */
	return 0;
}

/* Runs behavioral assertions while the caller owns both realms. */
static int
probe_run(
	struct realm_probe *probe,
	struct vm_realm *parent,
	struct vm_realm *child)
{
	struct vm_function *native;
	vm_value answer;
	int error;

	/* Gives each realm independently observable state and script functions. */
	error = probe_script(
	    parent,
	    "var marker=11;"
	    " function read(){return marker;"
	    "} function raise(){throw new TypeError('parent');"
	    "} function relay(n,back){if(n===0)return marker;"
	    "return back(n-1,relay)+marker;"
	    "} function Target(){} var asyncValue=0;",
	    &answer);
	if (error != 0)
		return error;

	/* Defines child closures, constructors, accessors and suspended functions. */
	error = probe_script(
	    child,
	    "var marker=22;"
	    "\n\nfunction raise(){throw new TypeError('child');"
	    "}\nfunction read(){return marker;"
	    "} function sum(x){return this.value+marker+x;"
	    "} function bounce(n,back){if(n===0)return marker;"
	    "return back(n-1,bounce)+marker;"
	    "} function closure(x){return function(y){x+=y;"
	    "return x+marker;"
	    "};"
	    "} function Box(x){this.value=x+marker;"
	    "this.target=new.target;"
	    "} class Derived extends Box{constructor(x){super(x);"
	    "this.extra=marker;"
	    "}} this.Derived=Derived; function* gen(){yield marker;"
	    "} function* badGen(){throw new TypeError('generator');"
	    "} async function asyncRead(){return marker;"
	    "} Object.defineProperty(this,'bad',{get:function(){throw new TypeE"
	    "rror('getter');"
	    "},set:function(){throw new TypeError('setter');"
	    "}});",
	    &answer);
	if (error != 0)
		return error;

	/* Connects the two live globals in both directions. */
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "child",
	    vm_value_cell(child->global),
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Publishes the checked native value into its owning global for subsequent assertions. */
	error = js_builtin_value(
	    child,
	    child->global,
	    "parent",
	    vm_value_cell(parent->global),
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Exposes a nested native call whose callee state must survive reentry. */
	native = vm_function_create_native(child, "reentry", 1, probe_native_reentry);
	if (native == NULL)
		return ENOMEM;
	error = js_builtin_value(
	    child,
	    child->global,
	    "reentry",
	    vm_value_cell(native),
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Exercises child arguments and this. */
	error = probe_check(
	    probe,
	    parent,
	    "child arguments and this",
	    "child.sum.call({value:2},3)===27;");
	if (error != 0)
		return error;

	/* Exercises child closure environment. */
	error = probe_check(
	    probe,
	    parent,
	    "child closure environment",
	    "var closed=child.closure(1); closed(3)===26 && closed(2)===28;");
	if (error != 0)
		return error;

	/* Exercises child constructor. */
	error = probe_check(
	    probe,
	    parent,
	    "child constructor",
	    "var box=new child.Box(3); box.value===25 && "
	    "box.target===child.Box && "
	    "Object.getPrototypeOf(box)===child.Box.prototype;");
	if (error != 0)
		return error;

	/* Exercises child derived constructor. */
	error = probe_check(
	    probe,
	    parent,
	    "child derived constructor",
	    "var derived=new child.Derived(4); derived.value===26;");
	if (error != 0)
		return error;

	/* Verifies the derived constructor retained its child global. */
	error = probe_check(
	    probe,
	    parent,
	    "derived realm",
	    "derived.extra===22;");
	if (error != 0)
		return error;

	/* Verifies super retained the originating construction target. */
	error = probe_check(
	    probe,
	    parent,
	    "derived target",
	    "derived.target===child.Derived;");
	if (error != 0)
		return error;

	/* Verifies the derived object retained its parent prototype. */
	error = probe_check(
	    probe,
	    parent,
	    "derived prototype",
	    "derived instanceof child.Box;");
	if (error != 0)
		return error;

	/* Exercises a supplied construction target through the embedding API. */
	error = probe_construct_target(parent, child, "Box");
	if (error != 0)
		return error;

	/* Exercises foreign new.target. */
	error = probe_check(
	    probe,
	    parent,
	    "foreign new.target",
	    "mixed.value===24;");
	if (error != 0)
		return error;

	/* Verifies a foreign constructor saw the supplied new.target. */
	error = probe_check(
	    probe,
	    parent,
	    "foreign target identity",
	    "mixed.target===Target;");
	if (error != 0)
		return error;

	/* Verifies the receiver's prototype comes from the supplied new.target. */
	error = probe_check(
	    probe,
	    parent,
	    "foreign target prototype",
	    "Object.getPrototypeOf(mixed)===Target.prototype;");
	if (error != 0)
		return error;

	/* Exercises returning reentry. */
	error = probe_check(
	    probe,
	    parent,
	    "returning reentry",
	    "child.bounce(2,relay)===55 && marker===11;");
	if (error != 0)
		return error;

	/* Exercises native state on reentry. */
	error = probe_check(
	    probe,
	    parent,
	    "native state on reentry",
	    "child.reentry(function(){return child.parseInt('7',10);});");
	if (error != 0)
		return error;

	/* Exercises child getter throw. */
	error = probe_check(
	    probe,
	    parent,
	    "child getter throw",
	    "var caught=false;try{child.bad;}catch(e){caught=e instanceof "
	    "child.TypeError;}caught;");
	if (error != 0)
		return error;

	/* Exercises child setter throw. */
	error = probe_check(
	    probe,
	    parent,
	    "child setter throw",
	    "caught=false;try{child.bad=1;}catch(e){caught=e instanceof "
	    "child.TypeError;}caught;");
	if (error != 0)
		return error;

	/* Exercises foreign generator. */
	error = probe_check(
	    probe,
	    parent,
	    "foreign generator",
	    "var g=child.gen(); g.next().value===22 && g.next().done;");
	if (error != 0)
		return error;

	/* Exercises foreign generator throw. */
	error = probe_check(
	    probe,
	    parent,
	    "foreign generator throw",
	    "caught=false;try{child.badGen().next();}catch(e){caught=e "
	    "instanceof child.TypeError;}caught;");
	if (error != 0)
		return error;

	/* Exercises foreign async promise. */
	error = probe_check(
	    probe,
	    parent,
	    "foreign async promise",
	    "var pending=child.asyncRead(); "
	    "pending.then(function(x){asyncValue=x;});pending instanceof "
	    "child.Promise;");
	if (error != 0)
		return error;

	/* Exercises bidirectional global. */
	error = probe_check(
	    probe,
	    child,
	    "bidirectional global",
	    "parent.read()===11 && marker===22;");
	if (error != 0)
		return error;

	/* Exercises bidirectional exception. */
	error = probe_check(
	    probe,
	    child,
	    "bidirectional exception",
	    "var caught=false;try{parent.raise();}catch(e){caught=e instanceof "
	    "parent.TypeError;}caught;");
	if (error != 0)
		return error;

	/* Exercises stack limit across realms. */
	error = probe_check(
	    probe,
	    parent,
	    "stack limit across realms",
	    "function loop(){return child.loop();} "
	    "child.loop=child.Function('return parent.loop()');var "
	    "caught=false;try{loop();}catch(e){caught=e.name==='RangeError';}ca"
	    "ught;");
	if (error != 0)
		return error;

	/* Exercises recovered after stack limit. */
	error = probe_check(
	    probe,
	    parent,
	    "recovered after stack limit",
	    "child.read()===22 && read()===11;");
	if (error != 0)
		return error;

	/* Runs the child's jobs while the parent callback retains its own realm. */
	error = vm_run_jobs(child, NULL, NULL);
	if (error != 0)
		return error;

	/* Completes pending jobs in this realm before later asynchronous assertions. */
	error = vm_run_jobs(parent, NULL, NULL);
	if (error != 0)
		return error;

	/* Exercises async callback realm. */
	error = probe_check(
	    probe,
	    parent,
	    "async callback realm",
	    "asyncValue===22 && marker===11;");
	if (error != 0)
		return error;

	/* Uses the target realm's default when its prototype is primitive. */
	error = probe_script(parent, "Target.prototype=7;", &answer);
	if (error != 0)
		return error;

	/* Constructs the next foreign receiver with the parent realm's explicit target. */
	error = probe_construct_target(parent, child, "Box");
	if (error != 0)
		return error;

	/* Records this independent named realm behavior before proceeding to later checks. */
	error = probe_check(
	    probe,
	    parent,
	    "target object fallback",
	    "Object.getPrototypeOf(mixed)===Object.prototype;");
	if (error != 0)
		return error;

	/* Maps native constructor defaults to the same target realm. */
	error = probe_construct_target(parent, child, "Array");
	if (error != 0)
		return error;

	/* Records this independent named realm behavior before proceeding to later checks. */
	error = probe_check(
	    probe,
	    parent,
	    "target array fallback",
	    "Object.getPrototypeOf(mixed)===Array.prototype && mixed.length===2;");
	if (error != 0)
		return error;

	/* Constructs the next foreign receiver with the parent realm's explicit target. */
	error = probe_construct_target(parent, child, "Error");
	if (error != 0)
		return error;

	/* Records this independent named realm behavior before proceeding to later checks. */
	error = probe_check(
	    probe,
	    parent,
	    "target error fallback",
	    "Object.getPrototypeOf(mixed)===Error.prototype;");
	if (error != 0)
		return error;

	/* Verifies C entry points, throw metadata and allocation-error unwinding. */
	error = probe_api(probe, parent, child);
	if (error != 0)
		return error;

	/* Succeeded: all behavioral and C-entry assertions were inventoried. */
	return 0;
}

/* Runs a small ASCII script without depending on page or iframe bindings. */
static int
probe_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct js_syntax_error syntax;
	struct wb_units units;
	int error;

	/* Converts source through the same UTF-16 boundary as normal scripts. */
	wb_units_init(&units);
	error = wb_utf8_to_units(
	    (const unsigned char *)source,
	    strlen(source),
	    &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Executes the source while its UTF-16 storage remains available. */
	error = js_run_script(
	    realm,
	    units.data,
	    units.length,
	    0,
	    answer,
	    &syntax);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* The checked completion no longer needs its temporary source storage. */
	wb_units_release(&units);

	/* Succeeded: the last expression's value is available to the assertion. */
	return 0;
}

/* Reports one assertion while retaining every failed check. */
static int
probe_check(
	struct realm_probe *probe,
	struct vm_realm *realm,
	const char *label,
	const char *source)
{
	vm_value answer;
	const char *outcome;
	int error;
	int printed;

	/* An uncaught exception is a failed check, not an infrastructure success. */
	error = probe_script(realm, source, &answer);
	if (error != 0 && error != VM_THROWN)
		return error;

	/* Records only a true assertion as a pass. */
	probe->checks++;
	outcome = "PASS";
	if (error != 0 || answer != VM_VALUE_TRUE) {
		probe->failures++;
		outcome = "FAIL";
	}

	/* Makes later checks independent of a previous thrown value. */
	realm->exception = VM_VALUE_UNDEFINED;
	printed = printf("%s %s\n", outcome, label);
	if (printed < 0)
		return EIO;

	/* Succeeded: the assertion's actual outcome is in the inventory. */
	return 0;
}

/* Throws a child TypeError to exercise exception transport from a native. */
static int
probe_throw(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(result);

	/* Supplies an exception whose constructor belongs to the child realm. */
	error = vm_throw_type_error(realm, "child native");
	if (error != 0)
		return error;

	/* Succeeded: the VM accepted the requested exception operation. */
	return 0;
}

/* Constructs a child function with a parent's explicit new.target. */
static int
probe_construct_target(
	struct vm_realm *parent,
	struct vm_realm *child,
	const char *name)
{
	vm_value key;
	vm_value constructor;
	vm_value target;
	vm_value argument;
	vm_value object;
	int error;

	/* Finds the function to construct in the child global. */
	key = vm_key_from_ascii(parent->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_get(parent, vm_value_cell(child->global), key, &constructor);
	if (error != 0)
		return error;

	/* Finds the explicitly different target in the parent global. */
	key = vm_key_from_ascii(parent->heap, "Target");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_get(parent, vm_value_cell(parent->global), key, &target);
	if (error != 0)
		return error;

	/* Supplies new.target without depending on an unimplemented Reflect API. */
	argument = vm_value_int32(2);
	error = vm_construct(
	    parent,
	    constructor,
	    &argument,
	    1,
	    target,
	    &object);
	if (error != 0)
		return error;

	/* Publishes the checked native value into its owning global for subsequent assertions. */
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "mixed",
	    object,
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Succeeded: script assertions can inspect the constructed receiver. */
	return 0;
}

/* Checks public VM APIs separately from bytecode dispatch. */
static int
probe_api(
	struct realm_probe *probe,
	struct vm_realm *parent,
	struct vm_realm *child)
{
	struct vm_function *native;
	vm_value answer;
	vm_value callback;
	vm_value key;
	vm_value saved_callee;
	vm_value saved_target;
	uint32_t line;
	uint32_t column;
	int error;
	int status;
	int site;

	/* Calls script code directly from the parent embedding. */
	key = vm_key_from_ascii(parent->heap, "read");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_get(parent, vm_value_cell(child->global), key, &callback);
	if (error != 0)
		return error;

	/* Runs the foreign callable through the public embedding boundary. */
	error = vm_call(parent, callback, VM_VALUE_UNDEFINED, NULL, 0, &answer);
	if (error != 0)
		return error;

	/* Publishes the checked native value into its owning global for subsequent assertions. */
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "apiResult",
	    answer,
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Records this independent named realm behavior before proceeding to later checks. */
	error = probe_check(probe, parent, "VM call global", "apiResult===22;");
	if (error != 0)
		return error;

	/* Checks a script throw's actual object and retained source position. */
	key = vm_key_from_ascii(parent->heap, "raise");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = vm_get(parent, vm_value_cell(child->global), key, &callback);
	if (error != 0)
		return error;

	/* Runs the foreign callable through the public embedding boundary. */
	status = vm_call(
	    parent,
	    callback,
	    VM_VALUE_UNDEFINED,
	    NULL,
	    0,
	    &answer);
	if (status != VM_THROWN)
		return EINVAL;
	site = vm_throw_site(parent, parent->exception, &line, &column);
	if (!site || line != 3)
		return EINVAL;
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "apiError",
	    parent->exception,
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;
	parent->exception = VM_VALUE_UNDEFINED;
	error = probe_check(
	    probe,
	    parent,
	    "VM script exception",
	    "apiError instanceof child.TypeError;");
	if (error != 0)
		return error;

	/* Calls a throwing native as both a function and a constructor. */
	native = vm_function_create_native(child, "throwing", 0, probe_throw);
	if (native == NULL)
		return ENOMEM;

	/* The checked function exposes the same native contract through construction. */
	native->construct = probe_throw;
	error = js_builtin_value(
	    child,
	    child->global,
	    "throwing",
	    vm_value_cell(native),
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Runs the foreign callable through the public embedding boundary. */
	status = vm_call(
	    parent,
	    vm_value_cell(native),
	    VM_VALUE_UNDEFINED,
	    NULL,
	    0,
	    &answer);
	if (status != VM_THROWN)
		return EINVAL;

	/* Publishes the checked native value into its owning global for subsequent assertions. */
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "apiError",
	    parent->exception,
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;
	parent->exception = VM_VALUE_UNDEFINED;
	error = probe_check(
	    probe,
	    parent,
	    "VM native exception",
	    "apiError instanceof child.TypeError;");
	if (error != 0)
		return error;

	/* Constructs through the public boundary while retaining the supplied new.target. */
	status = vm_construct(
	    parent,
	    vm_value_cell(native),
	    NULL,
	    0,
	    vm_value_cell(native),
	    &answer);
	if (status != VM_THROWN)
		return EINVAL;

	/* Publishes the checked native value into its owning global for subsequent assertions. */
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "apiError",
	    parent->exception,
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;
	parent->exception = VM_VALUE_UNDEFINED;
	error = probe_check(
	    probe,
	    parent,
	    "VM constructor exception",
	    "apiError instanceof child.TypeError;");
	if (error != 0)
		return error;

	/* Ensures ordinary allocation failures are not rewritten as script throws. */
	native = vm_function_create_native(child, "nomem", 0, probe_nomem);
	if (native == NULL)
		return ENOMEM;

	/* The checked function exposes the same native contract through construction. */
	native->construct = probe_nomem;
	saved_callee = child->callee;
	saved_target = child->new_target;
	status = vm_call(
	    parent,
	    vm_value_cell(native),
	    VM_VALUE_UNDEFINED,
	    NULL,
	    0,
	    &answer);
	if (status != ENOMEM)
		return EINVAL;

	/* Errno refusal must restore the suspended native state without a script throw. */
	if (child->callee != saved_callee || child->new_target != saved_target)
		return EINVAL;
	status = vm_construct(
	    parent,
	    vm_value_cell(native),
	    NULL,
	    0,
	    vm_value_cell(native),
	    &answer);
	if (status != ENOMEM)
		return EINVAL;

	/* Errno refusal must restore the suspended native state without a script throw. */
	if (child->callee != saved_callee || child->new_target != saved_target)
		return EINVAL;

	/* Records successful C unwinding checks as a separate named assertion. */
	error = probe_check(probe, parent, "native failure unwind", "true;");
	if (error != 0)
		return error;

	/* Confirms all foreign runs left both explicit VM stacks idle. */
	if (parent->stack_top != 0 || child->stack_top != 0)
		return EINVAL;

	/* Both realms must also release their active recursion count after unwinding. */
	if (parent->depth != 0 || child->depth != 0)
		return EINVAL;

	/* Succeeded: both stacks are reusable after success and failure. */
	return 0;
}

/* Invokes a foreign callback without losing this native's callee state. */
static int
probe_native_reentry(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value saved_callee;
	vm_value saved_target;
	vm_value ignored;
	int error;

	UNUSED_PARAMETER(this_value);

	/* Keeps state the nested native/script calls may temporarily replace. */
	if (count < 1)
		return EINVAL;

	/* Remember the exact outer state before any foreign callback can temporarily replace it. */
	saved_callee = realm->callee;
	saved_target = realm->new_target;
	error = vm_call(realm, args[0], VM_VALUE_UNDEFINED, NULL, 0, &ignored);
	if (error != 0)
		return error;

	/* Exercises collection with the outer native and its saved state live. */
	vm_heap_collect(realm->heap);

	/* Requires both native call fields to belong to the outer invocation. */
	*result = VM_VALUE_FALSE;
	if (realm->callee != saved_callee || realm->new_target != saved_target)
		return 0;

	/* Succeeded: nested calls restored the outer native state. */
	*result = VM_VALUE_TRUE;
	return 0;
}

/* Supplies a real native errno contract without a production fault switch. */
static int
probe_nomem(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(result);

	/* Succeeded: supplies the fixture's native allocation-refusal contract. */
	return ENOMEM;
}
