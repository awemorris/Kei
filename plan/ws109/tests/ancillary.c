/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Checks actual native Wayland ancillary delivery and recovery through public reads.
 */

#include "userland/desktop/libwayland/internal.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

static int ancillary_rights(void);
static int ancillary_truncated(void);
static int ancillary_send(int socket_fd, int descriptor, size_t count);
static int ancillary_read(struct wl_display *display);

/*
 * Checks fd ownership, native padding and truncated-control recovery.
 */
int
main(
	void)
{
	int error;
	void (*previous)(int);

	/* Makes loss of the last pipe reader observable without terminating this probe. */
	previous = signal(SIGPIPE, SIG_IGN);
	if (previous == SIG_ERR)
		return 1;

	/* Exercises unpadded final controls and delayed rights on an incomplete byte stream. */
	error = ancillary_rights();
	if (error != 0)
		return error;

	/* Exercises the kernel's actual MSG_CTRUNC path and connection cleanup. */
	error = ancillary_truncated();
	if (error != 0)
		return error;

	/* Publishes only the verified native transport contract. */
	(void)puts("ancillary: PASS unpadded rights, delayed FIFO, CLOEXEC, truncation recovery");

	/* Succeeded: the production reader preserved fd identity and recovered every hold. */
	return 0;
}

/* Checks ordered descriptors and the lifetime of unread ancillary payloads. */
static int
ancillary_rights(
	void)
{
	struct wl_display *display;
	int sockets[2];
	int first[2];
	int second[2];
	int error;
	int flags;
	ssize_t bytes;
	char byte;

	/* Creates a real public connection rather than a substituted reader. */
	error = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (error != 0)
		return 1;

	/* Transfers the client endpoint into the production display owner. */
	display = wl_display_connect_to_fd(sockets[0]);
	if (display == NULL)
		return 1;

	/* Creates independently identifiable descriptor payloads. */
	error = pipe(first);
	if (error != 0)
		return 1;

	/* Creates the second descriptor before transferring either reference. */
	error = pipe(second);
	if (error != 0)
		return 1;

	/* Queues one unpadded control alongside an incomplete message byte. */
	error = ancillary_send(sockets[1], first[0], 1);
	if (error != 0)
		return error;

	/* Retains the first rights independently of an incomplete protocol header. */
	error = ancillary_read(display);
	if (error != 0)
		return error;

	/* Appends later rights without discarding the earlier fragmented stream. */
	error = ancillary_send(sockets[1], second[0], 1);
	if (error != 0)
		return error;

	/* Receives the second native control through the same public read barrier. */
	error = ancillary_read(display);
	if (error != 0)
		return error;

	/* Requires both rights to remain in arrival order until a complete event takes them. */
	if (display->input_descriptor_count != 2)
		return 1;

	/* Requires native recvmsg to mark the first transferred fd close-on-exec. */
	flags = fcntl(display->input_descriptors[0], F_GETFD);
	if (flags < 0)
		return 1;

	/* An inherited application child must not retain pending Wayland rights. */
	if ((flags & FD_CLOEXEC) == 0)
		return 1;

	/* Requires the later descriptor to have the same inheritance protection. */
	flags = fcntl(display->input_descriptors[1], F_GETFD);
	if (flags < 0)
		return 1;

	/* Checks protection independently for each native descriptor. */
	if ((flags & FD_CLOEXEC) == 0)
		return 1;

	/* Tags the first pipe so swapped rights cannot satisfy the FIFO assertion. */
	bytes = write(first[1], "A", 1);
	if (bytes != 1)
		return 1;

	/* Checks the actual open-file identity of the earlier reference. */
	bytes = read(display->input_descriptors[0], &byte, 1);
	if (bytes != 1 || byte != 'A')
		return 1;

	/* Tags the later pipe independently of the earlier one. */
	bytes = write(second[1], "B", 1);
	if (bytes != 1)
		return 1;

	/* Checks the actual open-file identity of the later reference. */
	bytes = read(display->input_descriptors[1], &byte, 1);
	if (bytes != 1 || byte != 'B')
		return 1;

	/* Leaves only the display-owned read ends before retiring the connection. */
	(void)close(first[0]);
	(void)close(second[0]);
	wl_display_disconnect(display);

	/* Disconnect must release even rights belonging to an incomplete protocol message. */
	bytes = write(first[1], "X", 1);
	if (bytes != -1 || errno != EPIPE)
		return 1;

	/* Checks release of the later reference independently. */
	bytes = write(second[1], "Y", 1);
	if (bytes != -1 || errno != EPIPE)
		return 1;

	/* Releases the probe's remaining endpoints after verifying connection ownership. */
	(void)close(first[1]);
	(void)close(second[1]);
	(void)close(sockets[1]);

	/* Succeeded: both received descriptors kept their identities and were retired. */
	return 0;
}

