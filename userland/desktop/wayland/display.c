/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Independent GPU import and the scheduling of window mode (WS035
 * compositing design, D0).
 *
 * The display is taken and shown only through Vulkan (the VK_KHR_display
 * swapchain of compose.c): the compositor that showed one surface directly
 * without composing (--direct, the compositor before WS035) was removed in
 * ws103-p002.  Every window, a fullscreen one too, is composed: the direct
 * scanout of a fullscreen window's image (fullscreen mode) was removed in
 * ws099-p015 (the 2026-09-30 user decision), so the edges' gestures and
 * their feedback draw over a fullscreen window as over any other.
 *
 * The compositor holds no GPU fd of its own (ws103-p006): client images come
 * in through Vulkan (import.c) and a client's acquire fence is only polled.
 */

#include "desktop.h"
#include "zwl.h"
#include "popup.h"
#include "toplevel.h"
#include "extras.h"
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static void adopt_commit(struct zwl_server *server, struct zwl_object *surface);
static void place_window(struct zwl_server *server, struct zwl_object *surface);
static int enter_window_mode(struct zwl_server *server);

/*
 * Reports whether a surface's queued image may be used: each of its acquire
 * fences is done.  Done fences are closed.  Nothing is waited for.
 *
 * A fence fd stands for its present alone (the WSI sends a new fence for each
 * present, ws103-p005) and becomes readable when the present's rendering is
 * done, or reports an error or a hang-up when it failed or cannot be read;
 * any of these is done, as a fence that can no longer be read is not waited
 * for.  The same holds for a sync_file on other systems.
 */
int
zwl_fence_ready(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	struct pollfd check;
	unsigned index;
	unsigned kept;
	int ready;

	/* A commit without fences is ready at once. */
	if (surface->fence_count == 0)
		return 1;

	/* Each fence still pending is kept; the others are closed. */
	kept = 0;
	for (index = 0; index < surface->fence_count; index++) {
		/* The fence's readiness now, without waiting. */
		check.fd = surface->fences[index].fd;
		check.events = POLLIN;
		check.revents = 0;
		ready = poll(&check, 1U, 0);

		/* Still rendering: nothing to report yet (an interrupted poll is asked again on a later pass). */
		if (ready == 0 ||
		    (ready < 0 &&
		     errno == EINTR)) {
			surface->fences[kept] = surface->fences[index];
			kept++;
			continue;
		}

		/* Done, failed, or a fence that can no longer be read, which is not waited for. */
		close(surface->fences[index].fd);
	}

	/* The pending fences remain. */
	surface->fence_count = kept;

	/* Some image is still being drawn. */
	if (kept != 0) {
		surface->fence_waited = 1;
		return 0;
	}

	/* Succeeded: all done. */
	if (server->log_frames && surface->fence_waited)
		printf("ZWL ACQUIRED surface=%u waited_ms=%llu\n", surface->id, (unsigned long long)(zwl_milliseconds() - surface->fence_ms));
	return 1;
}

/*
 * Applies the commits of this event-loop pass and shows the result: a
 * composed frame of window mode, fullscreen windows included.
 */
void
zwl_schedule(
	struct zwl_server *server)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *top;
	uint64_t now;
	int ready;
	int error;

	/* An OS-paused display cannot compose or present until authority returns. */
	if (server->os_paused != 0)
		return;

	/* The glass look's clock turns over. */
	if (server->glass)
		zwl_glass_tick(server);

	/* The desktop surface's program and place (desktop.c). */
	zwl_desktop_tick(server);

	/* Every committed surface takes its new image. */
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			/* Only a live surface with a commit. */
			if (surface->kind != ZWL_SURFACE ||
			    !surface->ready ||
			    surface->dead)
				continue;

			/* A commit whose image is still being drawn waits for a later pass. */
			ready = zwl_fence_ready(server, surface);
			if (ready)
				adopt_commit(server, surface);
		}
	}

	/* The first pass enters window mode, which the compositor stays in (a fullscreen window is composed too). */
	top = zwl_top_window(server);
	if (!server->windowed) {
		error = enter_window_mode(server);
		if (error != 0) {
			/* A switch waiting for the frame in flight is tried again on a later pass. */
			if (error == EAGAIN)
				return;
			printf("ZWL MODE_ERROR errno=%d\n", error);
			server->failed = 1;
			return;
		}
	}

	/* Input goes to the topmost window, or to the desktop pressed last (desktop.c). */
	server->front_surface = zwl_desktop_front(server, top);
	zwl_seat_focus(server);

	/* wl_shm images are copied, and their buffers released, while no frame is in flight. */
	(void)zwl_shm_upload(server);

	/*
	 * The windows the last frame told get a moment to commit (frame
	 * pacing), unless the pointer moved and waits to be shown, or App Home
	 * or Wiseview was asked to open or close (ws099-p002).
	 */
	now = zwl_milliseconds();
	if (server->awaiting != 0 &&
	    !server->pointer_moved &&
	    server->transition == NULL &&
	    now - server->frame_done_ms < server->frame_wait_ms)
		return;

	/* Window mode draws when something changed (all of it, or a part) and no frame is in flight. */
	if (server->dirty || server->damaged) {
		error = zwl_compose_draw(server);
		if (error != 0) {
			printf("ZWL FAILED site=compose_draw errno=%d\n", error);
			server->failed = 1;
		}
	}
}

