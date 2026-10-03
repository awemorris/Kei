/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of libkeiland-backend's session on zedBSD (ws131-p006):
 * session-zedbsd.c against a socket pair standing for sessiond, as the
 * login screen (the greeter's descriptor) and as a session (the control
 * descriptor).  Prints "host-session: N/M passed".
 *
 *   sh plan/ws131/tests/host-session.sh
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void check(int passed, const char *what);
static struct kl_backend *open_backend(int greeter, int session);
static void heard_stop(void *data, unsigned reason);
static void heard_answer(void *data, unsigned request, int error);
static int read_line(int descriptor, const char *expected);
static void test_login_screen(void);
static void test_session(void);
static void test_unanswered(void);

static unsigned checks_run;
static unsigned checks_passed;

/* What the callbacks heard last, and how often. */
static unsigned stop_reason;
static unsigned stop_count;
static unsigned answer_request;
static int answer_error;
static unsigned answer_count;

/* The peer's descriptor the stop callback reads from, to see that RELEASED comes after it. */
static int stop_peer = -1;
static int released_before_stop;

/*
 * Runs the login screen's, the session's and the deadline's checks.
 */
int
main(
	void)
{
	/* A write to a closed peer reports EPIPE rather than ending the test. */
	(void)signal(SIGPIPE, SIG_IGN);

	/* The three cases. */
	test_login_screen();
	test_session();
	test_unanswered();

	/* The summary. */
	printf("host-session: %u/%u passed\n", checks_passed, checks_run);
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

/* Opens a backend with the login screen's and the session's descriptors (-1 for none). */
static struct kl_backend *
open_backend(
	int greeter,
	int session)
{
	struct kl_backend_options options;
	struct kl_backend_host host;
	struct kl_backend *backend;
	int error;

	/* The compositor gives the backend descriptors that do not block. */
	if (greeter >= 0)
		(void)fcntl(greeter, F_SETFL, fcntl(greeter, F_GETFL) | O_NONBLOCK);
	if (session >= 0)
		(void)fcntl(session, F_SETFL, fcntl(session, F_GETFL) | O_NONBLOCK);

	/* The callbacks record what they heard. */
	memset(&options, 0, sizeof(options));
	memset(&host, 0, sizeof(host));
	options.greeter_descriptor = greeter;
	options.session_descriptor = session;
	host.session_stop = heard_stop;
	host.session_answer = heard_answer;
	error = kl_backend_open(&options, &host, &backend);
	if (error != 0)
		return NULL;

	/* Nothing heard yet. */
	stop_count = 0U;
	answer_count = 0U;
	return backend;
}

/* Records a stop, and whether RELEASED had already been written when it came. */
static void
heard_stop(
	void *data,
	unsigned reason)
{
	char byte;
	ssize_t count;

	/* The peer must not have RELEASED yet: the compositor gives the display back first. */
	(void)data;
	stop_reason = reason;
	stop_count++;
	if (stop_peer >= 0) {
		count = recv(stop_peer, &byte, 1U, MSG_DONTWAIT | MSG_PEEK);
		released_before_stop = count > 0;
	}
}

/* Records an answer. */
static void
heard_answer(
	void *data,
	unsigned request,
	int error)
{
	(void)data;
	answer_request = request;
	answer_error = error;
	answer_count++;
}

/* Reads one line from the peer and compares it. */
static int
read_line(
	int descriptor,
	const char *expected)
{
	char line[256];
	ssize_t count;
	int same;

	/* What the backend wrote (the test writes nothing else to this side). */
	memset(line, 0, sizeof(line));
	count = recv(descriptor, line, strlen(expected), MSG_DONTWAIT);
	if (count != (ssize_t)strlen(expected))
		return 0;
	same = strcmp(line, expected);
	return same == 0;
}

/* The login screen: READY and GO, AUTH and its answers, POWER's answer, the end with RELEASED after the stop. */
static void
test_login_screen(
	void)
{
	struct kl_backend *backend;
	int ends[2];
	int error;

	/* sessiond's end (0) and the login screen's (1). */
	error = socketpair(AF_UNIX, SOCK_STREAM, 0, ends);
	check(error == 0, "a socket pair stands for sessiond");
	if (error != 0)
		return;
	backend = open_backend(ends[1], -1);
	check(backend != NULL, "the login screen's backend opens");
	if (backend == NULL)
		return;
	check(kl_backend_session_managed(backend) == 0, "the login screen is not a managed session");

	/* GO already waiting: READY is written and the wait ends at once. */
	(void)write(ends[0], "GO\n", 3U);
	error = kl_backend_session_ready(backend);
	check(error == 0 && read_line(ends[0], "READY\n"), "READY written, GO taken");

	/* AUTH, and a second request while it waits: EBUSY. */
	error = kl_backend_session_authenticate(backend, "kei", "secret");
	check(error == 0 && read_line(ends[0], "AUTH kei secret\n"), "AUTH kei secret written");
	error = kl_backend_session_authenticate(backend, "kei", "again");
	check(error == EBUSY, "a second request while one waits is EBUSY");
	error = kl_backend_session_authenticate(backend, "two words", "x");
	check(error == EINVAL, "a name with a space is EINVAL");
	error = kl_backend_session_unlock(backend, "x");
	check(error == ENOTSUP, "the login screen has no unlock");

	/* FAIL answers AUTH with EACCES. */
	(void)write(ends[0], "FAIL\n", 5U);
	kl_backend_tick(backend, 1000U);
	check(answer_count == 1U && answer_request == KL_BACKEND_SESSION_AUTH && answer_error == EACCES, "FAIL answers AUTH with EACCES");

	/* ERROR and OK, split over two reads. */
	(void)kl_backend_session_authenticate(backend, "kei", "secret");
	(void)read_line(ends[0], "AUTH kei secret\n");
	(void)write(ends[0], "ERR", 3U);
	kl_backend_tick(backend, 1001U);
	check(answer_count == 1U, "half a line answers nothing");
	(void)write(ends[0], "OR\n", 3U);
	kl_backend_tick(backend, 1002U);
	check(answer_count == 2U && answer_error == EIO, "ERROR answers with EIO");

	/* Power's OK is power's answer. */
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_REBOOT);
	check(error == 0 && read_line(ends[0], "POWER reboot\n"), "POWER reboot written");
	(void)write(ends[0], "OK\n", 3U);
	kl_backend_tick(backend, 1003U);
	check(answer_count == 3U && answer_request == KL_BACKEND_SESSION_POWER && answer_error == 0, "OK answers POWER");

	/* A line nobody asked for: request NONE, EPROTO. */
	(void)write(ends[0], "HELLO\n", 6U);
	kl_backend_tick(backend, 1004U);
	check(answer_count == 4U && answer_request == KL_BACKEND_SESSION_NONE && answer_error == EPROTO, "an unasked line is NONE, EPROTO");

	/* sessiond shuts its side down: ENDED, then RELEASED (written after the stop). */
	stop_peer = ends[0];
	released_before_stop = 0;
	(void)shutdown(ends[0], SHUT_WR);
	kl_backend_tick(backend, 1005U);
	check(stop_count == 1U && stop_reason == KL_BACKEND_SESSION_ENDED, "the shut-down side ends the login screen");
	check(!released_before_stop && read_line(ends[0], "RELEASED\n"), "RELEASED comes after the stop");
	stop_peer = -1;

	/* Nothing more is read after the end. */
	kl_backend_tick(backend, 1006U);
	check(stop_count == 1U, "the end is told once");
	kl_backend_close(backend);
	(void)close(ends[0]);
	(void)close(ends[1]);
}

