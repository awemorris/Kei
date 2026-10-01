/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Checks real native forkpty controlling-terminal and bidirectional stream behavior. */
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int child_terminal(void);
static int parent_terminal(int descriptor);

/* Owns a real native PTY child and retires both process and descriptor on every outcome. */
int
main(
	void)
{
	pid_t child;
	pid_t waited;
	int descriptor;
	int error;
	int status;
	int exited;
	int exit_code;

	/* Native libutil creates the child's session and controlling terminal. */
	child = forkpty(&descriptor, NULL, NULL, NULL);
	if (child < 0)
		return 1;

	/* The child returns only its actual terminal inquiry and stream result. */
	if (child == 0) {
		error = child_terminal();
		_exit(error);
	}

	/* A finite parent exchange cannot leave a hung child or a borrowed master behind. */
	error = parent_terminal(descriptor);
	if (error != 0)
		(void)kill(child, SIGKILL);

	/* Reaps only the process created by this probe, including failed exchanges. */
	waited = waitpid(child, &status, 0);
	(void)close(descriptor);
	if (waited != child)
		return 1;

	/* Parent stream failure is reported after the child and master have retired. */
	if (error != 0)
		return 1;

	/* Native child exit must confirm its terminal and canonical input checks passed. */
	exited = WIFEXITED(status);
	if (exited == 0)
		return 1;

	/* An abnormal child result cannot supply the successful native PTY contract. */
	exit_code = WEXITSTATUS(status);
	if (exit_code != 0)
		return 1;

	/* Succeeded: native controlling-terminal identity and both data directions were verified. */
	(void)puts("PASS native forkpty controlling terminal/input/output/close/wait");
	return 0;
}

/* Reads canonical terminal input and writes a reply from an actual controlling-terminal session. */
static int
child_terminal(
	void)
{
	char input[64];
	ssize_t size;
	pid_t terminal;
	pid_t session;
	int attached;
	int same;

	/* The slave must be a terminal rather than an ordinary pipe or socket. */
	attached = isatty(STDIN_FILENO);
	if (attached != 1)
		return 1;

	/* Independently verifies that native forkpty installed this session's controlling terminal. */
	terminal = tcgetsid(STDIN_FILENO);
	if (terminal < 0)
		return 1;

	/* The child session ID must match the terminal's controlling session. */
	session = getsid(0);
	if (session != terminal)
		return 1;

	/* The native line discipline delivers the parent's complete newline-terminated input. */
	size = read(STDIN_FILENO, input, sizeof(input));
	if (size != sizeof("native-input\n") - 1)
		return 1;

	/* The child must see the exact input written through its master. */
	same = memcmp(input, "native-input\n", sizeof("native-input\n") - 1);
	if (same != 0)
		return 1;

	/* Returns an independently generated reply through the slave terminal. */
	size = write(STDOUT_FILENO, "native-output\n", sizeof("native-output\n") - 1);
	if (size != sizeof("native-output\n") - 1)
		return 1;

	/* Succeeded: native session, canonical input and slave output all behave as a real PTY. */
	return 0;
}

/* Sends input to the real master and waits a bounded interval for the child's reply. */
static int
parent_terminal(
	int descriptor)
{
	struct pollfd readiness;
	char output[256];
	char *found;
	ssize_t size;
	size_t used;
	int attempt;
	int error;

	/* Sends the line through the master while preserving its caller's descriptor owner. */
	size = write(descriptor, "native-input\n", sizeof("native-input\n") - 1);
	if (size != sizeof("native-input\n") - 1)
		return EIO;

	/* Gives actual terminal output a finite window without requiring a fake child or time source. */
	used = 0;
	output[0] = '\0';
	for (attempt = 0; attempt < 100; attempt++) {
		readiness.fd = descriptor;
		readiness.events = POLLIN;
		readiness.revents = 0;
		error = poll(&readiness, 1, 20);
		if (error < 0)
			return errno;

		/* A quiet terminal still has time remaining in the same bounded exchange. */
		if (error == 0)
			continue;

		/* Leaves one byte for a terminator before reading an actual master output segment. */
		size = read(descriptor, output + used, sizeof(output) - used - 1);
		if (size < 0)
			return errno;

		/* EOF before the reply means the child failed its independent terminal contract. */
		if (size == 0)
			return EIO;

		/* Output extent belongs to this master snapshot, including line-discipline echo. */
		used += (size_t)size;
		output[used] = '\0';
		found = strstr(output, "native-output");
		if (found != NULL)
			break;

		/* A full unexpected stream cannot be treated as the expected finite reply. */
		if (used == sizeof(output) - 1)
			return EOVERFLOW;
	}

	/* A missing child reply is a bounded timeout rather than an indefinite wait. */
	if (attempt == 100)
		return ETIMEDOUT;

	/* Succeeded: the native child replied through the actual kernel PTY stream. */
	return 0;
}
