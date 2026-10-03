/* Probes bound function publication across a length getter collection. */
#include "js/js.h"
#include <stdio.h>
#include <string.h>
static const void *probe_stack;
static unsigned collected;
static int collect_bind(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct wb_units units;
	struct js_syntax_error syntax;
	struct vm_cell *found;
	vm_value result;
	const char *script;
	int status;

	/* Normal VM construction precedes a real collector callback. */
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
	status = js_builtin_method(realm, realm->global, "collectBind", 0, collect_bind);
	if (status != 0)
		return 2;

	/* The bound output is native-only when its target's length getter runs. */
	script = "function target(x){return x;}Object.defineProperty(target,'length',{get(){collectBind();return 1;},configurable:true});target.bind(null)";
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)script, strlen(script), &units);
	if (status != 0)
		return 2;
	status = js_run_script(realm, units.data, units.length, 0, &result, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return 2;
	found = vm_heap_find_cell(heap, (uintptr_t)vm_value_as_cell(result));
	printf("bound output live=%d actual_collections=%u\n", found != NULL, collected);
	if (found == NULL || collected != 1)
		return 1;
	return 0;
}
static int
collect_bind(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Read only registered VM roots during this production collection. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, probe_stack);
	collected++;
	*result = VM_VALUE_UNDEFINED;
	return 0;
}
