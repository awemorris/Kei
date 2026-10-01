/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Shares native seatd lifetime between the FreeBSD OS adapter and shared evdev. */
#ifndef ZWL_SEAT_FREEBSD_H
#define ZWL_SEAT_FREEBSD_H

struct zwl_server;

/* Connect owns the native service; close tolerates every partial startup. */
int zwl_freebsd_seat_connect(struct zwl_server *server);
void zwl_freebsd_seat_close(struct zwl_server *server);
/* Primary acquisition must precede Vulkan's independent inquiry open. */
int zwl_freebsd_primary_open(struct zwl_server *server);
int zwl_freebsd_primary_fd(void);
const char *zwl_freebsd_primary_path(void);
/* The authority's descriptor remains polled while display and input are paused. */
int zwl_freebsd_seat_poll_fd(void);
int zwl_freebsd_seat_dispatch(struct zwl_server *server);

#endif
