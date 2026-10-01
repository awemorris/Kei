/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Implements only the authenticated, fd-carrying D-Bus calls the seat needs. */
#include "dbus-linux.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#define DBUS_TIMEOUT_MS 5000U

/* One bounded encoder refuses growth before writing any bytes. */
struct dbus_writer {
	unsigned char *bytes;
	size_t used;
};

static int bus_now(uint64_t *now);
static int bus_wait(int fd, short events, uint64_t deadline);
static int bus_send(struct linux_dbus *bus, const void *bytes, size_t size, uint64_t deadline);
static int bus_auth_line(struct linux_dbus *bus, char *line, size_t capacity, uint64_t deadline);
static uint32_t bus_get32(const unsigned char *bytes);
static void bus_put32(unsigned char *bytes, uint32_t value);
static int writer_align(struct dbus_writer *writer, size_t alignment);
static int writer_string(struct dbus_writer *writer, char type, const char *string);
static int writer_number(struct dbus_writer *writer, uint32_t number);
static int writer_field(struct dbus_writer *writer, unsigned code, char type, const char *string);
static int bus_build(struct dbus_reply *message, uint32_t serial, const char *destination, const char *path, const char *interface, const char *member, const char *signature, const struct dbus_arg *args, size_t count);
static int reader_string(struct dbus_reply *message, size_t *cursor, size_t end, char type, const char **string);
static int bus_parse(struct dbus_reply *message);
static int bus_receive(struct linux_dbus *bus, struct dbus_reply **message);
static int bus_queue(struct linux_dbus *bus, struct dbus_reply *message);

/*
 * Authenticates this UID and negotiates ancillary file passing on the system bus.
 */
int
dbus_open_system(
	struct linux_dbus *bus)
{
	struct sockaddr_un address;
	struct dbus_reply *reply;
	char uid[32];
	char request[160];
	char line[256];
	uint64_t deadline = 0;
	size_t length;
	size_t index;
	int error;
	int status;
	socklen_t status_size;

	/* A failed open can always be unwound by the same close path. */
	memset(bus, 0, sizeof(*bus));
	bus->fd = -1;
	error = bus_now(&deadline);
	if (error != 0)
		return error;
	deadline += DBUS_TIMEOUT_MS;

	/* Never wait indefinitely for an overloaded local socket listener. */
	bus->fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (bus->fd < 0)
		return errno;

	/* The system endpoint is fixed; callers cannot redirect device authority. */
	memset(&address, 0, sizeof(address));
	address.sun_family = AF_UNIX;
	memcpy(address.sun_path, "/run/dbus/system_bus_socket", sizeof("/run/dbus/system_bus_socket"));
	error = connect(bus->fd, (struct sockaddr *)&address, sizeof(address));
	if (error != 0) {
		/* An asynchronous connect must finish inside the authentication deadline. */
		if (errno != EINPROGRESS)
			return errno;
		error = bus_wait(bus->fd, POLLOUT, deadline);
		if (error != 0)
			return error;
		status_size = sizeof(status);
		error = getsockopt(bus->fd, SOL_SOCKET, SO_ERROR, &status, &status_size);
		if (error != 0)
			return errno;
		if (status != 0)
			return status;
	}

	/* EXTERNAL carries the decimal UID encoded as hexadecimal ASCII bytes. */
	status = snprintf(uid, sizeof(uid), "%lu", (unsigned long)getuid());
	if (status < 0 || (size_t)status >= sizeof(uid))
		return EOVERFLOW;
	length = (size_t)status;
	request[0] = '\0';
	memcpy(request + 1, "AUTH EXTERNAL ", 14);

	/* Every decimal digit contributes exactly two hexadecimal digits. */
	for (index = 0; index < length; index++) {
		status = snprintf(request + 15 + index * 2, 3, "%02x", (unsigned char)uid[index]);
		if (status != 2)
			return EOVERFLOW;
	}

