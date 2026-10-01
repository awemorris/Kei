/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Selected core Wayland, xdg-shell and typed GPU-buffer request semantics.
 */

#include "desktop.h"
#include "zwl.h"
#include "menu.h"
#include "titlebar.h"
#include "inset.h"
#include "edit.h"
#include "popup.h"
#include "toplevel.h"
#include "subsurface.h"
#include "data.h"
#include "extras.h"
#include "panels.h"
#include "tablet.h"
#include "ime.h"
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* wl_output's version 4 events: its name and its description. */
#define OUTPUT_NAME		4U
#define OUTPUT_DESCRIPTION	5U

/* xdg_wm_base's error for a binding destroyed under its own live xdg_surfaces. */
#define WM_ERROR_DEFUNCT_SURFACES	1U

/* The registry advertises only implemented interfaces and their actual versions. */
struct zwl_global {
	uint32_t name;
	const char *interface;
	uint32_t version;
	enum zwl_kind kind;
};

/* Stable global names are scoped to one compositor process generation. */
static const struct zwl_global globals[] = {
	{ 1, "wl_compositor", 4, ZWL_COMPOSITOR },
	{ 2, "xdg_wm_base", 4, ZWL_WM },
	{ 3, NULL, 0, ZWL_FACTORY },
	{ 4, "wl_output", 4, ZWL_OUTPUT },
	{ 5, "wl_seat", 5, ZWL_SEAT },
	{ 6, "wl_shm", 1, ZWL_SHM },
	{ 7, "xdg_menu_manager_v1", 2, ZWL_MENU_MANAGER },
	{ 8, "wl_subcompositor", 1, ZWL_SUBCOMPOSITOR },
	{ 9, "wl_data_device_manager", 3, ZWL_DATA_MANAGER },
	{ 10, "zxdg_decoration_manager_v1", 1, ZWL_DECORATION_MANAGER },
	{ 11, "wp_cursor_shape_manager_v1", 1, ZWL_CURSOR_SHAPE_MANAGER },
	{ 12, "wp_viewporter", 1, ZWL_VIEWPORTER },
	{ 13, "zwp_text_input_manager_v3", 1, ZWL_TEXT_INPUT_MANAGER },
	{ 14, "zwp_input_method_manager_v2", 1, ZWL_INPUT_METHOD_MANAGER },
	{ 15, "zwp_virtual_keyboard_manager_v1", 1, ZWL_VIRTUAL_KEYBOARD_MANAGER },
	{ 16, "keiland_titlebar_manager_v1", 3, ZWL_TITLEBAR_MANAGER },
	{ 17, "keiland_glass_manager_v1", 2, ZWL_GLASS_MANAGER },
	{ 18, "zwp_primary_selection_device_manager_v1", 1, ZWL_PRIMARY_MANAGER },
	{ 19, "zwp_tablet_manager_v2", 1, ZWL_TABLET_MANAGER },
	{ 20, "keiland_ime_status_manager_v1", 1, ZWL_IME_STATUS_MANAGER },
	{ 21, "keiland_desktop_manager_v1", 1, ZWL_DESKTOP_MANAGER },
	{ 22, "keiland_keyboard_inset_manager_v1", 1, ZWL_KEYBOARD_INSET_MANAGER },
	{ 23, "keiland_edit_manager_v1", 1, ZWL_EDIT_MANAGER },
};

static void global_identity(const struct zwl_global *global, const char **interface, uint32_t *version);
static uint32_t word_at(const unsigned char *bytes, size_t offset);
static int string_at(const unsigned char *bytes, size_t size, size_t offset, const char **text, size_t *next);
static int registry_events(struct zwl_object *registry);
static int output_events(struct zwl_object *output);
static int output_names(struct zwl_object *output);
static int bind_global(struct zwl_object *registry, const unsigned char *bytes, size_t size);
static int surface_request(struct zwl_object *surface, uint32_t opcode, const unsigned char *bytes, size_t size);
static int surface_commit(struct zwl_object *surface);
static int shell_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
static void append_callbacks(struct zwl_object **list, struct zwl_object *callbacks);
static void add_damage(struct zwl_object *surface, int32_t x, int32_t y, int32_t width, int32_t height);
static void commit_fence(struct zwl_object *surface, unsigned attached);
static void commit_damage(struct zwl_object *surface);
static int send_bounds(struct zwl_object *surface);
static void window_bounds(struct zwl_server *server, int32_t *width, int32_t *height);

/*
 * Dispatches one validated frame through its client-local interface identity.
 */
