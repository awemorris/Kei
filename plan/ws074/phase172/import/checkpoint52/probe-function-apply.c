/* Probes arguments retained while a later indexed getter invokes real GC. */
#include "js/js.h"
#include <stdio.h>
#include <string.h>
static const void *probe_stack;
static unsigned collected;
static int collect_args(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct wb_units units;
	struct js_syntax_error syntax;
	vm_value list;
	vm_value result;
	const char *script;
	int status;

	/* Build real VM structures before forcing collector-only ownership. */
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
	status = js_builtin_method(realm, realm->global, "collectArgs", 0, collect_args);
	if (status != 0)
		return 2;

	/* The first getter creates a temporary object that the second getter can collect. */
	script = "function f(a,b){return a.value+b;}var list=[];Object.defineProperty(list,'0',{get(){return {value:40};}});Object.defineProperty(list,'1',{get(){collectArgs();return 2;}});list.length=2;f.apply(undefined,list)";
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)script, strlen(script), &units);
	if (status != 0)
		return 2;
	status = js_run_script(realm, units.data, units.length, 0, &list, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return 2;
	result = list;
	printf("apply result=%g is_number=%d actual_collections=%u\n", vm_value_as_number(result), vm_value_is_number(result), collected);
	if (collected != 1 || !vm_value_is_number(result) || vm_value_as_number(result) != 42.0)
		return 1;
	return 0;
}
static int
collect_args(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Deny conservative C stack scanning for this production collection. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, probe_stack);
	collected++;
	*result = VM_VALUE_UNDEFINED;
	return 0;
}
