/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws099-p028 (zdesktop's log with NUL bytes): the host reproduction.  A
 * writer prints a line to its standard output (a file the shell opened
 * with ">"), the file is truncated by another ">" (a new zdesktop started
 * while the old one still ends), and the old writer prints again.  Without
 * O_APPEND its next write lands at its old offset and leaves a hole of NUL
 * bytes; with O_APPEND set on the open file (as zdesktop now does at
 * start, log_append in main.c) it lands at the end.
 *
 *   host-log-append FILE plain|append   writes, waits for FILE to shrink, writes again
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int
main(
	int argc,
	char **argv)
{
	struct stat status;
	int flags;
	int tries;

	if (argc != 3)
		return 2;

	/* O_APPEND on the standard output's open file, as zdesktop sets it. */
	if (strcmp(argv[2], "append") == 0) {
		flags = fcntl(STDOUT_FILENO, F_GETFL);
		if (flags >= 0)
			(void)fcntl(STDOUT_FILENO, F_SETFL, flags | O_APPEND);
	}

	/* A first line, long enough that a hole after a truncation is plain to see. */
	printf("ZWL first line of the old run, written before the file is truncated by the next run\n");
	fflush(stdout);

	/* Waits for the file to be truncated (the next run's ">"). */
	for (tries = 0; tries < 200; tries++) {
		if (stat(argv[1], &status) == 0 && status.st_size == 0)
			break;
		usleep(10000);
	}

	/* The old run's last line. */
	printf("ZWL DONE old run\n");
	fflush(stdout);
	return 0;
}
