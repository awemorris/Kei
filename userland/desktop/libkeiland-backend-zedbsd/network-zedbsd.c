/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The desktop's view of the network (ws035-p013), over networkd.
 *
 * A watch is one connection to networkd that asked SUBSCRIBE and stays
 * open: networkd writes the state on it at once and again whenever it
 * changes.  The state is the text "net show" prints -- a line an interface,
 * "NAME static|unconfigured online|offline" -- and a last line on the
 * Wi-Fi, "wifi state=NAME interface=IF ssid=HEX radios=N".
 *
 * A request (a scan, a join, a disconnect, the Wi-Fi switch) is one more
 * connection that carries one frame each way, as "net wifi ..." sends it.
 * The frame is written at once and the connection kept until the answer is
 * readable; the updates look at both connections without waiting, so the
 * caller's loop never stops for the daemon.
 *
 * This is the one place in the desktop that knows networkd's protocol
 * (userland/base/net/protocol.h); a change of the daemon's protocol is
 * made here and nowhere else.
 */

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

#include "userland/base/net/protocol.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/* The watch's request ID, and the first of the requests'. */
#define NETWORK_WATCH_ID 1U

/* How long a watch that could not be made waits before it is tried again. */
#define NETWORK_RETRY_MS 1000U

/* How long a frame that has begun to arrive may take to arrive whole. */
#define NETWORK_FRAME_SECONDS 2U

/* The longest line of a state or a scan that is read. */
#define NETWORK_LINE_MAX 512U

/*
 * One watch of the network: the SUBSCRIBE connection, the state it last
 * reported, the request outstanding (its connection and kind), the last
 * request that finished with its errno value, and the last scan's
 * networks, the strongest first.
 *
 * watch is -1 while there is no watch; retry_ms is when one is tried
 * again.  request_fd is -1 while no request is outstanding.
 * The subscriber owns this allocation from open until close; completed
 * requests retain their outcome here after their connection is released.
 */
struct kl_backend_network {
	int watch;
	uint64_t retry_ms;
	struct kl_backend_network_state state;
	int request_fd;
	unsigned request;
	uint32_t request_opcode;
	uint32_t request_id;
	uint32_t next_id;
	unsigned finished;
	int finished_error;
	struct kl_backend_network_ap scan[KL_BACKEND_NETWORK_SCAN_MAX];
	size_t scan_count;
};

static int network_connect(int *descriptor);
static int network_watch(struct kl_backend_network *network);
static void network_unwatch(struct kl_backend_network *network, unsigned *changed);
static int network_readable(int descriptor);
static int network_read_watch(struct kl_backend_network *network, unsigned *changed);
static int network_read_answer(struct kl_backend_network *network, unsigned *changed);
static int network_fields(const unsigned char *payload, size_t length, uint32_t *status, uint32_t *error, const char **output, size_t *output_length);
static void network_parse_state(struct kl_backend_network_state *state, const char *output, size_t length);
static void network_parse_interface(struct kl_backend_network_state *state, const char *line, char *wired, size_t wired_size, unsigned *wifi_online);
static void network_parse_wifi(struct kl_backend_network_state *state, const char *line);
static unsigned network_wifi_state(const char *name);
static int network_line(const char *output, size_t length, size_t *start, char *line, size_t size);
static void network_parse_scan(struct kl_backend_network *network, const char *output, size_t length);
static void network_parse_ap(struct kl_backend_network *network, const char *line);
static const char *network_word(const char *line, const char *key, char *field_text, size_t size);
static void network_ssid_text(const char *hex, char *text, size_t size);
static uint64_t network_milliseconds(void);

/*
 * Starts watching the network.
 *
 * A daemon that is not running yet is not a failure: the watch is made by a later update.
 */
struct kl_backend_network *
kl_backend_network_open(
	void)
{
	struct kl_backend_network *network;

	/* The watch's record, with no connection. */
	network = calloc(1, sizeof(*network));
	if (network == NULL)
		return NULL;

	/* Starts both connections as unowned and reserves the watch request ID. */
	network->watch = -1;
	network->request_fd = -1;
	network->next_id = NETWORK_WATCH_ID + 1U;

	/* The watch, when the daemon is there (otherwise the updates try again). */
	(void)network_watch(network);

	/* Succeeded: the watch exists, connected or not. */
	return network;
}

