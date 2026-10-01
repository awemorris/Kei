/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The backend chain test: fills and copies one MiB through real Vulkan commands.
 */

#define _GNU_SOURCE
#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHAIN_BYTES (1024U * 1024U)

static VkResult chain_copy(VkPhysicalDevice physical, VkDevice device, VkQueue queue, uint32_t family);
static VkResult chain_buffer(VkDevice device, const VkPhysicalDeviceMemoryProperties *properties, VkBuffer *buffer, VkDeviceMemory *memory);

/*
 * Checks that the maintained Vulkan chain reaches lavapipe and preserves command results.
 */
int
main(
	void)
{
	Dl_info library;
	VkApplicationInfo application;
	VkInstanceCreateInfo create;
	VkDeviceQueueCreateInfo queue_create;
	VkDeviceCreateInfo device_create;
	VkPhysicalDevice physical;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceDriverProperties driver;
	VkPhysicalDeviceProperties2 properties2;
	PFN_vkGetPhysicalDeviceProperties2KHR get_properties2;
	VkQueueFamilyProperties families[32];
	VkExtensionProperties extensions[256];
	VkInstance instance;
	VkDevice device;
	VkQueue queue;
	PFN_vkVoidFunction procedure;
	const char *suffix;
	const char *instance_extension = "VK_KHR_get_physical_device_properties2";
	char *reenter;
	uint32_t count;
	uint32_t index;
	uint32_t family;
	float priority;
	VkResult error;
	int found;
	int differs;
	unsigned surface_found;
	unsigned wayland_found;
	size_t length;
	size_t suffix_length;

	/* The fake backend test needs to reach the interceptor even before ordinary capabilities exist. */
	reenter = getenv("VK_CHAIN_REENTER");
	if (reenter != NULL) {
		/* Calls the recursively interposed creation path rather than the normal workload. */
		memset(&create, 0, sizeof(create));
		create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		error = vkCreateInstance(&create, NULL, &instance);

		/* A fake backend must be caught by recursion detection rather than succeed. */
		(void)fprintf(stderr, "fake creation unexpectedly returned %d\n", error);
		return 1;
	}

	/* Confirms that DT_NEEDED resolves to the staged Keiland library. */
	found = dladdr((void *)vkGetInstanceProcAddr, &library);
	if (found == 0)
		return 1;

	/* Requires the test's own install tree rather than the system Vulkan library. */
	suffix = "/opt/keiland/lib/libvulkan.so.1";
	length = strlen(library.dli_fname);
	suffix_length = strlen(suffix);
	if (length < suffix_length)
		return 1;

	/* Compares the installed suffix after proving it fits. */
	differs = strcmp(library.dli_fname + length - suffix_length, suffix);
	if (differs != 0)
		return 1;

	/* Creates the Vulkan 1.0 instance used by Keiland applications. */
	memset(&application, 0, sizeof(application));
	application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application.apiVersion = VK_API_VERSION_1_0;

	/* Builds the instance request without a backend WSI extension. */
	memset(&create, 0, sizeof(create));
	create.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create.pApplicationInfo = &application;
	create.enabledExtensionCount = 1;
	create.ppEnabledExtensionNames = &instance_extension;

	/* Creates an instance through the compatibility library's public entry. */
	error = vkCreateInstance(&create, NULL, &instance);
	if (error != VK_SUCCESS)
		return 1;

	/* Requires that foreign X11 WSI names remain unavailable. */
	procedure = vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");
	if (procedure != NULL)
		return 1;

	/* Enumerates the filtered instance extensions. */
	count = 256;
	error = vkEnumerateInstanceExtensionProperties(NULL, &count, extensions);
	if (error != VK_SUCCESS)
		return 1;

	/* Counts our own advertised surface extensions alongside the denied X11 names. */
	surface_found = 0;
	wayland_found = 0;

	/* Rejects foreign platform surface extensions in the advertised list. */
	for (index = 0; index < count; index++) {
		/* Our public instance list includes the common surface extension. */
		differs = strcmp(extensions[index].extensionName, "VK_KHR_surface");
		if (differs == 0)
			surface_found = 1;

		/* Our Wayland surface extension is advertised independently of backend platform WSI. */
		differs = strcmp(extensions[index].extensionName, "VK_KHR_wayland_surface");
		if (differs == 0)
			wayland_found = 1;

		/* Checks XCB surface ownership. */
		differs = strcmp(extensions[index].extensionName, "VK_KHR_xcb_surface");
		if (differs == 0)
			return 1;

		/* Checks Xlib surface ownership. */
		differs = strcmp(extensions[index].extensionName, "VK_KHR_xlib_surface");
		if (differs == 0)
			return 1;
	}

	/* Both own surface extensions must be available to ordinary applications. */
	if (surface_found == 0)
		return 1;

	/* Wayland availability is required even with the host display environment unset. */
	if (wayland_found == 0)
		return 1;

	/* An instance that did not enable Wayland cannot query its owned creation procedure. */
	procedure = vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");
	if (procedure != NULL)
		return 1;

	/* Obtains one physical device from the backend instance. */
	count = 1;
	error = vkEnumeratePhysicalDevices(instance, &count, &physical);
	if (error != VK_SUCCESS || count != 1)
		return 1;

	/* Reports the physical backend identity. */
	vkGetPhysicalDeviceProperties(physical, &properties);
	(void)printf("chain deviceName=%s\n", properties.deviceName);

	/* Reads driver identity when the backend offers the extended property query. */
	get_properties2 = (PFN_vkGetPhysicalDeviceProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR");
	if (get_properties2 != NULL) {
		/* Initializes the requested driver property record. */
		memset(&driver, 0, sizeof(driver));
		driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;

		/* Chains driver identity to the physical-device property query. */
		memset(&properties2, 0, sizeof(properties2));
		properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		properties2.pNext = &driver;

		/* Reports the driver behind the compatibility boundary. */
		get_properties2(physical, &properties2);
		(void)printf("chain driverName=%s\n", driver.driverName);
	}

	/* Finds a backend queue family able to execute the transfer commands. */
	count = 32;
	vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, families);
	family = UINT32_MAX;
	for (index = 0; index < count; index++) {
		/* Selects a transfer-capable queue with an available queue slot. */
		if ((families[index].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 && families[index].queueCount != 0) {
			family = index;
			break;
		}
	}

	/* Refuses a backend without a usable transfer queue. */
	if (family == UINT32_MAX)
		return 1;

	/* Requests one queue from the selected family. */
	priority = 1.0f;
	memset(&queue_create, 0, sizeof(queue_create));
	queue_create.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create.queueFamilyIndex = family;
	queue_create.queueCount = 1;
	queue_create.pQueuePriorities = &priority;

	/* Creates a device without using backend swapchain functionality. */
	memset(&device_create, 0, sizeof(device_create));
	device_create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create.queueCreateInfoCount = 1;
	device_create.pQueueCreateInfos = &queue_create;

	/* Obtains the backend device through our interceptor. */
	error = vkCreateDevice(physical, &device_create, NULL, &device);
	if (error != VK_SUCCESS)
		return 1;

	/* Confirms that backend device procedures remain accessible. */
	procedure = vkGetDeviceProcAddr(device, "vkCmdFillBuffer");
	if (procedure == NULL)
		return 1;

	/* Retrieves a queue whose ownership the compatibility library records. */
	vkGetDeviceQueue(device, family, 0, &queue);

	/* Runs the real one-MiB fill/copy/fence workload. */
	error = chain_copy(physical, device, queue, family);

	/* Retires the device before its parent instance. */
	vkDestroyDevice(device, NULL);
	vkDestroyInstance(instance, NULL);

	/* Reports any command or byte-verification failure. */
	if (error != VK_SUCCESS)
		return 1;

	/* Publishes the verified chain result. */
	(void)puts("vk-chain-test: PASS");

	/* Succeeded: the Keiland library reached the backend and preserved every copied byte. */
	return 0;
}

/* Records and submits the fill/copy workload, then checks every host-visible word. */
static VkResult
chain_copy(
	VkPhysicalDevice physical,
	VkDevice device,
	VkQueue queue,
	uint32_t family)
{
	VkPhysicalDeviceMemoryProperties properties;
	VkBuffer source;
	VkBuffer destination;
	VkDeviceMemory source_memory;
	VkDeviceMemory destination_memory;
	VkCommandPoolCreateInfo pool_create;
	VkCommandBufferAllocateInfo allocate;
	VkCommandBufferBeginInfo begin;
	VkBufferMemoryBarrier barrier;
	VkBufferCopy copy;
	VkFenceCreateInfo fence_create;
	VkSubmitInfo submit;
	VkCommandPool pool;
	VkCommandBuffer command;
	VkFence fence;
	VkResult error;
	void *mapped;
	uint32_t *words;
	uint32_t index;

	/* Queries memory types used by both transfer buffers. */
	vkGetPhysicalDeviceMemoryProperties(physical, &properties);

	/* Creates the transfer source and checks its allocation immediately. */
	error = chain_buffer(device, &properties, &source, &source_memory);
	if (error != VK_SUCCESS)
		return error;

	/* Creates the host-visible destination independently. */
	error = chain_buffer(device, &properties, &destination, &destination_memory);
	if (error != VK_SUCCESS)
		return error;

	/* Creates a command pool for the selected transfer queue. */
	memset(&pool_create, 0, sizeof(pool_create));
	pool_create.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_create.queueFamilyIndex = family;
	error = vkCreateCommandPool(device, &pool_create, NULL, &pool);
	if (error != VK_SUCCESS)
		return error;

	/* Allocates one primary command buffer from that pool. */
	memset(&allocate, 0, sizeof(allocate));
	allocate.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate.commandPool = pool;
	allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate.commandBufferCount = 1;
	error = vkAllocateCommandBuffers(device, &allocate, &command);
	if (error != VK_SUCCESS)
		return error;

	/* Begins recording the transfer workload. */
	memset(&begin, 0, sizeof(begin));
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	error = vkBeginCommandBuffer(command, &begin);
	if (error != VK_SUCCESS)
		return error;

	/* Fills every source word with the chosen nonzero pattern. */
	vkCmdFillBuffer(command, source, 0, CHAIN_BYTES, 0x5a5a5a5aU);

	/* Makes fill writes visible to the following transfer reads. */
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = source;
	barrier.size = CHAIN_BYTES;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 1, &barrier, 0, NULL);

	/* Copies the completed source pattern into the independently allocated destination. */
	memset(&copy, 0, sizeof(copy));
	copy.size = CHAIN_BYTES;
	vkCmdCopyBuffer(command, source, destination, 1, &copy);

	/* Makes destination transfer writes visible to host reads after fence completion. */
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	barrier.buffer = destination;
	vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, NULL, 1, &barrier, 0, NULL);

	/* Finishes recording before submitting any work. */
	error = vkEndCommandBuffer(command);
	if (error != VK_SUCCESS)
		return error;

	/* Creates the submission-completion fence. */
	memset(&fence_create, 0, sizeof(fence_create));
	fence_create.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	error = vkCreateFence(device, &fence_create, NULL, &fence);
	if (error != VK_SUCCESS)
		return error;

	/* Submits the recorded workload to the retrieved backend queue. */
	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &command;
	error = vkQueueSubmit(queue, 1, &submit, fence);
	if (error != VK_SUCCESS)
		return error;

	/* Bounds the real completion wait independently of the outer command timeout. */
	error = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_C(10000000000));
	if (error != VK_SUCCESS)
		return error;

	/* Maps the host-coherent destination after the completion fence. */
	error = vkMapMemory(device, destination_memory, 0, CHAIN_BYTES, 0, &mapped);
	if (error != VK_SUCCESS)
		return error;

	/* Verifies every copied word rather than sampling a single byte. */
	words = mapped;
	error = VK_SUCCESS;
	for (index = 0; index < CHAIN_BYTES / sizeof(*words); index++) {
		/* Records any mismatch before releasing the mapped memory. */
		if (words[index] != 0x5a5a5a5aU) {
			error = VK_ERROR_UNKNOWN;
			break;
		}
	}

	/* Retires the mapping and completion resources after all host reads. */
	vkUnmapMemory(device, destination_memory);
	vkDestroyFence(device, fence, NULL);
	vkDestroyCommandPool(device, pool, NULL);

	/* Releases the source buffer before its backing memory. */
	vkDestroyBuffer(device, source, NULL);
	vkFreeMemory(device, source_memory, NULL);

	/* Releases the destination buffer before its backing memory. */
	vkDestroyBuffer(device, destination, NULL);
	vkFreeMemory(device, destination_memory, NULL);

	/* Reports a byte mismatch separately from successful GPU completion. */
	if (error != VK_SUCCESS)
		return error;

	/* Succeeded: all one-MiB transfer words match the submitted pattern. */
	return VK_SUCCESS;
}

