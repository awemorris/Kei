/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Exercises the production WPA client against an independent native datagram peer.
 * The peer proves wire contracts, not actual WiFi radio operation.
 */
#include "userland/desktop/libkeiland/wpa/network-wpa.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int probe_wire(struct keiland_network *network);
static int await_state(struct keiland_network *network, unsigned wifi);
static int await_request(struct keiland_network *network, unsigned request, const char *ssid, int expected);
static int await_scan(struct keiland_network *network);
static int pause_update(struct keiland_network *network, unsigned *changed);

/*
 * Verifies escaped credentials, profile persistence, scans, requests and deadlines.
 */
int
main(
	void)
{
	struct keiland_network *network;
	int error;

	/* Opens the normal production endpoint selected by the native backend. */
	network = keiland_network_open();
	if (network == NULL)
		return 1;

	/* Keeps cleanup outside assertions so every failed contract releases both sockets. */
	error = probe_wire(network);
	keiland_network_close(network);
	if (error != 0) {
		(void)fprintf(stderr, "WPA wire contract failed errno=%d\n", error);
		return 1;
	}

	/* Succeeded: the bounded native wire contract passed without a physical-radio claim. */
	(void)printf("PASS native WPA mock wire credentials/scan/profiles/join/disconnect/timeout\n");
	return 0;
}

/* Runs independent credential operations while a live watch owns its own peers. */
static int
probe_wire(
	struct keiland_network *network)
{
	char saved[4][KEILAND_NETWORK_SSID_MAX];
	size_t count;
	int same;
	int error;
	uint64_t started;
	uint64_t elapsed;

	/* Waits for an actual attached monitor and complete disconnected STATUS reply. */
	error = await_state(network, KEILAND_WIFI_DISCONNECTED);
	if (error != 0)
		return error;

	/* Persists quote-sensitive public fixture data on an independent command socket. */
	error = keiland_network_save_key("WS109\"\\wire", "test\"\\passphrase");
	if (error != 0)
		return error;

	/* Reads the saved profile independently of the watch's request state. */
	count = keiland_network_get_saved(saved, 4);
	if (count != 1)
		return EPROTO;

	/* Exact decoded SSIDs must survive both the hex write and the daemon listing. */
	same = strcmp(saved[0], "WS109\"\\wire");
	if (same != 0)
		return EPROTO;

	/* Refreshing profiles must complete without implicitly selecting a connection. */
	error = await_request(network, KEILAND_NETWORK_REQUEST_PROFILES, NULL, 0);
	if (error != 0)
		return error;

	/* A successful scan acknowledgement and later result event are distinct contracts. */
	error = await_request(network, KEILAND_NETWORK_REQUEST_SCAN, NULL, 0);
	if (error != 0)
		return error;

	/* The asynchronous result table must deduplicate and sort the independent BSS rows. */
	error = await_scan(network);
	if (error != 0)
		return error;

	/* Joining a missing saved profile returns ENOENT rather than choosing another one. */
	error = await_request(network, KEILAND_NETWORK_REQUEST_JOIN, "unsaved", ENOENT);
	if (error != 0)
		return error;

	/* Explicit join resolves and selects the exact saved profile. */
	error = await_request(network, KEILAND_NETWORK_REQUEST_JOIN, "WS109\"\\wire", 0);
	if (error != 0)
		return error;

	/* The monitor's connection event refreshes STATUS independently of JOIN completion. */
	error = await_state(network, KEILAND_WIFI_CONNECTED);
	if (error != 0)
		return error;

	/* Explicit disconnect completes without taking the guest's actual wired link down. */
	error = await_request(network, KEILAND_NETWORK_REQUEST_DISCONNECT, NULL, 0);
	if (error != 0)
		return error;

	/* A second scan receives no peer reply and must finish at the production deadline. */
	started = kwpa_milliseconds();
	error = await_request(network, KEILAND_NETWORK_REQUEST_SCAN, NULL, ETIMEDOUT);
	if (error != 0)
		return error;

	/* Deadline evidence comes from the monotonic clock, with bounded scheduling tolerance. */
	elapsed = kwpa_milliseconds() - started;
	if (elapsed < 1900 || elapsed > 4000)
		return EPROTO;
	(void)printf("command timeout %llu ms\n", (unsigned long long)elapsed);

	/* Succeeded: the public contracts retain socket, request and credential ownership. */
	return 0;
}

