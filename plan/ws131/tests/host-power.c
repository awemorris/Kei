/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of libkeiland-backend's power (ws131-p005).
 *
 * Built twice by host-power.sh: with libkeiland-backend-zedbsd's
 * power-zedbsd.c (HOST_POWER_ZEDBSD), where the login screen's descriptor
 * is one end of a socket pair standing for sessiond, and with the shared
 * unsupported implementation Linux and FreeBSD use.  Prints
 * "host-power: N/M passed" and exits 0 only when every check passed.
 *
 *   sh plan/ws131/tests/host-power.sh
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void check(int passed, const char *what);
static struct kl_backend *open_backend(int descriptor);
static void test_unsupported(void);
#ifdef HOST_POWER_ZEDBSD
static void test_login_screen(void);
static void test_session(void);
#endif

static unsigned checks_run;
static unsigned checks_passed;

/*
 * Runs the checks of the implementation this build has.
 */
int
main(
	void)
{
	/* Every implementation refuses without a backend. */
	test_unsupported();

#ifdef HOST_POWER_ZEDBSD
	/* zedBSD: the login screen asks sessiond, a session asks nothing. */
	test_login_screen();
	test_session();
#endif

	/* The summary. */
	printf("host-power: %u/%u passed\n", checks_passed, checks_run);
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
	printf("%s: %s\n", passed ? "ok" : "FAIL", what);
}

/* Opens a backend whose login screen's descriptor is descriptor (-1 for a session). */
static struct kl_backend *
open_backend(
	int descriptor)
{
	struct kl_backend_options options;
	struct kl_backend_host host;
	struct kl_backend *backend;
	int error;

	/* No callback is needed by the power. */
	memset(&options, 0, sizeof(options));
	memset(&host, 0, sizeof(host));
	options.greeter_descriptor = descriptor;
	error = kl_backend_open(&options, &host, &backend);
	if (error != 0)
		return NULL;

	/* Succeeded: the caller closes it. */
	return backend;
}

/* The answers without a backend, and what an implementation without power answers. */
static void
test_unsupported(
	void)
{
	struct kl_backend_power_state state;
	struct kl_backend *backend;
	int error;

	/* Without a backend: EINVAL. */
	error = kl_backend_power_get_state(NULL, &state);
	check(error == EINVAL, "get_state without a backend is EINVAL");
	error = kl_backend_power_action(NULL, KL_BACKEND_POWER_REBOOT);
	check(error == EINVAL, "action without a backend is EINVAL");

#ifndef HOST_POWER_ZEDBSD
	/* Linux and FreeBSD: nothing known, nothing offered, every action ENOTSUP. */
	backend = open_backend(-1);
	check(backend != NULL, "the backend opens");
	if (backend == NULL)
		return;
	memset(&state, 0xff, sizeof(state));
	error = kl_backend_power_get_state(backend, &state);
	check(error == 0 && state.actions == 0U && state.source == KL_BACKEND_POWER_SOURCE_UNKNOWN &&
	    state.percent == -1, "unsupported: no action, the source unknown");
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_POWEROFF);
	check(error == ENOTSUP, "unsupported: poweroff is ENOTSUP");
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_SUSPEND);
	check(error == ENOTSUP, "unsupported: suspend is ENOTSUP");
	kl_backend_close(backend);
#else
	(void)backend;
#endif
}

#ifdef HOST_POWER_ZEDBSD
/* The login screen: Shut Down and Restart offered, one request written whole, then EBUSY. */
static void
test_login_screen(
	void)
{
	struct kl_backend_power_state state;
	struct kl_backend *backend;
	char line[64];
	ssize_t count;
	int ends[2];
	int error;

	/* sessiond's end and the login screen's end. */
	error = socketpair(AF_UNIX, SOCK_STREAM, 0, ends);
	check(error == 0, "a socket pair stands for sessiond");
	if (error != 0)
		return;
	backend = open_backend(ends[1]);
	check(backend != NULL, "the login screen's backend opens");
	if (backend == NULL)
		return;

	/* Shut Down and Restart, not Suspend. */
	error = kl_backend_power_get_state(backend, &state);
	check(error == 0 && state.actions == (KL_BACKEND_POWER_ACTION_BIT(KL_BACKEND_POWER_POWEROFF) |
	    KL_BACKEND_POWER_ACTION_BIT(KL_BACKEND_POWER_REBOOT)), "login screen: poweroff and reboot offered");
	check(state.source == KL_BACKEND_POWER_SOURCE_UNKNOWN && state.percent == -1, "login screen: the source unknown");
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_SUSPEND);
	check(error == ENOTSUP, "login screen: suspend is ENOTSUP");
	error = kl_backend_power_action(backend, 40U);
	check(error == ENOTSUP, "login screen: an unknown action is ENOTSUP");

	/* Restart: sessiond reads the line POWER reboot. */
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_REBOOT);
	check(error == 0, "login screen: reboot is asked");
	memset(line, 0, sizeof(line));
	count = read(ends[0], line, sizeof(line) - 1U);
	check(count == 13 && strcmp(line, "POWER reboot\n") == 0, "sessiond reads POWER reboot");

	/* A second action while the machine ends: EBUSY, nothing written. */
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_POWEROFF);
	check(error == EBUSY, "login screen: a second action is EBUSY");
	kl_backend_close(backend);

	/* Shut Down on a new backend: POWER poweroff. */
	backend = open_backend(ends[1]);
	check(backend != NULL, "a second login screen's backend opens");
	if (backend != NULL) {
		error = kl_backend_power_action(backend, KL_BACKEND_POWER_POWEROFF);
		memset(line, 0, sizeof(line));
		count = read(ends[0], line, sizeof(line) - 1U);
		check(error == 0 && count == 15 && strcmp(line, "POWER poweroff\n") == 0, "sessiond reads POWER poweroff");
		kl_backend_close(backend);
	}

	/* sessiond gone: the error of the write. */
	(void)close(ends[0]);
	backend = open_backend(ends[1]);
	if (backend != NULL) {
		error = kl_backend_power_action(backend, KL_BACKEND_POWER_REBOOT);
		check(error == EPIPE, "sessiond gone: the write's EPIPE");
		kl_backend_close(backend);
	}
	(void)close(ends[1]);
}

/* A session: sessiond takes no power request on its descriptor, so nothing is offered. */
static void
test_session(
	void)
{
	struct kl_backend_power_state state;
	struct kl_backend *backend;
	int error;

	/* No login screen's descriptor. */
	backend = open_backend(-1);
	check(backend != NULL, "the session's backend opens");
	if (backend == NULL)
		return;
	error = kl_backend_power_get_state(backend, &state);
	check(error == 0 && state.actions == 0U, "session: no action offered");
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_POWEROFF);
	check(error == ENOTSUP, "session: poweroff is ENOTSUP");
	kl_backend_close(backend);
}
#endif
