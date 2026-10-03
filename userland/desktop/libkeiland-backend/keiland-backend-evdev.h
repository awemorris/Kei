/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The input devices of libkeiland-backend (ws131-p007): the evdev types and
 * codes, and the calls that find, read and close the devices.
 *
 * zedBSD, Linux and FreeBSD share the evdev interface; only the header
 * that defines it differs, and this is the one place that chooses it (the
 * one block of the desktop selected by operating system, decision D3).
 * The compositor includes this header for the event types and codes it
 * interprets; nothing else of the operating system reaches it.
 */

#ifndef KL_BACKEND_EVDEV_H
#define KL_BACKEND_EVDEV_H

#if defined(__linux__)
#include <linux/input.h>
#elif defined(__FreeBSD__)
#include <dev/evdev/input.h>
#else
#include <uapi/input.h>
#endif

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

struct kl_backend;

/* The longest device path offered, with its NUL. */
#define KL_BACKEND_INPUT_PATH_MAX	64U

/*
 * The capability bitmaps of one evdev node, as EVIOCGBIT reports them.
 *
 * One instance lives on the stack while a node is classified.
 */
struct kl_backend_input_caps {
	unsigned long event[EV_MAX / (8U * sizeof(unsigned long)) + 1U];
	unsigned long key[KEY_MAX / (8U * sizeof(unsigned long)) + 1U];
	unsigned long relative[REL_MAX / (8U * sizeof(unsigned long)) + 1U];
	unsigned long absolute[ABS_MAX / (8U * sizeof(unsigned long)) + 1U];
};

/*
 * Opens every evdev node (/dev/input/eventN) the compositor does not read
 * yet (host input_known), through the seat where there is one (nothing
 * while it is paused), with the compositor's monotonic clock for the
 * events' times, and offers each to the compositor (host input_found).
 * A node the compositor does not keep is closed here.  A missing directory
 * is no error; the compositor scans again from time to time for devices
 * that come later.  The callbacks are called from this call.
 */
void kl_backend_input_scan(struct kl_backend *backend);

/*
 * Reads an axis's range, the device's name, its identity: 0 or an errno
 * value.
 */
int kl_backend_input_absinfo(int descriptor, uint32_t axis, struct input_absinfo *info);
int kl_backend_input_name(int descriptor, char *name, size_t size);
int kl_backend_input_id(int descriptor, struct input_id *id);

/*
 * Reads whole events without waiting: the count, 0 at the device's end, or
 * -1 with errno (EAGAIN when nothing is ready, EIO for a torn event, ENODEV
 * when the seat revoked the device: see kl_backend_seat_device_revoked).
 */
ssize_t kl_backend_input_read(int descriptor, struct input_event *events, size_t capacity);

/*
 * Closes a device the compositor kept, through the seat that opened it.
 */
void kl_backend_input_close(struct kl_backend *backend, int descriptor);

#endif
