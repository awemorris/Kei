/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Tests real native seat authority with observable renderer/input boundary collaborators. */
#include "userland/desktop/wayland/freebsd/seat-freebsd.h"
#include "userland/desktop/wayland/evdev/seat.h"
#include "userland/desktop/wayland/compose.h"
#include "userland/desktop/wayland/zwl-os.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Boundary recorders observe only ordering; no GPU, Vulkan frame or protocol notification is simulated. */
static unsigned withdrawal_step;

static int probe_refusal(struct zwl_server *server);
static int probe_lifetime(struct zwl_server *server);
static int await_pause(struct zwl_server *server, unsigned paused);
static int inspect_input(int descriptor);

/* Selects an actual-service refusal or native VT-notification lifetime probe. */
int
main(
	int argc,
	char **argv)
{
	struct zwl_server server;
	int same;
	int error;
	size_t count;

	/* The owned controller must supply the intended scenario explicitly. */
	if (argc != 2)
		return 1;

	/* No common window, client or GPU resource is created by this authority test. */
	memset(&server, 0, sizeof(server));
	same = strcmp(argv[1], "refusal");
	if (same == 0) {
		error = probe_refusal(&server);
	} else {
		error = probe_lifetime(&server);
	}

	/* Cleanup is idempotent after refusal, withdrawal, success and service loss. */
	zwl_os_close(&server);
	zwl_os_close(&server);
	if (error != 0) {
		(void)fprintf(stderr, "native backend probe errno=%d\n", error);
		return 1;
	}

	/* An absent client must contribute no service descriptor after repeated cleanup. */
	count = zwl_os_poll_count(&server);
	if (count != 0)
		return 1;

	/* Succeeded: this actual native authority scenario preserved its ownership contract. */
	(void)printf("PASS native backend %s\n", argv[1]);
	return 0;
}

/* Records frame retirement while the primary generation is already withdrawn. */
void
zwl_compose_quiesce(
	struct zwl_server *server)
{
	/* The real backend must publish pause before asking a renderer to retire a frame. */
	if (server->os_paused == 0 || withdrawal_step != 0) {
		server->failed = 1;
		return;
	}

	/* The recorder establishes the required predecessor for output retirement. */
	withdrawal_step = 1;

	/* Succeeded: ordering was observed; no physical frame-completion claim is made. */
	return;
}

/* Records display retirement before the backend returns input and primary files. */
void
zwl_compose_output_close(
	struct zwl_server *server)
{
	/* An attached kernel input file must still exist when output retirement is requested. */
	if (withdrawal_step != 1 || server->inputs[0].live == 0) {
		server->failed = 1;
		return;
	}

	/* The recorder establishes the required predecessor for input retirement. */
	withdrawal_step = 2;

	/* Succeeded: no native DRM device or display result is fabricated by this observation. */
	return;
}

/* Retires a real kernel lease while recording the common input-notification boundary. */
void
zwl_input_close(
	struct zwl_server *server,
	struct zwl_input_device *device)
{
	/* Common input notification must follow frame and display retirement. */
	if (withdrawal_step != 2) {
		server->failed = 1;
		return;
	}

	/* Returns the actual daemon-owned file using the production authority's release operation. */
	zwl_seat_device_close(server, device->fd);
	device->fd = -1;
	device->live = 0;
	withdrawal_step = 3;

	/* Succeeded: the kernel lease is closed, while only the common notification was represented. */
	return;
}

/* Verifies that a real missing primary cannot be reported as working native display authority. */
static int
probe_refusal(
	struct zwl_server *server)
{
	VkResult result;
	int error;
	int descriptor;

	/* QEMU has real evdev devices but no native DRM primary; the service must return ENOENT. */
	error = zwl_os_open(server);
	if (error != ENOENT)
		return EPROTO;

	/* A refused real primary cannot publish a valid descriptor. */
	descriptor = zwl_freebsd_primary_fd();
	if (descriptor != -1)
		return EPROTO;

	/* Display refusal occurs before accessing a nonexistent compositor Vulkan instance. */
	result = zwl_os_display_acquire(server, VK_NULL_HANDLE, VK_NULL_HANDLE);
	if (result != VK_ERROR_INITIALIZATION_FAILED)
		return EPROTO;

	/* Succeeded: the service and Vulkan boundary report absence rather than a successful stub. */
	return 0;
}

/* Uses the actual service for activation, device withdrawal, fresh acquisition and disconnection. */
static int
probe_lifetime(
	struct zwl_server *server)
{
	int descriptor;
	int extra;
	int error;
	char command;
	ssize_t bytes;
	struct pollfd notification;

	/* Connects without falsely requiring an absent DRM node for this input-authority test. */
	error = zwl_freebsd_seat_connect(server);
	if (error != 0)
		return error;

	/* Obtains a real native mouse lease through the production backend's flag and ownership checks. */
	descriptor = zwl_seat_device_open(server, "/dev/input/event1");
	if (descriptor < 0)
		return errno;

	/* Attaches one real file to the boundary recorder's common input slot. */
	server->inputs[0].fd = descriptor;
	server->inputs[0].live = 1;
	server->compose = (struct zwl_compose *)&withdrawal_step;
	error = inspect_input(descriptor);
	if (error != 0)
		return error;

