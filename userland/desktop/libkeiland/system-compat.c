/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The old network calls of <keiland.h>, kept while Settings still uses them
 * (WS131 p003 to p011).
 *
 * The network's operating-system code moved to libkeiland-backend
 * (plan/ws131/design.md section 3).  Until Settings reaches the network
 * through the compositor's extension (ws131-p011, which removes this file),
 * keiland_network_* keeps its names, types and meaning and forwards every
 * call to kl_backend_network_*, whose sources are built into this library
 * for the time being.  The two headers describe the same values; the checks
 * below stop the build if they ever differ, since requests, states and
 * change bits are passed through unchanged.
 */

#include <keiland.h>

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The forwarded values must mean the same on both sides of the forwarding. */
#if KEILAND_NETWORK_SSID_MAX != KL_BACKEND_NETWORK_SSID_MAX || \
    KEILAND_NETWORK_NAME_MAX != KL_BACKEND_NETWORK_NAME_MAX || \
    KEILAND_NETWORK_SCAN_MAX != KL_BACKEND_NETWORK_SCAN_MAX || \
    KEILAND_NETWORK_LINKS_MAX != KL_BACKEND_NETWORK_LINKS_MAX || \
    KEILAND_NETWORK_DNS_MAX != KL_BACKEND_NETWORK_DNS_MAX || \
    KEILAND_NETWORK_ADDRESS_MAX != KL_BACKEND_NETWORK_ADDRESS_MAX || \
    KEILAND_NETWORK_KEY_MIN != KL_BACKEND_NETWORK_KEY_MIN || \
    KEILAND_NETWORK_KEY_MAX != KL_BACKEND_NETWORK_KEY_MAX
#error "the network's sizes differ between keiland.h and keiland-backend.h"
#endif

#if KEILAND_NETWORK_NONE != KL_BACKEND_NETWORK_NONE || \
    KEILAND_NETWORK_WIRED != KL_BACKEND_NETWORK_WIRED || \
    KEILAND_NETWORK_WIFI != KL_BACKEND_NETWORK_WIFI || \
    KEILAND_WIFI_ABSENT != KL_BACKEND_WIFI_ABSENT || \
    KEILAND_WIFI_OFF != KL_BACKEND_WIFI_OFF || \
    KEILAND_WIFI_SEARCHING != KL_BACKEND_WIFI_SEARCHING || \
    KEILAND_WIFI_CONNECTING != KL_BACKEND_WIFI_CONNECTING || \
    KEILAND_WIFI_CONNECTED != KL_BACKEND_WIFI_CONNECTED || \
    KEILAND_WIFI_DISCONNECTED != KL_BACKEND_WIFI_DISCONNECTED
#error "the network's kinds or Wi-Fi states differ between keiland.h and keiland-backend.h"
#endif

#if KEILAND_NETWORK_CHANGED_STATE != KL_BACKEND_NETWORK_CHANGED_STATE || \
    KEILAND_NETWORK_CHANGED_SCAN != KL_BACKEND_NETWORK_CHANGED_SCAN || \
    KEILAND_NETWORK_CHANGED_DONE != KL_BACKEND_NETWORK_CHANGED_DONE || \
    KEILAND_NETWORK_REQUEST_NONE != KL_BACKEND_NETWORK_REQUEST_NONE || \
    KEILAND_NETWORK_REQUEST_SCAN != KL_BACKEND_NETWORK_REQUEST_SCAN || \
    KEILAND_NETWORK_REQUEST_JOIN != KL_BACKEND_NETWORK_REQUEST_JOIN || \
    KEILAND_NETWORK_REQUEST_DISCONNECT != KL_BACKEND_NETWORK_REQUEST_DISCONNECT || \
    KEILAND_NETWORK_REQUEST_WIFI_ON != KL_BACKEND_NETWORK_REQUEST_WIFI_ON || \
    KEILAND_NETWORK_REQUEST_WIFI_OFF != KL_BACKEND_NETWORK_REQUEST_WIFI_OFF || \
    KEILAND_NETWORK_REQUEST_PROFILES != KL_BACKEND_NETWORK_REQUEST_PROFILES
