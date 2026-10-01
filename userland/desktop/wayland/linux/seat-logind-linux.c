/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Keeps logind device leases distinct from each compositor input descriptor. */
#include "seat-linux.h"
#include "dbus-linux.h"
#include "../zwl.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>

#define LOGIND_DEVICES (ZWL_INPUT_MAX + 1U)

/* One device holds exactly one TakeDevice lease until release or seat cleanup. */
struct logind_device {
	unsigned owned;
	unsigned paused;
	unsigned display;
	uint32_t major;
	uint32_t minor;
	int fd;
	char path[4096];
};

/* The single compositor process owns this bus until OS cleanup. */
static struct linux_dbus seat_bus = {-1, 0, NULL, 0, 0, {NULL}, 0};

/* Device leases stay live while paused; slot zero is reserved for the primary node. */
static struct logind_device seat_devices[LOGIND_DEVICES];

/* The actual escaped path returned by GetSession determines every later call and match. */
static char seat_session[4096];

/* Only signals from the resolved unique logind owner may change device authority. */
static char seat_owner[256];

/* Control is returned only when TakeControl succeeded, including partial startup. */
static unsigned seat_control;

/* A paused primary node suppresses common composition and input discovery. */
static unsigned seat_paused;

static int seat_call(const char *member, const char *signature, const struct dbus_arg *args, size_t count, struct dbus_reply **reply);
static int seat_device_call(const char *member, struct logind_device *device);
static int seat_take(struct logind_device *device, const char *path, unsigned display);
static void seat_release(struct logind_device *device);
static struct zwl_input_device *seat_input(struct zwl_server *server, struct logind_device *device);
static int seat_signal(struct dbus_reply *message, void *data);
static int seat_pause(struct zwl_server *server, struct logind_device *device, const char *kind);
static int seat_resume(struct zwl_server *server, struct logind_device *device, struct dbus_reply *message, uint32_t index);

/*
 * Takes session control and the primary device before Vulkan inquiry begins.
 */
int
zwl_linux_logind_seat_open(
	struct zwl_server *server)
{
	struct dbus_arg argument;
	struct dbus_reply *reply;
	const char *session;
	const char *value;
	const char *path;
	char match[4608];
	size_t length;
	int error;
	int same;
	int written;

	/* Session metadata must identify the caller's existing login session. */
	session = getenv("XDG_SESSION_ID");
	if (session == NULL || session[0] == '\0')
		return EINVAL;
	error = dbus_open_system(&seat_bus);
	if (error != 0)
		return error;

	/* Resolve service authority once; arbitrary unicast signals cannot impersonate it. */
	memset(&argument, 0, sizeof(argument));
	argument.string = "org.freedesktop.login1";
	error = dbus_call(&seat_bus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "GetNameOwner", "s", &argument, 1, &reply);
	if (error != 0)
		return error;
	same = strcmp(reply->signature, "s");
	error = EPROTO;
	if (same == 0)
		error = dbus_read_string(reply, &value);
	if (error != 0) {
		dbus_reply_free(reply);
		return error;
	}

	/* Store the unique owner before releasing borrowed message storage. */
	length = strlen(value);
	if (length >= sizeof(seat_owner)) {
		dbus_reply_free(reply);
		return EOVERFLOW;
	}

	/* Retains the validated device or session value before its next use. */
	memcpy(seat_owner, value, length + 1);
	dbus_reply_free(reply);

