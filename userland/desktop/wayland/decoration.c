/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Negotiates each toplevel's decoration ownership. Clients without an SSD
 * request keep their own decorations. A native titlebar is an explicit
 * Keiland SSD request, subordinate to an xdg-decoration object's choice.
 * Configure snapshots carry mode ownership through ack and surface commit.
 */

#include "extras.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The manager and decoration wire requests, and the decoration configure event. */
#define MANAGER_DESTROY 0U
#define MANAGER_GET_TOPLEVEL_DECORATION 1U
#define DECORATION_DESTROY 0U
#define DECORATION_SET_MODE 1U
#define DECORATION_UNSET_MODE 2U
#define DECORATION_CONFIGURE 0U

/* The ownership modes specified by xdg-decoration v1. */
#define MODE_CLIENT_SIDE 1U
#define MODE_SERVER_SIDE 2U

/* Protocol errors distinguish already-mapped, duplicate, orphaned and invalid requests. */
#define DECORATION_ERROR_UNCONFIGURED_BUFFER 0U
#define DECORATION_ERROR_ALREADY_CONSTRUCTED 1U
#define DECORATION_ERROR_ORPHANED 2U
#define DECORATION_ERROR_INVALID_MODE 3U

/*
 * One outstanding xdg_surface configure's decoration ownership. The
 * toplevel owns this list; an acknowledgment retires the acknowledged and
 * older snapshots, and toplevel teardown frees everything still pending.
 */
struct zwl_decoration_configure {
	struct zwl_decoration_configure *next;
	uint32_t serial;
	uint32_t mode;
	uint64_t generation;
};

static int decoration_create(struct zwl_object *manager, const unsigned char *bytes, size_t size);
static int decoration_answer(struct zwl_object *decoration);
static struct zwl_object *decoration_top(const struct zwl_object *surface);
static void decoration_reset(struct zwl_object *toplevel);
static void decoration_invalidate(struct zwl_object *toplevel);
static void decoration_free(struct zwl_object *toplevel);
static uint32_t decoration_word(const unsigned char *bytes, size_t offset);

/*
 * Dispatches decoration requests while keeping preferred and visible modes apart.
 */
int
zwl_decoration_request(
    struct zwl_object *object,
    uint32_t opcode,
	const unsigned char *bytes,
    size_t size)
{
	struct zwl_object *toplevel;
	uint32_t mode;
	int error;

	/* The manager either retires or associates one decoration with a toplevel. */
	if (object->kind == ZWL_DECORATION_MANAGER) {
		if (opcode == MANAGER_DESTROY && size == 0U) {
			zwl_object_destroy(object);

			/* Succeeded: existing decoration objects retain their ownership. */
			return 0;
		}

		/* Rejects every other manager operation. */
		if (opcode != MANAGER_GET_TOPLEVEL_DECORATION)
			return EPROTO;

		/* Associates and configures the requested decoration. */
		error = decoration_create(object, bytes, size);
		if (error != 0)
			return error;

		/* Succeeded: the new decoration has an initial CSD proposal. */
		return 0;
	}

	/* A valid destructor withdraws this decoration at the next surface commit. */
	if (opcode == DECORATION_DESTROY) {
		if (size != 0U)
			return EPROTO;

		/* Detaches the object and stages client ownership. */
		zwl_object_destroy(object);

		/* Succeeded: the toplevel remains alive. */
		return 0;
	}

	/* An orphaned decoration cannot configure a window that has gone. */
	toplevel = object->decoration_toplevel;
	if (toplevel == NULL) {
		error = zwl_error_code(object->client, object->id, DECORATION_ERROR_ORPHANED, "the toplevel is gone");
		if (error != 0)
			return error;

		/* Refuses state changes on the orphaned object. */
		return EPROTO;
	}

	/* A preferred mode must be a complete and recognized protocol word. */
	mode = 0U;
	if (opcode == DECORATION_SET_MODE) {
		if (size != 4U)
			return EPROTO;

		/* Decodes the client's decoration ownership preference. */
		mode = decoration_word(bytes, 0U);
		if (mode != MODE_CLIENT_SIDE && mode != MODE_SERVER_SIDE) {
			error = zwl_error_code(object->client, object->id, DECORATION_ERROR_INVALID_MODE, "not a decoration mode");
			if (error != 0)
				return error;

			/* Refuses modes outside the protocol enumeration. */
			return EPROTO;
		}
	} else if (opcode == DECORATION_UNSET_MODE) {
		/* An unset preference carries no payload and defaults to CSD. */
		if (size != 0U)
			return EPROTO;
	} else {
		/* Refuses unknown decoration operations. */
		return EPROTO;
	}

	/* Publishes a new proposal without discarding an already acknowledged mode. */
	toplevel->decoration_preferred = mode;
	printf("ZWL DECORATION asked client=%llu mode=%u\n", (unsigned long long)object->client->number, mode);

	/* Sends decoration configure before the corresponding xdg_surface configure. */
	error = decoration_answer(object);
	if (error != 0)
		return error;

	/* Succeeded: the proposal waits for acknowledgment and surface commit. */
	return 0;
}

