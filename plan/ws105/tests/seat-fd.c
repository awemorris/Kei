/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks master validation and caller ownership against the guest's actual DRM. */
#include <vulkan/vulkan.h>
#include <libdrm/drm.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Refuses a nonmaster file and leaves both caller-owned files usable after release.
 */
int
main(
	void)
{
	VkInstance instance;
	VkPhysicalDevice physical;
	VkDisplayPropertiesKHR display;
	VkInstanceCreateInfo info;
	struct drm_auth authentication;
	const char *extensions[3];
	uint32_t count;
	VkResult result;
	int master;
	int nonmaster;
	int error;

	/* The first file is the root seat; the second file deliberately has no master authority. */
	master = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (master < 0)
		return 1;
	nonmaster = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
	if (nonmaster < 0) {
		(void)close(master);
		return 2;
	}

	/* Display inquiry uses its own file while the original seat remains master. */
	memset(&info, 0, sizeof(info));
	info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	extensions[0] = VK_KHR_DISPLAY_EXTENSION_NAME;
	extensions[1] = VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME;
	extensions[2] = VK_EXT_ACQUIRE_DRM_DISPLAY_EXTENSION_NAME;
	info.enabledExtensionCount = 3;
	info.ppEnabledExtensionNames = extensions;
	result = vkCreateInstance(&info, NULL, &instance);
	if (result != VK_SUCCESS)
		return 3;
	count = 1;
	result = vkEnumeratePhysicalDevices(instance, &count, &physical);
	if (result != VK_SUCCESS || count != 1)
		return 4;
	count = 1;
	result = vkGetPhysicalDeviceDisplayPropertiesKHR(physical, &count, &display);
	if (result != VK_SUCCESS || count != 1)
		return 5;

	/* A same-card file without master must fail before a swapchain can modeset. */
	result = vkAcquireDrmDisplayEXT(physical, nonmaster, display.display);
	if (result != VK_ERROR_INITIALIZATION_FAILED)
		return 6;
	error = fcntl(nonmaster, F_GETFD);
	if (error < 0)
		return 7;

	/* An already-master seat file succeeds without taking authority a second time. */
	result = vkAcquireDrmDisplayEXT(physical, master, display.display);
	if (result != VK_SUCCESS)
		return 8;
	result = vkReleaseDisplayEXT(physical, display.display);
	if (result != VK_SUCCESS)
		return 9;
	error = fcntl(master, F_GETFD);
	if (error < 0)
		return 10;

	/* Release closes only the duplicate; the caller still holds current master authority. */
	memset(&authentication, 0, sizeof(authentication));
	error = ioctl(master, DRM_IOCTL_AUTH_MAGIC, &authentication);
	if (error != -1 || errno != EINVAL)
		return 11;
	vkDestroyInstance(instance, NULL);
	(void)close(nonmaster);
	(void)close(master);

	/* Succeeded: bad authority was refused and caller file lifetime was preserved. */
	puts("seat-fd: PASS nonmaster-refused caller-files-live caller-master-retained");
	return 0;
}