	/* GetSession returns the exact escaped path; self and auto are not signal paths. */
	argument.string = session;
	error = dbus_call(&seat_bus, "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "GetSession", "s", &argument, 1, &reply);
	if (error != 0)
		return error;
	same = strcmp(reply->signature, "o");
	error = EPROTO;
	if (same == 0)
		error = dbus_read_string(reply, &value);
	if (error != 0) {
		dbus_reply_free(reply);
		return error;
	}

	/* A truncated session path must never redirect later device calls. */
	length = strlen(value);
	if (length >= sizeof(seat_session)) {
		dbus_reply_free(reply);
		return EOVERFLOW;
	}

	/* Retains the validated device or session value before its next use. */
	memcpy(seat_session, value, length + 1);
	dbus_reply_free(reply);

	/* Subscribe before control so no pause can be missed during startup. */
	written = snprintf(match, sizeof(match), "type='signal',sender='org.freedesktop.login1',interface='org.freedesktop.login1.Session',path='%s'", seat_session);
	if (written < 0 || (size_t)written >= sizeof(match))
		return EOVERFLOW;
	error = dbus_add_match(&seat_bus, match);
	if (error != 0)
		return error;
	argument.number = 0;
	error = seat_call("TakeControl", "b", &argument, 1, &reply);
	if (error != 0)
		return error;
	dbus_reply_free(reply);
	seat_control = 1;

	/* The selected primary path remains authoritative for the Vulkan backend as well. */
	path = getenv("KEILAND_DRM_DEVICE");
	if (path == NULL)
		path = "/dev/dri/card0";
	error = seat_take(&seat_devices[0], path, 1);
	if (error != 0)
		return error;
	seat_paused = seat_devices[0].paused;
	server->os_paused = seat_paused;
	printf("ZWL SEAT logind session=%s\n", seat_session);

	/* Succeeded: logind, rather than direct open, owns device authority. */
	return 0;
}

/*
 * Returns all device leases and session control, including paused descriptors.
 */
void
zwl_linux_logind_seat_close(
	struct zwl_server *server)
{
	struct dbus_reply *reply;
	unsigned index;
	int error;

	/* Common input cleanup may have skipped negative paused descriptors. */
	for (index = 0; index < LOGIND_DEVICES; index++)
		seat_release(&seat_devices[index]);

	/* Control is relinquished only if this process successfully took it. */
	if (seat_control != 0) {
		error = seat_call("ReleaseControl", "", NULL, 0, &reply);
		if (error == 0)
			dbus_reply_free(reply);
		seat_control = 0;
	}

	/* Losing the bus also returns server-side leases after any transport failure. */
	dbus_close(&seat_bus);
	seat_paused = 0;
	server->os_paused = 0;

	/* Succeeded: no session authority survives this compositor. */
	return;
}

/*
 * Acquires one active input lease for the common evdev classifier.
 */
int
zwl_linux_logind_device_open(
	struct zwl_server *server,
	const char *path)
{
	unsigned index;
	int error;

	/* Input discovery is postponed until the primary seat is active again. */
	(void)server;
	if (seat_paused != 0) {
		errno = EAGAIN;
		return -1;
	}

	/* The common live-path table prevents a second take of an already owned node. */
	for (index = 1; index < LOGIND_DEVICES; index++) {
		if (seat_devices[index].owned == 0)
			break;
	}

	/* The ownership table is bounded by the common input capacity. */
	if (index == LOGIND_DEVICES) {
		errno = EMFILE;
		return -1;
	}

	/* Retains the validated device or session value before its next use. */
	error = seat_take(&seat_devices[index], path, 0);
	if (error != 0) {
		errno = error;
		return -1;
	}

	/* An inactive new node cannot be probed; release before a later scan retries it. */
	if (seat_devices[index].paused != 0) {
		seat_release(&seat_devices[index]);
		errno = EAGAIN;
		return -1;
	}

	/* Succeeded: the common record will borrow this service-owned descriptor. */
	return seat_devices[index].fd;
}

/*
 * Releases the lease corresponding to one input descriptor.
 */
void
zwl_linux_logind_device_close(
	struct zwl_server *server,
	int descriptor)
{
	unsigned index;

	/* A paused common record has no descriptor; final seat cleanup owns its lease. */
	(void)server;
	if (descriptor < 0)
		return;

	/* Only the exact owned descriptor can release a service lease. */
	for (index = 1; index < LOGIND_DEVICES; index++) {
		if (seat_devices[index].owned != 0 && seat_devices[index].fd == descriptor) {
			seat_release(&seat_devices[index]);
			return;
		}
	}

	/* A descriptor outside the ownership table must not be closed speculatively. */
	return;
}

/*
 * Supplies the current logind primary descriptor for Vulkan acquisition.
 */
int
zwl_linux_logind_drm_fd(
	void)
{
	/* An absent lease cannot authorize a display acquisition. */
	if (seat_devices[0].owned == 0)
		return -1;

	/* Succeeded: Vulkan may duplicate this active service-owned file. */
	return seat_devices[0].fd;
}

/*
 * Supplies the selected primary-node path for the compatibility library.
 */
const char *
zwl_linux_logind_drm_path(
	void)
{
	/* Succeeded: the pathname remains stable across fd replacement on resume. */
	return seat_devices[0].path;
}

/*
 * Reports whether the primary seat is paused.
 */
int
zwl_linux_logind_seat_paused(
	void)
{
	/* Succeeded: common input discovery observes the same pause as composition. */
	return (int)seat_paused;
}

/*
 * Supplies the authenticated system-bus socket to the OS poll range.
 */
int
zwl_linux_logind_poll_fd(
	void)
{
	int descriptor;

	/* The bus remains polled while the primary device itself is paused. */
	descriptor = dbus_fd(&seat_bus);

	/* Succeeded: resume signals can wake the common event loop. */
	return descriptor;
}

/*
 * Applies queued and newly arrived device signals before common input reads.
 */
int
zwl_linux_logind_dispatch(
	struct zwl_server *server)
{
	int error;

