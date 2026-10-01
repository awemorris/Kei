/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Forwards shared evdev ownership to Linux's selected direct or logind seat. */
#include "../evdev/seat.h"
#include "seat-linux.h"

/*
 * Opens one native input device through the selected Linux seat authority.
 */
int
zwl_seat_device_open(
	struct zwl_server *server,
	const char *path)
{
	int descriptor;

	/* Retains the existing Linux seat's device lease and error convention. */
	descriptor = zwl_linux_device_open(server, path);
	if (descriptor < 0)
		return -1;

	/* Succeeded: the caller may read the selected seat's owned nonblocking descriptor. */
	return descriptor;
}

/*
 * Returns one native input device to the selected Linux seat owner.
 */
void
zwl_seat_device_close(
	struct zwl_server *server,
	int descriptor)
{
	/* The same selected seat releases both the descriptor and any service device lease. */
	zwl_linux_device_close(server, descriptor);

	/* Succeeded: shared evdev no longer owns this seat device. */
	return;
}

/*
 * Reports whether the Linux seat has withdrawn input device authority.
 */
int
zwl_seat_paused(
	void)
{
	int paused;

	/* Service notifications remain ordered by the existing Linux seat state machine. */
	paused = zwl_linux_seat_paused();
	if (paused != 0)
		return 1;

	/* Succeeded: the selected seat currently permits shared evdev discovery. */
	return 0;
}

/*
 * Preserves a service lease whose kernel revocation precedes its seat notification.
 */
int
zwl_seat_device_revoked(
	struct zwl_server *server,
	int descriptor)
{
	int retained;

	/* Linux logind decides whether this descriptor awaits an ordered pause/resume event. */
	retained = zwl_linux_device_revoked(server, descriptor);
	if (retained != 0)
		return 1;

	/* Succeeded: ordinary device retirement may close this unretained input descriptor. */
	return 0;
}
