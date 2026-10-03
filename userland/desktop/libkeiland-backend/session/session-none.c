/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The session where no session manager speaks to the compositor (Linux and
 * FreeBSD; libkeiland-backend since ws131-p006).
 *
 * A native system session takes the display at once, has no login screen
 * and no lock through a manager, and ends with Log Out through the
 * compositor's ordinary shutdown: every request answers ENOTSUP and the
 * tick has nothing to read.
 */

#include "userland/desktop/libkeiland-backend/backend-private.h"

#include <errno.h>
#include <stddef.h>

/*
 * Has no hand-over to wait for.
 */
int
kl_backend_session_ready(
	struct kl_backend *backend)
{
	/* The display may be taken at once. */
	if (backend == NULL)
		return EINVAL;
	return ENOTSUP;
}

/*
 * Has no manager to ask for a Log Out.
 */
int
kl_backend_session_logout(
	struct kl_backend *backend)
{
	/* The compositor ends by itself. */
	if (backend == NULL)
		return EINVAL;
	return ENOTSUP;
}

/*
 * Has no login screen to log in from.
 */
int
kl_backend_session_authenticate(
	struct kl_backend *backend,
	const char *user,
	const char *password)
{
	/* Nothing is sent, and nothing is kept. */
	(void)user;
	(void)password;
	if (backend == NULL)
		return EINVAL;
	return ENOTSUP;
}

/*
 * Has no manager to unlock through.
 */
int
kl_backend_session_unlock(
	struct kl_backend *backend,
	const char *password)
{
	/* Nothing is sent, and nothing is kept. */
	(void)password;
	if (backend == NULL)
		return EINVAL;
	return ENOTSUP;
}

/*
 * Tells that no manager started the session.
 */
int
kl_backend_session_managed(
	const struct kl_backend *backend)
{
	/* Neither lock nor Log Out goes through a manager. */
	(void)backend;
	return 0;
}

/*
 * Has nothing to read.
 */
void
kl_backend_session_tick(
	struct kl_backend *backend,
	uint64_t now_ms)
{
	/* No manager sends anything. */
	(void)backend;
	(void)now_ms;
}
