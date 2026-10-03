/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of the desktop's audio (ws100-p003; libkeiland-backend's
 * zedBSD side since ws131-p004): the backend against a
 * pretend audiod on a socket of its own (audio-zedbsd.c built with
 * AUDIO_SOCKET_PATH), a child process that answers HELLO with WELCOME,
 * SUBSCRIBE with DONE and the volume, DEVICE_VOLUME with DONE and the new
 * volume, and FEEDBACK with ERROR EINVAL (an audiod without it), then goes
 * away and comes back.  Prints "host-audio: N/M passed".
 *
 *   plan/ws100/tests/host-audio.sh
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include "userland/base/audiod/protocol.h"

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int checks;
static int passed;

static void check(int ok, const char *what);
static pid_t pretend_start(unsigned device, unsigned volume);
static void pretend_serve(int listener, unsigned device, unsigned volume);
static void pretend_reply(int fd, uint32_t type, uint32_t serial, uint32_t error);
static void pretend_volume(int fd, unsigned left, unsigned right, unsigned muted);
static void wait_update(struct kl_backend_audio *audio, unsigned want, unsigned *seen);
static void pause_ms(unsigned milliseconds);

/* Runs the checks; returns 0 when all passed. */
int
main(
	void)
{
	struct kl_backend_audio *audio;
	struct kl_backend_audio_state state;
	unsigned seen;
	pid_t child;
	int error;

	/* A broken socket must not end the test. */
	signal(SIGPIPE, SIG_IGN);

	/* 1. No audiod: the record exists, not connected, not reachable, and a set is refused. */
	unlink(AUDIO_SOCKET_PATH);
	audio = kl_backend_audio_open();
	check(audio != NULL, "open without audiod");
	check(kl_backend_audio_fd(audio) < 0, "no descriptor without audiod");
	kl_backend_audio_get_state(audio, &state);
	check(state.reachable == 0U, "not reachable without audiod");
	error = kl_backend_audio_set_volume(audio, 50U, 50U, 0U);
	check(error == ENOTCONN, "set refused without audiod");
	error = kl_backend_audio_set_volume(audio, 101U, 50U, 0U);
	check(error == EINVAL, "an out-of-range volume is refused");

	/* 2. audiod comes: the next update (after the wait) connects, WELCOME and the volume arrive. */
	child = pretend_start(1U, 70U);
	pause_ms(1100U);
	wait_update(audio, KL_BACKEND_AUDIO_CHANGED_VOLUME, &seen);
	kl_backend_audio_get_state(audio, &state);
	check(state.reachable == 1U && state.device == 1U && state.rate == 48000U, "reachable with the device after connecting");
	check(state.left == 70U && state.right == 70U && state.muted == 0U, "the volume as it stands");
	check(kl_backend_audio_fd(audio) >= 0, "a descriptor while connected");

	/* 3. A set comes back as a report. */
	error = kl_backend_audio_set_volume(audio, 35U, 35U, 1U);
	check(error == 0, "set sent");
	wait_update(audio, KL_BACKEND_AUDIO_CHANGED_VOLUME, &seen);
	kl_backend_audio_get_state(audio, &state);
	check(state.left == 35U && state.muted == 1U, "the new volume reported");

	/* 4. The feedback sound to an audiod without it: sent, the ERROR passed over, still connected. */
	error = kl_backend_audio_feedback(audio);
	check(error == 0, "feedback sent");
	wait_update(audio, 0U, &seen);
	check(kl_backend_audio_fd(audio) >= 0, "still connected after an ERROR");

	/* 5. audiod goes: not reachable; it comes back with no device: reachable, device 0. */
	kill(child, SIGTERM);
	waitpid(child, NULL, 0);
	wait_update(audio, KL_BACKEND_AUDIO_CHANGED_REACHABLE, &seen);
	kl_backend_audio_get_state(audio, &state);
	check(state.reachable == 0U, "not reachable after audiod went");
	child = pretend_start(0U, 100U);
	pause_ms(1100U);
	wait_update(audio, KL_BACKEND_AUDIO_CHANGED_VOLUME, &seen);
	kl_backend_audio_get_state(audio, &state);
	check(state.reachable == 1U && state.device == 0U && state.left == 100U, "connected again, no device");

	/* The end. */
	kl_backend_audio_close(audio);
	kill(child, SIGTERM);
	waitpid(child, NULL, 0);
	unlink(AUDIO_SOCKET_PATH);
	printf("host-audio: %d/%d passed\n", passed, checks);
	if (passed != checks)
		return 1;

	/* Succeeded. */
	return 0;
}

/* Counts one check. */
static void
check(
	int ok,
	const char *what)
{
	/* One more, and whether it held. */
	checks++;
	if (ok) {
		passed++;
		printf("ok: %s\n", what);
	} else {
		printf("FAIL: %s\n", what);
	}
}

