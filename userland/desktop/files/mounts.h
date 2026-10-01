/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Defines private mount enumeration without exposing native storage or mount-table APIs. */
#ifndef FM_MOUNTS_H
#define FM_MOUNTS_H

struct fm_mounts;

/* One current mount record borrows strings until the next iterator read or final close. */
struct fm_mount {
	const char *path;
	const char *type;
};

/* Open publishes an owned iterator on success zero; failure returns a positive errno unchanged. */
int fm_mounts_open(struct fm_mounts **mounts);
/* Next returns one record, zero at end, or -1/errno; refusal/end leaves the record unchanged. */
int fm_mounts_next(struct fm_mounts *mounts, struct fm_mount *mount);
/* Close tolerates NULL and retires only this iterator's native ownership. */
void fm_mounts_close(struct fm_mounts *mounts);

#endif
