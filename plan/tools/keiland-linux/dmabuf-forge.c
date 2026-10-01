/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Exports a real small dma-buf, then forges its height on the standard
 * Wayland wire to verify rejection before the compositor calls Vulkan.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include "userland/desktop/libvulkan-compat/linux-dmabuf-v1-client-protocol.h"

/* One bounded probe owns its Vulkan allocation and the independent Wayland connection. */
struct forge_probe {
	VkInstance instance;
	VkPhysicalDevice physical;
	VkDevice device;
	VkImage image;
	VkDeviceMemory memory;
	int fd;
	uint32_t offset;
	uint32_t stride;
	struct wl_display *display;
	struct wl_registry *registry;
	struct zwp_linux_dmabuf_v1 *dmabuf;
	struct zwp_linux_buffer_params_v1 *params;
	struct wl_buffer *buffer;
};

/* The private params constructor produces one protocol object on the caller's queue. */
static const struct wl_interface *factory_params_types[] = {&zwp_linux_buffer_params_v1_interface};

/* Immediate construction supplies the standard buffer identity and four plain scalars. */
static const struct wl_interface *params_buffer_types[] = {&wl_buffer_interface, NULL, NULL, NULL, NULL};

/* An asynchronous created event carries a standard server-allocated buffer identity. */
static const struct wl_interface *params_created_types[] = {&wl_buffer_interface};

/* The probe binds revision three and exposes only its two factory requests. */
static const struct wl_message factory_requests[] = {
    {"destroy", "", NULL},
    {"create_params", "n", factory_params_types}};

/* The factory's two discovery events retain the canonical revision-three wire layout. */
static const struct wl_message factory_events[] = {
    {"format", "u", NULL},
    {"modifier", "3uuu", NULL}};

/* The four parameter methods remain byte-for-byte compatible with the standard XML. */
static const struct wl_message params_requests[] = {
    {"destroy", "", NULL},
    {"add", "huuuuu", NULL},
    {"create", "iiuu", NULL},
    {"create_immed", "2niiuu", params_buffer_types}};

/* The event table permits the display's fatal error to be dispatched normally. */
static const struct wl_message params_events[] = {
    {"created", "n", params_created_types},
    {"failed", "", NULL}};

/* The executable owns this private protocol identity without adding it to libwayland. */
const struct wl_interface zwp_linux_dmabuf_v1_interface = {
    "zwp_linux_dmabuf_v1", 3, 2, factory_requests, 2, factory_events};

/* Each params proxy describes exactly one temporary batch. */
const struct wl_interface zwp_linux_buffer_params_v1_interface = {
    "zwp_linux_buffer_params_v1", 3, 4, params_requests, 2, params_events};

static VkResult forge_image(struct forge_probe *probe);
static VkResult forge_memory(struct forge_probe *probe);
static int forge_wire(struct forge_probe *probe);
static void forge_cleanup(struct forge_probe *probe);
static void forge_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void forge_global_remove(void *data, struct wl_registry *registry, uint32_t name);

/*
 * Verifies that a forged height cannot exceed the real dma-buf allocation.
 */
int
main(
	void)
{
	struct forge_probe probe;
	VkResult status;
	int error;

	/* Initializes partial-cleanup ownership before either subsystem is opened. */
	memset(&probe, 0, sizeof(probe));
	probe.fd = -1;

	/* Creates and exports the real, small allocation used by the forged request. */
	status = forge_image(&probe);
	if (status != VK_SUCCESS) {
		fprintf(stderr, "dmabuf-forge: FAIL Vulkan result=%d\n", status);
		forge_cleanup(&probe);
		return 1;
	}

	/* Sends the invalid layout and reads the compositor's exact protocol error. */
	error = forge_wire(&probe);
	if (error != 0) {
		fprintf(stderr, "dmabuf-forge: FAIL wire errno=%d\n", error);
		forge_cleanup(&probe);
		return 1;
	}

	/* Retires all independently owned resources after the server's rejection. */
	forge_cleanup(&probe);
	printf("dmabuf-forge: PASS\n");

	/* Succeeded: a real descriptor with an oversized description was refused. */
	return 0;
}

