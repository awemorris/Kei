/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The live instance ownership records and backend extension filtering.
 */

#include "compat.h"
#include <stdlib.h>
#include <string.h>

/* The live instances, inserted after creation and removed before destruction under compat_mutex. */
static struct compat_instance *compat_instances;

static VkResult instance_physicals(struct compat_instance *instance);

/*
 * Creates a backend instance and retains its physical-device ownership.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateInstance(
	const VkInstanceCreateInfo *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkInstance *pInstance)
{
	struct compat_instance *instance;
	uint32_t index;
	VkResult error;
	int wsi;

	/* Requires the public creation inputs before using the backend. */
	if (pCreateInfo == NULL || pInstance == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* The chain-only phase does not offer WSI extensions. */
	for (index = 0; index < pCreateInfo->enabledExtensionCount; index++) {
		/* Rejects extensions whose handles would belong to the backend WSI. */
		wsi = compat_wsi_extension(pCreateInfo->ppEnabledExtensionNames[index]);
		if (wsi != 0)
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	/* Enters the creation boundary and initializes the process-wide backend. */
	compat_enter("vkCreateInstance");
	if (compat_backend.create_instance == NULL) {
		/* Leaves an unavailable driver without creating a foreign instance. */
		compat_leave();
		return VK_ERROR_INCOMPATIBLE_DRIVER;
	}

	/* Allocates the ownership record before creating the backend resource. */
	instance = calloc(1, sizeof(*instance));
	if (instance == NULL) {
		/* Leaves a refused allocation without invoking the backend. */
		compat_leave();
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	/* Creates the instance through the directly resolved backend entry point. */
	error = compat_backend.create_instance(pCreateInfo, pAllocator, pInstance);
	if (error != VK_SUCCESS) {
		/* Releases the unused record and the call boundary on failure. */
		free(instance);
		compat_leave();
		return error;
	}

	/* Remembers the API version without changing the caller's backend handle. */
	instance->handle = *pInstance;
	instance->api_version = VK_API_VERSION_1_0;
	if (pCreateInfo->pApplicationInfo != NULL && pCreateInfo->pApplicationInfo->apiVersion != 0)
		instance->api_version = pCreateInfo->pApplicationInfo->apiVersion;

	/* Records which physical devices belong to this backend instance. */
	error = instance_physicals(instance);
	if (error != VK_SUCCESS) {
		/* Unwinds the created instance before reporting a bookkeeping failure. */
		compat_backend.destroy_instance(*pInstance, pAllocator);
		free(instance->physicals);
		free(instance);
		*pInstance = VK_NULL_HANDLE;
		compat_leave();
		return error;
	}

	/* Publishes a fully initialized ownership record. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Makes the new instance discoverable only after its ownership is complete. */
	instance->next = compat_instances;
	compat_instances = instance;

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Leaves the completed creation boundary. */
	compat_leave();

	/* Succeeded: the caller owns the backend instance and we retain its ownership record. */
	return VK_SUCCESS;
}

/*
 * Retires instance bookkeeping and destroys the backend instance.
 */
VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(
	VkInstance instance,
	const VkAllocationCallbacks *pAllocator)
{
	struct compat_instance **link;
	struct compat_instance *record;

	/* Vulkan permits destruction of a null instance. */
	if (instance == VK_NULL_HANDLE)
		return;

	/* Enters the destruction boundary before any backend call. */
	compat_enter("vkDestroyInstance");

	/* Removes the ownership record while its backend handle is still live. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Begins the retirement search without a matched ownership record. */
	record = NULL;

	/* Locates the instance's link for removal. */
	for (link = &compat_instances; *link != NULL; link = &(*link)->next) {
		/* Retires only the matching backend handle. */
		if ((*link)->handle == instance) {
			record = *link;
			*link = record->next;
			break;
		}
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Destroys the resource through the backend's directly resolved interceptor. */
	if (compat_backend.destroy_instance == NULL)
		compat_missing("vkDestroyInstance");

	/* Releases the backend instance after its bookkeeping is no longer discoverable. */
	compat_backend.destroy_instance(instance, pAllocator);

	/* Releases physical-device membership with its parent record. */
	if (record != NULL) {
		free(record->physicals);
		free(record);
	}

	/* Leaves the completed destruction boundary. */
	compat_leave();

	/* Succeeded: the instance and its ownership record are retired. */
	return;
}

/*
 * Lists backend instance extensions without backend WSI extensions.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(
	const char *pLayerName,
	uint32_t *pPropertyCount,
	VkExtensionProperties *pProperties)
{
	VkResult error;

	/* Requires the enumeration count destination. */
	if (pPropertyCount == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Enters the backend enumeration boundary. */
	compat_enter("vkEnumerateInstanceExtensionProperties");

	/* Filters only driver extensions; named layer extensions retain their own meaning. */
	error = compat_extensions(VK_NULL_HANDLE, pLayerName, pPropertyCount, pProperties);

	/* Leaves the completed enumeration boundary. */
	compat_leave();

	/* Reports an incomplete caller array or a backend enumeration failure. */
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: the caller has the available non-WSI extension list. */
	return VK_SUCCESS;
}

/*
 * Lists the backend's instance layers or an empty list without a backend.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(
	uint32_t *pPropertyCount,
	VkLayerProperties *pProperties)
{
	VkResult error;

	/* Requires the enumeration count destination. */
	if (pPropertyCount == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Enters the backend's global layer enumeration boundary. */
	compat_enter("vkEnumerateInstanceLayerProperties");
	if (compat_backend.instance_layers == NULL) {
		/* An absent backend provides no layers and is not an enumeration failure. */
		*pPropertyCount = 0;
		compat_leave();
		return VK_SUCCESS;
	}

	/* Preserves the backend's layer-specific enumeration contract. */
	error = compat_backend.instance_layers(pPropertyCount, pProperties);

	/* Leaves the completed layer enumeration boundary. */
	compat_leave();

	/* Preserves a backend error or an incomplete caller array. */
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: the layer list is the backend's own list. */
	return VK_SUCCESS;
}

/*
 * Reports the backend's API version, falling back to Vulkan 1.0.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceVersion(
	uint32_t *pApiVersion)
{
	VkResult error;

	/* Requires a version destination. */
	if (pApiVersion == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Enters the backend's global version boundary. */
	compat_enter("vkEnumerateInstanceVersion");
	if (compat_backend.instance_version == NULL) {
		/* A Vulkan 1.0 or missing backend has no version entry point. */
		*pApiVersion = VK_API_VERSION_1_0;
		compat_leave();
		return VK_SUCCESS;
	}

	/* Preserves the backend's supported API version. */
	error = compat_backend.instance_version(pApiVersion);

	/* Leaves the completed version query. */
	compat_leave();

	/* Preserves a backend version-query error. */
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: the caller knows the backend's supported version. */
	return VK_SUCCESS;
}

/*
 * Finds a live ownership record for an instance whose lifetime the caller holds.
 */
struct compat_instance *
compat_instance_get(
	VkInstance handle)
{
	struct compat_instance *instance;

	/* Searches the live ownership list while insertion and removal are excluded. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Finds the caller's backend instance handle. */
	for (instance = compat_instances; instance != NULL; instance = instance->next) {
		/* Stops at the requested owner. */
		if (instance->handle == handle)
			break;
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Succeeded: returns the live record, or NULL for an unknown handle. */
	return instance;
}

/*
 * Finds the instance owning a physical device without wrapping its dispatchable handle.
 */
struct compat_instance *
compat_instance_for_physical(
	VkPhysicalDevice physical)
{
	struct compat_instance *instance;
	struct compat_instance *owner;
	uint32_t index;

	/* Searches immutable physical-device membership under the live-list mutex. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Starts without a known physical-device owner. */
	owner = NULL;

	/* Inspects each still-live instance's physical-device membership. */
	for (instance = compat_instances; instance != NULL; instance = instance->next) {
		/* Finds the physical device in this instance's immutable list. */
		for (index = 0; index < instance->physical_count; index++) {
			/* Records the instance whose backend returned this device. */
			if (instance->physicals[index] == physical) {
				owner = instance;
				break;
			}
		}

		/* Stops once physical-device ownership is established. */
		if (owner != NULL)
			break;
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Succeeded: returns the live physical-device owner or NULL. */
	return owner;
}

/*
 * Copies backend extension lists while excluding foreign WSI responsibilities.
 */
VkResult
compat_extensions(
	VkPhysicalDevice physical,
	const char *layer,
	uint32_t *count,
	VkExtensionProperties *properties)
{
	VkExtensionProperties *available;
	uint32_t total;
	uint32_t index;
	uint32_t kept;
	uint32_t capacity;
	VkResult error;
	int wsi;

	/* Missing global enumeration functions provide an empty successful list. */
	if (physical == VK_NULL_HANDLE && compat_backend.instance_extensions == NULL) {
		*count = 0;
		return VK_SUCCESS;
	}

	/* A physical-device query requires the backend's device enumeration function. */
	if (physical != VK_NULL_HANDLE && compat_backend.device_extensions == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Named layer extension lists are not filtered or rewritten. */
	if (layer != NULL) {
		/* Selects the matching backend enumeration entry point. */
		if (physical == VK_NULL_HANDLE) {
			error = compat_backend.instance_extensions(layer, count, properties);
		} else {
			error = compat_backend.device_extensions(physical, layer, count, properties);
		}

		/* Preserves a backend error or an incomplete caller array. */
		if (error != VK_SUCCESS)
			return error;

		/* Succeeded: the caller has the layer's unchanged extension list. */
		return VK_SUCCESS;
	}

	/* Measures the backend list before allocating its temporary storage. */
	total = 0;
	if (physical == VK_NULL_HANDLE) {
		error = compat_backend.instance_extensions(NULL, &total, NULL);
	} else {
		error = compat_backend.device_extensions(physical, NULL, &total, NULL);
	}

	/* Does not expose an unmeasured extension list on failure. */
	if (error != VK_SUCCESS)
		return error;

	/* Allocates one temporary list, including the valid empty-list case. */
	available = calloc((size_t)total + 1, sizeof(*available));
	if (available == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* Retrieves the actual backend properties. */
	if (physical == VK_NULL_HANDLE) {
		error = compat_backend.instance_extensions(NULL, &total, available);
	} else {
		error = compat_backend.device_extensions(physical, NULL, &total, available);
	}

	/* Releases temporary storage on failed or unstable backend enumeration. */
	if (error != VK_SUCCESS) {
		free(available);
		return error;
	}

	/* Keeps non-WSI entries in backend order within the caller's capacity. */
	capacity = 0;
	if (properties != NULL)
		capacity = *count;
	kept = 0;
	for (index = 0; index < total; index++) {
		/* Excludes backend surface, display and presentation ownership. */
		wsi = compat_wsi_extension(available[index].extensionName);
		if (wsi != 0)
			continue;

		/* Writes only slots the caller supplied. */
		if (properties != NULL && kept < capacity)
			properties[kept] = available[index];

		/* Counts retained entries independently of caller truncation. */
		kept++;
	}

	/* Releases the backend list after filtering. */
	free(available);

	/* Reports the complete count without an output array. */
	if (properties == NULL) {
		*count = kept;
		return VK_SUCCESS;
	}

	/* Reports the number actually written when the caller supplied too few slots. */
	if (kept > capacity) {
		*count = capacity;
		return VK_INCOMPLETE;
	}

	/* Succeeded: all retained extension properties fit the caller's array. */
	*count = kept;
	return VK_SUCCESS;
}

/* Populates immutable physical-device membership after successful instance creation. */
static VkResult
instance_physicals(
	struct compat_instance *instance)
{
	PFN_vkEnumeratePhysicalDevices enumerate;
	VkResult error;

	/* Resolves an unmodified instance procedure from the backend's own resolver. */
	enumerate = (PFN_vkEnumeratePhysicalDevices)compat_backend.get_instance_proc(instance->handle, "vkEnumeratePhysicalDevices");
	if (enumerate == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Measures the physical-device membership. */
	error = enumerate(instance->handle, &instance->physical_count, NULL);
	if (error != VK_SUCCESS)
		return error;

	/* Allocates membership storage even for a valid zero-device instance. */
	instance->physicals = calloc((size_t)instance->physical_count + 1, sizeof(*instance->physicals));
	if (instance->physicals == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* Fills the instance's ownership list before publishing it. */
	error = enumerate(instance->handle, &instance->physical_count, instance->physicals);
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: physical-device handles retain their instance owner. */
	return VK_SUCCESS;
}
