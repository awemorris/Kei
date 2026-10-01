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

/* Explicit selection is fixed before any device lease and retained until seat cleanup. */
static unsigned seat_logind;
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

	/* Unknown seat names cannot silently fall back to root device access. */
	same = strcmp(seat, "logind");
	if (same == 0) {
		seat_logind = 1;
	} else {
		same = strcmp(seat, "direct");
		if (same != 0)
			return EINVAL;
		seat_logind = 0;
	}

	/* Opens master before the compatibility library's inquiry file and uses the same exact path. */
	error = zwl_linux_seat_open(server);
	/* A backend refusal cannot supply seat authority. */
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
	/* Only the service seat contributes a bus descriptor to the snapshot. */
	(void)server;
	if (seat_logind != 0)
		return 1;

	/* Succeeded: the root seat contributes no service descriptor. */
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
	/* The service socket remains readable while display and input are paused. */
	(void)server;
	if (seat_logind != 0) {
		descriptors[0].fd = zwl_linux_logind_poll_fd();
		descriptors[0].events = POLLIN;
	}

	/* Succeeded: the selected seat's entire OS poll range is populated. */
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
	int error;

	/* Queued signals need dispatch even when the current socket readiness is zero. */
	if (seat_logind == 0)
		return;
	error = zwl_linux_logind_dispatch(server);
	if (error != 0) {
		printf("ZWL SEAT error errno=%d\n", error);
		server->failed = 1;
		return;
	}

	/* A disconnected authority stops the compositor through ordinary cleanup. */
	if ((descriptors[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
		server->failed = 1;

	/* Succeeded: the common loop observes the newest device generation. */
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

/*
 * Opens the selected Linux seat before Vulkan startup.
 */
int
zwl_linux_seat_open(
	struct zwl_server *server)
{
	int error;

	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		error = zwl_linux_logind_seat_open(server);
	} else {
		error = zwl_linux_direct_seat_open(server);
	}

	/* A backend refusal cannot supply seat authority. */
	if (error != 0)
		return error;

	/* Succeeded: the selected backend owns the process seat. */
	return 0;
}

/*
 * Returns the selected backend seat and all remaining device leases.
 */
void
zwl_linux_seat_close(
	struct zwl_server *server)
{
	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		zwl_linux_logind_seat_close(server);
	} else {
		zwl_linux_direct_seat_close(server);
	}

	/* Succeeded: this resource no longer retains seat authority. */
	return;
}

/*
 * Opens an input file through the selected seat authority.
 */
int
zwl_linux_device_open(
	struct zwl_server *server,
	const char *path)
{
	int descriptor;

	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		descriptor = zwl_linux_logind_device_open(server, path);
	} else {
		descriptor = zwl_linux_direct_device_open(server, path);
	}

	/* No input descriptor exists when its backend refused the device. */
	if (descriptor < 0)
		return -1;

	/* Succeeded: the selected backend owns this input descriptor. */
	return descriptor;
}

/*
 * Returns one input descriptor to its selected seat owner.
 */
void
zwl_linux_device_close(
	struct zwl_server *server,
	int descriptor)
{
	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		zwl_linux_logind_device_close(server, descriptor);
	} else {
		zwl_linux_direct_device_close(server, descriptor);
	}

	/* Succeeded: this resource no longer retains seat authority. */
	return;
}

/*
 * Supplies the selected backend primary node for Vulkan acquisition.
 */
int
zwl_linux_drm_fd(
	void)
{
	int descriptor;

	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		descriptor = zwl_linux_logind_drm_fd();
	} else {
		descriptor = zwl_linux_direct_drm_fd();
	}

	/* Succeeded: the selected backend supplies this seat property. */
	return descriptor;
}

/*
 * Supplies the exact primary path owned by the selected seat.
 */
const char *
zwl_linux_drm_path(
	void)
{
	const char *path;

	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		path = zwl_linux_logind_drm_path();
	} else {
		path = zwl_linux_direct_drm_path();
	}

	/* Succeeded: the selected backend supplies this seat property. */
	return path;
}

/*
 * Reports whether the selected seat has withdrawn device authority.
 */
int
zwl_linux_seat_paused(
	void)
{
	int paused;

	/* Selection remains stable throughout this compositor lifetime. */
	if (seat_logind != 0) {
		paused = zwl_linux_logind_seat_paused();
	} else {
		paused = zwl_linux_direct_seat_paused();
	}

	/* Succeeded: the selected backend supplies this seat property. */
	return paused;
}

/*
 * Retains service device ownership when revocation precedes the bus notification.
 */
int
zwl_linux_device_revoked(
	struct zwl_server *server,
	int descriptor)
{
	int retained;

	/* Direct device failure has no future service resume notification. */
	if (seat_logind == 0)
		return 0;
	retained = zwl_linux_logind_device_revoked(server, descriptor);

	/* Succeeded: the caller knows whether common teardown must wait for logind. */
	return retained;
}
