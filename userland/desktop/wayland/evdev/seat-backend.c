/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Forwards the shared evdev reader's device leases to libkeiland-backend's
 * seat (Linux: logind or direct; FreeBSD: seatd; ws131-p006).
 */

#include "seat.h"
#include "../zwl.h"

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <stdio.h>

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
 * Tells the seat an input's reading failed as revoked: a device the seat
 * keeps is set aside until it resumes, another is closed as usual.
 */
int
zwl_seat_device_revoked(
	struct zwl_server *server,
	int descriptor)
{
	int retained;

	struct zwl_input_device *input;
	unsigned index;

	/* logind keeps the device for its later resume; seatd keeps nothing. */
	retained = kl_backend_seat_device_revoked(server->backend, descriptor);
	if (retained == 0)
		return 0;

	/* A kept device is not read until the seat resumes it (input_resumed) or says it is gone. */
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		input = &server->inputs[index];
		if (input->live != 0 && input->fd == descriptor) {
			input->fd = -1;
			printf("ZWL SEAT input_revoked path=%s lease=retained\n", input->path);
			break;
		}
	}
	return 1;
}
