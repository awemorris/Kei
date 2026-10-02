/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks synchronization failure ownership against actual native pipe descriptors.
 */

#include "userland/desktop/libvulkan-compat/dma-sync.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Checks that unsupported and invalid native synchronization never consumes caller fds.
 */
int
main(
	void)
{
	int pipes[2];
	int exported;
	int error;
	int flags;
	ssize_t bytes;
	char byte;

	/* Uses real kernel descriptors rather than an ioctl substitute. */
	error = pipe(pipes);
	if (error != 0)
		return 1;

	/* An unsupported buffer must leave the caller's output untouched. */
	exported = 123;
	error = compat_dma_sync_export(pipes[0], COMPAT_DMA_SYNC_WRITE, &exported);
	if (error != -1 || errno != ENOTTY)
		return 1;

	/* A rejected export cannot publish a new descriptor identity. */
	if (exported != 123)
		return 1;

	/* A rejected import must retain the borrowed sync descriptor. */
	error = compat_dma_sync_import(pipes[0], COMPAT_DMA_SYNC_WRITE, pipes[1]);
	if (error != -1 || errno != ENOTTY)
		return 1;

	/* Requires the original sync descriptor to remain open after the rejection. */
	flags = fcntl(pipes[1], F_GETFD);
	if (flags < 0)
		return 1;

	/* Confirms the borrowed descriptor still names the original pipe writer. */
	bytes = write(pipes[1], "S", 1);
	if (bytes != 1)
		return 1;

	/* Confirms the buffer descriptor retains the original pipe reader. */
	bytes = read(pipes[0], &byte, 1);
	if (bytes != 1 || byte != 'S')
		return 1;

	/* An invalid buffer must preserve the kernel's distinct EBADF diagnostic. */
	error = compat_dma_sync_export(-1, COMPAT_DMA_SYNC_READ, &exported);
	if (error != -1 || errno != EBADF)
		return 1;

	/* An invalid export also leaves the caller's output untouched. */
	if (exported != 123)
		return 1;

	/* An invalid import cannot close its borrowed descriptor while reporting EBADF. */
	error = compat_dma_sync_import(-1, COMPAT_DMA_SYNC_READ, pipes[1]);
	if (error != -1 || errno != EBADF)
		return 1;

	/* Checks ownership separately after the invalid-buffer path. */
	flags = fcntl(pipes[1], F_GETFD);
	if (flags < 0)
		return 1;

	/* Releases the probe's descriptors after every ownership assertion. */
	error = close(pipes[0]);
	if (error != 0)
		return 1;

	/* Retires the independently owned completion peer after its reader was closed. */
	error = close(pipes[1]);
	if (error != 0)
		return 1;

	/* Publishes the actual failure contract without claiming successful DMA-BUF synchronization. */
	(void)puts("sync-rejected: PASS errno/output/fd ownership; actual GPU synchronization untested");

	/* Succeeded: rejected native ioctls preserved the caller's descriptors and diagnostics. */
	return 0;
}