/* Waits for a complete state using production nonblocking updates and a finite bound. */
static int
await_state(
	struct keiland_network *network,
	unsigned wifi)
{
	struct keiland_network_state state;
	unsigned changed;
	uint64_t deadline;
	uint64_t now;
	int error;

	/* Bounds all polling independently of the client's reply deadline. */
	deadline = kwpa_milliseconds() + 4000;
	for (;;) {
		/* Samples the actual watch after draining its native datagrams. */
		error = pause_update(network, &changed);
		if (error != 0)
			return error;

		/* Reachability requires a complete STATUS rather than ATTACH alone. */
		keiland_network_get_state(network, &state);
		if (state.reachable != 0 && state.wifi == wifi)
			break;

		/* An unresponsive fixture cannot keep the probe running forever. */
		now = kwpa_milliseconds();
		if (now >= deadline)
			return ETIMEDOUT;
	}

	/* Succeeded: an actual native reply supplied the requested radio state. */
	return 0;
}

/* Waits for the public completion event and preserves the exact request errno. */
static int
await_request(
	struct keiland_network *network,
	unsigned request,
	const char *ssid,
	int expected)
{
	unsigned changed;
	unsigned finished;
	uint64_t deadline;
	uint64_t now;
	int error;
	int reported;

	/* Queues one production request while the independent peer controls replies. */
	error = keiland_network_request(network, request, ssid);
	if (error != 0)
		return error;

	/* Allows the client's two-second deadline and a bounded scheduling margin. */
	deadline = kwpa_milliseconds() + 4000;
	for (;;) {
		/* Advances only through the public nonblocking state machine. */
		error = pause_update(network, &changed);
		if (error != 0)
			return error;

		/* Consumes completion only when the watch explicitly publishes CHANGED_DONE. */
		if ((changed & KEILAND_NETWORK_CHANGED_DONE) != 0)
			break;

		/* Stops an incomplete fixture interaction without extending its acceptance bound. */
		now = kwpa_milliseconds();
		if (now >= deadline)
			return ETIMEDOUT;
	}

	/* Completion retains both the request identity and its exact error. */
	finished = keiland_network_get_request(network, &reported);
	if (finished != request || reported != expected) {
		(void)fprintf(stderr, "request %u finished %u errno=%d expected=%d\n", request, finished, reported, expected);
		return EPROTO;
	}

	/* Succeeded: the requested operation published its expected completion. */
	return 0;
}

/* Checks that actual asynchronous scan replies replace the cache with strongest BSS rows. */
static int
await_scan(
	struct keiland_network *network)
{
	struct keiland_network_ap aps[KEILAND_NETWORK_SCAN_MAX];
	unsigned changed;
	uint64_t deadline;
	uint64_t now;
	size_t count;
	int error;
	int same;

	/* Waits for result processing without assuming it precedes scan acknowledgement. */
	deadline = kwpa_milliseconds() + 4000;
	for (;;) {
		/* Drains the event and command sockets through the actual client. */
		error = pause_update(network, &changed);
		if (error != 0)
			return error;

		/* A complete fixture table contains two SSIDs from three BSS rows. */
		count = keiland_network_get_scan(network, aps, KEILAND_NETWORK_SCAN_MAX);
		if (count == 2)
			break;

		/* A missing result event cannot be replaced by a cached scan. */
		now = kwpa_milliseconds();
		if (now >= deadline)
			return ETIMEDOUT;
	}

	/* The duplicate secure SSID must keep its stronger RSSI and first position. */
	same = strcmp(aps[0].ssid, "WS109\"\\wire");
	if (same != 0 || aps[0].rssi != -35 || aps[0].secured != 1)
		return EPROTO;

	/* The second SSID is an independent open network with its own signal. */
	same = strcmp(aps[1].ssid, "open");
	if (same != 0 || aps[1].rssi != -50 || aps[1].secured != 0)
		return EPROTO;

	/* Succeeded: deduplication and signal ordering match the independent wire fixture. */
	return 0;
}

/* Gives the independent server a scheduling turn before each nonblocking update. */
static int
pause_update(
	struct keiland_network *network,
	unsigned *changed)
{
	struct timespec pause = {0, 10000000};
	int error;

	/* A real monotonic wait keeps the probe from hiding deadline behavior. */
	error = nanosleep(&pause, NULL);
	if (error != 0)
		return errno;

	/* Advances the default production path without a fake clock or injected response. */
	error = keiland_network_update(network, changed);
	if (error != 0)
		return error;

	/* Succeeded: one bounded public update has completed. */
	return 0;
}
