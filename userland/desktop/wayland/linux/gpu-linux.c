/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Receives one-plane Linux dma-bufs and passes their Vulkan images and
 * implicit acquire fences to the compositor's common ownership machinery.
 */
#include "../compose.h"
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Bounds physical-device enumeration independently of untrusted client data. */
#define GPU_ENUM_MAX 256U
#define GPU_ARGB8888 0x34325241U
#define GPU_XRGB8888 0x34325258U

/* The fatal parameter errors have their protocol-defined numeric identities. */
enum gpu_error {
	GPU_ALREADY_USED,
	GPU_PLANE_IDX,
	GPU_PLANE_SET,
	GPU_INCOMPLETE,
	GPU_INVALID_FORMAT,
	GPU_INVALID_DIMENSIONS,
	GPU_OUT_OF_BOUNDS,
	GPU_INVALID_BUFFER
};

/* One params batch or imported buffer owns its plane fd until final object retirement. */
struct gpu_buffer {
	int fd;
	unsigned used;
	uint32_t offset;
	uint32_t stride;
	uint64_t modifier;
};

/* One compositor process has one Vulkan device; extension selection precedes the first binding. */
static unsigned gpu_dma_available;
static unsigned gpu_modifier_available;

/* The sampled, importable modifier set is cached once for the process's selected physical device. */
static VkPhysicalDevice gpu_physical;
static uint64_t gpu_modifiers[GPU_ENUM_MAX];
static uint32_t gpu_modifier_count;
static unsigned gpu_queried;

static uint32_t gpu_word(const unsigned char *bytes, size_t offset);
static int gpu_formats(struct zwl_compose *compose);
static int gpu_modifier_importable(struct zwl_compose *compose, uint64_t modifier);
static int gpu_modifier_known(uint64_t modifier);
static int gpu_factory_request(struct zwl_object *factory, uint32_t opcode, const unsigned char *bytes, size_t size);
static int gpu_params_add(struct zwl_object *params, const unsigned char *bytes, size_t size);
static int gpu_params_create(struct zwl_object *params, uint32_t opcode, const unsigned char *bytes, size_t size);
static int gpu_params_validate(struct zwl_object *params, uint32_t width, uint32_t height, uint32_t format, uint64_t *allocation_bytes);
static int gpu_protocol_error(struct zwl_object *params, uint32_t code, const char *reason);
static int gpu_create_buffer(struct zwl_object *params, uint32_t opcode, uint32_t id, uint32_t width, uint32_t height, uint32_t format, uint64_t allocation_bytes);
static VkResult gpu_image(struct zwl_object *buffer, const struct gpu_buffer *plane, uint32_t width, uint32_t height);
static VkResult gpu_image_memory(struct zwl_compose *compose, const struct gpu_buffer *plane, VkImage image, VkDeviceMemory *memory);
static void gpu_image_release(struct zwl_compose *compose, VkImage image, VkDeviceMemory memory);

/*
 * Names the standard Linux dma-buf factory.
 */
const char *
zwl_gpu_global_interface(
	void)
{
	/* Succeeded: Linux clients share the standard dma-buf protocol. */
	return "zwp_linux_dmabuf_v1";
}

/*
 * Reports the supported format-and-modifier protocol revision.
 */
uint32_t
zwl_gpu_global_version(
	void)
{
	/* Succeeded: revision three supplies modifier events without feedback objects. */
	return 3U;
}

/*
 * Sends a new factory binding its importable formats and modifiers.
 */
int
zwl_gpu_bind(
	struct zwl_object *factory)
{
	uint32_t formats[2] = {GPU_ARGB8888, GPU_XRGB8888};
	uint32_t words[3];
	uint32_t format_index;
	uint32_t modifier_index;
	int error;

	/* Resolves the compositor device's sampled import capabilities once. */
	error = gpu_formats(factory->client->server->compose);
	if (error != 0)
		return error;

	/* A display-only backend has no buffer format to advertise. */
	if (gpu_modifier_count == 0U)
		return 0;

	/* Publishes both alpha interpretations of the same byte format. */
	for (format_index = 0; format_index < 2U; format_index++) {
		/* Versions one through three receive the legacy format event. */
		error = zwl_emit(factory->client, factory->id, 0U, &formats[format_index], sizeof(uint32_t));
		if (error != 0)
			return error;

		/* Modifier events belong only to revision three of this factory. */
		if (factory->version < 3U)
			continue;

		/* Each advertised modifier is independently sampled and importable. */
		for (modifier_index = 0; modifier_index < gpu_modifier_count; modifier_index++) {
			/* Encodes the opaque modifier without narrowing its high half. */
			words[0] = formats[format_index];
			words[1] = (uint32_t)(gpu_modifiers[modifier_index] >> 32);
			words[2] = (uint32_t)gpu_modifiers[modifier_index];
			error = zwl_emit(factory->client, factory->id, 1U, words, sizeof(words));
			if (error != 0)
				return error;
		}
	}

