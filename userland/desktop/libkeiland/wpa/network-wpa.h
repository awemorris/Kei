/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Private control connections shared by the watch and credential operations.
 */
#ifndef KEILAND_NETWORK_WPA_H
#define KEILAND_NETWORK_WPA_H
#include <keiland.h>
#include <stdint.h>

#define KWPA_REPLY_MAX 4096U

/* One connection owns its private directory and socket until kwpa_close. */
struct kwpa_socket {
	int fd;
	char directory[64];
	char path[108];
	char interface[KEILAND_NETWORK_NAME_MAX];
};

int kwpa_open(struct kwpa_socket *connection, const char *interface);
void kwpa_close(struct kwpa_socket *connection);
int kwpa_send(struct kwpa_socket *connection, const char *command);
int kwpa_read(struct kwpa_socket *connection, char *reply, size_t capacity);
int kwpa_call(struct kwpa_socket *connection, const char *command, char *reply, size_t capacity);
int kwpa_ok(const char *reply);
void kwpa_decode(const char *encoded, char *ssid, size_t capacity);
int kwpa_profile(char *reply, const char *ssid, unsigned *id);
uint64_t kwpa_milliseconds(void);
int kwpa_radio(const char *interface, unsigned enabled);
void kwpa_links(struct keiland_network_state *state);
#endif
