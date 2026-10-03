/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of wl_log_set_handler_client (ws115-p009).
 *
 * A socketpair stands in for the compositor: its end writes a wl_display.error
 * event about the display object, and the client end, zedBSD's
 * libwayland-client built for the host, reads it.  The handler has to receive
 * the standard library's line, "wl_display@1: error 3: boom", once, and the
 * display has to report the protocol error.  The exit status is the result.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <wayland-client.h>

/*
 * The line the handler last received, and how many it received.
 */
static char test_line[256];
static int test_lines;

static void test_log(const char *format, va_list arguments) __attribute__((__format__(__printf__, 1, 0)));

/*
 * Sends the error event and checks what the handler and the display report.
 */
int
main(void)
{
	struct wl_display *display;
	uint32_t event[8];
	int sockets[2];
	int made;
	int dispatched;
	int error;
	int compared;
	ssize_t written;

	/* Connects a client to one end of a socketpair. */
	made = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
	if (made != 0)
		return 1;

	wl_log_set_handler_client(test_log);
	display = wl_display_connect_to_fd(sockets[0]);
	if (display == NULL)
		return 1;

	/*
	 * Writes wl_display.error (object 1, opcode 0): the object 1, code 3 and
	 * the string "boom" (length 5 with its NUL, padded to 8).
	 */
	memset(event, 0, sizeof(event));
	event[0] = 1U;
	event[1] = (28U << 16) | 0U;
	event[2] = 1U;
	event[3] = 3U;
	event[4] = 5U;
	memcpy(&event[5], "boom", 5);
	written = write(sockets[1], event, 28);
	if (written != 28)
		return 1;

	/* Reads it; the dispatch fails with the protocol error. */
	dispatched = wl_display_dispatch(display);
	error = wl_display_get_error(display);
	printf("dispatch=%d error=%d lines=%d line=%s", dispatched, error, test_lines, test_line);

	/* Refuses a dispatch that did not fail with the protocol error. */
	if (dispatched != -1 || error == 0) {
		printf("\nwl-log: FAIL display\n");
		return 1;
	}

	/* Refuses anything but the one standard line. */
	compared = strcmp(test_line, "wl_display@1: error 3: boom\n");
	if (test_lines != 1 || compared != 0) {
		printf("wl-log: FAIL handler\n");
		return 1;
	}

	/* Releases the connection and the peer's end. */
	wl_display_disconnect(display);
	close(sockets[1]);
	printf("wl-log: PASS\n");

	/* Succeeded: the handler had the line. */
	return 0;
}

/* Records a line the library logs. */
static void
test_log(
	const char *format,
	va_list arguments)
{
	/* Keeps the formatted line and counts it. */
	vsnprintf(test_line, sizeof(test_line), format, arguments);
	test_lines++;
}
