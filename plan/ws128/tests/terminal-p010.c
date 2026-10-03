/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of ws128-p010 (BUG-150): Emacs in Terminal drew its
 * screen a line off.
 *
 *   terminal-p010 show FILE COLUMNS ROWS   replays FILE into a grid and
 *                                          prints the grid and the cursor
 *   terminal-p010 emacs FILE ROW TEXT      checks the first frame of
 *                                          emacs -nw -Q on an 80x24 xterm:
 *                                          the cursor on ROW, row 2
 *                                          starting with TEXT
 *   terminal-p010 wrap                     checks the right margin: a
 *                                          character in the last column
 *                                          leaves the wrap pending
 *
 * It links the terminal's own screen.c and width.c.
 */

#include "userland/desktop/terminal/terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The largest replayed file; the captures are a few kilobytes. */
#define TEST_FILE_MAX	(1024U * 1024U)

/*
 * The grid under test; file-scope because it is too large for the stack.
 * It lives for the whole run.
 */
static struct terminal_screen test_screen;

/* The bytes of the replayed file; file-scope for the same reason. */
static unsigned char test_bytes[TEST_FILE_MAX];

/* How many checks failed; the run fails when it is not zero at the end. */
static unsigned test_failures;

static int test_replay(const char *path, unsigned columns, unsigned rows);
static void test_show(void);
static void test_emacs(unsigned cursor_row, const char *second);
static void test_wrap(void);
static void test_row_text(unsigned row, char *text, size_t size);
static int test_row_starts(unsigned row, const char *prefix);
static int test_row_blank(unsigned row);
static void test_expect(int condition, const char *what);
static void test_write(const char *text);

/*
 * Runs the test named on the command line.
 */
