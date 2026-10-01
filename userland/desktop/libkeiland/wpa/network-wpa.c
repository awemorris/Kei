/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The absent Wi-Fi service, placeholder until ws105-p010.
 */

#include <keiland.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* One caller-owned subscription, kept from open until close without a service. */
struct keiland_network {
	struct keiland_network_state state;
};

/*
 * Allocates a network subscription even when the service is absent.
 */
struct keiland_network *
keiland_network_open(
	void)
{
	struct keiland_network *network;

	/* Keeps a valid subscription for later service discovery. */
	network = calloc(1, sizeof(*network));
	if (network == NULL)
		return NULL;

	/* Succeeded: the caller owns an absent-service subscription. */
	return network;
}

/*
 * Releases a network subscription.
 */
void
keiland_network_close(
	struct keiland_network *network)
{
	/* Releases the caller's record; a missing record is harmless. */
	free(network);

	/* Succeeded: the subscription is no longer owned. */
	return;
}

/*
 * Reports that an absent network service has no changes.
 */
int
keiland_network_update(
	struct keiland_network *network,
	unsigned *changed)
{
	/* Requires a destination for the change mask. */
	if (changed == NULL)
		return EINVAL;

	/* An absent service cannot publish changes. */
	*changed = 0U;
	if (network == NULL)
		return EINVAL;

	/* Succeeded: the service remains absent. */
	return 0;
}

/*
 * Copies the last known network state.
 */
void
keiland_network_get_state(
	const struct keiland_network *network,
	struct keiland_network_state *state)
{
	/* A missing subscription has an empty state. */
	memset(state, 0, sizeof(*state));
	if (network == NULL)
		return;

	/* Publishes the subscription's unchanged state. */
	*state = network->state;

	/* Succeeded: the caller holds the state. */
	return;
}

/*
 * Reports an empty scan while the service is absent.
 */
size_t
keiland_network_get_scan(
	const struct keiland_network *network,
	struct keiland_network_ap *aps,
	size_t capacity)
{
	/* There are no access points to copy. */
	(void)network;
	(void)aps;
	(void)capacity;

	/* Succeeded: zero scan entries are available. */
	return 0;
}

/*
 * Refuses requests while the service is absent.
 */
int
keiland_network_request(
	struct keiland_network *network,
	unsigned request,
	const char *ssid)
{
	/* Requires a subscription before considering a request. */
	if (network == NULL)
		return EINVAL;

	/* No request can be sent to the absent service. */
	(void)request;
	(void)ssid;

	/* Reports the missing connection. */
	return ENOTCONN;
}

/*
 * Reports that no network request is outstanding.
 */
unsigned
keiland_network_get_request(
	const struct keiland_network *network,
	int *error)
{
	/* An absent service has no pending or finished request. */
	(void)network;
	if (error != NULL)
		*error = 0;

	/* Succeeded: no request is outstanding. */
	return KEILAND_NETWORK_REQUEST_NONE;
}
