/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Meson cross probe program: it links against the probe library.
 */

#include <stdio.h>

#include "probe.h"

/*
 * Prints the probe checksum of a fixed string.
 */
int
main(void)
{
	unsigned long checksum;

	/* Asks the library for the checksum of a fixed string. */
	checksum = (unsigned long)probe_checksum("zedbsd");

	/* Reports the checksum for a caller that runs the program. */
	printf("probe %08lx\n", checksum);

	/* Succeeded: the checksum was printed. */
	return 0;
}
