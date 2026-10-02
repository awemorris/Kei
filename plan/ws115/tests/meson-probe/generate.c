/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Meson cross probe generator: a build-machine program that writes the
 * header the probe library includes.
 */

#include <stdio.h>

/*
 * Writes probe-seed.h to the path given as the only argument.
 */
int
main(
	int argc,
	char **argv)
{
	FILE *output;
	int written;
	int closed;

	/* Refuses a call without exactly one output path. */
	if (argc != 2)
		return 2;

	/* Opens the header the build asked for. */
	output = fopen(argv[1], "w");
	if (output == NULL)
		return 1;

	/* Writes the seed the library starts its checksum from. */
	written = fprintf(output, "#define PROBE_SEED 0x5a5aUL\n");
	if (written < 0) {
		fclose(output);
		return 1;
	}

	/* Closes the header; a failed close means it was not completely written. */
	closed = fclose(output);
	if (closed != 0)
		return 1;

	/* Succeeded: the header is written. */
	return 0;
}
