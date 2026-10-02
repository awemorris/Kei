/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Reuses the existing real Vulkan image exporter without invoking its forged protocol request. */
#define main forge_unused_main
#include "plan/tools/keiland-linux/dmabuf-forge.c"
#undef main

#include <sys/stat.h>
#include "userland/desktop/libvulkan-compat/dma-sync.h"

/*
 * Observes a real exported native buffer's file flags and reservation operation.
 */
int
main(
	void)
{
	struct forge_probe probe;
	struct stat attributes;
	VkResult status;
	int descriptor_flags;
	int access_flags;
	int inspected;
	int synchronization;
	int exported;
	int native_error;
	int flags_error;

	/* Owns partial exporter cleanup before the first allocation. */
	memset(&probe, 0, sizeof(probe));
	probe.fd = -1;
	status = forge_image(&probe);
	if (status != VK_SUCCESS) {
		forge_cleanup(&probe);
		return 1;
	}

	/* Requires a live descriptor before interpreting its unusual access representation. */
	descriptor_flags = fcntl(probe.fd, F_GETFD);
	if (descriptor_flags < 0) {
		forge_cleanup(&probe);
		return 1;
	}

	/* Distinguishes FreeBSD's successful minus-one flags from an actual query failure. */
	errno = 0;
	access_flags = fcntl(probe.fd, F_GETFL);
	flags_error = errno;
	if (flags_error != 0) {
		forge_cleanup(&probe);
		return 1;
	}

	/* Checks native descriptor presence without trusting the driver's unfinished stat fields. */
	memset(&attributes, 0, sizeof(attributes));
	inspected = fstat(probe.fd, &attributes);
	if (inspected != 0) {
		forge_cleanup(&probe);
		return 1;
	}

	/* Records the live file-query contract independently of any reservation fallback. */
	(void)printf("native-queries fdflags=%d access=%x errno=%d fstat=%d\n", descriptor_flags, (unsigned)access_flags, flags_error, inspected);

	/* Runs the production native reservation export on this independently owned live buffer. */
	exported = 999;
	errno = 0;
	synchronization = compat_dma_sync_export(probe.fd, COMPAT_DMA_SYNC_WRITE, &exported);
	native_error = errno;

	/* Reports real kernel observations even when synchronization is unsupported. */
	(void)printf("native-buffer fd=%d fdflags=%x access=%x mode=%x sync=%d errno=%d output=%d\n",
		     probe.fd, descriptor_flags, (unsigned)access_flags, (unsigned)(access_flags & O_ACCMODE),
		     synchronization, native_error, exported);

	/* A successful native export would transfer a separate owned sync descriptor. */
	if (synchronization == 0 && exported >= 0) {
		inspected = close(exported);
		if (inspected != 0) {
			forge_cleanup(&probe);
			return 1;
		}
	}

	/* Retires the original native buffer only after every observation is complete. */
	forge_cleanup(&probe);

	/* Succeeded: the actual native file and production ioctl result were observed. */
	return 0;
}
