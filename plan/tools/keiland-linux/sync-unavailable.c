/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Test-only observations of kernel imports and backend fence waits; production has no test switch. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <linux/dma-buf.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <vulkan/vulkan.h>

/* This finite single-device test resolves its backend procedures before submitting any frame. */
static PFN_vkGetDeviceProcAddr probe_device_proc;

/* The one test device's unmodified fence waiter; populated during WSI table initialization. */
static PFN_vkWaitForFences probe_wait_fences;

static PFN_vkVoidFunction probe_get_device_proc(VkDevice device, const char *name);
static VkResult probe_wait(VkDevice device, uint32_t count, const VkFence *fences, VkBool32 all, uint64_t timeout);

/*
 * Records actual IMPORT_SYNC_FILE outcomes and optionally injects the one capability failure.
 */
int
ioctl(
	int fd,
	unsigned long request,
	...)
{
	va_list arguments;
	void *argument;
	struct dma_buf_import_sync_file *import;
	int (*next)(int, unsigned long, ...);
	int error;
	int saved_errno;

	/* This isolated test workload supplies pointer-valued third arguments to every ioctl. */
	va_start(arguments, request);
	argument = va_arg(arguments, void *);
	va_end(arguments);

	/* The compile-time fallback fixture supplies precisely the production unsupported-kernel result. */
#ifdef COMPAT_TEST_SYNC_UNAVAILABLE
	if (request == DMA_BUF_IOCTL_IMPORT_SYNC_FILE) {
		/* Records the injected failure without ever changing the kernel reservation. */
		import = argument;
		fprintf(stderr, "IMPORT_SYNC fd=%d flags=%u result=-1 errno=%d\n", fd, import->flags, ENOTTY);
		errno = ENOTTY;
		return -1;
	}
#endif

	/* Other operations use the actual libc ioctl with unchanged arguments. */
	next = (int (*)(int, unsigned long, ...))dlvsym(RTLD_NEXT, "ioctl", "GLIBC_2.2.5");
	if (next == NULL) {
		errno = ENOSYS;
		return -1;
	}

	/* Preserves errno across diagnostic output so production capability selection is unchanged. */
	error = next(fd, request, argument);
	saved_errno = errno;
	if (request == DMA_BUF_IOCTL_IMPORT_SYNC_FILE) {
		/* A successful kernel import, rather than a stub fence count, proves actual payload transfer. */
		import = argument;
		fprintf(stderr, "IMPORT_SYNC fd=%d flags=%u result=%d errno=%d\n", fd, import->flags, error, saved_errno);
	}

	/* Returns the actual kernel outcome, including its original error code. */
	errno = saved_errno;
	return error;
}

/*
 * Observes only the WSI's private device resolver while preserving backend binding isolation.
 */
void *
dlsym(
	void *handle,
	const char *name)
{
	void *(*next)(void *, const char *);
	void *address;
	int differs;

	/* glibc's versioned resolver avoids recursion into this observation wrapper. */
	next = (void *(*)(void *, const char *))dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.2.5");
	if (next == NULL)
		return NULL;

	/* Obtains the original symbol before replacing the one private lookup under observation. */
	address = next(handle, name);
	differs = strcmp(name, "vkGetDeviceProcAddr");
	if (differs == 0) {
		/* The backend remains RTLD_DEEPBIND; its own procedure calls do not pass through this fixture. */
		if (address != NULL) {
			probe_device_proc = (PFN_vkGetDeviceProcAddr)address;
			return (void *)probe_get_device_proc;
		}
	}

	/* Every unrelated symbol retains its original address. */
	return address;
}

/* Wraps only the private fence waiter returned while the chain initializes its device table. */
static PFN_vkVoidFunction
probe_get_device_proc(
	VkDevice device,
	const char *name)
{
	PFN_vkVoidFunction address;
	int differs;

	/* Gets the actual driver procedure with unchanged device and name. */
	address = probe_device_proc(device, name);
	differs = strcmp(name, "vkWaitForFences");
	if (differs == 0) {
		/* This single-device fixture preserves the underlying driver's complete wait behavior. */
		if (address != NULL) {
			probe_wait_fences = (PFN_vkWaitForFences)address;
			return (PFN_vkVoidFunction)probe_wait;
		}
	}

	/* Every unrelated device operation retains its original backend procedure. */
	return address;
}

/* Records completed private waits without changing their arguments or results. */
static VkResult
probe_wait(
	VkDevice device,
	uint32_t count,
	const VkFence *fences,
	VkBool32 all,
	uint64_t timeout)
{
	VkResult result;

	/* Calls the original driver fence waiter before recording CPU completion evidence. */
	result = probe_wait_fences(device, count, fences, all, timeout);
	fprintf(stderr, "PRIVATE_WAIT count=%u all=%u result=%d\n", count, all, result);

	/* Preserves a driver failure or timeout for the original caller. */
	if (result != VK_SUCCESS)
		return result;

	/* Succeeded: the original fence wait completed. */
	return VK_SUCCESS;
}
