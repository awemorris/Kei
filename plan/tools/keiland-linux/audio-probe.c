/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks production ALSA controls and another process's mixer notifications.
 */
#include <keiland.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int audio_probe(struct keiland_audio *audio);
static int volume(struct keiland_audio *audio, unsigned percent, unsigned muted);
static int mixer(const char *command, const char *expected);

/*
 * Owns one subscription throughout the isolated ALSA acceptance checks.
 */
int
main(
	void)
{
	struct keiland_audio *audio;
	int available;
	int error;

	/* The real caller must be able to discover a supported mixer. */
	available = keiland_audio_available();
	if (available != 1) {
		fprintf(stderr, "audio-probe: FAIL unavailable\n");
		return 1;
	}

	/* Subscription cleanup remains owned by main on every outcome. */
	audio = keiland_audio_open();
	if (audio == NULL)
		return 1;
	error = audio_probe(audio);
	keiland_audio_close(audio);
	if (error != 0) {
		fprintf(stderr, "audio-probe: FAIL errno=%d %s\n", error, strerror(error));
		return 1;
	}

	/* Succeeded: ALSA values, notifications, mute and silent feedback all passed. */
	printf("audio-probe: PASS\n");
	return 0;
}

/* Exercises the public read/write and poll paths against the actual guest mixer. */
static int
audio_probe(
	struct keiland_audio *audio)
{
	struct keiland_audio_state state;
	struct pollfd descriptor;
	unsigned changed;
	int error;
	int polled;

	/* A reachable card with a supported element is a real audio device. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0)
		return error;
	keiland_audio_get_state(audio, &state);
	if (state.reachable != 1 || state.device != 1)
		return ENODEV;

	/* The library and amixer must observe the same forty-percent volume. */
	error = volume(audio, 40, 0);
	if (error != 0)
		return error;
	error = mixer("amixer -c 0 get Master", "[on]");
	if (error != 0)
		return error;

	/* Drain the library's own notifications before testing another process. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0)
		return error;
	error = mixer("amixer -c 0 set Master 70%", "[on]");
	if (error != 0)
		return error;
	descriptor.fd = keiland_audio_fd(audio);
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	polled = poll(&descriptor, 1, 3000);
	if (polled != 1 || (descriptor.revents & POLLIN) == 0)
		return ETIMEDOUT;
	error = keiland_audio_update(audio, &changed);
	if (error != 0)
		return error;
	keiland_audio_get_state(audio, &state);
	if ((changed & KEILAND_AUDIO_CHANGED_VOLUME) == 0 ||
	    state.left < 67 ||
	    state.left > 73)
		return EPROTO;
	printf("audio-probe: external event left=%u right=%u changed=%u PASS\n", state.left, state.right, changed);

	/* Mute must reach ALSA rather than changing only a private state cache. */
	error = volume(audio, 40, 1);
	if (error != 0)
		return error;
	error = mixer("amixer -c 0 get Master", "[off]");
	if (error != 0)
		return error;
	error = volume(audio, 40, 0);
	if (error != 0)
		return error;
	error = mixer("amixer -c 0 get Master", "[on]");
	if (error != 0)
		return error;
	error = keiland_audio_feedback(audio);
	if (error != 0)
		return error;

	/* Succeeded: feedback accepted without PCM playback as documented. */
	return 0;
}

/* Checks hardware readback after a public volume and mute request. */
static int
volume(
	struct keiland_audio *audio,
	unsigned percent,
	unsigned muted)
{
	struct keiland_audio_state state;
	unsigned changed;
	int error;

	/* Device state is obtained by update, never by assuming the write succeeded. */
	error = keiland_audio_set_volume(audio, percent, percent, muted);
	if (error != 0)
		return error;
	error = keiland_audio_update(audio, &changed);
	if (error != 0)
		return error;
	keiland_audio_get_state(audio, &state);
	printf("audio-probe: volume left=%u right=%u muted=%u changed=%u\n", state.left, state.right, state.muted, changed);
	if (state.left != percent ||
	    state.right != percent ||
	    state.muted != muted)
		return EPROTO;

	/* Succeeded: the public state agrees with the actual written control. */
	return 0;
}

/* Runs the independent ALSA utility and checks its audible state. */
static int
mixer(
	const char *command,
	const char *expected)
{
	FILE *process;
	char line[256];
	char *read_line;
	char *found;
	unsigned seen;
	int status;

	/* The external utility uses its own control connection. */
	process = popen(command, "r");
	if (process == NULL)
		return errno;
	seen = 0;

	/* Preserve the actual raw and percent values as test evidence. */
	for (;;) {
		read_line = fgets(line, sizeof(line), process);
		if (read_line == NULL)
			break;
		fputs(line, stdout);
		found = strstr(line, expected);
		if (found != NULL)
			seen++;
	}

	/* Utility failure or an absent switch state fails the independent check. */
	status = pclose(process);
	if (status != 0 || seen != 2)
		return EIO;

	/* Succeeded: both channels report the expected switch state. */
	return 0;
}