	/* Succeeded: discovery is complete before the client's following roundtrip. */
	return 0;
}

/*
 * Handles factory constructors and one-use dma-buf parameter batches.
 */
int
zwl_gpu_request(
	struct zwl_object *object,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	int error;

	/* The factory creates params but owns no plane itself. */
	if (object->kind == ZWL_FACTORY) {
		error = gpu_factory_request(object, opcode, bytes, size);
		if (error != 0)
			return error;

		/* Succeeded: the factory request has consumed its complete payload. */
		return 0;
	}

	/* The common dispatcher must not route unrelated objects into this module. */
	if (object->kind != ZWL_GPU_OBJECT)
		return EPROTO;

	/* A destructor is valid before or after a batch has been consumed. */
	if (opcode == 0U && size == 0U) {
		zwl_object_destroy(object);
		return 0;
	}

	/* Dispatches only the parameter methods defined by the negotiated version. */
	if (opcode == 1U) {
		error = gpu_params_add(object, bytes, size);
	} else if (opcode == 2U || (opcode == 3U && object->version >= 2U)) {
		error = gpu_params_create(object, opcode, bytes, size);
	} else {
		/* An unknown opcode has no descriptor or child ownership to consume. */
		return EPROTO;
	}

	/* Preserves a pending fd or a client-ending protocol/import error. */
	if (error != 0)
		return error;

	/* Succeeded: the requested batch operation has finished. */
	return 0;
}

/*
 * Releases a parameter or buffer's retained plane descriptor.
 */
void
zwl_gpu_object_free(
	struct zwl_object *object)
{
	struct gpu_buffer *plane;

	/* Shared-memory and other common objects own no Linux GPU record. */
	plane = object->gpu_private;
	if (plane == NULL)
		return;

	/* Final object retirement follows the last compositor frame that borrowed its image. */
	if (plane->fd >= 0)
		(void)close(plane->fd);

	/* Removes the OS-owned record after its plane descriptor has retired. */
	free(plane);
	object->gpu_private = NULL;

	/* Succeeded: this object retains no Linux buffer ownership. */
	return;
}

/*
 * Attaches the client's implicit write fence to the surface's next commit.
 */
void
zwl_gpu_commit(
	struct zwl_object *surface,
	struct zwl_object *buffer)
{
	struct gpu_buffer *plane;
	struct dma_buf_export_sync_file fence;
	int error;
	int saved_error;

	/* Shared-memory commits do not carry an external reservation object. */
	if (buffer == NULL)
		return;

	/* Only imported dma-bufs have a retained plane descriptor. */
	plane = buffer->gpu_private;
	if (plane == NULL)
		return;

	/* A full pending-fence array cannot silently discard a client's acquire fence. */
	if (surface->acquire_count >= ZWL_FENCE_MAX) {
		(void)zwl_error(surface->client, surface->id, "too many acquire fences");
		return;
	}

	/* Exports only the writers that must finish before the compositor samples the image. */
	memset(&fence, 0, sizeof(fence));
	fence.flags = DMA_BUF_SYNC_READ;
	fence.fd = -1;
	error = ioctl(plane->fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &fence);
	if (error != 0) {
		/* Older kernels rely on the client's completed CPU-wait presentation path. */
		saved_error = errno;
		if (saved_error == ENOTTY)
			return;

		/* A failed reservation inquiry ends this client before further GPU work. */
		printf("ZWL IMPORT_ERROR client=%" PRIu64 " errno=%d\n", (uint64_t)surface->client->number, saved_error);
		(void)zwl_error(surface->client, surface->id, "cannot export acquire fence");
		return;
	}

	/* Marks the exported descriptor close-on-exec before transferring it into common state. */
	error = fcntl(fence.fd, F_SETFD, FD_CLOEXEC);
	if (error < 0) {
		(void)close(fence.fd);
		(void)zwl_error(surface->client, surface->id, "cannot retain acquire fence");
		return;
	}

	/* Each sync_file is a fresh payload at generation one, owned by common commit cleanup. */
	surface->acquire[surface->acquire_count].fd = fence.fd;
	surface->acquire[surface->acquire_count].generation = 1U;
	surface->acquire_count++;

	/* Per-frame diagnostics expose the fence transfer without changing default behavior. */
	if (surface->client->server->log_frames)
		printf("ZWL ACQUIRE_FENCE client=%" PRIu64 " surface=%u generation=1\n", (uint64_t)surface->client->number, surface->id);

	/* Succeeded: common commit polling now owns the client's acquire fence. */
	return;
}

/*
 * Supplies the Linux display-acquisition instance extensions.
 */