/* Creates a single-plane modifier image through the same frontend used by real clients. */
static VkResult
forge_image(
	struct forge_probe *probe)
{
	const char *extensions[] = {
	    VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
	    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
	    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME};
	VkApplicationInfo application;
	VkInstanceCreateInfo instance;
	VkPhysicalDevice devices[8];
	VkDeviceQueueCreateInfo queue;
	VkDeviceCreateInfo device;
	VkImageDrmFormatModifierListCreateInfoEXT modifier;
	VkExternalMemoryImageCreateInfo external;
	VkImageCreateInfo image;
	VkImageSubresource subresource;
	VkSubresourceLayout layout;
	VkMemoryGetFdInfoKHR export_info;
	PFN_vkGetMemoryFdKHR export_fd;
	VkResult status;
	uint64_t linear;
	uint32_t count;
	float priority;
	int error;

	/* Uses promoted core dependencies while retaining explicit DMA-BUF and modifier extensions. */
	memset(&application, 0, sizeof(application));
	application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application.apiVersion = VK_API_VERSION_1_2;

	/* No display surface or WSI extension is needed to create the exported probe image. */
	memset(&instance, 0, sizeof(instance));
	instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance.pApplicationInfo = &application;
	status = vkCreateInstance(&instance, NULL, &probe->instance);
	if (status != VK_SUCCESS)
		return status;

	/* Selects the guest's first physical device from a bounded enumeration. */
	count = 8U;
	status = vkEnumeratePhysicalDevices(probe->instance, &count, devices);
	if (status != VK_SUCCESS)
		return status;

	/* A successful empty enumeration cannot supply an image allocator. */
	if (count == 0U)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* The guest's lavapipe graphics queue is family zero. */
	probe->physical = devices[0];
	priority = 1.0f;
	memset(&queue, 0, sizeof(queue));
	queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue.queueCount = 1U;
	queue.pQueuePriorities = &priority;

	/* Enables precisely the three external-image features exercised by the probe. */
	memset(&device, 0, sizeof(device));
	device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device.queueCreateInfoCount = 1U;
	device.pQueueCreateInfos = &queue;
	device.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
	device.ppEnabledExtensionNames = extensions;
	status = vkCreateDevice(probe->physical, &device, NULL, &probe->device);
	if (status != VK_SUCCESS)
		return status;

	/* Requests the guest's supported single-plane LINEAR modifier. */
	linear = 0U;
	memset(&modifier, 0, sizeof(modifier));
	modifier.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT;
	modifier.drmFormatModifierCount = 1U;
	modifier.pDrmFormatModifiers = &linear;

	/* Declares exportable external memory for the real image. */
	memset(&external, 0, sizeof(external));
	external.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	external.pNext = &modifier;
	external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

	/* Keeps the actual image small while using a real sampled RGB allocation. */
	memset(&image, 0, sizeof(image));
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.pNext = &external;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = VK_FORMAT_B8G8R8A8_UNORM;
	image.extent.width = 64U;
	image.extent.height = 64U;
	image.extent.depth = 1U;
	image.mipLevels = 1U;
	image.arrayLayers = 1U;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
	image.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	status = vkCreateImage(probe->device, &image, NULL, &probe->image);
	if (status != VK_SUCCESS)
		return status;

	/* Gives the image a dedicated, exportable allocation of its own required size. */
	status = forge_memory(probe);
	if (status != VK_SUCCESS)
		return status;

	/* The modifier's memory-plane layout supplies the genuine offset and stride. */
	memset(&subresource, 0, sizeof(subresource));
	subresource.aspectMask = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
	vkGetImageSubresourceLayout(probe->device, probe->image, &subresource, &layout);

	/* The protocol has only unsigned 32-bit offset and stride fields. */
	if (layout.offset > UINT32_MAX || layout.rowPitch > UINT32_MAX)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Retains the real layout while the request later forges only its height. */
	probe->offset = (uint32_t)layout.offset;
	probe->stride = (uint32_t)layout.rowPitch;
	export_fd = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(probe->device, "vkGetMemoryFdKHR");
	if (export_fd == NULL)
		return VK_ERROR_EXTENSION_NOT_PRESENT;

