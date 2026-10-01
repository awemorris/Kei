/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The deliberately interposed backend used to verify recursion refusal.
 */

#include <vulkan/vulkan.h>

/* Forces a PLT reference: gcc may bind a direct recursive C call to a local alias even at -O0. */
extern VkResult fake_backend_create(const VkInstanceCreateInfo *info, const VkAllocationCallbacks *allocator, VkInstance *instance) __asm__("vkCreateInstance");

/*
 * Calls instance creation by its public symbol to provoke backend interposition.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(
	const VkInstanceCreateInfo *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkInstance *pInstance)
{
	VkResult error;

	/* Resolves through the PLT when compiled with gcc without symbolic binding. */
	error = fake_backend_create(pCreateInfo, pAllocator, pInstance);
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: an unexpected return is preserved for the test to reject. */
	return VK_SUCCESS;
}

/*
 * Exposes a distinct backend resolver for the library identity check.
 */
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(
	VkInstance instance,
	const char *pName)
{
	/* This fake backend never supplies real dispatch procedures. */
	(void)instance;
	(void)pName;

	/* Succeeded: no usable backend procedure exists. */
	return NULL;
}

/*
 * Reports an empty extension list from the deliberately incomplete backend.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(
	const char *pLayerName,
	uint32_t *pPropertyCount,
	VkExtensionProperties *pProperties)
{
	/* The fake backend offers no extensions to its caller. */
	(void)pLayerName;
	(void)pProperties;
	*pPropertyCount = 0;

	/* Succeeded: the extension list is empty. */
	return VK_SUCCESS;
}
