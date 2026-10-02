/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Owns native seatd activation and real device leases without renderer or DRM driver code. */
#include "seat-freebsd.h"
#include "../evdev/seat.h"
#include "../zwl.h"
#include <errno.h>
#include <fcntl.h>
#include <libseat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Slot zero belongs to the primary node; all other slots belong to evdev readers. */
#define SEAT_LEASE_MAX (ZWL_INPUT_MAX + 1U)

/* One device has separate kernel-fd and daemon-device-ID ownership until release. */
struct seat_lease {
	int descriptor;
	int device;
	unsigned live;
};

/* The single compositor owns this client until ordinary or partial-startup cleanup. */
static struct libseat *seat_client;

/* Libseat borrows the listener pointer; its callback table must outlive every dispatch and final close. */
static struct libseat_seat_listener seat_listener;

/* Real kernel files and protocol IDs are published together and retired before daemon ACK. */
static struct seat_lease seat_leases[SEAT_LEASE_MAX];

/* A daemon-disabled seat cannot admit new device requests or retain revoked descriptors. */
static unsigned seat_paused = 1;

/* Pending library callbacks cannot reactivate resources once final cleanup has begun. */
static unsigned seat_closing;

/* Resume reopens the selected primary only after its first acquisition was requested. */
static unsigned primary_requested;

/* The exact absolute primary path stays fixed until the client's entire lifetime ends. */
static char primary_path[4096];

static int lease_open(struct zwl_server *server, unsigned slot, const char *path);
static void lease_close(struct zwl_server *server, unsigned slot);
static void seat_enable(struct libseat *seat, void *data);
static void seat_disable(struct libseat *seat, void *data);

/*
 * Connects the compositor to its real native seat authority before requesting any device.
 */
int
zwl_freebsd_seat_connect(
	struct zwl_server *server)
{
	const char *requested;
	size_t length;
	int error;
	int attempt;

	/* Refuses another live owner instead of discarding its device leases. */
	if (seat_client != NULL)
		return EBUSY;

	/* Keeps the native primary choice consistent with the later Vulkan inquiry. */
	requested = getenv("KEILAND_DRM_DEVICE");
	if (requested == NULL)
		requested = "/dev/dri/card0";

	/* A relative or truncated choice cannot establish authoritative device identity. */
	length = strlen(requested);
	if (requested[0] != '/' || length >= sizeof(primary_path))
		return EINVAL;

	/* Publishes the fixed device choice before any daemon activation can reopen it. */
	memcpy(primary_path, requested, length + 1);
	primary_requested = 0;
	seat_closing = 0;
	seat_paused = 1;
	server->os_paused = 1;

	/* Only the verified MIT seatd provider may supply native authority; noop is never selected. */
	error = setenv("LIBSEAT_BACKEND", "seatd", 1);
	if (error != 0)
		return errno;

	/* Callback storage is borrowed until this client's final close has returned. */
	seat_listener.enable_seat = seat_enable;
	seat_listener.disable_seat = seat_disable;
	seat_client = libseat_open_seat(&seat_listener, server);
	if (seat_client == NULL)
		return errno;

	/* Gives startup activation a bounded wait while retaining its real service descriptor. */
	for (attempt = 0; attempt < 100; attempt++) {
		error = libseat_dispatch(seat_client, 20);
		if (error < 0)
			return errno;

		/* A native activation callback is the only proof that device access is available. */
		if (seat_paused == 0)
			break;
	}

	/* An inactive seat cannot silently proceed with root device access. */
	if (seat_paused != 0)
		return ETIMEDOUT;

	/* Succeeded: the native service owns this compositor's activated session. */
	return 0;
}

/*
 * Retires every surviving device owner before returning the native seat to its daemon.
 */
void
zwl_freebsd_seat_close(
	struct zwl_server *server)
{
	unsigned slot;
	int error;

	/* Cleanup cannot admit a new input reader while its leases are being returned. */
	seat_paused = 1;
	server->os_paused = 1;
	primary_requested = 0;
	seat_closing = 1;

	/* Partial startup and already retired inputs leave only explicitly live owners. */
	for (slot = 0; slot < SEAT_LEASE_MAX; slot++)
		lease_close(server, slot);

	/* The service client is independently owned even when primary acquisition failed. */
	if (seat_client != NULL) {
		error = libseat_close_seat(seat_client);
		seat_client = NULL;
		if (error != 0)
			server->failed = 1;
	}

	/* Succeeded: repeated cleanup has no remaining file or protocol owner. */
	return;
}

/*
 * Takes a real primary-node lease before Vulkan opens its independent inquiry file.
 */
int
zwl_freebsd_primary_open(
	struct zwl_server *server)
{
	int error;

	/* A paused or absent authority supplies no display ownership. */
	if (seat_client == NULL || seat_paused != 0)
		return EACCES;

	/* Resume must acquire a fresh primary lease before unpausing the common event loop. */
	primary_requested = 1;
	error = lease_open(server, 0, primary_path);
	if (error != 0)
		return error;