/* Checks that excess kernel-delivered rights never survive a rejected connection. */
static int
ancillary_truncated(
	void)
{
	struct wl_display *display;
	int sockets[2];
	int pipe_fds[2];
	int error;
	ssize_t bytes;

	/* Creates a separate connection whose failure cannot taint the successful case. */
	error = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (error != 0)
		return 1;

	/* Gives the production reader its normal connection ownership. */
	display = wl_display_connect_to_fd(sockets[0]);
	if (display == NULL)
		return 1;

	/* Uses one pipe so any leaked duplicate keeps a directly observable reader alive. */
	error = pipe(pipe_fds);
	if (error != 0)
		return 1;

	/* Exceeds the reader's eight-right ancillary buffer through a real kernel send. */
	error = ancillary_send(sockets[1], pipe_fds[0], 64);
	if (error != 0)
		return error;

	/* Requires the public read to reject the actual truncated control. */
	error = ancillary_read(display);
	if (error == 0)
		return 1;

	/* Requires the rejection to describe a transport violation rather than fake success. */
	error = wl_display_get_error(display);
	if (error != EPROTO)
		return 1;

	/* Leaves no legitimate read owner after the rejected connection is retired. */
	(void)close(pipe_fds[0]);
	wl_display_disconnect(display);

	/* Any leaked ancillary duplicate would make this write succeed. */
	bytes = write(pipe_fds[1], "X", 1);
	if (bytes != -1 || errno != EPIPE)
		return 1;

	/* Releases only endpoints still owned by the probe. */
	(void)close(pipe_fds[1]);
	(void)close(sockets[1]);

	/* Succeeded: truncated native rights retained no pipe-reader reference. */
	return 0;
}

/* Sends an unpadded final rights record with a single incomplete wire byte. */
static int
ancillary_send(
	int socket_fd,
	int descriptor,
	size_t count)
{
	struct msghdr message;
	struct iovec vector;
	struct cmsghdr *control;
	union {
		struct cmsghdr alignment;
		unsigned char bytes[CMSG_SPACE(64 * sizeof(int))];
	} ancillary;
	int descriptors[64];
	size_t index;
	ssize_t bytes;
	char byte = 0;

	/* Bounds the fixed sender storage independently of the receiver's smaller buffer. */
	if (count > 64)
		return 1;

	/* Supplies independently duplicated kernel references to the same open file. */
	for (index = 0; index < count; index++)
		descriptors[index] = descriptor;

	/* Describes the fragmented stream payload without inventing a complete protocol event. */
	memset(&message, 0, sizeof(message));
	vector.iov_base = &byte;
	vector.iov_len = 1;
	message.msg_iov = &vector;
	message.msg_iovlen = 1;

	/* Omits final padding deliberately; the receiver must still retain every valid fd. */
	memset(&ancillary, 0, sizeof(ancillary));
	control = (struct cmsghdr *)ancillary.bytes;
	control->cmsg_level = SOL_SOCKET;
	control->cmsg_type = SCM_RIGHTS;
	control->cmsg_len = CMSG_LEN(count * sizeof(int));
	memcpy(CMSG_DATA(control), descriptors, count * sizeof(int));
	message.msg_control = ancillary.bytes;
	message.msg_controllen = control->cmsg_len;

	/* Sends actual rights through the native kernel rather than overriding recvmsg. */
	bytes = sendmsg(socket_fd, &message, MSG_NOSIGNAL);
	if (bytes != 1)
		return 1;

	/* Succeeded: the kernel accepted the complete ancillary reference set. */
	return 0;
}

/* Receives one native socket payload through the public coordinated read API. */
static int
ancillary_read(
	struct wl_display *display)
{
	int error;

	/* Registers the read hold required by ordinary Wayland event loops. */
	error = wl_display_prepare_read(display);
	if (error != 0)
		return error;

	/* Lets the production wire parser retain or recover the kernel's delivered rights. */
	error = wl_display_read_events(display);
	if (error != 0)
		return error;

	/* Succeeded: the public read barrier released its hold after native delivery. */
	return 0;
}