	/* Device callbacks may synchronously acknowledge pause while retaining later signals. */
	error = dbus_dispatch(&seat_bus, seat_signal, server);
	if (error != 0)
		return error;

	/* Succeeded: the seat's descriptor generations agree with current authority. */
	return 0;
}

/*
 * Excludes a revoked input file while preserving its lease for the pending signal.
 */
int
zwl_linux_logind_device_revoked(
	struct zwl_server *server,
	int descriptor)
{
	struct logind_device *device;
	struct zwl_input_device *input;
	unsigned index;

	/* Only this seat's exact current input descriptor can retain service ownership. */
	for (index = 1; index < LOGIND_DEVICES; index++) {
		device = &seat_devices[index];
		if (device->owned == 0 || device->fd != descriptor)
			continue;
		device->paused = 1;
		input = seat_input(server, device);
		if (input != NULL)
			input->fd = -1;
		printf("ZWL SEAT input_revoked device=%u:%u lease=retained\n", device->major, device->minor);
		return 1;
	}

	/* An unknown descriptor cannot acquire logind ownership by reporting an error. */
	return 0;
}

/* Calls the exact session object without rederiving its escaped path. */
static int
seat_call(
	const char *member,
	const char *signature,
	const struct dbus_arg *args,
	size_t count,
	struct dbus_reply **reply)
{
	int error;

	/* Every device operation is scoped to the session whose control we own. */
	error = dbus_call(&seat_bus, "org.freedesktop.login1", seat_session, "org.freedesktop.login1.Session", member, signature, args, count, reply);
	if (error != 0)
		return error;

	/* Succeeded: the caller owns the method response. */
	return 0;
}

/* Sends a lease transition naming the device's immutable major and minor. */
static int
seat_device_call(
	const char *member,
	struct logind_device *device)
{
	struct dbus_arg args[2];
	struct dbus_reply *reply;
	int error;

	/* The service identity is independent of the current, replaceable descriptor. */
	memset(args, 0, sizeof(args));
	args[0].number = device->major;
	args[1].number = device->minor;
	error = seat_call(member, "uu", args, 2, &reply);
	if (error != 0)
		return error;
	dbus_reply_free(reply);

	/* Succeeded: logind has accepted this device transition. */
	return 0;
}

/* Takes a device once and preserves its service identity for later release. */
static int
seat_take(
	struct logind_device *device,
	const char *path,
	unsigned display)
{
	struct stat status;
	struct dbus_arg args[2];
	struct dbus_reply *reply;
	uint32_t index;
	uint32_t inactive;
	size_t length;
	int descriptor;
	int flags;
	int character;
	int error;
	int same;

	/* The path must fit intact and designate a character device. */
	length = strlen(path);
	if (length >= sizeof(device->path) || path[0] != '/')
		return EINVAL;
	error = stat(path, &status);
	if (error != 0)
		return errno;
	character = S_ISCHR(status.st_mode);
	if (character == 0)
		return ENODEV;

	/* No device is opened directly; logind supplies its owned file. */
	memset(args, 0, sizeof(args));
	args[0].number = (uint32_t)major(status.st_rdev);
	args[1].number = (uint32_t)minor(status.st_rdev);
	error = seat_call("TakeDevice", "uu", args, 2, &reply);
	if (error != 0)
		return error;

	/* A successful method owns a lease even if its response is malformed. */
	memset(device, 0, sizeof(*device));
	device->fd = -1;
	device->owned = 1;
	device->display = display;
	device->major = args[0].number;
	device->minor = args[1].number;
	memcpy(device->path, path, length + 1);
	same = strcmp(reply->signature, "hb");
	error = EPROTO;
	if (same == 0)
		error = dbus_read_number(reply, &index);
	if (error != 0) {
		dbus_reply_free(reply);
		seat_release(device);
		return error;
	}

	/* The active flag is a D-Bus Boolean, never an arbitrary integer. */
	error = dbus_read_number(reply, &inactive);
	if (error != 0 ||
	    inactive > 1 ||
	    reply->cursor != reply->size) {
		dbus_reply_free(reply);
		seat_release(device);
		return EPROTO;
	}

	/* Take the indexed fd before releasing the message's remaining rights. */
	descriptor = dbus_take_fd(reply, index);
	dbus_reply_free(reply);
	if (descriptor < 0) {
		seat_release(device);
		return EPROTO;
	}

	/* Retains the validated device or session value before its next use. */
	device->fd = descriptor;
	device->paused = inactive;

	/* Every input read remains nonblocking after transfer from the service. */
	flags = fcntl(descriptor, F_GETFL);
	if (flags < 0) {
		error = errno;
		seat_release(device);
		return error;
	}

	/* Retains the validated device or session value before its next use. */
	error = fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
	if (error != 0) {
		error = errno;
		seat_release(device);
		return error;
	}

	/* Succeeded: this slot owns exactly one service lease and descriptor. */
	return 0;
}

/* Returns a taken lease exactly once even when the transport has failed. */
static void
seat_release(
	struct logind_device *device)
{
	int error;

