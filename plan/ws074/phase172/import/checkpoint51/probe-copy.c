/* Probes object spread during an actual getter-triggered collection. */
#include "js/js.h"
#include <stdio.h>
#include <string.h>
static const void *probe_stack;
static unsigned collected;
static int collect_copy(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct vm_object *target;
	struct vm_cell *found;
	struct wb_units units;
	struct js_syntax_error syntax;
	vm_value source;
	vm_value value;
	const char *script;
	int status;

	/* Construct ordinary production builtins with a real collector. */
	probe_stack = __builtin_frame_address(0);
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, probe_stack);
	status = vm_realm_create(heap, &realm);
	if (status != 0)
		return 2;
	status = js_install_builtins(realm);
	if (status != 0)
		return 2;
	status = js_builtin_method(realm, realm->global, "collectCopy", 0, collect_copy);
	if (status != 0)
		return 2;

	/* User getter collects while destination and key array are native-only. */
	script = "var source={get x(){collectCopy();return 42;}};source";
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)script, strlen(script), &units);
	if (status != 0)
		return 2;
	status = js_run_script(realm, units.data, units.length, 0, &source, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return 2;
	target = vm_object_create(heap, realm->object_prototype);
	if (target == NULL)
		return 2;
	status = vm_copy_data_properties(realm, vm_value_cell(target), source);
	if (status != 0)
		return 2;
	found = vm_heap_find_cell(heap, (uintptr_t)&target->cell);
	if (found == NULL)
		return 1;
	status = vm_get(realm, vm_value_cell(target), vm_key_from_ascii(heap, "x"), &value);
	if (status != 0)
		return 2;
	printf("copy output live=%d value=%d actual_collections=%u\n", found != NULL, vm_value_as_int32(value), collected);
	if (collected != 1 || !vm_value_is_int32(value) || vm_value_as_int32(value) != 42)
		return 1;
	return 0;
}
static int
collect_copy(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Exclude conservative rescue during this actual collector run. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, probe_stack);
	collected++;
	*result = vm_value_int32(42);
	return 0;
}
