/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercises native dynamic own-property policy through the production VM and GC. */

#include "js/js.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* A collectible synthetic native owner exposes changing views of an ordinary payload. */
struct native_state {
	struct vm_cell cell;
	vm_value payload;
	vm_value accessor;
	unsigned count;
	int lookup_error;
	int keys_error;
	int define_error;
	int delete_error;
	int extensions_error;
};

/* Assertion totals cover script semantics, C error boundaries and real collection. */
static unsigned checks;
/* Failure accounting stays available after individual independent observations. */
static unsigned failures;

static void native_trace(struct vm_heap *heap, struct vm_cell *cell);
static int native_own(struct vm_object *object, vm_value key, struct vm_property *property);
static int native_keys(struct vm_heap *heap, struct vm_object *object, struct wb_vector *keys);
static int native_define(struct vm_realm *realm, struct vm_object *object, vm_value key, const struct vm_descriptor *descriptor, int *handled, int *done);
static int native_delete(struct vm_heap *heap, struct vm_object *object, vm_value key, int *handled, int *deleted);
static int native_extensions(struct vm_object *object, int *allowed);

/* The owner traces its payload while the ordinary object traces internal native state. */
static const struct vm_cell_type native_type = { "native-property-test", native_trace, NULL };

/* This is an ordinary registered C property policy, never a hidden engine test mode. */
static const struct vm_native_operations native_operations = {
	native_own, native_keys, native_define, native_delete, native_extensions
};

static void native_check(int condition, const char *name);
static int native_script(struct vm_realm *realm, const char *source, vm_value *answer);
static void native_expect(struct vm_realm *realm, const char *source, const char *name, int expected_error);
static int native_case(struct vm_heap *heap);

