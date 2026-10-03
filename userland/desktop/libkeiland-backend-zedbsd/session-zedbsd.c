/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The session on zedBSD: sessiond's lines (ws035-p095, ws035-p101,
 * ws035-p102; libkeiland-backend since ws131-p006).
 *
 * sessiond starts the compositor with one descriptor: the login screen's
 * (--auth-fd, options.greeter_descriptor) or a session's (--control-fd,
 * options.session_descriptor).  Every message is a line:
 *
 *   READY                   GO: the display may be taken
 *   AUTH name password      OK: the user is in; FAIL; ERROR (login screen)
 *   UNLOCK password         OK; FAIL (a session's lock screen)
 *   POWER poweroff|reboot   OK (login screen, power-zedbsd.c)
 *   LOGOUT                  QUIT: the greeter is up, the session ends
 *   RELEASED                (none): the display has been given back
 *
 * READY is said just before the compositor first takes the display, when
 * everything slow is done, and GO is awaited: sessiond sends it once the
 * greeter has ended and let the display go, so the screen goes from the
 * greeter's last frame to the desktop's first without the text console
 * between them.  Without GO in time (another sessiond, or none) the
 * display is taken anyway.
 *
 * The login screen ends when sessiond shuts its side of the socket down
 * (the session is ready to take the display); a session ends on QUIT.  In
 * both the compositor gives the display back first and then RELEASED is
 * said, before the slower rest of its end.  When sessiond closes a
 * session's descriptor, the session carries on without it.
 *
 * The descriptors do not block: the tick reads what has come, cuts it into
 * lines and answers the request awaited (one at a time).
 */

#include "userland/desktop/libkeiland-backend/backend-private.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* How long the compositor waits for GO, and for QUIT after LOGOUT (milliseconds). */
#define SESSION_WAIT_MS 20000U
#define SESSION_LOGOUT_MS 30000U

/* The longest line the compositor sends (AUTH with a name and a password). */
#define SESSION_REQUEST_MAX 192U

static int session_descriptor(const struct kl_backend *backend);
static int session_send(struct kl_backend *backend, unsigned request, const char *line);
static void session_answered(struct kl_backend *backend, const char *line);
static void session_end(struct kl_backend *backend, unsigned reason);
static uint64_t session_milliseconds(void);

/*
 * Says READY and waits for GO, before the display is first taken.
 */
int
kl_backend_session_ready(
	struct kl_backend *backend)
{
	struct pollfd entry;
	uint64_t started;
	uint64_t waited;
	char line[16];
	size_t used;
	ssize_t count;
	char byte;
	int descriptor;
	int status;
	int same;

	/* The login screen's descriptor, or the session's; none when sessiond did not start the compositor. */
	if (backend == NULL)
		return EINVAL;
	descriptor = session_descriptor(backend);
	if (descriptor < 0)
		return ENOTSUP;

	/* READY. */
	count = write(descriptor, "READY\n", 6U);
	if (count < 0)
		return errno;
	if (count != 6)
		return EIO;

	/* GO, a line of its own, read a byte at a time (nothing after it is taken). */
	started = session_milliseconds();
	used = 0;
	for (;;) {
		/* The time left. */
		waited = session_milliseconds() - started;
		if (waited >= SESSION_WAIT_MS)
			break;

		/* A byte, or the end of the wait. */
		entry.fd = descriptor;
		entry.events = POLLIN;
		entry.revents = 0;
		status = poll(&entry, 1, (int)(SESSION_WAIT_MS - waited));
		if (status < 0 && errno == EINTR)
			continue;

		/* Ends the wait when polling failed or its time expired. */
		if (status <= 0)
			break;

		/* Reads only the byte that polling made available. */
		count = read(descriptor, &byte, 1U);
		if (count < 0 &&
		    (errno == EINTR ||
		    errno == EAGAIN))
			continue;

		/* Ends the wait when the peer closed or a read failed. */
		if (count <= 0)
			break;

		/* A whole line is looked at; a long one is thrown away. */
		if (byte != '\n') {
			/* Keeps a bounded line while passing over excess bytes. */
			if (used + 1U < sizeof(line))
				line[used++] = byte;
			continue;
		}

		/* The line ends here. */
		line[used] = '\0';
		used = 0;
		same = strcmp(line, "GO");
		if (same == 0)
			return 0;
	}

	/* The display is taken without GO. */
	return ETIMEDOUT;
}

/*
 * Asks sessiond for a greeter (LOGOUT); the session goes on until QUIT.
 */
int
kl_backend_session_logout(
	struct kl_backend *backend)
{
	int error;

	/* Only a session sessiond started and still listens to. */
	if (backend == NULL)
		return EINVAL;
	if (backend->options.session_descriptor < 0 || backend->session_gone)
		return ENOTSUP;

	/* Once: the answer of the first is awaited. */
	if (backend->logout_asked)
		return 0;

	/* LOGOUT; its answer is QUIT, not a request's answer. */
	error = session_send(backend, KL_BACKEND_SESSION_NONE, "LOGOUT\n");
	if (error != 0)
		return error;

	/* Succeeded: the next tick starts the deadline. */
	backend->logout_asked = 1U;
	backend->logout_ms = 0U;
	return 0;
}

/*
 * Asks sessiond to log a user in (AUTH).
 */
int
kl_backend_session_authenticate(
	struct kl_backend *backend,
	const char *user,
	const char *password)
{
	char line[SESSION_REQUEST_MAX];
	int length;
	int error;

	/* Only the login screen asks, with a name and a password that fit one line. */
	if (backend == NULL || user == NULL || password == NULL)
		return EINVAL;
	if (backend->options.greeter_descriptor < 0)
		return ENOTSUP;
	if (strchr(user, ' ') != NULL || strchr(user, '\n') != NULL || strchr(password, '\n') != NULL)
		return EINVAL;
	length = snprintf(line, sizeof(line), "AUTH %s %s\n", user, password);
	if (length < 0 || (size_t)length >= sizeof(line)) {
		memset(line, 0, sizeof(line));
		return EINVAL;
	}

	/* The request; nothing of the password is kept once it is sent. */
	error = session_send(backend, KL_BACKEND_SESSION_AUTH, line);
	memset(line, 0, sizeof(line));
	if (error != 0)
		return error;

	/* Succeeded: the answer comes through session_answer. */
	return 0;
}

/*
 * Asks sessiond to unlock the session's lock screen (UNLOCK).
 */
int
kl_backend_session_unlock(
	struct kl_backend *backend,
	const char *password)
{
	char line[SESSION_REQUEST_MAX];
	int length;
	int error;

	/* Only a session sessiond started and still listens to. */
	if (backend == NULL || password == NULL)
		return EINVAL;
	if (backend->options.session_descriptor < 0 || backend->session_gone)
		return ENOTSUP;
	if (strchr(password, '\n') != NULL)
		return EINVAL;
	length = snprintf(line, sizeof(line), "UNLOCK %s\n", password);
	if (length < 0 || (size_t)length >= sizeof(line)) {
		memset(line, 0, sizeof(line));
		return EINVAL;
	}

	/* The request; nothing of the password is kept once it is sent. */
	error = session_send(backend, KL_BACKEND_SESSION_UNLOCK, line);
	memset(line, 0, sizeof(line));
	if (error != 0)
		return error;

	/* Succeeded: the answer comes through session_answer. */
	return 0;
}

/*
 * Tells whether sessiond started this session and still listens.
 */
int
kl_backend_session_managed(
	const struct kl_backend *backend)
{
	/* No backend, the login screen, or a session sessiond left. */
	if (backend == NULL || backend->options.session_descriptor < 0 || backend->session_gone)
		return 0;

	/* Succeeded: it can be locked and logged out through sessiond. */
	return 1;
}

/*
 * Reads what sessiond sent, and ends a Log Out that was not answered in time.
 */
void
kl_backend_session_tick(
	struct kl_backend *backend,
	uint64_t now_ms)
{
	ssize_t count;
	char *end;
	int descriptor;

	/* Nothing to read without sessiond, or after it left. */
	descriptor = session_descriptor(backend);
	if (descriptor < 0 || backend->session_gone)
		return;

	/* A Log Out sessiond did not answer in time ends the compositor anyway. */
	if (backend->logout_asked) {
		if (backend->logout_ms == 0U)
			backend->logout_ms = now_ms;
		if (now_ms - backend->logout_ms >= SESSION_LOGOUT_MS) {
			backend->logout_asked = 0U;
			if (backend->host.session_stop != NULL)
				backend->host.session_stop(backend->host.data, KL_BACKEND_SESSION_UNANSWERED);
			return;
		}
	}

	/* What has come. */
	count = read(descriptor, backend->session_line + backend->session_used, sizeof(backend->session_line) - 1U - backend->session_used);
	if (count < 0)
		return;

	/* sessiond done with the login screen ends it (RELEASED is still said on the open side). */
	if (count == 0 && backend->options.greeter_descriptor >= 0) {
		backend->session_gone = 1U;
		session_end(backend, KL_BACKEND_SESSION_ENDED);
		return;
	}

	/* A session carries on without sessiond, whose descriptor is closed. */
	if (count == 0) {
		backend->session_gone = 1U;
		(void)close(descriptor);
		backend->options.session_descriptor = -1;
		return;
	}

	/* Each whole line. */
	backend->session_used += (size_t)count;
	backend->session_line[backend->session_used] = '\0';
	for (;;) {
		end = strchr(backend->session_line, '\n');
		if (end == NULL)
			break;

		/* Acts on the line before taking it out of what is kept. */
		*end = '\0';
		session_answered(backend, backend->session_line);
		backend->session_used -= (size_t)(end - backend->session_line) + 1U;
		memmove(backend->session_line, end + 1, backend->session_used + 1U);
	}

	/* A line that never ends is thrown away. */
	if (backend->session_used + 1U >= sizeof(backend->session_line))
		backend->session_used = 0U;
}

/* Returns the descriptor to sessiond: the login screen's, a session's, or -1. */
static int
session_descriptor(
	const struct kl_backend *backend)
{
	/* The login screen asks on --auth-fd. */
	if (backend->options.greeter_descriptor >= 0)
		return backend->options.greeter_descriptor;

	/* A session's --control-fd (-1 without sessiond). */
	return backend->options.session_descriptor;
}

/* Writes one request line whole and remembers which request awaits its answer. */
static int
session_send(
	struct kl_backend *backend,
	unsigned request,
	const char *line)
{
	size_t length;
	ssize_t written;

	/* One request at a time. */
	if (request != KL_BACKEND_SESSION_NONE && backend->session_request != KL_BACKEND_SESSION_NONE)
		return EBUSY;

	/* The whole line in one write (it is short). */
	length = strlen(line);
	written = write(session_descriptor(backend), line, length);
	if (written < 0)
		return errno;
	if ((size_t)written != length)
		return EIO;

	/* Succeeded: the answer is awaited. */
	if (request != KL_BACKEND_SESSION_NONE)
		backend->session_request = request;
	return 0;
}

/* Acts on one line sessiond sent. */
static void
session_answered(
	struct kl_backend *backend,
	const char *line)
{
	unsigned request;
	int same;
	int error;

	/* QUIT: the greeter is ready; the display goes back, then the compositor ends. */
	same = strcmp(line, "QUIT");
	if (same == 0 && backend->options.greeter_descriptor < 0) {
		backend->logout_asked = 0U;
		session_end(backend, KL_BACKEND_SESSION_QUIT);
		return;
	}

	/* An answer: granted, refused, failed, or not understood. */
	error = EPROTO;
	if (strcmp(line, "OK") == 0)
		error = 0;
	else if (strcmp(line, "FAIL") == 0)
		error = EACCES;
	else if (strcmp(line, "ERROR") == 0)
		error = EIO;

	/* The request it answers is no longer awaited. */
	request = backend->session_request;
	backend->session_request = KL_BACKEND_SESSION_NONE;
	if (backend->host.session_answer != NULL)
		backend->host.session_answer(backend->host.data, request, error);
}

/* Ends the compositor: it gives the display back in the callback, then sessiond hears RELEASED. */
static void
session_end(
	struct kl_backend *backend,
	unsigned reason)
{
	int descriptor;

	/* The compositor closes its output (and asks to stop). */
	if (backend->host.session_stop != NULL)
		backend->host.session_stop(backend->host.data, reason);

	/* sessiond hears the display is free (it may be gone already). */
	descriptor = session_descriptor(backend);
	if (descriptor >= 0)
		(void)write(descriptor, "RELEASED\n", 9U);
}

/* The monotonic clock in milliseconds, for the wait for GO. */
static uint64_t
session_milliseconds(
	void)
{
	struct timespec now;
	int error;

	/* The clock cannot fail with this identifier; a failure reads as 0. */
	error = clock_gettime(CLOCK_MONOTONIC, &now);
	if (error != 0)
		return 0U;

	/* Succeeded: the milliseconds since an arbitrary start. */
	return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
}
