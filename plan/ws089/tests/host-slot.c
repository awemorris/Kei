/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws089-p012 (C1): checks Settings' network requests on the host against a
 * pretend libkeiland that carries one request at a time, as the real one
 * does.  A switch, a disconnect or a join asked for while another request
 * (a scan, usually) is out must wait in the slot and be sent when that one
 * is answered; a scan is not kept; a later ask replaces what waited; a join
 * with a new key saves the key at once and is told to the daemon after the
 * outstanding request.  Built and run by host-slot.sh; the last line says
 * host-slot: PASS or FAIL.
 */

#include "settings.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* How many requests the pretend daemon remembers being sent. */
#define SLOT_SENT_MAX		16

/*
 * The pretend libkeiland: the request outstanding (NONE when none), the
 * one that finished and how, whether a finish is to be reported, and the
 * requests sent in order with the network a join named.  It lives for the
 * whole test and is reset before each case.
 */
struct slot_daemon {
	unsigned outstanding;
	unsigned finished;
	int finished_error;
	unsigned report;
	unsigned sent[SLOT_SENT_MAX];
	char sent_ssid[SLOT_SENT_MAX][KEILAND_NETWORK_SSID_MAX];
	unsigned sent_count;
	int key_saved;
};

/* The pretend daemon of the case being run. */
static struct slot_daemon slot_daemon;

/* The handle the pretend libkeiland gives out (its contents are never read). */
static char slot_handle;

/* How many checks failed. */
static int slot_failures;

static void slot_reset(struct se_app *app);
static void slot_finish(struct se_app *app, int error);
static void slot_expect_sent(const char *name, unsigned count, const unsigned *requests);
static void slot_expect(const char *name, int condition);

/*
 * Runs the cases and prints each check's verdict and the total.
 */
int
main(void)
{
	static struct se_app app;
	static const unsigned scan_then_off[] = { KEILAND_NETWORK_REQUEST_SCAN, KEILAND_NETWORK_REQUEST_WIFI_OFF };
	static const unsigned scan_then_join[] = { KEILAND_NETWORK_REQUEST_SCAN, KEILAND_NETWORK_REQUEST_JOIN };
	static const unsigned scan_profiles_join[] = { KEILAND_NETWORK_REQUEST_SCAN, KEILAND_NETWORK_REQUEST_PROFILES, KEILAND_NETWORK_REQUEST_JOIN };
	static const unsigned join_then_join[] = { KEILAND_NETWORK_REQUEST_JOIN, KEILAND_NETWORK_REQUEST_JOIN };
	static const unsigned scan_only[] = { KEILAND_NETWORK_REQUEST_SCAN };
	static const unsigned scan_then_disconnect[] = { KEILAND_NETWORK_REQUEST_SCAN, KEILAND_NETWORK_REQUEST_DISCONNECT };

	/* 1. The switch pressed during a scan is sent after the scan's answer. */
	slot_reset(&app);
	se_network_scan(&app);
	se_network_wifi(&app, 0);
	slot_expect("1 switch waits", app.network.pending_request == KEILAND_NETWORK_REQUEST_WIFI_OFF);
	slot_expect("1 no busy refusal", app.network.message_bad == 0);
	slot_finish(&app, 0);
	slot_expect_sent("1 scan then off", 2U, scan_then_off);

	/* 2. A saved network joined during a scan: Joining is said at once, the join follows the scan. */
	slot_reset(&app);
	se_network_scan(&app);
	se_network_join(&app, "Cafe Guest");
	slot_expect("2 joining said", strcmp(app.network.message, "Joining Cafe Guest...") == 0);
	slot_finish(&app, 0);
	slot_expect_sent("2 scan then join", 2U, scan_then_join);
	slot_expect("2 join names the network", strcmp(slot_daemon.sent_ssid[1], "Cafe Guest") == 0);
	slot_finish(&app, 0);
	slot_expect("2 connected", strcmp(app.network.message, "Connected to Cafe Guest.") == 0);

	/* 3. A new key during a scan: saved at once, the daemon told after the scan, then the join. */
	slot_reset(&app);
	se_network_scan(&app);
	se_network_join_key(&app, "Neighbor 5G", "correct horse");
	slot_expect("3 key saved at once", slot_daemon.key_saved == 1);
	slot_expect("3 profiles wait", app.network.pending_request == KEILAND_NETWORK_REQUEST_PROFILES);
	slot_finish(&app, 0);
	slot_finish(&app, 0);
	slot_expect_sent("3 scan, profiles, join", 3U, scan_profiles_join);
	slot_expect("3 join names the network", strcmp(slot_daemon.sent_ssid[2], "Neighbor 5G") == 0);
	slot_finish(&app, 0);
	slot_expect("3 connected", strcmp(app.network.message, "Connected to Neighbor 5G.") == 0);

	/* 4. A second join while the first is out: the first's failure is said, then the second goes. */
	slot_reset(&app);
	se_network_join(&app, "OSC Venue");
	se_network_join(&app, "Kei Lab");
	slot_finish(&app, EIO);
	slot_expect_sent("4 join then join", 2U, join_then_join);
	slot_expect("4 second join named", strcmp(slot_daemon.sent_ssid[1], "Kei Lab") == 0);
	slot_expect("4 join under way", strcmp(app.network.join_ssid, "Kei Lab") == 0);
	slot_finish(&app, 0);
	slot_expect("4 connected", strcmp(app.network.message, "Connected to Kei Lab.") == 0);

	/* 5. The Scan button during a scan is not kept. */
	slot_reset(&app);
	se_network_scan(&app);
	se_network_scan(&app);
	slot_expect("5 scan not kept", app.network.pending_request == KEILAND_NETWORK_REQUEST_NONE);
	slot_finish(&app, 0);
	slot_expect_sent("5 one scan", 1U, scan_only);

	/* 6. Two presses during a scan: the later one waits in place of the first. */
	slot_reset(&app);
	se_network_scan(&app);
	se_network_wifi(&app, 0);
	se_network_disconnect(&app);
	slot_finish(&app, 0);
	slot_expect_sent("6 the later press", 2U, scan_then_disconnect);

	/* The verdict. */
	if (slot_failures != 0) {
		printf("host-slot: FAIL (%d)\n", slot_failures);
		return 1;
	}

	/* Succeeded: every case passed. */
	printf("host-slot: PASS\n");
	return 0;
}

