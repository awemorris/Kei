/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of libkeiland-backend's Linux seat choice, its direct seat
 * and the power's refusals (ws131-p006).  The direct seat opens /dev/null as
 * its primary node and /dev/zero as an input; standard input is not a
 * virtual terminal here, so the console is left alone.  logind's seat (its
 * signals, the pause and the resume) needs a session and is checked on a
 * Linux guest, not here.  Prints "host-seat-linux: N/M passed".
 *
 *   sh plan/ws131/tests/host-seat-linux.sh
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void check(int passed, const char *what);

static unsigned checks_run;
static unsigned checks_passed;

/*
 * Chooses the seat by name, opens the direct one, and asks the power for
 * what it refuses without the bus.
 */
int
main(
	void)
{
	struct kl_backend_options options;
	struct kl_backend_host host;
	struct kl_backend *backend;
	int descriptor;
	int error;

	/* A backend without callbacks. */
	memset(&options, 0, sizeof(options));
	memset(&host, 0, sizeof(host));
	options.greeter_descriptor = -1;
	options.session_descriptor = -1;
	error = kl_backend_open(&options, &host, &backend);
	check(error == 0, "the backend opens");
	if (error != 0)
		return 1;

	/* An unknown seat name refuses rather than falling back to root access. */
	(void)setenv("KEILAND_SEAT", "elsewhere", 1);
	error = kl_backend_seat_open(backend);
	check(error == EINVAL, "an unknown seat is EINVAL");
	kl_backend_seat_close(backend);

	/* A relative primary node refuses. */
	(void)setenv("KEILAND_SEAT", "direct", 1);
	(void)setenv("KEILAND_DRM_DEVICE", "dev/null", 1);
	error = kl_backend_seat_open(backend);
	check(error == EINVAL, "a relative primary node is EINVAL");
	kl_backend_seat_close(backend);

	/* The direct seat with /dev/null for the primary node. */
	(void)setenv("KEILAND_DRM_DEVICE", "/dev/null", 1);
	error = kl_backend_seat_open(backend);
	check(error == 0, "the direct seat opens");
	check(kl_backend_seat_primary_fd(backend) >= 0 && strcmp(kl_backend_seat_primary_path(backend), "/dev/null") == 0, "the primary node is taken");
	check(kl_backend_seat_paused(backend) == 0 && kl_backend_poll_count(backend) == 0U, "the direct seat never pauses and polls nothing");

	/* An input opens directly, nonblocking; a revoked one is not kept. */
	descriptor = kl_backend_seat_device_open(backend, "/dev/zero");
	check(descriptor >= 0, "an input opens directly");
	check(kl_backend_seat_device_revoked(backend, descriptor) == 0, "the direct seat keeps no revoked input");
	kl_backend_seat_device_close(backend, descriptor);

	/* Close returns the primary node. */
	kl_backend_seat_close(backend);
	check(kl_backend_seat_primary_fd(backend) < 0, "close returns the primary node");

	/* The power refuses an unknown action before asking anyone. */
	error = kl_backend_power_action(backend, 40U);
	check(error == ENOTSUP, "an unknown power action is ENOTSUP");
	error = kl_backend_power_action(NULL, KL_BACKEND_POWER_REBOOT);
	check(error == EINVAL, "a power action without a backend is EINVAL");

	/* A Linux session has no session manager. */
	check(kl_backend_session_managed(backend) == 0 && kl_backend_session_ready(backend) == ENOTSUP, "no session manager on Linux");
	kl_backend_close(backend);

	/* The summary. */
	printf("host-seat-linux: %u/%u passed\n", checks_passed, checks_run);
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
