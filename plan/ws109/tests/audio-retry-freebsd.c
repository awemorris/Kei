/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks permission recovery in the same unprivileged native OSS subscription.
 */

#include <keiland.h>
#include <errno.h>
#include <grp.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

/*
 * Waits for the fixture to restore mixer access and checks a real native reconnect.
 */
int
main(
	void)
{
	struct keiland_audio *audio;
	struct keiland_audio_state state;
	struct timespec delay = {1, 100000000};
	unsigned changed;
	int error;
	int available;
	int byte;

	/* Removes inherited privileged groups before attempting any native device access. */
	error = setgroups(0, NULL);
	if (error != 0)
		return 1;

	/* Uses a fixture-only numeric unprivileged group without relying on an account name. */
	error = setgid(65534);
	if (error != 0)
		return 1;

	/* Makes the fixture's temporary device denial observable to the production open path. */
	error = setuid(65534);
	if (error != 0)
		return 1;

	/* No accessible native output control may be reported as available. */
	available = keiland_audio_available();
	if (available != 0)
		return 1;

	/* Device denial remains recoverable without allocating a replacement subscription. */
	audio = keiland_audio_open();
	if (audio == NULL)
		return 1;

	/* Requires the initial public state to report the denied native mixer. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0) {
		keiland_audio_close(audio);
		return 1;
	}

	/* Reads actual denial state rather than a substituted backend result. */
	keiland_audio_get_state(audio, &state);
	if (state.reachable != 0 || state.device != 0) {
		keiland_audio_close(audio);
		return 1;
	}

	/* An inaccessible mixer cannot accept a public hardware mutation. */
	error = keiland_audio_set_volume(audio, 40, 40, 0);
	if (error != ENOTCONN) {
		keiland_audio_close(audio);
		return 1;
	}

	/* Publishes a coordination point only after actual native open denial was verified. */
	error = puts("audio-retry: DENIED");
	if (error == EOF) {
		keiland_audio_close(audio);
		return 1;
	}

	/* Makes that coordination point visible before waiting for permission restoration. */
	error = fflush(stdout);
	if (error != 0) {
		keiland_audio_close(audio);
		return 1;
	}

	/* The controlling fixture restores the original permissions before allowing this retry. */
	byte = getchar();
	if (byte != 'R') {
		keiland_audio_close(audio);
		return 1;
	}

	/* Waits out the production reconnect interval rather than overriding its clock or retry logic. */
	error = nanosleep(&delay, NULL);
	if (error != 0) {
		keiland_audio_close(audio);
		return 1;
	}

	/* The original subscription must discover the newly accessible native device. */
	error = keiland_audio_update(audio, &changed);
	if (error != 0) {
		keiland_audio_close(audio);
		return 1;
	}

	/* Retires the reconnected descriptor after capturing its actual public state. */
	keiland_audio_get_state(audio, &state);
	keiland_audio_close(audio);

	/* Requires actual native control arrival in both snapshot and public change mask. */
	if (state.reachable != 1 || state.device != 1)
		return 1;

	/* Reachability arrival must remain distinct from ordinary volume changes. */
	if ((changed & KEILAND_AUDIO_CHANGED_REACHABLE) == 0)
		return 1;

	/* Publishes the checked real permission-recovery outcome. */
	(void)puts("audio-retry: PASS same subscription reconnected after native permission restoration");

	/* Succeeded: the caller recovered through the ordinary native retry path. */
	return 0;
}
