/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host test of the zedBSD CMSG_NXTHDR (plan/ws115/proposed/libc-cmsg-nxthdr.diff).
 *
 * It is compiled against zedBSD's own <sys/socket.h> with the build
 * machine's compiler and calls no library function, so the exit status is
 * the result: 0 when every boundary case answers as expected.
 */

#include <sys/socket.h>

/*
 * Space for three headers and their padding, aligned like a header.
 */
static union {
	struct cmsghdr header;
	unsigned char bytes[3 * 32];
} control;

/*
 * Runs the boundary cases and reports the first one that fails.
 */
int
main(void)
{
	struct msghdr message;
	struct cmsghdr *first;
	struct cmsghdr *second;
	struct cmsghdr *third;
	struct cmsghdr *answer;
	struct cmsghdr *rounded;
	size_t space;

	/* Describes a buffer holding two headers of one int each. */
	space = CMSG_SPACE(sizeof(int));
	message.msg_control = control.bytes;
	message.msg_controllen = 2 * space;
	first = CMSG_FIRSTHDR(&message);
	if (first == (struct cmsghdr *)0)
		return 1;
	first->cmsg_len = CMSG_LEN(sizeof(int));
	second = (struct cmsghdr *)(control.bytes + space);
	second->cmsg_len = CMSG_LEN(sizeof(int));

	/* The step from the first header lands on the second. */
	answer = CMSG_NXTHDR(&message, first);
	if (answer != second)
		return 2;

	/* There is nothing after the last header. */
	answer = CMSG_NXTHDR(&message, second);
	if (answer != (struct cmsghdr *)0)
		return 3;

	/* A null current header asks for the first one. */
	answer = CMSG_NXTHDR(&message, (struct cmsghdr *)0);
	if (answer != first)
		return 4;

	/* A header shorter than its own fixed part ends the walk. */
	first->cmsg_len = sizeof(struct cmsghdr) - 1;
	answer = CMSG_NXTHDR(&message, first);
	if (answer != (struct cmsghdr *)0)
		return 5;

	/* A header longer than the buffer ends the walk. */
	first->cmsg_len = 2 * space + 1;
	answer = CMSG_NXTHDR(&message, first);
	if (answer != (struct cmsghdr *)0)
		return 6;

	/* An unaligned length is rounded up before the step. */
	first->cmsg_len = CMSG_LEN(1);
	rounded = (struct cmsghdr *)(control.bytes + CMSG_ALIGN(CMSG_LEN(1)));
	answer = CMSG_NXTHDR(&message, first);
	if (answer != rounded)
		return 7;

	/* A next header whose recorded length runs past the end ends the walk. */
	first->cmsg_len = CMSG_LEN(sizeof(int));
	second->cmsg_len = space + 1;
	answer = CMSG_NXTHDR(&message, first);
	if (answer != (struct cmsghdr *)0)
		return 8;

	/* A next header whose fixed part does not fit ends the walk. */
	second->cmsg_len = CMSG_LEN(sizeof(int));
	message.msg_controllen = space + sizeof(struct cmsghdr) - 1;
	answer = CMSG_NXTHDR(&message, first);
	if (answer != (struct cmsghdr *)0)
		return 9;

	/* A current header outside the buffer ends the walk. */
	message.msg_controllen = 2 * space;
	third = (struct cmsghdr *)(control.bytes + 2 * space);
	answer = CMSG_NXTHDR(&message, third);
	if (answer != (struct cmsghdr *)0)
		return 10;

	/* Succeeded: every boundary case answered as expected. */
	return 0;
}
