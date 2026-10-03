/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The kernel log.
 *
 * Log records are appended to a fixed ring buffer that drops its oldest
 * bytes when full and counts what it dropped, and every record is mirrored
 * to the platform debug console, and to the kernel console unless the log is
 * quiet (the boot parameter kmsg=quiet: the records are kept for dmesg only,
 * until the console is revealed).  kern_logf() renders a record with the
 * kcrt printf subset (kern_vsnprintf()) without a heap or floating point.
 */

#include "kern/klog.h"
#include "kern/atomic.h"
#include "kern/lock.h"
#include "kern/platform.h"
#include "kern/text-display.h"
#include <stdarg.h>
#include <kern/kcrt.h>
#include <hal/hal.h>

/*
 * The size of the ring.  amd64 keeps 512 KiB, so a driver's long start log
 * (the i915's, read over SSH after a start root asked for, ws118-p005) is
 * still whole when dmesg reads it; the smaller machines keep 32 KiB.
 */
#if defined(__x86_64__)
#define KLOG_CAPACITY (512U * 1024U)
#else
#define KLOG_CAPACITY (32U * 1024U)
#endif

/*
 * How long a record waits for another CPU's record to reach the consoles
 * before it is written anyway: a CPU that stopped in the middle of one (a
 * panic) must not stop every other CPU's output.
 */
#define KLOG_MIRROR_SPIN_MAX 50000000U

static struct spinlock klog_lock;
static char klog_buffer[KLOG_CAPACITY];
static size_t klog_oldest;
static size_t klog_used;
static uint64_t klog_dropped;

/*
 * The CPU (its number plus one) writing a record to the consoles, or 0.
 *
 * The consoles take one character at a time, so two CPUs logging at once
 * mixed their records character by character (BUG-031).  A record is
 * written whole while its CPU holds this.  It is not a lock of the rank
 * order: a record may be logged under any lock, and the console takes its
 * own locks per character, as before.
 */
static volatile unsigned klog_mirror_owner;

/*
 * Nonzero while the log is quiet: set from the boot parameters before the
 * first driver logs, cleared when the console is revealed (a reader on the
 * console, a diagnostic that bypasses the log).  Read without the lock.
 */
static volatile int klog_quiet;

static void append_locked(const char *bytes, size_t length);
static int mirror_enter(void);
static void mirror_leave(int owned);

/*
 * Initializes an empty kernel log.
 */
void
kern_log_init(
	void)
{
	/* Starts with an empty ring and no dropped bytes. */
	spin_init(&klog_lock, LOCK_RANK_KLOG, "kernel log");
	klog_oldest = 0;
	klog_used = 0;
	klog_dropped = 0;
}

/*
 * Appends one record to the log and mirrors it to the debug console.
 */
void
kern_log_write(
	const char *bytes,
	size_t length)
{
	char chunk[257];
	unsigned long irq;
	void (*console_output)(int c);
	size_t at;
	size_t n;
	int owned;
	int quiet;

	/* Ignores an empty record. */
	if (bytes == NULL || length == 0)
		return;

	/* Appends the record to the ring buffer. */
	irq = spin_lock_irqsave(&klog_lock);

	append_locked(bytes, length);

	spin_unlock_irqrestore(&klog_lock, irq);

	/* Writes the record to the consoles whole, apart from other CPUs' records. */
	owned = mirror_enter();

	/*
	 * Mirrors the record to the console the kernel has published, so a
	 * driver diagnostic is visible on screen and not only in the ring.
	 */
	console_output = __atomic_load_n(&kernel_putc, __ATOMIC_ACQUIRE);
	quiet = __atomic_load_n(&klog_quiet, __ATOMIC_ACQUIRE);
	if (console_output != NULL && !quiet) {
		for (at = 0; at < length; at++)
			console_output((unsigned char)bytes[at]);
	}

	/* A quiet console shows only that the boot goes on (ws035-p107). */
	if (quiet)
		kern_text_progress();

	/* Mirrors the record to the debug console in terminated chunks. */
	at = 0;
	while (at < length) {
		n = length - at;
		if (n > sizeof(chunk) - 1U)
			n = sizeof(chunk) - 1U;
		kern_memcpy(chunk, bytes + at, n);
		chunk[n] = 0;
		kern_platform_debug_write(chunk);
		at += n;
	}

	/* Lets another CPU's record reach the consoles. */
	mirror_leave(owned);
}

/*
 * Makes the log quiet (records for dmesg only) or loud again.
 */
void
kern_log_set_quiet(
	int quiet)
{
	/* The next record sees it. */
	__atomic_store_n(&klog_quiet, quiet != 0, __ATOMIC_RELEASE);
}

/*
 * Reports whether the log is quiet.
 */