/* A session: LOGOUT and QUIT with RELEASED after the stop, UNLOCK, and sessiond leaving. */
static void
test_session(
	void)
{
	struct kl_backend *backend;
	int ends[2];
	int error;

	/* sessiond's end (0) and the session's (1). */
	error = socketpair(AF_UNIX, SOCK_STREAM, 0, ends);
	if (error != 0)
		return;
	backend = open_backend(-1, ends[1]);
	check(backend != NULL, "the session's backend opens");
	if (backend == NULL)
		return;
	check(kl_backend_session_managed(backend) == 1, "a session sessiond started is managed");
	error = kl_backend_session_authenticate(backend, "kei", "x");
	check(error == ENOTSUP, "a session has no log in");
	error = kl_backend_power_action(backend, KL_BACKEND_POWER_POWEROFF);
	check(error == ENOTSUP, "a session has no power action");

	/* UNLOCK and OK. */
	error = kl_backend_session_unlock(backend, "secret");
	check(error == 0 && read_line(ends[0], "UNLOCK secret\n"), "UNLOCK secret written");
	(void)write(ends[0], "OK\n", 3U);
	kl_backend_tick(backend, 2000U);
	check(answer_count == 1U && answer_request == KL_BACKEND_SESSION_UNLOCK && answer_error == 0, "OK answers UNLOCK");

	/* LOGOUT once, QUIT, then RELEASED after the stop. */
	error = kl_backend_session_logout(backend);
	check(error == 0 && read_line(ends[0], "LOGOUT\n"), "LOGOUT written");
	error = kl_backend_session_logout(backend);
	check(error == 0 && !read_line(ends[0], "LOGOUT\n"), "a second Log Out writes nothing");
	stop_peer = ends[0];
	released_before_stop = 0;
	(void)write(ends[0], "QUIT\n", 5U);
	kl_backend_tick(backend, 2001U);
	check(stop_count == 1U && stop_reason == KL_BACKEND_SESSION_QUIT, "QUIT ends the session");
	check(!released_before_stop && read_line(ends[0], "RELEASED\n"), "RELEASED comes after the stop");
	stop_peer = -1;

	/* sessiond closes: the session carries on, no longer managed. */
	(void)close(ends[0]);
	kl_backend_tick(backend, 2002U);
	check(stop_count == 1U && kl_backend_session_managed(backend) == 0, "sessiond gone: the session carries on unmanaged");
	error = kl_backend_session_logout(backend);
	check(error == ENOTSUP, "Log Out without sessiond ends at once (ENOTSUP)");
	kl_backend_close(backend);
}

/* A Log Out sessiond never answers ends the compositor after 30 s. */
static void
test_unanswered(
	void)
{
	struct kl_backend *backend;
	int ends[2];
	int error;

	/* sessiond's end (0) and the session's (1). */
	error = socketpair(AF_UNIX, SOCK_STREAM, 0, ends);
	if (error != 0)
		return;
	backend = open_backend(-1, ends[1]);
	if (backend == NULL)
		return;

	/* LOGOUT; the deadline starts at the first tick. */
	(void)kl_backend_session_logout(backend);
	kl_backend_tick(backend, 5000U);
	kl_backend_tick(backend, 34999U);
	check(stop_count == 0U, "no stop before 30 s");
	kl_backend_tick(backend, 35000U);
	check(stop_count == 1U && stop_reason == KL_BACKEND_SESSION_UNANSWERED, "unanswered after 30 s");
	kl_backend_close(backend);
	(void)close(ends[0]);
	(void)close(ends[1]);
}
