/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Vulkan procedure boundary and its per-thread recursion detection.
 */

#include "compat.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* One thread's nested backend calls, owned by its pthread key until thread exit. */
struct compat_thread {
	const char *calls[64];
	unsigned depth;
};

/* Protects live ownership records; Vulkan callers separately synchronize handle destruction. */
pthread_mutex_t compat_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Publishes the pthread key before any thread obtains its backend call record. */
static pthread_once_t compat_thread_once = PTHREAD_ONCE_INIT;

/* Owns one recursion record per calling thread; pthread releases each record when its thread exits. */
static pthread_key_t compat_thread_key;

#define COMPAT_FORWARD_DECLARATIONS
#include "forward.inc"
#undef COMPAT_FORWARD_DECLARATIONS

static const struct compat_name *compat_find_name(const char *name);
static int compat_global_name(const char *name);
static void compat_thread_open(void);
static struct compat_thread *compat_thread_get(void);

#include "forward.inc"

/*
 * Resolves instance procedures while retaining ownership of intercepted names.
 */
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(
	VkInstance instance,
	const char *pName)
{
	const struct compat_name *entry;
	PFN_vkVoidFunction address;
	int global;

	/* A null name cannot identify a procedure. */
	if (pName == NULL)
		return NULL;

	/* Enters the backend boundary before querying any exported entry point. */
	compat_enter("vkGetInstanceProcAddr");

	/* Restricts null-instance lookups to the global entry points. */
	global = compat_global_name(pName);
	if (instance == VK_NULL_HANDLE && global == 0) {
		/* Leaves the refused procedure query. */
		compat_leave();
		return NULL;
	}

	/* Keeps denied WSI names away from the backend's foreign surface handles. */
	entry = compat_find_name(pName);
	if (entry != NULL && entry->kind == 'N') {
		/* Leaves the refused WSI query. */
		compat_leave();
		return NULL;
	}

	/* Resolves our interceptors directly and forwards all other procedure queries. */
	address = NULL;
	if (entry != NULL && entry->kind == 'I') {
		address = entry->address;
	} else if (compat_backend.get_instance_proc != NULL) {
		/* Obtains the backend's dispatch entry for an unmodified procedure. */
		address = compat_backend.get_instance_proc(instance, pName);
	}

	/* Leaves the completed procedure lookup. */
	compat_leave();

	/* Succeeded: returns the procedure available for this instance. */
	return address;
}

/*
 * Resolves device procedures while retaining device interceptors.
 */
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetDeviceProcAddr(
	VkDevice device,
	const char *pName)
{
	const struct compat_name *entry;
	PFN_vkVoidFunction address;
	int differs;

	/* A device procedure requires a device and a name. */
	if (device == VK_NULL_HANDLE || pName == NULL)
		return NULL;

	/* Detects recursion before any backend lookup. */
	compat_enter("vkGetDeviceProcAddr");

	/* Refuses names owned by unsupported WSI extensions. */
	entry = compat_find_name(pName);
	if (entry != NULL && entry->kind == 'N') {
		/* Leaves the refused WSI query. */
		compat_leave();
		return NULL;
	}

	/* Distinguishes the four device-level interceptors from instance procedures. */
	address = NULL;
	differs = strcmp(pName, "vkGetDeviceProcAddr");
	if (differs == 0)
		address = (PFN_vkVoidFunction)vkGetDeviceProcAddr;

	/* Keeps device destruction in our ownership boundary. */
	differs = strcmp(pName, "vkDestroyDevice");
	if (differs == 0)
		address = (PFN_vkVoidFunction)vkDestroyDevice;

	/* Records queues retrieved through either public queue entry point. */
	differs = strcmp(pName, "vkGetDeviceQueue");
	if (differs == 0)
		address = (PFN_vkVoidFunction)vkGetDeviceQueue;

	/* Records queues retrieved through the Vulkan 1.1 entry point. */
	differs = strcmp(pName, "vkGetDeviceQueue2");
	if (differs == 0)
		address = (PFN_vkVoidFunction)vkGetDeviceQueue2;

	/* Lets the backend resolve unmodified device procedures. */
	if (address == NULL && compat_backend.get_device_proc != NULL)
		address = compat_backend.get_device_proc(device, pName);

	/* Leaves the completed device lookup. */
	compat_leave();

	/* Succeeded: returns the procedure available for this device. */
	return address;
}

/*
 * Enters one backend call and refuses recursion into the same exported function.
 */
void
compat_enter(
	const char *name)
{
	struct compat_thread *thread;
	unsigned index;
	int differs;

	/* Initializes backend state before observing any trampoline. */
	(void)compat_backend_ready();

	/* Obtains this thread's lifetime-managed backend call record. */
	thread = compat_thread_get();

	/* Detects a cycle in the current thread's active exported calls. */
	for (index = 0; index < thread->depth; index++) {
		/* Compares procedure names even when their strings came from different tables. */
		differs = strcmp(thread->calls[index], name);
		if (differs == 0) {
			/* Explains the symbol cycle before terminating rather than hanging. */
			(void)fprintf(stderr, "libvulkan-compat: the backend called back into %s (symbol interposition); unset KEILAND_VULKAN_NO_DEEPBIND\n", name);
			abort();
		}
	}

	/* A malformed backend must not overrun the bounded recursion record. */
	if (thread->depth == sizeof(thread->calls) / sizeof(thread->calls[0])) {
		/* Explains the exhausted backend call stack. */
		(void)fputs("libvulkan-compat: backend call nesting overflow\n", stderr);
		abort();
	}

	/* Keeps this name active until the matching compat_leave. */
	thread->calls[thread->depth] = name;
	thread->depth++;

	/* Succeeded: this thread now owns one active backend call. */
	return;
}