int
main(
	int argc,
	char **argv)
{
	unsigned columns;
	unsigned rows;
	int error;

	/* Replays a file and prints the grid it leaves. */
	if (argc == 5 && strcmp(argv[1], "show") == 0) {
		columns = (unsigned)strtoul(argv[3], NULL, 10);
		rows = (unsigned)strtoul(argv[4], NULL, 10);
		error = test_replay(argv[2], columns, rows);
		if (error != 0)
			return 2;

		test_show();
		return 0;
	}

	/* Replays the Emacs capture and checks its frame. */
	if (argc == 5 && strcmp(argv[1], "emacs") == 0) {
		error = test_replay(argv[2], 80U, 24U);
		if (error != 0)
			return 2;

		test_show();
		rows = (unsigned)strtoul(argv[3], NULL, 10);
		test_emacs(rows, argv[4]);
	} else if (argc == 2 && strcmp(argv[1], "wrap") == 0) {
		test_wrap();
	} else {
		fprintf(stderr, "usage: terminal-p010 show FILE COLUMNS ROWS | emacs FILE ROW TEXT | wrap\n");
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

/* Reads a capture into a fresh grid of the given size. */
static int
test_replay(
	const char *path,
	unsigned columns,
	unsigned rows)
{
	FILE *file;
	size_t length;

	/* Refuses a size the grid cannot hold. */
	if (columns == 0U || rows == 0U || columns > TERMINAL_MAX_COLUMNS || rows > TERMINAL_MAX_ROWS) {
		fprintf(stderr, "terminal-p010: bad size %ux%u\n", columns, rows);
		return -1;
	}

	/* Reads the bytes the program wrote to its terminal. */
	file = fopen(path, "rb");
	if (file == NULL) {
		perror(path);
		return -1;
	}

	length = fread(test_bytes, 1U, sizeof(test_bytes), file);
	fclose(file);

	/* Feeds them to a new grid as the pty would. */
	terminal_screen_init(&test_screen, columns, rows);
	terminal_screen_write(&test_screen, test_bytes, length);

	/* Succeeded: the grid holds what the program drew. */
	return 0;
}

/* Prints each row of the grid between bars, and the cursor. */
static void
test_show(void)
{
	char text[TERMINAL_MAX_COLUMNS * 4U + 1U];
	unsigned row;

	/* Each row, numbered from one as a terminal counts them. */
	for (row = 0U; row < test_screen.rows; row++) {
		test_row_text(row, text, sizeof(text));
		printf("%2u|%s|\n", row + 1U, text);
	}

	/* The cursor, numbered from one too. */
	printf("cursor row=%u column=%u\n", test_screen.cursor_row + 1U, test_screen.cursor_column + 1U);
}

/*
 * Checks the first frame of emacs -nw -Q: the menu bar on the first row,
 * the buffer's first line on the second (second is its start, or empty
 * for an empty *scratch*, whose rows 2 to 22 are then all blank), the
 * mode line on the row above the echo area, and the cursor on the first
 * column of cursor_row (counted from one).
 */
static void
test_emacs(
	unsigned cursor_row,
	const char *second)
{
	unsigned row;
	int blank;

	/* The menu bar is the first row. */
	test_expect(test_row_starts(0U, "File Edit Options Buffers Tools"), "emacs: the menu bar is on row 1");

	/* The buffer starts on row 2. */
	test_expect(test_row_starts(1U, second), "emacs: the buffer starts on row 2");

	/* An empty *scratch* leaves rows 2 to 22 blank. */
	if (second[0] == '\0') {
		blank = 1;
		for (row = 1U; row < 22U; row++) {
			if (!test_row_blank(row))
				blank = 0;
		}

		test_expect(blank, "emacs: rows 2 to 22 are the empty buffer");
	}

	/* The mode line is row 23, just above the echo area. */
	test_expect(test_row_starts(22U, "-UUU:"), "emacs: the mode line is on row 23");

	/* The cursor waits where Emacs put point, on the first column. */
	test_expect(test_screen.cursor_row + 1U == cursor_row && test_screen.cursor_column == 0U, "emacs: the cursor is on the row of point, column 1");
}

/*
 * Checks the right margin as xterm and the VT100 keep it (terminfo's
 * am and xenl): a character in the last column leaves the cursor there
 * with a wrap pending; only the next printed character wraps, and a
 * carriage return, a line feed or a move cancels the wrap.
 */
static void
test_wrap(void)
{
	char line[11];

	/* Ten columns of text on a ten-column row leave the cursor on its last column. */
	memset(line, 'a', 10U);
	line[10] = '\0';
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write(line);
	test_expect(test_screen.cursor_row == 0U && test_screen.cursor_column == 9U, "a full row keeps the cursor on its last column");

	/* CR LF after a full row goes to the next row, not the one after it. */
	test_write("\r\n");
	test_expect(test_screen.cursor_row == 1U && test_screen.cursor_column == 0U, "CR LF after a full row moves one row down");

	/* The next character after a full row wraps, and an SGR between them keeps the wrap. */
	test_write(line);
	test_write("\033[1m" "b");
	test_expect(test_screen.cursor_row == 2U && test_screen.cursor_column == 1U, "a character after a full row and an SGR wraps");
	test_expect(terminal_screen_cell(&test_screen, 0U, 2U)->codepoint == 'b', "the wrapped character starts the next row");
	test_expect(terminal_screen_cell(&test_screen, 9U, 1U)->codepoint == 'a', "the wrap keeps the last column of the full row");

	/* A full last row does not scroll the screen; the next character does. */
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write("top\033[4;1H");
	test_write(line);
	test_expect(test_row_starts(0U, "top") && test_screen.cursor_row == 3U, "a full last row does not scroll");
	test_write("c");
	test_expect(test_row_starts(0U, "") && test_screen.cursor_row == 3U && test_screen.cursor_column == 1U, "the next character scrolls and wraps");

	/* A cursor move cancels the pending wrap. */
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write(line);
	test_write("\033[1;5H" "x");
	test_expect(test_screen.cursor_row == 0U && test_screen.cursor_column == 5U, "a move after a full row cancels the wrap");

	/* A backspace from the pending wrap goes to the column before the last, as xterm does. */
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write(line);
	test_write("\b" "y");
	test_expect(terminal_screen_cell(&test_screen, 8U, 0U)->codepoint == 'y' && test_screen.cursor_row == 0U, "a backspace cancels the wrap");

	/* A wide character that does not fit the last column still starts the next row. */
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write("aaaaaaaaa" "\xe6\x97\xa5");
	test_expect(test_screen.cursor_row == 1U && test_screen.cursor_column == 2U, "a wide character at the last column goes to the next row");

	/* A wide character that ends on the last column leaves the wrap pending too. */
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write("aaaaaaaa" "\xe6\x97\xa5" "\r\n");
	test_expect(test_screen.cursor_row == 1U && test_screen.cursor_column == 0U, "CR LF after a wide character in the last two columns moves one row");

	/* ESC 7 and ESC 8 bring the cursor back without a stale wrap. */
	terminal_screen_init(&test_screen, 10U, 4U);
	test_write("\0337");
	test_write(line);
	test_write("\0338" "z");
	test_expect(test_screen.cursor_row == 0U && test_screen.cursor_column == 1U, "ESC 8 cancels the wrap");
}

/* Copies a row's characters as UTF-8, trailing blanks left out. */
static void
test_row_text(
	unsigned row,
	char *text,
	size_t size)
{
	const struct terminal_cell *cell;
	size_t length;
	size_t end;
	unsigned column;
	uint32_t codepoint;

	/* Each cell's character; a blank or empty cell is a space. */
	length = 0U;
	end = 0U;
	for (column = 0U; column < test_screen.columns; column++) {
		cell = terminal_screen_cell(&test_screen, column, row);
		if (cell->continuation)
			continue;

		codepoint = cell->codepoint;
		if (codepoint == 0U)
			codepoint = ' ';

		/* The character as UTF-8 while it fits. */
		if (length + 5U > size)
			break;

		if (codepoint < 0x80U) {
			text[length++] = (char)codepoint;
		} else if (codepoint < 0x800U) {
			text[length++] = (char)(0xc0U | (codepoint >> 6));
			text[length++] = (char)(0x80U | (codepoint & 0x3fU));
		} else if (codepoint < 0x10000U) {
			text[length++] = (char)(0xe0U | (codepoint >> 12));
			text[length++] = (char)(0x80U | ((codepoint >> 6) & 0x3fU));
			text[length++] = (char)(0x80U | (codepoint & 0x3fU));
		} else {
			text[length++] = (char)(0xf0U | (codepoint >> 18));
			text[length++] = (char)(0x80U | ((codepoint >> 12) & 0x3fU));
			text[length++] = (char)(0x80U | ((codepoint >> 6) & 0x3fU));
			text[length++] = (char)(0x80U | (codepoint & 0x3fU));
		}

		/* The text ends after its last character that is not a space. */
		if (codepoint != ' ')
			end = length;
	}

	text[end] = '\0';
}

/* Reports whether a row's text starts with the prefix (an empty prefix: whether the row is blank). */
static int
test_row_starts(
	unsigned row,
	const char *prefix)
{
	char text[TERMINAL_MAX_COLUMNS * 4U + 1U];

	/* The row's text. */
	test_row_text(row, text, sizeof(text));

	/* An empty prefix asks for a blank row. */
	if (prefix[0] == '\0') {
		if (text[0] == '\0')
			return 1;

		return 0;
	}

	/* Reports whether the text starts with the prefix. */
	if (strncmp(text, prefix, strlen(prefix)) == 0)
		return 1;

	return 0;
}

/* Reports whether a row holds only blanks. */
static int
test_row_blank(
	unsigned row)
{
	int blank;

	/* A blank row's text is empty. */
	blank = test_row_starts(row, "");

	/* Reports the row's state. */
	return blank;
}

/* Counts a failed check and prints every check. */
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