int
kern_log_quiet(
	void)
{
	int quiet;

	/* The current state. */
	quiet = __atomic_load_n(&klog_quiet, __ATOMIC_ACQUIRE);
	return quiet;
}

/*
 * Formats one record with the kernel printf subset and logs it.
 *
 * The subset is kern_vsnprintf()'s.  Output is truncated to the internal
 * 512-byte buffer.
 */
void
kern_logf(
	const char *format,
	...)
{
	char text[512];
	va_list args;
	int rendered;
	size_t used;

	/* Ignores a missing format. */
	if (format == NULL)
		return;

	/* Renders the record into the buffer. */
	va_start(args, format);
	rendered = kern_vsnprintf(text, sizeof(text), format, args);
	va_end(args);

	/* Keeps only the text that fits, and nothing for a failed rendering. */
	used = 0;
	if (rendered > 0)
		used = (size_t)rendered;
	if (used > sizeof(text) - 1U)
		used = sizeof(text) - 1U;

	/* Records the rendered text. */
	kern_log_write(text, used);
}

/*
 * Copies the retained log into a caller buffer.
 *
 * The retained length is always reported; the copy happens only when the
 * buffer can hold all of it.
 */
size_t
kern_log_snapshot(
	char *buffer,
	size_t capacity,
	uint64_t *dropped)
{
	unsigned long irq;
	size_t i;
	size_t needed;

	irq = spin_lock_irqsave(&klog_lock);

	/* Reports the retained length and the dropped byte count. */
	needed = klog_used;
	if (dropped)
		*dropped = klog_dropped;

	/* Copies the ring contents in order when the buffer is large enough. */
	if (buffer && capacity >= needed) {
		for (i = 0; i < needed; i++)
			buffer[i] = klog_buffer[(klog_oldest + i) % KLOG_CAPACITY];
	}

	spin_unlock_irqrestore(&klog_lock, irq);

	/* Reports the retained length. */
	return needed;
}

/*
 * Reports the ring buffer capacity in bytes.
 */
size_t
kern_log_capacity(
	void)
{
	/* Reports the fixed capacity. */
	return KLOG_CAPACITY;
}

/* Appends bytes to the ring, dropping the oldest bytes when it is full. */
static void
append_locked(
	const char *bytes,
	size_t length)
{
	size_t i;
	uint64_t lost;
	size_t evicted;

	/* Keeps only the tail of a record larger than the whole ring. */
	if (length >= KLOG_CAPACITY) {
		lost = (uint64_t)klog_used + (uint64_t)(length - KLOG_CAPACITY);
		if (UINT64_MAX - klog_dropped < lost)
			klog_dropped = UINT64_MAX;
		else
			klog_dropped += lost;
		bytes += length - KLOG_CAPACITY;
		length = KLOG_CAPACITY;
		klog_oldest = 0;
		klog_used = 0;
	} else if (length > KLOG_CAPACITY - klog_used) {
		/* Evicts just enough of the oldest bytes to fit the record. */
		evicted = length - (KLOG_CAPACITY - klog_used);
		klog_oldest = (klog_oldest + evicted) % KLOG_CAPACITY;
		klog_used -= evicted;
		if (UINT64_MAX - klog_dropped < (uint64_t)evicted)
			klog_dropped = UINT64_MAX;
		else
			klog_dropped += (uint64_t)evicted;
	}

	/* Copies the record after the retained bytes, wrapping around. */
	for (i = 0; i < length; i++)
		klog_buffer[(klog_oldest + klog_used + i) % KLOG_CAPACITY] = bytes[i];
	klog_used += length;
}

/*
 * Takes the consoles for one record of this CPU.
 *
 * Reports whether it took them: a record logged while this CPU already
 * writes one (an interrupt, a nested diagnostic) goes out at once, and so
 * does a record that waited too long for another CPU.
 */
static int
mirror_enter(
	void)
{
	unsigned me;
	unsigned expected;
	unsigned spins;
	int taken;

	/* Names this CPU, zero meaning nobody. */
	me = (unsigned)hal_cpu_current() + 1U;

	/* Waits for the consoles to be free, within a bound. */
	for (spins = 0; spins < KLOG_MIRROR_SPIN_MAX; spins++) {
		expected = 0;
		taken = atomic_raw_compare_exchange(&klog_mirror_owner,
		    &expected, me);
		if (taken)
			return 1;

		/* This CPU is already writing a record; nesting goes out at once. */
		if (expected == me)
			return 0;
		hal_compiler_barrier();
	}

	/* Another CPU held them too long; the record goes out anyway. */
	return 0;
}

/* Gives the consoles back after a record that took them. */
static void
mirror_leave(
	int owned)
{
	/* A record that did not take them has nothing to give back. */
	if (!owned)
		return;
	atomic_raw_store_release(&klog_mirror_owner, 0);
}