/*
 * Verifies changing native properties and error propagation through real VM entry points.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	int error;
	int printed;

	/* Normal conservative stack ownership is used except at explicit GC assertions. */
	error = vm_heap_create(&heap, 0);
	if (error != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	error = native_case(heap);
	if (error != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Complete the actual native protocol inventory before successful heap finalization. */
	vm_heap_destroy(heap);

	/* All native protocol observations contribute to the process status. */
	printed = printf("native property checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* A failing dynamic property or ownership contract invalidates the fixture. */
	if (failures != 0)
		return 1;

	/* Succeeded: native policies and ordinary VM boundaries behaved as observed. */
	return 0;
}

/* Records independent contracts without hiding later observations. */
static void
native_check(
	int condition,
	const char *name)
{
	int printed;

	/* Named failures remain visible while the fixture completes its bounded cases. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: the observation was recorded. */
	return;
}

/* Traces the original native payload through the default production collector. */
static void
native_trace(
	struct vm_heap *heap,
	struct vm_cell *cell)
{
	struct native_state *state;

	/* Native property descriptors may expose this payload after a later lookup. */
	state = (struct native_state *)cell;
	vm_heap_mark_word(heap, state->payload);
	vm_heap_mark_word(heap, state->accessor);

	/* Succeeded: both actual native edges participated in this collector visit. */
	return;
}

/* Exposes virtual indices and one readonly named value without storing property slots. */
static int
native_own(
	struct vm_object *object,
	vm_value key,
	struct vm_property *property)
{
	struct native_state *state;
	uint32_t index;
	int indexed;
	int named;
	int string;
	int accessor;

	/* The supported callback error outcome is distinct from absence and presence. */
	state = (struct native_state *)vm_value_as_cell(object->internal);
	if (state->lookup_error != 0)
		return -state->lookup_error;

	/* Dynamic indices reflect the current native count on every lookup. */
	indexed = vm_value_is_array_index(key, &index);
	if (indexed && index < state->count) {
		property->holder = object;
		property->temporary = vm_value_int32((int32_t)index + 10);
		property->value = &property->temporary;
		property->attributes = VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE;
		return 1;
	}

	/* Named values retain the original collectible payload through internal state. */
	string = vm_value_is_string(key);
	named = 0;
	if (string)
		named = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(key), "named");
	if (named && state->count != 0) {
		property->holder = object;
		property->temporary = state->payload;
		property->value = &property->temporary;
		property->attributes = VM_PROPERTY_CONFIGURABLE;
		return 1;
	}

	/* Virtual accessors use a separately traced pair with the usual receiver dispatch. */
	accessor = 0;
	if (string)
		accessor = vm_string_equal_ascii((struct vm_string *)vm_value_as_cell(key), "virtualGetter");
	if (accessor && state->accessor != VM_VALUE_UNDEFINED) {
		property->holder = object;
		property->temporary = state->accessor;
		property->value = &property->temporary;
		property->attributes = VM_PROPERTY_ACCESSOR | VM_PROPERTY_CONFIGURABLE;
		return 1;
	}

	/* Succeeded: this key has no current virtual property; ordinary storage may supply it. */
	return 0;
}

/* Supplies policy order, including a duplicate ordinary name to test safe key merging. */
static int
native_keys(
	struct vm_heap *heap,
	struct vm_object *object,
	struct wb_vector *keys)
{
	struct native_state *state;
	vm_value key;
	unsigned index;
	int error;

	/* Failed native key enumeration cannot silently become an ordinary key list. */
	state = (struct native_state *)vm_value_as_cell(object->internal);
	if (state->keys_error != 0)
		return state->keys_error;

	/* Current virtual numeric keys precede current virtual names and ordinary expandos. */
	for (index = 0; index < state->count; index++) {
		key = vm_value_int32((int32_t)index);
		error = wb_vector_push(keys, &key);
		if (error != 0)
			return error;
	}

	/* The disappearing named property is absent whenever this view is empty. */
	if (state->count != 0) {
		key = vm_key_from_ascii(heap, "named");
		if (key == VM_VALUE_EMPTY)
			return ENOMEM;
		error = wb_vector_push(keys, &key);
		if (error != 0)
			return error;
	}

	/* An enabled virtual accessor contributes its own nonenumerable key before expandos. */
	if (state->accessor != VM_VALUE_UNDEFINED) {
		key = vm_key_from_ascii(heap, "virtualGetter");
		if (key == VM_VALUE_EMPTY)
			return ENOMEM;
		error = wb_vector_push(keys, &key);
		if (error != 0)
			return error;
	}

	/* Ordinary key overlap must retain only its first policy-ordered occurrence. */
	key = vm_key_from_ascii(heap, "expando");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	error = wb_vector_push(keys, &key);
	if (error != 0)
		return error;

	/* Succeeded: the native prefix is appended without persisting fake properties. */
	return 0;
}

/* Refuses any numeric definition while leaving ordinary nonnumeric expandos available. */
static int
native_define(
	struct vm_realm *realm,
	struct vm_object *object,
	vm_value key,
	const struct vm_descriptor *descriptor,
	int *handled,
	int *done)
{
	struct native_state *state;
	uint32_t index;
	int indexed;

	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(descriptor);

	/* A callback failure precedes any definition or fallback storage change. */
	state = (struct native_state *)vm_value_as_cell(object->internal);
	if (state->define_error != 0)
		return state->define_error;

	/* Supported and unsupported numeric keys share this native readonly policy. */
	indexed = vm_value_is_array_index(key, &index);
	*handled = 0;
	*done = 0;
	if (indexed)
		*handled = 1;

	/* Succeeded: native numeric definitions refuse, and ordinary names may fall through. */
	return 0;
}

/* Refuses deleting current virtual properties despite their configurable descriptors. */
static int
native_delete(
	struct vm_heap *heap,
	struct vm_object *object,
	vm_value key,
	int *handled,
	int *deleted)
{
	struct native_state *state;
	struct vm_property property;
	int found;

	UNUSED_PARAMETER(heap);

	/* Deletion errors must not mutate ordinary storage or claim success. */
	state = (struct native_state *)vm_value_as_cell(object->internal);
	if (state->delete_error != 0)
		return state->delete_error;

	/* Calling the separate policy helper avoids recursive VM dispatch on this object. */
	found = native_own(object, key, &property);
	if (found < 0)
		return -found;
	*handled = found;
	*deleted = 0;

	/* Succeeded: current virtual properties stay, while ordinary keys may fall through. */
	return 0;
}

/* Keeps dynamic views extensible and preserves a distinct native refusal outcome. */
static int
native_extensions(
	struct vm_object *object,
	int *allowed)
{
	struct native_state *state;

	/* Native errors and ordinary refusal do not alter flags or descriptors. */
	state = (struct native_state *)vm_value_as_cell(object->internal);
	if (state->extensions_error != 0)
		return state->extensions_error;
	*allowed = 0;

	/* Succeeded: this dynamic native view refuses preventing extensions. */
	return 0;
}

/* Executes fixture source through the ordinary production parser and interpreter. */
static int
native_script(
	struct vm_realm *realm,
	const char *source,
	vm_value *answer)
{
	struct wb_units units;
	struct js_syntax_error syntax;
	int error;

	/* Input conversion owns its temporary buffer until script execution completes. */
	wb_units_init(&units);
	error = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Script semantic operations dispatch the same native hooks as production objects. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Completed native execution no longer borrows converted fixture source. */
	wb_units_release(&units);

	/* Succeeded: the completion value is available. */
	return 0;
}

/* Requires true for normal contracts and the exact positive errno for failure contracts. */
static void
native_expect(
	struct vm_realm *realm,
	const char *source,
	const char *name,
	int expected_error)
{
	vm_value answer;
	int error;
	int same;

	/* Expected native errors are observed at the host boundary rather than hidden as properties. */
	answer = VM_VALUE_UNDEFINED;
	error = native_script(realm, source, &answer);
	same = 0;
	if (error == expected_error) {
		if (expected_error != 0 || answer == VM_VALUE_TRUE)
			same = 1;
	}

	/* Every contract reports its exact completion/error without changing the engine. */
	native_check(same, name);

	/* Succeeded: the observation was recorded even if it invalidates the final result. */
	return;
}

/* Builds one ordinary realm and collectible native object for bounded protocol checks. */
static int
native_case(
	struct vm_heap *heap)
{
	struct vm_realm *realm;
	struct vm_object *object;
	struct vm_object *payload;
	struct native_state *state;
	struct vm_property property;
	struct vm_cell *found_cell;
	vm_value answer;
	vm_value key;
	uintptr_t state_address;
	uintptr_t payload_address;
	int same;
	int error;
	int found;

	/* This realm installs the actual standard library before the native fixture is exposed. */
	error = vm_realm_create(heap, &realm);
	if (error != 0)
		return error;
	error = js_install_builtins(realm);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The private owner exists before either its wrapper or payload is allocated. */
	state = vm_heap_alloc(heap, &native_type, sizeof(*state));
	if (state == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Initialize traceable native values before the next actual heap allocation. */
	state->payload = VM_VALUE_UNDEFINED;
	state->accessor = VM_VALUE_UNDEFINED;
	state->count = 2;

	/* A genuine ordinary object is the named virtual property's collectible payload. */
	payload = vm_object_create(heap, realm->object_prototype);
	if (payload == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Native owner state supplies all changing data without a raw callback context pointer. */
	state->payload = vm_value_cell(payload);
	object = vm_object_create(heap, realm->object_prototype);
	if (object == NULL) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* Ordinary setup finishes before native dynamic policy becomes active. */
	error = js_builtin_value(realm, object, "expando", vm_value_int32(23), VM_PROPERTY_DEFAULT);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* The wrapper traces its internal owner, which separately traces its payload. */
	object->kind = VM_KIND_PLATFORM;
	object->internal = vm_value_cell(state);
	object->native_operations = &native_operations;
	error = js_builtin_value(realm, realm->global, "nativeView", vm_value_cell(object), VM_PROPERTY_DEFAULT);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Individual semantics and C boundary cases are recorded below. */
	native_expect(realm, "nativeView[0]===10&&nativeView['1']===11&&nativeView[2]===undefined", "virtual live indices", 0);
	native_expect(realm, "'0' in nativeView&&'named' in nativeView&& !('2' in nativeView)", "in operator native presence", 0);
	native_expect(realm, "nativeView.hasOwnProperty('0')&&Object.hasOwn(nativeView,'named')&&!Object.hasOwn(nativeView,'2')", "hasOwn native presence", 0);
	native_expect(realm, "(function(){var d=Object.getOwnPropertyDescriptor(nativeView,'0');return d.value===10&&!d.writable&&d.enumerable&&d.configurable&&d.get===undefined;})()", "virtual data descriptor flags", 0);
	native_expect(realm, "Object.keys(nativeView).join(',')==='0,1,expando'", "enumerable keys and merged deduplication", 0);
	native_expect(realm, "Object.getOwnPropertyNames(nativeView).join(',')==='0,1,named,expando'", "native prefix order and nonenumerable names", 0);
	native_expect(realm, "(function(){var a=[];for(var k in nativeView)a.push(k);return a.join(',')==='0,1,expando';})()", "for-in dynamic descriptors", 0);
	native_expect(realm, "Object.getOwnPropertyDescriptors(nativeView)['1'].value===11", "descriptor enumeration native values", 0);
	native_expect(realm, "(function(){var a=Object.assign({},nativeView);return a[0]===10&&a[1]===11&&a.expando===23&&a.named===undefined;})()", "Object.assign native enumerable view", 0);
	native_expect(realm, "(function(){var a={...nativeView};return a[0]===10&&a[1]===11&&a.named===undefined;})()", "spread native enumerable view", 0);
	native_expect(realm, "JSON.stringify(nativeView)==='{\\\"0\\\":10,\\\"1\\\":11,\\\"expando\\\":23}'", "JSON native view", 0);
	native_expect(realm, "(function(){var a=Object.create(nativeView);return a[0]===10&&'named' in a&&!a.hasOwnProperty('0');})()", "prototype chain virtual lookup", 0);
	native_expect(realm, "(function(){nativeView[0]=99;return nativeView[0]===10;})()", "readonly assignment preserves virtual data", 0);
	native_expect(realm, "(function(){try{(function(){'use strict';nativeView[0]=99;})();return false;}catch(e){return e.name==='TypeError';}})()", "strict readonly assignment throws", 0);
	native_expect(realm, "(function(){nativeView[99]=99;return nativeView[99]===undefined&&!Object.hasOwn(nativeView,'99');})()", "native definition guards unsupported index", 0);
	native_expect(realm, "(function(){try{Object.defineProperty(nativeView,'0',{value:99});return false;}catch(e){return e.name==='TypeError';}})()", "native descriptor definition refuses", 0);
	native_expect(realm, "delete nativeView[0]===false&&delete nativeView[99]===true", "native deletion policy independent from configurable descriptor", 0);
	native_expect(realm, "(function(){nativeView.extra=37;var ok=nativeView.extra===37;delete nativeView.extra;return ok&&nativeView.extra===undefined;})()", "ordinary expando set and delete fall through", 0);
	native_expect(realm, "(function(){try{Object.preventExtensions(nativeView);return false;}catch(e){return e.name==='TypeError'&&Object.isExtensible(nativeView);}})()", "native preventExtensions refusal preserves extensibility", 0);
	native_expect(realm, "(function(){try{Object.freeze(nativeView);return false;}catch(e){return e.name==='TypeError'&&Object.isExtensible(nativeView)&&Object.getOwnPropertyDescriptor(nativeView,'0').configurable;}})()", "freeze refusal preserves descriptors", 0);
	native_expect(realm, "(function(){try{Object.seal(nativeView);return false;}catch(e){return e.name==='TypeError'&&Object.isExtensible(nativeView);}})()", "seal refusal preserves extensibility", 0);
	native_expect(realm, "!Object.isFrozen(nativeView)&&!Object.isSealed(nativeView)", "native integrity remains extensible", 0);
	native_expect(realm, "(function(){var o={a:1};Object.freeze(o);return Object.isFrozen(o)&&!Object.isExtensible(o)&&o.a===1;})()", "ordinary object integrity unchanged", 0);

	/* Ordinary accessors on a native object still receive the original semantic receiver. */
	native_expect(realm, "(function(){Object.defineProperty(nativeView,'ordinaryGetter',{get:function(){return this[1];},configurable:true});var ok=nativeView.ordinaryGetter===11;delete nativeView.ordinaryGetter;return ok;})()", "ordinary accessor dispatch on native object", 0);

	/* Changing owner data updates indices and names without another factory or getter. */
	state->count = 0;
	native_expect(realm, "nativeView[0]===undefined&&nativeView.named===undefined&&!('0' in nativeView)", "empty dynamic view drops indices and names", 0);
	native_expect(realm, "Object.keys(nativeView).join(',')==='expando'", "empty dynamic view enumeration", 0);
	state->count = 3;
	native_expect(realm, "nativeView[2]===12&&Object.keys(nativeView).join(',')==='0,1,2,expando'", "expanded dynamic view immediately visible", 0);

	/* The ordinary bypass cannot confuse virtual values with stored private or brand data. */
	found = vm_object_get_own_ordinary(object, vm_value_int32(0), &property);
	same = 0;
	if (found == 0)
		same = 1;
	native_check(same, "ordinary bypass sees no fake virtual property slots");

	/* A negative lookup outcome must survive every semantic reader and cleanup path. */
	state->lookup_error = ENOMEM;
	found = vm_object_get_own(object, vm_value_int32(0), &property);
	same = 0;
	if (found == -ENOMEM)
		same = 1;
	native_check(same, "C own lookup keeps negative errno distinct from found");
	error = vm_object_get(object, vm_value_int32(0), &answer);
	same = 0;
	if (error == ENOMEM)
		same = 1;
	native_check(same, "C value lookup converts negative presence error into status");

	/* Script readers propagate the same native lookup failure across their descriptor boundaries. */
	native_expect(realm, "nativeView[0]", "get propagates native lookup error", ENOMEM);
	native_expect(realm, "'0' in nativeView", "in propagates native lookup error", ENOMEM);
	native_expect(realm, "nativeView.hasOwnProperty('0')", "method lookup propagates native error", ENOMEM);
	native_expect(realm, "Object.hasOwn(nativeView,'0')", "hasOwn propagates native lookup error", ENOMEM);
	native_expect(realm, "Object.prototype.hasOwnProperty.call(nativeView,'0')", "prototype hasOwn propagates native lookup error", ENOMEM);
	native_expect(realm, "Object.getOwnPropertyDescriptor(nativeView,'0')", "descriptor propagates native lookup error", ENOMEM);
	native_expect(realm, "Object.keys(nativeView)", "keys descriptors propagate native lookup error", ENOMEM);
	native_expect(realm, "Object.values(nativeView)", "values propagate native lookup error", ENOMEM);
	native_expect(realm, "Object.getOwnPropertyDescriptors(nativeView)", "descriptor enumeration cleanup propagates error", ENOMEM);
	native_expect(realm, "Object.assign({},nativeView)", "assign cleanup propagates lookup error", ENOMEM);
	native_expect(realm, "({...nativeView})", "spread propagates lookup error", ENOMEM);
	native_expect(realm, "JSON.stringify(nativeView)", "JSON cleanup propagates lookup error", ENOMEM);
	native_expect(realm, "(function(){for(var k in nativeView){}return true;})()", "for-in propagates lookup error", ENOMEM);
	native_expect(realm, "Object.prototype.propertyIsEnumerable.call(nativeView,'0')", "propertyIsEnumerable propagates native error", ENOMEM);
	native_expect(realm, "nativeView.expando=91", "assignment propagates lookup error", ENOMEM);

	/* Inherited lookup and JSON reviver-created native edges preserve the same failure. */
	native_expect(realm, "Object.create(nativeView)[0]", "native lookup error survives prototype chain", ENOMEM);
	native_expect(realm, "JSON.parse('{\"a\":1,\"b\":2}',function(k,v){if(k==='a')this.b=nativeView;return v;})", "JSON reviver cleanup propagates native lookup failure", ENOMEM);

	/* Integrity readers must not swallow a virtual descriptor failure as a false boolean. */
	object->flags |= VM_OBJECT_NOT_EXTENSIBLE;
	native_expect(realm, "Object.isFrozen(nativeView)", "integrity inspection propagates native descriptor error", ENOMEM);
	native_expect(realm, "Object.isSealed(nativeView)", "seal inspection propagates native descriptor error", ENOMEM);
	object->flags &= ~VM_OBJECT_NOT_EXTENSIBLE;

	/* Restored normal operation proves prior native errors leave no poisoned VM state. */
	state->lookup_error = 0;
	native_expect(realm, "nativeView[0]===10&&nativeView.expando===23", "lookup error recovery", 0);
	state->keys_error = EOVERFLOW;
	native_expect(realm, "Object.getOwnPropertyNames(nativeView)", "native keys error survives enumeration", EOVERFLOW);
	native_expect(realm, "JSON.stringify(nativeView)", "native keys error survives JSON cleanup", EOVERFLOW);
	state->keys_error = 0;
	state->define_error = EBUSY;
	native_expect(realm, "Object.defineProperty(nativeView,'extra',{value:5})", "native define error survives descriptor boundary", EBUSY);
	native_expect(realm, "nativeView.extra=5", "native define error survives assignment boundary", EBUSY);
	state->define_error = 0;
	state->delete_error = EACCES;
	native_expect(realm, "delete nativeView.expando", "native delete error survives operation boundary", EACCES);
	state->delete_error = 0;
	state->extensions_error = EBUSY;
	native_expect(realm, "Object.preventExtensions(nativeView)", "native extension error propagates", EBUSY);
	native_expect(realm, "Object.freeze(nativeView)", "native extension error propagates through freeze", EBUSY);
	state->extensions_error = 0;

	/* A stored accessor pair becomes virtual while its source property is removed. */
	error = native_script(realm, "Object.defineProperty(nativeView,'sourceGetter',{get:function(){return this[1];},configurable:true});true", &answer);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Native state owns the pair before ordinary deletion removes its original slot. */
	key = vm_key_from_ascii(heap, "sourceGetter");
	if (key == VM_VALUE_EMPTY) {
		vm_realm_destroy(realm);
		return ENOMEM;
	}

	/* The ordinary source property must exist before its accessor pair can be retained. */
	found = vm_object_get_own_ordinary(object, key, &property);
	if (found < 0) {
		vm_realm_destroy(realm);
		return -found;
	}

	/* A present native source accessor must expose its complete ordinary value slot. */
	if (found == 0 || property.value == NULL) {
		vm_realm_destroy(realm);
		return EINVAL;
	}

	/* Virtual publication never points at the former stored slot's changing memory. */
	state->accessor = *property.value;
	native_expect(realm, "delete nativeView.sourceGetter&&nativeView.virtualGetter===11", "virtual accessor calls with original receiver", 0);
	native_expect(realm, "typeof Object.getOwnPropertyDescriptor(nativeView,'virtualGetter').get==='function'", "virtual accessor descriptor preserves getter identity", 0);
	native_expect(realm, "Object.create(nativeView).virtualGetter===11", "inherited virtual accessor uses derived receiver", 0);

	/* The wrapper's traced internal value retains both owner and payload without C stack scanning. */
	state_address = (uintptr_t)state;
	payload_address = (uintptr_t)payload;
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found_cell = vm_heap_find_cell(heap, state_address);
	same = 0;
	if (found_cell != NULL)
		same = 1;
	native_check(same, "reachable native object traces its private dynamic owner");
	found_cell = vm_heap_find_cell(heap, payload_address);
	same = 0;
	if (found_cell != NULL)
		same = 1;
	native_check(same, "native owner traces virtual named payload during actual GC");
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	native_expect(realm, "typeof nativeView.named==='object'&&nativeView[2]===12&&nativeView.virtualGetter===11", "dynamic values remain usable after GC", 0);

	/* Dropping the only object root permits the entire native graph to be reclaimed. */
	error = native_script(realm, "nativeView=null;true", &answer);
	if (error != 0) {
		vm_realm_destroy(realm);
		return error;
	}

	/* Explicit collection excludes test locals rather than artificially keeping native owners alive. */
	vm_heap_set_stack_base(heap, NULL);
	vm_heap_collect(heap);
	found_cell = vm_heap_find_cell(heap, state_address);
	same = 0;
	if (found_cell == NULL)
		same = 1;
	native_check(same, "last native object root drop reclaims private owner");
	found_cell = vm_heap_find_cell(heap, payload_address);
	same = 0;
	if (found_cell == NULL)
		same = 1;
	native_check(same, "last native object root drop reclaims virtual payload");
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	vm_realm_destroy(realm);

	/* Succeeded: every bounded native property and real collection observation finished. */
	return 0;
}