	/* Succeeded: Vulkan may now duplicate the native service's primary file. */
	return 0;
}

/*
 * Supplies the native primary file without transferring its original owner.
 */
int
zwl_freebsd_primary_fd(
	void)
{
	/* An absent lease is a refusal, never an arbitrary valid descriptor. */
	if (seat_leases[0].live == 0)
		return -1;

	/* Succeeded: Vulkan can borrow this live primary-node descriptor. */
	return seat_leases[0].descriptor;
}

/*
 * Supplies the fixed native primary choice used by both the seat and Vulkan.
 */
const char *
zwl_freebsd_primary_path(
	void)
{
	/* Succeeded: the pathname remains stable throughout the service client's lifetime. */
	return primary_path;
}

/*
 * Supplies the daemon connection even while device access is withdrawn.
 */
int
zwl_freebsd_seat_poll_fd(
	void)
{
	int descriptor;

	/* Partial startup may never have created a client connection. */
	if (seat_client == NULL)
		return -1;

	/* Uses the library's own readiness contract rather than inspecting its private connection. */
	descriptor = libseat_get_fd(seat_client);
	if (descriptor < 0)
		return -1;

	/* Succeeded: activation and withdrawal notifications remain pollable. */
	return descriptor;
}

/*
 * Processes queued native seat notifications without delaying the compositor's event loop.
 */
int
zwl_freebsd_seat_dispatch(
	struct zwl_server *server)
{
	int error;

	/* An absent authority cannot sustain a running compositor. */
	if (seat_client == NULL)
		return ENOTCONN;

	/* Dispatch also delivers notifications already buffered by synchronous device requests. */
	error = libseat_dispatch(seat_client, 0);
	if (error < 0)
		return errno;

	/* A failed callback has already withdrawn access and requires ordinary service teardown. */
	if (server->failed != 0)
		return EIO;

	/* Succeeded: the shared loop observes the latest native device generation. */
	return 0;
}

/*
 * Obtains an independently tracked nonblocking evdev lease from the native daemon.
 */
int
zwl_seat_device_open(
	struct zwl_server *server,
	const char *path)
{
	unsigned slot;
	int error;

	/* Device discovery cannot bypass a withdrawal or a disconnected authority. */
	if (seat_client == NULL || seat_paused != 0) {
		errno = EACCES;
		return -1;
	}

	/* Finds a free input slot without replacing another reader's file. */
	for (slot = 1; slot < SEAT_LEASE_MAX; slot++) {
		/* A free slot owns neither a kernel file nor a daemon device ID. */
		if (seat_leases[slot].live == 0)
			break;
	}

	/* Exhaustion cannot discard an existing input lease. */
	if (slot == SEAT_LEASE_MAX) {
		errno = EMFILE;
		return -1;
	}

	/* The same owner checks flags and preserves the daemon's native error on refusal. */
	error = lease_open(server, slot, path);
	if (error != 0) {
		errno = error;
		return -1;
	}

	/* Succeeded: the shared evdev reader receives its own live nonblocking descriptor. */
	return seat_leases[slot].descriptor;
}

/*
 * Returns an input file and its protocol owner to the same native seat.
 */
void
zwl_seat_device_close(
	struct zwl_server *server,
	int descriptor)
{
	unsigned slot;

	/* Only a file explicitly published by this authority can retire one of its leases. */
	for (slot = 1; slot < SEAT_LEASE_MAX; slot++) {
		/* Empty slots cannot match a previously closed and reused descriptor number. */
		if (seat_leases[slot].live == 0)
			continue;

		/* The original native owner retires both resources together. */
		if (seat_leases[slot].descriptor == descriptor) {
			lease_close(server, slot);
			return;
		}
	}

	/* Succeeded: no live lease matches this already retired or foreign descriptor. */
	return;
}

/*
 * Reports whether shared evdev discovery must wait for native activation.
 */
int
zwl_seat_paused(
	void)
{
	/* A withdrawn authority forbids every new input device request. */
	if (seat_paused != 0)
		return 1;

	/* Succeeded: the native daemon currently permits input device access. */
	return 0;
}

/*
 * Lets ordinary input retirement close a revoked lease before the daemon's later notification.
 */
int
zwl_seat_device_revoked(
	struct zwl_server *server,
	int descriptor)
{
	/* Seatd requires fresh device opens on activation, so retaining a dead reader gains nothing. */
	(void)server;
	(void)descriptor;

	/* Succeeded: common input cleanup may retire this file immediately. */
	return 0;
}

