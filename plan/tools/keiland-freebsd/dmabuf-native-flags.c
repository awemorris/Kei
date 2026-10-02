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
	int pipes[2];
	int closed;
	ssize_t bytes;
	char byte;

	/* Owns partial exporter cleanup before the first allocation. */
	memset(&probe, 0, sizeof(probe));
	probe.fd = -1;
	pipes[0] = -1;
	pipes[1] = -1;
	status = forge_image(&probe);
	if (status != VK_SUCCESS) {
		inspected = 1;
		goto cleanup;
	}

	/* Requires a live descriptor before interpreting its unusual access representation. */
	descriptor_flags = fcntl(probe.fd, F_GETFD);
	if (descriptor_flags < 0) {
		inspected = 1;
		goto cleanup;
	}

	/* Distinguishes FreeBSD's successful minus-one flags from an actual query failure. */
	errno = 0;
	access_flags = fcntl(probe.fd, F_GETFL);
	flags_error = errno;
	if (flags_error != 0) {
		inspected = 1;
		goto cleanup;
	}

	/* Checks native descriptor presence without trusting the driver's unfinished stat fields. */
	memset(&attributes, 0, sizeof(attributes));
	inspected = fstat(probe.fd, &attributes);
	if (inspected != 0) {
		inspected = 1;
		goto cleanup;
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
		     probe.fd,
		     descriptor_flags,
		     (unsigned)access_flags,
		     (unsigned)(access_flags & O_ACCMODE),
		     synchronization,
		     native_error,
		     exported);

	/* The exact live zeroaccess fingerprint must select unavailable transport without output mutation. */
	if (access_flags == -1) {
		/* Rejects an altered output or a refusal outside the supported native fallback. */
		if (synchronization != -1 ||
		    native_error != ENOTTY ||
		    exported != 999) {
			inspected = 1;
			goto cleanup;
		}

		/* A real borrowed completion peer remains owned by the caller after the unsupported import. */
		inspected = pipe(pipes);
		if (inspected != 0) {
			inspected = 1;
			goto cleanup;
		}

		/* Recognizes only a live completion descriptor on the unavailable buffer transport. */
		synchronization = compat_dma_sync_import(probe.fd, COMPAT_DMA_SYNC_WRITE, pipes[1]);
		native_error = errno;
		if (synchronization != -1 || native_error != ENOTTY) {
			inspected = 1;
			goto cleanup;
		}

		/* Writes through the original completion descriptor to prove it was not consumed. */
		bytes = write(pipes[1], "S", 1);
		if (bytes != 1) {
			inspected = 1;
			goto cleanup;
		}

		/* Receives the same byte through the original kernel pipe. */
		bytes = read(pipes[0], &byte, 1);
		if (bytes != 1 || byte != 'S') {
			inspected = 1;
			goto cleanup;
		}

		/* An invalid completion descriptor cannot be reclassified as a capability result. */
		synchronization = compat_dma_sync_import(probe.fd, COMPAT_DMA_SYNC_WRITE, -1);
		native_error = errno;
		if (synchronization != -1 || native_error != EBADF) {
			inspected = 1;
			goto cleanup;
		}

		/* A formerly live completion descriptor also retains its original invalid-fd diagnostic. */
		closed = pipes[1];
		inspected = close(pipes[1]);
		pipes[1] = -1;
		if (inspected != 0) {
			inspected = 1;
			goto cleanup;
		}

		/* Keeps a closed completion distinguishable from a live unsupported buffer. */
		synchronization = compat_dma_sync_import(probe.fd, COMPAT_DMA_SYNC_WRITE, closed);
		native_error = errno;
		if (synchronization != -1 || native_error != EBADF) {
			inspected = 1;
			goto cleanup;
		}

		/* Retires the remaining peer after its borrowed ownership was checked. */
		inspected = close(pipes[0]);
		pipes[0] = -1;
		if (inspected != 0) {
			inspected = 1;
			goto cleanup;
		}

		/* Reports the real GPU-buffer capability and completion-descriptor boundary. */
		(void)puts("PASS live GPU zeroaccess export/import ENOTTY, unchanged output, borrowed fd, invalid completion EBADF");
	}

	/* A successful native export would transfer a separate owned sync descriptor. */
	if (synchronization == 0 && exported >= 0) {
		inspected = close(exported);
		if (inspected != 0) {
			inspected = 1;
			goto cleanup;
		}
	}

	/* All required observations passed before the shared lifetime cleanup. */
	inspected = 0;

cleanup:
	/* Failure retires every pipe peer admitted before the original GPU exporter. */
	if (pipes[0] >= 0)
		(void)close(pipes[0]);

	/* A peer already closed by the positive ownership check must not be closed twice. */
	if (pipes[1] >= 0)
		(void)close(pipes[1]);

	/* The exporter owns all partial Vulkan handles even when an observation fails. */
	forge_cleanup(&probe);

	/* Reports a rejected observation after retiring its independently owned resources. */
	if (inspected != 0)
		return 1;

	/* Succeeded: the actual native file and production ioctl result were observed. */
	return 0;
}
