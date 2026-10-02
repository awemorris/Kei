/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks actual sidebar persistence and native buffered-file failure propagation. */
#include "userland/desktop/files/files.h"
#include <sys/resource.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verify_list(const char *file, const char *expected);
static int verify_failure(struct fm_places *places);

/* Exercises only the fixture's config directory and ordinary public favorite operations. */
int
main(
	int argc,
	char **argv)
{
	struct fm_places *places;
	char file[FM_PATH_MAX];
	int error;
	int index;

	/* The controller must supply an exclusively owned absolute config directory. */
	if (argc != 2 || argv[1][0] != '/')
		return 1;

	/* Selects an ordinary per-user config root without changing production test behavior. */
	error = setenv("XDG_CONFIG_HOME", argv[1], 1);
	if (error != 0)
		return 1;

	/* Allocates one real sidebar record instead of replacing the persistence implementation. */
	places = calloc(1, sizeof(*places));
	if (places == NULL)
		return 1;

	/* Three ordered favorites surround one non-persisted virtual Home entry. */
	places->count = 4;
	places->items[0].section = FM_SECTION_FAVORITES;
	places->items[0].location.kind = FM_LOCATION_HOME;

	/* Populates each persisted folder with its distinct absolute path. */
	for (index = 1; index < 4; index++) {
		places->items[index].section = FM_SECTION_FAVORITES;
		places->items[index].location.kind = FM_LOCATION_FOLDER;
		(void)snprintf(places->items[index].location.path, FM_PATH_MAX, "/folder%d", index);
	}

	/* Appending preserves all existing favorite folders and excludes Home. */
	error = fm_places_add_favorite(places, "/new");
	if (error != 0)
		return 1;

	/* Reads the actual persisted file, independently of the sidebar source. */
	(void)snprintf(file, sizeof(file), "%s/files/sidebar", argv[1]);
	error = verify_list(file, "/folder1\n/folder2\n/folder3\n/new\n");
	if (error != 0)
		return 1;

	/* Duplicate paths remain an explicit refusal. */
	error = fm_places_add_favorite(places, "/folder1");
	if (error != EEXIST)
		return 1;

	/* Moving upward places the dragged folder before its target. */
	error = fm_places_move_favorite(places, 3, 1);
	if (error != 0)
		return 1;

	/* The actual stored order must retain every other favorite. */
	error = verify_list(file, "/folder3\n/folder1\n/folder2\n");
	if (error != 0)
		return 1;

	/* Moving downward places the dragged folder after its target. */
	error = fm_places_move_favorite(places, 1, 3);
	if (error != 0)
		return 1;

	/* The downward move preserves surviving folder order. */
	error = verify_list(file, "/folder2\n/folder3\n/folder1\n");
	if (error != 0)
		return 1;

	/* Removal omits only its selected ordinary favorite. */
	error = fm_places_remove_favorite(places, 2);
	if (error != 0)
		return 1;

	/* The virtual dashboard remains absent from the persistence file. */
	error = verify_list(file, "/folder1\n/folder3\n");
	if (error != 0)
		return 1;

	/* A virtual place cannot be moved or removed as an ordinary persisted folder. */
	error = fm_places_remove_favorite(places, 0);
	if (error != EINVAL)
		return 1;

	/* Kernel-enforced file-size refusal must not become successful persistence. */
	error = verify_failure(places);
	free(places);
	if (error != 0)
		return 1;

	/* Succeeded: real persistence preserves order and reports native buffered write failures. */
	(void)puts("PASS Favorites append/duplicate/up/down/remove/virtual refusal/actual kernel write failure");
	return 0;
}

/* Compares independently read file bytes with the requested persisted sidebar order. */
static int
verify_list(
	const char *file,
	const char *expected)
{
	char bytes[1024];
	FILE *stream;
	size_t count;
	int error;
	int closed;

	/* The production writer must have created a real readable sidebar list. */
	stream = fopen(file, "rb");
	if (stream == NULL)
		return errno;

	/* Retains the actual byte sequence and independently observes any read failure. */
	count = fread(bytes, 1, sizeof(bytes) - 1, stream);
	error = ferror(stream);
	closed = fclose(stream);
	if (error != 0 || closed != 0)
		return EIO;

	/* Exact persisted text includes every newline and the original folder order. */
	bytes[count] = '\0';
	error = strcmp(bytes, expected);
	if (error != 0)
		return EPROTO;

	/* Succeeded: the actual file contains the independently expected list bytes. */
	return 0;
}

/* Refuses real file growth with a process-local resource limit and restores it before returning. */
static int
verify_failure(
	struct fm_places *places)
{
	struct rlimit original;
	struct rlimit limited;
	struct sigaction ignored;
	struct sigaction previous;
	int error;
	int status;

	/* Saves the actual process limit before imposing a bounded kernel write refusal. */
	status = getrlimit(RLIMIT_FSIZE, &original);
	if (status != 0)
		return errno;

	/* An ignored file-size signal allows libc to report the actual buffered write errno. */
	memset(&ignored, 0, sizeof(ignored));
	ignored.sa_handler = SIG_IGN;
	status = sigemptyset(&ignored.sa_mask);
	if (status != 0)
		return errno;

	/* Retains the process's signal action for restoration after the real syscall check. */
	status = sigaction(SIGXFSZ, &ignored, &previous);
	if (status != 0)
		return errno;

	/* The hard limit remains unchanged; only this probe's soft file extent becomes zero. */
	limited = original;
	limited.rlim_cur = 0;
	status = setrlimit(RLIMIT_FSIZE, &limited);
	if (status != 0) {
		error = errno;
		(void)sigaction(SIGXFSZ, &previous, NULL);
		return error;
	}

	/* Uses the real common writer and real kernel failure, without a fabricated stdio provider. */
	error = fm_places_add_favorite(places, "/kernel-refused");

	/* Restores process-local file limits before any further controller-visible result. */
	status = setrlimit(RLIMIT_FSIZE, &original);
	if (status != 0)
		return errno;

	/* Restores the original signal disposition before classifying the observed refusal. */
	status = sigaction(SIGXFSZ, &previous, NULL);
	if (status != 0)
		return errno;

	/* A buffered kernel write refusal must be reported through the documented errno convention. */
	if (error != EFBIG)
		return EPROTO;

	/* Succeeded: the real stream close propagated the kernel's failed persistence. */
	return 0;
}