/*
 * Stops watching and drops an outstanding request.
 */
void
kl_backend_network_close(
	struct kl_backend_network *network)
{
	/* Nothing to close. */
	if (network == NULL)
		return;

	/* The two connections, and the record. */
	if (network->watch >= 0)
		(void)close(network->watch);

	/* Retires any independent request connection before freeing its record. */
	if (network->request_fd >= 0)
		(void)close(network->request_fd);

	/* Releases the watch record after neither connection can report to it. */
	free(network);

	/* Succeeded: both connections and the watch record are released. */
	return;
}

/*
 * Reads what has arrived on the watch and on the request without waiting, and makes the watch again when it went (at most once a second).
 */
int
kl_backend_network_update(
	struct kl_backend_network *network,
	unsigned *changed)
{
	uint64_t now;
	int error;

	/* Nothing has changed yet. */
	*changed = 0;

	/* Refuses an operation without its network watch record. */
	if (network == NULL)
		return EINVAL;

	/* A lost watch is made again when its wait is over. */
	if (network->watch < 0) {
		/* Retries only when the previous watch attempt's delay expires. */
		now = network_milliseconds();
		if (now >= network->retry_ms) {
			error = network_watch(network);
			if (error == 0)
				*changed |= KL_BACKEND_NETWORK_CHANGED_STATE;
		}
	}

	/* The states the watch has reported. */
	if (network->watch >= 0)
		(void)network_read_watch(network, changed);

	/* The answer to the request outstanding. */
	if (network->request_fd >= 0)
		(void)network_read_answer(network, changed);

	/* Succeeded: *changed says what the reads found. */
	return 0;
}

/*
 * Copies the network's state as last reported.
 */
void
kl_backend_network_get_state(
	const struct kl_backend_network *network,
	struct kl_backend_network_state *state)
{
	/* A missing watch knows nothing. */
	memset(state, 0, sizeof(*state));

	/* Leaves the caller's state empty when no watch exists. */
	if (network == NULL)
		return;

	/* The state the watch keeps. */
	*state = network->state;

	/* Succeeded: the caller holds the last reported network state. */
	return;
}

/*
 * Copies up to capacity networks of the last scan, the strongest first, and returns how many there are.
 */
size_t
kl_backend_network_get_scan(
	const struct kl_backend_network *network,
	struct kl_backend_network_ap *aps,
	size_t capacity)
{
	size_t count;

	/* A missing watch has scanned nothing. */
	if (network == NULL)
		return 0;

	/* As many as fit. */
	count = network->scan_count;
	if (count > capacity)
		count = capacity;

	/* Copies retained access points when the caller provided room. */
	if (count != 0)
		memcpy(aps, network->scan, count * sizeof(*aps));

	/* Succeeded: reports the full scan size, even if the copy was bounded. */
	return network->scan_count;
}

/*
 * Sends a request to the daemon; its answer arrives through the updates.
 */
