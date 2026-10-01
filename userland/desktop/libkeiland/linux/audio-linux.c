/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The absent ALSA control device, placeholder until ws105-p010.
 */

#include <keiland.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* One caller-owned audio subscription, retained from open until close. */
struct keiland_audio {
	struct keiland_audio_state state;
};

/*
 * Allocates an audio subscription even when no device is reachable.
 */
struct keiland_audio *
keiland_audio_open(
	void)
{
	struct keiland_audio *audio;

	/* Retains a subscription that later updates can reconnect. */
	audio = calloc(1, sizeof(*audio));
	if (audio == NULL)
		return NULL;

	/* Succeeded: the caller owns the subscription. */
	return audio;
}

/*
 * Releases an audio subscription.
 */
void
keiland_audio_close(
	struct keiland_audio *audio)
{
	/* Releases the record without an owned device descriptor. */
	free(audio);

	/* Succeeded: the subscription is released. */
	return;
}

/*
 * Reports the absence of a pollable audio device.
 */
int
keiland_audio_fd(
	const struct keiland_audio *audio)
{
	/* The placeholder owns no device descriptor. */
	(void)audio;

	/* Reports that no descriptor can be polled. */
	return -1;
}

/*
 * Reports no changes while audio is unavailable.
 */
int
keiland_audio_update(
	struct keiland_audio *audio,
	unsigned *changed)
{
	/* Requires a destination for the change mask. */
	if (changed == NULL)
		return EINVAL;

	/* No device can publish a change. */
	*changed = 0U;
	if (audio == NULL)
		return EINVAL;

	/* Succeeded: the subscription remains disconnected. */
	return 0;
}

/*
 * Copies the disconnected audio state.
 */
void
keiland_audio_get_state(
	const struct keiland_audio *audio,
	struct keiland_audio_state *state)
{
	/* A missing subscription reports an empty state. */
	memset(state, 0, sizeof(*state));
	if (audio == NULL)
		return;

	/* Copies the subscription's unchanged state. */
	*state = audio->state;

	/* Succeeded: the caller holds the audio state. */
	return;
}

/*
 * Refuses volume requests while no audio device is connected.
 */
int
keiland_audio_set_volume(
	struct keiland_audio *audio,
	unsigned left,
	unsigned right,
	unsigned muted)
{
	/* Rejects invalid subscription and volume arguments. */
	if (audio == NULL)
		return EINVAL;

	/* Keeps the public channel range even while disconnected. */
	if (left > 100U ||
	    right > 100U ||
	    muted > 1U)
		return EINVAL;

	/* Reports the missing audio connection. */
	return ENOTCONN;
}

/*
 * Refuses feedback while no audio device is connected.
 */
int
keiland_audio_feedback(
	struct keiland_audio *audio)
{
	/* A request requires an existing subscription. */
	if (audio == NULL)
		return EINVAL;

	/* Reports the missing audio connection. */
	return ENOTCONN;
}

/*
 * Reports that the placeholder has no audio service.
 */
int
keiland_audio_available(
	void)
{
	/* Succeeded: no audio service is available. */
	return 0;
}