int
zwl_dispatch(
	struct zwl_client *client,
	uint32_t id,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	struct zwl_object *object;
	struct zwl_object *created;
	uint32_t new_id;
	enum zwl_kind kind;
	int error;

	/* Unknown IDs cannot select another client's protocol objects or GPU resources. */
	object = zwl_find(client, id);
	if (object == NULL) {
		error = zwl_error(client, id, "unknown object");
		return error;
	}

	/* Every method validates its exact payload length and interface opcode. */
	error = EPROTO;
	switch (object->kind) {
	case ZWL_DISPLAY:
		/* Both supported display constructors contain exactly one new_id. */
		if (size != 4U || opcode > 1U)
			break;

		/* A sync callback is completed after preceding requests have been processed. */
		new_id = word_at(bytes, 0);
		kind = ZWL_CALLBACK;
		if (opcode == 1U)
			kind = ZWL_REGISTRY;

		/* Duplicate or forbidden IDs are rejected before emitting constructor events. */
		created = zwl_create(client, new_id, kind, 1);
		if (created == NULL)
			break;

		/* Registry discovery and one-shot sync use their canonical server events. */
		if (opcode == 1U)
			error = registry_events(created);
		else {
			zwl_callbacks_done(&created);
			error = 0;
		}

		break;
	case ZWL_REGISTRY:
		/* bind is the sole registry request. */
		if (opcode == 0)
			error = bind_global(object, bytes, size);
		break;
	case ZWL_COMPOSITOR:
		/* The compositor creates independent surfaces or region identities. */
		if (size != 4U || opcode > 1U)
			break;

		/* Regions need no retained geometry for this opaque fullscreen policy. */
		kind = ZWL_SURFACE;
		if (opcode == 1U)
			kind = ZWL_REGION;

		/* The child surface inherits the negotiated compositor version. */
		new_id = word_at(bytes, 0);
		created = zwl_create(client, new_id, kind, object->version);
		if (created != NULL)
			error = 0;
		break;
	case ZWL_SURFACE:
		error = surface_request(object, opcode, bytes, size);
		break;
	case ZWL_REGION:
		/* Region destruction retires only its protocol identity. */
		if (opcode == 0 && size == 0) {
			zwl_object_destroy(object);
			error = 0;
		} else if ((opcode == 1U || opcode == 2U) && size == 16U) {
			/* Damage and input regions do not alter the single opaque scanout plane. */
			error = 0;
		}

		break;
	case ZWL_BUFFER:
		/* Buffer destruction keeps any active GPU use alive independently. */
		if (opcode == 0 && size == 0) {
			zwl_object_destroy(object);
			error = 0;
		}

		break;
	case ZWL_FACTORY:
		error = zwl_gpu_request(object, opcode, bytes, size);
		break;
	case ZWL_SHM:
	case ZWL_SHM_POOL:
		error = zwl_shm_request(object, opcode, bytes, size);
		break;
	case ZWL_WM:
	case ZWL_XDG_SURFACE:
	case ZWL_TOPLEVEL:
		error = shell_request(object, opcode, bytes, size);
		break;
	case ZWL_SEAT:
	case ZWL_POINTER:
	case ZWL_KEYBOARD:
	case ZWL_TOUCH:
		error = zwl_seat_request(object, opcode, bytes, size);
		break;
	case ZWL_MENU_MANAGER:
	case ZWL_MENU:
	case ZWL_TOPLEVEL_MENU:
	case ZWL_CONTEXT_MENU:
		/* The System Menu and its context menus (menu.c). */
		error = zwl_menu_request(object, opcode, bytes, size);
		break;
	case ZWL_TITLEBAR_MANAGER:
	case ZWL_TITLEBAR:
		/* The Titlebar Presentation (titlebar.c). */
		error = zwl_titlebar_request(object, opcode, bytes, size);
		break;
	case ZWL_POSITIONER:
	case ZWL_POPUP:
		/* xdg_positioner and xdg_popup (popup.c). */
		error = zwl_popup_request(object, opcode, bytes, size);
		break;
	case ZWL_OUTPUT:
		/* release, from version 3, is the only output request. */
		if (opcode == 0 && size == 0 && object->version >= 3U) {
			zwl_object_destroy(object);
			error = 0;
		}

		/* Any other output request stays refused. */
		break;
	case ZWL_DATA_MANAGER:
	case ZWL_DATA_SOURCE:
	case ZWL_DATA_DEVICE:
	case ZWL_DATA_OFFER:
		/* The clipboard (data.c). */
		error = zwl_data_request(object, opcode, bytes, size);
		break;
	case ZWL_PRIMARY_MANAGER:
	case ZWL_PRIMARY_SOURCE:
	case ZWL_PRIMARY_DEVICE:
	case ZWL_PRIMARY_OFFER:
		/* The primary selection (primary.c). */
		error = zwl_primary_request(object, opcode, bytes, size);
		break;
	case ZWL_DECORATION_MANAGER:
	case ZWL_DECORATION:
		/* xdg-decoration (decoration.c). */
		error = zwl_decoration_request(object, opcode, bytes, size);
		break;
	case ZWL_CURSOR_SHAPE_MANAGER:
	case ZWL_CURSOR_SHAPE_DEVICE:
		/* cursor-shape (cursor.c). */
		error = zwl_cursor_shape_request(object, opcode, bytes, size);
		break;
	case ZWL_VIEWPORTER:
	case ZWL_VIEWPORT:
		/* viewporter (viewport.c). */
		error = zwl_viewport_request(object, opcode, bytes, size);
		break;
	case ZWL_GLASS_MANAGER:
	case ZWL_GLASS:
		/* A surface's glass panels (panels.c). */
		error = zwl_panels_request(object, opcode, bytes, size);
		break;
	case ZWL_SUBCOMPOSITOR:
		/* wl_subcompositor (subsurface.c). */
		error = zwl_subcompositor_request(object, opcode, bytes, size);
		break;
	case ZWL_SUBSURFACE:
		/* wl_subsurface (subsurface.c). */
		error = zwl_subsurface_request(object, opcode, bytes, size);
		break;
	case ZWL_TEXT_INPUT_MANAGER:
	case ZWL_TEXT_INPUT:
		/* The text input protocol (text-input.c). */
		error = zwl_text_input_request(object, opcode, bytes, size);
		break;
	case ZWL_INPUT_METHOD_MANAGER:
	case ZWL_INPUT_METHOD:
	case ZWL_INPUT_POPUP:
	case ZWL_KEYBOARD_GRAB:
	case ZWL_VIRTUAL_KEYBOARD_MANAGER:
	case ZWL_VIRTUAL_KEYBOARD:
	case ZWL_IME_STATUS_MANAGER:
	case ZWL_IME_STATUS:
		/* The input method's protocols (input-method.c). */
		error = zwl_ime_request(object, opcode, bytes, size);
		break;
	case ZWL_TABLET_MANAGER:
	case ZWL_TABLET_SEAT:
	case ZWL_TABLET:
	case ZWL_TABLET_TOOL:
		/* The pen tablets (tablet.c). */
		error = zwl_tablet_request(object, opcode, bytes, size);
		break;
	case ZWL_DESKTOP_MANAGER:
	case ZWL_DESKTOP_SURFACE:
		/* The desktop surface (desktop.c, ws094-p002). */
		error = zwl_desktop_request(object, opcode, bytes, size);
		break;
	case ZWL_KEYBOARD_INSET_MANAGER:
	case ZWL_KEYBOARD_INSET:
		/* The keyboard inset (inset.c, ws102-p015). */
		error = zwl_inset_request(object, opcode, bytes, size);
		break;
	case ZWL_EDIT_MANAGER:
	case ZWL_EDIT:
		/* The editing operations (edit.c, ws102-p017). */
		error = zwl_edit_request(object, opcode, bytes, size);
		break;
	default:
		/* Callback objects and version-2 outputs have no client requests. */
		break;
	}

	/* Ancillary delivery may trail a complete frame; retain it without consuming state. */
	if (error == EAGAIN)
		return EAGAIN;

	/* Preserve a specific protocol error already emitted by a delegated handler. */
	if (error != 0) {
		/* A delegated terminal error owns the connection's sole final error event. */
		if (!client->fatal)
			error = zwl_error(client, id, "invalid or unsupported request");

		/* The final error remains queued for bounded flush before disconnect. */
		return error;
	}

	/* Succeeded: the complete request has been processed exactly once. */
	return 0;
}

