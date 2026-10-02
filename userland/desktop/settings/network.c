/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The network's backend of Settings (ws089-p003): the daemon's state and
 * scans through libkeiland's watch (keiland_network_*), the interfaces, the
 * DNS servers and the saved networks read by libkeiland, and the requests
 * the network pages make -- the Wi-Fi switch, a scan, a join, a disconnect,
 * and a join with a new key (the key saved, the daemon told, the network
 * joined, one request after the other).
 *
 * Nothing here waits: the main loop calls se_network_poll often while a
 * network page is shown or a request is outstanding, and the answers
 * arrive through it.  Settings never speaks networkd's protocol itself
 * (plan/ws089/design.md section 6).
 */

#include "settings.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* How often the interfaces and the activity are read, and the DNS servers and the saved networks, in milliseconds. */
#define NETWORK_LINKS_MS	1000U
#define NETWORK_FILES_MS	5000U

/* How long a scan is fresh while a Wi-Fi list is shown, in milliseconds. */
#define NETWORK_SCAN_MS		20000U

/* How often the main loop polls while a network page is shown or a request is outstanding, in milliseconds. */
#define NETWORK_POLL_MS		250

static void network_read_files(struct se_app *app);
static int network_read_links(struct se_app *app, uint64_t now);
static void network_finished(struct se_app *app);
static void network_send(struct se_app *app, unsigned request, const char *ssid);
static void network_message(struct se_app *app, int bad, const char *format, const char *ssid);
static int network_page_shown(const struct se_app *app);
static int network_radio_on(const struct se_network *network);

/*
 * Starts watching the network (a daemon that is not running yet is found
 * by a later poll).
 */
void
se_network_open(
	struct se_app *app)
{
	struct se_network *network;

	/* The watch. */
	network = &app->network;
	network->handle = keiland_network_open();
	if (network->handle == NULL) {
		se_log("NETWORK none errno=%d", errno);
		return;
	}

	/* The watch is live, and no request is outstanding. */
	network->live = 1;
	network->request = KEILAND_NETWORK_REQUEST_NONE;

	/* What is known without the daemon: the interfaces, the servers and the saved networks. */
	(void)network_read_links(app, app->now);
	network_read_files(app);
	se_log("NETWORK open links=%lu dns=%lu saved=%lu", (unsigned long)network->link_count, (unsigned long)network->dns_count, (unsigned long)network->saved_count);
}

/*
 * Reads what the daemon reported, and the interfaces now and then; asks
 * for a scan when a Wi-Fi list is shown and the last one is old.
 */
void
se_network_poll(
	struct se_app *app,
	uint64_t now)
{
	struct se_network *network;
	unsigned changed;
	int addresses;
	int shown;
	int error;
	int radio;

	/* Nothing to poll without a watch. */
	network = &app->network;
	if (network->live == 0)
		return;

	/* What arrived from the daemon. */
	shown = network_page_shown(app);
	error = keiland_network_update(network->handle, &changed);
	if (error != 0)
		changed = 0;

	/* A new state. */
	if ((changed & KEILAND_NETWORK_CHANGED_STATE) != 0U) {
		keiland_network_get_state(network->handle, &network->state);
		se_log("NETWORK state reachable=%u connected=%u kind=%u interface=%s wifi=%u ssid=%s", network->state.reachable, network->state.connected, network->state.kind, network->state.interface, network->state.wifi, network->state.ssid);
		app->dirty = 1;
	}

	/* A new scan. */
	if ((changed & KEILAND_NETWORK_CHANGED_SCAN) != 0U) {
		network->scan_count = keiland_network_get_scan(network->handle, network->scan, SE_NETWORK_SCAN);
		if (network->scan_count > SE_NETWORK_SCAN)
			network->scan_count = SE_NETWORK_SCAN;
		network->scanned_at = now;
		se_log("NETWORK scan count=%lu", (unsigned long)network->scan_count);
		app->dirty = 1;
	}

	/* A request that finished. */
	if ((changed & KEILAND_NETWORK_CHANGED_DONE) != 0U)
		network_finished(app);

	/* The interfaces and the activity, once a second; a shown page is drawn again, and Home when an address changed. */
	if (now - network->sampled_at >= NETWORK_LINKS_MS) {
		addresses = network_read_links(app, now);
		if (shown != 0)
			app->dirty = 1;
		if (addresses != 0 && app->page == SE_PAGE_HOME)
			app->dirty = 1;
	}

	/* The servers and the saved networks, now and then. */
	if (now - network->polled_at >= NETWORK_FILES_MS) {
		network->polled_at = now;
		network_read_files(app);
	}

	/* A fresh scan while a Wi-Fi list is shown and the radio is on. */
	if (shown == 0)
		return;
	if (network->request != KEILAND_NETWORK_REQUEST_NONE)
		return;
	radio = network_radio_on(network);
	if (radio == 0)
		return;
	if (network->scanned_at != 0U && now - network->scanned_at < NETWORK_SCAN_MS)
		return;
	network->scanned_at = now;
	network_send(app, KEILAND_NETWORK_REQUEST_SCAN, NULL);
}

