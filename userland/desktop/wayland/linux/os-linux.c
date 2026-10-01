/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Owns Linux seat startup, console mode and Vulkan display acquisition. */
#include "seat-linux.h"
#include "../compose.h"
#include "../zwl-os.h"
#include <errno.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* The saved console modes remain valid only after their corresponding inquiry succeeded. */
static int console_mode;
static int keyboard_mode;

/* Each successfully changed console property is restored independently on partial startup failure. */
static unsigned console_changed;
static unsigned keyboard_changed;

/*
 * Takes the Linux seat before Vulkan inquiry begins.
 */
int
zwl_os_open(
	struct zwl_server *server)
{
	const char *seat;
	const char *type;
	const char *session;
	const char *runtime;
	const char *path;
	int same;
	int error;
	int length;

	/* Explicit seat selection takes precedence over session metadata. */
	seat = getenv("KEILAND_SEAT");
	if (seat == NULL) {
		/* Only a real Wayland session with an ID selects logind automatically. */
		seat = "direct";
		type = getenv("XDG_SESSION_TYPE");
		session = getenv("XDG_SESSION_ID");
		if (type != NULL && session != NULL) {
			same = strcmp(type, "wayland");
			if (same == 0 && session[0] != '\0')
				seat = "logind";
		}
	}

	/* The later logind module has an explicit failure until it is available. */
	same = strcmp(seat, "logind");
	if (same == 0) {
		fprintf(stderr, "seat logind is not built yet\n");
		return ENOTSUP;
	}

	/* Unknown seat names cannot silently fall back to root device access. */
	same = strcmp(seat, "direct");
	if (same != 0)
		return EINVAL;

	/* Opens master before the compatibility library's inquiry file and uses the same exact path. */
	error = zwl_linux_seat_open(server);
	if (error != 0)
		return error;

	/* Publishes the primary-node choice to libvulkan-compat without changing caller-provided intent. */
	path = zwl_linux_drm_path();
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

	/* Console mode changes apply only when standard input is a Linux virtual terminal. */
	error = ioctl(STDIN_FILENO, KDGETMODE, &console_mode);
	if (error != 0)
		return 0;

	/* Saves the keyboard mode before changing either property. */
	error = ioctl(STDIN_FILENO, KDGKBMODE, &keyboard_mode);
	if (error != 0)
		return errno;

	/* Suppresses console drawing while Vulkan owns the CRTC. */
	error = ioctl(STDIN_FILENO, KDSETMODE, KD_GRAPHICS);
	if (error != 0)
		return errno;

	/* Partial cleanup must restore graphics mode even if the keyboard operation fails. */
	console_changed = 1;
	error = ioctl(STDIN_FILENO, KDSKBMODE, K_OFF);
	if (error != 0)
		return errno;

	/* Succeeded: evdev owns input without also sending keystrokes to the console. */
	keyboard_changed = 1;
	return 0;
}

/*
 * Restores the Linux console and returns the seat.
 */
void
zwl_os_close(
	struct zwl_server *server)
{
	/* Restores each successfully changed property after display teardown, including partial startup. */
	if (keyboard_changed != 0) {
		(void)ioctl(STDIN_FILENO, KDSKBMODE, keyboard_mode);
		keyboard_changed = 0;
	}

	/* Text drawing resumes only after the original CRTC has been restored by Vulkan. */
	if (console_changed != 0) {
		(void)ioctl(STDIN_FILENO, KDSETMODE, console_mode);
		console_changed = 0;
	}

	/* No primary-node file outlives the compositor's display session. */
	zwl_linux_seat_close(server);

	/* Succeeded: console and descriptor ownership have returned to the OS. */
	return;
}

/*
 * Counts the direct seat's event-loop descriptors.
 */
size_t
zwl_os_poll_count(
	const struct zwl_server *server)
{
	/* A direct seat needs no service socket in the poll snapshot. */
	(void)server;

	/* Succeeded: no OS descriptor needs polling. */
	return 0;
}

/*
 * Fills the direct seat's empty poll range.
 */
void
zwl_os_poll_fill(
	struct zwl_server *server,
	struct pollfd *descriptors)
{
	/* The direct seat's zero-sized range contains no entries. */
	(void)server;
	(void)descriptors;

	/* Succeeded: the caller's remaining poll entries are unchanged. */
	return;
}

/*
 * Handles the direct seat's empty event range.
 */
void
zwl_os_poll_done(
	struct zwl_server *server,
	const struct pollfd *descriptors)
{
	/* A direct seat has no service event to dispatch. */
	(void)server;
	(void)descriptors;

	/* Succeeded: common input processing may continue. */
	return;
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
	descriptor = zwl_linux_drm_fd();
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
