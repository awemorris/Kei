/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Defines the boundary between the compositor and its OS's GPU module.
 * The module owns the buffer protocol and image import; the compositor
 * receives a Vulkan image, its memory, size and format.
 */
#ifndef ZWL_GPU_H
#define ZWL_GPU_H

#include <stddef.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

struct zwl_object;

/*
 * What the compositor's Vulkan device can take, against which a description
 * is checked before any of its values reaches Vulkan.
 *
 * The server retains these limits after selecting its Vulkan device;
 * imports use them until that device is destroyed during compose cleanup.
 */
struct zwl_gpu_limits {
	uint32_t max_dimension;
	uint32_t memory_type_count;
};

/* Names the OS's Wayland global for GPU buffers. */
const char *zwl_gpu_global_interface(void);
/* Reports the version offered for that global. */
uint32_t zwl_gpu_global_version(void);
/* Handles a GPU request: 0, EAGAIN for a pending fd, or a client-ending errno. */
int zwl_gpu_request(struct zwl_object *factory, uint32_t opcode, const unsigned char *bytes, size_t size);
/* Sends the OS-owned factory binding its initial format snapshot. */
int zwl_gpu_bind(struct zwl_object *factory);
/* Retires any OS-owned params or retained buffer descriptors. */
void zwl_gpu_object_free(struct zwl_object *object);
/* Takes any buffer-owned fence before a surface's commit moves its fences. */
void zwl_gpu_commit(struct zwl_object *surface, struct zwl_object *buffer);
/*
 * Copies up to capacity extension names and returns how many the module
 * needs; device creation fails when that exceeds capacity.
 */
uint32_t zwl_gpu_instance_extensions(const char **names, uint32_t capacity);
uint32_t zwl_gpu_device_extensions(VkPhysicalDevice physical, const char **names, uint32_t capacity);
/* Reports the frame fence's exported handle type, or 0 for status polling. */
VkExternalFenceHandleTypeFlagBits zwl_gpu_frame_fence_type(void);

#endif
