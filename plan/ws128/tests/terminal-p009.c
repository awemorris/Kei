/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of ws128-p009: Terminal's "Treat Ambiguous-Width
 * Characters as Wide".
 *
 *   terminal-p009 table      prints every code point's width (off, on)
 *                            as ranges, for terminal-p009.sh to compare
 *                            with the Unicode data
 *   terminal-p009 check      the grid and the settings file
 *   terminal-p009 speed      the time of a large output, off and on
 *
 * It links the terminal's own screen.c, width.c and settings.c.
 */

#include "userland/desktop/terminal/terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The largest code point. */
#define TEST_LAST_CODEPOINT	0x10ffffU

/*
 * The grid under test; file-scope because it is too large for the stack.
 * It lives for the whole run.
 */
static struct terminal_screen test_screen;

/* How many checks failed; the run fails when it is not zero at the end. */
static unsigned test_failures;

static void test_table(void);
static void test_check(void);
static void test_speed(void);
static void test_expect(int condition, const char *what);
static void test_write(const char *text);
static double test_seconds(void);

/*
 * Runs the test named on the command line.
 */
int
main(
	int argc,
	char **argv)
{
	/* The test to run. */
	if (argc != 2) {
		fprintf(stderr, "usage: terminal-p009 table|check|speed\n");
		return 2;
	}

	/* One test per run. */
	if (strcmp(argv[1], "table") == 0) {
		test_table();
	} else if (strcmp(argv[1], "check") == 0) {
		test_check();
	} else if (strcmp(argv[1], "speed") == 0) {
		test_speed();
	} else {
		fprintf(stderr, "terminal-p009: unknown test %s\n", argv[1]);
		return 2;
	}

	/* Reports the outcome. */
	if (test_failures != 0U) {
		printf("FAIL %u\n", test_failures);
		return 1;
	}

	/* Succeeded: every check held. */
	return 0;
}

/* Prints the ranges of code points that are wide with the setting off, and with it on. */
static void
test_table(void)
{
	uint32_t codepoint;
	uint32_t first;
	int setting;
	int wide;
	int open;

	/* Off, then on: each run of wide code points as "off|on FIRST LAST". */
	for (setting = 0; setting <= 1; setting++) {
		open = 0;
		first = 0U;
		for (codepoint = 0U; codepoint <= TEST_LAST_CODEPOINT + 1U; codepoint++) {
			wide = 0;
			if (codepoint <= TEST_LAST_CODEPOINT)
				wide = terminal_width_wide(codepoint, setting);

			/* A run starts at a wide code point after a narrow one, and ends at a narrow one. */
			if (wide && !open) {
				first = codepoint;
				open = 1;
			} else if (!wide && open) {
				printf("%s %04X %04X\n", setting ? "on" : "off", (unsigned)first, (unsigned)(codepoint - 1U));
				open = 0;
			}
		}
	}
}

