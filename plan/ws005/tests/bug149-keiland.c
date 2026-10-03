/*
 * ws005-p025 (BUG-149 (a)): libkeiland's network request against a slow daemon.
 * Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
 *
 * Links the real libkeiland network client (network-zedbsd.c) and the networkd
 * protocol, and stands a stand-in daemon on NETWORKD_SOCKET that keeps the
 * watch silent and answers a join only after BUG149_DELAY_SECONDS with the
 * refusal ENOENT (as for a network that is not there).  The request, written
 * and then shut down for writing, must finish with the daemon's ENOENT after
 * the delay, not with EIO before it.  Run as root: the stand-in replaces the
 * real daemon's socket name (the guest is a throwaway snapshot).
 *
 * The last line is "PASS keiland-slow-join ..." or "FAIL keiland-slow-join ...".
 */

#include <keiland.h>

#include "userland/base/net/protocol.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* How long the stand-in daemon takes to answer a request (above libkeiland's 2-second frame bound). */
#define BUG149_DELAY_SECONDS 3U

/* The most connections the stand-in daemon keeps open (watches). */
#define BUG149_KEPT_MAX 16

static long bug149_now_ms(void);
static int bug149_listen(void);
static void bug149_daemon(int listener);
static void bug149_answer(int connection, const struct networkd_protocol_header *request);

/* Starts the stand-in daemon, sends one join through libkeiland, and judges the outcome. */
int
main(void)
{
	struct keiland_network *network;
	struct timespec tick;
	unsigned changed;
	unsigned finished;
	long start;
	long elapsed;
	pid_t daemon;
	int listener;
	int error;
	int done;
	int i;

	/* The stand-in daemon on the daemon's socket name. */
	listener = bug149_listen();
	if (listener < 0) {
		printf("FAIL keiland-slow-join listen errno=%d\n", errno);
		return 1;
	}

	/* The stand-in runs in a child; the client side keeps no listener. */
	daemon = fork();
	if (daemon == 0)
		bug149_daemon(listener);
	(void)close(listener);

	/* The client, as Settings opens it, and a few updates for the watch. */
	tick.tv_sec = 0;
	tick.tv_nsec = 100000000L;
	network = keiland_network_open();
	if (network == NULL) {
		printf("FAIL keiland-slow-join open errno=%d\n", errno);
		(void)kill(daemon, SIGKILL);
		return 1;
	}

	/* A few updates, in which the watch connects. */
	for (i = 0; i < 5; i++) {
		(void)keiland_network_update(network, &changed);
		(void)nanosleep(&tick, NULL);
	}

	/* The join, then the updates every 100 ms until it finishes (at most 10 s). */
	start = bug149_now_ms();
	error = keiland_network_request(network, KEILAND_NETWORK_REQUEST_JOIN, "bug149-no-such-ssid");
	if (error != 0) {
		printf("FAIL keiland-slow-join request error=%d\n", error);
		(void)kill(daemon, SIGKILL);
		return 1;
	}

	/* The updates until the join is done. */
	done = 0;
	error = 0;
	finished = KEILAND_NETWORK_REQUEST_NONE;
	for (i = 0; i < 100 && !done; i++) {
		(void)keiland_network_update(network, &changed);
		if ((changed & KEILAND_NETWORK_CHANGED_DONE) != 0) {
			finished = keiland_network_get_request(network, &error);
			done = 1;
			break;
		}

		/* The next update in 100 ms. */
		(void)nanosleep(&tick, NULL);
	}

	/* The client and the stand-in are done. */
	elapsed = bug149_now_ms() - start;
	keiland_network_close(network);
	(void)kill(daemon, SIGKILL);
	(void)waitpid(daemon, NULL, 0);

	/* The daemon's own refusal, after its delay. */
	if (done && finished == KEILAND_NETWORK_REQUEST_JOIN && error == ENOENT &&
	    elapsed >= (long)BUG149_DELAY_SECONDS * 1000L - 300L) {
		printf("PASS keiland-slow-join updates=%d elapsed=%ldms error=%d\n", i, elapsed, error);
		return 0;
	}

	/* Anything else: EIO before the answer, no answer, or the wrong one. */
	printf("FAIL keiland-slow-join done=%d updates=%d elapsed=%ldms error=%d (want %d after %u s)\n",
	    done, i, elapsed, error, ENOENT, BUG149_DELAY_SECONDS);
	return 1;
}

/* The monotonic clock in milliseconds. */
static long
bug149_now_ms(void)
{
	struct timespec now;
	int failed;

	/* A failed clock reads as zero. */
	failed = clock_gettime(CLOCK_MONOTONIC, &now);
	if (failed != 0)
		return 0;

	/* Succeeded. */
	return (long)now.tv_sec * 1000L + now.tv_nsec / 1000000L;
}

/* Listens on the daemon's socket name in place of the real daemon. */
static int
bug149_listen(void)
{
	struct sockaddr_un address;
	int listener;
	int failed;

	/* A stream socket bound to NETWORKD_SOCKET. */
	listener = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listener < 0)
		return -1;
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	(void)snprintf(address.sun_path, sizeof(address.sun_path), "%s", NETWORKD_SOCKET);
	(void)unlink(NETWORKD_SOCKET);
	failed = bind(listener, (struct sockaddr *)&address, sizeof(address));
	if (failed != 0) {
		(void)close(listener);
		return -1;
	}

	/* Accepting connections. */
	failed = listen(listener, 8);
	if (failed != 0) {
		(void)close(listener);
		return -1;
	}

	/* Succeeded. */
	return listener;
}

/* Accepts connections forever: a watch is kept silent, a request is answered late. */
static void
bug149_daemon(
	int listener)
{
	struct networkd_protocol_header header;
	unsigned char payload[NETWORKD_RESPONSE_MAX];
	int kept[BUG149_KEPT_MAX];
	int kept_count;
	int connection;
	int failed;

	/* Each connection in turn. */
	kept_count = 0;
	for (;;) {
		connection = accept(listener, NULL, NULL);
		if (connection < 0)
			continue;

		/* The connection's one frame. */
		failed = networkd_protocol_read_frame(connection, &header, payload, sizeof(payload), sizeof(payload));
		if (failed != 0) {
			(void)close(connection);
			continue;
		}

		/* A watch: kept open and never answered, so no state arrives. */
		if (header.opcode == NETWORKD_OP_SUBSCRIBE) {
			if (kept_count < BUG149_KEPT_MAX) {
				kept[kept_count] = connection;
				kept_count++;
			} else {
				(void)close(connection);
			}

			/* The next connection. */
			continue;
		}

		/* A request: answered after the delay. */
		bug149_answer(connection, &header);
		(void)close(connection);
	}
}

/* Waits the delay, then refuses the request with ENOENT. */
static void
bug149_answer(
	int connection,
	const struct networkd_protocol_header *request)
{
	struct networkd_protocol_header header;
	struct networkd_field_writer writer;
	unsigned char payload[64];

	/* The daemon's slow work. */
	(void)sleep(BUG149_DELAY_SECONDS);

	/* The refusal: status ERROR, errno ENOENT, for the same request. */
	networkd_field_writer_init(&writer, payload, sizeof(payload));
	(void)networkd_field_write_u32(&writer, NETWORKD_FIELD_STATUS, NETWORKD_RESULT_ERROR);
	(void)networkd_field_write_u32(&writer, NETWORKD_FIELD_ERROR, ENOENT);
	header.request_id = request->request_id;
	header.opcode = request->opcode;
	header.payload_length = writer.used;
	(void)networkd_protocol_write_frame(connection, &header, payload);
}
