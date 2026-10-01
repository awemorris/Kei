/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Shares Linux seat device ownership between OS and evdev modules. */
#ifndef ZWL_SEAT_LINUX_H
#define ZWL_SEAT_LINUX_H

struct zwl_server;

/* Opens the root seat before Vulkan inquiry can open the primary node. */
int zwl_linux_seat_open(struct zwl_server *server);
/* Returns every seat-owned display resource. */
void zwl_linux_seat_close(struct zwl_server *server);
/* Opens an independently owned nonblocking input descriptor. */
int zwl_linux_device_open(struct zwl_server *server, const char *path);
/* Closes one input descriptor and its seat ownership. */
void zwl_linux_device_close(struct zwl_server *server, int descriptor);
/* Supplies the acquired primary-node descriptor to Vulkan. */
int zwl_linux_drm_fd(void);
/* Supplies the exact card pathname used by the seat. */
const char *zwl_linux_drm_path(void);
/* Reports whether device access is paused by the seat service. */
int zwl_linux_seat_paused(void);

#endif