/*
 * Reports how long the main loop may sleep before the network wants a
 * poll: a short while while a network page is shown or a request is
 * outstanding, else -1 (the idle limit).
 */
int
se_network_wait(
	struct se_app *app)
{
	int shown;

	/* Without a watch nothing is due. */
	if (app->network.live == 0)
		return -1;

	/* An answer awaited, or a page that shows the network. */
	shown = network_page_shown(app);
	if (shown != 0 || app->network.request != KEILAND_NETWORK_REQUEST_NONE)
		return NETWORK_POLL_MS;

	/* Nothing due. */
	return -1;
}

/*
 * Stops watching the network and wipes a key being typed.
 */
void
se_network_close(
	struct se_app *app)
{
	/* The key, then the watch. */
	se_field_clear(&app->network.key);
	keiland_network_close(app->network.handle);
	app->network.handle = NULL;
	app->network.live = 0;
}

/*
 * Turns the Wi-Fi on or off.
 */
void
se_network_wifi(
	struct se_app *app,
	int on)
{
	/* The switch's request. */
	if (on != 0) {
		network_send(app, KEILAND_NETWORK_REQUEST_WIFI_ON, NULL);
	} else {
		network_send(app, KEILAND_NETWORK_REQUEST_WIFI_OFF, NULL);
	}
}

/*
 * Asks for a scan of the networks around.
 */
void
se_network_scan(
	struct se_app *app)
{
	/* The scan's request. */
	app->network.scanned_at = app->now;
	network_send(app, KEILAND_NETWORK_REQUEST_SCAN, NULL);
}

/*
 * Joins a network whose key is saved.
 */
void
se_network_join(
	struct se_app *app,
	const char *ssid)
{
	/* The join, named. */
	(void)snprintf(app->network.join_ssid, sizeof(app->network.join_ssid), "%s", ssid);
	app->network.join_step = SE_JOIN_CONNECT;
	network_send(app, KEILAND_NETWORK_REQUEST_JOIN, ssid);
	if (app->network.request == KEILAND_NETWORK_REQUEST_JOIN)
		network_message(app, 0, "Joining %s...", ssid);
}

/*
 * Joins a network with a key just typed: the key is saved in the user's
 * store, the daemon is told, and the network is joined when it answered.
 */
void
se_network_join_key(
	struct se_app *app,
	const char *ssid,
	const char *key)
{
	struct se_network *network;
	int error;

	/* One request at a time. */
	network = &app->network;
	if (network->request != KEILAND_NETWORK_REQUEST_NONE) {
		network_message(app, 1, "Wait for the network to answer, then try %s again.", ssid);
		return;
	}

	/* The key, saved (the store checks its length too); a refusal says why. */
	error = keiland_network_save_key(ssid, key);
	if (error == EINVAL) {
		network_message(app, 1, "The key of %s must be 8 to 63 characters.", ssid);
		return;
	} else if (error != 0) {
		se_log("NETWORK save-key failed errno=%d", error);
		network_message(app, 1, "The key of %s could not be saved.", ssid);
		return;
	}

	/* The log line the tests read (never the key). */
	se_log("NETWORK save-key ok");

	/* The saved networks as they are now, and the daemon told; the join follows its answer. */
	network_read_files(app);
	(void)snprintf(network->join_ssid, sizeof(network->join_ssid), "%s", ssid);
	network->join_step = SE_JOIN_PROFILES;
	network_send(app, KEILAND_NETWORK_REQUEST_PROFILES, NULL);
	if (network->request == KEILAND_NETWORK_REQUEST_PROFILES)
		network_message(app, 0, "Joining %s...", ssid);
}

