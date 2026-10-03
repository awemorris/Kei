/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws081-p019 (BUG-156): reads the PS/2 mouse's input device for a while
 * and prints what it reported, for the QEMU test of the IntelliMouse
 * protocols (ps2-wheel-qemu.sh).  Opening the device starts the mouse,
 * and so identifies it; the kernel logs "i8042: mouse id=N packet=M".
 * At the end one line:
 *
 *   PS2WHEEL device=PATH x=SUM y=SUM wheel_up=N wheel_down=N hwheel=SUM
 *            left=PRESSES side=PRESSES extra=PRESSES reports=N
 *
 *   ps2wheel [--seconds=N]      (default 10)
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include <uapi/input.h>

/* The input devices looked at, and the name of the one read. */
#define WHEEL_DEVICES	32
#define WHEEL_NAME	"PC/AT PS/2 mouse"

/* What the mouse reported during the run. */
struct wheel_totals {
	long x;
	long y;
	long wheel_up;
	long wheel_down;
	long hwheel;
	long left;
	long side;
	long extra;
	long reports;
};

static int wheel_find(char *path, size_t size);
static int64_t wheel_now_ms(void);

/*
 * Reads the PS/2 mouse until the time is up and prints the totals.
 */
int
main(
	int argc,
	char **argv)
{
	struct input_event events[64];
	struct wheel_totals totals;
	struct pollfd entry;
	char path[64];
	int64_t end;
	int64_t now;
	ssize_t count;
	long seconds;
	int descriptor;
	int ready;
	int index;
	int found;

	/* The run's length. */
	seconds = 10;
	for (index = 1; index < argc; index++) {
		if (strncmp(argv[index], "--seconds=", 10) == 0) {
			seconds = strtol(argv[index] + 10, NULL, 10);
		} else {
			fprintf(stderr, "usage: ps2wheel [--seconds=N]\n");
			return 2;
		}
	}

	/* The PS/2 mouse's device. */
	found = wheel_find(path, sizeof(path));
	if (found != 0) {
		printf("PS2WHEEL none\n");
		return 1;
	}

	/* Opened: the kernel starts and identifies the mouse. */
	descriptor = open(path, O_RDONLY);
	if (descriptor < 0) {
		printf("PS2WHEEL open errno=%d\n", errno);
		return 1;
	}
	printf("PS2WHEEL ready device=%s\n", path);
	fflush(stdout);

	/* Each event until the time is up. */
	memset(&totals, 0, sizeof(totals));
	end = wheel_now_ms() + seconds * 1000;
	for (;;) {
		/* The time left. */
		now = wheel_now_ms();
		if (now >= end)
			break;

		/* Waits for events. */
		entry.fd = descriptor;
		entry.events = POLLIN;
		entry.revents = 0;
		ready = poll(&entry, 1, (int)(end - now));
		if (ready <= 0)
			continue;
		count = read(descriptor, events, sizeof(events));
		if (count <= 0)
			break;

		/* Each event added to the totals. */
		for (index = 0; index < (int)(count / (ssize_t)sizeof(events[0])); index++) {
			if (events[index].type == EV_SYN && events[index].code == SYN_REPORT) {
				totals.reports++;
			} else if (events[index].type == EV_REL && events[index].code == REL_X) {
				totals.x += events[index].value;
			} else if (events[index].type == EV_REL && events[index].code == REL_Y) {
				totals.y += events[index].value;
			} else if (events[index].type == EV_REL && events[index].code == REL_WHEEL) {
				if (events[index].value > 0)
					totals.wheel_up += events[index].value;
				else
					totals.wheel_down -= events[index].value;
			} else if (events[index].type == EV_REL && events[index].code == REL_HWHEEL) {
				totals.hwheel += events[index].value;
			} else if (events[index].type == EV_KEY && events[index].value == 1) {
				if (events[index].code == BTN_LEFT)
					totals.left++;
				else if (events[index].code == BTN_SIDE)
					totals.side++;
				else if (events[index].code == BTN_EXTRA)
					totals.extra++;
			}
		}
	}

	/* The totals. */
	(void)close(descriptor);
	printf("PS2WHEEL device=%s x=%ld y=%ld wheel_up=%ld wheel_down=%ld hwheel=%ld left=%ld side=%ld extra=%ld reports=%ld\n",
	    path, totals.x, totals.y, totals.wheel_up, totals.wheel_down, totals.hwheel, totals.left, totals.side, totals.extra, totals.reports);
	return 0;
}

/* Finds the input device named WHEEL_NAME; returns 0 with its path, 1 when there is none. */
static int
wheel_find(
	char *path,
	size_t size)
{
	char name[128];
	int descriptor;
	int index;
	int result;

	/* Each event device in turn. */
	for (index = 0; index < WHEEL_DEVICES; index++) {
		(void)snprintf(path, size, "/dev/input/event%d", index);
		descriptor = open(path, O_RDONLY | O_NONBLOCK);
		if (descriptor < 0)
			continue;

		/* Its name. */
		memset(name, 0, sizeof(name));
		result = ioctl(descriptor, EVIOCGNAME(sizeof(name) - 1U), name);
		(void)close(descriptor);
		if (result < 0)
			continue;
		if (strcmp(name, WHEEL_NAME) == 0)
			return 0;
	}

	/* None. */
	return 1;
}

/* Gives the monotonic clock in milliseconds. */
static int64_t
wheel_now_ms(void)
{
	struct timespec now;

	(void)clock_gettime(CLOCK_MONOTONIC, &now);
	return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}