#error "the network's change bits or requests differ between keiland.h and keiland-backend.h"
#endif

/*
 * A watch of the network opened through the old call.
 *
 * Allocated by keiland_network_open and freed by keiland_network_close;
 * backend is the backend's watch it forwards to, owned by this one.
 */
struct keiland_network {
	struct kl_backend_network *backend;
};

static void network_copy_ap(struct keiland_network_ap *to, const struct kl_backend_network_ap *from);
static void network_copy_link(struct keiland_network_link *to, const struct kl_backend_network_link *from);

/*
 * Starts watching the network through the backend.
 */
struct keiland_network *
keiland_network_open(
	void)
{
	struct keiland_network *network;

	/* Allocates the old watch. */
	network = malloc(sizeof(*network));
	if (network == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	/* Opens the backend's watch it forwards to (errno is the backend's when it fails). */
	network->backend = kl_backend_network_open();
	if (network->backend == NULL) {
		free(network);
		return NULL;
	}

	/* Succeeded: the caller owns the watch. */
	return network;
}

/*
 * Stops watching the network.
 */
void
keiland_network_close(
	struct keiland_network *network)
{
	/* A watch that was never opened has nothing to close. */
	if (network == NULL)
		return;

	/* Closes the backend's watch, then the old one. */
	kl_backend_network_close(network->backend);
	free(network);
}

/*
 * Reads what has arrived from the network's daemon.
 */
int
keiland_network_update(
	struct keiland_network *network,
	unsigned *changed)
{
	int error;

	/* Reads through the backend; the change bits are the same on both sides. */
	error = kl_backend_network_update(network->backend, changed);
	if (error != 0)
		return error;

	/* Succeeded: *changed says what changed. */
	return 0;
}

/*
 * Copies the network's state as last reported.
 */
void
keiland_network_get_state(
	const struct keiland_network *network,
	struct keiland_network_state *state)
{
	struct kl_backend_network_state reported;

	/* Asks the backend for the state. */
	kl_backend_network_get_state(network->backend, &reported);

	/* Copies the state field by field into the old structure. */
	state->reachable = reported.reachable;
	state->connected = reported.connected;
	state->kind = reported.kind;
	memcpy(state->interface, reported.interface, sizeof(state->interface));
	memcpy(state->wired, reported.wired, sizeof(state->wired));
	state->wifi = reported.wifi;
	memcpy(state->wifi_interface, reported.wifi_interface, sizeof(state->wifi_interface));
	memcpy(state->ssid, reported.ssid, sizeof(state->ssid));
}

/*
 * Copies up to capacity networks of the last scan.
 */
size_t
keiland_network_get_scan(
	const struct keiland_network *network,
	struct keiland_network_ap *aps,
	size_t capacity)
{
	struct kl_backend_network_ap found[KL_BACKEND_NETWORK_SCAN_MAX];
	size_t count;
	size_t copied;
	size_t index;

	/* Asks the backend for the whole scan (it never keeps more than the array holds). */
	count = kl_backend_network_get_scan(network->backend, found, KL_BACKEND_NETWORK_SCAN_MAX);

	/* Copies as many as the caller has room for and the backend filled. */
	copied = count;
	if (copied > KL_BACKEND_NETWORK_SCAN_MAX)
		copied = KL_BACKEND_NETWORK_SCAN_MAX;
	if (copied > capacity)
		copied = capacity;

	/* Converts each network found into the old structure. */
	for (index = 0; index < copied; index++)
		network_copy_ap(&aps[index], &found[index]);

	/* Reports how many networks the scan has. */
	return count;
}

/*
 * Sends a request to the network's daemon.
 */
int
keiland_network_request(
	struct keiland_network *network,
	unsigned request,
	const char *ssid)
{
	int error;

	/* Sends through the backend; the request numbers are the same on both sides. */
	error = kl_backend_network_request(network->backend, request, ssid);
	if (error != 0)
		return error;

	/* Succeeded: the answer arrives through the updates. */
	return 0;
}

/*
 * Tells the request outstanding, or the one that finished and its error.
 */
unsigned
keiland_network_get_request(
	const struct keiland_network *network,
	int *error)
{
	unsigned request;

	/* Asks the backend; the request numbers are the same on both sides. */
	request = kl_backend_network_get_request(network->backend, error);

	/* Reports the request. */
	return request;
}

/*
 * Copies up to capacity interfaces.
 */
size_t
keiland_network_get_links(
	struct keiland_network_link *links,
	size_t capacity)
{
	struct kl_backend_network_link found[KL_BACKEND_NETWORK_LINKS_MAX];
	size_t count;
	size_t copied;
	size_t index;

	/* Asks the backend for every interface it reports. */
	count = kl_backend_network_get_links(found, KL_BACKEND_NETWORK_LINKS_MAX);

	/* Copies as many as the caller has room for and the backend filled. */
	copied = count;
	if (copied > KL_BACKEND_NETWORK_LINKS_MAX)
		copied = KL_BACKEND_NETWORK_LINKS_MAX;
	if (copied > capacity)
		copied = capacity;

	/* Converts each interface into the old structure. */
	for (index = 0; index < copied; index++)
		network_copy_link(&links[index], &found[index]);

	/* Reports how many interfaces there are. */
	return count;
}

/*
 * Copies up to capacity DNS servers.
 */
size_t
keiland_network_get_dns(
	char (*servers)[KEILAND_NETWORK_ADDRESS_MAX],
	size_t capacity)
{
	size_t count;

	/* Asks the backend; an address's text has the same size on both sides. */
	count = kl_backend_network_get_dns(servers, capacity);

	/* Reports how many servers were copied. */
	return count;
}

/*
 * Saves the key of a Wi-Fi network in the user's credential store.
 */
int
keiland_network_save_key(
	const char *ssid,
	const char *key)
{
	int error;

	/* Saves through the backend. */
	error = kl_backend_network_save_key(ssid, key);
	if (error != 0)
		return error;

	/* Succeeded: the network is joined by itself from now on. */
	return 0;
}

/*
 * Copies up to capacity SSIDs the user has saved keys for.
 */
size_t
keiland_network_get_saved(
	char (*ssids)[KEILAND_NETWORK_SSID_MAX],
	size_t capacity)
{
	size_t count;

	/* Asks the backend; an SSID's text has the same size on both sides. */
	count = kl_backend_network_get_saved(ssids, capacity);

	/* Reports how many SSIDs are saved. */
	return count;
}

/* Copies one network a scan found into the old structure. */
static void
network_copy_ap(
	struct keiland_network_ap *to,
	const struct kl_backend_network_ap *from)
{
	/* Copies the SSID, the signal and whether it asks for a key. */
	memcpy(to->ssid, from->ssid, sizeof(to->ssid));
	to->rssi = from->rssi;
	to->secured = from->secured;
}

/* Copies one interface into the old structure. */
static void
network_copy_link(
	struct keiland_network_link *to,
	const struct kl_backend_network_link *from)
{
	/* Copies the name and the interface's flags. */
	memcpy(to->name, from->name, sizeof(to->name));
	to->up = from->up;
	to->running = from->running;
	to->loopback = from->loopback;

	/* Copies the addresses, the hardware address and the MTU. */
	memcpy(to->address, from->address, sizeof(to->address));
	memcpy(to->netmask, from->netmask, sizeof(to->netmask));
	memcpy(to->hardware, from->hardware, sizeof(to->hardware));
	to->mtu = from->mtu;

	/* Copies the byte counters. */
	to->received_bytes = from->received_bytes;
	to->sent_bytes = from->sent_bytes;
}
