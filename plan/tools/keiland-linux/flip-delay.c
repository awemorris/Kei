/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Forces one transient DRM poll timeout in a test-only preload library. */
#include <dlfcn.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>

/* The fixture's single render thread needs only one real poll pointer. */
static int (*delay_poll)(struct pollfd *, nfds_t, int);

/* Each finite probe process receives exactly one delayed DRM event wait. */
static unsigned delay_used;

/*
 * Delays one DRM wait and preserves all subsequent real kernel results.
 */
int
poll(
	struct pollfd *descriptors,
	nfds_t count,
	int milliseconds)
{
	struct stat status;
	struct timespec delay;
	int error;
	int character;
	unsigned device_major;

	/* Resolves the unchanged libc operation before inspecting any candidate descriptor. */
	if (delay_poll == NULL) {
		delay_poll = (int (*)(struct pollfd *, nfds_t, int))dlsym(RTLD_NEXT, "poll");
		if (delay_poll == NULL)
			abort();
	}

	/* Only a single primary DRM descriptor is part of this fixture's injection scope. */
	if (delay_used == 0 &&
	    count == 1 &&
	    milliseconds > 0) {
		/* The descriptor must name an actual character device owned by DRM. */
		error = fstat(descriptors[0].fd, &status);
		if (error == 0) {
			device_major = major(status.st_rdev);
			character = S_ISCHR(status.st_mode);
			if (character != 0 && device_major == 226) {
				/* Simulates a transient scheduler delay without changing any production setting. */
				delay_used = 1;
				delay.tv_sec = 0;
				delay.tv_nsec = 250000000;
				(void)nanosleep(&delay, NULL);
				descriptors[0].revents = 0;
				fprintf(stderr, "DRM_DELAY requested_ms=%d delayed_ms=250 result=0\n", milliseconds);
				return 0;
			}
		}
	}

	/* Returns the real poll result and descriptor events unchanged outside the single injection. */
	error = delay_poll(descriptors, count, milliseconds);
	if (error < 0)
		return error;

	/* Succeeded: every ordinary wait retains its real kernel outcome. */
	return error;
}