/* Reads one possibly unaligned native-endian protocol word. */
static uint32_t
word_at(
	const unsigned char *bytes,
	size_t offset)
{
	uint32_t word;

	/* Callers validate the containing payload before requesting a word. */
	memcpy(&word, bytes + offset, sizeof(word));

	/* Succeeded: return the decoded scalar without pointer-alignment assumptions. */
	return word;
}

/* Validates one non-null string and returns the following aligned argument position. */
static int
string_at(
	const unsigned char *bytes,
	size_t size,
	size_t offset,
	const char **text,
	size_t *next)
{
	uint32_t length;
	size_t aligned;
	size_t actual;

	/* The string length word must fit before its payload can be inspected. */
	if (offset > size || size - offset < 4U)
		return EPROTO;

	/* Non-null strings contain one terminator within the declared byte count. */
	length = word_at(bytes, offset);
	if (length == 0 || length > size - offset - 4U)
		return EPROTO;

	/* Aligned storage must also fit even when the actual text length is unaligned. */
	aligned = ((size_t)length + 3U) & ~(size_t)3U;
	if (aligned > size - offset - 4U)
		return EPROTO;

	/* Embedded NUL bytes may not conceal trailing non-string arguments. */
	*text = (const char *)bytes + offset + 4U;
	if ((*text)[length - 1U] != '\0')
		return EPROTO;

	/* The declared byte count must describe this exact terminated string. */
	actual = strlen(*text);
	if (actual + 1U != length)
		return EPROTO;

	/* Succeeded: the next argument follows the string's padded storage. */
	*next = offset + 4U + aligned;
	return 0;
}

/* Reports a global's interface name and version: the table's, or the OS module's for its GPU buffer global. */
static void
global_identity(
	const struct zwl_global *global,
	const char **interface,
	uint32_t *version)
{
	/* The OS module names its GPU buffer global (zwl-gpu.h; keiland_gpu_buffer_v1 version 3 on zedBSD). */
	if (global->kind == ZWL_FACTORY) {
		*interface = zwl_gpu_global_interface();
		*version = zwl_gpu_global_version();
		return;
	}

	/* Every other global is the table's. */
	*interface = global->interface;
	*version = global->version;

	/* Succeeded: the caller holds the advertised identity of this global. */
	return;
}

/* Announces only the selected protocol globals in stable registry order. */
static int
registry_events(
	struct zwl_object *registry)
{
	const char *interface;
	uint32_t version;
	unsigned char payload[128];
	uint32_t word;
	size_t index;
	size_t length;
	size_t offset;
	int error;
	int visible;

	/* Each global event carries name, interface string and supported version. */
	for (index = 0; index < sizeof(globals) / sizeof(globals[0]); index++) {
		/* The input method's globals are shown to the input method alone (input-method.c). */
		visible = zwl_ime_global_visible(registry->client, globals[index].kind);
		if (!visible)
			continue;

		/* Resolve the OS-owned GPU global before encoding its registry event. */
		global_identity(&globals[index], &interface, &version);
		if (interface == NULL)
			continue;

		/* Encode this advertised interface as one canonical registry global event. */
		memset(payload, 0, sizeof(payload));
		word = globals[index].name;
		memcpy(payload, &word, 4);
		length = strlen(interface) + 1U;
		word = (uint32_t)length;
		memcpy(payload + 4, &word, 4);
		memcpy(payload + 8, interface, length);
		offset = 8U + ((length + 3U) & ~(size_t)3U);
		word = version;
		memcpy(payload + offset, &word, 4);
		error = zwl_emit(registry->client, registry->id, 0, payload, offset + 4U);
		if (error != 0)
			return error;
	}

	/* Succeeded: discovery events precede any following display sync callback. */
	return 0;
}

/* Supplies the version-2 output geometry, current mode, scale and completion event. */
static int
output_events(
	struct zwl_object *output)
{
	unsigned char geometry[60];
	uint32_t words[4];
	uint32_t word;
	int error;

	/*
	 * Geometry includes unknown physical dimensions, and the make and the
	 * model say that the display is not known (ws035-p121): the compositor
	 * draws to the GPU's scanout and never learns the monitor's EDID.
	 */
	memset(geometry, 0, sizeof(geometry));
	word = 8;
	memcpy(geometry + 20, &word, 4);
	memcpy(geometry + 24, "Unknown", 8);
	word = 8;
	memcpy(geometry + 32, &word, 4);
	memcpy(geometry + 36, "Unknown", 8);
	error = zwl_emit(output->client, output->id, 0, geometry, 48);
	if (error != 0)
		return error;

	/* This single mode is current and preferred in the selected fullscreen policy. */
	words[0] = 3;
	words[1] = output->client->server->width;
	words[2] = output->client->server->height;
	words[3] = output->client->server->refresh;
	error = zwl_emit(output->client, output->id, 1, words, sizeof(words));
	if (error != 0)
		return error;

	/* Scale and done were introduced in output version 2. */
	if (output->version >= 2U) {
		/* This output uses one buffer pixel per surface coordinate. */
		word = 1;
		error = zwl_emit(output->client, output->id, 3, &word, sizeof(word));
		if (error != 0)
			return error;

		/* Version 4 names the output and describes it (ws035-p078). */
		if (output->version >= 4U) {
			error = output_names(output);
			if (error != 0)
				return error;
		}

		/* The done event commits all preceding output properties. */
		error = zwl_emit(output->client, output->id, 2, NULL, 0);
		if (error != 0)
			return error;
	}

	/* Succeeded: output properties are complete for the negotiated version. */
	return 0;
}

/*
 * Sends a version 4 output its name and description: the name stays the
 * same for the whole run (one output), the description gives its size.
 * Both name the display, not the system (ws035-p121): the compositor does
 * not know the connector, so the name is a neutral DISPLAY-1.
 */
static int
output_names(
	struct zwl_object *output)
{
	char text[64];
	unsigned char payload[80];
	uint32_t length;
	int error;

	/* The name, a string in the wire's padded form. */
	memset(payload, 0, sizeof(payload));
	length = (uint32_t)sizeof("DISPLAY-1");
	memcpy(payload, &length, sizeof(length));
	memcpy(payload + 4, "DISPLAY-1", length);
	error = zwl_emit(output->client, output->id, OUTPUT_NAME, payload, 4U + ((length + 3U) & ~3U));
	if (error != 0)
		return error;

	/* The description. */
	(void)snprintf(text, sizeof(text), "Display %ux%u", output->client->server->width, output->client->server->height);
	memset(payload, 0, sizeof(payload));
	length = (uint32_t)strlen(text) + 1U;
	memcpy(payload, &length, sizeof(length));
	memcpy(payload + 4, text, length);
	error = zwl_emit(output->client, output->id, OUTPUT_DESCRIPTION, payload, 4U + ((length + 3U) & ~3U));
	if (error != 0)
		return error;