	/* Exports the actual kernel DMA-BUF object backing the image allocation. */
	memset(&export_info, 0, sizeof(export_info));
	export_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
	export_info.memory = probe->memory;
	export_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
	status = export_fd(probe->device, &export_info, &probe->fd);
	if (status != VK_SUCCESS)
		return status;

	/* Keeps the process-owned exported descriptor out of later executable children. */
	error = fcntl(probe->fd, F_SETFD, FD_CLOEXEC);
	if (error < 0)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Succeeded: the probe owns a real 64-by-64 DMA-BUF and its native plane layout. */
	return VK_SUCCESS;
}

/* Allocates exportable memory dedicated to the probe's actual image. */
static VkResult
forge_memory(
	struct forge_probe *probe)
{
	VkMemoryRequirements requirements;
	VkMemoryDedicatedAllocateInfo dedicated;
	VkExportMemoryAllocateInfo exported;
	VkMemoryAllocateInfo allocate;
	VkResult status;
	uint32_t bits;
	uint32_t memory_type;

	/* Selects the lowest memory type allowed by the real image. */
	vkGetImageMemoryRequirements(probe->device, probe->image, &requirements);
	bits = requirements.memoryTypeBits;
	if (bits == 0U)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Finds the first compatible type without assuming any host-visible property. */
	memory_type = 0U;
	while ((bits & 1U) == 0U) {
		bits >>= 1;
		memory_type++;
	}

	/* The exported allocation belongs to this one immutable image. */
	memset(&dedicated, 0, sizeof(dedicated));
	dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	dedicated.image = probe->image;

	/* Exposes a kernel DMA-BUF handle rather than an opaque Vulkan-private file. */
	memset(&exported, 0, sizeof(exported));
	exported.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
	exported.pNext = &dedicated;
	exported.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

	/* Uses the image's Vulkan requirement size for the dedicated export allocation. */
	memset(&allocate, 0, sizeof(allocate));
	allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate.pNext = &exported;
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = memory_type;
	status = vkAllocateMemory(probe->device, &allocate, NULL, &probe->memory);
	if (status != VK_SUCCESS)
		return status;

	/* Binds the real allocation before exporting its memory descriptor. */
	status = vkBindImageMemory(probe->device, probe->image, probe->memory, 0U);
	if (status != VK_SUCCESS)
		return status;

	/* Succeeded: the exported allocation is dedicated and bound to the small image. */
	return VK_SUCCESS;
}

/* Sends the forged immediate constructor and checks its exact fatal error code. */
static int
forge_wire(
	struct forge_probe *probe)
{
	static const struct wl_registry_listener listener = {forge_global, forge_global_remove};
	union wl_argument arguments[6];
	const struct wl_interface *interface;
	struct wl_proxy *proxy;
	uint32_t object;
	uint32_t code;
	uint32_t params_id;
	int error;
	int same;

	/* Opens a separate client process connection to the running compositor. */
	probe->display = wl_display_connect(NULL);
	if (probe->display == NULL)
		return ECONNREFUSED;

	/* Obtains the registry before installing its bounded discovery listener. */
	probe->registry = wl_display_get_registry(probe->display);
	if (probe->registry == NULL)
		return ENOMEM;

	/* The registry listener binds only a standard revision-three DMA-BUF factory. */
	error = wl_registry_add_listener(probe->registry, &listener, probe);
	if (error != 0)
		return EIO;

	/* Receives registry globals and queues the factory binding. */
	error = wl_display_roundtrip(probe->display);
	if (error < 0)
		return EIO;

	/* The compositor must actually offer the Linux protocol under test. */
	if (probe->dmabuf == NULL)
		return ENOTSUP;

	/* Receives the bound factory's initial format events before creating params. */
	error = wl_display_roundtrip(probe->display);
	if (error < 0)
		return EIO;

	/* Constructs one fresh params proxy using the private canonical wire table. */
	memset(arguments, 0, sizeof(arguments));
	proxy = wl_proxy_marshal_array_flags((struct wl_proxy *)probe->dmabuf, 1U, &zwp_linux_buffer_params_v1_interface, 3U, 0U, arguments);
	if (proxy == NULL)
		return ENOMEM;

