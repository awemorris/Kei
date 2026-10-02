/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The timers of the window: setTimeout, setInterval, their clear
 * functions, and requestAnimationFrame as a timer one frame away.
 *
 * The window does not read a clock: the page sets its time and asks it to
 * run what is due, so the headless modes can run a page's timers on a
 * virtual clock and the window on the real one.  A round of the timers
 * runs only the ones made before the round began, so a timer that sets a
 * timer of no delay cannot keep the round going for ever.
 */

#include "bind/internal.h"

#include <errno.h>
#include <string.h>

/* The time between two animation frames, in milliseconds (60 frames a second). */
#define TIMER_FRAME		16.0

/* The shortest period of an interval, in milliseconds. */
#define TIMER_MIN_INTERVAL	1.0

/* The most arguments a timer passes on to its callback. */
#define TIMER_ARGUMENTS_MAX	16U

static int timer_add(struct vm_realm *realm, const vm_value *args, unsigned count, int repeat, vm_value *result);
static int timer_find_due(const struct bind_window *window, uint64_t limit, size_t *index);
static int timer_run(struct bind_window *window, const struct bind_timer *timer);
static int timer_run_due(struct bind_window *window);

/*
 * Reports when the next timer is due; zero when there is none.
 */
int
bind_next_timer(
	const struct bind_window *window,
	double *due)
{
	const struct bind_timer *timer;
	size_t index;
	int found;

	/* The earliest due time among the timers. */
	found = 0;
	for (index = 0; index < window->timers.count; index++) {
		timer = wb_vector_at(&window->timers, index);
		if (!found || timer->due < *due) {
			*due = timer->due;
			found = 1;
		}
	}

	/* Whether there is a timer at all. */
	return found;
}

/*
 * Runs the timers due at the window's time, earliest first, each
 * followed by a microtask checkpoint.  Returns 0, or ENOMEM.
 */
int
bind_run_timers(
	struct bind_window *window)
{
	struct vm_cell *owner;
	int status;

	/* Parent-realm callbacks can remove the child's last connected reference. */
	owner = NULL;
	if (window->realm->managed) {
		owner = &window->realm->cell;
		status = vm_heap_add_root(window->realm->heap, &owner);
		if (status != 0)
			return status;
	}

	/* Keeps the child host alive across every timer callback and checkpoint. */
	status = timer_run_due(window);
	if (owner != NULL)
		vm_heap_remove_root(window->realm->heap, &owner);

	/* Any callback failure retains its original return contract. */
	if (status != 0)
		return status;

	/* Succeeded: the bounded round has no remaining due callback. */
	return 0;
}

/*
 * Marks the callbacks and arguments of a window's timers.
 */
void
bind_timers_trace(
	struct vm_heap *heap,
	struct bind_window *window)
{
	const struct bind_timer *timer;
	size_t index;

	/* Each timer's callback and arguments. */
	for (index = 0; index < window->timers.count; index++) {
		timer = wb_vector_at(&window->timers, index);
		vm_heap_mark_value(heap, timer->callback);
		vm_heap_mark_value(heap, timer->arguments);
	}
}

/*
 * Calls a function (or runs a string as a script) once after a delay and
 * reports the timer's number (setTimeout).
 */
int
bind_set_timeout(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(this_value);

	/* A timer that runs once. */
	status = timer_add(realm, args, count, 0, result);
	if (status != 0)
		return status;

	/* Succeeded: the timer's number is reported. */
	return 0;
}

/*
 * Calls a function (or runs a string as a script) every period and
 * reports the timer's number (setInterval).
 */
int
bind_set_interval(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(this_value);

	/* A timer that repeats. */
	status = timer_add(realm, args, count, 1, result);
	if (status != 0)
		return status;

	/* Succeeded: the timer's number is reported. */
	return 0;
}

/*
 * Cancels a timer by its number (clearTimeout, clearInterval,
 * cancelAnimationFrame share the numbers); an unknown number is ignored.
 */
