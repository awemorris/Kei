/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Binds the compositor's OS interface to native seatd and standard Vulkan display ownership. */
#include "seat-freebsd.h"
#include "../compose.h"
#include "../zwl-os.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Takes real native seat and primary ownership before Vulkan performs any device inquiry. */
int
zwl_os_open(
	struct zwl_server *server)
{
	const char *selection;
	const char *runtime;
	const char *path;
	int same;
	int error;
	int length;

	/* Native service authority is explicit; unknown providers cannot fall back to direct root access. */
	selection = getenv("KEILAND_SEAT");
	if (selection != NULL) {
		same = strcmp(selection, "seatd");
		if (same != 0)
			return EINVAL;
	}

	/* Keeps the daemon connection alive across activation and device withdrawal. */
	error = zwl_freebsd_seat_connect(server);
	if (error != 0)
		return error;

	/* The real primary must be acquired before the compatibility library opens its inquiry file. */
	error = zwl_freebsd_primary_open(server);
	if (error != 0)
		return error;

	/* Vulkan's native backend must inquire about the exact same primary node owned by the seat. */
	path = zwl_freebsd_primary_path();
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

/* Returns the native authority after common input and Vulkan cleanup have finished. */
void
zwl_os_close(
	struct zwl_server *server)
{
	/* The daemon restores its native VT policy after the compositor releases every device owner. */
	zwl_freebsd_seat_close(server);

	/* Succeeded: no native seat file or service session remains owned by this compositor. */
	return;
}

/* Counts the service descriptor that must remain observable while output is withdrawn. */
size_t
zwl_os_poll_count(
	const struct zwl_server *server)
{
	int descriptor;

	/* Partial startup without a connection contributes no invalid event-loop entry. */
	(void)server;
	descriptor = zwl_freebsd_seat_poll_fd();
	if (descriptor < 0)
		return 0;

	/* Succeeded: the real native authority contributes one readiness descriptor. */
	return 1;
}

/* Publishes the daemon's readiness entry to the common event-loop snapshot. */
void
zwl_os_poll_fill(
	struct zwl_server *server,
	struct pollfd *descriptors)
{
	/* Library-owned connection readiness also carries buffered activation and withdrawal events. */
	(void)server;
	descriptors[0].fd = zwl_freebsd_seat_poll_fd();
	descriptors[0].events = POLLIN;

	/* Succeeded: the service connection remains polled independently of display state. */
	return;
}

/* Delivers native ownership changes and marks a disconnected authority for ordinary teardown. */
void
zwl_os_poll_done(
	struct zwl_server *server,
	const struct pollfd *descriptors)
{
	int error;

	/* Dispatches buffered library notifications even when this snapshot reports no new readability. */
	error = zwl_freebsd_seat_dispatch(server);
	if (error != 0) {
		(void)fprintf(stderr, "wayland: native seat dispatch errno=%d\n", error);
		server->failed = 1;
		return;
	}

	/* Loss of the authority is fatal even if its last queued notification was successfully delivered. */
	if ((descriptors[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
		server->failed = 1;

	/* Succeeded: the common loop observes the daemon's latest native device generation. */
	return;
}

/* Lets Vulkan acquire the display using its own duplicate of the native primary lease. */
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
	descriptor = zwl_freebsd_primary_fd();
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

/* Releases Vulkan's display duplicate before the native seat returns its original primary file. */
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
