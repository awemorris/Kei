/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks actual native interface metadata and denied administrative writes.
 */
#include "userland/desktop/libkeiland/wpa/network-wpa.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

static int probe_links(const char *interface);

/*
 * Runs native link observation or an already unprivileged radio permission probe.
 */
int
main(
	int argc,
	char **argv)
{
	int error;
	int same;

	/* Requires a selected interface and an explicit fixture operation. */
	if (argc != 3)
		return 2;

	/* The fixture drops privilege before this process can alter interface flags. */
	same = strcmp(argv[2], "denied");
	if (same == 0) {
		error = kwpa_radio(argv[1], 1);
		if (error != EPERM && error != EACCES)
			return 1;
		(void)printf("PASS native administrative write denied errno=%d\n", error);
		return 0;
	}

	/* Reads actual interfaces and resolver configuration without a control peer. */
	error = probe_links(argv[1]);
	if (error != 0) {
		(void)fprintf(stderr, "native link probe failed errno=%d\n", error);
		return 1;
	}

	/* Succeeded: native snapshots and absent-supplicant wired state agree. */
	return 0;
}

/* Compares real native link metadata with the public connected state. */
static int
probe_links(
	const char *interface)
{
	struct keiland_network_link links[KEILAND_NETWORK_LINKS_MAX];
	struct keiland_network_state state;
	struct keiland_network *network;
	struct sockaddr_un address;
	char servers[KEILAND_NETWORK_DNS_MAX][KEILAND_NETWORK_ADDRESS_MAX];
	char oversized[256];
	socklen_t extent;
	size_t count;
	size_t index;
	size_t dns;
	unsigned changed;
	int same;
	int error;
	int wireless;
	int found;

	/* Copies actual native addresses, hardware identity and counters. */
	count = keiland_network_get_links(links, KEILAND_NETWORK_LINKS_MAX);
	if (count == 0)
		return ENODEV;

	/* Finds the owned fixture's external interface among all kernel records. */
	found = 0;
	for (index = 0; index < count; index++) {
		/* Prints independent fields for comparison with the native ifconfig utility. */
		(void)printf("link %s up=%u running=%u loopback=%u ip=%s mask=%s mtu=%u rx=%llu tx=%llu mac=%02x:%02x:%02x:%02x:%02x:%02x\n", links[index].name, links[index].up, links[index].running, links[index].loopback, links[index].address, links[index].netmask, links[index].mtu, (unsigned long long)links[index].received_bytes, (unsigned long long)links[index].sent_bytes, links[index].hardware[0], links[index].hardware[1], links[index].hardware[2], links[index].hardware[3], links[index].hardware[4], links[index].hardware[5]);

		/* Restricts positive traffic and addressed state to the fixture's external link. */
		same = strcmp(links[index].name, interface);
		if (same != 0)
			continue;

		/* An addressed running interface must publish its real MTU and traffic. */
		if (links[index].up != 1 ||
		    links[index].running != 1 ||
		    links[index].loopback != 0 ||
		    links[index].address[0] == '\0' ||
		    links[index].netmask[0] == '\0' ||
		    links[index].mtu == 0 ||
		    links[index].received_bytes == 0 ||
		    links[index].sent_bytes == 0)
			return EPROTO;
		found = 1;
	}

	/* A missing requested interface invalidates the fixture rather than passing silently. */
	if (found == 0)
		return ENODEV;

	/* A real wired interface cannot satisfy native wireless classification. */
	wireless = kwpa_wireless(interface);
	if (wireless != 0)
		return EPROTO;

	/* Reports real dotted IPv4 resolver entries from the guest's configuration. */
	dns = keiland_network_get_dns(servers, KEILAND_NETWORK_DNS_MAX);
	if (dns == 0)
		return EPROTO;

	/* Copies all bounded resolver entries into the evidence. */
	for (index = 0; index < dns; index++)
		(void)printf("dns %s\n", servers[index]);

	/* Invalid native interface names cannot alter another interface. */
	error = kwpa_radio("ws109missing", 1);
	if (error == 0)
		return EPROTO;
	(void)printf("missing interface errno=%d\n", error);

	/* Rejects an oversized Unix pathname without publishing an address extent. */
	memset(oversized, 'x', sizeof(oversized) - 1);
	oversized[sizeof(oversized) - 1] = '\0';
	extent = 123;
	error = kwpa_socket_address(&address, oversized, &extent);
	if (error != ENAMETOOLONG || extent != 123)
		return EPROTO;

	/* A missing supplicant must leave wired connectivity observable. */
	network = keiland_network_open();
	if (network == NULL)
		return ENOMEM;
	error = keiland_network_update(network, &changed);
	if (error != 0) {
		keiland_network_close(network);
		return error;
	}

	/* Releases the watch before comparing its independent state snapshot. */
	keiland_network_get_state(network, &state);
	keiland_network_close(network);
	if (state.reachable != 0 ||
	    state.connected != 1 ||
	    state.kind != KEILAND_NETWORK_WIRED)
		return EPROTO;

	/* The selected public wired connection must identify the actual external link. */
	same = strcmp(state.interface, interface);
	if (same != 0)
		return EPROTO;
	(void)printf("PASS actual native wired state, DNS, MAC, MTU, counters, no supplicant\n");

	/* Succeeded: native kernel metadata and public wired selection agree. */
	return 0;
}
