/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Meson cross probe library: one function that uses zlib and a value
 * generated during the build.
 */

#include <string.h>
#include <zlib.h>

#include "probe.h"
#include "probe-seed.h"

/*
 * Computes the CRC-32 of a string, started from the generated seed.
 *
 * The seed comes from a header a build-machine program wrote, so a correct
 * value shows that the native and the cross halves of the build met.
 */
uint32_t
probe_checksum(
	const char *text)
{
	uLong checksum;
	size_t length;

	/* Measures the text the checksum covers. */
	length = strlen(text);

	/* Folds the text into the checksum that starts from the seed. */
	checksum = crc32(PROBE_SEED, (const Bytef *)text, (uInt)length);

	/* Succeeded: the checksum of the text. */
	return (uint32_t)checksum;
}
