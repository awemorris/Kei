/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws089-p003: the network's backend for the host tests, in place of
 * userland/desktop/settings/network.c: made-up states (host_network_fake)
 * and requests that print what they would ask of the daemon and change the
 * made-up state as the daemon would.  Test code only; the program never
 * has it.
 */

#include "settings.h"

#include <stdio.h>
#include <string.h>

void host_network_fake(struct se_app *app, const char *scenario);

/* One made-up network of a scan. */
static void
host_ap(
	struct se_network *network,
	const char *ssid,
	int rssi,
	unsigned secured)
{
	struct keiland_network_ap *ap;

	ap = &network->scan[network->scan_count];
	(void)snprintf(ap->ssid, sizeof(ap->ssid), "%s", ssid);
	ap->rssi = rssi;
	ap->secured = secured;
	network->scan_count++;
}

/* One made-up interface. */
static void
host_link(
	struct se_network *network,
	const char *name,
	const char *address,
	unsigned running,
	uint64_t received,
	uint64_t sent)
{
	struct keiland_network_link *link;

	link = &network->links[network->link_count];
	memset(link, 0, sizeof(*link));
	(void)snprintf(link->name, sizeof(link->name), "%s", name);
	link->up = 1;
	link->running = running;
	(void)snprintf(link->address, sizeof(link->address), "%s", address);
	if (address[0] != '\0')
		(void)snprintf(link->netmask, sizeof(link->netmask), "%s", "255.255.255.0");
	link->hardware[0] = 0x52;
	link->hardware[1] = 0x54;
	link->hardware[5] = (unsigned char)(network->link_count + 0x10);
	link->mtu = 1500;
	link->received_bytes = received;
	link->sent_bytes = sent;
	network->link_count++;
}

/*
 * Fills the network with a made-up state: "wifi" (on a Wi-Fi network, a
 * wired interface up too), "wired" (wired only, the radio off), "absent"
 * (no radio), "down" (the daemon not running).
 */
void
host_network_fake(
	struct se_app *app,
	const char *scenario)
{
	struct se_network *network;
	unsigned index;

	network = &app->network;
	memset(network, 0, sizeof(*network));
	network->state.reachable = 1;
	if (strcmp(scenario, "down") == 0) {
		network->state.reachable = 0;
		return;
	}

	/* The interfaces and the servers. */
	host_link(network, "lo0", "127.0.0.1", 1, 0, 0);
	network->links[0].loopback = 1;
	host_link(network, "em0", "10.0.2.15", 1, 1200000000ULL, 320000000ULL);
	(void)snprintf(network->dns[0], sizeof(network->dns[0]), "%s", "10.0.2.3");
	(void)snprintf(network->dns[1], sizeof(network->dns[1]), "%s", "1.1.1.1");
	network->dns_count = 2;
	(void)snprintf(network->state.wired, sizeof(network->state.wired), "%s", "em0");
	network->state.connected = 1;
	network->state.kind = KEILAND_NETWORK_WIRED;
	(void)snprintf(network->state.interface, sizeof(network->state.interface), "%s", "em0");

	/* The activity of the last two minutes. */
	for (index = 0; index < SE_USAGE_SAMPLES; index++) {
		network->received[index] = 400000U + (index * 37U % 23U) * 60000U + (index > 70U && index < 90U ? 1500000U : 0U);
		network->sent[index] = 80000U + (index * 11U % 17U) * 20000U;
	}
	network->usage_count = SE_USAGE_SAMPLES;
	network->usage_next = 0;
	network->received_total = 1200000000ULL;
	network->sent_total = 320000000ULL;

	/* The radio. */
	if (strcmp(scenario, "absent") == 0) {
		network->state.wifi = KEILAND_WIFI_ABSENT;
		return;
	}
	if (strcmp(scenario, "wired") == 0) {
		network->state.wifi = KEILAND_WIFI_OFF;
		return;
	}

	/* On a Wi-Fi network: the radio, its scan and the saved keys. */
	host_link(network, "wlan0", "192.168.1.24", 1, 50000000ULL, 9000000ULL);
	network->state.wifi = KEILAND_WIFI_CONNECTED;
	network->state.kind = KEILAND_NETWORK_WIFI;
	(void)snprintf(network->state.interface, sizeof(network->state.interface), "%s", "wlan0");
	(void)snprintf(network->state.wifi_interface, sizeof(network->state.wifi_interface), "%s", "wlan0");
	(void)snprintf(network->state.ssid, sizeof(network->state.ssid), "%s", "Kei Lab");
	host_ap(network, "Kei Lab", -48, 1);
	host_ap(network, "Cafe Guest", -63, 1);
	host_ap(network, "OSC Venue", -70, 1);
	host_ap(network, "Neighbor 5G", -79, 1);
	host_ap(network, "Library Free", -82, 0);
	(void)snprintf(network->saved[0], sizeof(network->saved[0]), "%s", "Kei Lab");
	(void)snprintf(network->saved[1], sizeof(network->saved[1]), "%s", "Cafe Guest");
	network->saved_count = 2;
}

/* The backend's calls, printed; the made-up daemon answers at once. */
void
se_network_open(
	struct se_app *app)
{
	(void)app;
}

void
se_network_poll(
	struct se_app *app,
	uint64_t now)
{
	(void)app;
	(void)now;
}

int
se_network_wait(
	struct se_app *app)
{
	(void)app;
	return -1;
}

void
se_network_close(
	struct se_app *app)
{
	se_field_clear(&app->network.key);
}

void
se_network_wifi(
	struct se_app *app,
	int on)
{
	printf("NETWORK wifi on=%d\n", on);
	app->network.state.wifi = on != 0 ? KEILAND_WIFI_SEARCHING : KEILAND_WIFI_OFF;
}

void
se_network_scan(
	struct se_app *app)
{
	(void)app;
	printf("NETWORK scan\n");
}

void
se_network_join(
	struct se_app *app,
	const char *ssid)
{
	printf("NETWORK join ssid=%s\n", ssid);
	(void)snprintf(app->network.join_ssid, sizeof(app->network.join_ssid), "%s", ssid);
	app->network.join_step = SE_JOIN_CONNECT;
	(void)snprintf(app->network.message, sizeof(app->network.message), "Connecting to %s...", ssid);
	app->network.message_bad = 0;
}

void
se_network_join_key(
	struct se_app *app,
	const char *ssid,
	const char *key)
{
	printf("NETWORK join-key ssid=%s key-length=%u\n", ssid, (unsigned)strlen(key));
	(void)snprintf(app->network.join_ssid, sizeof(app->network.join_ssid), "%s", ssid);
	app->network.join_step = SE_JOIN_PROFILES;
	(void)snprintf(app->network.message, sizeof(app->network.message), "Connecting to %s...", ssid);
	app->network.message_bad = 0;
}

void
se_network_disconnect(
	struct se_app *app)
{
	printf("NETWORK disconnect\n");
	app->network.state.wifi = KEILAND_WIFI_DISCONNECTED;
	app->network.state.ssid[0] = '\0';
}
