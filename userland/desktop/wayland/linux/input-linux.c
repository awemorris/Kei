/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Accesses Linux evdev through seat-owned descriptors with monotonic event timestamps. */
#include "../zwl.h"
#include "seat-linux.h"
#include <time.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* The directory whose eventN nodes are the evdev devices. */
#define INPUT_DIRECTORY "/dev/input"

/* The one Linux input seat borrows this server until common service cleanup. */
static struct zwl_server *input_server;

static int event_node_name(const char *name);
static int device_open(struct zwl_server *server, const char *path);
static void probe_device(struct zwl_server *server, const char *path);
static int read_capabilities(int descriptor, struct zwl_input_caps *capabilities);

/*
 * Opens every evdev pointer and keyboard not already open.
 *
 * A missing directory leaves the seat without devices; it is not an error.
 * The event loop calls this periodically so late devices are picked up.
 */
void
zwl_input_scan(
	struct zwl_server *server)
{
	DIR *directory;
	struct dirent *entry;
	char path[ZWL_INPUT_PATH_MAX];
	int length;
	int valid;
	int open_already;
	int paused;

	/* The next rescan is due one period from now. */
	input_server = server;
	server->input_scan_time = zwl_milliseconds();

	/* A service-paused seat cannot acquire newly discovered input devices. */
	paused = zwl_linux_seat_paused();
	if (paused != 0)
		return;

	/* Without the directory there is nothing to read. */
	directory = opendir(INPUT_DIRECTORY);
	if (directory == NULL)
		return;

	/* Every eventN entry is a candidate device. */
	while (1) {
		/* The end of the directory ends the scan. */
		entry = readdir(directory);
		if (entry == NULL)
			break;

		/* Only eventN nodes speak evdev. */
		valid = event_node_name(entry->d_name);
		if (!valid)
			continue;

		/* An overlong name cannot be an ordinary device node. */
		length = snprintf(path, sizeof(path), "%s/%s", INPUT_DIRECTORY, entry->d_name);
		if (length < 0 || (size_t)length >= sizeof(path))
			continue;

		/* A node already being read is left alone. */
		open_already = device_open(server, path);
		if (open_already)
			continue;

		/* Classify the node and keep it if it is a pointer or a keyboard. */
		probe_device(server, path);
	}

	/* The directory stream is no longer needed. */
	closedir(directory);

	/* Succeeded: every present pointer and keyboard is open. */
	return;
}

/*
 * Reads an input axis range.
 *
 * A nonnegative ioctl response is success; failures return an errno value.
 */
int
zwl_input_device_absinfo(
	int descriptor,
	uint32_t axis,
	struct input_absinfo *info)
{
	int error;

	/* Queries the device's metadata. */
	error = ioctl(descriptor, EVIOCGABS(axis), info);
	if (error < 0)
		return errno;

	/* Succeeded: the device supplied the metadata. */
	return 0;
}

/*
 * Reads an input device's name.
 *
 * A nonnegative ioctl response is success; failures return an errno value.
 */
int
zwl_input_device_name(
	int descriptor,
	char *name,
	size_t size)
{
	int error;

	/* Queries the device's metadata. */
	error = ioctl(descriptor, EVIOCGNAME(size), name);
	if (error < 0)
		return errno;

	/* Succeeded: the device supplied the metadata. */
	return 0;
}

/*
 * Reads an input device's identity.
 *
 * A nonnegative ioctl response is success; failures return an errno value.
 */
int
zwl_input_device_id(
	int descriptor,
	struct input_id *id)
{
	int error;

	/* Queries the device's metadata. */
	error = ioctl(descriptor, EVIOCGID, id);
	if (error < 0)
		return errno;

	/* Succeeded: the device supplied the metadata. */
	return 0;
}

/*
 * Reads whole input events without blocking.
 *
 * EOF returns zero; failures preserve read's errno, or use EIO for torn events.
 */
ssize_t
zwl_input_device_read(
	int descriptor,
	struct input_event *events,
	size_t capacity)
{
	ssize_t bytes;
	int error;
	int retained;

	/* Reads as many whole events as the buffer holds. */
	bytes = read(descriptor, events, capacity * sizeof(events[0]));
	if (bytes < 0) {
		/* Logind can revoke the kernel file before its ordered bus notification arrives. */
		error = errno;
		if (error == ENODEV) {
			retained = zwl_linux_device_revoked(input_server, descriptor);
			if (retained != 0)
				error = EAGAIN;
		}

		/* A retained lease waits for pause/resume; ordinary direct-device failures still close. */
		errno = error;
		return -1;
	}

	/* Reports the end of the device. */
	if (bytes == 0)
		return 0;

	/* Refuses an incomplete evdev event. */
	if (((size_t)bytes % sizeof(events[0])) != 0) {
		errno = EIO;
		return -1;
	}

	/* Succeeded: returns the number of complete events. */
	return bytes / (ssize_t)sizeof(events[0]);
}

