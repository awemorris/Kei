/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The seat's callbacks of libkeiland-backend (ws131-p006): what the
 * compositor does when the seat takes the display and the input devices
 * away (another session has the display) and gives them back.
 *
 * The backend calls these from kl_backend_poll_done; they change the
 * compositor's state only and never call back into the backend (the seat
 * tells its service after they return).  Inputs are known by their device
 * path, which stays the same while their descriptors change.
 */

#include "userland/desktop/wayland/zwl.h"

#include <stdio.h>
#include <string.h>

static struct zwl_input_device *backend_input(struct zwl_server *server, const char *path);

/*
 * The seat is paused: drawing stops and the output closes now (Vulkan's
 * duplicate of the primary node goes before the seat returns it).
 */
void
zwl_backend_session_paused(
	void *data)
{
	struct zwl_server *server;

	/* No frame and no new input device from now on. */
	server = data;
	server->os_paused = 1;

	/* The frame in flight finishes and the output closes. */
	if (server->compose != NULL) {
		zwl_compose_quiesce(server);
		zwl_compose_output_close(server);
	}

	/* The next activation makes a new output rather than reusing this one. */
	server->windowed = 0;
	printf("ZWL SEAT paused\n");
}

/*
 * The seat is active again: the next frame opens the output, and the next
 * scan finds the input devices.
 */
void
zwl_backend_session_resumed(
	void *data)
{
	struct zwl_server *server;

	/* The ordinary scheduler opens the output and scans the inputs. */
	server = data;
	server->os_paused = 0;
	server->input_scan_time = 0;
	server->windowed = 0;
	server->dirty = 1;
	printf("ZWL SEAT resumed\n");
}

/*
 * One input is paused: it is not read until it resumes (its descriptor
 * stays the seat's).
 */
void
zwl_backend_input_paused(
	void *data,
	const char *path)
{
	struct zwl_input_device *input;

	/* A node the compositor did not keep has nothing to stop. */
	input = backend_input(data, path);
	if (input == NULL)
		return;
	input->fd = -1;
}

/*
 * One input resumes on a new descriptor; a partial report of the old one
 * does not enter it.
 */
void
zwl_backend_input_resumed(
	void *data,
	const char *path,
	int descriptor)
{
	struct zwl_input_device *input;

	/* A node the compositor did not keep has nothing to resume. */
	input = backend_input(data, path);
	if (input == NULL)
		return;
	input->fd = descriptor;
	input->frame_count = 0;
	input->discarding = 0;
}

/*
 * One input is gone: it is forgotten (its descriptor was the seat's and is
 * closed).
 */
void
zwl_backend_input_gone(
	void *data,
	const char *path)
{
	struct zwl_input_device *input;

	/* A node the compositor did not keep has nothing to forget. */
	input = backend_input(data, path);
	if (input == NULL)
		return;
	zwl_input_forget(data, input);
}

/* Finds the input the compositor keeps for a device path, or NULL. */
static struct zwl_input_device *
backend_input(
	struct zwl_server *server,
	const char *path)
{
	unsigned index;
	int same;

	/* The path stays while the descriptors change. */
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		if (server->inputs[index].live == 0)
			continue;
		same = strcmp(server->inputs[index].path, path);
		if (same == 0)
			return &server->inputs[index];
	}

	/* No input of that path. */
	return NULL;
}