	/* Unallocated and already released slots have no service ownership. */
	if (device->owned == 0)
		return;

	/* Closing the local file never substitutes for returning a live service lease. */
	if (seat_bus.fd >= 0) {
		error = seat_device_call("ReleaseDevice", device);
		if (error != 0)
			fprintf(stderr, "wayland: logind release %u:%u errno=%d\n", device->major, device->minor, error);
	}

	/* A disconnected bus has already returned service control; local rights still close here. */
	if (device->fd >= 0)
		(void)close(device->fd);
	memset(device, 0, sizeof(*device));
	device->fd = -1;

	/* Succeeded: repeated cleanup cannot return or close this lease again. */
	return;
}

/* Finds the stable common input slot even while its fd is excluded from polling. */
static struct zwl_input_device *
seat_input(
	struct zwl_server *server,
	struct logind_device *device)
{
	unsigned index;
	int same;

	/* Path identity survives paused and resumed descriptor generations. */
	for (index = 0; index < ZWL_INPUT_MAX; index++) {
		if (server->inputs[index].live == 0)
			continue;
		same = strcmp(server->inputs[index].path, device->path);
		if (same == 0)
			return &server->inputs[index];
	}

	/* A node rejected by classification owns no common input slot. */
	return NULL;
}

/* Validates signal authority and routes transitions only to known leases. */
static int
seat_signal(
	struct dbus_reply *message,
	void *data)
{
	struct zwl_server *server;
	struct logind_device *device;
	const char *kind;
	uint32_t device_major;
	uint32_t device_minor;
	uint32_t index;
	unsigned slot;
	int same;
	int pause;
	int error;

	/* Other daemon signals are harmless and never affect device ownership. */
	server = data;
	if (message->sender == NULL)
		return 0;
	same = strcmp(message->sender, seat_owner);
	if (same != 0)
		return 0;
	same = strcmp(message->path, seat_session);
	if (same != 0)
		return 0;
	same = strcmp(message->interface, "org.freedesktop.login1.Session");
	if (same != 0)
		return 0;
	same = strcmp(message->member, "PauseDevice");
	pause = 0;
	if (same == 0) {
		pause = 1;
		same = strcmp(message->signature, "uus");
	} else {
		same = strcmp(message->member, "ResumeDevice");
		if (same != 0)
			return 0;
		same = strcmp(message->signature, "uuh");
	}

	/* A recognized device signal must carry its exact argument representation. */
	if (same != 0)
		return EPROTO;
	error = dbus_read_number(message, &device_major);
	if (error != 0)
		return error;
	error = dbus_read_number(message, &device_minor);
	if (error != 0)
		return error;

	/* A released or unsupported device cannot acquire a new lease through a signal. */
	device = NULL;
	for (slot = 0; slot < LOGIND_DEVICES; slot++) {
		if (seat_devices[slot].owned != 0 &&
		    seat_devices[slot].major == device_major &&
		    seat_devices[slot].minor == device_minor) {
			device = &seat_devices[slot];
			break;
		}
	}

	/* The message destructor closes unused resume descriptors for absent leases. */
	if (device == NULL)
		return 0;
	if (pause != 0) {
		error = dbus_read_string(message, &kind);
		if (error != 0)
			return error;
		if (message->cursor != message->size)
			return EPROTO;
		error = seat_pause(server, device, kind);
		if (error != 0)
			return error;
	} else {
		error = dbus_read_number(message, &index);
		if (error != 0)
			return error;
		if (message->cursor != message->size)
			return EPROTO;
		error = seat_resume(server, device, message, index);
		if (error != 0)
			return error;
	}

	/* Succeeded: the device transition and common state agree. */
	return 0;
}

/* Stops device use before acknowledging a cooperative pause. */
static int
seat_pause(
	struct zwl_server *server,
	struct logind_device *device,
	const char *kind)
{
	struct zwl_input_device *input;
	int cooperative;
	int gone;
	int same;
	int error;

	/* Only the three logind pause kinds have defined ownership semantics. */
	cooperative = 0;
	gone = 0;
	same = strcmp(kind, "pause");
	if (same == 0) {
		cooperative = 1;
	} else {
		same = strcmp(kind, "gone");
		if (same == 0) {
			gone = 1;
		} else {
			same = strcmp(kind, "force");
			if (same != 0)
				return EPROTO;
		}
	}

	/* Pause excludes descriptors but retains the TakeDevice lease for resume. */
	device->paused = 1;
	input = NULL;
	if (device->display != 0) {
		seat_paused = 1;
		server->os_paused = 1;
		zwl_compose_quiesce(server);
		zwl_compose_output_close(server);
		server->windowed = 0;
	} else {
		input = seat_input(server, device);
		if (input != NULL)
			input->fd = -1;
	}

	/* The log records actual notification kind rather than assuming VT behavior. */
	printf("ZWL SEAT pause device=%u:%u type=%s display=%u\n", device->major, device->minor, kind, device->display);
	if (cooperative != 0) {
		error = seat_device_call("PauseDeviceComplete", device);
		if (error != 0)
			return error;
	}

	/* Gone devices cannot receive a resume; common teardown returns their lease once. */
	if (gone != 0) {
		if (device->display != 0) {
			seat_release(device);
			return ENODEV;
		}

		/* Restoring the owned fd lets ordinary common teardown find its lease. */
		if (input != NULL) {
			input->fd = device->fd;
			zwl_input_close(server, input);
		} else {
			seat_release(device);
		}
	}

	/* Succeeded: a cooperative acknowledgement follows complete withdrawal of use. */
	return 0;
}

/* Replaces a revoked file and restores the common input or display generation. */
static int
seat_resume(
	struct zwl_server *server,
	struct logind_device *device,
	struct dbus_reply *message,
	uint32_t index)
{
	struct zwl_input_device *input;
	int descriptor;
	int flags;
	int error;
	int clock_id;

	/* The message owns this new file until all descriptor setup has succeeded. */
	if (index >= message->fd_count)
		return EPROTO;
	descriptor = message->fds[index];
	if (descriptor < 0)
		return EPROTO;
	flags = fcntl(descriptor, F_GETFL);
	if (flags < 0)
		return errno;
	error = fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
	if (error != 0)
		return errno;

	/* Resumed evdev reports must retain the common monotonic timestamp domain. */
	if (device->display == 0) {
		clock_id = CLOCK_MONOTONIC;
		error = ioctl(descriptor, EVIOCSCLOCKID, &clock_id);
		if (error != 0)
			return errno;
	}

	/* Only one descriptor generation is owned after successful replacement. */
	descriptor = dbus_take_fd(message, index);
	if (descriptor < 0)
		return EPROTO;
	if (device->fd >= 0)
		(void)close(device->fd);
	device->fd = descriptor;
	device->paused = 0;

	/* Display reopening uses the scheduler's existing enter-window-mode path. */
	if (device->display != 0) {
		seat_paused = 0;
		server->os_paused = 0;
		server->windowed = 0;
		server->dirty = 1;
	} else {
		/* A partial evdev report from the revoked file cannot enter the new generation. */
		input = seat_input(server, device);
		if (input != NULL) {
			input->fd = descriptor;
			input->frame_count = 0;
			input->discarding = 0;
		}
	}

	/* Succeeded: input and display use only the newly supplied file. */
	printf("ZWL SEAT resume device=%u:%u display=%u\n", device->major, device->minor, device->display);
	return 0;
}