/*
 * Leaves the Wi-Fi network the machine is on.
 */
void
se_network_disconnect(
	struct se_app *app)
{
	/* The disconnect's request. */
	network_send(app, KEILAND_NETWORK_REQUEST_DISCONNECT, NULL);
}

/* Reads the DNS servers and the saved networks. */
static void
network_read_files(
	struct se_app *app)
{
	struct se_network *network;
	size_t count;

	/* The servers. */
	network = &app->network;
	network->dns_count = keiland_network_get_dns(network->dns, SE_NETWORK_DNS);

	/* The saved networks, as many as are kept. */
	count = keiland_network_get_saved(network->saved, SE_NETWORK_SAVED);
	if (count > SE_NETWORK_SAVED)
		count = SE_NETWORK_SAVED;
	network->saved_count = count;
}

/*
 * Reads the interfaces, and records a second of activity (the bytes of
 * every interface but the loopback).  Returns 1 when the interfaces or
 * their addresses differ from the last reading (Home shows an address).
 */
static int
network_read_links(
	struct se_app *app,
	uint64_t now)
{
	struct se_network *network;
	char addresses[SE_NETWORK_LINKS][KEILAND_NETWORK_ADDRESS_MAX];
	size_t old_count;
	int differs;
	int moved;
	uint64_t received;
	uint64_t sent;
	uint64_t elapsed;
	uint64_t received_rate;
	uint64_t sent_rate;
	size_t count;
	size_t index;

	/* The addresses of the last reading, to tell whether they moved. */
	network = &app->network;
	old_count = network->link_count;
	for (index = 0; index < old_count; index++)
		(void)snprintf(addresses[index], sizeof(addresses[index]), "%s", network->links[index].address);

	/* The interfaces, as many as are kept. */
	count = keiland_network_get_links(network->links, SE_NETWORK_LINKS);
	if (count > SE_NETWORK_LINKS)
		count = SE_NETWORK_LINKS;
	network->link_count = count;

	/* Interfaces that came or went count as moved. */
	moved = 0;
	if (count != old_count)
		moved = 1;

	/* So does an address that changed. */
	for (index = 0; moved == 0 && index < count; index++) {
		differs = strcmp(addresses[index], network->links[index].address);
		if (differs != 0)
			moved = 1;
	}

	/* The bytes of every interface but the loopback. */
	received = 0;
	sent = 0;
	for (index = 0; index < count; index++) {
		if (network->links[index].loopback != 0)
			continue;
		received += network->links[index].received_bytes;
		sent += network->links[index].sent_bytes;
	}

	/* The rates since the last sample (none for the first, or after a counter went back). */
	elapsed = now - network->sampled_at;
	if (network->sampled_at != 0U &&
	    elapsed > 0U &&
	    received >= network->received_total &&
	    sent >= network->sent_total) {
		received_rate = (received - network->received_total) * 1000U / elapsed;
		sent_rate = (sent - network->sent_total) * 1000U / elapsed;
		if (received_rate > 0xffffffffU)
			received_rate = 0xffffffffU;
		if (sent_rate > 0xffffffffU)
			sent_rate = 0xffffffffU;
		network->received[network->usage_next] = (uint32_t)received_rate;
		network->sent[network->usage_next] = (uint32_t)sent_rate;
		network->usage_next = (network->usage_next + 1U) % SE_USAGE_SAMPLES;
		if (network->usage_count < SE_USAGE_SAMPLES)
			network->usage_count++;
	}

	/* The totals and the time of this sample. */
	network->received_total = received;
	network->sent_total = sent;
	network->sampled_at = now;

	/* Succeeded: whether the interfaces moved. */
	return moved;
}

/* Carries on after a request finished: the next step of a join, or a message about the outcome. */
static void
network_finished(
	struct se_app *app)
{
	struct se_network *network;
	unsigned request;
	int error;

	/* The request that finished and how. */
	network = &app->network;
	request = keiland_network_get_request(network->handle, &error);
	network->request = KEILAND_NETWORK_REQUEST_NONE;
	se_log("NETWORK done request=%u errno=%d", request, error);
	app->dirty = 1;