/* Allocates one transfer buffer with host-visible, host-coherent backing memory. */
static VkResult
chain_buffer(
	VkDevice device,
	const VkPhysicalDeviceMemoryProperties *properties,
	VkBuffer *buffer,
	VkDeviceMemory *memory)
{
	VkBufferCreateInfo create;
	VkMemoryRequirements requirements;
	VkMemoryAllocateInfo allocate;
	VkResult error;
	uint32_t index;
	uint32_t type;
	VkMemoryPropertyFlags flags;

	/* Creates a buffer usable as either transfer source or destination. */
	memset(&create, 0, sizeof(create));
	create.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create.size = CHAIN_BYTES;
	create.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	error = vkCreateBuffer(device, &create, NULL, buffer);
	if (error != VK_SUCCESS)
		return error;

	/* Queries backing-memory size, alignment and acceptable memory types. */
	vkGetBufferMemoryRequirements(device, *buffer, &requirements);

	/* Selects host-visible coherent memory accepted by this specific buffer. */
	type = UINT32_MAX;
	for (index = 0; index < properties->memoryTypeCount; index++) {
		/* Skips memory types outside the buffer's supported bit mask. */
		if ((requirements.memoryTypeBits & (1U << index)) == 0)
			continue;

		/* Requires both host visibility and coherence for a direct post-fence read. */
		flags = properties->memoryTypes[index].propertyFlags;
		if ((flags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			type = index;
			break;
		}
	}

	/* Releases the unbound buffer when no suitable memory type exists. */
	if (type == UINT32_MAX) {
		vkDestroyBuffer(device, *buffer, NULL);
		return VK_ERROR_FEATURE_NOT_PRESENT;
	}

	/* Allocates backing storage for this one buffer. */
	memset(&allocate, 0, sizeof(allocate));
	allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = type;
	error = vkAllocateMemory(device, &allocate, NULL, memory);
	if (error != VK_SUCCESS) {
		/* Releases the unbound buffer after a failed memory allocation. */
		vkDestroyBuffer(device, *buffer, NULL);
		return error;
	}

	/* Binds the allocated memory through the forwarded backend call. */
	error = vkBindBufferMemory(device, *buffer, *memory, 0);
	if (error != VK_SUCCESS) {
		/* Releases the unbound buffer and its memory after a failed binding. */
		vkDestroyBuffer(device, *buffer, NULL);
		vkFreeMemory(device, *memory, NULL);
		return error;
	}

	/* Succeeded: the caller owns the transfer buffer and its backing allocation. */
	return VK_SUCCESS;
}