int
kl_backend_network_request(
	struct kl_backend_network *network,
	unsigned request,
	const char *ssid)
{
	struct networkd_protocol_header header;
	struct networkd_field_writer writer;
	unsigned char payload[NETWORKD_REQUEST_MAX];
	uint32_t opcode;
	size_t length;
	int descriptor;
	int error;

	/* One request at a time. */

	/* Refuses an operation without its network watch record. */
	if (network == NULL)
		return EINVAL;

	/* Keeps one request outstanding so its reply has an unambiguous owner. */
	if (network->request_fd >= 0)
		return EBUSY;

	/* The daemon's operation for the request. */
	switch (request) {
	case KL_BACKEND_NETWORK_REQUEST_SCAN:
		opcode = NETWORKD_OP_WIFI_LIST;
		break;
	case KL_BACKEND_NETWORK_REQUEST_JOIN:
		opcode = NETWORKD_OP_WIFI_CONNECT;
		break;
	case KL_BACKEND_NETWORK_REQUEST_DISCONNECT:
		opcode = NETWORKD_OP_WIFI_DISCONNECT;
		break;
	case KL_BACKEND_NETWORK_REQUEST_WIFI_ON:
		opcode = NETWORKD_OP_WIFI_ENABLE;
		break;
	case KL_BACKEND_NETWORK_REQUEST_WIFI_OFF:
		opcode = NETWORKD_OP_WIFI_DISABLE;
		break;
	case KL_BACKEND_NETWORK_REQUEST_PROFILES:
		opcode = NETWORKD_OP_WIFI_PROFILES_CHANGED;
		break;
	default:
		return EINVAL;
	}

	/* A join names its network, and nothing else does. */
	networkd_field_writer_init(&writer, payload, sizeof(payload));
	if (request == KL_BACKEND_NETWORK_REQUEST_JOIN) {
		/* An SSID of one to 32 bytes. */
		if (ssid == NULL)
			return EINVAL;

		/* Refuses an SSID outside the daemon's bounded name field. */
		length = strlen(ssid);
		if (length == 0 || length > KL_BACKEND_NETWORK_SSID_MAX - 1U)
			return EINVAL;

		/* The SSID field. */
		error = networkd_field_write(&writer, NETWORKD_FIELD_SSID, ssid, length);
		if (error != 0)
			return EINVAL;
	}

	/* A connection of the request's own. */
	error = network_connect(&descriptor);
	if (error != 0)
		return error;

	/* The frame, and the end of what this side sends (the daemon answers after it). */
	header.request_id = network->next_id;
	header.opcode = opcode;
	header.payload_length = writer.used;
	error = networkd_protocol_write_frame(descriptor, &header, payload);
	if (error != 0) {
		error = errno;
		(void)close(descriptor);
		return error;
	}

	/* This side has said all it will. */
	(void)shutdown(descriptor, SHUT_WR);

	/* The next request gets the next ID (never the watch's, never 0). */
	network->next_id++;
	if (network->next_id <= NETWORK_WATCH_ID)
		network->next_id = NETWORK_WATCH_ID + 1U;

	/* The request is outstanding until its answer is read. */
	network->request_fd = descriptor;
	network->request = request;
	network->request_opcode = opcode;
	network->request_id = header.request_id;

	/* Succeeded: the request is on its way. */
	return 0;
}

/*
 * Tells the request outstanding, or the one that finished last and its errno value.
 */
unsigned
kl_backend_network_get_request(
	const struct kl_backend_network *network,
	int *error)
{
	/* No watch, no request. */
	*error = 0;
	if (network == NULL)
		return KL_BACKEND_NETWORK_REQUEST_NONE;

	/* The outstanding one. */
	if (network->request_fd >= 0)
		return network->request;

	/* Succeeded: reports the last request and its stored outcome. */
	*error = network->finished_error;
	return network->finished;
}

/* Connects to the daemon's socket. */
static int
network_connect(
	int *descriptor)
{
	struct sockaddr_un address;
	int connection;
	int error;

	/* A stream socket that the programs the desktop starts do not inherit. */
	connection = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (connection < 0)
		return errno;

	/* The daemon's socket; one that is not there is a daemon not running. */
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	(void)snprintf(address.sun_path, sizeof(address.sun_path), "%s", NETWORKD_SOCKET);
	error = connect(connection, (struct sockaddr *)&address, sizeof(address));
	if (error != 0) {
		error = errno;
		(void)close(connection);

		/* Maps an absent listening daemon to the missing-service convention. */
		if (error == ECONNREFUSED)
			return ENOENT;

		/* Reports the original connection failure after closing its descriptor. */
		return error;
	}

	/* Succeeded: the connection is the caller's. */
	*descriptor = connection;
	return 0;
}

/* Makes the watch: a connection that asked SUBSCRIBE. */
static int
network_watch(
	struct kl_backend_network *network)
{
	struct networkd_protocol_header header;
	int descriptor;
	int error;

	/* Whatever happens, the next try waits a while. */
	network->retry_ms = network_milliseconds() + NETWORK_RETRY_MS;

	/* The connection. */
	error = network_connect(&descriptor);
	if (error != 0)
		return error;

	/* SUBSCRIBE; the daemon answers with the state and keeps the connection. */
	header.request_id = NETWORK_WATCH_ID;
	header.opcode = NETWORKD_OP_SUBSCRIBE;
	header.payload_length = 0;
	error = networkd_protocol_write_frame(descriptor, &header, NULL);
	if (error != 0) {
		error = errno;
		(void)close(descriptor);
		return error;
	}

	/* Succeeded: the watch waits for its first state. */
	network->watch = descriptor;
	return 0;
}

