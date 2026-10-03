/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Binds the compositor's OS interface on Linux to libkeiland-backend's seat
 * (logind or direct, and the virtual terminal; ws131-p006) and to Vulkan's
 * display acquisition.
 */
#include "../compose.h"
#include "../zwl-os.h"

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Takes the Linux seat before Vulkan inquiry begins.
 */
int
zwl_os_open(
	struct zwl_server *server)
{
	const char *runtime;
	const char *path;
	int error;
	int length;

	/* The seat, its primary node and the virtual terminal, before the compatibility library's inquiry file. */
	error = kl_backend_seat_open(server->backend);
	if (error != 0)
		return error;
	server->os_paused = (unsigned)kl_backend_seat_paused(server->backend);

	/* Publishes the primary-node choice to libvulkan-compat without changing caller-provided intent. */
	path = kl_backend_seat_primary_path(server->backend);
	error = setenv("KEILAND_DRM_DEVICE", path, 1);
	if (error != 0)
		return errno;

	/* An explicit socket remains the caller's selected namespace. */
	if (server->socket_given == 0) {
		/* The user's runtime directory is preferred, otherwise the development socket lives in tmp. */
		runtime = getenv("XDG_RUNTIME_DIR");
		if (runtime == NULL || runtime[0] == '\0')
			runtime = "/tmp";

		/* Refuses a truncated Unix socket path rather than binding a different endpoint. */
		length = snprintf(server->socket_path, sizeof(server->socket_path), "%s/wayland-keiland", runtime);
		if (length < 0 || (size_t)length >= sizeof(server->socket_path))
			return ENAMETOOLONG;
	}

	/* Succeeded: the seat owns the display. */
	return 0;
}

/*
 * Restores the Linux console and returns the seat.
 */
void
zwl_os_close(
	struct zwl_server *server)
{
	/* The virtual terminal is restored after the display has been given back, then the seat returns. */
	kl_backend_seat_close(server->backend);
	server->os_paused = 0;
}

/*
 * Counts the seat's event-loop descriptors (logind's bus).
 */
size_t
zwl_os_poll_count(
	const struct zwl_server *server)
{
	size_t count;

	/* The backend's descriptors. */
	count = kl_backend_poll_count(server->backend);
	return count;
}

/*
 * Fills the seat's poll range.
 */
void
zwl_os_poll_fill(
	struct zwl_server *server,
	struct pollfd *descriptors)
{
	/* The backend fills its descriptors. */
	kl_backend_poll_fill(server->backend, descriptors);
}

/*
 * Handles the seat's events: the backend calls back to pause, resume or end.
 */
void
zwl_os_poll_done(
	struct zwl_server *server,
	const struct pollfd *descriptors)
{
	/* The callbacks are in backend-host.c. */
	kl_backend_poll_done(server->backend, descriptors);
}

/*
 * Gives Vulkan a duplicate of the seat's primary-node file.
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

	/* Resolves our enabled DRM-acquisition procedure on this compositor instance. */
	acquire = (PFN_vkAcquireDrmDisplayEXT)vkGetInstanceProcAddr(server->compose->instance, "vkAcquireDrmDisplayEXT");
	if (acquire == NULL)
		return VK_ERROR_EXTENSION_NOT_PRESENT;

	/* Vulkan duplicates the seat file; the compositor keeps its own original ownership. */
	descriptor = kl_backend_seat_primary_fd(server->backend);
	result = acquire(physical, descriptor, display);
	if (result != VK_SUCCESS)
		return result;

	/* Succeeded: the display may create its KMS swapchain. */
	return VK_SUCCESS;
}

/*
 * Releases Vulkan's private master-file duplicate.
 */
void
zwl_os_display_release(
	struct zwl_server *server,
	VkPhysicalDevice physical,
	VkDisplayKHR display)
{
	PFN_vkReleaseDisplayEXT release;
	VkResult result;

	/* Resolves the direct-mode release procedure after the swapchain has retired. */
	release = (PFN_vkReleaseDisplayEXT)vkGetInstanceProcAddr(server->compose->instance, "vkReleaseDisplayEXT");
	if (release == NULL)
		return;

	/* Returning this duplicate leaves the seat's original descriptor owned until OS cleanup. */
	result = release(physical, display);
	if (result != VK_SUCCESS)
		fprintf(stderr, "wayland: display release result=%d\n", result);

	/* Succeeded: ordinary shutdown can restore the VT and close the seat. */
	return;
}
