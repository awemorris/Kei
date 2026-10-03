/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * BUG-133 (ws074-p177): wb_units_reserve must refuse, not wrap, a need whose
 * doubled capacity has a byte count past SIZE_MAX.  On a 64-bit host an empty
 * buffer asked for 2^62 + 1 units rounded to 2^63 units, 2^64 bytes, which
 * wrapped to 0: realloc(NULL, 0) succeeded and the reserve reported room it
 * did not have.  A host test against base/buffer.c:
 *
 *	cc -I userland/desktop/libbrowser plan/ws074/tests/bug133-units.c \
 *	    userland/desktop/libbrowser/base/buffer.c -o bug133-units && ./bug133-units
 */

#include "base/base.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

/* Runs the cases and prints PASS or FAIL. */
int
main(
	void)
{
	struct wb_units units;
	uint16_t sample[3];
	size_t huge;
	int failures;
	int error;

	/* The ticket's request: 2^62 + 1 units on a 64-bit size_t (2^(bits-2) + 1 in general). */
	failures = 0;
	huge = (((size_t)-1) >> 2) + 2U;
	wb_units_init(&units);
	error = wb_units_reserve(&units, huge);
	if (error != ENOMEM) {
		printf("bug133: reserve of %zu units: error %d capacity %zu (want ENOMEM) FAIL\n", huge, error, units.capacity);
		failures++;
	} else {
		printf("bug133: reserve of %zu units: ENOMEM ok\n", huge);
	}

	/* Frees the buffer. */
	wb_units_release(&units);

	/* A request past half of SIZE_MAX is refused as before. */
	wb_units_init(&units);
	error = wb_units_reserve(&units, ((size_t)-1) / 2U + 1U);
	if (error != ENOMEM) {
		printf("bug133: reserve past SIZE_MAX/2: error %d FAIL\n", error);
		failures++;
	} else {
		printf("bug133: reserve past SIZE_MAX/2: ENOMEM ok\n");
	}

	/* Frees the buffer. */
	wb_units_release(&units);

	/* Ordinary appends still work. */
	sample[0] = 'a';
	sample[1] = 'b';
	sample[2] = 'c';
	wb_units_init(&units);
	error = wb_units_append(&units, sample, 3);
	if (error != 0 || units.length != 3 || units.data[2] != 'c') {
		printf("bug133: an ordinary append: error %d length %zu FAIL\n", error, units.length);
		failures++;
	} else {
		printf("bug133: an ordinary append: ok\n");
	}

	/* Frees the buffer. */
	wb_units_release(&units);

	/* The verdict. */
	if (failures != 0) {
		printf("bug133: FAIL\n");
		return 1;
	}

	/* Every case passed. */
	printf("bug133: PASS\n");
	return 0;
}