/*
 * Leaves the latest backend call on this thread.
 */
void
compat_leave(
	void)
{
	struct compat_thread *thread;

	/* Retrieves the record created by the matching entry. */
	thread = pthread_getspecific(compat_thread_key);
	if (thread == NULL)
		return;

	/* Retires the name that protected the just-completed backend call. */
	if (thread->depth != 0) {
		thread->depth--;
		thread->calls[thread->depth] = NULL;
	}

	/* Succeeded: the enclosing call stack is restored. */
	return;
}

/*
 * Terminates a call that the selected backend cannot provide.
 */
void
compat_missing(
	const char *name)
{
	/* Explains why an exported core entry point cannot be executed. */
	(void)fprintf(stderr, "libvulkan-compat: the backend has no %s\n", name);
	abort();
}

/*
 * Identifies backend WSI extensions that must not reach our callers.
 */
int
compat_wsi_extension(
	const char *name)
{
	const char *part;
	int differs;

	/* Surfaces belong to our WSI, including platform-specific and query extensions. */
	part = strstr(name, "surface");
	if (part != NULL)
		return 1;

	/* Display output belongs to our KMS implementation. */
	part = strstr(name, "display");
	if (part != NULL)
		return 1;

	/* Swapchains and presentation extensions cannot consume backend WSI handles. */
	part = strstr(name, "swapchain");
	if (part != NULL)
		return 1;

	/* Hides presentation-related extensions not implemented by this WSI. */
	part = strstr(name, "present");
	if (part != NULL)
		return 1;

	/* HDR metadata takes a swapchain even though its extension name does not say so. */
	differs = strcmp(name, "VK_EXT_hdr_metadata");
	if (differs == 0)
		return 1;

	/* Succeeded: this extension is unrelated to backend WSI ownership. */
	return 0;
}

/* Finds a maintained procedure entry without delegating denied names to the backend. */
static const struct compat_name *
compat_find_name(
	const char *name)
{
	size_t index;
	int differs;

	/* Looks up the single maintained function ownership table. */
	for (index = 0; index < sizeof(compat_names) / sizeof(compat_names[0]); index++) {
		/* Identifies the exact exported or denied procedure name. */
		differs = strcmp(name, compat_names[index].name);
		if (differs == 0)
			return &compat_names[index];
	}

	/* Reports that this is an unmodified backend extension procedure. */
	return NULL;
}

/* Identifies procedures legal before an instance has been created. */
static int
compat_global_name(
	const char *name)
{
	int differs;

	/* Instance creation is the initial global operation. */
	differs = strcmp(name, "vkCreateInstance");
	if (differs == 0)
		return 1;

	/* Instance extension enumeration is available without an instance. */
	differs = strcmp(name, "vkEnumerateInstanceExtensionProperties");
	if (differs == 0)
		return 1;

	/* Instance layers can be listed before creating an instance. */
	differs = strcmp(name, "vkEnumerateInstanceLayerProperties");
	if (differs == 0)
		return 1;

	/* The supported API version can be queried globally. */
	differs = strcmp(name, "vkEnumerateInstanceVersion");
	if (differs == 0)
		return 1;

	/* The global procedure resolver can resolve itself. */
	differs = strcmp(name, "vkGetInstanceProcAddr");
	if (differs == 0)
		return 1;

	/* Reports that this procedure requires a live instance. */
	return 0;
}

/* Publishes a pthread key whose destructor releases each thread's call record. */
static void
compat_thread_open(
	void)
{
	int error;

	/* Creates the recursion-record lifetime without static TLS loader dependencies. */
	error = pthread_key_create(&compat_thread_key, free);
	if (error != 0) {
		/* Explains why the process cannot provide safe backend call tracking. */
		(void)fputs("libvulkan-compat: cannot allocate backend thread key\n", stderr);
		abort();
	}

	/* Succeeded: every caller can own a record until its thread exits. */
	return;
}

/* Retrieves or creates the current thread's backend call stack. */
static struct compat_thread *
compat_thread_get(
	void)
{
	struct compat_thread *thread;
	int error;

	/* Publishes the lifetime key before querying its thread-specific record. */
	(void)pthread_once(&compat_thread_once, compat_thread_open);

	/* Reuses the current thread's active recursion record. */
	thread = pthread_getspecific(compat_thread_key);
	if (thread != NULL)
		return thread;

	/* Allocates one record for this thread's lifetime. */
	thread = calloc(1, sizeof(*thread));
	if (thread == NULL) {
		/* Refuses to execute a backend without recursion protection. */
		(void)fputs("libvulkan-compat: no memory for backend thread record\n", stderr);
		abort();
	}

	/* Transfers the record's ownership to the pthread key destructor. */
	error = pthread_setspecific(compat_thread_key, thread);
	if (error != 0) {
		/* Releases an unowned record before explaining the lifetime failure. */
		free(thread);
		(void)fputs("libvulkan-compat: cannot retain backend thread record\n", stderr);
		abort();
	}

	/* Succeeded: the calling thread owns its bounded backend call stack. */
	return thread;
}