	/* The leading NUL proves Unix peer credentials before the authentication command. */
	memcpy(request + 15 + length * 2, "\r\n", 2);
	error = bus_send(bus, request, 17 + length * 2, deadline);
	if (error != 0)
		return error;
	error = bus_auth_line(bus, line, sizeof(line), deadline);
	if (error != 0)
		return error;
	status = strncmp(line, "OK ", 3);
	if (status != 0)
		return EACCES;

	/* Refuses a connection that cannot transfer the logind device descriptors. */
	error = bus_send(bus, "NEGOTIATE_UNIX_FD\r\n", 19, deadline);
	if (error != 0)
		return error;
	error = bus_auth_line(bus, line, sizeof(line), deadline);
	if (error != 0)
		return error;
	status = strcmp(line, "AGREE_UNIX_FD\r\n");
	if (status != 0)
		return ENOTSUP;

	/* Binary frames begin only after the authentication exchange is complete. */
	error = bus_send(bus, "BEGIN\r\n", 7, deadline);
	if (error != 0)
		return error;
	error = dbus_call(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "Hello", "", NULL, 0, &reply);
	if (error != 0)
		return error;
	dbus_reply_free(reply);

	/* Succeeded: this bus owns an authenticated unique connection name. */
	return 0;
}

/*
 * Closes the bus and every file owned by partial or queued frames.
 */
void
dbus_close(
	struct linux_dbus *bus)
{
	size_t index;

	/* No queued signal may retain a descriptor after seat cleanup. */
	for (index = 0; index < bus->signal_count; index++)
		dbus_reply_free(bus->signals[index]);

	/* Partial receives own rights even before the frame can be decoded. */
	dbus_reply_free(bus->receiving);
	if (bus->fd >= 0)
		(void)close(bus->fd);

	/* Succeeded: repeated cleanup sees an empty, disconnected bus. */
	memset(bus, 0, sizeof(*bus));
	bus->fd = -1;
	return;
}

/*
 * Sends one method and retains intervening signals while awaiting its serial.
 */
int
dbus_call(
	struct linux_dbus *bus,
	const char *destination,
	const char *path,
	const char *interface,
	const char *member,
	const char *signature,
	const struct dbus_arg *args,
	size_t count,
	struct dbus_reply **reply)
{
	struct dbus_reply *message;
	uint64_t deadline = 0;
	uint64_t now = 0;
	uint32_t serial;
	int error;

	/* Each call owns a unique nonzero serial and one finite deadline. */
	*reply = NULL;
	bus->serial++;
	if (bus->serial == 0)
		bus->serial++;
	serial = bus->serial;
	error = bus_now(&deadline);
	if (error != 0)
		return error;
	deadline += DBUS_TIMEOUT_MS;

	/* An encoder failure cannot leak a partially transmitted request. */
	message = calloc(1, sizeof(*message));
	if (message == NULL)
		return ENOMEM;
	error = bus_build(message, serial, destination, path, interface, member, signature, args, count);
	if (error != 0) {
		dbus_reply_free(message);
		return error;
	}

	/* A partial send failure terminates the framing connection. */
	error = bus_send(bus, message->bytes, message->size, deadline);
	dbus_reply_free(message);
	if (error != 0) {
		dbus_close(bus);
		return error;
	}

	/* Signals keep their original arrival order until the main loop dispatches them. */
	for (;;) {
		/* Even a continuously busy signal stream cannot extend this method deadline. */
		error = bus_now(&now);
		if (error != 0 || now >= deadline) {
			dbus_close(bus);
			if (error != 0)
				return error;
			return ETIMEDOUT;
		}

		/* Reads the next bounded frame, retaining any partial suffix. */
		error = bus_receive(bus, &message);
		if (error == EAGAIN) {
			error = bus_wait(bus->fd, POLLIN, deadline);
			if (error == 0)
				continue;
		}

		/* A timeout or broken frame cannot leave rights behind on the bus. */
		if (error != 0) {
			dbus_close(bus);
			return error;
		}

		/* Only the matching return may complete this synchronous call. */
		if ((message->type == 2 ||
		     message->type == 3) &&
		    message->reply_serial == serial) {
			if (message->type == 3) {
				fprintf(stderr, "wayland: dbus %s: %s\n", member, message->error);
				dbus_reply_free(message);
				return EACCES;
			}

			/* Succeeded: the caller now owns the response and its ancillary files. */
			*reply = message;
			return 0;
		}

		/* Unrelated replies are discarded; signals are bounded rather than silently lost. */
		if (message->type == 4) {
			error = bus_queue(bus, message);
			if (error != 0) {
				dbus_reply_free(message);
				dbus_close(bus);
				return error;
			}
		} else {
			dbus_reply_free(message);
		}
	}
}

