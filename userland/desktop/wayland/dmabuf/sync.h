/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Defines native reservation export ownership for the shared dma-buf protocol. */
#ifndef ZWL_DMABUF_SYNC_H
#define ZWL_DMABUF_SYNC_H

/* Borrows buffer_fd; publishes a caller-owned sync fd only on success. Returns -1/errno on failure. */
int zwl_dmabuf_export_read(int buffer_fd, int *sync_fd);

#endif
