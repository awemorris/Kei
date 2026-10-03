/*
 * ws005-p025 (BUG-149): what poll reports after a socket's own write shutdown.
 * Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
 *
 * A client that writes its request, shuts its writing down, and polls for
 * POLLIN must wait until the answer (or the end of the stream) arrives: the
 * write shutdown withholds POLLOUT and is not an error.  The peer's close
 * still reports POLLHUP, a peer that stopped reading still reports an error
 * for the writer, and a pending socket error still reports POLLERR.
 *
 * TCP (BUG-151, ws005-p027) is held to the same rule: a client's own
 * SHUT_WR reports nothing until the server answers, the answer is
 * readable, and the server's FIN after it reports POLLHUP.
 *
 * Each case prints "PASS name", "FAIL name ..." or "SKIP name ...", and the
 * last line is "RESULT pass=N fail=N".
 * The exit status is 0 only when no case failed.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The number of cases that passed so far; only main's thread counts. */
static int bug149_passed;

/* The number of cases that failed so far; only main's thread counts. */
static int bug149_failed;

static long bug149_now_ms(void);
static int bug149_poll(int descriptor, short events, int timeout_ms, short *revents, long *elapsed_ms);
static void bug149_result(const char *name, int passed, const char *detail);
static void bug149_loopback(struct sockaddr_in *address, unsigned short port);
static void bug149_shutwr_nothing_yet(void);
static void bug149_shutwr_waits_for_answer(void);
static void bug149_nonblocking_waits(void);
static void bug149_peer_close_hangs_up(void);
static void bug149_shut_rdwr(void);
static void bug149_peer_stopped_reading(void);
static void bug149_datagram_shutwr(void);
static void bug149_udp_shutwr(void);
static void bug149_socket_error(void);
static void bug149_tcp_shutwr(void);

/* Runs every case and reports the totals. */
int
main(void)
{
	/* The cases, each on fresh sockets. */
	bug149_shutwr_nothing_yet();
	bug149_shutwr_waits_for_answer();
	bug149_nonblocking_waits();
	bug149_peer_close_hangs_up();
	bug149_shut_rdwr();
	bug149_peer_stopped_reading();
	bug149_datagram_shutwr();
	bug149_udp_shutwr();
	bug149_socket_error();
	bug149_tcp_shutwr();

	/* The totals. */
	printf("RESULT pass=%d fail=%d\n", bug149_passed, bug149_failed);
	if (bug149_failed != 0)
		return 1;

	/* Succeeded: every case passed or was skipped. */
	return 0;
}

/* The monotonic clock in milliseconds. */
static long
bug149_now_ms(void)
{
	struct timespec now;
	int failed;

	/* A failed clock reads as zero, which only shortens a measured wait. */
	failed = clock_gettime(CLOCK_MONOTONIC, &now);
	if (failed != 0)
		return 0;

	/* Succeeded. */
	return (long)now.tv_sec * 1000L + now.tv_nsec / 1000000L;
}

/* Polls one descriptor and measures how long the poll took. */
static int
bug149_poll(
	int descriptor,
	short events,
	int timeout_ms,
	short *revents,
	long *elapsed_ms)
{
	struct pollfd entry;
	long start;
	int ready;

	/* One descriptor, the events asked. */
	entry.fd = descriptor;
	entry.events = events;
	entry.revents = 0;
	start = bug149_now_ms();
	ready = poll(&entry, 1, timeout_ms);
	*elapsed_ms = bug149_now_ms() - start;
	*revents = entry.revents;

	/* The poll's own result. */
	return ready;
}

/* Prints one case's outcome and counts it. */
static void
bug149_result(
	const char *name,
	int passed,
	const char *detail)
{
	/* A pass. */
	if (passed) {
		bug149_passed++;
		printf("PASS %s %s\n", name, detail);
		return;
	}

	/* A failure. */
	bug149_failed++;
	printf("FAIL %s %s\n", name, detail);
}

