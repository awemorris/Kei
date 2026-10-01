/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Linux build's library contract check before service backends are installed.
 */

#include <keiland.h>
#include <stdio.h>

/*
 * Checks the library version and the initially absent network and audio services.
 */
int
main(
	void)
{
	struct keiland_network *network;
	struct keiland_network_state state;
	char servers[KEILAND_NETWORK_DNS_MAX][KEILAND_NETWORK_ADDRESS_MAX];
	unsigned version;
	int available;

	/* Requires the desktop contract prepared by WS104. */
	version = keiland_version();
	if (version != 21U)
		return 1;

	/* Retains a network subscription even without a backend service. */
	network = keiland_network_open();
	if (network == NULL)
		return 1;

	/* Copies the absent network service's state. */
	keiland_network_get_state(network, &state);

	/* Releases the record before reporting any later failure. */
	keiland_network_close(network);

	/* The placeholder must not claim a reachable Wi-Fi service. */
	if (state.reachable != 0)
		return 1;

	/* The placeholder must not claim an available ALSA control. */
	available = keiland_audio_available();
	if (available != 0)
		return 1;

	/* Exercises the real resolver reader without requiring a particular host configuration. */
	(void)keiland_network_get_dns(servers, KEILAND_NETWORK_DNS_MAX);

	/* Publishes the checked build-time library contract. */
	(void)puts("lib-smoke: PASS");

	/* Succeeded: the library contract and resolver reader are usable. */
	return 0;
}