/*
 * Detaches decoration identities and frees toplevel-owned configure snapshots.
 */
void
zwl_decoration_object_gone(
    struct zwl_object *object)
{
	struct zwl_object *toplevel;

	/* A destroyed decoration withdraws SSD at the next commit of its surviving window. */
	if (object->kind == ZWL_DECORATION) {
		toplevel = object->decoration_toplevel;
		if (toplevel != NULL) {
			toplevel->decoration = NULL;

			/* Invalidates outstanding proposals from the destroyed object. */
			decoration_reset(toplevel);
		}

		/* Removes the retired object's link to its surviving toplevel. */
		object->decoration_toplevel = NULL;

		/* The surviving toplevel owns its pending mode and snapshots. */
		return;
	}

	/* A toplevel's pending configuration history dies with its identity. */
	if (object->kind == ZWL_TOPLEVEL) {
		decoration_free(object);
		if (object->decoration != NULL)
			object->decoration->decoration_toplevel = NULL;

		/* Removes the retired toplevel's association. */
		object->decoration = NULL;
	}

	/* No decoration identity names the retired object. */
	return;
}

/*
 * Captures the decoration proposal belonging to one xdg_surface configure serial.
 */
int
zwl_decoration_configure(
    struct zwl_object *surface,
    uint32_t serial)
{
	struct zwl_decoration_configure *snapshot;
	struct zwl_decoration_configure **link;
	struct zwl_object *toplevel;

	/* Popups and surfaces without a toplevel have no decoration negotiation. */
	toplevel = decoration_top(surface);
	if (toplevel == NULL)
		return 0;

	/* Each serial preserves its own mode even if a newer proposal follows. */
	snapshot = calloc(1, sizeof(*snapshot));
	if (snapshot == NULL)
		return ENOMEM;

	/* Initializes this toplevel-owned record before exposing it through the list. */
	snapshot->serial = serial;
	snapshot->mode = MODE_CLIENT_SIDE;
	if (toplevel->decoration_configured == MODE_SERVER_SIDE)
		snapshot->mode = MODE_SERVER_SIDE;

	/* Records which decoration identity proposed this ownership. */
	snapshot->generation = toplevel->decoration_generation;

	/* Appends in configure order so an acknowledgment can retire its prefix. */
	link = &toplevel->decoration_configures;
	while (*link != NULL)
		link = &(*link)->next;

	/* Publishes the initialized record at the end of the history. */
	*link = snapshot;

	/* Succeeded: this serial identifies a stable decoration proposal. */
	return 0;
}

/*
 * Stages the mode from an acknowledged configure and retires older proposals.
 */
