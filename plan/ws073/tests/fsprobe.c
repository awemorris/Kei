/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws073-p045 (BUG-135): measures how long file operations take in the
 * guest, to find the stalls of seconds that stat() showed.
 *
 *   fsprobe stat PATH COUNT INTERVAL_MS     stat() PATH COUNT times, INTERVAL_MS apart
 *   fsprobe open PATH COUNT INTERVAL_MS     open() and close() PATH the same way
 *   fsprobe fsync DIR COUNT INTERVAL_MS     write 16 KiB to a file in DIR and fsync() it, the same way
 *   fsprobe replace PATH COUNT INTERVAL_MS  write PATH.new, fsync() it and rename() it over PATH (as the desktop's
 *                                           preferences are saved), the same way
 *   fsprobe nap - COUNT INTERVAL_MS         sleep 10 ms as the operation (a stall of the whole guest
 *                                           or of its scheduler, which no file operation causes)
 *   fsprobe write DIR SECONDS               write and fsync files in DIR without pause (a load for the others)
 *
 * Each timed mode prints one line for an operation over 100 ms (its time
 * since the start and how long it took) and a summary line:
 *   FSPROBE op=stat count=N p50_us= p90_us= p99_us= max_us= over100ms= over1s=
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The most operations one run times. */
#define PROBE_MAX	100000

/* The time over which an operation is reported on its own, in microseconds. */
#define PROBE_SLOW_US	100000ULL

/* The bytes one fsync operation writes. */
#define PROBE_WRITE_BYTES	16384

/* The time each operation took, in microseconds, for the summary (sorted at the end). */
static unsigned long long probe_times[PROBE_MAX];

static unsigned long long probe_now_us(void);
static int probe_once(const char *mode, const char *path, unsigned long index);
static int probe_compare(const void *left, const void *right);
static int probe_write_load(const char *directory, unsigned long seconds);
static int probe_write_file(const char *path);

/*
 * Runs one mode of the probe.
 */
int
main(
	int argc,
	char **argv)
{
	unsigned long long started;
	unsigned long long before;
	unsigned long long took;
	unsigned long count;
	unsigned long interval;
	unsigned long index;
	unsigned long over_100ms;
	unsigned long over_1s;
	struct timespec pause;
	int error;
	int same;

	/* The load mode has its own arguments. */
	if (argc == 4) {
		same = strcmp(argv[1], "write");
		if (same == 0)
			return probe_write_load(argv[2], strtoul(argv[3], NULL, 10));
	}

	/* A timed mode: the operation, the path, how many and how far apart. */
	if (argc != 5) {
		fprintf(stderr, "usage: fsprobe stat|open|fsync|replace PATH COUNT INTERVAL_MS, or fsprobe write DIR SECONDS\n");
		return 2;
	}
	count = strtoul(argv[3], NULL, 10);
	interval = strtoul(argv[4], NULL, 10);
	if (count == 0UL || count > PROBE_MAX)
		count = PROBE_MAX;

	/* Each operation, timed; a slow one is said at once. */
	over_100ms = 0;
	over_1s = 0;
	started = probe_now_us();
	for (index = 0; index < count; index++) {
		before = probe_now_us();
		error = probe_once(argv[1], argv[2], index);
		took = probe_now_us() - before;
		if (error != 0) {
			fprintf(stderr, "fsprobe: %s %s: %s\n", argv[1], argv[2], strerror(error));
			return 1;
		}
		probe_times[index] = took;

		/* A slow one: when, and how long. */
		if (took >= PROBE_SLOW_US) {
			over_100ms++;
			if (took >= 1000000ULL)
				over_1s++;
			printf("FSPROBE slow op=%s index=%lu at_ms=%llu took_us=%llu\n", argv[1], index, (before - started) / 1000ULL, took);
			fflush(stdout);
		}

		/* The pause before the next. */
		pause.tv_sec = (time_t)(interval / 1000UL);
		pause.tv_nsec = (long)(interval % 1000UL) * 1000000L;
		if (interval != 0UL)
			(void)nanosleep(&pause, NULL);
	}

	/* The summary, from the sorted times. */
	qsort(probe_times, count, sizeof(probe_times[0]), probe_compare);
	printf("FSPROBE op=%s count=%lu p50_us=%llu p90_us=%llu p99_us=%llu max_us=%llu over100ms=%lu over1s=%lu\n", argv[1], count,
	       probe_times[count / 2UL], probe_times[count * 9UL / 10UL], probe_times[count * 99UL / 100UL], probe_times[count - 1UL],
	       over_100ms, over_1s);

	/* Succeeded: every operation was timed. */
	return 0;
}