/*
 * Installs one signal subscription through the authenticated bus daemon.
 */
int
dbus_add_match(
	struct linux_dbus *bus,
	const char *rule)
{
	struct dbus_arg argument;
	struct dbus_reply *reply;
	int error;

	/* The daemon owns the subscription for this connection's lifetime. */
	memset(&argument, 0, sizeof(argument));
	argument.string = rule;
	error = dbus_call(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "AddMatch", "s", &argument, 1, &reply);
	if (error != 0)
		return error;
	dbus_reply_free(reply);

	/* Succeeded: subsequent matching signals can join the bounded queue. */
	return 0;
}

/*
 * Supplies the socket used by the compositor's OS poll range.
 */
int
dbus_fd(
	const struct linux_dbus *bus)
{
	/* Succeeded: a negative fd explicitly denotes a closed connection. */
	return bus->fd;
}

/*
 * Dispatches queued and currently readable signals without blocking the main loop.
 */
int
dbus_dispatch(
	struct linux_dbus *bus,
	dbus_signal_fn callback,
	void *data)
{
	struct dbus_reply *message;
	size_t index;
	unsigned count;
	int error;

	/* A per-pass bound preserves client service under a busy signal stream. */
	for (count = 0; count < LINUX_DBUS_SIGNALS; count++) {
		/* Queued signals must run even when the socket's current revents is zero. */
		if (bus->signal_count != 0) {
			message = bus->signals[0];
			bus->signal_count--;

			/* Retains arrival order while removing this owned message. */
			for (index = 0; index < bus->signal_count; index++)
				bus->signals[index] = bus->signals[index + 1];
		} else {
			/* Receives at most the remainder of one frame, with no blocking read. */
			error = bus_receive(bus, &message);
			if (error == EAGAIN)
				return 0;
			if (error != 0) {
				dbus_close(bus);
				return error;
			}
		}

		/* The callback may take a resume fd; untaken rights close with the frame. */
		error = 0;
		if (message->type == 4)
			error = callback(message, data);
		dbus_reply_free(message);
		if (error != 0)
			return error;
	}

	/* Succeeded: remaining work stays owned for the next finite poll pass. */
	return 0;
}

/*
 * Returns every untaken ancillary descriptor and the response storage.
 */
void
dbus_reply_free(
	struct dbus_reply *reply)
{
	size_t index;

	/* Callers may unwind before a response has been allocated. */
	if (reply == NULL)
		return;

	/* Transferred rights are marked negative and never closed a second time. */
	for (index = 0; index < reply->fd_count; index++) {
		if (reply->fds[index] >= 0)
			(void)close(reply->fds[index]);
	}

	/* Succeeded: no descriptor lifetime depends on this frame any more. */
	free(reply);
	return;
}

/*
 * Reads one aligned unsigned, Boolean or file index from a reply body.
 */
int
dbus_read_number(
	struct dbus_reply *reply,
	uint32_t *number)
{
	size_t cursor;

	/* Padding and a complete scalar must fit before any body byte is read. */
	cursor = (reply->cursor + 3U) & ~(size_t)3U;
	if (cursor > reply->size || reply->size - cursor < 4)
		return EPROTO;
	*number = bus_get32(reply->bytes + cursor);
	reply->cursor = cursor + 4;

	/* Succeeded: the next body argument follows this scalar. */
	return 0;
}