/* Fills in a loopback IPv4 address with a port (0 lets the system choose). */
static void
bug149_loopback(
	struct sockaddr_in *address,
	unsigned short port)
{
	/* 127.0.0.1 and the port, in network order. */
	memset(address, 0, sizeof(*address));
	address->sin_family = AF_INET;
	address->sin_port = htons(port);
	address->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
}

/* After the request and the write shutdown, nothing is readable yet. */
static void
bug149_shutwr_nothing_yet(void)
{
	char detail[128];
	int pair[2];
	short revents;
	long elapsed;
	int failed;
	int ready;

	/* A connected stream pair. */
	failed = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	if (failed != 0) {
		bug149_result("stream-shutwr-nothing-yet", 0, "socketpair failed");
		return;
	}

	/* The request, then the end of this side's writing. */
	(void)write(pair[0], "req", 3);
	(void)shutdown(pair[0], SHUT_WR);

	/* No answer has been written, so nothing is ready. */
	ready = bug149_poll(pair[0], POLLIN, 0, &revents, &elapsed);
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x", ready, (unsigned)revents);
	bug149_result("stream-shutwr-nothing-yet", ready == 0 && revents == 0, detail);
	(void)close(pair[0]);
	(void)close(pair[1]);
}

/* A poll for POLLIN after the write shutdown wakes at the answer, then at the end. */
static void
bug149_shutwr_waits_for_answer(void)
{
	struct timespec delay;
	char detail[160];
	char buffer[16];
	int pair[2];
	short revents;
	long elapsed;
	ssize_t count;
	pid_t child;
	int failed;
	int ready;
	int passed;

	/* A connected stream pair. */
	failed = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	if (failed != 0) {
		bug149_result("stream-shutwr-waits-answer", 0, "socketpair failed");
		return;
	}

	/* The server reads the request to its end, waits 300 ms, answers, and closes. */
	child = fork();
	if (child == 0) {
		(void)close(pair[0]);
		for (;;) {
			count = read(pair[1], buffer, sizeof(buffer));
			if (count <= 0)
				break;
		}

		/* The server's slow work, then its answer. */
		delay.tv_sec = 0;
		delay.tv_nsec = 300000000L;
		(void)nanosleep(&delay, NULL);
		(void)write(pair[1], "ans", 3);
		_exit(0);
	}

	/* The client's request and write shutdown. */
	(void)close(pair[1]);
	(void)write(pair[0], "req", 3);
	(void)shutdown(pair[0], SHUT_WR);

	/* The poll waits for the answer, which comes after about 300 ms. */
	ready = bug149_poll(pair[0], POLLIN, 3000, &revents, &elapsed);
	passed = ready == 1;
	if ((revents & POLLIN) == 0)
		passed = 0;
	if ((revents & POLLERR) != 0)
		passed = 0;
	if (elapsed < 200)
		passed = 0;
	count = read(pair[0], buffer, sizeof(buffer));
	if (count != 3)
		passed = 0;
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x elapsed=%ldms read=%ld",
	    ready, (unsigned)revents, elapsed, (long)count);
	bug149_result("stream-shutwr-waits-answer", passed, detail);

	/* After the server has gone, the end of the stream: POLLIN and POLLHUP, no error. */
	(void)waitpid(child, NULL, 0);
	ready = bug149_poll(pair[0], POLLIN, 1000, &revents, &elapsed);
	passed = ready == 1;
	if ((revents & POLLHUP) == 0)
		passed = 0;
	if ((revents & POLLERR) != 0)
		passed = 0;
	count = read(pair[0], buffer, sizeof(buffer));
	if (count != 0)
		passed = 0;
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x read=%ld", ready, (unsigned)revents, (long)count);
	bug149_result("stream-shutwr-peer-closed-hup", passed, detail);
	(void)close(pair[0]);
}

