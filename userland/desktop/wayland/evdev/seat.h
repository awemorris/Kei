/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Defines the native seat authority used by shared evdev discovery and reads. */
#ifndef ZWL_EVDEV_SEAT_H
#define ZWL_EVDEV_SEAT_H

struct zwl_server;

/* Returns an owned nonblocking/CLOEXEC fd, or -1/errno; device_close returns that same lease. */
int zwl_seat_device_open(struct zwl_server *server, const char *path);
void zwl_seat_device_close(struct zwl_server *server, int descriptor);
/* A paused service retains its device leases until its own ordered notification. */
int zwl_seat_paused(const struct zwl_server *server);
int zwl_seat_device_revoked(struct zwl_server *server, int descriptor);

#endif