/* Drops the watch, which leaves the state unknown until it is made again. */
static void
network_unwatch(
	struct kl_backend_network *network,
	unsigned *changed)
{
	/* The connection. */
	(void)close(network->watch);
	network->watch = -1;
	network->retry_ms = network_milliseconds() + NETWORK_RETRY_MS;

	/* The daemon can no longer be reached, which the caller shows. */
	memset(&network->state, 0, sizeof(network->state));
	*changed |= KL_BACKEND_NETWORK_CHANGED_STATE;

	/* Succeeded: the watch is retired and unreachable state is published. */
	return;
}

/* Tells whether a connection has something to read (or has ended) now. */
static int
network_readable(
	int descriptor)
{
	struct pollfd poll_descriptor;
	unsigned char byte;
	ssize_t peeked;
	int ready;
	int error;

	/* A look without waiting. */
	poll_descriptor.fd = descriptor;
	poll_descriptor.events = POLLIN;
	poll_descriptor.revents = 0;
	ready = poll(&poll_descriptor, 1, 0);
	if (ready <= 0)
		return 0;

	/*
	 * Asks the socket itself whether a byte or the end has arrived.  A request's
	 * connection is shut down for writing once the request is sent, and zedBSD's
	 * poll reports that shutdown of our own side as POLLERR at once: without this
	 * look the answer's timed read would start before the answer and run out
	 * (every join longer than its two seconds failed with EIO).
	 */
	peeked = recv(descriptor, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
	if (peeked < 0) {
		error = errno;

		/* Nothing has arrived yet: the poll only saw our own shutdown. */
		if (error == EAGAIN || error == EINTR)
			return 0;
#if EWOULDBLOCK != EAGAIN

		/* The same answer under its other name, where it has one. */
		if (error == EWOULDBLOCK)
			return 0;
#endif
	}

	/* Succeeded: a byte, the end, or a failure the read will report is there. */
	return 1;
}

/* Reads the states the watch has reported, the last one standing. */
static int
network_read_watch(
	struct kl_backend_network *network,
	unsigned *changed)
{
	struct networkd_protocol_header header;
	unsigned char payload[NETWORKD_RESPONSE_MAX];
	const char *output;
	size_t output_length;
	uint32_t status;
	uint32_t error;
	int readable;
	int failed;

	/* Each frame that has arrived. */
	for (;;) {
		/* Nothing more has arrived. */
		readable = network_readable(network->watch);
		if (!readable)
			break;

		/* A frame, whole; a watch that ended or broke is dropped. */
		failed = networkd_protocol_read_frame_timed(network->watch, &header, payload, sizeof(payload), NETWORKD_RESPONSE_MAX, NETWORK_FRAME_SECONDS);
		if (failed != 0) {
			network_unwatch(network, changed);
			return EIO;
		}

		/* A frame that is not the watch's, or does not read, ends the watch. */
		failed = network_fields(payload, header.payload_length, &status, &error, &output, &output_length);
		if (failed != 0 || header.opcode != NETWORKD_OP_SUBSCRIBE) {
			network_unwatch(network, changed);
			return EPROTO;
		}

		/* A refusal (no right to look, too many watchers) ends the watch; it is tried again later. */
		if (status != NETWORKD_RESULT_OK) {
			network_unwatch(network, changed);
			return (int)error;
		}

		/* The state. */
		network_parse_state(&network->state, output, output_length);
		*changed |= KL_BACKEND_NETWORK_CHANGED_STATE;
	}

	/* Succeeded: all currently readable state frames have been applied. */
	return 0;
}

/* Reads the answer to the request outstanding, when it has arrived. */
static int
network_read_answer(
	struct kl_backend_network *network,
	unsigned *changed)
{
	struct networkd_protocol_header header;
	unsigned char payload[NETWORKD_RESPONSE_MAX];
	const char *output;
	size_t output_length;
	uint32_t status;
	uint32_t error;
	int readable;
	int failed;

	/* Not yet. */
	readable = network_readable(network->request_fd);
	if (!readable)
		return 0;

	/* The answer, whole; the request is over either way. */
	failed = networkd_protocol_read_frame_timed(network->request_fd, &header, payload, sizeof(payload), NETWORKD_RESPONSE_MAX, NETWORK_FRAME_SECONDS);
	(void)close(network->request_fd);
	network->request_fd = -1;
	network->finished = network->request;
	network->request = KL_BACKEND_NETWORK_REQUEST_NONE;
	*changed |= KL_BACKEND_NETWORK_CHANGED_DONE;
	if (failed != 0) {
		network->finished_error = EIO;
		return EIO;
	}

	/* An answer to another request, or one that does not read. */
	failed = network_fields(payload, header.payload_length, &status, &error, &output, &output_length);
	if (failed != 0 ||
	    header.request_id != network->request_id ||
	    header.opcode != network->request_opcode) {
		network->finished_error = EPROTO;
		return EPROTO;
	}

	/* A scan's networks, even from a scan that was only partly done. */
	if (network->finished == KL_BACKEND_NETWORK_REQUEST_SCAN) {
		network_parse_scan(network, output, output_length);
		*changed |= KL_BACKEND_NETWORK_CHANGED_SCAN;
	}

	/* A request done in part is done for a scan; otherwise only OK is success. */
	network->finished_error = 0;
	if (status == NETWORKD_RESULT_DEGRADED && network->finished == KL_BACKEND_NETWORK_REQUEST_SCAN)
		return 0;

	/* Retains a refused daemon request as the visible completed outcome. */
	if (status != NETWORKD_RESULT_OK) {
		network->finished_error = EIO;

		/* Keeps the daemon's specific errno when it supplied one. */
		if (error != 0)
			network->finished_error = (int)error;

		/* Reports the refusal retained for the request observer. */
		return network->finished_error;
	}

	/* Succeeded: the request was done. */
	return 0;
}

/* Reads a frame's fields: its status and errno value, and its text (possibly none). */
static int
network_fields(
	const unsigned char *payload,
	size_t length,
	uint32_t *status,
	uint32_t *error,
	const char **output,
	size_t *output_length)
{
	struct networkd_field_reader reader;
	struct networkd_field field;
	unsigned seen;
	int failed;

	/* Nothing read yet. */
	*status = NETWORKD_RESULT_ERROR;
	*error = EIO;
	*output = "";
	*output_length = 0;
	seen = 0;

	/* Each field; the ones this side does not use are passed over. */
	networkd_field_reader_init(&reader, payload, length);
	for (;;) {
		/* Stops the field traversal when no complete field remains. */
		failed = networkd_field_read(&reader, &field);
		if (failed != 0)
			break;

		/* The outcome. */
		if (field.type == NETWORKD_FIELD_STATUS) {
			/* Accepts this mandatory outcome field only when its scalar decodes. */
			failed = networkd_field_read_u32(&field, status);
			if (failed == 0)
				seen |= 1U;
			continue;
		}

		/* The reason for a refusal. */
		if (field.type == NETWORKD_FIELD_ERROR) {
			/* Accepts this mandatory outcome field only when its scalar decodes. */
			failed = networkd_field_read_u32(&field, error);
			if (failed == 0)
				seen |= 2U;
			continue;
		}

		/* The text. */
		if (field.type == NETWORKD_FIELD_OUTPUT) {
			*output = (const char *)field.value;
			*output_length = field.length;
		}
	}

	/* A frame needs its outcome and its reason. */
	if ((seen & 3U) != 3U)
		return EPROTO;

	/* Succeeded: the fields are read. */
	return 0;
}

/* Reads the interfaces and Wi-Fi state from the daemon's text. */
static void
network_parse_state(
	struct kl_backend_network_state *state,
	const char *output,
	size_t length)
{
	char line[NETWORK_LINE_MAX];
	char wired[KL_BACKEND_NETWORK_NAME_MAX];
	unsigned wifi_online;
	size_t start;
	int more;
	int differs;

	/*
	 * Reads a state: the interfaces' lines and the Wi-Fi's.  The connection is
	 * the Wi-Fi's when the Wi-Fi is connected and its interface is online, and
	 * otherwise the first other interface that is online with an address.
	 */

	/* A reachable daemon with nothing known yet. */
	memset(state, 0, sizeof(*state));
	state->reachable = 1;

	/* Starts interface selection without a wired or online radio candidate. */
	wired[0] = '\0';
	wifi_online = 0;

	/* The Wi-Fi's line first, so that the interfaces' lines know which one is the radio. */
	start = 0;
	for (;;) {
		/* Stops when the bounded daemon output contains no further line. */
		more = network_line(output, length, &start, line, sizeof(line));
		if (!more)
			break;

		/* The Wi-Fi's line. */
		differs = strncmp(line, "wifi ", 5);
		if (differs == 0)
			network_parse_wifi(state, line);
	}

	/* Then each interface's line (not the Wi-Fi's, not an empty one). */
	start = 0;
	for (;;) {
		/* Stops when the bounded daemon output contains no further line. */
		more = network_line(output, length, &start, line, sizeof(line));
		if (!more)
			break;

		/* The Wi-Fi's line and an empty one are no interface. */
		differs = strncmp(line, "wifi ", 5);
		if (differs == 0 || line[0] == '\0')
			continue;

		/* An interface's line. */
		network_parse_interface(state, line, wired, sizeof(wired), &wifi_online);
	}

	/* The wired interface, whichever carries the connection. */
	(void)snprintf(state->wired, sizeof(state->wired), "%s", wired);

	/* A connected Wi-Fi carries the connection. */
	if (state->wifi == KL_BACKEND_WIFI_CONNECTED && wifi_online) {
		state->connected = 1;
		state->kind = KL_BACKEND_NETWORK_WIFI;
		(void)snprintf(state->interface, sizeof(state->interface), "%s", state->wifi_interface);
		return;
	}

	/* Otherwise a wired interface that is up with an address. */
	if (wired[0] != '\0') {
		state->connected = 1;
		state->kind = KL_BACKEND_NETWORK_WIRED;
		(void)snprintf(state->interface, sizeof(state->interface), "%s", wired);
	}

	/* Succeeded: the reachable connection state is classified. */
	return;
}

/* Copies the next bounded line and advances the text cursor. */
static int
network_line(
	const char *output,
	size_t length,
	size_t *start,
	char *line,
	size_t size)
{
	size_t end;
	size_t copied;

	/*
	 * Copies the next line of a text from *start into line (cut to its size)
	 * and moves *start past it.  Returns 0 when the text is used up.
	 */

	/* The text is used up. */
	if (*start >= length)
		return 0;

	/* The line runs to its newline or to the end. */
	end = *start;
	while (end < length && output[end] != '\n')
		end++;

	/* Its text, as much as fits. */
	copied = end - *start;
	if (copied >= size)
		copied = size - 1U;

	/* Publishes the bounded line and advances past its original newline. */
	memcpy(line, output + *start, copied);
	line[copied] = '\0';
	*start = end + 1U;

	/* Succeeded: the next line is copied and the cursor advances. */
	return 1;
}

/* Selects an online radio or the first configured wired interface. */
static void
network_parse_interface(
	struct kl_backend_network_state *state,
	const char *line,
	char *wired,
	size_t wired_size,
	unsigned *wifi_online)
{
	char name[KL_BACKEND_NETWORK_NAME_MAX];
	char address[32];
	char link[32];
	int count;
	int differs;

	/*
	 * Reads an interface's line, "NAME static|unconfigured online|offline":
	 * the radio's being online, or the first other interface (not the
	 * loopback) online with an address.
	 */

	/* The three words. */
	count = sscanf(line, "%15s %31s %31s", name, address, link);
	if (count != 3)
		return;

	/* The loopback is no connection. */
	differs = strncmp(name, "lo", 2);
	if (differs == 0)
		return;

	/* An interface that is not up is no connection either. */
	differs = strcmp(link, "online");
	if (differs != 0)
		return;

	/* The radio's interface: its being up is what a connected Wi-Fi needs. */
	differs = strcmp(name, state->wifi_interface);
	if (state->wifi_interface[0] != '\0' && differs == 0) {
		*wifi_online = 1;
		return;
	}

	/* The first other interface with an address carries a wired connection. */
	differs = strcmp(address, "static");
	if (differs == 0 && wired[0] == '\0')
		(void)snprintf(wired, wired_size, "%s", name);

	/* Succeeded: the interface is classified without replacing an earlier wired choice. */
	return;
}

/* Reads the Wi-Fi's line, "wifi state=NAME interface=IF ssid=HEX radios=N". */
static void
network_parse_wifi(
	struct kl_backend_network_state *state,
	const char *line)
{
	char field_text[80];
	const char *found;
	int differs;

	/* No radio at all. */
	found = network_word(line, "radios", field_text, sizeof(field_text));
	differs = strcmp(field_text, "0");
	if (found != NULL && differs == 0) {
		state->wifi = KL_BACKEND_WIFI_ABSENT;
		return;
	}

	/* The radio the managed connection uses ("-" for none). */
	found = network_word(line, "interface", field_text, sizeof(field_text));
	differs = strcmp(field_text, "-");
	if (found != NULL && differs != 0)
		(void)snprintf(state->wifi_interface, sizeof(state->wifi_interface), "%s", field_text);

	/* The network it is on or joining. */
	found = network_word(line, "ssid", field_text, sizeof(field_text));
	differs = strcmp(field_text, "-");
	if (found != NULL && differs != 0)
		network_ssid_text(field_text, state->ssid, sizeof(state->ssid));

	/* The daemon's state, in the desktop's terms (the ones not named are left unconnected). */
	found = network_word(line, "state", field_text, sizeof(field_text));
	if (found == NULL)
		field_text[0] = '\0';

	/* Classifies the daemon state, including an absent or unknown name. */
	state->wifi = network_wifi_state(field_text);

	/* Succeeded: the radio identity and state are decoded. */
	return;
}

/* Tells the desktop's Wi-Fi state for networkd's name of it. */
static unsigned
network_wifi_state(
	const char *name)
{
	int differs;

	/* Off. */
	differs = strcmp(name, "disabled");
	if (differs == 0)
		return KL_BACKEND_WIFI_OFF;

	/* Looking for a network with a profile. */
	differs = strcmp(name, "auto-searching");
	if (differs == 0)
		return KL_BACKEND_WIFI_SEARCHING;

	/* Joining, for the first time or again. */
	differs = strcmp(name, "connecting");
	if (differs == 0)
		return KL_BACKEND_WIFI_CONNECTING;

	/* Treats a reconnect as the same visible joining state. */
	differs = strcmp(name, "reconnecting");
	if (differs == 0)
		return KL_BACKEND_WIFI_CONNECTING;

	/* On a network. */
	differs = strcmp(name, "connected");
	if (differs == 0)
		return KL_BACKEND_WIFI_CONNECTED;

	/* Succeeded: other state names describe an unconnected radio. */
	return KL_BACKEND_WIFI_DISCONNECTED;
}

/* Reads and sorts the strongest access point of each scanned SSID. */
static void
network_parse_scan(
	struct kl_backend_network *network,
	const char *output,
	size_t length)
{
	struct kl_backend_network_ap moved;
	char line[NETWORK_LINE_MAX];
	const char *found;
	size_t start;
	size_t index;
	size_t place;
	int more;

	/*
	 * Reads a scan's networks: its lines
	 * "interface=IF bss index=N ssid=HEX bssid=HEX ... rssi=N ... security=HEX ...",
	 * one network an SSID (the strongest access point), the strongest first.
	 */

	/* A new scan replaces the last. */
	network->scan_count = 0;

	/* Each line that names an access point. */
	start = 0;
	for (;;) {
		/* Stops when the bounded daemon output contains no further line. */
		more = network_line(output, length, &start, line, sizeof(line));
		if (!more)
			break;

		/* An access point's line. */
		found = strstr(line, " bss ");
		if (found != NULL)
			network_parse_ap(network, line);
	}

	/* The strongest first (an insertion sort of a few entries). */
	for (index = 1; index < network->scan_count; index++) {
		/* Inserts this access point after every stronger retained one. */
		moved = network->scan[index];
		place = index;
		while (place > 0 && network->scan[place - 1].rssi < moved.rssi) {
			/* Shifts a weaker access point to reserve this signal-ranked position. */
			network->scan[place] = network->scan[place - 1];
			place--;
		}

		/* The entry goes where the stronger ones end. */
		network->scan[place] = moved;
	}

	/* Succeeded: the scan keeps the strongest access point per SSID in signal order. */
	return;
}

/* Reads one access point's line into the scan (the stronger of two of one SSID stays). */
static void
network_parse_ap(
	struct kl_backend_network *network,
	const char *line)
{
	struct kl_backend_network_ap ap;
	char field_text[80];
	const char *found;
	unsigned long security;
	size_t index;
	int differs;

	/* The SSID; a hidden network (no SSID) is not listed. */
	memset(&ap, 0, sizeof(ap));

	/* Skips a hidden access point without a public SSID. */
	found = network_word(line, "ssid", field_text, sizeof(field_text));
	if (found == NULL || field_text[0] == '\0')
		return;

	/* Rejects a hexadecimal SSID that decodes to an empty public name. */
	network_ssid_text(field_text, ap.ssid, sizeof(ap.ssid));
	if (ap.ssid[0] == '\0')
		return;

	/* The signal. */
	ap.rssi = -100;
	found = network_word(line, "rssi", field_text, sizeof(field_text));
	if (found != NULL)
		ap.rssi = atoi(field_text);

	/* Any security bit asks for a key. */
	found = network_word(line, "security", field_text, sizeof(field_text));
	if (found != NULL) {
		/* Marks any advertised security mode as requiring a saved key. */
		security = strtoul(field_text, NULL, 16);
		if (security != 0)
			ap.secured = 1;
	}

	/* An SSID already listed keeps its stronger access point. */
	for (index = 0; index < network->scan_count; index++) {
		/* Skips access points naming a different retained network. */
		differs = strcmp(network->scan[index].ssid, ap.ssid);
		if (differs != 0)
			continue;

		/* The stronger one stays. */
		if (ap.rssi > network->scan[index].rssi)
			network->scan[index] = ap;

		/* Leaves this SSID represented by exactly one strongest access point. */
		return;
	}

	/* A new one, while there is room. */
	if (network->scan_count < KL_BACKEND_NETWORK_SCAN_MAX) {
		network->scan[network->scan_count] = ap;
		network->scan_count++;
	}

	/* Succeeded: the access point is retained when room remains. */
	return;
}

/* Copies a named whole-word field and reports its position. */
static const char *
network_word(
	const char *line,
	const char *key,
	char *field_text,
	size_t size)
{
	const char *at;
	size_t key_length;
	size_t length;

	/*
	 * Finds " KEY=VALUE" (or "KEY=VALUE" at the start) in a line and copies the
	 * value; returns where it was found, or NULL.
	 */

	/* Nothing found yet. */
	field_text[0] = '\0';

	/* Each place the key appears, as a whole word followed by '='. */
	key_length = strlen(key);
	for (at = strstr(line, key);
	     at != NULL;
	     at = strstr(at + 1, key)) {
		/* Part of a longer word. */
		if (at != line && at[-1] != ' ')
			continue;

		/* Requires the complete key to be followed by its value separator. */
		if (at[key_length] != '=')
			continue;

		/* The value, up to the next space. */
		at += key_length + 1U;
		length = strcspn(at, " ");
		if (length >= size)
			length = size - 1U;

		/* Copies and terminates the bounded field before returning its position. */
		memcpy(field_text, at, length);
		field_text[length] = '\0';
		break;
	}

	/* Refuses a field not present as a complete key. */
	if (at == NULL)
		return NULL;

	/* Succeeded: the field text is copied and its position is retained. */
	return at;
}

/* Turns a hexadecimal SSID into text a menu can show (a control byte shows as '?'). */
static void
network_ssid_text(
	const char *hex,
	char *text,
	size_t size)
{
	unsigned decoded_byte;
	size_t used;
	int count;

	/* Each pair of digits is one byte. */
	used = 0;
	while (hex[0] != '\0' &&
	       hex[1] != '\0' &&
	       used + 1U < size) {
		/* A pair that is not hexadecimal ends the SSID. */
		count = sscanf(hex, "%2x", &decoded_byte);
		if (count != 1)
			break;

		/* A control byte cannot be drawn. */
		if (decoded_byte < 0x20U || decoded_byte == 0x7fU)
			decoded_byte = '?';

		/* Publishes one drawable SSID byte and advances to the next pair. */
		text[used] = (char)decoded_byte;
		used++;
		hex += 2;
	}

	/* The text ends where the digits did. */
	text[used] = '\0';

	/* Succeeded: the decoded SSID is terminated. */
	return;
}

/* Reads the monotonic clock in milliseconds. */
static uint64_t
network_milliseconds(
	void)
{
	struct timespec now;
	int error;

	/* The clock; one that fails reads as the start of time. */
	error = clock_gettime(CLOCK_MONOTONIC, &now);
	if (error != 0)
		return 0;

	/* Succeeded: expresses the monotonic clock in milliseconds. */
	return (uint64_t)now.tv_sec * 1000U + (uint64_t)now.tv_nsec / 1000000U;
}
