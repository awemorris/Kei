/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Tests actual DOM ownership helpers through ordinary script-created trees. */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Behavioral accounting spans the bounded script and determines the fixture exit status. */
static unsigned checks;
/* Independent failures remain visible instead of stopping at the first ordinary contract. */
static unsigned failures;

static int owner_install(struct vm_realm *realm);
static int owner_verify(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int owner_get(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int owner_listed(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int owner_member(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);
static int owner_element(struct vm_realm *realm, const vm_value *args, unsigned count, struct dom_element **out);
static int owner_script(struct vm_realm *realm);

/*
 * Runs the scoped ordinary-tree foundation without installing any production JS APIs.
 */
int
main(
	void)
{
	struct vm_heap *heap;
	struct vm_realm *realm;
	struct dom_document *document;
	struct bind_window *window;
	struct bind_host host;
	int status;
	int printed;

	/* The real heap uses its normal conservative stack during script construction. */
	status = vm_heap_create(&heap, 0);
	if (status != 0)
		return 2;
	vm_heap_set_stack_base(heap, __builtin_frame_address(0));
	status = vm_realm_create(heap, &realm);
	if (status != 0) {
		vm_heap_destroy(heap);
		return 2;
	}

	/* Built-in language support is installed before any binding or fixture callback. */
	status = js_install_builtins(realm);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* An ordinary Document exists before its manual binding owner is created. */
	document = dom_document_create(heap);
	if (document == NULL) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* The fixture has no asynchronous host resource or hidden engine configuration. */
	memset(&host, 0, sizeof(host));
	status = bind_window_create(realm, document, &host, &window);
	if (status != 0) {
		vm_realm_destroy(realm);
		vm_heap_destroy(heap);
		return 2;
	}

	/* Only this embedding exposes the actual C helpers to its bounded script. */
	status = owner_install(realm);
	if (status == 0)
		status = owner_script(realm);

	/* Manual embedding ownership is released independently of script success. */
	bind_window_destroy(window);
	vm_realm_destroy(realm);
	vm_heap_destroy(heap);
	if (status != 0) {
		printed = fprintf(stderr, "form owner fixture execution error: %d\n", status);
		if (printed < 0)
			return 2;
		return 2;
	}

	/* Behavioral failures determine a distinct result from construction or script errors. */
	printed = printf("form owner checks: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;
	if (failures != 0)
		return 1;

	/* Succeeded: ordinary live ownership used the actual production DOM C helpers. */
	return 0;
}

/* Registers independent fixture-only C entry points with immediate error propagation. */
static int
owner_install(
	struct vm_realm *realm)
{
	int status;

	/* Assertions account for every independent behavioral script observation. */
	status = js_builtin_method(realm, realm->global, "ownerVerify", 1, owner_verify);
	if (status != 0)
		return status;

	/* Ownership remains a direct query of the actual DOM Element. */
	status = js_builtin_method(realm, realm->global, "ownerOf", 1, owner_get);
	if (status != 0)
		return status;

	/* Category inspection calls production helpers without installing public DOM stubs. */
	status = js_builtin_method(realm, realm->global, "listedControl", 1, owner_listed);
	if (status != 0)
		return status;

	/* Controls membership remains distinct from the listed category for image inputs. */
	status = js_builtin_method(realm, realm->global, "controlMember", 1, owner_member);
	if (status != 0)
		return status;

	/* Succeeded: every fixture entry point is usable through ordinary script calls. */
	return 0;
}

/* Accounts for script observations while its descriptive labels remain in console output. */
static int
owner_verify(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(receiver);

	/* Only a true Boolean completion supplies acceptance for a named script observation. */
	checks++;
	if (count == 0 || args[0] != VM_VALUE_TRUE)
		failures++;
	*result = VM_VALUE_UNDEFINED;

	/* Succeeded: later independent observations may continue after this assertion. */
	return 0;
}

/* Reports the actual current owner wrapper rather than an expected synthetic association. */
static int
owner_get(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	struct dom_element *owner;
	int status;

	UNUSED_PARAMETER(receiver);

	/* Genuine DOM Elements alone enter the production C query. */
	status = owner_element(realm, args, count, &element);
	if (status != 0)
		return status;
	owner = dom_form_owner(element);
	if (owner == NULL) {
		*result = VM_VALUE_NULL;
		return 0;
	}

	/* The normal wrapper selects the actual owner Document's relevant realm. */
	status = bind_wrap(bind_window_of(realm), &owner->node, result);
	if (status != 0)
		return status;

	/* Succeeded: the observed owner is a genuine current DOM form. */
	return 0;
}

/* Exposes the production listed-category predicate only to this host fixture. */
static int
owner_listed(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	int listed;
	int status;

	UNUSED_PARAMETER(receiver);

	/* Element branding precedes the actual category query. */
	status = owner_element(realm, args, count, &element);
	if (status != 0)
		return status;
	listed = dom_form_listed(element);
	*result = vm_value_boolean(listed);

	/* Succeeded: script sees the actual built-in listed classification. */
	return 0;
}

/* Exposes the production controls-collection predicate without installing a DOM API stub. */
static int
owner_member(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_element *element;
	int member;
	int status;

	UNUSED_PARAMETER(receiver);

	/* Actual DOM type/content state supplies the collection-membership category. */
	status = owner_element(realm, args, count, &element);
	if (status != 0)
		return status;
	member = dom_form_control_member(element);
	*result = vm_value_boolean(member);

	/* Succeeded: script sees the current production member predicate. */
	return 0;
}

/* Rejects non-Elements without interpreting a synthetic object's properties. */
static int
owner_element(
	struct vm_realm *realm,
	const vm_value *args,
	unsigned count,
	struct dom_element **out)
{
	struct dom_node *node;
	int status;

	/* The fixture uses the same argument brand helper as production DOM methods. */
	status = bind_argument_node(realm, js_argument(args, count, 0), &node);
	if (status != 0)
		return status;
	if (node->type != DOM_ELEMENT) {
		status = vm_throw_type_error(realm, "Expected an actual Element.");
		return status;
	}

	/* Succeeded: the private C query receives the actual DOM element. */
	*out = (struct dom_element *)node;
	return 0;
}

/* Reads and executes bounded source through the normal parser and interpreter. */
static int
owner_script(
	struct vm_realm *realm)
{
	struct wb_buffer bytes;
	struct wb_units units;
	struct js_syntax_error syntax;
	vm_value answer;
	int status;

	/* Input ownership is released on every read or conversion outcome. */
	wb_buffer_init(&bytes);
	wb_units_init(&units);
	status = wb_file_read("plan/ws074/tests/form-owner.js", &bytes);
	if (status != 0) {
		wb_buffer_release(&bytes);
		wb_units_release(&units);
		return status;
	}

	/* Convert complete fixture bytes before releasing their input storage. */
	status = wb_utf8_to_units(bytes.data, bytes.length, &units);
	wb_buffer_release(&bytes);
	if (status != 0) {
		wb_units_release(&units);
		return status;
	}

	/* The script creates and mutates every test node through existing production bindings. */
	status = js_run_script(realm, units.data, units.length, 0, &answer, &syntax);
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the actual engine executed every bounded ownership scenario. */
	return 0;
}
