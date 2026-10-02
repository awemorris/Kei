/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Owns an actual FreeBSD kernel mount snapshot instead of a Linux mount-table file. */
#include "../mounts.h"
#include <sys/param.h>
#include <sys/mount.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* One enumeration owns a stable native record array until the sidebar releases it. */
struct fm_mounts {
	struct statfs *snapshot;
	size_t count;
	size_t at;
};

/*
 * Captures a bounded native kernel mount list and publishes only a complete owned snapshot.
 */
int
fm_mounts_open(
	struct fm_mounts **result)
{
	struct fm_mounts *mounts;
	size_t capacity;
	size_t bytes;
	int count;
	int received;
	int saved;

	/* A missing publication slot cannot receive an owned native snapshot. */
	if (result == NULL)
		return EINVAL;

	/* Counts real mounts without performing blocking filesystem statistics updates. */
	count = getfsstat(NULL, 0, MNT_NOWAIT);
	if (count < 0)
		return errno;

	/* A real mounted root must exist before this snapshot can describe native volumes. */
	if (count == 0)
		return EIO;

	/* One guard record makes a concurrently filled snapshot an explicit retry refusal. */
	capacity = (size_t)count + 1;
	if (capacity > (size_t)LONG_MAX / sizeof(struct statfs))
		return EOVERFLOW;

	/* Allocates one independent owner before acquiring its record-array storage. */
	mounts = malloc(sizeof(*mounts));
	if (mounts == NULL)
		return errno;

	/* The owner starts with no published records or current cursor. */
	mounts->snapshot = NULL;
	mounts->count = 0;
	mounts->at = 0;

	/* Native snapshot bytes must fit both the allocation and getfsstat's signed extent. */
	bytes = capacity * sizeof(struct statfs);
	mounts->snapshot = malloc(bytes);
	if (mounts->snapshot == NULL) {
		saved = errno;
		free(mounts);
		return saved;
	}

	/* The second kernel inquiry captures actual native records rather than a simulated mount source. */
	received = getfsstat(mounts->snapshot, (long)bytes, MNT_NOWAIT);
	if (received < 0) {
		saved = errno;
		fm_mounts_close(mounts);
		return saved;
	}

	/* Concurrent growth cannot silently truncate the set of filesystem records. */
	if ((size_t)received >= capacity) {
		fm_mounts_close(mounts);
		return EAGAIN;
	}

	/* Publishes only the complete observed generation and its independent ownership. */
	mounts->count = (size_t)received;
	*result = mounts;

	/* Succeeded: native kernel mount records remain stable until iterator close. */
	return 0;
}

/*
 * Supplies the next native mounted directory and filesystem type without transferring storage.
 */
int
fm_mounts_next(
	struct fm_mounts *mounts,
	struct fm_mount *mount)
{
	struct statfs *entry;

	/* Missing owners or output records are explicit API refusals. */
	if (mounts == NULL || mount == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* End of this stable native generation leaves the caller's prior record unchanged. */
	if (mounts->at == mounts->count)
		return 0;

	/* The iterator cursor advances only after selecting a real captured kernel record. */
	entry = &mounts->snapshot[mounts->at];
	mounts->at++;
	mount->path = entry->f_mntonname;
	mount->type = entry->f_fstypename;

	/* Succeeded: common Places filtering can use the actual native mount point and type. */
	return 1;
}

/*
 * Frees only this enumeration's native record storage and owner.
 */
void
fm_mounts_close(
	struct fm_mounts *mounts)
{
	/* A refused or unstarted enumeration has no snapshot owner to return. */
	if (mounts == NULL)
		return;

	/* Snapshot ownership is independent of libc's global getmntinfo storage and all mount lifetimes. */
	free(mounts->snapshot);
	free(mounts);

	/* Succeeded: no native snapshot allocation remains owned by this iterator. */
	return;
}
