/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Linux Vulkan chain's internal records; dispatchable handles remain backend handles.
 */

#ifndef KEILAND_VULKAN_COMPAT_H
#define KEILAND_VULKAN_COMPAT_H

#include <pthread.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

/* Backend entry points resolved directly by dlsym, never through an interposed procedure query. */
struct compat_backend {
	void *handle;
	PFN_vkGetInstanceProcAddr get_instance_proc;
	PFN_vkGetDeviceProcAddr get_device_proc;
	PFN_vkCreateInstance create_instance;
	PFN_vkDestroyInstance destroy_instance;
	PFN_vkEnumerateInstanceExtensionProperties instance_extensions;
	PFN_vkEnumerateInstanceLayerProperties instance_layers;
	PFN_vkEnumerateInstanceVersion instance_version;
	PFN_vkEnumerateDeviceExtensionProperties device_extensions;
	PFN_vkCreateDevice create_device;
	PFN_vkDestroyDevice destroy_device;
	PFN_vkGetDeviceQueue get_queue;
	PFN_vkGetDeviceQueue2 get_queue2;
};

/* One live backend instance, owned until destruction with its physical-device ownership list. */
struct compat_instance {
	struct compat_instance *next;
	VkInstance handle;
	uint32_t api_version;
	uint32_t physical_count;
	VkPhysicalDevice *physicals;
};

/* One live backend device, owned until destruction and referring to its still-live instance. */
struct compat_device {
	struct compat_device *next;
	VkDevice handle;
	VkPhysicalDevice physical;
	struct compat_instance *instance;
	uint32_t api_version;
};

/* One retrieved queue, owned by its device and retired before that device is destroyed. */
struct compat_queue {
	struct compat_queue *next;
	VkQueue handle;
	struct compat_device *device;
	uint32_t family;
};

/* One immutable maintained procedure name and its forwarding or interception responsibility. */
struct compat_name {
	const char *name;
	char kind;
	PFN_vkVoidFunction address;
};

/* Published by pthread_once; backend code remains loaded for the lifetime of the process. */
extern struct compat_backend compat_backend;

/* Protects the instance, device and queue ownership lists; callers obey Vulkan handle lifetime rules. */
extern pthread_mutex_t compat_mutex;

int compat_backend_ready(void);
void compat_forward_fill(void *handle);
void compat_enter(const char *name);
void compat_leave(void);
void compat_missing(const char *name) __attribute__((noreturn));
int compat_wsi_extension(const char *name);
VkResult compat_extensions(VkPhysicalDevice physical, const char *layer, uint32_t *count, VkExtensionProperties *properties);
struct compat_instance *compat_instance_get(VkInstance handle);
struct compat_instance *compat_instance_for_physical(VkPhysicalDevice physical);
struct compat_device *compat_device_get(VkDevice handle);
struct compat_queue *compat_queue_get(VkQueue handle);

#endif
