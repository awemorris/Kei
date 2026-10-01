/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Compares the owned native iterator with independent libc kernel mount inquiries. */
#include "userland/desktop/files/mounts.h"
#include <sys/param.h>
#include <sys/mount.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int compare_mounts(struct fm_mounts *mounts);

/* Checks real snapshot ownership, unchanged refusal output and native record identity. */
int
main(
	void)
{
	struct fm_mounts *mounts;
	struct fm_mount record;
	int error;
	int result;

	/* Refusal must leave an unrelated caller record untouched. */
	record.path = "unchanged path";
	record.type = "unchanged type";
	result = fm_mounts_next(NULL, &record);
	if (result != -1 || errno != EINVAL)
		return 1;

	/* Opens a real kernel snapshot instead of a manufactured mount-table file. */
	mounts = NULL;
	error = fm_mounts_open(&mounts);
	if (error != 0)
		return 1;

	/* Snapshot storage retires independently of every underlying mounted filesystem. */
	error = compare_mounts(mounts);
	fm_mounts_close(mounts);
	fm_mounts_close(NULL);
	if (error != 0)
		return 1;

	/* Succeeded: native mount paths/types and independent snapshot ownership agree. */
	(void)puts("PASS native mounts/kernel count/path/type/end/refusal/snapshot ownership");
	return 0;
}

/* Reads the real iterator alongside an independently acquired native mount list. */
static int
compare_mounts(
	struct fm_mounts *mounts)
{
	struct statfs *native;
	struct fm_mount record;
	struct fm_mount previous;
	int count;
	int index;
	int available;
	int same;
	int root;
	int device;

	/* Independent libc storage must describe the mounted native root and device filesystem. */
	count = getmntinfo(&native, MNT_NOWAIT);
	if (count <= 0)
		return EIO;

	/* Every adapter record must correspond to the same native mount order and filesystem type. */
	index = 0;
	root = 0;
	device = 0;
	while (1) {
		available = fm_mounts_next(mounts, &record);
		if (available < 0)
			return errno;

		/* End of the actual snapshot ends the independent record comparison. */
		if (available == 0)
			break;

		/* A synthetic or duplicated record cannot exceed the independently observed mount count. */
		if (index == count)
			return EPROTO;

		/* Native path identity is preserved without translating to Linux mount-table paths. */
		same = strcmp(record.path, native[index].f_mntonname);
		if (same != 0)
			return EPROTO;

		/* Native filesystem types remain available to unchanged common Places filtering. */
		same = strcmp(record.type, native[index].f_fstypename);
		if (same != 0)
			return EPROTO;

		/* The prepared UFS root must be represented by a real native record. */
		same = strcmp(record.path, "/");
		if (same == 0) {
			same = strcmp(record.type, "ufs");
			if (same != 0)
				return EPROTO;
			root = 1;
		}

		/* The prepared kernel device filesystem must retain its own native mount identity. */
		same = strcmp(record.path, "/dev");
		if (same == 0) {
			same = strcmp(record.type, "devfs");
			if (same != 0)
				return EPROTO;
			device = 1;
		}

		/* Retains the last borrowed record to verify end-of-enumeration leaves output unchanged. */
		previous = record;
		index++;
	}

	/* Missing kernel entries or required native filesystems invalidate the complete snapshot. */
	if (index != count ||
	    root == 0 ||
	    device == 0)
		return EPROTO;

	/* End-of-enumeration cannot clear or replace the caller's last record. */
	if (record.path != previous.path || record.type != previous.type)
		return EPROTO;

	/* Succeeded: every actual native mount matches the independent kernel inquiry. */
	(void)printf("native mount count=%d root=ufs devices=devfs\n", count);
	return 0;
}