	/* Succeeded: the output is named. */
	return 0;
}

/* Resolves a registry name, exact interface string and supported requested version. */
static int
bind_global(
	struct zwl_object *registry,
	const unsigned char *bytes,
	size_t size)
{
	const char *interface;
	const char *offered;
	uint32_t offered_version;
	struct zwl_object *object;
	uint32_t name;
	uint32_t version;
	uint32_t id;
	size_t offset;
	size_t index;
	int error;
	int same;
	int visible;

	/* The dynamic bind signature contains a name followed by string/version/new_id. */
	if (size < 16U)
		return EPROTO;

	/* Validate the variable-width string before reading its following scalar arguments. */
	name = word_at(bytes, 0);
	error = string_at(bytes, size, 4, &interface, &offset);
	if (error != 0)
		return error;

	/* No extra bytes may trail the constructor's version and new identity. */
	if (offset + 8U != size)
		return EPROTO;

	/* A global cannot be bound under an unrelated interface or newer version. */
	version = word_at(bytes, offset);
	id = word_at(bytes, offset + 4U);
	for (index = 0; index < sizeof(globals) / sizeof(globals[0]); index++) {
		/* Stable numeric names select which interface and version may be bound. */
		if (globals[index].name != name)
			continue;

		/* Names, interface strings and negotiated versions are checked together. */
		global_identity(&globals[index], &offered, &offered_version);
		if (offered == NULL)
			return EPROTO;
		same = strcmp(interface, offered);
		if (same != 0 ||
		    version == 0 ||
		    version > offered_version)
			return EPROTO;

		/* A global the connection was not shown cannot be bound (the input method's, input-method.c). */
		visible = zwl_ime_global_visible(registry->client, globals[index].kind);
		if (!visible)
			return EPROTO;

		/* A successful binding creates exactly one independent client-side object. */
		object = zwl_create(registry->client, id, globals[index].kind, version);
		if (object == NULL)
			return EPROTO;

		/* Output bindings immediately receive the negotiated property snapshot. */
		if (object->kind == ZWL_OUTPUT) {
			/* Publish the newly bound output's complete initial property snapshot. */
			error = output_events(object);
			if (error != 0)
				return error;
		}

		/* wl_shm bindings learn the formats they may use. */
		if (object->kind == ZWL_SHM) {
			error = zwl_shm_bind(object);
			if (error != 0)
				return error;
		}

		/* Seat bindings immediately learn the present device classes and the seat name. */
		if (object->kind == ZWL_SEAT) {
			/* Publish the newly bound seat's capabilities and name. */
			error = zwl_seat_bind(object);
			if (error != 0)
				return error;
		}

		/* The selected binding needs no further global search. */
		break;
	}

	/* An exhausted search found no advertised global with the requested name. */
	if (index == sizeof(globals) / sizeof(globals[0]))
		return EPROTO;

	/* Succeeded: the registry binding owns its new protocol identity and initial events. */
	return 0;
}

/* Applies surface requests to pending state without prematurely releasing current scanout. */
static int
surface_request(
	struct zwl_object *surface,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	struct zwl_object *object;
	struct zwl_object *previous;
	uint32_t id;
	uint32_t scalar;
	uint32_t x;
	uint32_t y;
	int error;

	/* Surface request availability follows the bound compositor version. */
	if ((opcode == 7U && surface->version < 2U) ||
	    (opcode == 8U && surface->version < 3U) ||
	    (opcode == 9U && surface->version < 4U))
		return EPROTO;

	/* The selected surface methods preserve ordinary double-buffering semantics. */
	switch (opcode) {
	case 0:
		/* Shell roles must retire before their underlying surface identity. */
		if (size != 0 || surface->role != NULL)
			return EPROTO;

		/* Destruction keeps any front allocation alive until unscan succeeds. */
		zwl_object_destroy(surface);
		break;
	case 1:
		/* This fullscreen WSI uses zero attach offsets. */
		if (size != 12U)
			return EPROTO;

		/* Nonzero offsets cannot describe this full-output scanout contract. */
		x = word_at(bytes, 4);
		y = word_at(bytes, 8);
		if (x != 0 || y != 0)
			return EPROTO;

		/* Only a buffer created by this connection may supply pending surface content. */
		id = word_at(bytes, 0);
		object = NULL;
		if (id != 0) {
			/* Pending content cannot borrow another interface or another client's identity. */
			object = zwl_find(surface->client, id);
			if (object == NULL || object->kind != ZWL_BUFFER)
				return EPROTO;
		}

		/* Acquiring the replacement first makes repeated attachment of the same buffer safe. */
		zwl_buffer_get(object);
		previous = surface->pending;
		surface->pending = object;
		surface->attached = 1;
		zwl_buffer_put(previous);
		break;
	case 2:
	case 9:
		/* Damage is a rectangle (x, y, width, height), in buffer pixels at scale one without transform. */
		if (size != 16U)
			return EPROTO;

		/* It joins the pending damage (a wl_shm image copies only those rows). */
		add_damage(surface, (int32_t)word_at(bytes, 0), (int32_t)word_at(bytes, 4), (int32_t)word_at(bytes, 8), (int32_t)word_at(bytes, 12));
		break;
	case 3:
		/* One frame request allocates one callback in pending state. */
		if (size != 4U)
			return EPROTO;

		/* A callback ID cannot alias any existing interface. */
		id = word_at(bytes, 0);
		object = zwl_create(surface->client, id, ZWL_CALLBACK, 1);
		if (object == NULL)
			return EPROTO;

		/* The next commit owns this ordered callback, not the present front buffer. */
		append_callbacks(&surface->callbacks, object);
		break;
	case 4:
	case 5:
		/* Region references are nullable and belong to the same client. */
		if (size != 4U)
			return EPROTO;

		/* This opaque fullscreen policy needs no retained input/opaque region geometry. */
		id = word_at(bytes, 0);
		if (id != 0) {
			/* Advisory regions still require a live object of the correct interface. */
			object = zwl_find(surface->client, id);
			if (object == NULL || object->kind != ZWL_REGION)
				return EPROTO;
		}

		/* Succeeded: the region reference was valid at request time. */
		break;
	case 6:
		/* Commit itself has no arguments. */
		if (size != 0)
			return EPROTO;

		/* Publish pending state only after all role and configure checks succeed. */
		error = surface_commit(surface);
		if (error != 0)
			return error;

		/* Succeeded: the event loop may present this surface's completed commit. */
		break;
	case 7:
	case 8:
		/* The selected unscaled linear scanout cannot interpret transformed image storage. */
		if (size != 4U)
			return EPROTO;

		/* Transform zero and buffer scale one preserve the exported immutable image geometry. */
		scalar = word_at(bytes, 0);
		if ((opcode == 7U && scalar != 0U) || (opcode == 8U && scalar != 1U))
			return EPROTO;

		/* Succeeded: the requested transform is the supported identity transform. */
		break;
	default:
		/* No other surface version was advertised. */
		return EPROTO;
	}

	/* Succeeded: this supported request updated the surface's pending protocol state. */
	return 0;
}