int
bind_clear_timer(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_timer *timer;
	double number;
	uint32_t id;
	size_t index;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The number. */
	*result = VM_VALUE_UNDEFINED;
	window = bind_window_of(realm);
	status = vm_to_number(realm, js_argument(args, count, 0), &number);
	if (status != 0)
		return status;
	if (!(number >= 1.0 && number <= 4294967295.0))
		return 0;
	id = (uint32_t)number;

	/* The timer with it goes (the last one takes its place). */
	for (index = 0; index < window->timers.count; index++) {
		timer = wb_vector_at(&window->timers, index);
		if (timer->id != id)
			continue;
		*timer = *(struct bind_timer *)wb_vector_at(&window->timers, window->timers.count - 1U);
		wb_vector_pop(&window->timers);
		break;
	}

	/* Succeeded: no timer has the number. */
	return 0;
}

/*
 * Calls a function with the time at the next animation frame and reports
 * its number (requestAnimationFrame).
 */
int
bind_request_animation_frame(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_timer timer;
	vm_value callback;
	int callable;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The callback must be a function. */
	window = bind_window_of(realm);
	callback = js_argument(args, count, 0);
	callable = vm_value_is_callable(callback);
	if (!callable) {
		status = vm_throw_type_error(realm, "Failed to execute 'requestAnimationFrame': The callback provided as parameter 1 is not a function.");
		return status;
	}

	/* A timer one frame away that passes the time. */
	memset(&timer, 0, sizeof(timer));
	timer.id = window->next_timer_id;
	timer.animation = 1;
	timer.due = window->now + TIMER_FRAME;
	timer.sequence = window->next_sequence;
	timer.callback = callback;
	timer.arguments = VM_VALUE_UNDEFINED;
	status = wb_vector_push(&window->timers, &timer);
	if (status != 0)
		return status;
	window->next_timer_id++;
	window->next_sequence++;

	/* Succeeded: the number is reported. */
	*result = vm_value_number((double)timer.id);
	return 0;
}

/* Adds a timer of setTimeout or setInterval from their arguments (handler, delay, arguments...). */
static int
timer_add(
	struct vm_realm *realm,
	const vm_value *args,
	unsigned count,
	int repeat,
	vm_value *result)
{
	struct bind_window *window;
	struct bind_timer timer;
	struct vm_string *code;
	vm_value callback;
	vm_value arguments;
	double delay;
	int callable;
	int status;

	/* The handler: a function, or a string to run as a script. */
	window = bind_window_of(realm);
	callback = js_argument(args, count, 0);
	callable = vm_value_is_callable(callback);
	if (!callable) {
		status = vm_to_string(realm, callback, &code);
		if (status != 0)
			return status;
		callback = vm_value_cell(code);
	}

	/* The delay: a number of milliseconds, zero when it is not one or is negative. */
	status = vm_to_number(realm, js_argument(args, count, 1), &delay);
	if (status != 0)
		return status;
	if (!(delay > 0.0))
		delay = 0.0;
	if (delay > 2147483647.0)
		delay = 2147483647.0;

	/* The arguments after the delay, passed on to the function. */
	arguments = VM_VALUE_UNDEFINED;
	if (count > 2U) {
		status = js_builtin_array(realm, args + 2, count - 2U, &arguments);
		if (status != 0)
			return status;
	}

	/* The timer. */
	memset(&timer, 0, sizeof(timer));
	timer.id = window->next_timer_id;
	timer.repeat = repeat;
	timer.due = window->now + delay;
	timer.interval = delay;
	if (timer.interval < TIMER_MIN_INTERVAL)
		timer.interval = TIMER_MIN_INTERVAL;
	timer.sequence = window->next_sequence;
	timer.callback = callback;
	timer.arguments = arguments;
	status = wb_vector_push(&window->timers, &timer);
	if (status != 0)
		return status;
	window->next_timer_id++;
	window->next_sequence++;

	/* Succeeded: the number is reported. */
	*result = vm_value_number((double)timer.id);
	return 0;
}