/* Checks the grid's widths, the change of the setting, and the settings file. */
static void
test_check(void)
{
	struct terminal_settings settings;
	struct terminal_cell *cell;
	char path[1024];
	char text[256];
	FILE *file;
	size_t length;
	int error;

	/* With the setting off, a circle is one cell and an ideograph two. */
	terminal_screen_init(&test_screen, 20U, 4U);
	test_write("\xe2\x97\x8b" "a");
	test_expect(test_screen.cursor_column == 2U, "off: circle and a take two cells");
	test_write("\xe6\x97\xa5");
	test_expect(test_screen.cursor_column == 4U, "off: an ideograph takes two cells");

	/* On: the next circle takes two cells; the first keeps its one. */
	terminal_screen_set_ambiguous_wide(&test_screen, 1);
	test_write("\xe2\x97\x8b");
	test_expect(test_screen.cursor_column == 6U, "on: a circle takes two cells");
	cell = terminal_screen_cell(&test_screen, 5U, 0U);
	test_expect(cell->continuation == 1, "on: the circle's right half is a continuation");
	cell = terminal_screen_cell(&test_screen, 1U, 0U);
	test_expect(cell->codepoint == 'a' && cell->continuation == 0, "on: the circle written before keeps one cell");

	/* On: the samples of the phase (black square, reference mark, alpha, yo, circled one, box line). */
	test_write("\xe2\x96\xa0" "\xe2\x80\xbb" "\xce\xb1" "\xd1\x91" "\xe2\x91\xa0" "\xe2\x94\x80");
	test_expect(test_screen.cursor_column == 18U, "on: six Ambiguous samples take twelve cells");

	/* On: ASCII, a combining accent and a soft hyphen stay one cell (the terminal draws them in a cell of their own). */
	terminal_screen_init(&test_screen, 20U, 4U);
	terminal_screen_set_ambiguous_wide(&test_screen, 1);
	test_write("e" "\xcc\x81" "\xc2\xad");
	test_expect(test_screen.cursor_column == 3U, "on: e, U+0301 and U+00AD take one cell each");
	test_expect(terminal_width_wide(0x0301U, 1) == 0, "on: U+0301 (Mn) is not wide");
	test_expect(terminal_width_wide(0x20e3U, 1) == 0, "on: U+20E3 (Me) is not wide");
	test_expect(terminal_width_wide(0xfe0fU, 1) == 0, "on: U+FE0F (Mn) is not wide");

	/* Off again: new circles are one cell. */
	terminal_screen_set_ambiguous_wide(&test_screen, 0);
	test_write("\xe2\x97\x8b");
	test_expect(test_screen.cursor_column == 4U, "off again: a circle takes one cell");

	/* A circle that does not fit at the end of a row starts the next one. */
	terminal_screen_init(&test_screen, 5U, 4U);
	terminal_screen_set_ambiguous_wide(&test_screen, 1);
	test_write("abcd" "\xe2\x97\x8b");
	test_expect(test_screen.cursor_row == 1U && test_screen.cursor_column == 2U, "on: a wide circle wraps whole");

	/* The settings file: missing means off. */
	terminal_settings_load(&settings);
	test_expect(settings.ambiguous_wide == 0, "settings: a missing file is off");

	/* Saving on writes the key, and loading reads it back. */
	settings.ambiguous_wide = 1;
	error = terminal_settings_save(&settings);
	test_expect(error == 0, "settings: save on");
	memset(&settings, 0, sizeof(settings));
	terminal_settings_load(&settings);
	test_expect(settings.ambiguous_wide == 1, "settings: load on");

	/* Another key and a comment are kept across a save. */
	snprintf(path, sizeof(path), "%s/.config/keiland/terminal.conf", getenv("HOME"));
	file = fopen(path, "a");
	test_expect(file != NULL, "settings: the file exists");
	if (file != NULL) {
		fputs("# kept\nfuture-key=7\n", file);
		fclose(file);
	}
	settings.ambiguous_wide = 0;
	error = terminal_settings_save(&settings);
	test_expect(error == 0, "settings: save off");
	file = fopen(path, "r");
	length = 0U;
	if (file != NULL) {
		length = fread(text, 1U, sizeof(text) - 1U, file);
		fclose(file);
	}
	text[length] = '\0';
	test_expect(strcmp(text, "# kept\nfuture-key=7\nambiguous-wide=0\n") == 0, "settings: other lines kept, the key replaced");
	terminal_settings_load(&settings);
	test_expect(settings.ambiguous_wide == 0, "settings: load off");

	/* The summary. */
	printf("check: %u failures\n", test_failures);
}

/* Times the same large output with the setting off and on. */
static void
test_speed(void)
{
	static const char sample[] = "The quick brown fox \xe2\x97\x8b\xe2\x96\xa0\xce\xb1\xd1\x91 \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e jumps over\r\n";
	double started;
	double off_seconds;
	double on_seconds;
	unsigned round;
	int setting;

	/* Each setting writes the sample a million times (about 45 MB). */
	off_seconds = 0.0;
	on_seconds = 0.0;
	for (setting = 0; setting <= 1; setting++) {
		terminal_screen_init(&test_screen, 120U, 40U);
		terminal_screen_set_ambiguous_wide(&test_screen, setting);
		started = test_seconds();
		for (round = 0U; round < 1000000U; round++)
			terminal_screen_write(&test_screen, (const unsigned char *)sample, sizeof(sample) - 1U);
		if (setting == 0)
			off_seconds = test_seconds() - started;
		else
			on_seconds = test_seconds() - started;
	}

	/* The times, and their ratio. */
	printf("speed: bytes=%lu off=%.3fs on=%.3fs ratio=%.3f\n", (unsigned long)(sizeof(sample) - 1U) * 1000000UL, off_seconds, on_seconds, on_seconds / off_seconds);
}

/* Counts and prints one check. */
static void
test_expect(
	int condition,
	const char *what)
{
	/* A failed check is counted. */
	if (!condition)
		test_failures++;

	/* Every check is printed. */
	printf("%s %s\n", condition ? "ok  " : "FAIL", what);
}

/* Writes UTF-8 text to the grid as the shell would. */
static void
test_write(
	const char *text)
{
	/* The bytes, without the NUL. */
	terminal_screen_write(&test_screen, (const unsigned char *)text, strlen(text));
}

/* Reports a monotonic time in seconds. */
static double
test_seconds(void)
{
	struct timespec now;

	/* The clock that never steps back. */
	clock_gettime(CLOCK_MONOTONIC, &now);

	/* The time. */
	return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
}
