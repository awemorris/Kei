/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Test-only ioctl interposition: exercise the production CPU fallback without a hidden production switch. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <linux/dma-buf.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/ioctl.h>

/* Overrides only the pointer-valued DMA-BUF imports used by the isolated probe client. */
int
ioctl(
	int fd,
	unsigned long request,
	...)
{
	va_list arguments;
	void *argument;
	int (*next)(int, unsigned long, ...);

	/* IMPORT_SYNC_FILE reports the same unsupported-kernel capability used by production fallback. */
	if (request == DMA_BUF_IOCTL_IMPORT_SYNC_FILE) {
		errno = ENOTTY;
		return -1;
	}

	/* Every ioctl in this probe workload supplies a pointer-valued third argument. */
	va_start(arguments, request);
	argument = va_arg(arguments, void *);
	va_end(arguments);

	/* Other operations use the actual kernel-facing libc entry point. */
	next = (int (*)(int, unsigned long, ...))dlsym(RTLD_NEXT, "ioctl");
	if (next == NULL) {
		errno = ENOSYS;
		return -1;
	}

	/* Preserves the underlying result and errno for every non-import ioctl. */
	return next(fd, request, argument);
}
