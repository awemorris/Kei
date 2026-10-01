/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Ends a native system session through ordinary compositor shutdown without sessiond. */
#include "../zwl.h"

/*
 * Permits immediate display acquisition in a native system session.
 */
void
zwl_handoff_wait(
	struct zwl_server *server)
{
	/* Native system sessions have no zedBSD sessiond handoff descriptor or READY/GO exchange. */
	server->handed_over = 1;

	/* Succeeded: display ownership may begin immediately. */
	return;
}

/*
 * Retires display output before the rest of session cleanup.
 */
void
zwl_handoff_release(
	struct zwl_server *server)
{
	/* Vulkan destroys the display chain before releasing its acquired master file. */
	zwl_compose_output_close(server);

	/* Succeeded: no display output remains owned by this session. */
	return;
}

/*
 * Requests ordinary compositor termination for Log Out.
 */
int
zwl_handoff_logout(
	struct zwl_server *server)
{
	/* The common Home handler stops when no external logout acknowledgment is required. */
	(void)server;

	/* Succeeded: the caller should finish the session immediately. */
	return 0;
}

/*
 * Handles the absence of a zedBSD sessiond service.
 */
void
zwl_handoff_tick(
	struct zwl_server *server)
{
	/* Native system sessions have no handoff response stream to consume. */
	(void)server;

	/* Succeeded: the ordinary event loop continues. */
	return;
}
