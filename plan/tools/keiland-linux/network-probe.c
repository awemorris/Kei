/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks the production network backend against the isolated simulated AP.
 */
#include <keiland.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int probe(struct keiland_network *network);
static int wait_state(struct keiland_network *network, unsigned wifi, unsigned seconds);
static int wait_request(struct keiland_network *network, unsigned request);

/*
 * Owns one watch while the simulated-radio criteria exercise the public API.
 */
int
main(
	void)
{
	struct keiland_network *network;
	int error;

	/* A missing allocation prevents all radio operations. */
	network = keiland_network_open();
	if (network == NULL)
		return 1;
	error = probe(network);
	keiland_network_close(network);
	if (error != 0) {
		fprintf(stderr, "network-probe: FAIL errno=%d %s\n", error, strerror(error));
		return 1;
	}

	/* Succeeded: the production backend met each simulated-radio criterion. */
	printf("network-probe: PASS\n");
	return 0;
}

/* Verifies scan, credentials, join, disconnect and details using one owned watch. */
static int
probe(
	struct keiland_network *network)
{
	struct keiland_network_state state;
	struct keiland_network_ap aps[KEILAND_NETWORK_SCAN_MAX];
	struct keiland_network_link links[KEILAND_NETWORK_LINKS_MAX];
	char saved[KEILAND_NETWORK_SCAN_MAX][KEILAND_NETWORK_SSID_MAX];
	char dns[KEILAND_NETWORK_DNS_MAX][KEILAND_NETWORK_ADDRESS_MAX];
	size_t count;
	size_t index;
	unsigned changed;
	unsigned iteration;
	unsigned found;
	unsigned wifi;
	unsigned wired;
	int error;
	int same;

	/* Wait for an actual reachable disconnected radio, not a placeholder state. */
	error = wait_state(network, KEILAND_WIFI_DISCONNECTED, 10);
	if (error != 0)
		return error;
	error = keiland_network_request(network, KEILAND_NETWORK_REQUEST_SCAN, NULL);
	if (error != 0)
		return error;
	error = wait_request(network, KEILAND_NETWORK_REQUEST_SCAN);
	if (error != 0)
		return error;

	/* A completed scan must expose the secured fixture AP within twenty seconds. */
	found = 0;
	for (iteration = 0; iteration < 1000; iteration++) {
		error = keiland_network_update(network, &changed);
		if (error != 0)
			return error;
		count = keiland_network_get_scan(network, aps, KEILAND_NETWORK_SCAN_MAX);
		for (index = 0; index < count; index++) {
			same = strcmp(aps[index].ssid, "keiland-test");
			if (same == 0 && aps[index].secured != 0)
				found = 1;
		}

		/* Report only an observed AP, never the accepted SCAN acknowledgement. */
		if (found != 0)
			break;
		(void)usleep(20000);
	}

	/* Timeout means the actual daemon scan did not meet its acceptance. */
	if (found == 0) {
		error = ETIMEDOUT;
		return error;
	}

	/* Saving a key must not itself connect the previously disconnected radio. */
	printf("network-probe: scan secured AP PASS\n");
	error = keiland_network_save_key("keiland-test", "keiland-pass");
	if (error != 0)
		return error;
	(void)usleep(200000);
	error = keiland_network_update(network, &changed);
	if (error != 0)
		return error;
	keiland_network_get_state(network, &state);
	if (state.wifi == KEILAND_WIFI_CONNECTED) {
		error = EPROTO;
		return error;
	}

	/* The application's usual profile refresh and JOIN sequence remains unchanged. */
	error = keiland_network_request(network, KEILAND_NETWORK_REQUEST_PROFILES, NULL);
	if (error != 0)
		return error;
	error = wait_request(network, KEILAND_NETWORK_REQUEST_PROFILES);
	if (error != 0)
		return error;
	error = keiland_network_request(network, KEILAND_NETWORK_REQUEST_JOIN, "keiland-test");
	if (error != 0)
		return error;
	error = wait_request(network, KEILAND_NETWORK_REQUEST_JOIN);
	if (error != 0)
		return error;
	error = wait_state(network, KEILAND_WIFI_CONNECTED, 30);
	if (error != 0)
		return error;
	keiland_network_get_state(network, &state);
	same = strcmp(state.ssid, "keiland-test");
	if (same != 0 ||
	    state.kind != KEILAND_NETWORK_WIFI ||
	    state.connected == 0) {
		error = EPROTO;
		return error;
	}

