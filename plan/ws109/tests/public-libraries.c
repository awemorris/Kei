/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Uses the installed public foundation headers as an independent native client.
 */

#include <wayland-client.h>
#include <truetype.h>
#include <vulkan/vulkan.h>
#include <errno.h>
#include <stdio.h>

/*
 * Checks native public ABI linkage and ordinary library error conventions.
 */
int
main(
	void)
{
	struct wl_array array;
	struct truetype_face *face;
	uint32_t version;
	uint32_t major;
	void *bytes;
	VkResult error;
	int font_error;

	/* Allocates caller-owned storage through the installed Wayland public ABI. */
	wl_array_init(&array);
	bytes = wl_array_add(&array, 32);
	if (bytes == NULL)
		return 1;

	/* Checks the public allocation extent before retiring its storage. */
	if (array.size != 32)
		return 1;

	/* Releases the public array through the same installed library. */
	wl_array_release(&array);

	/* Checks native errno behavior across the public TrueType boundary. */
	face = NULL;
	font_error = truetype_open(NULL, 0, 0, &face);
	if (font_error != EINVAL)
		return 1;

	/* Uses the standard Vulkan prototype to reach our installed frontend. */
	error = vkEnumerateInstanceVersion(&version);
	if (error != VK_SUCCESS)
		return 1;

	/* Requires a usable system version behind the native frontend. */
	major = VK_VERSION_MAJOR(version);
	if (major < 1)
		return 1;

	/* Publishes only the verified installed-header/linkage result. */
	(void)puts("public-libraries: PASS installed Wayland/TrueType/standard Vulkan ABI");

	/* Succeeded: the independent native client used all three installed interfaces. */
	return 0;
}