/* Reads the monotonic clock in microseconds. */
static unsigned long long
probe_now_us(void)
{
	struct timespec now;

	/* The clock (it does not fail with this clock id). */
	(void)clock_gettime(CLOCK_MONOTONIC, &now);

	/* The time in microseconds. */
	return (unsigned long long)now.tv_sec * 1000000ULL + (unsigned long long)now.tv_nsec / 1000ULL;
}

/* Carries out one operation of a mode; returns 0 or an errno value. */
static int
probe_once(
	const char *mode,
	const char *path,
	unsigned long index)
{
	struct stat status;
	struct timespec nap;
	char file[512];
	int descriptor;
	int result;
	int same;

	/* stat(). */
	same = strcmp(mode, "stat");
	if (same == 0) {
		result = stat(path, &status);
		if (result != 0)
			return errno;
		return 0;
	}

	/* open() and close(). */
	same = strcmp(mode, "open");
	if (same == 0) {
		descriptor = open(path, O_RDONLY);
		if (descriptor < 0)
			return errno;
		(void)close(descriptor);
		return 0;
	}

	/* A write and fsync() of a file of its own in the folder (one of eight, reused). */
	same = strcmp(mode, "fsync");
	if (same == 0) {
		(void)snprintf(file, sizeof(file), "%s/fsprobe-%lu", path, index % 8UL);
		return probe_write_file(file);
	}

	/* A 10 ms sleep, which takes longer only when the guest itself stalls. */
	same = strcmp(mode, "nap");
	if (same == 0) {
		nap.tv_sec = 0;
		nap.tv_nsec = 10000000L;
		(void)nanosleep(&nap, NULL);
		return 0;
	}

	/* A new file beside the path, flushed and renamed over it. */
	same = strcmp(mode, "replace");
	if (same == 0) {
		(void)snprintf(file, sizeof(file), "%s.new", path);
		result = probe_write_file(file);
		if (result != 0)
			return result;
		result = rename(file, path);
		if (result != 0)
			return errno;
		return 0;
	}

	/* No such mode. */
	return EINVAL;
}

/* Orders two times for qsort(). */
static int
probe_compare(
	const void *left,
	const void *right)
{
	unsigned long long a;
	unsigned long long b;

	/* The two times. */
	a = *(const unsigned long long *)left;
	b = *(const unsigned long long *)right;

	/* Smaller first. */
	if (a < b)
		return -1;
	if (a > b)
		return 1;
	return 0;
}

/* Writes and fsyncs files in a folder for some seconds without pause; returns 0 or 1. */
static int
probe_write_load(
	const char *directory,
	unsigned long seconds)
{
	unsigned long long until;
	unsigned long long now;
	unsigned long files;
	char file[512];
	int error;

	/* Until the time is up, a file after another. */
	until = probe_now_us() + (unsigned long long)seconds * 1000000ULL;
	files = 0;
	for (;;) {
		now = probe_now_us();
		if (now >= until)
			break;
		(void)snprintf(file, sizeof(file), "%s/fsload-%lu", directory, files % 32UL);
		error = probe_write_file(file);
		if (error != 0) {
			fprintf(stderr, "fsprobe: write %s: %s\n", file, strerror(error));
			return 1;
		}
		files++;
	}

	/* How much was written. */
	printf("FSPROBE op=write files=%lu seconds=%lu\n", files, seconds);
	return 0;
}

/* Writes PROBE_WRITE_BYTES to a file (made or truncated) and fsyncs it; returns 0 or an errno value. */
static int
probe_write_file(
	const char *path)
{
	static char bytes[PROBE_WRITE_BYTES];
	ssize_t written;
	int descriptor;
	int result;
	int error;

	/* The file. */
	descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (descriptor < 0)
		return errno;

	/* Its bytes, then their way to the disk. */
	memset(bytes, 'z', sizeof(bytes));
	written = write(descriptor, bytes, sizeof(bytes));
	if (written != (ssize_t)sizeof(bytes)) {
		error = errno;
		(void)close(descriptor);
		if (error == 0)
			error = EIO;
		return error;
	}
	result = fsync(descriptor);
	if (result != 0) {
		error = errno;
		(void)close(descriptor);
		return error;
	}

	/* Succeeded: the file is on the disk. */
	(void)close(descriptor);
	return 0;
}