/* Starts a case: a fresh pretend daemon and a fresh Settings watching it, on Home (no periodic scan). */
static void
slot_reset(
	struct se_app *app)
{
	/* The daemon remembers nothing. */
	memset(&slot_daemon, 0, sizeof(slot_daemon));

	/* Settings watches it, the Wi-Fi on, nothing shown that scans by itself. */
	memset(app, 0, sizeof(*app));
	app->page = SE_PAGE_HOME;
	se_network_open(app);
	app->network.state.reachable = 1;
	app->network.state.wifi = KEILAND_WIFI_CONNECTED;
}

/* Answers the outstanding request with an errno value, and lets Settings read the answer. */
static void
slot_finish(
	struct se_app *app,
	int error)
{
	/* The answer, reported at the next update. */
	slot_daemon.finished = slot_daemon.outstanding;
	slot_daemon.finished_error = error;
	slot_daemon.outstanding = KEILAND_NETWORK_REQUEST_NONE;
	slot_daemon.report = KEILAND_NETWORK_CHANGED_DONE;

	/* Settings reads it. */
	app->now += 1000U;
	se_network_poll(app, app->now);
}

/* Checks the requests sent so far, in order. */
static void
slot_expect_sent(
	const char *name,
	unsigned count,
	const unsigned *requests)
{
	unsigned index;
	int same;

	/* As many as expected, each the one expected. */
	same = 1;
	if (slot_daemon.sent_count != count)
		same = 0;
	for (index = 0; index < count && index < slot_daemon.sent_count; index++) {
		if (slot_daemon.sent[index] != requests[index])
			same = 0;
	}

	/* The verdict, with what was sent when it differs. */
	slot_expect(name, same);
	if (same != 0)
		return;
	for (index = 0; index < slot_daemon.sent_count; index++)
		printf("  sent %u: %u %s\n", index, slot_daemon.sent[index], slot_daemon.sent_ssid[index]);
}

/* Records one check's verdict. */
static void
slot_expect(
	const char *name,
	int condition)
{
	/* A failed check counts. */
	if (condition == 0) {
		printf("%s: FAIL\n", name);
		slot_failures++;
		return;
	}

	/* A passed check. */
	printf("%s: ok\n", name);
}

/*
 * The pretend libkeiland: a handle.
 */