/* Commits pending state after the initial xdg-shell configure/acknowledgment exchange. */
static int
surface_commit(
	struct zwl_object *surface)
{
	struct zwl_object *role;
	struct zwl_object *previous;
	struct zwl_server *server;
	unsigned attached;
	uint32_t replaced;
	int error;

	/* The viewport's pending source and destination apply with the commit (viewport.c). */
	zwl_viewport_commit(surface);

	/* So do the glass panels (panels.c). */
	zwl_panels_commit(surface);

	/* A sub-surface's commit waits for its parent's when it is synchronized (subsurface.c). */
	if (surface->sub_role != NULL) {
		error = zwl_subsurface_commit(surface);
		if (error != 0)
			return error;
		return 0;
	}

	/*
	 * A cursor surface's content is used directly, with no configure; a
	 * surface with no role yet keeps its content the same way, unshown
	 * (a client commits its cursor surface before set_cursor names it).
	 * Its sub-surfaces go with it.
	 */
	role = surface->role;
	server = surface->client->server;
	attached = surface->attached;
	if (surface->cursor_role || role == NULL) {
		error = zwl_surface_queue(surface);
		if (error != 0)
			return error;
		zwl_subsurface_applied(surface);
		return 0;
	}

	/* Otherwise only a toplevel supplies presentable content in this compositor. */
	if (role->top == NULL)
		return EPROTO;

	/* A window geometry set since the last commit applies from this one. */
	if (surface->pending_geometry_set) {
		memcpy(surface->geometry, surface->pending_geometry, sizeof(surface->geometry));
		surface->geometry_set = 1;
		surface->pending_geometry_set = 0;
	}

	/* The first empty commit requests the compositor's configure state. */
	if (!surface->configured) {
		/* Initial configure must precede every buffer-bearing map or remap. */
		if (surface->pending != NULL)
			return EPROTO;

		/* A popup gets its place and size (popup.c); a window chooses its size, a fullscreen one gets the output's. */
		if (role->top->kind == ZWL_POPUP) {
			error = zwl_popup_send_configure(surface);
		} else {
			error = zwl_window_send_configure(surface);
		}

		/* A client that cannot take the configure is failed. */
		if (error != 0)
			return error;

		/* Only ack_configure can authorize a later buffer-bearing commit. */
		surface->configured = 1;
		surface->attached = 0;
		return 0;
	}

	/* No buffer becomes presentable before its configure has been acknowledged. */
	if (!surface->acknowledged)
		return EPROTO;

	/* A commit without attach reuses its existing surface content. */
	if (!surface->attached) {
		/* Latest committed content may still be waiting for presentation. */
		surface->pending = surface->current;
		if (surface->ready)
			surface->pending = surface->queued;

		/* A metadata-only commit preserves the last committed, possibly queued content. */
		zwl_buffer_get(surface->pending);
	}

	/* Explicit unmap returns xdg-shell to its initial configure handshake state. */
	if (surface->attached && surface->pending == NULL) {
		surface->configured = 0;
		surface->acknowledged = 0;
		surface->configure_serial = 0;
	}

	/* Names the commit when the per-frame lines were asked for (with the image it replaces, 0 for none). */
	if (server->log_frames && attached && surface->pending != NULL) {
		replaced = 0U;
		if (surface->queued != NULL)
			replaced = surface->queued->id;
		printf("ZWL COMMIT client=%llu surface=%u buffer=%u queued=%u\n", (unsigned long long)surface->client->number, surface->id,
		       surface->pending->id, replaced);
	}

	/* New commits replace only an unpresented queued image; current scanout keeps its hold. */
	previous = surface->queued;
	surface->queued = surface->pending;
	surface->pending = NULL;
	surface->attached = 0;
	surface->ready = 1;
	server->commit_order++;
	surface->commit_order = server->commit_order;
	if (surface->queued != NULL)
		surface->queued->busy = 1;

	/* The damage goes with the commit, and so does its acquire fence. */
	commit_damage(surface);
	commit_fence(surface, attached);

	/* Dropped mailbox images are reusable once no other compositor use remains. */
	zwl_buffer_put(previous);
	append_callbacks(&surface->committed_callbacks, surface->callbacks);
	surface->callbacks = NULL;

	/* The window's sub-surfaces go with its state (subsurface.c). */
	zwl_subsurface_applied(surface);

	/* Succeeded: the scheduler owns the latest pending image and all frame callbacks. */
	return 0;
}

/*
 * Commits the pending state of a surface without a shell role now (a
 * cursor, a sub-surface, a surface waiting for its role): the attached
 * image, or the current one again, with its damage, acquire fences and
 * frame callbacks, for the scheduler.
 */
int
zwl_surface_queue(
	struct zwl_object *surface)
{
	struct zwl_object *previous;
	unsigned attached;

	/* The attached image, or the current one kept. */
	attached = surface->attached;
	previous = surface->queued;
	if (surface->attached) {
		surface->queued = surface->pending;
		surface->pending = NULL;
	} else {
		surface->queued = surface->current;
		zwl_buffer_get(surface->queued);
	}

	/* The content, its damage and its acquire fence are committed. */
	surface->attached = 0;
	surface->ready = 1;
	if (surface->queued != NULL)
		surface->queued->busy = 1;
	commit_damage(surface);
	commit_fence(surface, attached);
	zwl_buffer_put(previous);
	append_callbacks(&surface->committed_callbacks, surface->callbacks);
	surface->callbacks = NULL;

	/* Succeeded: the scheduler takes the committed image. */
	return 0;
}