/*
 * Closes an input device descriptor.
 *
 * The seat module returns both descriptor and service ownership.
 */
void
zwl_input_device_close(
	struct zwl_server *server,
	int descriptor)
{
	/* Releases the node through its seat owner. */
	zwl_linux_device_close(server, descriptor);

	/* Succeeded: the descriptor is no longer owned by the seat. */
	return;
}

/* Reports whether a directory entry is named eventN. */
static int
event_node_name(
	const char *name)
{
	const char *cursor;
	int prefix;

	/* The name starts with "event" and has at least one more character. */
	prefix = strncmp(name, "event", 5);
	if (prefix != 0 || name[5] == '\0')
		return 0;

	/* Everything after the prefix is a decimal digit. */
	for (cursor = name + 5; *cursor != '\0'; cursor++) {
		/* Any other character makes it some other kind of node. */
		if (*cursor < '0' || *cursor > '9')
			return 0;
	}

	/* Succeeded: the entry is an evdev node. */
	return 1;
}

/* Reports whether a device node is already open in the table. */
static int
device_open(
	struct zwl_server *server,
	const char *path)
{
	unsigned index;
	int same;

	/* Compare the path with every slot in use. */
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		/* A free slot names no device. */
		if (!server->inputs[index].live)
			continue;

		/* The same path means the same node. */
		same = strcmp(server->inputs[index].path, path);
		if (same == 0)
			break;
	}

	/* Refuses an entry not represented by any live descriptor. */
	if (index == ZWL_INPUT_MAX)
		return 0;

	/* Succeeded: a live descriptor already owns this device node. */
	return 1;
}

/* Opens one node and hands its capability bitmaps to the seat. */
static void
probe_device(
	struct zwl_server *server,
	const char *path)
{
	struct zwl_input_caps capabilities;
	int clock_id;
	int descriptor;
	int error;

	/* Opens a nonblocking descriptor that children cannot inherit. */
	descriptor = zwl_linux_device_open(server, path);
	if (descriptor < 0)
		return;

	/* Matches the common compositor's monotonic clock before interpreting event timestamps. */
	clock_id = CLOCK_MONOTONIC;
	error = ioctl(descriptor, EVIOCSCLOCKID, &clock_id);
	if (error != 0) {
		zwl_linux_device_close(server, descriptor);
		return;
	}

	/* Reads the bits that determine the device's role. */
	error = read_capabilities(descriptor, &capabilities);
	if (error != 0) {
		zwl_linux_device_close(server, descriptor);
		return;
	}

	/* Transfers the descriptor to the seat's classification. */
	zwl_input_probe(server, descriptor, path, &capabilities);

	/* Succeeded: the seat has kept or closed the descriptor. */
	return;
}

/* Reads the event, key, relative and absolute capability bitmaps of one node. */
static int
read_capabilities(
	int descriptor,
	struct zwl_input_caps *capabilities)
{
	int error;

	/* Absent bitmaps read as empty. */
	memset(capabilities, 0, sizeof(*capabilities));

	/* The event types the node can produce. */
	error = ioctl(descriptor, EVIOCGBIT(0, sizeof(capabilities->event)), capabilities->event);
	if (error < 0)
		return errno;

	/* The key and button codes the node can produce. */
	error = ioctl(descriptor, EVIOCGBIT(EV_KEY, sizeof(capabilities->key)), capabilities->key);
	if (error < 0)
		return errno;

	/* The relative axes the node can produce. */
	error = ioctl(descriptor, EVIOCGBIT(EV_REL, sizeof(capabilities->relative)), capabilities->relative);
	if (error < 0)
		return errno;

	/* The absolute axes the node can produce. */
	error = ioctl(descriptor, EVIOCGBIT(EV_ABS, sizeof(capabilities->absolute)), capabilities->absolute);
	if (error < 0)
		return errno;

	/* Succeeded: the four bitmaps describe the node. */
	return 0;
}
