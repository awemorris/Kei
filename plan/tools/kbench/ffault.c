/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * BUG-027's measurement of the file-backed page fault: the first MiB of a
 * file mapped MAP_PRIVATE and read one byte a page, the same mapping read
 * again, a second private mapping written one byte a page (copy on write),
 * and the same bytes read with read() a MiB at a time.  Prints microseconds
 * per page and the whole time of each.
 *
 *	ffault FILE [MIB]	(MIB default 32: the ticket's first 32 MiB of libLLVM)
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The size of the pages the test touches, and of one read() of the file. */
#define FFAULT_PAGE_SIZE 4096L
#define FFAULT_CHUNK (1024L * 1024L)

static int fault_pass(const char *name, int descriptor, long pages, int write);
static int read_pass(int descriptor, long pages);
static long long now_ns(void);
static void report(const char *name, long long start, long long end, long pages);

/*
 * Runs the passes in the ticket's order and reports each.
 *
 * Run it right after the guest starts for the first (cold) numbers, and once
 * more for the second (the file's pages already in memory).
 */
int
main(
	int argc,
	char **argv)
{
	struct stat status;
	long mebibytes;
	long pages;
	int descriptor;
	int error;

	/* Reads the arguments. */
	if (argc < 2) {
		fprintf(stderr, "usage: ffault FILE [MIB]\n");
		return 2;
	}

	/* Takes the size to map, 32 MiB unless given. */
	mebibytes = 32;
	if (argc > 2)
		mebibytes = atol(argv[2]);

	/* Opens the file and keeps the whole pages it has. */
	descriptor = open(argv[1], O_RDONLY);
	if (descriptor < 0) {
		perror(argv[1]);
		return 1;
	}

	/* Reads the file's size. */
	error = fstat(descriptor, &status);
	if (error != 0) {
		perror("fstat");
		return 1;
	}

	/* Keeps to the whole pages the file has. */
	pages = mebibytes * (FFAULT_CHUNK / FFAULT_PAGE_SIZE);
	if (status.st_size / FFAULT_PAGE_SIZE < pages)
		pages = (long)(status.st_size / FFAULT_PAGE_SIZE);

	/* The read faults, and again on the same mapping. */
	error = fault_pass("read fault", descriptor, pages, 0);
	if (error != 0)
		return 1;

	/* The written ones, on a mapping of their own. */
	error = fault_pass("cow write fault", descriptor, pages, 1);
	if (error != 0)
		return 1;

	/* The same bytes through read(). */
	error = read_pass(descriptor, pages);
	if (error != 0)
		return 1;

	/* Closes the file. */
	close(descriptor);
	return 0;
}

/* Maps the pages privately and touches each: reads (and reads again) or writes. */
static int
fault_pass(
	const char *name,
	int descriptor,
	long pages,
	int write)
{
	volatile char *memory;
	long long start;
	long page;
	char sink;
	int protection;

	/* Maps the pages. */
	protection = PROT_READ;
	if (write)
		protection |= PROT_WRITE;
	memory = mmap(NULL, (size_t)(pages * FFAULT_PAGE_SIZE), protection, MAP_PRIVATE, descriptor, 0);
	if (memory == MAP_FAILED) {
		perror("mmap");
		return -1;
	}

	/* Touches one byte of each page. */
	sink = 0;
	start = now_ns();
	for (page = 0; page < pages; page++) {
		if (write)
			memory[page * FFAULT_PAGE_SIZE] = (char)page;
		else
			sink ^= memory[page * FFAULT_PAGE_SIZE];
	}

	/* Prints the time of the first touches. */
	report(name, start, now_ns(), pages);

	/* A read mapping is read once more, its pages now present. */
	if (!write) {
		start = now_ns();
		for (page = 0; page < pages; page++)
			sink ^= memory[page * FFAULT_PAGE_SIZE];

		/* Prints the time of the second touches. */
		report("read again", start, now_ns(), pages);
	}

	/* Unmaps the pages. */
	munmap((void *)memory, (size_t)(pages * FFAULT_PAGE_SIZE));
	(void)sink;
	return 0;
}

/* Reads the same bytes a MiB at a time. */
static int
read_pass(
	int descriptor,
	long pages)
{
	char *buffer;
	long long start;
	long remaining;
	long want;
	ssize_t got;

	/* Allocates the buffer. */
	buffer = malloc(FFAULT_CHUNK);
	if (buffer == NULL) {
		perror("malloc");
		return -1;
	}

	/* Reads from the start until the pages are covered. */
	remaining = pages * FFAULT_PAGE_SIZE;
	start = now_ns();
	while (remaining > 0) {
		want = FFAULT_CHUNK;
		if (remaining < want)
			want = remaining;
		got = pread(descriptor, buffer, (size_t)want, pages * FFAULT_PAGE_SIZE - remaining);
		if (got <= 0) {
			perror("pread");
			free(buffer);
			return -1;
		}

		/* Counts what was read. */
		remaining -= got;
	}

	/* Prints the time of the reads. */
	report("read() 1 MiB", start, now_ns(), pages);

	/* Frees the buffer. */
	free(buffer);
	return 0;
}

/* Returns the monotonic clock in nanoseconds. */
static long long
now_ns(
	void)
{
	struct timespec time;

	/* Reads the clock. */
	clock_gettime(CLOCK_MONOTONIC, &time);

	/* Combines the seconds and nanoseconds. */
	return (long long)time.tv_sec * 1000000000LL + time.tv_nsec;
}

/* Prints microseconds per page and the whole time. */
static void
report(
	const char *name,
	long long start,
	long long end,
	long pages)
{
	printf("FFAULT %-16s pages=%ld total_ms=%lld us_per_page=%.2f\n", name, pages,
	       (end - start) / 1000000LL, (double)(end - start) / 1000.0 / (double)pages);
}
