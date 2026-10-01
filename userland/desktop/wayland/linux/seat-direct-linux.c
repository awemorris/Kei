/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Opens the development seat without calling any DRM ioctl in the compositor. */
#include "seat-linux.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* The primary node is retained until the display and every input device have retired. */
static int seat_drm = -1;

/* The selected pathname is fixed before the first Vulkan instance is created. */
static char seat_path[4096];

/*
 * Opens the root seat's selected primary node.
 */
int
zwl_linux_seat_open(
	struct zwl_server *server)
{
	const char *requested;
	size_t length;

	/* One compositor owns the process seat; server storage holds common state. */
	(void)server;
	requested = getenv("KEILAND_DRM_DEVICE");
	if (requested == NULL)
		requested = "/dev/dri/card0";

	/* A bounded absolute path is required for authoritative device selection. */
	length = strlen(requested);
	if (length >= sizeof(seat_path) || requested[0] != '/')
		return EINVAL;

	/* The first primary-node open takes automatic master before Vulkan inquiry opens its separate file. */
	memcpy(seat_path, requested, length + 1);
	seat_drm = open(seat_path, O_RDWR | O_CLOEXEC);
	if (seat_drm < 0)
		return errno;

	/* Succeeded: Vulkan will receive this already owned file through acquire_drm_display. */
	return 0;
}

/*
 * Returns the root seat's primary descriptor.
 */
void
zwl_linux_seat_close(
	struct zwl_server *server)
{
	/* A failed startup may never have opened a primary node. */
	(void)server;
	if (seat_drm >= 0) {
		(void)close(seat_drm);
		seat_drm = -1;
	}

	/* Succeeded: no display descriptor remains owned by this seat. */
	return;
}

/*
 * Opens one nonblocking evdev descriptor.
 */
int
zwl_linux_device_open(
	struct zwl_server *server,
	const char *path)
{
	int descriptor;

	/* Direct input access has no separate service ownership. */
	(void)server;
	descriptor = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (descriptor < 0)
		return -1;

	/* Succeeded: the caller owns this independently closeable descriptor. */
	return descriptor;
}

/*
 * Releases one input descriptor.
 */
void
zwl_linux_device_close(
	struct zwl_server *server,
	int descriptor)
{
	/* Device access returns after the common seat has removed its live input record. */
	(void)server;
	(void)close(descriptor);

	/* Succeeded: this input file is no longer owned. */
	return;
}

/*
 * Supplies the live primary-node descriptor.
 */
int
zwl_linux_drm_fd(
	void)
{
	/* Succeeded: an invalid descriptor explicitly denotes absent seat ownership. */
	return seat_drm;
}

/*
 * Supplies the selected primary-node pathname.
 */
const char *
zwl_linux_drm_path(
	void)
{
	/* Succeeded: the process owns this stable pathname until shutdown. */
	return seat_path;
}

/*
 * Reports direct-seat device availability.
 */
int
zwl_linux_seat_paused(
	void)
{
	/* Succeeded: a direct seat has no service-driven pause state. */
	return 0;
}