/* Implements the selected fullscreen xdg-shell role and configure lifetime. */
static int
shell_request(
	struct zwl_object *object,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	struct zwl_object *surface;
	struct zwl_object *created;
	struct zwl_object *other;
	const char *text;
	size_t offset;
	uint32_t id;
	uint32_t parent;
	uint32_t positioner;
	uint32_t serial;
	int32_t width;
	int32_t height;
	int error;

	/* The global shell creates one xdg role for an existing role-free surface. */
	if (object->kind == ZWL_WM) {
		/*
		 * The binding may retire only when no live xdg_surface was made
		 * from it (xdg-shell's defunct_surfaces, BUG-112): xdg_surfaces of
		 * the client's other bindings (a library's own, a chooser's) do
		 * not keep it.
		 */
		if (opcode == 0 && size == 0) {
			/* Each live role of this binding still depends on it. */
			for (other = object->client->objects; other != NULL; other = other->next) {
				if (other->kind == ZWL_XDG_SURFACE &&
				    !other->dead &&
				    other->wm_base == object) {
					error = zwl_error_code(object->client, object->id, WM_ERROR_DEFUNCT_SURFACES, "xdg_surfaces of this binding live");
					return error;
				}
			}

			/* This binding no longer has live shell children. */
			zwl_object_destroy(object);
			return 0;
		}

		/* The answer to a ping (toplevel.c); one to no ping is ignored. */
		if (opcode == 3U && size == 4U) {
			serial = word_at(bytes, 0);
			zwl_ping_pong(object->client, serial);
			return 0;
		}

		/* A positioner, for the popups (popup.c). */
		if (opcode == 1U && size == 4U) {
			id = word_at(bytes, 0);
			error = zwl_positioner_create(object, id);
			if (error != 0)
				return error;
			return 0;
		}

		/* Only get_xdg_surface is left. */
		if (opcode != 2U || size != 8U)
			return EPROTO;

		/* The role cannot be attached to a foreign object or an already assigned surface. */
		id = word_at(bytes, 4);
		surface = zwl_find(object->client, id);
		if (surface == NULL ||
		    surface->kind != ZWL_SURFACE ||
		    surface->role != NULL ||
		    surface->sub_role != NULL)
			return EPROTO;

		/* Publish both directions only after the role identity is allocated. */
		id = word_at(bytes, 0);
		created = zwl_create(object->client, id, ZWL_XDG_SURFACE, object->version);
		if (created == NULL)
			return EPROTO;

		/* The surface owns no additional memory reference to its protocol role; the role remembers its binding. */
		created->surface = surface;
		created->wm_base = object;
		surface->role = created;
		return 0;
	}

	/* Shell objects cannot operate after their underlying surface disappears. */
	surface = object->surface;
	if (surface == NULL)
		return EPROTO;

	/* xdg_surface owns the configure serial and the sole toplevel child. */
	if (object->kind == ZWL_XDG_SURFACE) {
		/* Parent retirement cannot invalidate a surviving toplevel child. */
		if (opcode == 0 && size == 0 && object->top == NULL) {
			zwl_object_destroy(object);
			return 0;
		}

		/* Toplevel creation is one-shot while the child remains alive. */
		if (opcode == 1U && size == 4U && object->top == NULL) {
			/* Allocate the child before publishing either shell backreference. */
			id = word_at(bytes, 0);
			created = zwl_create(object->client, id, ZWL_TOPLEVEL, object->version);
			if (created == NULL)
				return EPROTO;

			/* Both shell layers refer to the same core surface. */
			created->surface = surface;
			created->role = object;
			object->top = created;
			return 0;
		}

		/* The popup role (popup.c): its id, a nullable parent xdg_surface and a positioner. */
		if (opcode == 2U && size == 12U) {
			id = word_at(bytes, 0);
			parent = word_at(bytes, 4);
			positioner = word_at(bytes, 8);
			error = zwl_popup_create(object, id, parent, positioner);
			if (error != 0)
				return error;
			return 0;
		}

		/* Window geometry is advisory, but its positive extent must be well formed. */
		if (opcode == 3U && size == 16U) {
			/* Geometry may be advisory, but its extent must remain strictly positive. */
			width = (int32_t)word_at(bytes, 8);
			height = (int32_t)word_at(bytes, 12);
			if (width <= 0 || height <= 0)
				return EPROTO;

			/* The geometry applies from the next commit (popups are placed from it). */
			surface->pending_geometry[0] = (int32_t)word_at(bytes, 0);
			surface->pending_geometry[1] = (int32_t)word_at(bytes, 4);
			surface->pending_geometry[2] = width;
			surface->pending_geometry[3] = height;
			surface->pending_geometry_set = 1;
			return 0;
		}

		/* An acknowledgment names a configure sent to this surface (the latest, or an earlier one). */
		if (opcode == 4U && size == 4U) {
			/* An old or foreign serial cannot authorize new buffer-bearing commits. */
			serial = word_at(bytes, 0);
			if (!surface->configured || serial == 0 || serial > surface->configure_serial)
				return EPROTO;

			/* Buffer-bearing commits may now publish this configured surface; a resize's end waits for this serial. */
			surface->acknowledged = 1;
			surface->acked_serial = serial;
			return 0;
		}

		/* No other xdg_surface request exists. */
		return EPROTO;
	}

	/* Toplevel methods validate their own payload before applying this fullscreen policy. */
	switch (opcode) {
	case 0:
		/* The child must retire before its xdg_surface parent can be destroyed. */
		if (size != 0)
			return EPROTO;

		/* Retire the child identity without destroying the underlying core surface. */
		zwl_object_destroy(object);
		break;
	case 2:
	case 3:
		/* Titles and application IDs are canonical strings. */
		error = string_at(bytes, size, 0, &text, &offset);
		if (error != 0 || offset != size)
			return EPROTO;

		/* The title is kept for the glass look's title bar (cut to fit); the application ID for the mark's letter. */
		if (opcode == 2U) {
			strncpy(surface->title, text, sizeof(surface->title) - 1U);
			surface->title[sizeof(surface->title) - 1U] = '\0';
			object->client->server->dirty = 1;
		} else {
			strncpy(surface->app_id, text, sizeof(surface->app_id) - 1U);
			surface->app_id[sizeof(surface->app_id) - 1U] = '\0';
			object->client->server->dirty = 1;
		}

		break;
	case 11:
		/* A fullscreen target is one nullable output identity. */
		if (size != 4U)
			return EPROTO;

		/* The default output needs no explicit proxy identity from this client. */
		id = word_at(bytes, 0);
		if (id != 0) {
			/* Explicit targets must refer to this connection's own output binding. */
			other = zwl_find(object->client, id);
			if (other == NULL || other->kind != ZWL_OUTPUT)
				return EPROTO;
		}

		/* The window becomes fullscreen: at the origin, the output's size (design D0, D6). */
		error = zwl_window_enter_fullscreen(surface);
		if (error != 0)
			return error;

		break;
	case 12:
		/* Leaving fullscreen has no payload. */
		if (size != 0)
			return EPROTO;

		/* The window returns to its place and size before fullscreen, or is centred (ws035-p138). */
		error = zwl_window_leave_fullscreen(surface);
		if (error != 0)
			return error;

		/* Succeeded: the window left fullscreen. */
		break;
	default:
		/* The requests of the window manager (toplevel.c): parent, window menu, move, resize, limits, maximize, minimize. */
		error = zwl_toplevel_request(object, surface, opcode, bytes, size);
		if (error != 0)
			return error;
		break;
	}

	/* Succeeded: the supported toplevel request is valid for this fullscreen role. */
	return 0;
}


