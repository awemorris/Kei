/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The old sound calls of <keiland.h>, kept while Settings still uses them
 * (WS131 p004 to p011).
 *
 * The sound's operating-system code moved to libkeiland-backend
 * (plan/ws131/design.md section 3).  Until Settings reaches the sound
 * through the compositor's extension (ws131-p011, which removes this file
 * with system-compat.c), keiland_audio_* keeps its names, types and meaning
 * and forwards every call to kl_backend_audio_*, whose sources are built
 * into this library for the time being.  It is a file of its own, not part
 * of system-compat.c, so that a program built without the network (the
 * host test of Settings, plan/ws089/tests/host-build.sh) can take the sound
 * alone.  The two headers describe the same change bits; the check below
 * stops the build if they ever differ, since they are passed through
 * unchanged.
 */

#include <keiland.h>

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdlib.h>

/* The forwarded change bits must mean the same on both sides of the forwarding. */
#if KEILAND_AUDIO_CHANGED_REACHABLE != KL_BACKEND_AUDIO_CHANGED_REACHABLE || \
    KEILAND_AUDIO_CHANGED_VOLUME != KL_BACKEND_AUDIO_CHANGED_VOLUME
#error "the sound's change bits differ between keiland.h and keiland-backend.h"
#endif

/*
 * A following of the sound's volume opened through the old call.
 *
 * Allocated by keiland_audio_open and freed by keiland_audio_close;
 * backend is the backend's following it forwards to, owned by this one.
 */
struct keiland_audio {
	struct kl_backend_audio *backend;
};

/*
 * Starts following the sound's volume through the backend.
 */
struct keiland_audio *
keiland_audio_open(
	void)
{
	struct keiland_audio *audio;

	/* Allocates the old following. */
	audio = malloc(sizeof(*audio));
	if (audio == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	/* Opens the backend's following it forwards to (errno is the backend's when it fails). */
	audio->backend = kl_backend_audio_open();
	if (audio->backend == NULL) {
		free(audio);
		return NULL;
	}

	/* Succeeded: the caller owns the following. */
	return audio;
}

/*
 * Stops following the sound's volume.
 */
void
keiland_audio_close(
	struct keiland_audio *audio)
{
	/* A following that was never opened has nothing to close. */
	if (audio == NULL)
		return;

	/* Closes the backend's following, then the old one. */
	kl_backend_audio_close(audio->backend);
	free(audio);
}

/*
 * The descriptor to poll for the sound service's reports.
 */
int
keiland_audio_fd(
	const struct keiland_audio *audio)
{
	int descriptor;

	/* Asks the backend (-1 when there is none). */
	descriptor = kl_backend_audio_fd(audio->backend);

	/* Reports the descriptor. */
	return descriptor;
}

/*
 * Reads what has arrived from the sound service.
 */
int
keiland_audio_update(
	struct keiland_audio *audio,
	unsigned *changed)
{
	int error;

	/* Reads through the backend; the change bits are the same on both sides. */
	error = kl_backend_audio_update(audio->backend, changed);
	if (error != 0)
		return error;

	/* Succeeded: *changed says what changed. */
	return 0;
}

/*
 * Copies what the sound service last reported.
 */
void
keiland_audio_get_state(
	const struct keiland_audio *audio,
	struct keiland_audio_state *state)
{
	struct kl_backend_audio_state reported;

	/* Asks the backend for the state. */
	kl_backend_audio_get_state(audio->backend, &reported);

	/* Copies the state field by field into the old structure. */
	state->reachable = reported.reachable;
	state->device = reported.device;
	state->rate = reported.rate;
	state->channels = reported.channels;
	state->left = reported.left;
	state->right = reported.right;
	state->muted = reported.muted;
}

/*
 * Asks the sound service for a volume and mute.
 */
int
keiland_audio_set_volume(
	struct keiland_audio *audio,
	unsigned left,
	unsigned right,
	unsigned muted)
{
	int error;

	/* Sends through the backend. */
	error = kl_backend_audio_set_volume(audio->backend, left, right, muted);
	if (error != 0)
		return error;

	/* Succeeded: the new volume comes back through the updates. */
	return 0;
}

/*
 * Asks the sound service for its short feedback sound.
 */
int
keiland_audio_feedback(
	struct keiland_audio *audio)
{
	int error;

	/* Sends through the backend. */
	error = kl_backend_audio_feedback(audio->backend);
	if (error != 0)
		return error;

	/* Succeeded: the service plays it. */
	return 0;
}

/*
 * Tells whether the sound service runs.
 */
int
keiland_audio_available(
	void)
{
	int running;

	/* Asks the backend; it neither connects nor waits. */
	running = kl_backend_audio_available();

	/* Reports 1 when the service runs. */
	return running;
}