/*
 * Notes that App Home or Wiseview was asked to open or close: the next
 * frame is drawn without waiting for the windows (frame pacing), and the
 * time from the request to its submission is logged (ws099-p002, C5).
 */
void
zwl_transition_request(
	struct zwl_server *server,
	const char *what)
{
	/* The request, the latest one if two come before a frame. */
	server->transition = what;
	server->transition_ms = zwl_milliseconds();
	server->dirty = 1;
}

/*
 * Ends the frame in flight when its fence fd became readable.
 */
void
zwl_frame_done(
	struct zwl_server *server)
{
	int error;

	/* The frame's held buffers and callbacks are released. */
	error = zwl_compose_complete(server);
	if (error != 0) {
		printf("ZWL FAILED site=frame_done errno=%d\n", error);
		server->failed = 1;
	}
}

/*
 * Returns the topmost mapped window of a live client, or NULL.
 */
struct zwl_object *
zwl_top_window(
	struct zwl_server *server)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *top;
	int desktop_surface;

	/* The highest map order on the desktop shown wins. */
	top = NULL;
	for (client = server->clients; client != NULL; client = client->next) {
		if (client->fatal)
			continue;
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			if (surface->kind != ZWL_SURFACE || surface->dead || !surface->mapped)
				continue;
			if (surface->desktop != server->desktop || surface->minimized)
				continue;

			/* The desktop's icons are never a window, so a closed last window does not give them the keyboard (desktop.c). */
			desktop_surface = zwl_desktop_is(surface);
			if (desktop_surface)
				continue;
			if (top == NULL || surface->map_order > top->map_order)
				top = surface;
		}
	}

	/* Succeeded: the window on top, if any. */
	return top;
}

/*
 * Centres a window at its current size: in the glass look in the space
 * under the system bar and a title bar (kept inside it), otherwise on the
 * output (ws035-p138).
 */
void
zwl_window_centre(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	int32_t space_width;
	int32_t space_height;
	uint32_t width;
	uint32_t height;

	/* Its size: its image's (a viewport's), else the output's. */
	width = server->width;
	height = server->height;
	if (surface->current != NULL)
		zwl_surface_size(surface, &width, &height);

	/* The glass look: the space's centre, the title bar under the system bar. */
	if (server->glass) {
		zwl_glass_space(server, &space_width, &space_height);
		surface->x = ((int32_t)server->width - (int32_t)width) / 2;
		surface->y = ZWL_GLASS_TOP + (space_height - (int32_t)height) / 2;
		zwl_glass_fit(server, (int32_t)width, (int32_t)height, &surface->x, &surface->y);
		return;
	}

	/* Otherwise the output's centre, not above or left of it. */
	surface->x = ((int32_t)server->width - (int32_t)width) / 2;
	surface->y = ((int32_t)server->height - (int32_t)height) / 2;
	if (surface->x < 0)
		surface->x = 0;
	if (surface->y < 0)
		surface->y = 0;
}

/*
 * Makes a surface's committed image its current one: a first image maps the
 * window, a null one unmaps it.  The image it replaces is released once no
 * frame holds it.
 */
