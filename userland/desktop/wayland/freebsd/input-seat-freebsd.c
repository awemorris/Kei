/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Forwards the shared evdev reader's device leases to libkeiland-backend's
 * FreeBSD seat (seatd, ws131-p006).
 */

#include "../evdev/seat.h"
#include "../zwl.h"

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

/*
 * Opens one input device through the seat.
 */
int
zwl_seat_device_open(
	struct zwl_server *server,
	const char *path)
{
	int descriptor;

	/* The seat's lease, or -1 with its errno. */
	descriptor = kl_backend_seat_device_open(server->backend, path);
	if (descriptor < 0)
		return -1;

	/* Succeeded: the reader owns this nonblocking descriptor until it closes it. */
	return descriptor;
}

/*
 * Returns one input device to the seat.
 */
void
zwl_seat_device_close(
	struct zwl_server *server,
	int descriptor)
{
	/* The seat closes the file and returns its device ID. */
	kl_backend_seat_device_close(server->backend, descriptor);
}

/*
 * Tells whether the seat admits no new input device.
 */
int
zwl_seat_paused(
	const struct zwl_server *server)
{
	int paused;

	/* Disabled by seatd, or not taken. */
	paused = kl_backend_seat_paused(server->backend);
	return paused;
}

/*
 * Lets an input whose reading failed be closed as usual (seatd wants fresh opens).
 */
int
zwl_seat_device_revoked(
	struct zwl_server *server,
	int descriptor)
{
	int retained;

	/* The seat keeps nothing. */
	retained = kl_backend_seat_device_revoked(server->backend, descriptor);
	return retained;
}
