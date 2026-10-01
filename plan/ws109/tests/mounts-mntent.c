/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Compares the shared mount iterator with an independently opened real mount table. */
#include "userland/desktop/files/mounts.h"
#include <errno.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_mounts(struct fm_mounts *mounts, FILE *table);

/* Verifies actual mount order, borrowed record lifetime and unchanged end output. */
int
main(
	void)
{
	struct fm_mounts *mounts;
	struct fm_mount record;
	FILE *table;
	int error;
	int result;

	/* Refusal leaves the caller's existing record intact. */
	record.path = "existing path";
	record.type = "existing type";
	result = fm_mounts_next(NULL, &record);
	if (result != -1 || errno != EINVAL)
		return 1;

	/* Acquires an iterator over the production mounted-filesystem table. */
	error = fm_mounts_open(&mounts);
	if (error != 0)
		return 1;

	/* Opens an independent stream over the same kernel-maintained table. */
	table = setmntent(MOUNTED, "r");
	if (table == NULL) {
		fm_mounts_close(mounts);
		return 1;
	}

	/* Releases both independent streams after comparing every real mount record. */
	error = compare_mounts(mounts, table);
	(void)endmntent(table);
	fm_mounts_close(mounts);
	fm_mounts_close(NULL);
	if (error != 0)
		return 1;

	/* Succeeded: shared mount-table enumeration preserves actual paths, types and order. */
	(void)puts("PASS shared mounts/actual table order/path/type/end/refusal/stream ownership");
	return 0;
}

/* Copies borrowed strings before the independent libc parser can replace its global record. */
static int
compare_mounts(
	struct fm_mounts *mounts,
	FILE *table)
{
	struct fm_mount record;
	struct fm_mount previous;
	struct mntent *entry;
	char *path;
	char *type;
	int available;
	int count;
	int same;
	int mismatch;
	int saved;

	/* Every adapter entry must correspond to the next independently parsed actual mount. */
	count = 0;
	while (1) {
		available = fm_mounts_next(mounts, &record);
		if (available < 0)
			return errno;

		/* End of the adapter leaves the caller's last borrowed record unchanged. */
		if (available == 0)
			break;

		/* Retains the adapter path before getmntent overwrites libc's shared parser storage. */
		path = strdup(record.path);
		if (path == NULL)
			return errno;

		/* Retains the adapter filesystem type with an independently checked allocation. */
		type = strdup(record.type);
		if (type == NULL) {
			saved = errno;
			free(path);
			return saved;
		}

		/* An independently opened stream must supply one matching real mount record. */
		entry = getmntent(table);
		if (entry == NULL) {
			free(type);
			free(path);
			return EPROTO;
		}

		/* Path mismatch must survive even if the filesystem types happen to agree. */
		mismatch = 0;
		same = strcmp(path, entry->mnt_dir);
		if (same != 0)
			mismatch = 1;

		/* Compares owned type storage so libc's global record cannot hide an adapter error. */
		same = strcmp(type, entry->mnt_type);
		if (same != 0)
			mismatch = 1;

		/* Every per-record copy retires before another production record is consumed. */
		free(type);
		free(path);
		if (mismatch != 0)
			return EPROTO;

		/* Retains the previous output pointers to detect an end-of-stream overwrite. */
		previous = record;
		count++;
	}

	/* The running system's mounted root requires at least one real record. */
	if (count == 0)
		return EPROTO;

	/* End of enumeration must not clear the last returned pointers. */
	if (record.path != previous.path || record.type != previous.type)
		return EPROTO;

	/* The independent stream cannot contain additional records omitted by the adapter. */
	entry = getmntent(table);
	if (entry != NULL)
		return EPROTO;

	/* Succeeded: all real mount-table records agree with independent stream parsing. */
	(void)printf("shared mount count=%d\n", count);
	return 0;
}