int
zwl_decoration_ack(
    struct zwl_object *surface,
    uint32_t serial)
{
	struct zwl_decoration_configure *snapshot;
	struct zwl_decoration_configure *retired;
	struct zwl_decoration_configure *after;
	struct zwl_object *toplevel;
	uint32_t mode;
	uint64_t generation;

	/* Popup acknowledgment follows its independent placement protocol. */
	toplevel = decoration_top(surface);
	if (toplevel == NULL)
		return 0;

	/* Locates the exact outstanding serial, never a foreign or consumed one. */
	snapshot = toplevel->decoration_configures;
	while (snapshot != NULL) {
		if (snapshot->serial == serial)
			break;

		/* Continues through still-outstanding proposals. */
		snapshot = snapshot->next;
	}

	/* An unknown or previously consumed configure cannot select a decoration mode. */
	if (snapshot == NULL)
		return EPROTO;

	/* Preserves the selected proposal before removing the consumed prefix. */
	mode = snapshot->mode;
	generation = snapshot->generation;

	/* Older generations remain valid xdg acknowledgments but cannot resurrect retired decorations. */
	if (generation == toplevel->decoration_generation) {
		toplevel->decoration_acked_mode = mode;
		toplevel->decoration_acked = 1;
	}

	/* Frees the acknowledged configure and every superseded predecessor. */
	after = snapshot->next;
	while (toplevel->decoration_configures != after) {
		retired = toplevel->decoration_configures;
		toplevel->decoration_configures = retired->next;
		free(retired);
	}

	/* Succeeded: the matching proposal waits for the next surface commit. */
	return 0;
}

/*
 * Applies acknowledged decoration ownership with the surface's committed state.
 */
void
zwl_decoration_commit(
    struct zwl_object *surface)
{
	struct zwl_object *toplevel;
	uint32_t mode;

	/* Only toplevels carry decoration ownership. */
	toplevel = decoration_top(surface);
	if (toplevel == NULL)
		return;

	/* A destroyed decoration withdraws SSD on this commit without a new configure. */
	mode = toplevel->decoration_committed;
	if (toplevel->decoration_reset) {
		mode = MODE_CLIENT_SIDE;
		toplevel->decoration_reset = 0;
		toplevel->decoration_acked = 0;
	} else if (toplevel->decoration_acked) {
		/* The acknowledged mode joins the content produced for that configure. */
		mode = toplevel->decoration_acked_mode;
		toplevel->decoration_acked = 0;
	}

	/* Every renderer and hit test observes the same committed ownership. */
	if (mode != toplevel->decoration_committed) {
		toplevel->decoration_committed = mode;
		surface->client->server->dirty = 1;
		printf("ZWL DECORATION applied client=%llu surface=%u mode=%u\n", (unsigned long long)surface->client->number, surface->id, mode);
	}

	/* Succeeded: drawing and hit testing follow the committed content. */
	return;
}

/*
 * Reports whether the compositor owns the committed decoration of a surface.
 */
int
zwl_decoration_server(
	const struct zwl_object *surface)
{
	struct zwl_object *toplevel;

	/* Surfaces outside a toplevel's role have no server decoration. */
	toplevel = decoration_top(surface);
	if (toplevel == NULL)
		return 0;

	/* Client decoration is the default, including zero-initialized ownership. */
	if (toplevel->decoration_committed != MODE_SERVER_SIDE)
		return 0;

	/* Succeeded: this surface committed an explicit SSD proposal. */
	return 1;
}

/*
 * Proposes native SSD ownership when a titlebar is explicitly created or removed.
 */
int
zwl_decoration_native_changed(
    struct zwl_object *toplevel)
{
	struct zwl_object *surface;
	uint32_t mode;
	int error;

	/* An xdg-decoration object's CSD or unset request takes precedence. */
	if (toplevel->decoration != NULL)
		return 0;

	/* The native extension explicitly requests the compositor's titlebar. */
	mode = MODE_CLIENT_SIDE;
	if (toplevel->titlebar != NULL)
		mode = MODE_SERVER_SIDE;

	/* An unchanged request needs no additional configure generation. */
	if (mode == toplevel->decoration_configured)
		return 0;

	/* Offers the new native ownership while preserving acknowledged content. */
	toplevel->decoration_configured = mode;
	surface = toplevel->surface;

	/* Removing the extension withdraws its decoration at the next commit. */
	if (mode == MODE_CLIENT_SIDE) {
		decoration_reset(toplevel);

		/* The surviving client already owns its content and next commit. */
		return 0;
	}

