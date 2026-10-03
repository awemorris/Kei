/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercises the public bus reader with independently marshaled fragmented frames. */
#include "userland/desktop/libkeiland-backend-linux/dbus-linux.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int file_count(void);
static int receive_signal(struct dbus_reply *message, void *data);

/*
 * Checks signal/reply interleaving and closes rights on refused or partial frames.
 */
int
main(
	int argc,
	char **argv)
{
	struct linux_dbus bus;
	struct dbus_reply *reply;
	uint32_t number;
	unsigned signals;
	unsigned pass;
	int expected;
	int before;
	int after;
	int error;
	int same;

	/* The harness supplies one inherited socket, never a production redirection option. */
	if (argc != 3)
		return 1;
	memset(&bus, 0, sizeof(bus));
	bus.fd = atoi(argv[1]);
	expected = atoi(argv[2]);
	before = file_count();
	if (before < 0)
		return 2;
	signals = 0;

	/* A successful call must retain signals that arrived before its matching response. */
	if (expected == 0) {
		error = dbus_call(&bus, "org.example.Peer", "/peer", "org.example.Peer", "Probe", "", NULL, 0, &reply);
		if (error != 0)
			return 3;
		same = strcmp(reply->signature, "u");
		if (same != 0)
			return 4;
		error = dbus_read_number(reply, &number);
		if (error != 0 || number != 1337)
			return 5;
		dbus_reply_free(reply);
	}

	/* Finite retries allow byte-by-byte fragmentation without hiding a blocking reader. */
	error = 0;
	for (pass = 0; pass < 1000; pass++) {
		error = dbus_dispatch(&bus, receive_signal, &signals);
		if (error != 0 || signals == 2)
			break;
		(void)usleep(1000);
	}

	/* Malformed frames must report the expected failure before any signal is exposed. */
	if (error != expected)
		return 6;
	if (expected == 0 && signals != 2)
		return 7;
	dbus_close(&bus);
	after = file_count();
	if (after != before - 1)
		return 8;

	/* Succeeded: only the bus socket disappeared; all received rights were returned. */
	printf("dbus-wire: PASS expected=%d signals=%u fd_before=%d fd_after=%d\n", expected, signals, before, after);
	return 0;
}

/* Counts live files independently of the bus ownership implementation. */
static int
file_count(
	void)
{
	DIR *directory;
	struct dirent *entry;
	int count;

	/* The directory's own descriptor appears in both samples and cancels out. */
	directory = opendir("/proc/self/fd");
	if (directory == NULL)
		return -1;
	count = 0;

	/* Dot entries are directory names rather than live process descriptors. */
	for (;;) {
		entry = readdir(directory);
		if (entry == NULL)
			break;
		if (entry->d_name[0] != '.')
			count++;
	}

	/* Succeeded: the sample excludes no received or partially decoded file. */
	(void)closedir(directory);
	return count;
}

/* Validates that each signal receives its own CLOEXEC payload file. */
static int
receive_signal(
	struct dbus_reply *message,
	void *data)
{
	unsigned *signals;
	uint32_t major;
	uint32_t minor;
	uint32_t index;
	char payload[8];
	ssize_t size;
	int descriptor;
	int flags;
	int same;
	int error;

	/* Both independent signal frames carry one resume handle and exactly three scalars. */
	signals = data;
	same = strcmp(message->signature, "uuh");
	if (same != 0)
		return EPROTO;
	error = dbus_read_number(message, &major);
	if (error != 0)
		return error;
	error = dbus_read_number(message, &minor);
	if (error != 0)
		return error;
	error = dbus_read_number(message, &index);
	if (error != 0)
		return error;
	if (major != 13 ||
	    minor != *signals + 64 ||
	    index != 0)
		return EPROTO;
	descriptor = dbus_take_fd(message, index);
	if (descriptor < 0)
		return EPROTO;
	flags = fcntl(descriptor, F_GETFD);
	if (flags < 0 || (flags & FD_CLOEXEC) == 0) {
		(void)close(descriptor);
		return EPROTO;
	}

	/* Different payload bytes reveal any accidental transfer across frame boundaries. */
	size = read(descriptor, payload, sizeof(payload));
	(void)close(descriptor);
	if (size != 1 || payload[0] != (char)('a' + *signals))
		return EPROTO;
	(*signals)++;

	/* Succeeded: this frame's one file was consumed and closed independently. */
	return 0;
}