/* A nonblocking client's poll after its write shutdown sleeps until its timeout. */
static void
bug149_nonblocking_waits(void)
{
	char detail[128];
	int pair[2];
	short revents;
	long elapsed;
	int failed;
	int ready;

	/* A nonblocking stream pair, as zsv1-client uses. */
	failed = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, pair);
	if (failed != 0) {
		bug149_result("stream-nonblock-shutwr-sleeps", 0, "socketpair failed");
		return;
	}

	/* The request, then the end of this side's writing. */
	(void)write(pair[0], "req", 3);
	(void)shutdown(pair[0], SHUT_WR);

	/* No answer comes, so the poll times out after about 500 ms instead of returning at once. */
	ready = bug149_poll(pair[0], POLLIN, 500, &revents, &elapsed);
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x elapsed=%ldms", ready, (unsigned)revents, elapsed);
	bug149_result("stream-nonblock-shutwr-sleeps", ready == 0 && elapsed >= 400, detail);
	(void)close(pair[0]);
	(void)close(pair[1]);
}

/* The peer's close is a hang-up and an end of file. */
static void
bug149_peer_close_hangs_up(void)
{
	char detail[128];
	char buffer[4];
	int pair[2];
	short revents;
	long elapsed;
	ssize_t count;
	int failed;
	int ready;
	int passed;

	/* A connected stream pair. */
	failed = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	if (failed != 0) {
		bug149_result("stream-peer-close-hup", 0, "socketpair failed");
		return;
	}

	/* The other end closes. */
	(void)close(pair[1]);

	/* POLLHUP and a readable end of file. */
	ready = bug149_poll(pair[0], POLLIN | POLLOUT, 0, &revents, &elapsed);
	passed = ready == 1;
	if ((revents & POLLHUP) == 0)
		passed = 0;
	if ((revents & POLLIN) == 0)
		passed = 0;
	count = read(pair[0], buffer, sizeof(buffer));
	if (count != 0)
		passed = 0;
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x read=%ld", ready, (unsigned)revents, (long)count);
	bug149_result("stream-peer-close-hup", passed, detail);
	(void)close(pair[0]);
}

/* A shutdown of both directions is an end of file without an error. */
static void
bug149_shut_rdwr(void)
{
	char detail[128];
	int pair[2];
	short revents;
	long elapsed;
	int failed;
	int ready;
	int passed;

	/* A connected stream pair. */
	failed = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	if (failed != 0) {
		bug149_result("stream-shut-rdwr-hup", 0, "socketpair failed");
		return;
	}

	/* This end shuts both directions down. */
	(void)shutdown(pair[0], SHUT_RDWR);

	/* Readable (end of file) and hung up, but no error and no POLLOUT. */
	ready = bug149_poll(pair[0], POLLIN | POLLOUT, 0, &revents, &elapsed);
	passed = ready == 1;
	if ((revents & POLLIN) == 0)
		passed = 0;
	if ((revents & POLLHUP) == 0)
		passed = 0;
	if ((revents & POLLERR) != 0)
		passed = 0;
	if ((revents & POLLOUT) != 0)
		passed = 0;
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x", ready, (unsigned)revents);
	bug149_result("stream-shut-rdwr-hup", passed, detail);
	(void)close(pair[0]);
	(void)close(pair[1]);
}

/* A writer whose peer stopped reading is told so (a write would fail with EPIPE). */
static void
bug149_peer_stopped_reading(void)
{
	char detail[128];
	int pair[2];
	short revents;
	long elapsed;
	int failed;
	int ready;
	int passed;

	/* A connected stream pair. */
	failed = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
	if (failed != 0) {
		bug149_result("stream-peer-shut-rd-err", 0, "socketpair failed");
		return;
	}

	/* The other end shuts its reading down. */
	(void)shutdown(pair[1], SHUT_RD);

	/* The writer is not offered POLLOUT and sees the error, as before the change. */
	ready = bug149_poll(pair[0], POLLOUT, 0, &revents, &elapsed);
	passed = ready == 1;
	if ((revents & POLLERR) == 0)
		passed = 0;
	if ((revents & POLLOUT) != 0)
		passed = 0;
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x", ready, (unsigned)revents);
	bug149_result("stream-peer-shut-rd-err", passed, detail);
	(void)close(pair[0]);
	(void)close(pair[1]);
}

