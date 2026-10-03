/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

#ifndef KERN_KERN_SYSCTL_H
#define KERN_KERN_SYSCTL_H

#include <stddef.h>
#include <stdint.h>

/*
 * What a GPU driver lets root do with the devices it holds back at boot
 * (hw.gpu.start).
 *
 * A driver that can hold its devices until root asks for the start (the
 * i915 with the boot parameter i915.start=manual) hands these to the kernel
 * once, while it registers.  The operations live as long as the kernel.
 */
struct kern_gpu_start_ops {
	/* Reports how many devices are held for the start. */
	uint64_t (*held)(void);

	/* Starts every held device; ENODEV when none is held. */
	int (*start)(void);
};

void
sysctl_init(void);

/*
 * Counts a GPU device whose driver has attached it and will publish its node
 * later (hw.gpu.attaching).
 */
void
kern_gpu_attach_begin(void);

/*
 * Uncounts a GPU device whose node is now published or never will be.
 */
void
kern_gpu_attach_end(void);

/*
 * Installs the operations behind hw.gpu.start; called once, on the boot
 * thread, before user space runs.
 */
void
kern_gpu_start_ops_set(
	const struct kern_gpu_start_ops *ops);

int
kern_sysctl(
	const int *name,
	unsigned namelen,
	void *oldp,
	size_t *oldlenp,
	const void *newp,
	size_t newlen,
	int superuser);

#endif
