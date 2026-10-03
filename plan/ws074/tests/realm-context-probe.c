/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Reproduces the shared-heap realm prerequisites discovered by q510.
 * This investigation probe deliberately fails on the recorded baseline.
 * It is invoked explicitly, outside the host-* passing regression suite.
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
		printed = fprintf(stderr, "realm probe infrastructure error: %d\n", error);
		if (printed < 0)
			return 2;
		return 2;
	}

	/* Successful execution ends before either independent realm is dismantled. */
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

	/* Recorded semantic failures remain reproducible process failures. */
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

/* Runs controls and cross-realm calls while the caller owns both realms. */
static int
probe_run(
	struct realm_probe *probe,
	struct vm_realm *parent,
	struct vm_realm *child)
{
	struct vm_function *native;
	vm_value answer;
	int error;

	/* Installs the parent's globals and built-ins. */
	error = probe_script(parent, "var marker = 11;", &answer);
	if (error != 0)
		return error;

	/* Gives the child a different global value and its own functions. */
	error = probe_script(
	    child,
	    "var marker = 22; function read() { return marker; } "
	    "function raise() { throw new TypeError('child'); } "
	    "Object.defineProperty(this, 'answer', "
	    "{get: function() { return marker; }});",
	    &answer);
	if (error != 0)
		return error;

	/* Exposes an exception from a native owned by the child realm. */
	native = vm_function_create_native(child, "fail", 0, probe_throw);
	if (native == NULL) {
		error = ENOMEM;
		return error;
	}

	/* Publishes the native before allowing the parent to reach the child. */
	error = js_builtin_value(
	    child,
	    child->global,
	    "fail",
	    vm_value_cell(native),
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Publish the actual child global on the independently initialized parent. */
	error = js_builtin_value(
	    parent,
	    parent->global,
	    "child",
	    vm_value_cell(child->global),
	    VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Checks ordinary execution before testing calls across the boundary. */
	error = probe_check(probe, parent, "parent control", "marker === 11;");
	if (error != 0)
		return error;

	/* Check the independent child control before transporting foreign calls. */
	error = probe_check(probe, child, "child control", "marker === 22;");
	if (error != 0)
		return error;

	/* Checks that bytecode calls use the callee's global environment. */
	error = probe_check(
	    probe,
	    parent,
	    "child bytecode global",
	    "child.read() === 22;");
	if (error != 0)
		return error;

	/* Check a constructor-created function against the actual child global. */
	error = probe_check(
	    probe,
	    parent,
	    "child Function global",
	    "new child.Function('return marker')() === 22;");
	if (error != 0)
		return error;

	/* Compares the accessor's VM API path with direct bytecode calls. */
	error = probe_check(
	    probe,
	    parent,
	    "child getter global",
	    "child.answer === 22;");
	if (error != 0)
		return error;

	/* Requires the parent's catch to receive the actual child exception. */
	error = probe_check(
	    probe,
	    parent,
	    "child native exception",
	    "var caught = false; try { child.fail(); } catch (e) { "
	    "caught = typeof e === 'object' && "
	    "e instanceof child.TypeError; } caught;");
	if (error != 0)
		return error;

	/* Observe the actual child SyntaxError thrown by dynamic construction. */
	error = probe_check(
	    probe,
	    parent,
	    "child constructor exception",
	    "caught = false; try { new child.Function('return )'); } catch (e) { "
	    "caught = typeof e === 'object' && "
	    "e instanceof child.SyntaxError; } caught;");
	if (error != 0)
		return error;

	/* Observe the actual child TypeError transported from bytecode. */
	error = probe_check(
	    probe,
	    parent,
	    "child bytecode exception identity",
	    "caught = false; try { child.raise(); } catch (e) { "
	    "caught = typeof e === 'object' && "
	    "e instanceof child.TypeError; } caught;");
	if (error != 0)
		return error;

	/* Succeeded: the bounded inventory is recorded, including its failures. */
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

	/* Run the original source before checking its actual execution outcome. */
	error = js_run_script(realm, units.data, units.length, 0, answer, &syntax);
	if (error != 0) {
		wb_units_release(&units);
		return error;
	}

	/* Completed execution no longer borrows converted probe source. */
	wb_units_release(&units);

	/* Succeeded: the last expression's value is available to the assertion. */
	return 0;
}

/* Reports one Boolean assertion, retaining the whole investigation inventory. */
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