uint32_t
zwl_gpu_instance_extensions(
	const char **names,
	uint32_t capacity)
{
	/* Copies only entries inside the caller's bounded extension array. */
	if (capacity > 0U)
		names[0] = VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME;

	/* DRM acquisition pairs the compositor's seat fd with Vulkan's own KMS backend. */
	if (capacity > 1U)
		names[1] = VK_EXT_ACQUIRE_DRM_DISPLAY_EXTENSION_NAME;

	/* Succeeded: two names are required even when the supplied array is too small. */
	return 2U;
}

/*
 * Supplies only available dma-buf and modifier device extensions.
 */
uint32_t
zwl_gpu_device_extensions(
	VkPhysicalDevice physical,
	const char **names,
	uint32_t capacity)
{
	static const char *wanted[] = {
	    VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
	    VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
	    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
	    VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
	    VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
	    VK_KHR_MAINTENANCE_1_EXTENSION_NAME};
	VkExtensionProperties extensions[GPU_ENUM_MAX];
	VkResult status;
	uint32_t count;
	uint32_t needed;
	uint32_t request_index;
	uint32_t extension_index;
	int same;

	/* Queries the selected physical device without assuming promoted core features on API 1.0. */
	count = GPU_ENUM_MAX;
	status = vkEnumerateDeviceExtensionProperties(physical, NULL, &count, extensions);
	if (status != VK_SUCCESS)
		return capacity + 1U;

	/* Resets capability discovery before a new compositor device is created. */
	gpu_dma_available = 0U;
	gpu_modifier_available = 0U;
	gpu_queried = 0U;
	gpu_modifier_count = 0U;
	gpu_physical = physical;

	/* Appends each required extension only when the physical device actually supplies it. */
	needed = 0U;
	for (request_index = 0U; request_index < sizeof(wanted) / sizeof(wanted[0]); request_index++) {
		/* Searches the measured, bounded physical-device extension list. */
		for (extension_index = 0U; extension_index < count; extension_index++) {
			/* Matches exact extension names rather than their related dependency prefixes. */
			same = strcmp(wanted[request_index], extensions[extension_index].extensionName);
			if (same != 0)
				continue;

			/* The caller still learns the complete required count when its array is short. */
			if (needed < capacity)
				names[needed] = wanted[request_index];

			/* DMA-BUF support enables imports; modifier support selects explicit layouts. */
			if (request_index == 0U)
				gpu_dma_available = 1U;

			/* Modifier queries are valid only after their extension has been enabled. */
			if (request_index == 1U)
				gpu_modifier_available = 1U;

			/* This unique extension occupies one output slot and needs no further matching. */
			needed++;
			break;
		}
	}

	/* Succeeded: the common compositor can enable the returned extension subset. */
	return needed;
}

/*
 * Selects ordinary Vulkan status polling for compositor frame completion.
 */
VkExternalFenceHandleTypeFlagBits
zwl_gpu_frame_fence_type(
	void)
{
	/* Succeeded: Linux frame fences remain intact for common status polling. */
	return 0;
}

/* Decodes one aligned wire word without relying on the payload's alignment. */
static uint32_t
gpu_word(
	const unsigned char *bytes,
	size_t offset)
{
	uint32_t word;

	/* Copies the canonical native-endian Wayland word into aligned storage. */
	memcpy(&word, bytes + offset, sizeof(word));

	/* Succeeded: the decoded scalar carries no unchecked pointer interpretation. */
	return word;
}

/* Caches single-plane sampled layouts that can import the selected device's dma-bufs. */
static int
gpu_formats(
	struct zwl_compose *compose)
{
	PFN_vkGetPhysicalDeviceFormatProperties2KHR query;
	VkDrmFormatModifierPropertiesEXT modifiers[GPU_ENUM_MAX];
	VkDrmFormatModifierPropertiesListEXT list;
	VkFormatProperties2 properties;
	uint32_t count;
	uint32_t index;
	int supported;

	/* A completed query remains stable for this process's one compositor device. */
	if (gpu_queried != 0U)
		return 0;

	/* A backend without DMA-BUF memory can still serve ordinary shared-memory clients. */
	if (gpu_dma_available == 0U) {
		gpu_queried = 1U;
		return 0;
	}

	/* Requires the same selected device that supplied the extension subset. */
	if (compose == NULL || compose->physical != gpu_physical)
		return EINVAL;

	/* An API-1.0 instance exposes the enabled properties2 extension's KHR entry point. */
	query = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(compose->instance, "vkGetPhysicalDeviceFormatProperties2KHR");
	if (query == NULL)
		return ENOTSUP;

