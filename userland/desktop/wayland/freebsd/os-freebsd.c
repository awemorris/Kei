/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Binds the compositor's OS interface to libkeiland-backend's FreeBSD seat
 * (seatd, ws131-p006) and standard Vulkan display ownership.
 */
#include "../compose.h"
#include "../zwl-os.h"

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Takes real native seat and primary ownership before Vulkan performs any device inquiry.
 */
int
zwl_os_open(
	struct zwl_server *server)
{
	const char *runtime;
	const char *path;
	int error;
	int length;

	/* seatd's activation and the primary node, before the compatibility library opens its inquiry file. */
	server->os_paused = 1;
	error = kl_backend_seat_open(server->backend);
	if (error != 0)
		return error;
	server->os_paused = (unsigned)kl_backend_seat_paused(server->backend);

	/* Vulkan's native backend must inquire about the exact same primary node owned by the seat. */
	path = kl_backend_seat_primary_path(server->backend);
	error = setenv("KEILAND_DRM_DEVICE", path, 1);
	if (error != 0)
		return errno;

	/* An explicit compositor socket retains the caller's selected namespace. */
	if (server->socket_given == 0) {
		/* Native sessions use their runtime directory, with the established development fallback. */
		runtime = getenv("XDG_RUNTIME_DIR");
		if (runtime == NULL || runtime[0] == '\0')
			runtime = "/tmp";

		/* Refuses a truncated endpoint instead of binding a different service name. */
		length = snprintf(server->socket_path, sizeof(server->socket_path), "%s/wayland-keiland", runtime);
		if (length < 0 || (size_t)length >= sizeof(server->socket_path))
			return ENAMETOOLONG;
	}

	/* Succeeded: seatd owns VT behavior and Vulkan can acquire the real native primary. */
	return 0;
}

/*
 * Returns the native authority after common input and Vulkan cleanup have finished.
 */
void
zwl_os_close(
	struct zwl_server *server)
{
	/* The daemon restores its native VT policy after the compositor releases every device owner. */
	kl_backend_seat_close(server->backend);
	server->os_paused = 1;

	/* Succeeded: no native seat file or service session remains owned by this compositor. */
	return;
}

/*
 * Counts the service descriptor that must remain observable while output is withdrawn.
 */
size_t
zwl_os_poll_count(
	const struct zwl_server *server)
{
	size_t count;

	/* seatd's descriptor, polled while the output is withdrawn too (none before the seat is taken). */
	count = kl_backend_poll_count(server->backend);
	return count;
}

/*
 * Publishes the daemon's readiness entry to the common event-loop snapshot.
 */
void
zwl_os_poll_fill(
	struct zwl_server *server,
	struct pollfd *descriptors)
{
	/* The backend fills seatd's descriptor. */
	kl_backend_poll_fill(server->backend, descriptors);
}

/*
 * Delivers native ownership changes and marks a disconnected authority for ordinary teardown.
 */
void
zwl_os_poll_done(
	struct zwl_server *server,
	const struct pollfd *descriptors)
{
	/* The backend dispatches seatd's notifications; the callbacks (backend-host.c) pause, resume or end. */
	kl_backend_poll_done(server->backend, descriptors);
}

/*
 * Lets Vulkan acquire the display using its own duplicate of the native primary lease.
 */
VkResult
zwl_os_display_acquire(
	struct zwl_server *server,
	VkPhysicalDevice physical,
	VkDisplayKHR display)
{
	PFN_vkAcquireDrmDisplayEXT acquire;
	VkResult result;
	int descriptor;

	/* Missing real primary ownership cannot be reported as a successful display acquisition. */
	descriptor = kl_backend_seat_primary_fd(server->backend);
	if (descriptor < 0)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Resolves the standard extension on the actual compositor instance. */
	acquire = (PFN_vkAcquireDrmDisplayEXT)vkGetInstanceProcAddr(server->compose->instance, "vkAcquireDrmDisplayEXT");
	if (acquire == NULL)
		return VK_ERROR_EXTENSION_NOT_PRESENT;

	/* The Vulkan adapter retains its own duplicate while the original remains seat-owned. */
	result = acquire(physical, descriptor, display);
	if (result != VK_SUCCESS)
		return result;

	/* Succeeded: the real display may now create its Vulkan swapchain. */
	return VK_SUCCESS;
}

/*
 * Releases Vulkan's display duplicate before the native seat returns its original primary file.
 */
void
zwl_os_display_release(
	struct zwl_server *server,
	VkPhysicalDevice physical,
	VkDisplayKHR display)
{
	PFN_vkReleaseDisplayEXT release;
	VkResult result;

	/* Resolves the standard release extension only after the output's swapchain has retired. */
	release = (PFN_vkReleaseDisplayEXT)vkGetInstanceProcAddr(server->compose->instance, "vkReleaseDisplayEXT");
	if (release == NULL) {
		server->failed = 1;
		return;
	}

	/* A real release failure remains visible to the common shutdown outcome. */
	result = release(physical, display);
	if (result != VK_SUCCESS) {
		(void)fprintf(stderr, "wayland: native display release result=%d\n", result);
		server->failed = 1;
		return;
	}

	/* Succeeded: native seat cleanup may return its original primary-node lease. */
	return;
}
