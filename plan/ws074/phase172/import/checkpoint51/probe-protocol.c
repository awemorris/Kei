/* Tests actual unpublished rest output ownership during a real native getter collection. */
#include "js/js.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
static const void *probe_stack;
static unsigned collected;
static int collect_rest(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct wb_units units;
	struct js_syntax_error syntax;
	struct vm_cell *held;
	struct vm_cell *found;
	vm_value input;
	vm_value iterator;
	vm_value answer;
	const char *source;
	int status;
	int printed;

	/* The ordinary manual realm supplies real builtins without a synthetic GC policy. */
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
	status = js_builtin_method(realm, realm->global, "collectRest", 0, collect_rest);
	if (status != 0)
		return 2;

	/* A real array indexed getter requests collection while the private rest array is unpublished. */
	source = "var input={ [Symbol.iterator](){ return { n:0, next(){ var ended=this.n++>0; return { get done(){collectRest();return ended;},get value(){collectRest();return 42;} }; } }; } }; input";
	wb_units_init(&units);
	status = wb_utf8_to_units((const unsigned char *)source, strlen(source), &units);
	if (status != 0)
		return 2;
	status = js_run_script(realm, units.data, units.length, 0, &input, &syntax);
	if (status != 0)
		return 2;
	wb_units_release(&units);
	status = vm_iter_start(realm, input, &iterator);
	if (status != 0)
		return 2;

	/* The caller owns only the completed input iterator, never the unpublished callee output. */
	held = vm_value_as_cell(iterator);
	status = vm_heap_add_root(heap, &held);
	if (status != 0)
		return 2;
	status = vm_iter_rest(realm, iterator, &answer);
	if (status != 0)
		return 2;
	found = vm_heap_find_cell(heap, (uintptr_t)vm_value_as_cell(answer));
	printed = printf("protocol rest output live=%d actual_collections=%u\n", found != NULL, collected);
	if (printed < 0)
		return 2;

	/* Missing actual output allocation is a reproducible ownership failure. */
	if (found == NULL || collected < 3)
		return 1;

	/* Succeeded: the unpublished native rest output survived its getter collection. */
	return 0;
}
static int
collect_rest(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(receiver);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Exclude all conservative C-stack rescue while using the unchanged production collector. */
	vm_heap_set_stack_base(realm->heap, NULL);
	vm_heap_collect(realm->heap);
	vm_heap_set_stack_base(realm->heap, probe_stack);
	collected++;
	*result = vm_value_int32(42);

	/* Succeeded: the ordinary array getter publishes its original numeric value. */
	return 0;
}
