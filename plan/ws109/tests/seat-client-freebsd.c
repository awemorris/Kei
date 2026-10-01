/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks real seatd authority against native evdev permissions and descriptor ownership. */
#include <sys/ioctl.h>
#include <dev/evdev/input.h>
#include <errno.h>
#include <fcntl.h>
#include <libseat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* One synchronous client's activation and acknowledgement state survives dispatch callbacks. */
struct probe_state {
	int enabled;
	int error;
};

static void seat_enabled(struct libseat *seat, void *data);
static void seat_disabled(struct libseat *seat, void *data);
static int probe_device(struct libseat *seat);
static int inspect_device(int descriptor);

/* Verifies that the native daemon transfers a real device to an otherwise denied client. */
int
main(
    void)
{
	struct libseat_seat_listener listener;
	struct probe_state state;
	struct libseat *seat;
	int descriptor;
	int error;
	int attempt;
	uid_t user;
	int groups;

	/* Refuses a controller that accidentally kept root or supplementary device permissions. */
	user = geteuid();
	if (user != 65534)
		return 1;

	/* The independent kernel open must deny access before the daemon grants a lease. */
	groups = getgroups(0, NULL);
	if (groups != 0)
		return 1;

	/* Demonstrates that this client cannot obtain the same device directly. */
	descriptor =
	    open("/dev/input/event1", O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (descriptor >= 0) {
		(void)close(descriptor);
		return 1;
	}

	/* Requires the expected access denial rather than an absent or invalid fixture. */
	if (errno != EACCES)
		return 1;

	/* Callbacks retain their state until the seat owner is closed. */
	memset(&state, 0, sizeof(state));
	listener.enable_seat = seat_enabled;
	listener.disable_seat = seat_disabled;

	/* Connects through the private seatd-only native client, with no substitute backend. */
	seat = libseat_open_seat(&listener, &state);
	if (seat == NULL) {
		(void)fprintf(stderr, "open native seat failed errno=%d\n",
			      errno);
		return 1;
	}

	/* Gives the real daemon a bounded opportunity to activate this session. */
	error = 0;
	for (attempt = 0; attempt < 100; attempt++) {
		error = libseat_dispatch(seat, 20);
		if (error < 0)
			break;

		/* Activation is published only by the daemon's callback. */
		if (state.enabled != 0)
			break;
	}

	/* Queries the device only after a real activation, then retires the seat on every outcome. */
	if (error < 0) {
		error = errno;
	} else if (state.enabled == 0) {
		error = ETIMEDOUT;
	} else {
		error = probe_device(seat);
	}

	/* Closing the seat releases any remaining daemon-owned session state. */
	descriptor = libseat_close_seat(seat);
	if (descriptor != 0 && error == 0)
		error = errno;

	/* Reports any device or callback failure after releasing the service owner. */
	if (state.error != 0 && error == 0)
		error = state.error;

	/* Makes a real service failure distinguishable from successful fd transfer. */
	if (error != 0) {
		(void)fprintf(stderr, "native seat probe failed errno=%d\n",
			      error);
		return 1;
	}

	/* Succeeded: kernel denial and daemon-authorized native device ownership were verified. */
	(void)printf("PASS native seatd uid=65534 groups=0 direct=EACCES "
		     "actual evdev lease/release\n");
	return 0;
}

/* Publishes the real activation to the client's bounded dispatch loop. */
static void
seat_enabled(
    struct libseat *seat,
    void *data)
{
	struct probe_state *state;

	/* Callback state belongs to main until libseat_close_seat has returned. */
	(void)seat;
	state = data;
	state->enabled = 1;

	/* Succeeded: the probe may now request an actual device. */
	return;
}

/* Acknowledges a withdrawal after the synchronous probe has ceased device use. */
static void
seat_disabled(
    struct libseat *seat,
    void *data)
{
	struct probe_state *state;
	int error;

	/* Withdraws activation before telling the daemon that this client is quiescent. */
	state = data;
	state->enabled = 0;
	error = libseat_disable_seat(seat);
	if (error != 0) {
		state->error = errno;
		return;
	}

	/* Succeeded: no probe descriptor remains in use during dispatch. */
	return;
}

/* Releases the received file separately from the daemon's device lease. */
static int
probe_device(
    struct libseat *seat)
{
	int descriptor;
	int device;
	int error;
	int status;

	/* Requests a daemon-opened native device that this user cannot open directly. */
	descriptor = -1;
	device = libseat_open_device(seat, "/dev/input/event1", &descriptor);
	if (device < 0)
		return errno;

	/* Checks the actual received kernel file before retiring both independent owners. */
	error = inspect_device(descriptor);
	status = close(descriptor);
	if (status != 0 && error == 0)
		error = errno;

	/* The protocol release does not close the client's descriptor on its behalf. */
	status = libseat_close_device(seat, device);
	if (status != 0 && error == 0)
		error = errno;

	/* Reports a descriptor or service release failure without abandoning the seat. */
	if (error != 0)
		return error;

	/* Succeeded: both device owners were explicitly retired. */
	return 0;
}

/* Reads kernel metadata through the transferred file rather than trusting a peer description. */
static int
inspect_device(
    int descriptor)
{
	struct input_id identity;
	char name[128];
	unsigned char bits[32];
	int error;
	int flags;

	/* Received device handles must not escape through a later exec. */
	flags = fcntl(descriptor, F_GETFD);
	if (flags < 0)
		return errno;

	/* Treats a missing inheritance guard as an ownership failure. */
	if ((flags & FD_CLOEXEC) == 0)
		return EPROTO;

	/* Obtains the real native evdev device name. */
	memset(name, 0, sizeof(name));
	error = ioctl(descriptor, EVIOCGNAME(sizeof(name)), name);
	if (error < 0)
		return errno;

	/* An empty answer cannot prove this is an actual input device. */
	if (name[0] == '\0')
		return EPROTO;

	/* Obtains kernel identity independently of the service's pathname. */
	memset(&identity, 0, sizeof(identity));
	error = ioctl(descriptor, EVIOCGID, &identity);
	if (error < 0)
		return errno;

	/* Obtains event capabilities through FreeBSD's actual evdev ioctl ABI. */
	memset(bits, 0, sizeof(bits));
	error = ioctl(descriptor, EVIOCGBIT(0, sizeof(bits)), bits);
	if (error < 0)
		return errno;

	/* The selected native mouse must expose actual event families. */
	if (bits[0] == 0)
		return EPROTO;

	/* Succeeded: the service supplied a kernel evdev descriptor with observable metadata. */
	(void)printf(
	    "native device=%s bus=%u vendor=%u product=%u events=%02x\n", name,
	    identity.bustype, identity.vendor, identity.product, bits[0]);
	return 0;
}
