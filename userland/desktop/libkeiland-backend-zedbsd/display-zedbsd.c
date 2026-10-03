/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The display on zedBSD (libkeiland-backend since ws131-p008; from the
 * compositor's os-zedbsd.c).
 *
 * sessiond hands the display over before the compositor starts, and
 * libvulkan reaches the display itself: there is no node to share, nothing
 * to acquire or release, and the compositor keeps its own default socket.
 */

#include "userland/desktop/libkeiland-backend/keiland-backend-display.h"

#include <errno.h>

/*
 * Has no node to share.
 */
int
kl_backend_display_node(
	const struct kl_backend *backend,
	int *descriptor,
	const char **path)
{
	/* libvulkan opens the display itself. */
	(void)backend;
	*descriptor = -1;
	*path = "";
	return ENOTSUP;
}

/*
 * Has nothing to hand over: libvulkan reaches the display.
 */
VkResult
kl_backend_display_acquire(
	struct kl_backend *backend,
	const struct kl_backend_vulkan *vulkan,
	VkPhysicalDevice physical,
	VkDisplayKHR display)
{
	/* Succeeded: the display needs no OS descriptor. */
	(void)backend;
	(void)vulkan;
	(void)physical;
	(void)display;
	return VK_SUCCESS;
}

/*
 * Has nothing to release: libvulkan releases the display with the
 * swapchain.
 */
VkResult
kl_backend_display_release(
	struct kl_backend *backend,
	const struct kl_backend_vulkan *vulkan,
	VkPhysicalDevice physical,
	VkDisplayKHR display)
{
	/* Succeeded: nothing was acquired here. */
	(void)backend;
	(void)vulkan;
	(void)physical;
	(void)display;
	return VK_SUCCESS;
}

/*
 * Keeps the compositor's own default socket.
 */
int
kl_backend_display_socket(
	char *path,
	size_t size)
{
	/* The compositor's default stays. */
	(void)path;
	(void)size;
	return ENOTSUP;
}
