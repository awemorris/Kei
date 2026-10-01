/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Retains Linux and zedBSD mount-table storage behind the file manager's private iterator. */
#include "../mounts.h"
#include <errno.h>
#include <mntent.h>
#include <stdlib.h>

/* One file manager enumeration owns its table stream until explicit close. */
struct fm_mounts {
	FILE *table;
};

/* Opens the same authoritative mounted-filesystem table used by the existing Places implementation. */
int
fm_mounts_open(
	struct fm_mounts **result)
{
	struct fm_mounts *mounts;
	int saved;

	/* A missing publication slot cannot receive an owned mount-table stream. */
	if (result == NULL)
		return EINVAL;

	/* Allocates the iterator before acquiring its independently owned stream. */
	mounts = malloc(sizeof(*mounts));
	if (mounts == NULL)
		return errno;

	/* Retains existing read-only MOUNTED selection and kernel-table error behavior. */
	mounts->table = setmntent(MOUNTED, "r");
	if (mounts->table == NULL) {
		saved = errno;
		free(mounts);
		return saved;
	}

	/* Publishes the owner only after its actual table stream is ready. */
	*result = mounts;

	/* Succeeded: the caller owns the same mount-table source as before. */
	return 0;
}

/* Copies the current table record's borrowed directory and filesystem type. */
int
fm_mounts_next(
	struct fm_mounts *mounts,
	struct fm_mount *mount)
{
	struct mntent *entry;

	/* A missing iterator or record is a refusal rather than a successful empty enumeration. */
	if (mounts == NULL || mount == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* Distinguishes the table API's end from a recorded parser or I/O error. */
	errno = 0;
	entry = getmntent(mounts->table);
	if (entry == NULL) {
		/* A real table-read error preserves the native parser's errno. */
		if (errno != 0)
			return -1;

		/* Succeeded: no further mount record remains in this stream. */
		return 0;
	}

	/* The old table API retains these strings only until the next record read. */
	mount->path = entry->mnt_dir;
	mount->type = entry->mnt_type;

	/* Succeeded: the caller may apply its unchanged common Places filtering. */
	return 1;
}

/* Returns this iterator's mount stream and allocation after the sidebar has copied its records. */
void
fm_mounts_close(
	struct fm_mounts *mounts)
{
	/* Partial startup may never have produced an iterator owner. */
	if (mounts == NULL)
		return;

	/* The stream and iterator retire together without altering mounted filesystems. */
	(void)endmntent(mounts->table);
	free(mounts);

	/* Succeeded: no table descriptor or iterator storage remains owned. */
	return;
}