/* Publishes a device owner only after the real daemon request and both fd guards succeed. */
static int
lease_open(
	struct zwl_server *server,
	unsigned slot,
	const char *path)
{
	int descriptor;
	int device;
	int flags;
	int error;

	/* Refuses to replace a live primary or input owner. */
	if (seat_leases[slot].live != 0)
		return EBUSY;

	/* Asks the native authority to open the kernel device and transfer its descriptor. */
	descriptor = -1;
	device = libseat_open_device(seat_client, path, &descriptor);
	if (device < 0)
		return errno;

	/* Retains both owners immediately so every partial flag failure can release them. */
	seat_leases[slot].descriptor = descriptor;
	seat_leases[slot].device = device;
	seat_leases[slot].live = 1;

	/* Reads the file status before adding nonblocking event-loop behavior. */
	flags = fcntl(descriptor, F_GETFL);
	if (flags < 0) {
		error = errno;
		lease_close(server, slot);
		return error;
	}

	/* A transferred file must not block the shared input reader. */
	error = fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
	if (error != 0) {
		error = errno;
		lease_close(server, slot);
		return error;
	}

	/* Reads the independent inheritance flags before preventing child-process leakage. */
	flags = fcntl(descriptor, F_GETFD);
	if (flags < 0) {
		error = errno;
		lease_close(server, slot);
		return error;
	}

	/* A compositor child cannot inherit any native seat file. */
	error = fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC);
	if (error != 0) {
		error = errno;
		lease_close(server, slot);
		return error;
	}

	/* Succeeded: the lease now satisfies shared evdev and primary ownership requirements. */
	return 0;
}

/* Retires the local file independently of the daemon's acknowledgement of its device ID. */
static void
lease_close(
	struct zwl_server *server,
	unsigned slot)
{
	int descriptor;
	int device;
	int error;

	/* Repeated shutdown and native input notifications cannot release an owner twice. */
	if (seat_leases[slot].live == 0)
		return;

	/* Withdraws publication before another callback can observe a retired lease. */
	descriptor = seat_leases[slot].descriptor;
	device = seat_leases[slot].device;
	seat_leases[slot].live = 0;
	seat_leases[slot].descriptor = -1;

	/* Kernel-file ownership ends even when the service has already disconnected. */
	error = close(descriptor);
	if (error != 0)
		server->failed = 1;

	/* Protocol ownership is independently returned, with refusal forcing normal shutdown. */
	if (seat_client != NULL) {
		error = libseat_close_device(seat_client, device);
		if (error != 0)
			server->failed = 1;
	}

	/* Succeeded: no local descriptor remains published by this lease. */
	return;
}

/* Reacquires withdrawn primary ownership before allowing shared drawing and discovery to resume. */
static void
seat_enable(
	struct libseat *seat,
	void *data)
{
	struct zwl_server *server;
	int error;

	/* Initial activation has no primary request; later activation must reopen the same real node. */
	(void)seat;
	server = data;

	/* Final client close may drain pending callbacks after common display and input teardown. */
	if (seat_closing != 0)
		return;

	/* Only a live session may reacquire the primary file for its next display generation. */
	if (primary_requested != 0) {
		error = lease_open(server, 0, primary_path);
		if (error != 0) {
			server->failed = 1;
			return;
		}
	}

	/* The common loop discovers fresh input handles and opens output from its ordinary scheduler. */
	seat_paused = 0;
	server->os_paused = 0;
	server->input_scan_time = 0;
	server->windowed = 0;
	server->dirty = 1;

	/* Succeeded: only newly acquired native device ownership is available after activation. */
	return;
}

/* Withdraws display and input owners before acknowledging the native daemon's disable request. */
static void
seat_disable(
	struct libseat *seat,
	void *data)
{
	struct zwl_server *server;
	unsigned slot;
	int error;

	/* A common event-loop snapshot must stop drawing and opening new input devices immediately. */
	server = data;

	/* Final session close owns retirement and must not repeat common renderer or input teardown. */
	if (seat_closing != 0)
		return;

	/* Native withdrawal stops common drawing and device discovery before retiring any file. */
	seat_paused = 1;
	server->os_paused = 1;

	/* Retires the in-flight frame and Vulkan's primary duplicate before returning the original lease. */
	if (server->compose != NULL) {
		zwl_compose_quiesce(server);
		zwl_compose_output_close(server);
	}

	/* The next activation's ordinary scheduler will create a new output rather than reuse old state. */
	server->windowed = 0;

	/* Common input teardown withdraws tablet/touch state and capability publication with each fd. */
	for (slot = 0; slot < ZWL_INPUT_MAX; slot++) {
		/* Only attached live input records need common protocol notifications. */
		if (server->inputs[slot].live != 0)
			zwl_input_close(server, &server->inputs[slot]);
	}

	/* Also retires any partial probe lease not yet attached to a common input record. */
	for (slot = 1; slot < SEAT_LEASE_MAX; slot++)
		lease_close(server, slot);

	/* Primary ownership returns only after Vulkan's output duplicate has been released. */
	lease_close(server, 0);

	/* The daemon may switch ownership only after this compositor has stopped using every file. */
	error = libseat_disable_seat(seat);
	if (error != 0)
		server->failed = 1;

	/* Succeeded: no revoked file survives into a later activation generation. */
	return;
}
