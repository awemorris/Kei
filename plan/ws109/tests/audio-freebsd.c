/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks the production OSS backend against independent native mixer controls.
 */

#include <keiland.h>
#include <errno.h>
#include <mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int audio_checks(struct keiland_audio *audio);
static int audio_set(struct keiland_audio *audio, unsigned left, unsigned right, unsigned muted);
static int audio_independent(unsigned left, unsigned right, unsigned muted);

/*
 * Tests native volume and mute while restoring the original hardware settings.
 */
int
main(
	void)
{
	struct keiland_audio *audio;
	struct keiland_audio_state original;
	unsigned changed;
	int available;
	int error;
	int restored;

	/* Requires an actual accessible native output control for this hardware probe. */
	available = keiland_audio_available();
	if (available != 1)
		return 1;

	/* Retains the subscription owner through success, failure and original-state restoration. */
	audio = keiland_audio_open();
	if (audio == NULL)
		return 1;

	/* Captures a real initial snapshot before any mixer mutation. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0) {
		keiland_audio_close(audio);
		return 1;
	}

	/* Saves the exact native percentages and mute state for the common restoration path. */
	keiland_audio_get_state(audio, &original);
	if (original.reachable != 1 || original.device != 1) {
		keiland_audio_close(audio);
		return 1;
	}

	/* Runs the mutation checks without relinquishing restoration ownership. */
	error = audio_checks(audio);

	/* Restores the original settings even if a preceding contract assertion failed. */
	restored = audio_set(audio, original.left, original.right, original.muted);
	if (restored != 0) {
		keiland_audio_close(audio);
		(void)fprintf(stderr, "audio-freebsd: restoration failed errno=%d\n", restored);
		return 1;
	}

	/* Releases the subscription only after its hardware changes have been reversed. */
	keiland_audio_close(audio);

	/* Preserves a failed assertion after successful original-state restoration. */
	if (error != 0) {
		(void)fprintf(stderr, "audio-freebsd: failed errno=%d; original settings restored\n", error);
		return 1;
	}

	/* Publishes the checked native hardware contract and its restored final values. */
	(void)printf("audio-freebsd: PASS volume/mute/external refresh; restored %u/%u mute=%u\n", original.left, original.right, original.muted);

	/* Succeeded: the real mixer contract passed and the probe left its settings restored. */
	return 0;
}

/* Checks local writes, independent hardware readback and another process's changes. */
static int
audio_checks(
	struct keiland_audio *audio)
{
	struct keiland_audio_state state;
	struct timespec delay = {1, 100000000};
	unsigned changed;
	int error;
	int descriptor;

	/* OSS has no pollable mixer event stream even while the device is connected. */
	descriptor = keiland_audio_fd(audio);
	if (descriptor != -1)
		return EPROTO;

	/* Rejects out-of-range channels before any native hardware mutation. */
	error = keiland_audio_set_volume(audio, 101, 50, 0);
	if (error != EINVAL)
		return EPROTO;

	/* Rejects an invalid mute flag independently of the channel range. */
	error = keiland_audio_set_volume(audio, 50, 50, 2);
	if (error != EINVAL)
		return EPROTO;

	/* Applies independent left/right percentages through the production backend. */
	error = audio_set(audio, 40, 65, 0);
	if (error != 0)
		return error;

	/* Applies native mute without replacing the remembered channel percentages with zero. */
	error = audio_set(audio, 40, 65, 1);
	if (error != 0)
		return error;

	/* Unmutes through the same public interface before testing another process. */
	error = audio_set(audio, 40, 65, 0);
	if (error != 0)
		return error;

	/* Changes native hardware through FreeBSD's independent mixer utility. */
	error = system("mixer vol.volume=0.70:0.30 vol.mute=off >/dev/null");
	if (error != 0)
		return EIO;

	/* Waits only for the backend's documented one-second refresh interval. */
	error = nanosleep(&delay, NULL);
	if (error != 0)
		return errno;

	/* Requires an externally changed mixer to become visible without an event fd. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0)
		return error;

	/* Requires the public change mask to report the externally changed volume. */
	if ((changed & KEILAND_AUDIO_CHANGED_VOLUME) == 0)
		return EPROTO;

	/* Reads the independent utility's values through the subscription's refreshed public cache. */
	keiland_audio_get_state(audio, &state);
	if (state.left != 70 || state.right != 30 || state.muted != 0)
		return EPROTO;

	/* Feedback remains a silent accepted request for the connected mixer-only backend. */
	error = keiland_audio_feedback(audio);
	if (error != 0)
		return error;

	/* Succeeded: public writes and external changes agreed with native hardware state. */
	return 0;
}

/* Applies a public change and checks both library and independent libmixer snapshots. */
static int
audio_set(
	struct keiland_audio *audio,
	unsigned left,
	unsigned right,
	unsigned muted)
{
	struct keiland_audio_state state;
	unsigned changed;
	int error;

	/* Applies a real OSS mutation through the production public interface. */
	error = keiland_audio_set_volume(audio, left, right, muted);
	if (error != 0)
		return error;

	/* A local mutation must be read back on the next ordinary update. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0)
		return error;

	/* Compares the actual device readback with the requested native percentages. */
	keiland_audio_get_state(audio, &state);
	if (state.left != left || state.right != right || state.muted != muted)
		return EPROTO;

	/* Checks the same values through a separately opened native control implementation. */
	error = audio_independent(left, right, muted);
	if (error != 0)
		return error;

	/* Succeeded: both independent native snapshots agree with this public mutation. */
	return 0;
}

/* Reads the native master volume and mute through base libmixer, not our ioctl wrapper. */
static int
audio_independent(
	unsigned left,
	unsigned right,
	unsigned muted)
{
	struct mixer *mixer;
	struct mix_dev *device;
	unsigned native_left;
	unsigned native_right;
	unsigned native_muted;
	int devno;
	int error;

	/* Opens a fresh independent snapshot rather than reusing stale libmixer cache fields. */
	mixer = mixer_open(NULL);
	if (mixer == NULL)
		return errno;

	/* Requires the actual HDA master control tested by the independent mixer utility. */
	device = mixer_get_dev_byname(mixer, "vol");
	if (device == NULL) {
		(void)mixer_close(mixer);
		return ENODEV;
	}

	/* Converts native libmixer fractions to the public percentage representation. */
	native_left = (unsigned)(device->vol.left * 100.0f + 0.5f);
	native_right = (unsigned)(device->vol.right * 100.0f + 0.5f);
	devno = device->devno;
	native_muted = 0;
	if ((mixer->mutemask & (1U << devno)) != 0)
		native_muted = 1;

	/* Releases the independent native descriptor before reporting a mismatch. */
	error = mixer_close(mixer);
	if (error != 0)
		return errno;

	/* Any disagreement must remain visible to the mutation owner's restoration path. */
	if (native_left != left || native_right != right || native_muted != muted)
		return EPROTO;

	/* Succeeded: base libmixer agrees with the production backend's requested hardware values. */
	return 0;
}
