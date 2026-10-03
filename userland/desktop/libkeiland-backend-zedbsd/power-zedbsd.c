/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The power on zedBSD (ws131-p005): the login screen's Shut Down and
 * Restart, through sessiond.
 *
 * sessiond takes "POWER poweroff" and "POWER reboot" on the login screen's
 * descriptor only (sessiond/greeter.c); a session's descriptor takes no
 * power request (sessiond/session.c), so inside a session no action is
 * offered (plan/ws131/design.md, decision D12).  The request is one short
 * line written whole.  sessiond's "OK" comes back on the same descriptor
 * as its answers to the login screen's other requests, and the session
 * (session-zedbsd.c) reads them all.  The power source is not read: there
 * is no battery interface yet.
 */

#include "userland/desktop/libkeiland-backend/backend-private.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* The longest request line. */
#define POWER_LINE_MAX 32U

static unsigned power_actions(const struct kl_backend *backend);

/*
 * Copies the power's state: the source unknown, and the actions the login
 * screen may ask for.
 */
int
kl_backend_power_get_state(
	const struct kl_backend *backend,
	struct kl_backend_power_state *state)
{
	/* A state needs a backend and somewhere to put it. */
	if (backend == NULL || state == NULL)
		return EINVAL;

	/* No battery interface: the source and the charge are unknown. */
	state->source = KL_BACKEND_POWER_SOURCE_UNKNOWN;
	state->percent = -1;
	state->charging = 0U;

	/* The actions sessiond takes from this compositor. */
	state->actions = power_actions(backend);

	/* Succeeded: the state is filled. */
	return 0;
}

/*
 * Asks sessiond to power the machine off or restart it.
 */
int
kl_backend_power_action(
	struct kl_backend *backend,
	unsigned action)
{
	char line[POWER_LINE_MAX];
	const char *word;
	size_t length;
	ssize_t written;

	/* An action needs a backend. */
	if (backend == NULL)
		return EINVAL;

	/* Only the actions sessiond takes from this compositor. */
	if (action >= 32U || (power_actions(backend) & KL_BACKEND_POWER_ACTION_BIT(action)) == 0U)
		return ENOTSUP;

	/* One action at a time, and none while another request of sessiond's awaits its answer. */
	if (backend->power_asked != 0U || backend->session_request != KL_BACKEND_SESSION_NONE)
		return EBUSY;

	/* The request's word. */
	word = "poweroff";
	if (action == KL_BACKEND_POWER_REBOOT)
		word = "reboot";

	/* The whole line in one write (it is short). */
	(void)snprintf(line, sizeof(line), "POWER %s\n", word);
	length = strlen(line);
	written = write(backend->options.greeter_descriptor, line, length);
	if (written < 0)
		return errno;
	if ((size_t)written != length)
		return EIO;

	/* Succeeded: sessiond ends the machine; its OK comes as session_answer(KL_BACKEND_SESSION_POWER). */
	backend->power_asked = action;
	backend->session_request = KL_BACKEND_SESSION_POWER;
	return 0;
}

/* The actions sessiond takes: Shut Down and Restart from the login screen, none from a session. */
static unsigned
power_actions(
	const struct kl_backend *backend)
{
	unsigned actions;

	/* A session's descriptor takes no power request. */
	if (backend->options.greeter_descriptor < 0)
		return 0U;

	/* The login screen's two buttons. */
	actions = KL_BACKEND_POWER_ACTION_BIT(KL_BACKEND_POWER_POWEROFF) |
	    KL_BACKEND_POWER_ACTION_BIT(KL_BACKEND_POWER_REBOOT);
	return actions;
}