/* A datagram socket's write shutdown withholds POLLOUT and is not an error. */
static void
bug149_datagram_shutwr(void)
{
	char detail[128];
	int pair[2];
	short revents;
	long elapsed;
	int failed;
	int ready;

	/* A connected datagram pair. */
	failed = socketpair(AF_UNIX, SOCK_DGRAM, 0, pair);
	if (failed != 0) {
		bug149_result("dgram-shutwr-no-err", 0, "socketpair failed");
		return;
	}

	/* This end shuts its writing down. */
	(void)shutdown(pair[0], SHUT_WR);

	/* Nothing to read, nothing writable, no error. */
	ready = bug149_poll(pair[0], POLLIN | POLLOUT, 0, &revents, &elapsed);
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x", ready, (unsigned)revents);
	bug149_result("dgram-shutwr-no-err", ready == 0 && revents == 0, detail);
	(void)close(pair[0]);
	(void)close(pair[1]);
}

/* A UDP socket's write shutdown withholds POLLOUT and is not an error. */
static void
bug149_udp_shutwr(void)
{
	struct sockaddr_in address;
	char detail[128];
	short revents;
	long elapsed;
	int descriptor;
	int failed;
	int ready;

	/* A UDP socket. */
	descriptor = socket(AF_INET, SOCK_DGRAM, 0);
	if (descriptor < 0) {
		printf("SKIP udp-shutwr-no-err socket errno=%d\n", errno);
		return;
	}

	/* Connected to the loopback discard port. */
	bug149_loopback(&address, 9);
	failed = connect(descriptor, (struct sockaddr *)&address, sizeof(address));
	if (failed != 0) {
		printf("SKIP udp-shutwr-no-err connect errno=%d\n", errno);
		(void)close(descriptor);
		return;
	}

	/* Its writing shut down (a family that cannot shut down has nothing to check). */
	failed = shutdown(descriptor, SHUT_WR);
	if (failed != 0) {
		printf("SKIP udp-shutwr-no-err shutdown errno=%d\n", errno);
		(void)close(descriptor);
		return;
	}

	/* No error and no POLLOUT. */
	ready = bug149_poll(descriptor, POLLIN | POLLOUT, 0, &revents, &elapsed);
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x", ready, (unsigned)revents);
	bug149_result("udp-shutwr-no-err", (revents & (POLLERR | POLLOUT)) == 0, detail);
	(void)close(descriptor);
}

/* A pending socket error still reports POLLERR (a refused loopback TCP connection). */
static void
bug149_socket_error(void)
{
	struct sockaddr_in address;
	char detail[160];
	socklen_t length;
	short revents;
	long elapsed;
	int descriptor;
	int error;
	int failed;
	int ready;
	int passed;

	/* A nonblocking TCP socket. */
	descriptor = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (descriptor < 0) {
		printf("SKIP tcp-refused-pollerr socket errno=%d\n", errno);
		return;
	}

	/* A connection to a loopback port nobody listens on. */
	bug149_loopback(&address, 1);
	failed = connect(descriptor, (struct sockaddr *)&address, sizeof(address));
	if (failed == 0) {
		printf("SKIP tcp-refused-pollerr port 1 accepted\n");
		(void)close(descriptor);
		return;
	}

	/* A refusal reported at once leaves no pending error to poll for. */
	if (errno != EINPROGRESS) {
		printf("SKIP tcp-refused-pollerr connect errno=%d (refused at once)\n", errno);
		(void)close(descriptor);
		return;
	}

	/* The refusal arrives as POLLERR with SO_ERROR set. */
	ready = bug149_poll(descriptor, POLLOUT, 3000, &revents, &elapsed);
	error = 0;
	length = sizeof(error);
	(void)getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &error, &length);
	passed = ready == 1;
	if ((revents & POLLERR) == 0)
		passed = 0;
	if (error == 0)
		passed = 0;
	snprintf(detail, sizeof(detail), "ready=%d revents=0x%x so_error=%d", ready, (unsigned)revents, error);
	bug149_result("tcp-refused-pollerr", passed, detail);
	(void)close(descriptor);
}

