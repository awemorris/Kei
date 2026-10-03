/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The backend object every operating system shares (WS131 p003).
 *
 * It keeps the compositor's callbacks and options for the areas that
 * report through them (struct kl_backend is in backend-private.h, which the
 * operating systems' areas share).  No area polls a descriptor yet: the
 * network and the sound are read by the compositor's own updates, and the
 * session's manager is read by the tick (ws131-p006), as the compositor
 * read it before.  The poll has nothing to do until the areas that need it
 * (the seat, the input devices) move here.
 */

#include "userland/desktop/libkeiland-backend/backend-private.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

/* Marks a parameter an interface requires but this implementation does not use yet. */
#define UNUSED_PARAMETER(name) ((void)(name))

/*
 * Opens the backend.
 *
 * The backend copies the callbacks and the options; the compositor's
 * structures need not outlive the call.
 */
int
kl_backend_open(
	const struct kl_backend_options *options,
	const struct kl_backend_host *host,
	struct kl_backend **backend)
{
	struct kl_backend *opened;

	/* Allocates the backend. */
	opened = malloc(sizeof(*opened));
	if (opened == NULL)
		return ENOMEM;

	/* Keeps the compositor's callbacks and options. */
	memset(opened, 0, sizeof(*opened));
	opened->host = *host;
	opened->options = *options;

	/* Succeeded: the compositor owns the backend until it closes it. */
	*backend = opened;
	return 0;
}

/*
 * Closes the backend.
 */
void
kl_backend_close(
	struct kl_backend *backend)
{
	/* Frees the backend (free takes NULL). */
	free(backend);
}

/*
 * Counts the descriptors the backend needs in the next poll.
 */
size_t
kl_backend_poll_count(
	const struct kl_backend *backend)
{
	UNUSED_PARAMETER(backend);

	/* No area waits on a descriptor yet. */
	return 0;
}

/*
 * Fills the backend's poll descriptors.
 */
void
kl_backend_poll_fill(
	struct kl_backend *backend,
	struct pollfd *descriptors)
{
	/* No area waits on a descriptor yet, so there is nothing to fill. */
	UNUSED_PARAMETER(backend);
	UNUSED_PARAMETER(descriptors);
}

/*
 * Handles what poll reported for the backend's descriptors.
 */
void
kl_backend_poll_done(
	struct kl_backend *backend,
	const struct pollfd *descriptors)
{
	/* No area waits on a descriptor yet, so nothing was reported. */
	UNUSED_PARAMETER(backend);
	UNUSED_PARAMETER(descriptors);
}

/*
 * Lets the backend do the work that waits for time.
 */
void
kl_backend_tick(
	struct kl_backend *backend,
	uint64_t now_ms)
{
	/* A compositor whose backend did not open has nothing to do here. */
	if (backend == NULL)
		return;

	/* The session manager's lines and the Log Out's deadline (ws131-p006). */
	kl_backend_session_tick(backend, now_ms);
}
