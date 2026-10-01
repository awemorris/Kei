/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The live backend devices and their queue ownership records.
 */

#include "compat.h"
#include <stdio.h>
#include <stdlib.h>

/* The live device records, published and retired under compat_mutex. */
static struct compat_device *compat_devices;

/* The queues retrieved from live devices, removed with their device under compat_mutex. */
static struct compat_queue *compat_queues;

static void device_queue_record(VkDevice device, VkQueue queue, uint32_t family);

/*
 * Creates a backend device while retaining its instance and physical-device ownership.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDevice(
	VkPhysicalDevice physicalDevice,
	const VkDeviceCreateInfo *pCreateInfo,
	const VkAllocationCallbacks *pAllocator,
	VkDevice *pDevice)
{
	struct compat_device *device;
	struct compat_instance *instance;
	PFN_vkGetPhysicalDeviceProperties get_properties;
	VkPhysicalDeviceProperties properties;
	VkResult error;
	uint32_t index;
	int wsi;

	/* Requires valid public creation inputs. */
	if (pCreateInfo == NULL || pDevice == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* The chain-only phase has no swapchain or presentation extension. */
	for (index = 0; index < pCreateInfo->enabledExtensionCount; index++) {
		/* Refuses an extension whose handles would belong to backend WSI. */
		wsi = compat_wsi_extension(pCreateInfo->ppEnabledExtensionNames[index]);
		if (wsi != 0)
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	/* Enters the device boundary through directly resolved backend calls. */
	compat_enter("vkCreateDevice");
	if (compat_backend.create_device == NULL) {
		/* Leaves a refused device creation without a backend entry point. */
		compat_leave();
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	/* Requires the still-live instance whose backend returned the physical device. */
	instance = compat_instance_for_physical(physicalDevice);
	if (instance == NULL) {
		/* Leaves an unknown physical device outside the bookkeeping boundary. */
		compat_leave();
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	/* Allocates ownership storage before creating a backend resource. */
	device = calloc(1, sizeof(*device));
	if (device == NULL) {
		/* Leaves a refused bookkeeping allocation. */
		compat_leave();
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	/* Queries the physical API version through an unmodified backend procedure. */
	get_properties = (PFN_vkGetPhysicalDeviceProperties)compat_backend.get_instance_proc(instance->handle, "vkGetPhysicalDeviceProperties");
	if (get_properties == NULL) {
		/* Releases the unused record before refusing an incomplete backend. */
		free(device);
		compat_leave();
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	/* Reads the device's capabilities without changing its backend handle. */
	get_properties(physicalDevice, &properties);

	/* Creates the device through its directly resolved interceptor entry point. */
	error = compat_backend.create_device(physicalDevice, pCreateInfo, pAllocator, pDevice);
	if (error != VK_SUCCESS) {
		/* Releases the unused record and the active call on backend failure. */
		free(device);
		compat_leave();
		return error;
	}

	/* Retains the effective API version and the device's unchanged backend handle. */
	device->handle = *pDevice;
	device->physical = physicalDevice;
	device->instance = instance;
	device->api_version = instance->api_version;
	if (properties.apiVersion < device->api_version)
		device->api_version = properties.apiVersion;

	/* Publishes the initialized device record under the ownership mutex. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Makes the initialized device discoverable by queue and presentation calls. */
	device->next = compat_devices;
	compat_devices = device;

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Leaves the completed device creation boundary. */
	compat_leave();

	/* Succeeded: the caller owns the backend device with retained ownership metadata. */
	return VK_SUCCESS;
}

/*
 * Retires device and queue ownership records before destroying the backend device.
 */
VKAPI_ATTR void VKAPI_CALL
vkDestroyDevice(
	VkDevice device,
	const VkAllocationCallbacks *pAllocator)
{
	struct compat_device **link;
	struct compat_device *record;
	struct compat_queue **queue_link;
	struct compat_queue *queue;

	/* Vulkan permits destruction of a null device. */
	if (device == VK_NULL_HANDLE)
		return;

	/* Enters the device destruction boundary. */
	compat_enter("vkDestroyDevice");

	/* Removes the device and its queues atomically from the live ownership lists. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Begins the retirement search without a matched ownership record. */
	record = NULL;

	/* Finds the device record to retire. */
	for (link = &compat_devices; *link != NULL; link = &(*link)->next) {
		/* Removes only the caller's device. */
		if ((*link)->handle == device) {
			record = *link;
			*link = record->next;
			break;
		}
	}

	/* Retires every cached queue whose parent device is being destroyed. */
	queue_link = &compat_queues;
	while (*queue_link != NULL) {
		/* Removes matching queues while preserving the next list link. */
		queue = *queue_link;
		if (queue->device == record) {
			*queue_link = queue->next;
			free(queue);
		} else {
			queue_link = &queue->next;
		}
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Requires the directly resolved backend destruction function. */
	if (compat_backend.destroy_device == NULL)
		compat_missing("vkDestroyDevice");

	/* Destroys the backend resource after its records are no longer discoverable. */
	compat_backend.destroy_device(device, pAllocator);

	/* Releases the retired parent ownership record. */
	free(record);

	/* Leaves the completed destruction boundary. */
	compat_leave();

	/* Succeeded: the device and every retrieved queue record are retired. */
	return;
}

/*
 * Lists physical-device extensions without backend presentation extensions.
 */
VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceExtensionProperties(
	VkPhysicalDevice physicalDevice,
	const char *pLayerName,
	uint32_t *pPropertyCount,
	VkExtensionProperties *pProperties)
{
	VkResult error;

	/* Requires the caller's enumeration count. */
	if (pPropertyCount == NULL)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Enters the physical-device extension enumeration boundary. */
	compat_enter("vkEnumerateDeviceExtensionProperties");

	/* Filters backend-owned WSI while preserving unrelated device extensions. */
	error = compat_extensions(physicalDevice, pLayerName, pPropertyCount, pProperties);

	/* Leaves the completed physical-device query. */
	compat_leave();

	/* Preserves a backend error or an incomplete caller array. */
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: the caller holds the available non-WSI device extensions. */
	return VK_SUCCESS;
}

/*
 * Retrieves a backend queue and retains its device ownership.
 */
VKAPI_ATTR void VKAPI_CALL
vkGetDeviceQueue(
	VkDevice device,
	uint32_t queueFamilyIndex,
	uint32_t queueIndex,
	VkQueue *pQueue)
{
	/* Enters the queue retrieval boundary through a directly resolved backend entry. */
	compat_enter("vkGetDeviceQueue");
	if (compat_backend.get_queue == NULL)
		compat_missing("vkGetDeviceQueue");

	/* Retrieves the unchanged backend queue handle. */
	compat_backend.get_queue(device, queueFamilyIndex, queueIndex, pQueue);

	/* Retains the queue's parent for later presentation calls. */
	device_queue_record(device, *pQueue, queueFamilyIndex);

	/* Leaves the completed queue retrieval boundary. */
	compat_leave();

	/* Succeeded: the caller holds the backend queue and its ownership is recorded. */
	return;
}

/*
 * Retrieves a Vulkan 1.1 queue and retains its device ownership.
 */
VKAPI_ATTR void VKAPI_CALL
vkGetDeviceQueue2(
	VkDevice device,
	const VkDeviceQueueInfo2 *pQueueInfo,
	VkQueue *pQueue)
{
	/* Enters the queue retrieval boundary through the backend's direct entry point. */
	compat_enter("vkGetDeviceQueue2");
	if (compat_backend.get_queue2 == NULL)
		compat_missing("vkGetDeviceQueue2");

	/* Preserves the backend's queue retrieval semantics. */
	compat_backend.get_queue2(device, pQueueInfo, pQueue);

	/* Records ownership for a queue successfully returned by the backend. */
	if (*pQueue != VK_NULL_HANDLE)
		device_queue_record(device, *pQueue, pQueueInfo->queueFamilyIndex);

	/* Leaves the completed queue retrieval boundary. */
	compat_leave();

	/* Succeeded: the backend queue result and ownership are preserved. */
	return;
}

/*
 * Finds a device record whose handle lifetime the caller already holds.
 */
struct compat_device *
compat_device_get(
	VkDevice handle)
{
	struct compat_device *device;

	/* Searches while device insertion and retirement are excluded. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Finds the caller's unchanged backend device handle. */
	for (device = compat_devices; device != NULL; device = device->next) {
		/* Stops at the requested live owner. */
		if (device->handle == handle)
			break;
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Succeeded: returns the live owner or NULL for an unknown device. */
	return device;
}

/*
 * Finds a queue record whose parent device lifetime the caller already holds.
 */
struct compat_queue *
compat_queue_get(
	VkQueue handle)
{
	struct compat_queue *queue;

	/* Searches while queue publication and parent-device retirement are excluded. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Finds the retrieved backend queue handle. */
	for (queue = compat_queues; queue != NULL; queue = queue->next) {
		/* Stops at the requested live queue. */
		if (queue->handle == handle)
			break;
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Succeeded: returns the queue owner or NULL when the queue was not retrieved. */
	return queue;
}

/* Records one retrieved queue without duplicating an existing handle's ownership. */
static void
device_queue_record(
	VkDevice handle,
	VkQueue queue_handle,
	uint32_t family)
{
	struct compat_device *device;
	struct compat_queue *queue;

	/* Resolves the live parent before publishing any queue metadata. */
	device = compat_device_get(handle);
	if (device == NULL)
		return;

	/* Finds or creates the single queue record under the ownership mutex. */
	(void)pthread_mutex_lock(&compat_mutex);

	/* Checks whether an earlier retrieval already recorded the queue. */
	for (queue = compat_queues; queue != NULL; queue = queue->next) {
		/* Stops at the existing queue record. */
		if (queue->handle == queue_handle)
			break;
	}

	/* Allocates a queue record only for a new retrieved handle. */
	if (queue == NULL) {
		/* Keeps queue ownership available to later presentation calls. */
		queue = calloc(1, sizeof(*queue));
		if (queue == NULL) {
			/* Releases the mutex before terminating an unreportable void-API allocation failure. */
			(void)pthread_mutex_unlock(&compat_mutex);
			(void)fputs("libvulkan-compat: no memory for queue ownership\n", stderr);
			abort();
		}

		/* Publishes one fully initialized queue ownership record. */
		queue->handle = queue_handle;
		queue->device = device;
		queue->family = family;
		queue->next = compat_queues;
		compat_queues = queue;
	}

	/* Ends list protection; callers retain the Vulkan handle lifetime themselves. */
	(void)pthread_mutex_unlock(&compat_mutex);

	/* Succeeded: the retrieved queue retains its parent device. */
	return;
}
