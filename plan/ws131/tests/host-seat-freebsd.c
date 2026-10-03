/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of libkeiland-backend's FreeBSD seat (ws131-p006):
 * seat-freebsd.c against a pretend libseat (below) that records every call
 * and delivers the activations and the withdrawals the test queues.  It
 * checks the order the seat keeps when seatd disables the session: the
 * compositor stops drawing, forgets each input, every lease closes, and
 * only then is seatd told; and that an activation reopens the primary node
 * before the compositor hears it.  FreeBSD itself is not built here (the
 * FreeBSD build is deferred, 2026-10-03 user); this is the seat's logic on
 * the host.  Prints "host-seat-freebsd: N/M passed".
 *
 *   sh plan/ws131/tests/host-seat-freebsd.sh
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <libseat.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void check(int passed, const char *what);
static void record(const char *event);
static void heard_paused(void *data);
static void heard_resumed(void *data);
static void heard_gone(void *data, const char *path);
static void heard_stop(void *data, unsigned reason);

static unsigned checks_run;
static unsigned checks_passed;

/* The calls and callbacks in order, as one line of words. */
static char journal[2048];

/* The pretend seatd: its listener, its data, and what dispatch delivers next (1 enable, 2 disable). */
static const struct libseat_seat_listener *fake_listener;
static void *fake_data;
static int fake_pending;
static int fake_open_devices;
static struct libseat *fake_seat = (struct libseat *)&fake_pending;

/*
 * Opens the seat, an input, then disables and enables it.
 */
int
main(
	void)
{
	struct kl_backend_options options;
	struct kl_backend_host host;
	struct kl_backend *backend;
	struct pollfd descriptor;
	int input;
	int error;

	/* The backend with the seat's callbacks. */
	memset(&options, 0, sizeof(options));
	memset(&host, 0, sizeof(host));
	options.greeter_descriptor = -1;
	options.session_descriptor = -1;
	host.session_paused = heard_paused;
	host.session_resumed = heard_resumed;
	host.input_gone = heard_gone;
	host.session_stop = heard_stop;
	error = kl_backend_open(&options, &host, &backend);
	check(error == 0, "the backend opens");
	if (error != 0)
		return 1;

	/* The first activation comes while opening; the compositor is not told of it. */
	(void)setenv("KEILAND_DRM_DEVICE", "/dev/null", 1);
	fake_pending = 1;
	error = kl_backend_seat_open(backend);
	check(error == 0 && kl_backend_seat_paused(backend) == 0, "the seat opens active");
	check(kl_backend_seat_primary_fd(backend) >= 0 && strcmp(kl_backend_seat_primary_path(backend), "/dev/null") == 0, "the primary node is taken");
	check(strstr(journal, "resumed") == NULL, "the first activation is not a resume");

	/* An input through the seat. */
	input = kl_backend_seat_device_open(backend, "/dev/zero");
	check(input >= 0 && fake_open_devices == 2, "an input opens through seatd");

	/* seatd disables: paused, the input gone, both leases closed, then disable_seat. */
	journal[0] = '\0';
	fake_pending = 2;
	check(kl_backend_poll_count(backend) == 1U, "seatd's descriptor is polled");
	kl_backend_poll_fill(backend, &descriptor);
	descriptor.revents = POLLIN;
	kl_backend_poll_done(backend, &descriptor);
	check(strcmp(journal, "paused gone:/dev/zero close close disable ") == 0, "disable: paused, gone, closes, then seatd hears");
	check(kl_backend_seat_paused(backend) == 1 && kl_backend_seat_primary_fd(backend) < 0 && fake_open_devices == 0, "disabled: nothing open");
	error = kl_backend_seat_device_open(backend, "/dev/zero");
	check(error < 0 && errno == EAGAIN, "no input while disabled (EAGAIN)");

	/* seatd enables: the primary node again, then the compositor hears it. */
	journal[0] = '\0';
	fake_pending = 1;
	kl_backend_poll_done(backend, &descriptor);
	check(strcmp(journal, "open:/dev/null resumed ") == 0, "enable: the primary node, then resumed");
	check(kl_backend_seat_paused(backend) == 0 && kl_backend_seat_primary_fd(backend) >= 0, "active again with a new primary node");

	/* A hung-up service ends the compositor. */
	journal[0] = '\0';
	descriptor.revents = POLLHUP;
	kl_backend_poll_done(backend, &descriptor);
	check(strcmp(journal, "stop:4 ") == 0, "a hung-up seatd: session_stop(LOST)");

	/* Close returns everything. */
	kl_backend_seat_close(backend);
	check(fake_open_devices == 0 && fake_listener == NULL, "close returns every lease and the client");
	kl_backend_close(backend);

	/* The summary. */
	printf("host-seat-freebsd: %u/%u passed\n", checks_passed, checks_run);
	if (checks_passed != checks_run)
		return 1;
	return 0;
}

/* Counts one check and prints it. */
static void
check(
	int passed,
	const char *what)
{
	checks_run++;
	if (passed)
		checks_passed++;
	printf("%s: %s (%s)\n", passed ? "ok" : "FAIL", what, journal);
}

/* Appends a word to the journal. */
static void
record(
	const char *event)
{
	size_t used;

	/* The journal is long enough for the test. */
	used = strlen(journal);
	(void)snprintf(journal + used, sizeof(journal) - used, "%s ", event);
}

static void
heard_paused(
	void *data)
{
	(void)data;
	record("paused");
}

static void
heard_resumed(
	void *data)
{
	(void)data;
	record("resumed");
}

static void
heard_gone(
	void *data,
	const char *path)
{
	char word[64];

	(void)data;
	(void)snprintf(word, sizeof(word), "gone:%s", path);
	record(word);
}

static void
heard_stop(
	void *data,
	unsigned reason)
{
	char word[32];

	(void)data;
	(void)snprintf(word, sizeof(word), "stop:%u", reason);
	record(word);
}

/* The pretend libseat. */
struct libseat *
libseat_open_seat(
	const struct libseat_seat_listener *listener,
	void *userdata)
{
	fake_listener = listener;
	fake_data = userdata;
	return fake_seat;
}

int
libseat_dispatch(
	struct libseat *seat,
	int timeout)
{
	int pending;

	/* Delivers what the test queued. */
	(void)timeout;
	pending = fake_pending;
	fake_pending = 0;
	if (pending == 1)
		fake_listener->enable_seat(seat, fake_data);
	else if (pending == 2)
		fake_listener->disable_seat(seat, fake_data);
	return 0;
}

int
libseat_disable_seat(
	struct libseat *seat)
{
	(void)seat;
	record("disable");
	return 0;
}

int
libseat_close_seat(
	struct libseat *seat)
{
	(void)seat;
	fake_listener = NULL;
	return 0;
}

int
libseat_open_device(
	struct libseat *seat,
	const char *path,
	int *fd)
{
	char word[64];

	(void)seat;
	*fd = open(path, O_RDONLY);
	if (*fd < 0)
		return -1;
	fake_open_devices++;
	(void)snprintf(word, sizeof(word), "open:%s", path);
	record(word);
	return 100 + *fd;
}

int
libseat_close_device(
	struct libseat *seat,
	int device_id)
{
	(void)seat;
	(void)device_id;
	fake_open_devices--;
	record("close");
	return 0;
}

int
libseat_get_fd(
	struct libseat *seat)
{
	(void)seat;
	return 0;
}