/*
 * Reads a string or object path without copying outside the owned frame.
 */
int
dbus_read_string(
	struct dbus_reply *reply,
	const char **string)
{
	int error;

	/* Body readers share the same bounded representation as header strings. */
	error = reader_string(reply, &reply->cursor, reply->size, 's', string);
	if (error != 0)
		return error;

	/* Succeeded: the string remains valid until the caller frees this reply. */
	return 0;
}

/*
 * Transfers exactly one ancillary descriptor into seat ownership.
 */
int
dbus_take_fd(
	struct dbus_reply *reply,
	uint32_t index)
{
	int fd;

	/* An absent or already transferred index is a malformed handle. */
	if (index >= reply->fd_count)
		return -1;
	fd = reply->fds[index];
	if (fd < 0)
		return -1;
	reply->fds[index] = -1;

	/* Succeeded: only the seat now closes this file. */
	return fd;
}

/* Measures deadlines using a clock unaffected by wall-clock corrections. */
static int
bus_now(
	uint64_t *now)
{
	struct timespec time;
	int error;

	/* Clock failure cannot make a bounded call wait forever. */
	error = clock_gettime(CLOCK_MONOTONIC, &time);
	if (error != 0)
		return errno;
	*now = (uint64_t)time.tv_sec * 1000U + (uint64_t)time.tv_nsec / 1000000U;

	/* Succeeded: the caller can compare this monotonic millisecond stamp. */
	return 0;
}

/* Waits for one socket operation inside the caller's original deadline. */
static int
bus_wait(
	int fd,
	short events,
	uint64_t deadline)
{
	struct pollfd descriptor;
	uint64_t now = 0;
	int error;
	int ready;

	/* Signal interruptions retain the same absolute timeout. */
	for (;;) {
		error = bus_now(&now);
		if (error != 0)
			return error;
		if (now >= deadline)
			return ETIMEDOUT;
		descriptor.fd = fd;
		descriptor.events = events;
		descriptor.revents = 0;
		ready = poll(&descriptor, 1, (int)(deadline - now));
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			return errno;
		}

		/* EOF and invalid descriptors cannot satisfy a transport operation. */
		if (ready == 0)
			return ETIMEDOUT;
		if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
			return EPIPE;
		if ((descriptor.revents & events) != 0)
			return 0;
	}
}

/* Sends every request byte without extending the method's deadline. */
static int
bus_send(
	struct linux_dbus *bus,
	const void *bytes,
	size_t size,
	uint64_t deadline)
{
	const unsigned char *source;
	ssize_t sent;
	int error;

	/* Partial writes advance only over bytes the socket accepted. */
	source = bytes;
	while (size != 0) {
		sent = send(bus->fd, source, size, MSG_DONTWAIT | MSG_NOSIGNAL);
		if (sent < 0) {
			if (errno == EINTR)
				continue;
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				return errno;
			error = bus_wait(bus->fd, POLLOUT, deadline);
			if (error != 0)
				return error;
			continue;
		}

		/* A zero-sized successful send cannot advance framing. */
		if (sent == 0)
			return EPIPE;
		source += sent;
		size -= (size_t)sent;
	}

	/* Succeeded: the complete request is on the authenticated connection. */
	return 0;
}

/* Receives one bounded authentication line without consuming binary-frame bytes. */
static int
bus_auth_line(
	struct linux_dbus *bus,
	char *line,
	size_t capacity,
	uint64_t deadline)
{
	struct iovec vector;
	struct msghdr header;
	ssize_t received;
	size_t used;
	int error;

	/* Ancillary transfer is negotiated only after this authentication exchange. */
	used = 0;
	while (used + 1 < capacity) {
		memset(&header, 0, sizeof(header));
		vector.iov_base = line + used;
		vector.iov_len = 1;
		header.msg_iov = &vector;
		header.msg_iovlen = 1;
		received = recvmsg(bus->fd, &header, MSG_DONTWAIT | MSG_CMSG_CLOEXEC);
		if (received < 0) {
			if (errno == EINTR)
				continue;
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				return errno;
			error = bus_wait(bus->fd, POLLIN, deadline);
			if (error != 0)
				return error;
			continue;
		}

		/* Truncated ancillary input is invalid even during authentication. */
		if (received == 0)
			return EPIPE;
		if ((header.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) != 0)
			return EPROTO;
		used++;
		if (used >= 2 &&
		    line[used - 2] == '\r' &&
		    line[used - 1] == '\n') {
			line[used] = '\0';
			return 0;
		}
	}

	/* An overlong line is refused without interpreting its truncated prefix. */
	return EOVERFLOW;
}