	/* A backend without modifiers accepts only genuinely sampled, importable linear images. */
	if (gpu_modifier_available == 0U) {
		/* Reads linear sampling support from the ordinary format properties. */
		memset(&properties, 0, sizeof(properties));
		properties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
		query(compose->physical, VK_FORMAT_B8G8R8A8_UNORM, &properties);

		/* External importability is required in addition to the sampling format feature. */
		if ((properties.formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0U) {
			supported = gpu_modifier_importable(compose, 0U);
			if (supported != 0) {
				gpu_modifiers[0] = 0U;
				gpu_modifier_count = 1U;
			}
		}

		/* Succeeded: the fallback layout set is now immutable for this device. */
		gpu_queried = 1U;
		return 0;
	}

	/* Measures the backend's v1 modifier list before allowing it to write into bounded storage. */
	memset(&list, 0, sizeof(list));
	list.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;

	/* Connects modifier enumeration to the selected byte format. */
	memset(&properties, 0, sizeof(properties));
	properties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
	properties.pNext = &list;
	query(compose->physical, VK_FORMAT_B8G8R8A8_UNORM, &properties);

	/* Refuses more entries than the module can retain rather than accepting a partial set. */
	count = list.drmFormatModifierCount;
	if (count > GPU_ENUM_MAX)
		return EOVERFLOW;

	/* Populates the measured layout array with only the allocated number of entries. */
	list.pDrmFormatModifierProperties = modifiers;
	query(compose->physical, VK_FORMAT_B8G8R8A8_UNORM, &properties);
	if (list.drmFormatModifierCount < count)
		count = list.drmFormatModifierCount;

	/* Retains only single-plane sampled layouts with real DMA-BUF import support. */
	for (index = 0U; index < count; index++) {
		/* Multi-plane formats need a different image and protocol ownership contract. */
		if (modifiers[index].drmFormatModifierPlaneCount != 1U)
			continue;

		/* The compositor must be able to sample every advertised modifier. */
		if ((modifiers[index].drmFormatModifierTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0U)
			continue;

		/* External image properties certify the actual modifier's importability. */
		supported = gpu_modifier_importable(compose, modifiers[index].drmFormatModifier);
		if (supported == 0)
			continue;

		/* These entries stay stable until the process's compositor device is retired. */
		gpu_modifiers[gpu_modifier_count] = modifiers[index].drmFormatModifier;
		gpu_modifier_count++;
	}

	/* Succeeded: each binding receives the same verified modifier snapshot. */
	gpu_queried = 1U;
	return 0;
}

/* Checks external import support for one sampled image layout. */
static int
gpu_modifier_importable(
	struct zwl_compose *compose,
	uint64_t modifier)
{
	PFN_vkGetPhysicalDeviceImageFormatProperties2KHR query;
	VkPhysicalDeviceImageDrmFormatModifierInfoEXT layout;
	VkPhysicalDeviceExternalImageFormatInfo external;
	VkPhysicalDeviceImageFormatInfo2 image;
	VkExternalImageFormatProperties external_properties;
	VkImageFormatProperties2 properties;
	VkResult status;

	/* The enabled API-1.0 properties2 extension supplies this physical query. */
	query = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)vkGetInstanceProcAddr(compose->instance, "vkGetPhysicalDeviceImageFormatProperties2KHR");
	if (query == NULL)
		return 0;

	/* Specifies the exact opaque modifier whose sampling support was measured. */
	memset(&layout, 0, sizeof(layout));
	layout.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT;
	layout.drmFormatModifier = modifier;
	layout.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	/* DMA-BUF is the sole external memory handle supplied by this protocol. */
	memset(&external, 0, sizeof(external));
	external.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
	external.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

	/* Modifier-less backends use ordinary linear tiling without modifier structs. */
	if (gpu_modifier_available != 0U)
		external.pNext = &layout;

	/* Asks for precisely the sampled two-dimensional byte format used by composition. */
	memset(&image, 0, sizeof(image));
	image.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
	image.pNext = &external;
	image.format = VK_FORMAT_B8G8R8A8_UNORM;
	image.type = VK_IMAGE_TYPE_2D;
	image.tiling = VK_IMAGE_TILING_LINEAR;
	image.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

	/* Explicit modifiers select their own Vulkan tiling mode. */
	if (gpu_modifier_available != 0U)
		image.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;

	/* Receives external-memory support separately from ordinary image size limits. */
	memset(&external_properties, 0, sizeof(external_properties));
	external_properties.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;

	/* Connects the support result to the image-format result. */
	memset(&properties, 0, sizeof(properties));
	properties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
	properties.pNext = &external_properties;
	status = query(compose->physical, &image, &properties);
	if (status != VK_SUCCESS)
		return 0;

	/* An ordinary format match does not imply external-memory importability. */
	if ((external_properties.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) == 0U)
		return 0;

	/* Succeeded: this sampled image layout can consume the client's dma-buf. */
	return 1;
}

/* Matches a client's opaque modifier against the advertised immutable set. */
static int
gpu_modifier_known(
	uint64_t modifier)
{
	uint32_t index;

	/* Searches only the cached importable modifier entries. */
	for (index = 0U; index < gpu_modifier_count; index++) {
		/* An exact token match selects one validated image layout. */
		if (gpu_modifiers[index] == modifier)
			return 1;
	}

	/* Refuses a modifier that this compositor did not advertise. */
	return 0;
}

/* Creates one independent params object or destroys its factory binding. */
static int
gpu_factory_request(
	struct zwl_object *factory,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	struct zwl_object *params;
	struct gpu_buffer *plane;
	uint32_t id;

	/* Factory destruction never invalidates previously constructed params or buffers. */
	if (opcode == 0U && size == 0U) {
		zwl_object_destroy(factory);
		return 0;
	}

	/* The only other factory request contains exactly one child identity. */
	if (opcode != 1U || size != 4U)
		return EPROTO;

	/* Creates the client-chosen params identity before attaching any OS-owned state. */
	id = gpu_word(bytes, 0U);
	params = zwl_create(factory->client, id, ZWL_GPU_OBJECT, factory->version);
	if (params == NULL)
		return EPROTO;

	/* Allocates one batch with an explicitly absent descriptor. */
	plane = calloc(1, sizeof(*plane));
	if (plane == NULL) {
		zwl_object_destroy(params);
		return ENOMEM;
	}

	/* The params object owns its record independently of the factory binding. */
	plane->fd = -1;
	params->gpu_private = plane;

	/* Succeeded: add and create can now populate this one-use batch. */
	return 0;
}

/* Consumes one plane descriptor only after the complete add payload has arrived. */
static int
gpu_params_add(
	struct zwl_object *params,
	const unsigned char *bytes,
	size_t size)
{
	struct gpu_buffer *plane;
	uint32_t index;
	int descriptor;
	int error;

	/* File descriptors occupy ancillary storage, not a sixth wire word. */
	if (size != 20U)
		return EPROTO;

	/* A pending descriptor leaves the complete request ready for the common retry path. */
	descriptor = zwl_take_fd(params->client);
	if (descriptor < 0)
		return EAGAIN;

	/* A consumed batch cannot collect another descriptor. */
	plane = params->gpu_private;
	if (plane->used != 0U) {
		(void)close(descriptor);
		error = gpu_protocol_error(params, GPU_ALREADY_USED, "params already used");
		return error;
	}

	/* This compositor's RGB import contract permits plane zero only. */
	index = gpu_word(bytes, 0U);
	if (index != 0U) {
		(void)close(descriptor);
		error = gpu_protocol_error(params, GPU_PLANE_IDX, "only plane zero is supported");
		return error;
	}

	/* Replacing a previously supplied plane would hide its ownership and is a protocol error. */
	if (plane->fd >= 0) {
		(void)close(descriptor);
		error = gpu_protocol_error(params, GPU_PLANE_SET, "plane already set");
		return error;
	}

	/* Marks the retained request descriptor close-on-exec before accepting the layout. */
	error = fcntl(descriptor, F_SETFD, FD_CLOEXEC);
	if (error < 0) {
		(void)close(descriptor);
		return EIO;
	}

	/* The params record owns this descriptor until import construction or batch destruction. */
	plane->fd = descriptor;
	plane->offset = gpu_word(bytes, 4U);
	plane->stride = gpu_word(bytes, 8U);
	plane->modifier = (uint64_t)gpu_word(bytes, 12U) << 32;
	plane->modifier |= gpu_word(bytes, 16U);

	/* Succeeded: the one supported plane is present with its complete wire layout. */
	return 0;
}

/* Validates and consumes one batch through synchronous or asynchronous buffer construction. */
static int
gpu_params_create(
	struct zwl_object *params,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	struct gpu_buffer *plane;
	uint32_t id;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t flags;
	size_t offset;
	uint64_t allocation_bytes;
	int error;

	/* Immediate construction adds the client-chosen wl_buffer identity to the four scalars. */
	offset = 0U;
	id = 0U;
	if (opcode == 3U)
		offset = 4U;

	/* Exact framing is checked before any layout scalar is decoded. */
	if (size != offset + 16U)
		return EPROTO;

	/* A batch remains consumed even when its first import fails. */
	plane = params->gpu_private;
	if (plane->used != 0U) {
		error = gpu_protocol_error(params, GPU_ALREADY_USED, "params already used");
		return error;
	}

	/* Marks the batch consumed before validating its proposed image description. */
	plane->used = 1U;
	width = gpu_word(bytes, offset);
	height = gpu_word(bytes, offset + 4U);
	format = gpu_word(bytes, offset + 8U);
	flags = gpu_word(bytes, offset + 12U);

	/* Only the immediate constructor uses a client-chosen new buffer identity. */
	if (opcode == 3U)
		id = gpu_word(bytes, 0U);

	/* Rejects transformations whose sampling contract this compositor does not implement. */
	if (flags != 0U) {
		/* Immediate construction must report its fatal unsupported-format error. */
		if (opcode == 3U) {
			error = gpu_protocol_error(params, GPU_INVALID_FORMAT, "buffer flags are unsupported");
			return error;
		}

		/* Asynchronous construction reports a failed event with no child buffer. */
		error = zwl_emit(params->client, params->id, 1U, NULL, 0U);
		if (error != 0)
			return error;

		/* Succeeded: the unsupported batch has reported its nonfatal failure. */
		return 0;
	}

	/* Bounds every client-controlled value before it can reach Vulkan import. */
	allocation_bytes = 0U;
	error = gpu_params_validate(params, width, height, format, &allocation_bytes);
	if (error != 0)
		return error;

	/* Creates and imports exactly one wl_buffer from this consumed batch. */
	error = gpu_create_buffer(params, opcode, id, width, height, format, allocation_bytes);
	if (error != 0)
		return error;

	/* Succeeded: the construction emitted its outcome and retained independent buffer ownership. */
	return 0;
}

/* Checks dimensions, layout and actual allocation bounds before calling any Vulkan importer. */
static int
gpu_params_validate(
	struct zwl_object *params,
	uint32_t width,
	uint32_t height,
	uint32_t format,
	uint64_t *allocation_bytes)
{
	struct gpu_buffer *plane;
	uint32_t maximum;
	uint64_t required;
	off_t bytes;
	int supported;
	int error;

	/* Every supported RGB format needs precisely plane zero. */
	plane = params->gpu_private;
	if (plane->fd < 0) {
		error = gpu_protocol_error(params, GPU_INCOMPLETE, "plane zero is missing");
		return error;
	}

	/* Interprets both supported fourcc values as the same byte format with different alpha. */
	if (format != GPU_ARGB8888 && format != GPU_XRGB8888) {
		error = gpu_protocol_error(params, GPU_INVALID_FORMAT, "unsupported buffer format");
		return error;
	}

	/* Unknown modifiers must not bypass the physical-device import capability query. */
	supported = gpu_modifier_known(plane->modifier);
	if (supported == 0) {
		error = gpu_protocol_error(params, GPU_INVALID_FORMAT, "unsupported buffer modifier");
		return error;
	}

	/* Unsigned decoding also rejects negative signed dimensions above the bounded Vulkan limit. */
	maximum = params->client->server->gpu_limits.max_dimension;
	if (width == 0U ||
	    height == 0U ||
	    width > maximum ||
	    height > maximum) {
		error = gpu_protocol_error(params, GPU_INVALID_DIMENSIONS, "invalid buffer dimensions");
		return error;
	}

	/* Four bytes per pixel must fit within each complete client-supplied row. */
	if ((uint64_t)plane->stride < (uint64_t)width * 4U) {
		error = gpu_protocol_error(params, GPU_OUT_OF_BOUNDS, "buffer stride is too short");
		return error;
	}

	/* DMA-BUF reports its kernel-owned allocation size without reading client memory. */
	bytes = lseek(plane->fd, 0, SEEK_END);
	if (bytes <= 0) {
		error = gpu_protocol_error(params, GPU_OUT_OF_BOUNDS, "cannot measure buffer allocation");
		return error;
	}

	/* The wide arithmetic cannot wrap for the wire's three unsigned 32-bit layout values. */
	required = (uint64_t)plane->offset + (uint64_t)plane->stride * height;
	if (required > (uint64_t)bytes) {
		error = gpu_protocol_error(params, GPU_OUT_OF_BOUNDS, "buffer layout exceeds allocation");
		return error;
	}

	/* Succeeded: the full image lies inside the descriptor's actual allocation. */
	*allocation_bytes = (uint64_t)bytes;
	return 0;
}

/* Reports one fatal params error and records the rejected import without calling Vulkan. */
static int
gpu_protocol_error(
	struct zwl_object *params,
	uint32_t code,
	const char *reason)
{
	int error;

	/* The common diagnostic prefix also counts pre-import validation failures. */
	printf("ZWL IMPORT_ERROR client=%" PRIu64 " errno=%d\n", (uint64_t)params->client->number, EINVAL);
	error = zwl_error_code(params->client, params->id, code, reason);
	if (error != 0)
		return error;

	/* Succeeded: the common protocol error machinery owns client termination. */
	return 0;
}

/* Constructs a buffer with independent descriptor ownership and reports its import outcome. */
static int
gpu_create_buffer(
	struct zwl_object *params,
	uint32_t opcode,
	uint32_t id,
	uint32_t width,
	uint32_t height,
	uint32_t format,
	uint64_t allocation_bytes)
{
	struct zwl_object *buffer;
	struct gpu_buffer *source;
	struct gpu_buffer *plane;
	VkResult status;
	int error;

	/* Asynchronous construction uses the compositor's reserved server identity range. */
	if (opcode == 2U) {
		buffer = zwl_create_server(params->client, ZWL_BUFFER, 1U);
	} else {
		buffer = zwl_create(params->client, id, ZWL_BUFFER, 1U);
	}

	/* Refuses an exhausted identity table or an invalid client-chosen buffer identity. */
	if (buffer == NULL)
		return EPROTO;

	/* Allocates the buffer's descriptor record separately from its one-use params. */
	plane = calloc(1, sizeof(*plane));
	if (plane == NULL) {
		zwl_object_destroy(buffer);
		return ENOMEM;
	}

	/* Copies immutable layout fields while giving the buffer its own close-on-exec fd. */
	source = params->gpu_private;
	*plane = *source;
	plane->fd = fcntl(source->fd, F_DUPFD_CLOEXEC, 0);
	if (plane->fd < 0) {
		free(plane);
		zwl_object_destroy(buffer);
		return EIO;
	}

	/* Final common object cleanup now owns the retained plane descriptor. */
	buffer->gpu_private = plane;
	status = gpu_image(buffer, plane, width, height);
	if (status != VK_SUCCESS) {
		/* A failed import retires its independently created buffer and all partial resources. */
		printf("ZWL IMPORT_ERROR client=%" PRIu64 " errno=%d\n", (uint64_t)params->client->number, EIO);
		zwl_object_destroy(buffer);

		/* Immediate creation cannot return an invalid wl_buffer to the client. */
		if (opcode == 3U) {
			error = zwl_error_code(params->client, params->id, GPU_INVALID_BUFFER, "Vulkan buffer import failed");
			return error;
		}

		/* Asynchronous creation reports a recoverable failed event without a child identity. */
		error = zwl_emit(params->client, params->id, 1U, NULL, 0U);
		if (error != 0)
			return error;

		/* Succeeded: the asynchronous failure is reported and owns no imported resource. */
		return 0;
	}

	/* The fourcc selects how common composition interprets the imported image's alpha byte. */
	if (format == GPU_ARGB8888)
		zwl_import_set_alpha(buffer, 1U);

	/* The imported resource and actual kernel allocation size share the target's diagnostic shape. */
	printf("ZWL IMPORT client=%" PRIu64 " buffer=%u width=%u height=%u bytes=%" PRIu64 "\n", (uint64_t)params->client->number, buffer->id, width, height, (uint64_t)allocation_bytes);

	/* Asynchronous construction announces its server-allocated new identity. */
	if (opcode == 2U) {
		error = zwl_emit(params->client, params->id, 0U, &buffer->id, sizeof(buffer->id));
		if (error != 0) {
			zwl_object_destroy(buffer);
			return error;
		}
	}

	/* Succeeded: common frame ownership can now borrow this immutable imported image. */
	return 0;
}

/* Creates an external image and transfers both Vulkan resources to common import ownership. */
static VkResult
gpu_image(
	struct zwl_object *buffer,
	const struct gpu_buffer *plane,
	uint32_t width,
	uint32_t height)
{
	struct zwl_compose *compose;
	VkSubresourceLayout layout;
	VkImageDrmFormatModifierExplicitCreateInfoEXT modifier;
	VkExternalMemoryImageCreateInfo external;
	VkImageCreateInfo create;
	VkImage image;
	VkDeviceMemory memory;
	VkImageSubresource subresource;
	VkSubresourceLayout actual;
	VkResult status;

	/* Builds the sole image plane from the validated client layout. */
	compose = buffer->client->server->compose;
	memset(&layout, 0, sizeof(layout));
	layout.offset = plane->offset;
	layout.rowPitch = plane->stride;

	/* Explicit layout import preserves the client's opaque modifier and row pitch. */
	memset(&modifier, 0, sizeof(modifier));
	modifier.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
	modifier.drmFormatModifier = plane->modifier;
	modifier.drmFormatModifierPlaneCount = 1U;
	modifier.pPlaneLayouts = &layout;

	/* DMA-BUF memory is independently imported for this one image. */
	memset(&external, 0, sizeof(external));
	external.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
	external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

	/* Modifier-less backends receive no extension-specific layout structure. */
	if (gpu_modifier_available != 0U)
		external.pNext = &modifier;

	/* Creates only a sampled, single-mip, single-layer two-dimensional image. */
	memset(&create, 0, sizeof(create));
	create.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create.pNext = &external;
	create.imageType = VK_IMAGE_TYPE_2D;
	create.format = VK_FORMAT_B8G8R8A8_UNORM;
	create.extent.width = width;
	create.extent.height = height;
	create.extent.depth = 1U;
	create.mipLevels = 1U;
	create.arrayLayers = 1U;
	create.samples = VK_SAMPLE_COUNT_1_BIT;
	create.tiling = VK_IMAGE_TILING_LINEAR;
	create.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	create.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	/* Explicit modifiers select their validated Vulkan tiling mode. */
	if (gpu_modifier_available != 0U)
		create.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;

	/* Image creation is checked before querying or importing its memory. */
	image = VK_NULL_HANDLE;
	memory = VK_NULL_HANDLE;
	status = vkCreateImage(compose->device, &create, NULL, &image);
	if (status != VK_SUCCESS)
		return status;

	/* The linear fallback must agree exactly with the backend's native row layout. */
	if (gpu_modifier_available == 0U) {
		/* Reads the created image's ordinary color-plane layout. */
		memset(&subresource, 0, sizeof(subresource));
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		vkGetImageSubresourceLayout(compose->device, image, &subresource, &actual);

		/* A different native offset or pitch cannot represent the supplied dma-buf safely. */
		if (actual.offset != plane->offset || actual.rowPitch != plane->stride) {
			gpu_image_release(compose, image, memory);
			return VK_ERROR_INVALID_EXTERNAL_HANDLE;
		}
	}

	/* Imports a duplicate fd using the actual image requirements and compatible memory types. */
	status = gpu_image_memory(compose, plane, image, &memory);
	if (status != VK_SUCCESS) {
		gpu_image_release(compose, image, memory);
		return status;
	}

	/* Binds the imported dedicated allocation before common image-view creation. */
	status = vkBindImageMemory(compose->device, image, memory, 0U);
	if (status != VK_SUCCESS) {
		gpu_image_release(compose, image, memory);
		return status;
	}

	/* Common adoption consumes both handles even if image-view or descriptor creation fails. */
	status = zwl_import_adopt(buffer, image, memory, width, height, VK_FORMAT_B8G8R8A8_UNORM);
	if (status != VK_SUCCESS)
		return status;

	/* Succeeded: the common buffer now owns both imported Vulkan resources. */
	return VK_SUCCESS;
}

/* Imports a dedicated allocation with fd ownership matching Vulkan's success convention. */
static VkResult
gpu_image_memory(
	struct zwl_compose *compose,
	const struct gpu_buffer *plane,
	VkImage image,
	VkDeviceMemory *memory)
{
	PFN_vkGetMemoryFdPropertiesKHR query;
	VkMemoryFdPropertiesKHR properties;
	VkMemoryRequirements requirements;
	VkMemoryDedicatedAllocateInfo dedicated;
	VkImportMemoryFdInfoKHR imported;
	VkMemoryAllocateInfo allocate;
	VkResult status;
	uint32_t bits;
	uint32_t memory_type;
	int descriptor;

	/* Requires the device's enabled external-fd memory query entry point. */
	query = (PFN_vkGetMemoryFdPropertiesKHR)vkGetDeviceProcAddr(compose->device, "vkGetMemoryFdPropertiesKHR");
	if (query == NULL)
		return VK_ERROR_EXTENSION_NOT_PRESENT;

	/* Measures which memory types the actual dma-buf descriptor can import. */
	memset(&properties, 0, sizeof(properties));
	properties.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;
	status = query(compose->device, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, plane->fd, &properties);
	if (status != VK_SUCCESS)
		return status;

	/* Intersects descriptor compatibility with the created image's memory requirements. */
	vkGetImageMemoryRequirements(compose->device, image, &requirements);
	bits = properties.memoryTypeBits & requirements.memoryTypeBits;
	if (bits == 0U)
		return VK_ERROR_INVALID_EXTERNAL_HANDLE;

	/* Selects the lowest compatible bit without shifting by the width of the bit mask. */
	memory_type = 0U;
	while ((bits & 1U) == 0U) {
		bits >>= 1;
		memory_type++;
	}

	/* Vulkan consumes this independently owned duplicate only on successful allocation. */
	descriptor = fcntl(plane->fd, F_DUPFD_CLOEXEC, 0);
	if (descriptor < 0)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* The imported memory is dedicated to the image whose requirements were measured. */
	memset(&dedicated, 0, sizeof(dedicated));
	dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	dedicated.image = image;

	/* Chains the owned external descriptor before its dedicated-image requirement. */
	memset(&imported, 0, sizeof(imported));
	imported.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
	imported.pNext = &dedicated;
	imported.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
	imported.fd = descriptor;

	/* Uses Vulkan's required size, which need not equal the dma-buf's page-rounded size. */
	memset(&allocate, 0, sizeof(allocate));
	allocate.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate.pNext = &imported;
	allocate.allocationSize = requirements.size;
	allocate.memoryTypeIndex = memory_type;
	status = vkAllocateMemory(compose->device, &allocate, NULL, memory);
	if (status != VK_SUCCESS) {
		(void)close(descriptor);
		return status;
	}

	/* Succeeded: Vulkan owns the duplicated descriptor and the returned dedicated allocation. */
	return VK_SUCCESS;
}

/* Releases partial image construction before common adoption owns either handle. */
static void
gpu_image_release(
	struct zwl_compose *compose,
	VkImage image,
	VkDeviceMemory memory)
{
	/* Imported memory cannot retire before the image bound to it. */
	if (image != VK_NULL_HANDLE)
		vkDestroyImage(compose->device, image, NULL);

	/* A failed allocation may leave no memory handle to destroy. */
	if (memory != VK_NULL_HANDLE)
		vkFreeMemory(compose->device, memory, NULL);

	/* Succeeded: partial construction owns no Vulkan resource. */
	return;
}
