/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Owns the bounded system-bus connection of the Linux seat and power (libkeiland-backend, ws131-p006). */
#ifndef KL_BACKEND_DBUS_LINUX_H
#define KL_BACKEND_DBUS_LINUX_H
#include <stddef.h>
#include <stdint.h>

#define LINUX_DBUS_MAX 65536U
#define LINUX_DBUS_FDS 16U
#define LINUX_DBUS_SIGNALS 32U

/* One received frame owns its ancillary files until a consumer takes them. */
struct dbus_reply {
	unsigned char bytes[LINUX_DBUS_MAX];
	size_t size;
	size_t body;
	size_t cursor;
	unsigned type;
	uint32_t serial;
	uint32_t reply_serial;
	const char *path;
	const char *interface;
	const char *member;
	const char *signature;
	const char *sender;
	const char *error;
	int fds[LINUX_DBUS_FDS];
	size_t fd_count;
};

/* A connection retains a partial frame and signals interleaved with replies. */
struct linux_dbus {
	int fd;
	uint32_t serial;
	struct dbus_reply *receiving;
	size_t received;
	size_t wanted;
	struct dbus_reply *signals[LINUX_DBUS_SIGNALS];
	size_t signal_count;
};

/* One simple method argument is marshaled according to the supplied signature. */
struct dbus_arg {
	const char *string;
	uint32_t number;
};

typedef int (*dbus_signal_fn)(struct dbus_reply *signal, void *data);

int dbus_open_system(struct linux_dbus *bus);
void dbus_close(struct linux_dbus *bus);
int dbus_call(struct linux_dbus *bus, const char *destination, const char *path, const char *interface, const char *member, const char *signature, const struct dbus_arg *args, size_t count, struct dbus_reply **reply);
int dbus_add_match(struct linux_dbus *bus, const char *rule);
int dbus_fd(const struct linux_dbus *bus);
int dbus_dispatch(struct linux_dbus *bus, dbus_signal_fn callback, void *data);
void dbus_reply_free(struct dbus_reply *reply);
int dbus_read_number(struct dbus_reply *reply, uint32_t *number);
int dbus_read_string(struct dbus_reply *reply, const char **string);
int dbus_take_fd(struct dbus_reply *reply, uint32_t index);
#endif
