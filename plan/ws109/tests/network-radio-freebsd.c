/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks an idempotent native flag write against independent kernel inquiries.
 */
#include "userland/desktop/libkeiland/wpa/network-wpa.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int probe_flags(int descriptor);

/*
 * Verifies that requesting an already enabled native interface preserves both flag halves.
 */
int
main(
	void)
{
	int descriptor;
	int error;

	/* Opens an independent native inquiry socket without sharing backend ownership. */
	descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (descriptor < 0)
		return 1;

	/* Releases the inquiry descriptor before returning any assertion outcome. */
	error = probe_flags(descriptor);
	(void)close(descriptor);
	if (error != 0) {
		(void)fprintf(stderr, "native flag preservation failed errno=%d\n", error);
		return 1;
	}

	/* Succeeded: the actual native write preserved all unrelated low and high flags. */
	return 0;
}

/* Compares the native flag pair before and after an idempotent administrative request. */
static int
probe_flags(
	int descriptor)
{
	struct ifreq before;
	struct ifreq after;
	int error;

	/* The owned guest's live SSH interface must already be administratively enabled. */
	memset(&before, 0, sizeof(before));
	memcpy(before.ifr_name, "vtnet0", sizeof("vtnet0"));
	error = ioctl(descriptor, SIOCGIFFLAGS, &before);
	if (error != 0)
		return errno;

	/* Refuses to change a fixture whose original administrative state is down. */
	if ((before.ifr_flags & IFF_UP) == 0)
		return EINVAL;

	/* Requests the existing state through the production backend's native flag write. */
	error = kwpa_radio("vtnet0", 1);
	if (error != 0)
		return error;

	/* Reads the installed flags through an independently owned native socket. */
	memset(&after, 0, sizeof(after));
	memcpy(after.ifr_name, "vtnet0", sizeof("vtnet0"));
	error = ioctl(descriptor, SIOCGIFFLAGS, &after);
	if (error != 0)
		return errno;

	/* Driver flags and native high-word flags must survive the administrative request. */
	if (before.ifr_flags != after.ifr_flags || before.ifr_flagshigh != after.ifr_flagshigh)
		return EPROTO;

	/* Succeeded: both kernel-reported flag halves are unchanged. */
	(void)printf("PASS native flag preservation low=%04x high=%04x\n", (unsigned short)after.ifr_flags, (unsigned short)after.ifr_flagshigh);
	return 0;
}