	/* Retains the exact offending identity before the protocol error retires the connection. */
	probe->params = (struct zwp_linux_buffer_params_v1 *)proxy;
	params_id = wl_proxy_get_id(proxy);
	arguments[0].h = probe->fd;
	arguments[1].u = 0U;
	arguments[2].u = probe->offset;
	arguments[3].u = probe->stride;
	arguments[4].u = 0U;
	arguments[5].u = 0U;
	(void)wl_proxy_marshal_array_flags(proxy, 1U, NULL, 3U, 0U, arguments);

	/* Forges only the height so the declared rows exceed the kernel's actual allocation. */
	arguments[0].n = 0U;
	arguments[1].i = 64;
	arguments[2].i = 4096;
	arguments[3].u = 0x34325241U;
	arguments[4].u = 0U;
	proxy = wl_proxy_marshal_array_flags(proxy, 3U, &wl_buffer_interface, 1U, 0U, arguments);
	if (proxy == NULL)
		return ENOMEM;

	/* The fatal error must arrive instead of a successful constructor roundtrip. */
	probe->buffer = (struct wl_buffer *)proxy;
	error = wl_display_roundtrip(probe->display);
	if (error >= 0)
		return EINVAL;

	/* Reads the standard Wayland error's object, interface and numeric params code. */
	interface = NULL;
	object = 0U;
	code = wl_display_get_protocol_error(probe->display, &interface, &object);
	if (code != 6U ||
	    object != params_id ||
	    interface == NULL)
		return EPROTO;

	/* The numeric code belongs to the params interface rather than an unrelated global. */
	same = strcmp(interface->name, "zwp_linux_buffer_params_v1");
	if (same != 0)
		return EPROTO;

	/* Succeeded: bounds rejection identifies the exact forged params batch. */
	printf("dmabuf-forge: error=out_of_bounds code=%u object=%u stride=%u\n", code, object, probe->stride);
	return 0;
}

/* Retires the rejected Wayland connection before the real exported allocation. */
static void
forge_cleanup(
	struct forge_probe *probe)
{
	/* Disconnecting frees all proxies without sending further requests after a fatal error. */
	if (probe->display != NULL)
		wl_display_disconnect(probe->display);

	/* The exported descriptor remains process-owned after Wayland has sent its duplicate. */
	if (probe->fd >= 0)
		(void)close(probe->fd);

	/* The image retires before the dedicated memory allocation bound to it. */
	if (probe->image != VK_NULL_HANDLE)
		vkDestroyImage(probe->device, probe->image, NULL);

	/* A failed setup may have no allocation to retire. */
	if (probe->memory != VK_NULL_HANDLE)
		vkFreeMemory(probe->device, probe->memory, NULL);

	/* Device teardown follows the last image and memory use. */
	if (probe->device != VK_NULL_HANDLE)
		vkDestroyDevice(probe->device, NULL);

	/* Instance teardown follows its only device. */
	if (probe->instance != VK_NULL_HANDLE)
		vkDestroyInstance(probe->instance, NULL);

	/* Succeeded: partial and complete probe setups retain no resource. */
	return;
}

/* Binds only the revision-three Linux DMA-BUF protocol offered by this server. */
static void
forge_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
	struct forge_probe *probe;
	int same;

	/* Other registry globals are irrelevant to this surface-free validation probe. */
	probe = data;
	same = strcmp(interface, "zwp_linux_dmabuf_v1");
	if (same != 0 ||
	    version < 3U ||
	    probe->dmabuf != NULL)
		return;

	/* Uses our private interface table and our own libwayland connection. */
	probe->dmabuf = wl_registry_bind(registry, name, &zwp_linux_dmabuf_v1_interface, 3U);

	/* Succeeded: later setup checks distinguish an absent factory from allocation failure. */
	return;
}

/* Keeps registry removal irrelevant to this finite, immediately connected probe. */
static void
forge_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t name)
{
	/* The probe owns no long-lived global resources beyond its one connection. */
	(void)data;
	(void)registry;
	(void)name;

	/* Succeeded: connection cleanup remains the sole owner of bound proxy retirement. */
	return;
}