/*
 * A loopback TCP client writes, shuts its writing down and polls (BUG-151):
 * nothing until the server answers, then the answer, then the server's FIN
 * as POLLHUP with the end of the stream.
 */
static void
bug149_tcp_shutwr(void)
{
	struct sockaddr_in address;
	socklen_t length;
	char detail[96];
	char answer[8];
	ssize_t count;
	ssize_t sent;
	ssize_t again;
	short revents;
	long elapsed;
	int listener;
	int client;
	int server;
	int failed;
	int ready;

	/* A TCP listener on a loopback port of the system's choice. */
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		printf("SKIP tcp-shutwr socket errno=%d\n", errno);
		return;
	}
	bug149_loopback(&address, 0);
	failed = bind(listener, (struct sockaddr *)&address, sizeof(address));
	if (failed == 0)
		failed = listen(listener, 1);
	length = sizeof(address);
	if (failed == 0)
		failed = getsockname(listener, (struct sockaddr *)&address, &length);
	if (failed != 0) {
		printf("SKIP tcp-shutwr listen errno=%d\n", errno);
		(void)close(listener);
		return;
	}

	/* The client connects and the server takes the connection. */
	client = socket(AF_INET, SOCK_STREAM, 0);
	failed = -1;
	if (client >= 0)
		failed = connect(client, (struct sockaddr *)&address, sizeof(address));
	server = -1;
	if (failed == 0)
		server = accept(listener, NULL, NULL);
	if (failed != 0 || server < 0) {
		printf("SKIP tcp-shutwr connect/accept errno=%d\n", errno);
		if (client >= 0)
			(void)close(client);
		(void)close(listener);
		return;
	}

	/* The request, and the client's writing shut down: nothing to report yet. */
	(void)write(client, "req", 3);
	(void)shutdown(client, SHUT_WR);
	ready = bug149_poll(client, POLLIN, 0, &revents, &elapsed);
	(void)snprintf(detail, sizeof(detail), "ready=%d revents=0x%x", ready, (unsigned)revents);
	bug149_result("tcp-shutwr-nothing-yet", ready == 0 && revents == 0, detail);

	/*
	 * The server answers after the client's FIN (its side in CLOSE_WAIT may
	 * still send) and shuts its side: the answer is readable.  The reads do
	 * not wait (MSG_DONTWAIT), so a kernel that has nothing to give fails
	 * the case instead of hanging the probe.
	 */
	sent = write(server, "ans", 3);
	(void)shutdown(server, SHUT_WR);
	ready = bug149_poll(client, POLLIN, 2000, &revents, &elapsed);
	memset(answer, 0, sizeof(answer));
	count = -1;
	if (ready == 1 && (revents & POLLIN) != 0)
		count = recv(client, answer, sizeof(answer) - 1U, MSG_DONTWAIT);
	(void)snprintf(detail, sizeof(detail), "sent=%ld ready=%d revents=0x%x read=%ld text=%s", (long)sent, ready, (unsigned)revents, (long)count, answer);
	bug149_result("tcp-shutwr-answer", sent == 3 && count == 3 && strcmp(answer, "ans") == 0, detail);

	/* The server's FIN after the answer: the end of the stream and a hangup, and the end again on the next read. */
	ready = bug149_poll(client, POLLIN, 2000, &revents, &elapsed);
	count = -1;
	if (ready == 1 && (revents & POLLIN) != 0)
		count = recv(client, answer, sizeof(answer) - 1U, MSG_DONTWAIT);
	again = recv(client, answer, sizeof(answer) - 1U, MSG_DONTWAIT);
	(void)snprintf(detail, sizeof(detail), "ready=%d revents=0x%x read=%ld again=%ld", ready, (unsigned)revents, (long)count, (long)again);
	bug149_result("tcp-peer-fin-hangs-up", count == 0 && again == 0 && (revents & POLLHUP) != 0, detail);

	/* The sockets go. */
	(void)close(client);
	(void)close(server);
	(void)close(listener);
}