/* Starts a pretend audiod in a child with a device (0 or 1) and a volume; returns its pid. */
static pid_t
pretend_start(
	unsigned device,
	unsigned volume)
{
	struct sockaddr_un address;
	pid_t child;
	int listener;
	int error;

	/* The socket, listening before the child is started. */
	unlink(AUDIO_SOCKET_PATH);
	listener = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	snprintf(address.sun_path, sizeof(address.sun_path), "%s", AUDIO_SOCKET_PATH);
	error = bind(listener, (struct sockaddr *)&address, sizeof(address));
	if (error == 0)
		error = listen(listener, 4);
	if (error != 0) {
		perror("pretend audiod");
		exit(2);
	}

	/* The child serves; the parent keeps nothing of it. */
	child = fork();
	if (child == 0) {
		pretend_serve(listener, device, volume);
		_exit(0);
	}

	/* The parent's copy of the socket. */
	close(listener);

	/* Succeeded: the child's pid. */
	return child;
}

/* Serves one client at a time until killed. */
static void
pretend_serve(
	int listener,
	unsigned device,
	unsigned volume)
{
	struct audiod_welcome welcome;
	struct audiod_volume request;
	struct audiod_header header;
	ssize_t count;
	int fd;

	/* Each client. */
	for (;;) {
		fd = accept(listener, NULL, NULL);
		if (fd < 0)
			return;

		/* Each request: the header, then the rest of its length. */
		for (;;) {
			count = recv(fd, &header, sizeof(header), MSG_WAITALL);
			if (count != (ssize_t)sizeof(header))
				break;

			/* The rest of the request. */
			memset(&request, 0, sizeof(request));
			request.header = header;
			if (header.length > sizeof(header) && header.length <= sizeof(request))
				(void)recv(fd, (uint8_t *)&request + sizeof(header), header.length - sizeof(header), MSG_WAITALL);

			/* The answers. */
			if (header.type == AUDIOD_HELLO) {
				memset(&welcome, 0, sizeof(welcome));
				welcome.header.type = AUDIOD_WELCOME;
				welcome.header.length = sizeof(welcome);
				welcome.header.serial = header.serial;
				welcome.version = AUDIOD_VERSION;
				welcome.device = device;
				welcome.rate = 48000U;
				welcome.channels = 2U;
				(void)send(fd, &welcome, sizeof(welcome), 0);
			} else if (header.type == AUDIOD_SUBSCRIBE) {
				pretend_reply(fd, AUDIOD_DONE, header.serial, 0U);
				pretend_volume(fd, volume, volume, 0U);
			} else if (header.type == AUDIOD_DEVICE_VOLUME) {
				pretend_reply(fd, AUDIOD_DONE, header.serial, 0U);
				pretend_volume(fd, request.left, request.right, request.muted);
			} else {
				pretend_reply(fd, AUDIOD_ERROR, header.serial, EINVAL);
			}
		}

		/* The client went. */
		close(fd);
	}
}

/* Sends DONE or ERROR for a serial. */
static void
pretend_reply(
	int fd,
	uint32_t type,
	uint32_t serial,
	uint32_t error)
{
	struct audiod_result result;

	/* The answer. */
	memset(&result, 0, sizeof(result));
	result.header.type = type;
	result.header.length = sizeof(result);
	result.header.serial = serial;
	result.error = error;
	(void)send(fd, &result, sizeof(result), 0);
}

/* Sends VOLUME_CHANGED. */
static void
pretend_volume(
	int fd,
	unsigned left,
	unsigned right,
	unsigned muted)
{
	struct audiod_volume volume;

	/* The report. */
	memset(&volume, 0, sizeof(volume));
	volume.header.type = AUDIOD_VOLUME_CHANGED;
	volume.header.length = sizeof(volume);
	volume.left = left;
	volume.right = right;
	volume.muted = muted;
	(void)send(fd, &volume, sizeof(volume), 0);
}

/* Updates for up to 3 seconds until a change bit is seen (with 0, updates for 300 ms). */
static void
wait_update(
	struct kl_backend_audio *audio,
	unsigned want,
	unsigned *seen)
{
	unsigned changed;
	unsigned round;

	/* Every 20 ms. */
	*seen = 0U;
	for (round = 0U; round < 150U; round++) {
		(void)kl_backend_audio_update(audio, &changed);
		*seen |= changed;
		if (want != 0U && (*seen & want) != 0U)
			return;
		if (want == 0U && round >= 15U)
			return;
		pause_ms(20U);
	}
}

/* Waits some milliseconds. */
static void
pause_ms(
	unsigned milliseconds)
{
	struct timespec time;

	/* The whole wait. */
	time.tv_sec = (time_t)(milliseconds / 1000U);
	time.tv_nsec = (long)(milliseconds % 1000U) * 1000000L;
	(void)nanosleep(&time, NULL);
}