struct keiland_network *
keiland_network_open(void)
{
	/* Any non-NULL handle. */
	return (struct keiland_network *)(void *)&slot_handle;
}

/*
 * The pretend libkeiland: nothing to close.
 */
void
keiland_network_close(
	struct keiland_network *network)
{
	/* The handle is static. */
	(void)network;
}

/*
 * The pretend libkeiland: reports a finish once.
 */
int
keiland_network_update(
	struct keiland_network *network,
	unsigned *changed)
{
	/* What changed since the last update. */
	(void)network;
	*changed = slot_daemon.report;
	slot_daemon.report = 0U;

	/* Succeeded. */
	return 0;
}

/*
 * The pretend libkeiland: the state never changes (the test sets it).
 */
void
keiland_network_get_state(
	const struct keiland_network *network,
	struct keiland_network_state *state)
{
	/* Nothing to copy. */
	(void)network;
	(void)state;
}

/*
 * The pretend libkeiland: no networks around.
 */
size_t
keiland_network_get_scan(
	const struct keiland_network *network,
	struct keiland_network_ap *aps,
	size_t capacity)
{
	/* None. */
	(void)network;
	(void)aps;
	(void)capacity;
	return 0;
}

/*
 * The pretend libkeiland: one request at a time, EBUSY otherwise, as the real one.
 */
int
keiland_network_request(
	struct keiland_network *network,
	unsigned request,
	const char *ssid)
{
	/* A request behind another is refused. */
	(void)network;
	if (slot_daemon.outstanding != KEILAND_NETWORK_REQUEST_NONE)
		return EBUSY;

	/* The request is out, and remembered. */
	slot_daemon.outstanding = request;
	if (slot_daemon.sent_count < SLOT_SENT_MAX) {
		slot_daemon.sent[slot_daemon.sent_count] = request;
		slot_daemon.sent_ssid[slot_daemon.sent_count][0] = '\0';
		if (ssid != NULL)
			(void)snprintf(slot_daemon.sent_ssid[slot_daemon.sent_count], KEILAND_NETWORK_SSID_MAX, "%s", ssid);
		slot_daemon.sent_count++;
	}

	/* Succeeded: sent. */
	return 0;
}

/*
 * The pretend libkeiland: the request out, or the one that finished and how.
 */
unsigned
keiland_network_get_request(
	const struct keiland_network *network,
	int *error)
{
	/* The outstanding one while it is out. */
	(void)network;
	*error = 0;
	if (slot_daemon.outstanding != KEILAND_NETWORK_REQUEST_NONE)
		return slot_daemon.outstanding;

	/* The finished one. */
	*error = slot_daemon.finished_error;
	return slot_daemon.finished;
}

/*
 * The pretend libkeiland: no interfaces.
 */
size_t
keiland_network_get_links(
	struct keiland_network_link *links,
	size_t capacity)
{
	/* None. */
	(void)links;
	(void)capacity;
	return 0;
}

/*
 * The pretend libkeiland: no DNS servers.
 */
size_t
keiland_network_get_dns(
	char (*servers)[KEILAND_NETWORK_ADDRESS_MAX],
	size_t capacity)
{
	/* None. */
	(void)servers;
	(void)capacity;
	return 0;
}

/*
 * The pretend libkeiland: a key is saved (the test only counts it; it is never kept).
 */
int
keiland_network_save_key(
	const char *ssid,
	const char *key)
{
	/* Counted. */
	(void)ssid;
	(void)key;
	slot_daemon.key_saved++;

	/* Succeeded. */
	return 0;
}

/*
 * The pretend libkeiland: no saved networks.
 */
size_t
keiland_network_get_saved(
	char (*ssids)[KEILAND_NETWORK_SSID_MAX],
	size_t capacity)
{
	/* None. */
	(void)ssids;
	(void)capacity;
	return 0;
}

/*
 * Settings' text field, wiped (the test types no key into one).
 */
void
se_field_clear(
	struct se_field *field)
{
	/* The field's bytes. */
	memset(field, 0, sizeof(*field));
}

/*
 * Settings' log, to standard error as the program's.
 */
void
se_log(
	const char *format,
	...)
{
	va_list arguments;

	/* The line. */
	fputs("ZSETTINGS ", stderr);
	va_start(arguments, format);
	vfprintf(stderr, format, arguments);
	va_end(arguments);
	fputc('\n', stderr);
}