	/* The daemon read the new key: the join follows. */
	if (request == KEILAND_NETWORK_REQUEST_PROFILES && network->join_step == SE_JOIN_PROFILES) {
		network->join_step = SE_JOIN_CONNECT;
		network_send(app, KEILAND_NETWORK_REQUEST_JOIN, network->join_ssid);
		return;
	}

	/* A join's outcome: the key form closes when it worked. */
	if (request == KEILAND_NETWORK_REQUEST_JOIN) {
		network->join_step = SE_JOIN_NONE;
		if (error == 0) {
			network->key_ssid[0] = '\0';
			se_field_clear(&network->key);
			network_message(app, 0, "Connected to %s.", network->join_ssid);
		} else if (error == ENOENT) {
			network_message(app, 1, "%s has no saved key.", network->join_ssid);
		} else if (error == EPERM && network->state.wifi == KEILAND_WIFI_OFF) {
			/* networkd refuses a join while Wi-Fi is off. */
			network_message(app, 1, "Wi-Fi is off. Turn it on to join %s.", network->join_ssid);
		} else if (error == EPERM) {
			/* Only root and the network group may control Wi-Fi (2026-10-02, ws005-p019). */
			network_message(app, 1, "This account may not control Wi-Fi, so it cannot join %s. Ask an administrator to add it to the network group.", network->join_ssid);
		} else {
			network_message(app, 1, "Could not join %s. Check the key and that the network is in reach.", network->join_ssid);
		}

		/* The join is over. */
		return;
	}

	/* Anything else that failed says so. */
	if (error != 0) {
		network->join_step = SE_JOIN_NONE;
		network_message(app, 1, "The network could not do that (%s).", strerror(error));
		return;
	}

	/* The switch's and the disconnect's success need no words; the state shows them. */
	if (request == KEILAND_NETWORK_REQUEST_WIFI_OFF || request == KEILAND_NETWORK_REQUEST_DISCONNECT)
		network->message[0] = '\0';
}

/* Sends a request, unless one is outstanding or there is no watch; a refusal is shown. */
static void
network_send(
	struct se_app *app,
	unsigned request,
	const char *ssid)
{
	struct se_network *network;
	int error;

	/* Without a watch nothing is sent. */
	network = &app->network;
	if (network->live == 0) {
		network_message(app, 1, "%s", "The network service is not running.");
		return;
	}

	/* One request at a time. */
	if (network->request != KEILAND_NETWORK_REQUEST_NONE) {
		network_message(app, 1, "%s", "Wait for the network to answer, then try again.");
		return;
	}

	/* The request. */
	error = keiland_network_request(network->handle, request, ssid);
	if (error != 0) {
		se_log("NETWORK request=%u refused errno=%d", request, error);
		network->join_step = SE_JOIN_NONE;
		network_message(app, 1, "The network could not do that (%s).", strerror(error));
		return;
	}

	/* Outstanding until its answer. */
	network->request = request;
	se_log("NETWORK request=%u sent", request);
	app->dirty = 1;
}

/* Sets the message the network pages show, with an SSID or another text put in (bad: shown in red). */
static void
network_message(
	struct se_app *app,
	int bad,
	const char *format,
	const char *ssid)
{
	/* The message, and its colour. */
	(void)snprintf(app->network.message, sizeof(app->network.message), format, ssid);
	app->network.message_bad = bad;
	app->dirty = 1;
	se_log("NETWORK message bad=%d text=%s", bad, app->network.message);
}

/* Tells whether a page that shows the network is shown. */
static int
network_page_shown(
	const struct se_app *app)
{
	/* Network, Wi-Fi and Ethernet show it. */
	if (app->page == SE_PAGE_NETWORK)
		return 1;
	if (app->page == SE_PAGE_WIFI)
		return 1;
	if (app->page == SE_PAGE_ETHERNET)
		return 1;

	/* No other page does. */
	return 0;
}

/* Tells whether the Wi-Fi radio is on (searching, joining, connected, or on and left unconnected). */
static int
network_radio_on(
	const struct se_network *network)
{
	/* A missing radio, and one turned off, are not on. */
	if (network->state.wifi == KEILAND_WIFI_ABSENT)
		return 0;
	if (network->state.wifi == KEILAND_WIFI_OFF)
		return 0;

	/* Every other state has the radio on. */
	return 1;
}