	/* A new native request supersedes a pending withdrawal of its old titlebar. */
	toplevel->decoration_reset = 0;

	/* The first empty commit will carry native SSD in its initial configure. */
	if (surface == NULL)
		return 0;

	/* Leaves initial configuration to the first empty commit. */
	if (!surface->configured)
		return 0;

	/* A mapped client must acknowledge the new ownership before it is drawn. */
	error = zwl_window_send_configure(surface);
	if (error != 0)
		return error;

	/* Succeeded: native decoration waits for the client's configure acknowledgment. */
	return 0;
}

/*
 * Reports the xdg window geometry used for configure sizes and state restoration.
 */
void
zwl_decoration_geometry(
	const struct zwl_object *surface,
    uint32_t *width,
    uint32_t *height)
{
	/* Starts with the buffer or viewport extent for windows without geometry. */
	zwl_surface_size(surface, width, height);

	/* Client shadows lie outside the extent specified by xdg_surface geometry. */
	if (surface->geometry_set) {
		*width = (uint32_t)surface->geometry[2];
		*height = (uint32_t)surface->geometry[3];
	}

	/* Succeeded: configure sizes exclude client decoration shadows. */
	return;
}

/* Creates one decoration before the first committed buffer and offers CSD ownership. */
static int
decoration_create(
    struct zwl_object *manager,
	const unsigned char *bytes,
    size_t size)
{
	struct zwl_object *toplevel;
	struct zwl_object *created;
	struct zwl_object *surface;
	uint32_t id;
	uint32_t toplevel_id;
	int error;

	/* The association contains exactly the new ID and its toplevel ID. */
	if (size != 8U)
		return EPROTO;

	/* Decodes each wire identity before consulting object ownership. */
	id = decoration_word(bytes, 0U);
	toplevel_id = decoration_word(bytes, 4U);
	toplevel = zwl_find(manager->client, toplevel_id);
	if (toplevel == NULL)
		return EPROTO;

	/* Refuses roles which cannot own toplevel decoration. */
	if (toplevel->kind != ZWL_TOPLEVEL)
		return EPROTO;

	/* The protocol permits exactly one decoration object on this toplevel. */
	if (toplevel->decoration != NULL) {
		error = zwl_error_code(manager->client, manager->id, DECORATION_ERROR_ALREADY_CONSTRUCTED, "the toplevel has a decoration");
		if (error != 0)
			return error;

		/* Refuses a second owner for the same toplevel. */
		return EPROTO;
	}

	/* Version one requires decoration negotiation before buffer content was committed. */
	surface = toplevel->surface;
	if (surface != NULL) {
		if (surface->current != NULL ||
		    surface->queued != NULL ||
		    surface->pending != NULL) {
			error = zwl_error_code(manager->client, manager->id, DECORATION_ERROR_UNCONFIGURED_BUFFER, "the toplevel already has content");
			if (error != 0)
				return error;

			/* Refuses the version-one association after window content exists. */
			return EPROTO;
		}
	}

	/* The object registry retains the associated decoration until its destructor. */
	created = zwl_create(manager->client, id, ZWL_DECORATION, manager->version);
	if (created == NULL)
		return EPROTO;

	/* Both directions are detached when either identity is retired. */
	created->decoration_toplevel = toplevel;
	toplevel->decoration = created;
	toplevel->decoration_preferred = 0U;
	decoration_invalidate(toplevel);
	toplevel->decoration_acked = 0;
	toplevel->decoration_reset = 0;

	/* Publishes the initial client-side ownership proposal. */
	error = decoration_answer(created);
	if (error != 0)
		return error;

	/* Succeeded: ownership now follows the decoration protocol. */
	return 0;
}

/* Offers the preferred mode and sends its corresponding surface configure. */
static int
decoration_answer(
    struct zwl_object *decoration)
{
	struct zwl_object *toplevel;
	struct zwl_object *surface;
	uint32_t mode;
	int error;

	/* An orphan has no ownership proposal to publish. */
	toplevel = decoration->decoration_toplevel;
	if (toplevel == NULL)
		return EPROTO;