/*
 * Tells the windows of xdg-shell version 4 new bounds when the space for
 * their bodies changed since the bounds were last sent (the glass look
 * given up when the output opened, another output size): each configured
 * window that is not fullscreen or docked hears configure_bounds and a
 * configure (in which it may choose its size again).
 */
void
zwl_window_bounds_refresh(
	struct zwl_server *server)
{
	struct zwl_client *client;
	struct zwl_object *surface;
	struct zwl_object *top;
	int32_t width;
	int32_t height;
	int error;

	/* Unchanged bounds, or none sent yet: nothing to tell. */
	window_bounds(server, &width, &height);
	if (server->bounds_width == 0 || (width == server->bounds_width && height == server->bounds_height))
		return;
	printf("ZWL BOUNDS changed width=%d height=%d was=%dx%d\n", width, height, server->bounds_width, server->bounds_height);
	server->bounds_width = width;
	server->bounds_height = height;

	/* Each configured window of version 4 that chooses its own size. */
	for (client = server->clients; client != NULL; client = client->next) {
		/* A failed client hears nothing. */
		if (client->fatal)
			continue;

		/* Each of its windows. */
		for (surface = client->objects; surface != NULL; surface = surface->next) {
			/* Only a configured toplevel's surface. */
			if (surface->kind != ZWL_SURFACE || surface->dead || surface->role == NULL || !surface->configured)
				continue;
			top = surface->role->top;
			if (top == NULL || top->kind != ZWL_TOPLEVEL || top->version < 4U)
				continue;

			/* Not a fullscreen or a docked one (their size is the compositor's). */
			if (surface->fullscreen || surface->maximized)
				continue;

			/* The bounds and a configure; a client that cannot take them is failed at its next request. */
			error = zwl_window_send_configure(surface);
			if (error != 0)
				printf("ZWL BOUNDS configure errno=%d\n", error);
		}
	}
}

/*
 * Makes a window fullscreen, when the client asks or the compositor decides
 * (the top-right corner's swipe, corner.c): it covers the output from the
 * origin, keeping its place and size to come back to, and a window already
 * configured is told now.  A fullscreen window is left as it is.  Returns 0,
 * or the error of sending the configure.
 */
int
zwl_window_enter_fullscreen(
	struct zwl_object *surface)
{
	int error;

	/* Already fullscreen: nothing changes. */
	if (surface->fullscreen)
		return 0;

	/* Its place and size before fullscreen, to come back to. */
	surface->fullscreen = 1;
	surface->window_x = surface->x;
	surface->window_y = surface->y;
	surface->window_width = 0;
	surface->window_height = 0;
	if (surface->current != NULL)
		zwl_surface_size(surface, &surface->window_width, &surface->window_height);

	/* It covers the output from the origin. */
	surface->x = 0;
	surface->y = 0;
	surface->client->server->dirty = 1;

	/* A window not configured yet learns it from its first configure. */
	if (!surface->configured)
		return 0;

	/* A window already configured is told now. */
	error = zwl_window_send_configure(surface);
	if (error != 0)
		return error;

	/* Succeeded: the window is fullscreen and knows it. */
	return 0;
}

/*
 * Takes a window out of fullscreen (the client asks, xdg_toplevel's
 * unset_fullscreen): back to the place and size it had as a window, kept
 * inside the space so that its title bar is not under the system bar; a
 * window that started fullscreen and was never placed is centred in the
 * space, now at its fullscreen size and again at its first image of
 * another size (ws035-p138, BUG-114).  A window already configured is told.
 * Returns 0, or the error of sending the configure.
 */
int
zwl_window_leave_fullscreen(
	struct zwl_object *surface)
{
	struct zwl_server *server;
	int error;

	/* Not fullscreen: nothing changes. */
	if (!surface->fullscreen)
		return 0;

	/* Back to its place, or the space's centre when it never had one. */
	server = surface->client->server;
	surface->fullscreen = 0;
	if (surface->placed) {
		surface->x = surface->window_x;
		surface->y = surface->window_y;
		if (server->glass && surface->window_width != 0U)
			zwl_glass_fit(server, (int32_t)surface->window_width, (int32_t)surface->window_height, &surface->x, &surface->y);
	} else {
		zwl_window_centre(server, surface);
		surface->place_pending = 1;
	}

	/* The output is drawn again; the log names where the window went. */
	server->dirty = 1;
	printf("ZWL WINDOW unfullscreen surface=%u x=%d y=%d placed=%u\n", surface->id, surface->x, surface->y, surface->placed);

	/* A window not configured yet learns it from its first configure. */
	if (!surface->configured)
		return 0;

	/* A window already configured is told now. */
	error = zwl_window_send_configure(surface);
	if (error != 0)
		return error;

	/* Succeeded: the window is a window again. */
	return 0;
}

/*
 * Sends a toplevel's configure: the output's size and the fullscreen and
 * activated states for a fullscreen window; otherwise its size before
 * fullscreen, or 0x0 (the client chooses), and activated.  The xdg_surface
 * configure with a new serial follows.
 */
int
zwl_window_send_configure(
	struct zwl_object *surface)
{
	struct zwl_server *server;
	struct zwl_object *role;
	uint32_t configure[5];
	size_t size;
	unsigned resizing;
	int error;

	/* The size and states; a window being resized (toplevel.c) has the resizing state too. */
	server = surface->client->server;
	role = surface->role;
	resizing = zwl_toplevel_resizing(server, surface);
	if (surface->fullscreen) {
		configure[0] = server->width;
		configure[1] = server->height;
		configure[2] = 8;
		configure[3] = 2;
		configure[4] = 4;
		size = 5U * sizeof(uint32_t);
	} else if (surface->maximized) {
		configure[0] = surface->window_width;
		configure[1] = surface->window_height;
		configure[2] = 8;
		configure[3] = 1;
		configure[4] = 4;
		size = 5U * sizeof(uint32_t);
	} else if (resizing) {
		configure[0] = surface->window_width;
		configure[1] = surface->window_height;
		configure[2] = 8;
		configure[3] = 3;
		configure[4] = 4;
		size = 5U * sizeof(uint32_t);
	} else {
		configure[0] = surface->window_width;
		configure[1] = surface->window_height;
		configure[2] = 4;
		configure[3] = 4;
		size = 4U * sizeof(uint32_t);
	}