	/* Kernel interfaces, saved profiles and resolvers are real backend outputs. */
	printf("network-probe: join wifi=%u kind=%u ssid=%s PASS\n", state.wifi, state.kind, state.ssid);
	count = keiland_network_get_links(links, KEILAND_NETWORK_LINKS_MAX);
	wifi = 0;
	wired = 0;
	for (index = 0; index < count; index++) {
		printf("network-probe: link %s up=%u running=%u ip=%s mtu=%u rx=%llu tx=%llu\n", links[index].name, links[index].up, links[index].running, links[index].address, links[index].mtu, (unsigned long long)links[index].received_bytes, (unsigned long long)links[index].sent_bytes);
		same = strcmp(links[index].name, "wlan0");
		if (same == 0)
			wifi = 1;
		same = strncmp(links[index].name, "enp", 3);
		if (same == 0)
			wired = 1;
	}

	/* Both wired and simulated wireless devices must be represented. */
	if (wifi == 0 || wired == 0) {
		error = ENODEV;
		return error;
	}

	/* The saved credential query must decode the same exact SSID. */
	found = 0;
	count = keiland_network_get_saved(saved, KEILAND_NETWORK_SCAN_MAX);
	for (index = 0; index < count; index++) {
		same = strcmp(saved[index], "keiland-test");
		if (same == 0)
			found = 1;
	}

	/* A missing saved profile is distinct from an association failure. */
	if (found == 0) {
		error = ENOENT;
		return error;
	}

	/* QEMU's resolver proves the existing DNS parser reads the guest configuration. */
	found = 0;
	count = keiland_network_get_dns(dns, KEILAND_NETWORK_DNS_MAX);
	for (index = 0; index < count; index++) {
		same = strcmp(dns[index], "10.0.2.3");
		if (same == 0)
			found = 1;
	}

	/* Resolver absence is an unmet detail criterion. */
	if (found == 0) {
		error = ENOENT;
		return error;
	}

	/* Explicit disconnect returns the radio to the public disconnected state. */
	error = keiland_network_request(network, KEILAND_NETWORK_REQUEST_DISCONNECT, NULL);
	if (error != 0)
		return error;
	error = wait_request(network, KEILAND_NETWORK_REQUEST_DISCONNECT);
	if (error != 0)
		return error;
	error = wait_state(network, KEILAND_WIFI_DISCONNECTED, 10);
	if (error != 0)
		return error;

	/* Succeeded: the production library satisfied each simulated-radio criterion. */
	return 0;
}

/* Waits a bounded time for a reachable radio state without blocking update. */
static int
wait_state(
	struct keiland_network *network,
	unsigned wifi,
	unsigned seconds)
{
	struct keiland_network_state state;
	unsigned iteration;
	unsigned changed;
	int error;

	/* Poll the ordinary public update path until the requested state is observed. */
	for (iteration = 0; iteration < seconds * 50; iteration++) {
		error = keiland_network_update(network, &changed);
		if (error != 0)
			return error;
		keiland_network_get_state(network, &state);
		if (state.reachable != 0 && state.wifi == wifi)
			return 0;
		(void)usleep(20000);
	}

	/* The bounded criterion failed rather than extending an investigation silently. */
	fprintf(stderr, "state reachable=%u wifi=%u ssid=%s\n", state.reachable, state.wifi, state.ssid);
	return ETIMEDOUT;
}

/* Waits for the exact public completion event and checks the daemon outcome. */
static int
wait_request(
	struct keiland_network *network,
	unsigned request)
{
	unsigned iteration;
	unsigned changed;
	unsigned finished;
	int error;
	int request_error;

	/* Commands have a two-second backend deadline; five seconds bounds the fixture. */
	for (iteration = 0; iteration < 250; iteration++) {
		error = keiland_network_update(network, &changed);
		if (error != 0)
			return error;
		if ((changed & KEILAND_NETWORK_CHANGED_DONE) != 0) {
			finished = keiland_network_get_request(network, &request_error);
			if (finished != request)
				return EPROTO;
			if (request_error != 0)
				return request_error;
			return 0;
		}

		/* The library remains responsible for each request's protocol state. */
		(void)usleep(20000);
	}

	/* No completion event arrived within the fixture's fixed bound. */
	return ETIMEDOUT;
}