/* Runs the finite due-timer round while its public caller retains the host. */
static int
timer_run_due(
	struct bind_window *window)
{
	struct bind_timer *queued;
	struct bind_timer timer;
	uint64_t limit;
	size_t index;
	int found;
	int status;

	/* Detached contexts cannot deliver callbacks, including newly queued timers. */
	if (window->detached) {
		window->timers.count = 0;
		return 0;
	}

	/* Only the timers made before this round run in it. */
	limit = window->next_sequence;
	for (;;) {
		found = timer_find_due(window, limit, &index);
		if (!found)
			break;

		/* An interval stays for its next period (as the newest timer); a timeout goes. */
		queued = wb_vector_at(&window->timers, index);
		timer = *queued;
		if (timer.repeat) {
			queued->due = window->now + timer.interval;
			queued->sequence = window->next_sequence;
			window->next_sequence++;
		} else {
			*queued = *(struct bind_timer *)wb_vector_at(&window->timers, window->timers.count - 1U);
			wb_vector_pop(&window->timers);
		}

		/* The callback. */
		status = timer_run(window, &timer);
		if (status != 0)
			return status;
	}

	/* Succeeded: no timer made before the round is due. */
	return 0;
}

/* Finds the timer to run next: the earliest due by the window's time, made before the limit; zero when none. */
static int
timer_find_due(
	const struct bind_window *window,
	uint64_t limit,
	size_t *index)
{
	const struct bind_timer *timer;
	const struct bind_timer *best;
	size_t item;

	/* The earliest due, and the oldest among equals. */
	best = NULL;
	for (item = 0; item < window->timers.count; item++) {
		timer = wb_vector_at(&window->timers, item);
		if (timer->due > window->now)
			continue;
		if (timer->sequence >= limit)
			continue;
		if (best != NULL && timer->due > best->due)
			continue;
		if (best != NULL && timer->due == best->due && timer->sequence > best->sequence)
			continue;
		best = timer;
		*index = item;
	}

	/* Whether one is due. */
	if (best == NULL)
		return 0;
	return 1;
}

/* Runs one timer's callback (a function with its arguments, or a string as a script), then a checkpoint. */
static int
timer_run(
	struct bind_window *window,
	const struct bind_timer *timer)
{
	struct vm_realm *realm;
	struct vm_object *array;
	struct wb_units source;
	vm_value arguments[TIMER_ARGUMENTS_MAX];
	vm_value returned;
	unsigned count;
	int is_string;
	int status;

	/* A string runs as a script. */
	realm = window->realm;
	is_string = vm_value_is_string(timer->callback);
	if (is_string) {
		wb_units_init(&source);
		status = vm_string_append_units((struct vm_string *)vm_value_as_cell(timer->callback), &source);
		if (status == 0)
			status = bind_run_script(window, source.data, source.length, "a timer");
		wb_units_release(&source);
		return status;
	}

	/* A function gets the time (an animation frame's) or the arguments given with it. */
	count = 0;
	if (timer->animation) {
		arguments[0] = vm_value_number(window->now);
		count = 1;
	} else if (timer->arguments != VM_VALUE_UNDEFINED) {
		array = (struct vm_object *)vm_value_as_cell(timer->arguments);
		for (count = 0; count < array->length && count < TIMER_ARGUMENTS_MAX; count++)
			arguments[count] = array->elements[count];
	}

	/* The call with the window as this; an exception is reported. */
	status = vm_call(realm, timer->callback, vm_value_cell(realm->global), arguments, count, &returned);
	if (status == VM_THROWN) {
		bind_report_exception(window, realm->exception);
		realm->exception = VM_VALUE_UNDEFINED;
		status = 0;
	}

	/* Running out of memory stops the page. */
	if (status != 0)
		return status;

	/* The microtasks the callback queued. */
	status = bind_checkpoint(window);
	if (status != 0)
		return status;

	/* Succeeded: the timer has run. */
	return 0;
}