	/* Also retains a real partial probe lease that has no common input record yet. */
	extra = zwl_seat_device_open(server, "/dev/input/event2");
	if (extra < 0)
		return errno;

	/* Asks the owned controller to switch the actual guest kernel's VT away from this session. */
	(void)puts("READY_PAUSE");
	(void)fflush(stdout);
	error = await_pause(server, 1);
	if (error != 0)
		return error;

	/* Both attached and partial-probe descriptors must be closed before the daemon can finish switching. */
	error = fcntl(descriptor, F_GETFD);
	if (error != -1 || errno != EBADF)
		return EPROTO;

	/* Partial probe ownership must be withdrawn even without a common input slot. */
	error = fcntl(extra, F_GETFD);
	if (error != -1 || errno != EBADF)
		return EPROTO;

	/* The daemon's completed VT switch proves acknowledgement followed local retirement. */
	if (withdrawal_step != 3 || server->inputs[0].live != 0)
		return EPROTO;

	/* Asks the owned controller to reactivate the original real guest VT. */
	(void)puts("PAUSED_RETIRED");
	(void)fflush(stdout);
	error = await_pause(server, 0);
	if (error != 0)
		return error;

	/* A new activation must schedule ordinary input discovery and output construction. */
	if (server->input_scan_time != 0 ||
	    server->dirty == 0 ||
	    server->windowed != 0)
		return EPROTO;

	/* Requests a fresh real file after activation rather than retaining a revoked handle. */
	descriptor = zwl_seat_device_open(server, "/dev/input/event1");
	if (descriptor < 0)
		return errno;

	/* Independent kernel metadata proves the reopened file is usable. */
	error = inspect_input(descriptor);
	zwl_seat_device_close(server, descriptor);
	if (error != 0)
		return error;

	/* Leaves a real independently owned file alive to verify local cleanup after service loss. */
	descriptor = zwl_seat_device_open(server, "/dev/input/event2");
	if (descriptor < 0)
		return errno;

	/* The controller kills only its own daemon while the client still owns a real input lease. */
	(void)puts("READY_DISCONNECT");
	(void)fflush(stdout);
	bytes = read(STDIN_FILENO, &command, sizeof(command));
	if (bytes != sizeof(command))
		return EIO;

	/* Loss of real authority marks the production service for ordinary teardown. */
	memset(&notification, 0, sizeof(notification));
	zwl_os_poll_fill(server, &notification);
	error = poll(&notification, 1, 1000);
	if (error <= 0)
		return ETIMEDOUT;

	/* Dispatches actual connection readiness after the owned daemon has exited. */
	zwl_os_poll_done(server, &notification);
	if (server->failed == 0)
		return EPROTO;

	/* Local fd cleanup must succeed even though protocol retirement can no longer reach the daemon. */
	zwl_os_close(server);
	error = fcntl(descriptor, F_GETFD);
	if (error != -1 || errno != EBADF)
		return EPROTO;

	/* Buffered shutdown callbacks cannot republish an active generation. */
	if (server->os_paused != 1)
		return EPROTO;

	/* Succeeded: real native authority withdrew, reopened and reported daemon loss. */
	return 0;
}

/* Binds callback delivery to a finite wait on the library's actual service descriptor. */
static int
await_pause(
	struct zwl_server *server,
	unsigned paused)
{
	struct pollfd notification;
	int attempt;
	int error;

	/* Observes real notifications without synthesizing an activation or disable callback. */
	for (attempt = 0; attempt < 100; attempt++) {
		memset(&notification, 0, sizeof(notification));
		zwl_os_poll_fill(server, &notification);
		error = poll(&notification, 1, 25);
		if (error < 0)
			return errno;

		/* Processes the production module's actual native notification and ownership operations. */
		zwl_os_poll_done(server, &notification);
		if (server->failed != 0)
			return EIO;

		/* Only the real daemon's callback can publish the desired generation state. */
		if (server->os_paused == paused)
			break;
	}

	/* A missing native notification is a bounded failure rather than an indefinite wait. */
	if (server->os_paused != paused)
		return ETIMEDOUT;

	/* Succeeded: the actual native service delivered the requested state change. */
	return 0;
}

/* Queries native kernel metadata and both event-loop fd guards on a real transferred input file. */
static int
inspect_input(
	int descriptor)
{
	char name[128];
	int flags;
	int error;

	/* Nonblocking status protects the shared compositor's event loop. */
	flags = fcntl(descriptor, F_GETFL);
	if (flags < 0)
		return errno;

	/* A readable native input file must never block another client's work. */
	if ((flags & O_NONBLOCK) == 0)
		return EPROTO;

	/* Inheritance protection is independent of event-loop status. */
	flags = fcntl(descriptor, F_GETFD);
	if (flags < 0)
		return errno;

	/* Compositor children cannot inherit a live seat-owned input file. */
	if ((flags & FD_CLOEXEC) == 0)
		return EPROTO;

	/* The actual native ioctl proves the reopened fd refers to a usable kernel input device. */
	memset(name, 0, sizeof(name));
	error = ioctl(descriptor, EVIOCGNAME(sizeof(name)), name);
	if (error < 0)
		return errno;

	/* An empty metadata answer is insufficient proof of usable device ownership. */
	if (name[0] == '\0')
		return EPROTO;

	/* Succeeded: the real file supports native evdev inquiries and both ownership guards. */
	return 0;
}
