/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks real reservation-export rejection without requiring a physical DMA buffer. */
#include "userland/desktop/wayland/dmabuf/sync.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Verifies rejected exports preserve the caller's buffer and output descriptor ownership.
 */
int
main(
	void)
{
	int descriptors[2];
	int exported;
	int error;
	int saved;
	int flags;

	/* An invalid borrowed descriptor cannot publish a new owned sync descriptor. */
	exported = 123;
	errno = 0;
	error = zwl_dmabuf_export_read(-1, &exported);
	if (error != -1 || errno != EBADF || exported != 123)
		return 1;

	/* Uses an actual non-DMA kernel file rather than mocking a driver response. */
	error = pipe(descriptors);
	if (error != 0)
		return 1;

	/* Unsupported native reservation operations must preserve errno and the borrowed file. */
	exported = 456;
	errno = 0;
	error = zwl_dmabuf_export_read(descriptors[0], &exported);
	saved = errno;
	flags = fcntl(descriptors[0], F_GETFD);
	(void)close(descriptors[0]);
	(void)close(descriptors[1]);
	if (error != -1 ||
	    saved != ENOTTY ||
	    exported != 456 ||
	    flags < 0)
		return 1;

	/* Succeeded: actual rejected exports preserve fd ownership and native errno. */
	(void)printf("PASS native dma-buf export EBADF/ENOTTY/unchanged output/borrowed fd\n");
	return 0;
}
