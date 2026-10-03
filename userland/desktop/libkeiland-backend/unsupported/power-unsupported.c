/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The power where the backend does not offer it yet (ws131-p005): FreeBSD.  The state says the source is unknown and no action may be
 * taken; an action answers ENOTSUP.
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stddef.h>

/*
 * Copies the power's state: nothing known, no action.
 */
int
kl_backend_power_get_state(
	const struct kl_backend *backend,
	struct kl_backend_power_state *state)
{
	/* A state needs a backend and somewhere to put it. */
	if (backend == NULL || state == NULL)
		return EINVAL;

	/* Nothing is known and nothing may be asked. */
	state->source = KL_BACKEND_POWER_SOURCE_UNKNOWN;
	state->percent = -1;
	state->charging = 0U;
	state->actions = 0U;

	/* Succeeded: the state is filled. */
	return 0;
}

/*
 * Refuses every action.
 */
int
kl_backend_power_action(
	struct kl_backend *backend,
	unsigned action)
{
	/* An action needs a backend. */
	if (backend == NULL)
		return EINVAL;

	/* No action is offered here. */
	(void)action;
	return ENOTSUP;
}
