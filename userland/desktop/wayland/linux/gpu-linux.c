/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Provides Linux display extensions while GPU-buffer clients await their separate implementation. */
#include "../zwl-gpu.h"
#include <errno.h>

/*
 * Omits the unfinished Linux GPU-buffer global.
 */
const char *
zwl_gpu_global_interface(
	void)
{
	/* Succeeded: registry enumeration must skip this absent factory. */
	return NULL;
}

/*
 * Reports no version for the absent GPU-buffer factory.
 */
uint32_t
zwl_gpu_global_version(
	void)
{
	/* Succeeded: no Linux GPU-buffer protocol is advertised yet. */
	return 0;
}

/*
 * Refuses requests to the absent GPU-buffer protocol.
 */
int
zwl_gpu_request(
	struct zwl_object *factory,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	/* An unadvertised factory has no request or descriptor ownership to consume. */
	(void)factory;
	(void)opcode;
	(void)bytes;
	(void)size;

	/* Refuses the invalid protocol request. */
	return EPROTO;
}

/*
 * Preserves common shared-memory commit behavior.
 */
void
zwl_gpu_commit(
	struct zwl_object *surface,
	struct zwl_object *buffer)
{
	/* Shared-memory clients have no external GPU fence to transfer. */
	(void)surface;
	(void)buffer;

	/* Succeeded: common commit processing retains its original state. */
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
	if (capacity > 0)
		names[0] = VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME;

	/* DRM acquisition pairs the compositor's seat fd with Vulkan's own KMS backend. */
	if (capacity > 1)
		names[1] = VK_EXT_ACQUIRE_DRM_DISPLAY_EXTENSION_NAME;

	/* Succeeded: two names are required even when the supplied array is too small. */
	return 2;
}

/*
 * Reports no additional GPU-buffer device extensions.
 */
uint32_t
zwl_gpu_device_extensions(
	VkPhysicalDevice physical,
	const char **names,
	uint32_t capacity)
{
	/* The common display device list already supplies everything needed for wl_shm composition. */
	(void)physical;
	(void)names;
	(void)capacity;

	/* Succeeded: no private device extension needs appending. */
	return 0;
}

/*
 * Selects common Vulkan status polling for frame completion.
 */
VkExternalFenceHandleTypeFlagBits
zwl_gpu_frame_fence_type(
	void)
{
	/* Succeeded: no external frame-fence descriptor is exported. */
	return 0;
}