/* Decodes one little-endian wire scalar independently of host alignment. */
static uint32_t
bus_get32(
	const unsigned char *bytes)
{
	/* Succeeded: each byte contributes its specified wire significance. */
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* Writes a scalar in the connection's explicit little-endian representation. */
static void
bus_put32(
	unsigned char *bytes,
	uint32_t value)
{
	/* Each independent byte avoids host alignment and aliasing assumptions. */
	bytes[0] = (unsigned char)value;
	bytes[1] = (unsigned char)(value >> 8);
	bytes[2] = (unsigned char)(value >> 16);
	bytes[3] = (unsigned char)(value >> 24);

	/* Succeeded: all four wire bytes have been stored. */
	return;
}

/* Pads an encoder to a protocol boundary without exceeding its frame. */
static int
writer_align(
	struct dbus_writer *writer,
	size_t alignment)
{
	size_t next;

	/* The current frame is already bounded, so alignment cannot wrap. */
	next = (writer->used + alignment - 1) & ~(alignment - 1);
	if (next > LINUX_DBUS_MAX)
		return EOVERFLOW;
	memset(writer->bytes + writer->used, 0, next - writer->used);
	writer->used = next;

	/* Succeeded: subsequent values start at their required alignment. */
	return 0;
}

/* Encodes a terminated string, path or signature with its length prefix. */
static int
writer_string(
	struct dbus_writer *writer,
	char type,
	const char *string)
{
	size_t length;
	size_t prefix;
	int error;

	/* NULL is not a valid protocol string even for an empty signature. */
	if (string == NULL)
		return EINVAL;
	length = strlen(string);
	prefix = 1;
	if (type != 'g') {
		error = writer_align(writer, 4);
		if (error != 0)
			return error;
		prefix = 4;
	} else {
		if (length > 255)
			return EOVERFLOW;
	}

	/* Refuses lengths that cannot leave room for the prefix and terminator. */
	if (length >= LINUX_DBUS_MAX || prefix + length + 1 > LINUX_DBUS_MAX - writer->used)
		return EOVERFLOW;
	if (type == 'g') {
		writer->bytes[writer->used] = (unsigned char)length;
	} else {
		bus_put32(writer->bytes + writer->used, (uint32_t)length);
	}

	/* Copies the terminator as part of the bounded representation. */
	writer->used += prefix;
	memcpy(writer->bytes + writer->used, string, length + 1);
	writer->used += length + 1;

	/* Succeeded: the next encoded value follows this complete string. */
	return 0;
}

/* Encodes one aligned numeric argument without unaligned stores. */
static int
writer_number(
	struct dbus_writer *writer,
	uint32_t number)
{
	int error;

	/* A numeric value reserves four bytes after its padding. */
	error = writer_align(writer, 4);
	if (error != 0)
		return error;
	if (LINUX_DBUS_MAX - writer->used < 4)
		return EOVERFLOW;
	bus_put32(writer->bytes + writer->used, number);
	writer->used += 4;

	/* Succeeded: the scalar is fully encoded. */
	return 0;
}

/* Appends one a(yv) header entry and its single-value variant. */
static int
writer_field(
	struct dbus_writer *writer,
	unsigned code,
	char type,
	const char *string)
{
	char signature[2];
	int error;

	/* Each field structure starts on an eight-byte boundary. */
	error = writer_align(writer, 8);
	if (error != 0)
		return error;
	if (writer->used >= LINUX_DBUS_MAX)
		return EOVERFLOW;
	writer->bytes[writer->used++] = (unsigned char)code;
	signature[0] = type;
	signature[1] = '\0';
	error = writer_string(writer, 'g', signature);
	if (error != 0)
		return error;
	error = writer_string(writer, type, string);
	if (error != 0)
		return error;

	/* Succeeded: this header field is complete. */
	return 0;
}

/* Builds one method call from the small seat argument vocabulary. */
static int
bus_build(
	struct dbus_reply *message,
	uint32_t serial,
	const char *destination,
	const char *path,
	const char *interface,
	const char *member,
	const char *signature,
	const struct dbus_arg *args,
	size_t count)
{
	struct dbus_writer writer;
	size_t fields;
	size_t body;
	size_t index;
	size_t length;
	char type;
	int error;

	/* Fixed header bytes precede the header-field array. */
	writer.bytes = message->bytes;
	writer.used = 16;
	length = strlen(signature);
	if (length != count)
		return EINVAL;

	/* The bus uses the explicit destination rather than implicit peer routing. */
	error = writer_field(&writer, 1, 'o', path);
	if (error != 0)
		return error;
	error = writer_field(&writer, 6, 's', destination);
	if (error != 0)
		return error;
	error = writer_field(&writer, 2, 's', interface);
	if (error != 0)
		return error;
	error = writer_field(&writer, 3, 's', member);
	if (error != 0)
		return error;
	error = writer_field(&writer, 8, 'g', signature);
	if (error != 0)
		return error;
	fields = writer.used - 16;

	/* Body alignment is outside the header array's declared length. */
	error = writer_align(&writer, 8);
	if (error != 0)
		return error;
	body = writer.used;

	/* Only the agreed simple argument types can be transmitted. */
	for (index = 0; index < count; index++) {
		type = signature[index];
		if (type == 's' ||
		    type == 'o' ||
		    type == 'g') {
			error = writer_string(&writer, type, args[index].string);
		} else if (type == 'u' || type == 'b') {
			if (type == 'b' && args[index].number > 1)
				return EINVAL;
			error = writer_number(&writer, args[index].number);
		} else {
			return ENOTSUP;
		}

		/* A refused argument never publishes a truncated request. */
		if (error != 0)
			return error;
	}

	/* The completed frame declares its exact lengths and nonzero call serial. */
	message->bytes[0] = 'l';
	message->bytes[1] = 1;
	message->bytes[2] = 0;
	message->bytes[3] = 1;
	bus_put32(message->bytes + 4, (uint32_t)(writer.used - body));
	bus_put32(message->bytes + 8, serial);
	bus_put32(message->bytes + 12, (uint32_t)fields);
	message->size = writer.used;

	/* Succeeded: this exact frame can be sent without ancillary files. */
	return 0;
}

/* Validates a string against its enclosing header or body boundary. */
static int
reader_string(
	struct dbus_reply *message,
	size_t *cursor,
	size_t end,
	char type,
	const char **string)
{
	size_t offset;
	size_t length;
	size_t index;

	/* Only signature strings have a one-byte length and no four-byte alignment. */
	offset = *cursor;
	if (type == 'g') {
		if (offset >= end)
			return EPROTO;
		length = message->bytes[offset++];
	} else {
		offset = (offset + 3U) & ~(size_t)3U;
		if (offset > end || end - offset < 4)
			return EPROTO;
		length = bus_get32(message->bytes + offset);
		offset += 4;
	}

	/* The terminator must be in this value's declared enclosing range. */
	if (offset >= end || length >= end - offset)
		return EPROTO;
	if (message->bytes[offset + length] != 0)
		return EPROTO;

	/* Embedded NUL bytes cannot truncate a path, member or authority name. */
	for (index = 0; index < length; index++) {
		if (message->bytes[offset + index] == 0)
			return EPROTO;
	}

	/* Succeeded: the returned pointer remains inside the owned message. */
	*string = (const char *)message->bytes + offset;
	*cursor = offset + length + 1;
	return 0;
}

/* Decodes only known header variants and validates received file counts. */
static int
bus_parse(
	struct dbus_reply *message)
{
	const char *variant;
	const char *value;
	size_t cursor;
	size_t end;
	uint32_t number;
	uint32_t seen;
	uint32_t fd_count;
	unsigned code;
	char expected;
	int error;

	/* Framing already established the bounded header-array extent. */
	end = 16U + bus_get32(message->bytes + 12);
	message->body = (end + 7U) & ~(size_t)7U;
	message->cursor = message->body;
	message->type = message->bytes[1];
	message->serial = bus_get32(message->bytes + 8);
	message->signature = "";
	message->error = "unknown";
	seen = 0;
	fd_count = 0;

	/* Header fields are unique structures whose variants must match their codes. */
	cursor = 16;
	while (cursor < end) {
		cursor = (cursor + 7U) & ~(size_t)7U;
		if (cursor >= end)
			return EPROTO;
		code = message->bytes[cursor++];
		if (code == 0 || code > 9)
			return EPROTO;
		if ((seen & (1U << code)) != 0)
			return EPROTO;
		seen |= 1U << code;
		error = reader_string(message, &cursor, end, 'g', &variant);
		if (error != 0)
			return error;

		/* Each known header variant has exactly one simple value. */
		expected = 's';
		if (code == 1)
			expected = 'o';
		if (code == 8)
			expected = 'g';
		if (code == 5 || code == 9)
			expected = 'u';
		if (variant[0] != expected || variant[1] != '\0')
			return EPROTO;

		/* Numeric header fields cannot run into body alignment padding. */
		if (expected == 'u') {
			cursor = (cursor + 3U) & ~(size_t)3U;
			if (cursor > end || end - cursor < 4)
				return EPROTO;
			number = bus_get32(message->bytes + cursor);
			cursor += 4;
			if (code == 5)
				message->reply_serial = number;
			if (code == 9)
				fd_count = number;
		} else {
			/* Text header fields borrow storage from the complete owned frame. */
			error = reader_string(message, &cursor, end, expected, &value);
			if (error != 0)
				return error;
			if (code == 1)
				message->path = value;
			if (code == 2)
				message->interface = value;
			if (code == 3)
				message->member = value;
			if (code == 4)
				message->error = value;
			if (code == 7)
				message->sender = value;
			if (code == 8)
				message->signature = value;
		}
	}

	/* All received rights must have been declared by this exact message. */
	if (fd_count != message->fd_count)
		return EPROTO;
	if (message->type == 2 || message->type == 3) {
		if (message->reply_serial == 0)
			return EPROTO;
	}

	/* Signals need their complete routing identity before reaching a callback. */
	if (message->type == 4) {
		if (message->path == NULL ||
		    message->interface == NULL ||
		    message->member == NULL)
			return EPROTO;
	}

	/* Succeeded: header pointers and ancillary indices are now safe to consume. */
	return 0;
}

/* Receives only the current frame's remaining bytes and owns all its rights. */
static int
bus_receive(
	struct linux_dbus *bus,
	struct dbus_reply **message)
{
	struct msghdr header;
	struct iovec vector;
	struct cmsghdr *control;
	union {
		struct cmsghdr alignment;
		unsigned char bytes[CMSG_SPACE(sizeof(int) * LINUX_DBUS_FDS)];
	} ancillary;
	struct dbus_reply *frame;
	const int *rights;
	ssize_t received;
	size_t index;
	size_t count;
	size_t fields;
	size_t body;
	size_t control_minimum;
	uint32_t serial;
	int error;
	int malformed;

	/* The first sixteen bytes establish an exact bounded receive target. */
	*message = NULL;
	if (bus->receiving == NULL) {
		bus->receiving = calloc(1, sizeof(*bus->receiving));
		if (bus->receiving == NULL)
			return ENOMEM;
		bus->received = 0;
		bus->wanted = 16;
	}

	/* Partial reads may stop at any byte without losing ancillary ownership. */
	frame = bus->receiving;
	control_minimum = CMSG_LEN(0);
	for (;;) {
		memset(&header, 0, sizeof(header));
		vector.iov_base = frame->bytes + bus->received;
		vector.iov_len = bus->wanted - bus->received;
		header.msg_iov = &vector;
		header.msg_iovlen = 1;
		header.msg_control = ancillary.bytes;
		header.msg_controllen = sizeof(ancillary.bytes);
		received = recvmsg(bus->fd, &header, MSG_DONTWAIT | MSG_CMSG_CLOEXEC);
		if (received < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return EAGAIN;
			return errno;
		}

		/* Every delivered right is either retained or immediately closed on overflow. */
		malformed = 0;
		control = CMSG_FIRSTHDR(&header);
		while (control != NULL) {
			if (control->cmsg_level != SOL_SOCKET ||
			    control->cmsg_type != SCM_RIGHTS ||
			    control->cmsg_len < control_minimum) {
				malformed = 1;
			} else {
				/* Kernel-delivered descriptors are CLOEXEC before any callback can spawn. */
				count = (control->cmsg_len - CMSG_LEN(0)) / sizeof(int);
				rights = (const int *)CMSG_DATA(control);

				/* Overflow never leaves an untracked descriptor in the process. */
				for (index = 0; index < count; index++) {
					if (frame->fd_count >= LINUX_DBUS_FDS) {
						(void)close(rights[index]);
						malformed = 1;
					} else {
						frame->fds[frame->fd_count++] = rights[index];
					}
				}
			}

			/* Advances only through ancillary records validated by the kernel. */
			control = CMSG_NXTHDR(&header, control);
		}

		/* Truncation or unknown control data invalidates the complete connection. */
		if ((header.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) != 0 || malformed)
			return EPROTO;
		if (received == 0)
			return EPIPE;
		bus->received += (size_t)received;
		if (bus->received != bus->wanted)
			continue;

		/* Little-endian version-one messages are the agreed bounded vocabulary. */
		if (bus->wanted == 16) {
			if (frame->bytes[0] != 'l' || frame->bytes[3] != 1)
				return EPROTO;
			if (frame->bytes[1] < 1 || frame->bytes[1] > 4)
				return EPROTO;
			if ((frame->bytes[2] & ~7U) != 0)
				return EPROTO;
			serial = bus_get32(frame->bytes + 8);
			if (serial == 0)
				return EPROTO;
			fields = bus_get32(frame->bytes + 12);
			body = bus_get32(frame->bytes + 4);
			if (fields > LINUX_DBUS_MAX - 16)
				return EOVERFLOW;
			bus->wanted = (16 + fields + 7U) & ~(size_t)7U;
			if (bus->wanted > LINUX_DBUS_MAX || body > LINUX_DBUS_MAX - bus->wanted)
				return EOVERFLOW;
			bus->wanted += body;
			if (bus->wanted != bus->received)
				continue;
		}

		/* Only a complete frame exposes routing pointers or transfers its rights. */
		frame->size = bus->received;
		error = bus_parse(frame);
		if (error != 0)
			return error;
		bus->receiving = NULL;
		bus->received = 0;
		bus->wanted = 0;
		*message = frame;

		/* Succeeded: the caller owns this one frame, never bytes from its successor. */
		return 0;
	}
}

/* Retains one signal without replacing or dropping earlier device transitions. */
static int
bus_queue(
	struct linux_dbus *bus,
	struct dbus_reply *message)
{
	/* Queue overflow is a fatal protocol condition rather than a lost pause. */
	if (bus->signal_count >= LINUX_DBUS_SIGNALS)
		return EOVERFLOW;
	bus->signals[bus->signal_count++] = message;

	/* Succeeded: the main loop now owns this signal in arrival order. */
	return 0;
}