	/* A window of xdg-shell version 4 learns first how large it may make itself. */
	if (!surface->fullscreen && !surface->maximized && role->top->version >= 4U) {
		error = send_bounds(surface);
		if (error != 0)
			return error;
	}

	/* The toplevel configure. */
	error = zwl_emit(surface->client, role->top->id, 0, configure, size);
	if (error != 0)
		return error;

	/* Nonzero serials distinguish an acknowledged configure from none. */
	server->serial++;
	if (server->serial == 0)
		server->serial++;

	/* The xdg_surface configure commits the toplevel state. */
	surface->configure_serial = server->serial;
	error = zwl_emit(surface->client, role->id, 0, &surface->configure_serial, 4);
	if (error != 0)
		return error;

	/* Succeeded. */
	printf("ZWL CONFIGURE client=%llu surface=%u serial=%u width=%u height=%u fullscreen=%u\n", (unsigned long long)surface->client->number, surface->id, surface->configure_serial, configure[0], configure[1], surface->fullscreen);
	return 0;
}

/*
 * Sends a window its bounds (xdg_toplevel.configure_bounds, version 4): the
 * largest size it should choose for itself, the space for window bodies in
 * the glass look (under the system bar and a floating title bar, shell.c)
 * and the output otherwise.  A client that chooses its own size keeps within
 * it, so that its window is seen whole.
 */
static int
send_bounds(
	struct zwl_object *surface)
{
	struct zwl_server *server;
	int32_t width;
	int32_t height;
	int32_t bounds[2];
	int error;

	/* The space the look leaves for a body. */
	server = surface->client->server;
	window_bounds(server, &width, &height);

	/* The width and the height, in that order. */
	bounds[0] = width;
	bounds[1] = height;
	error = zwl_emit(surface->client, surface->role->top->id, 2, bounds, sizeof(bounds));
	if (error != 0)
		return error;

	/* The bounds the windows know now. */
	server->bounds_width = width;
	server->bounds_height = height;

	/* Succeeded. */
	printf("ZWL BOUNDS client=%llu surface=%u width=%d height=%d\n", (unsigned long long)surface->client->number, surface->id, width, height);
	return 0;
}

/* Gives the space the look leaves for a window's body: under the system bar and a floating title bar in the glass look, the output otherwise. */
static void
window_bounds(
	struct zwl_server *server,
	int32_t *width,
	int32_t *height)
{
	/* The output. */
	*width = (int32_t)server->width;
	*height = (int32_t)server->height;

	/* Less the glass look's bars and margins (shell.c). */
	if (server->glass)
		zwl_glass_space(server, width, height);
}

/* Adds a rectangle to a surface's pending damage (their bounding box). */
static void
add_damage(
	struct zwl_object *surface,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height)
{
	/* An empty rectangle adds nothing. */
	if (width <= 0 || height <= 0)
		return;

	/* The first rectangle, or the box around both. */
	if (!surface->damaged) {
		surface->damage[0] = x;
		surface->damage[1] = y;
		surface->damage[2] = x + width;
		surface->damage[3] = y + height;
		surface->damaged = 1;
		return;
	}

	/* The box grows to hold the rectangle. */
	if (x < surface->damage[0])
		surface->damage[0] = x;
	if (y < surface->damage[1])
		surface->damage[1] = y;
	if (x + width > surface->damage[2])
		surface->damage[2] = x + width;
	if (y + height > surface->damage[3])
		surface->damage[3] = y + height;
}

/*
 * Moves a commit's acquire fences to the queued image.  A commit that
 * attached a buffer replaces the queued fences (with none when it gave
 * none); one that did not keeps the fences of the image it reuses.
 */
static void
commit_fence(
	struct zwl_object *surface,
	unsigned attached)
{
	unsigned index;

	/* Give the OS module the attached GPU buffer before moving commit fences. */
	if (attached &&
	    surface->queued != NULL &&
	    surface->queued->import != NULL &&
	    surface->queued->shm == NULL)
		zwl_gpu_commit(surface, surface->queued);

	/* The reused image still waits for its own fences. */
	if (!attached && surface->acquire_count == 0)
		return;

	/* The replaced image's fences are no longer waited for. */
	for (index = 0; index < surface->fence_count; index++)
		close(surface->fences[index].fd);

	/* The commit's fences are the queued image's. */
	for (index = 0; index < surface->acquire_count; index++)
		surface->fences[index] = surface->acquire[index];
	surface->fence_count = surface->acquire_count;
	surface->acquire_count = 0;
	surface->fence_ms = zwl_milliseconds();
	surface->fence_waited = 0;
}

/*
 * Moves a commit's damage to the committed damage, joined with any not yet
 * copied (two commits may come before one copy).
 */
static void
commit_damage(
	struct zwl_object *surface)
{
	/* No damage leaves the committed damage as it is. */
	if (!surface->damaged)
		return;

	/* The first, or the box around both. */
	if (!surface->committed_damaged) {
		memcpy(surface->committed_damage, surface->damage, sizeof(surface->damage));
		surface->committed_damaged = 1;
	} else {
		if (surface->damage[0] < surface->committed_damage[0])
			surface->committed_damage[0] = surface->damage[0];
		if (surface->damage[1] < surface->committed_damage[1])
			surface->committed_damage[1] = surface->damage[1];
		if (surface->damage[2] > surface->committed_damage[2])
			surface->committed_damage[2] = surface->damage[2];
		if (surface->damage[3] > surface->committed_damage[3])
			surface->committed_damage[3] = surface->damage[3];
	}

	/* The pending damage starts again. */
	surface->damaged = 0;
}



/* Preserves callback request order across pending-state commits and mailbox replacement. */
static void
append_callbacks(
	struct zwl_object **list,
	struct zwl_object *callbacks)
{
	/* Each existing callback must complete before later callbacks in the same surface stream. */
	while (*list != NULL)
		list = &(*list)->callback_next;

	/* Ownership of the supplied chain moves to this surface state. */
	*list = callbacks;

	/* Succeeded: the surface state owns the ordered callback chain. */
	return;
}