	/* Unspecified ownership defaults to client-side decoration. */
	mode = MODE_CLIENT_SIDE;
	if (toplevel->decoration_preferred == MODE_SERVER_SIDE)
		mode = MODE_SERVER_SIDE;

	/* Retains the offered ownership for the next surface configure. */
	toplevel->decoration_configured = mode;

	/* Decoration configure precedes the matching xdg_surface configure. */
	error = zwl_emit(decoration->client, decoration->id, DECORATION_CONFIGURE, &mode, sizeof(mode));
	if (error != 0)
		return error;

	/* Records the mode actually sent on the decoration wire. */
	printf("ZWL DECORATION configure client=%llu mode=%u\n", (unsigned long long)decoration->client->number, mode);

	/* The first empty surface commit sends the initial xdg configure. */
	surface = toplevel->surface;
	if (surface == NULL)
		return 0;

	/* Leaves initial configuration to the first empty commit. */
	if (!surface->configured)
		return 0;

	/* Subsequent proposals receive their own acknowledgment serial. */
	error = zwl_window_send_configure(surface);
	if (error != 0)
		return error;

	/* Succeeded: the client knows the offered ownership and its configure boundary. */
	return 0;
}

/* Withdraws decoration proposals while preserving the last visible mode until commit. */
static void
decoration_reset(
    struct zwl_object *toplevel)
{
	/* Older snapshots must never resurrect an identity that no longer exists. */
	decoration_invalidate(toplevel);
	toplevel->decoration_preferred = 0U;
	toplevel->decoration_configured = MODE_CLIENT_SIDE;
	toplevel->decoration_acked = 0;
	toplevel->decoration_reset = 1;

	/* The next surface commit withdraws SSD. */
	return;
}

/* Invalidates old identity proposals even when the lifecycle generation wraps. */
static void
decoration_invalidate(
	struct zwl_object *toplevel)
{
	struct zwl_decoration_configure *snapshot;

	/* Zero marks retired proposals; active lifecycle generations always remain nonzero. */
	snapshot = toplevel->decoration_configures;
	while (snapshot != NULL) {
		snapshot->generation = 0U;
		snapshot = snapshot->next;
	}

	/* Unsigned wrap is defined; skipping zero prevents stale proposals becoming active. */
	toplevel->decoration_generation++;
	if (toplevel->decoration_generation == 0U)
		toplevel->decoration_generation = 1U;

	/* Every older snapshot is invalid regardless of its original generation. */
	return;
}

/* Frees every configure snapshot owned by a retired toplevel. */
static void
decoration_free(
    struct zwl_object *toplevel)
{
	struct zwl_decoration_configure *snapshot;

	/* The toplevel is the sole owner of this pending history. */
	while (toplevel->decoration_configures != NULL) {
		snapshot = toplevel->decoration_configures;
		toplevel->decoration_configures = snapshot->next;
		free(snapshot);
	}

	/* No configure history outlives the toplevel. */
	return;
}

/* Finds the toplevel whose committed surface owns decoration state. */
static struct zwl_object *
decoration_top(
	const struct zwl_object *surface)
{
	struct zwl_object *toplevel;

	/* A surface without a shell role has no toplevel identity. */
	if (surface == NULL)
		return NULL;

	/* A surface without a role cannot own decoration state. */
	if (surface->role == NULL)
		return NULL;

	/* Popup placement and decoration ownership are independent. */
	toplevel = surface->role->top;
	if (toplevel == NULL)
		return NULL;

	/* Refuses roles which cannot own toplevel decoration. */
	if (toplevel->kind != ZWL_TOPLEVEL)
		return NULL;

	/* Succeeded: its toplevel owns every decoration field. */
	return toplevel;
}

/* Reads an unaligned native-endian protocol word. */
static uint32_t
decoration_word(
	const unsigned char *bytes,
	size_t offset)
{
	uint32_t word;

	/* Wire framing validates the extent; copying avoids alignment assumptions. */
	memcpy(&word, bytes + offset, sizeof(word));

	/* Succeeded: the decoded wire word. */
	return word;
}
