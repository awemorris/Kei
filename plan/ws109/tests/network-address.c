/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks the selected OS's Unix-domain address packing against its actual kernel.
 */
#include "userland/desktop/libkeiland/wpa/network-wpa.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int probe_address(void);

/*
 * Runs a native datagram round trip inside one owned private temporary directory.
 */
int
main(
	void)
{
	int error;

	/* The kernel must accept the same address extent used by the actual WPA connection. */
	error = probe_address();
	if (error != 0) {
		(void)fprintf(stderr, "native socket address failed errno=%d\n", error);
		return 1;
	}

	/* Succeeded: the selected native Unix-domain address works with real datagrams. */
	(void)printf("PASS native Unix-domain address bind/connect/send/receive\n");
	return 0;
}

/* Sends a real datagram through a socket bound to its own native endpoint. */
static int
probe_address(
	void)
{
	struct sockaddr_un address;
	char directory[] = "/tmp/ws109-address-XXXXXX";
	char path[128];
	char reply[32];
	char *owned;
	socklen_t extent;
	ssize_t bytes;
	int descriptor;
	int error;

	/* Acquires a private directory so no guessed public socket can be overwritten. */
	owned = mkdtemp(directory);
	if (owned == NULL)
		return errno;

	/* Owns this pathname exclusively until the directory is released. */
	(void)snprintf(path, sizeof(path), "%s/socket", directory);
	descriptor = -1;
	error = kwpa_socket_address(&address, path, &extent);
	if (error != 0)
		goto cleanup;

	/* Opens a nonblocking socket to keep an unexpected missing datagram finite. */
	descriptor = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (descriptor < 0) {
		error = errno;
		goto cleanup;
	}

	/* Binds with the production helper's native byte extent. */
	error = bind(descriptor, (struct sockaddr *)&address, extent);
	if (error != 0) {
		error = errno;
		goto cleanup;
	}

	/* Selects the actual private kernel endpoint as the datagram peer. */
	error = connect(descriptor, (struct sockaddr *)&address, extent);
	if (error != 0) {
		error = errno;
		goto cleanup;
	}

	/* Publishes one complete marker datagram through the actual native socket. */
	bytes = send(descriptor, "ws109", sizeof("ws109"), 0);
	if (bytes < 0) {
		error = errno;
		goto cleanup;
	}

	/* A short datagram write cannot satisfy the round-trip contract. */
	if (bytes != sizeof("ws109")) {
		error = EPROTO;
		goto cleanup;
	}

	/* Reads the queued kernel datagram without relying on a mock address parser. */
	bytes = recv(descriptor, reply, sizeof(reply), 0);
	if (bytes < 0) {
		error = errno;
		goto cleanup;
	}

	/* Validates the complete marker before inspecting its bounded string. */
	if (bytes != sizeof("ws109")) {
		error = EPROTO;
		goto cleanup;
	}

	/* Exact bytes prove this endpoint rather than an unrelated shared socket replied. */
	error = memcmp(reply, "ws109", sizeof("ws109"));
	if (error != 0) {
		error = EPROTO;
		goto cleanup;
	}

cleanup:
	/* Releases only this probe's descriptor and private directory contents. */
	if (descriptor >= 0)
		(void)close(descriptor);
	(void)unlink(path);
	(void)rmdir(directory);

	/* Reports the concrete native socket failure after restoring all ownership. */
	if (error != 0)
		return error;

	/* Succeeded: the native kernel accepted and delivered the packed endpoint. */
	return 0;
}
