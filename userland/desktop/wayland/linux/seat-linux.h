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

/* Private backends keep device ownership inside the Linux OS boundary. */
int zwl_linux_direct_seat_open(struct zwl_server *server);
void zwl_linux_direct_seat_close(struct zwl_server *server);
int zwl_linux_direct_device_open(struct zwl_server *server, const char *path);
void zwl_linux_direct_device_close(struct zwl_server *server, int descriptor);
int zwl_linux_direct_drm_fd(void);
const char *zwl_linux_direct_drm_path(void);
int zwl_linux_direct_seat_paused(void);
int zwl_linux_logind_seat_open(struct zwl_server *server);
void zwl_linux_logind_seat_close(struct zwl_server *server);
int zwl_linux_logind_device_open(struct zwl_server *server, const char *path);
void zwl_linux_logind_device_close(struct zwl_server *server, int descriptor);
int zwl_linux_logind_drm_fd(void);
const char *zwl_linux_logind_drm_path(void);
int zwl_linux_logind_seat_paused(void);
int zwl_linux_logind_poll_fd(void);
int zwl_linux_logind_dispatch(struct zwl_server *server);

#endif