static void
adopt_commit(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	struct zwl_object *previous;
	uint32_t new_width;
	uint32_t new_height;

	/* The queued image becomes current; a wl_shm image is copied before the next frame; its window is drawn again (damage.c). */
	previous = surface->current;
	surface->current = surface->queued;
	surface->queued = NULL;
	zwl_damage_commit(server, surface, previous);
	surface->ready = 0;
	surface->fresh = 1;
	if (surface->current != NULL && surface->current->shm != NULL)
		surface->shm_upload = 1;

	/* The time the image was taken, when the per-frame lines were asked for (ws099-p015's pen latency). */
	if (server->log_frames)
		printf("ZWL LAT adopt surface=%u at_us=%llu\n", surface->id, (unsigned long long)zwl_microseconds());

	/* An awaited window has committed. */
	if (surface->awaited) {
		surface->awaited = 0;
		server->awaiting--;
	}

	/* A cursor, or a surface with no role, is not a window. */
	if (surface->cursor_role || surface->role == NULL) {
		zwl_buffer_put(previous);
		return;
	}

	/* Nor is a popup (popup.c): its first image shows it with its parent, a null one hides it. */
	if (surface->role->top != NULL && surface->role->top->kind == ZWL_POPUP) {
		if (previous == NULL && surface->current != NULL)
			zwl_popup_mapped(server, surface);
		if (previous != NULL && surface->current == NULL)
			zwl_callbacks_done(&surface->committed_callbacks);
		zwl_buffer_put(previous);
		return;
	}

	/* A first image maps the window on top; a null one unmaps it. */
	if (surface->current != NULL && !surface->mapped) {
		surface->mapped = 1;
		server->map_order++;
		surface->map_order = server->map_order;
		surface->desktop = server->desktop;
		place_window(server, surface);
		printf("ZWL MAP client=%llu surface=%u x=%d y=%d\n", (unsigned long long)surface->client->number, surface->id, surface->x, surface->y);
		zwl_glass_mapped(server, surface);
	} else if (surface->current == NULL && surface->mapped) {
		surface->mapped = 0;
		zwl_callbacks_done(&surface->committed_callbacks);
		printf("ZWL UNMAP client=%llu surface=%u\n", (unsigned long long)surface->client->number, surface->id);
	}

	/*
	 * A window that left a fullscreen it started in is centred at its first
	 * image of a size other than the output's (the client draws its window
	 * size after the configure; ws035-p138).
	 */
	if (surface->place_pending && surface->mapped && !surface->fullscreen && surface->current != NULL) {
		zwl_surface_size(surface, &new_width, &new_height);
		if (new_width != server->width || new_height != server->height) {
			surface->place_pending = 0;
			zwl_window_centre(server, surface);
			surface->placed = 1;
			printf("ZWL WINDOW centred surface=%u x=%d y=%d width=%u height=%u\n", surface->id, surface->x, surface->y, new_width, new_height);
		}
	}

	/* A window resized from its left or top edge keeps its other edges where they were (toplevel.c). */
	zwl_toplevel_committed(server, surface);

	/* The replaced image. */
	zwl_buffer_put(previous);
}

/*
 * Places a new window: a fullscreen one at the origin, others centred and
 * cascaded by ZWL_CASCADE_STEP (design D6).
 */
static void
place_window(
	struct zwl_server *server,
	struct zwl_object *surface)
{
	int32_t step;
	int32_t width;
	int32_t height;
	uint32_t buffer_width;
	uint32_t buffer_height;

	/* A fullscreen window covers the output. */
	if (surface->fullscreen) {
		surface->x = 0;
		surface->y = 0;
		return;
	}

	/* Centred at its size (a viewport's, viewport.c), then moved down and right by the number of windows placed so far (in a cycle of eight). */
	width = (int32_t)server->width;
	height = (int32_t)server->height;
	if (surface->current != NULL) {
		zwl_surface_size(surface, &buffer_width, &buffer_height);
		width = (int32_t)buffer_width;
		height = (int32_t)buffer_height;
	}

	/* The cascade step of this window, which now has a place to come back to from fullscreen. */
	step = ZWL_CASCADE_STEP * (int32_t)(server->windows % 8U);
	server->windows++;
	surface->placed = 1;

	/* The glass look keeps the space of the system bar and a title bar free. */
	if (server->glass) {
		zwl_glass_place(server, surface, width, height, step);
		return;
	}

	/* Centred on the output. */
	surface->x = ((int32_t)server->width - width) / 2 + step;
	surface->y = ((int32_t)server->height - height) / 2 + step;
	if (surface->x < 0)
		surface->x = 0;
	if (surface->y < 0)
		surface->y = 0;
}

/*
 * Enters window mode, once at the start: the swapchain is made over the
 * display once sessiond hands it over.
 */
static int
enter_window_mode(
	struct zwl_server *server)
{
	uint64_t start;
	int error;

	/* The switch is timed from here (ZWL MODE). */
	start = zwl_milliseconds();

	/*
	 * The display surface and the pipelines need no lease, so they are made
	 * while the display is still another's (ws035-p130); a failure is tried
	 * again below, after the hand-over.
	 */
	(void)zwl_compose_output_prepare(server);

	/* The first time, sessiond hands the display over (the greeter goes first). */
	zwl_handoff_wait(server);

	/* The swapchain over the display. */
	error = zwl_compose_output_open(server);
	if (error != 0)
		return error;

	/* Succeeded: the next pass draws a frame. */
	server->windowed = 1;
	server->dirty = 1;
	server->mode_switch_ms = zwl_milliseconds() - start;
	printf("ZWL MODE window switch_ms=%llu at_ms=%llu\n", (unsigned long long)server->mode_switch_ms, (unsigned long long)zwl_milliseconds());
	return 0;
}
