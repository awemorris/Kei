/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Reading and writing whole files.
 */

#include "base/base.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

/* How many bytes one read asks for. */
#define FILE_CHUNK	(64U * 1024U)

static int file_write_all(int descriptor, const unsigned char *bytes, size_t length);

/*
 * Appends the whole contents of a file to a buffer.
 */
int
wb_file_read(
	const char *path,
	struct wb_buffer *buffer)
{
	ssize_t count;
	int descriptor;
	int error;

	/* Opens the file. */
	descriptor = open(path, O_RDONLY);
	if (descriptor < 0)
		return errno;

	/* Reads chunks until the end of the file. */
	for (;;) {
		error = wb_buffer_reserve(buffer, FILE_CHUNK);
		if (error != 0) {
			close(descriptor);
			return error;
		}

		/* Reads the next chunk into the room. */
		count = read(descriptor, buffer->data + buffer->length, FILE_CHUNK);
		if (count < 0) {
			/* A read cut short by a signal is tried again. */
			error = errno;
			if (error == EINTR)
				continue;

			/* Any other failure ends the reading. */
			close(descriptor);
			return error;
		}

		/* An empty read is the end of the file. */
		if (count == 0)
			break;

		/* Keeps the bytes read and the NUL after them. */
		buffer->length += (size_t)count;
		buffer->data[buffer->length] = '\0';
	}

	/* Closes the file. */
	close(descriptor);

	/* Succeeded: the contents are at the end of the buffer. */
	return 0;
}

/*
 * Replaces a file with length bytes.
 */
int
wb_file_write(
	const char *path,
	const void *bytes,
	size_t length)
{
	int descriptor;
	int error;

	/* Creates or empties the file. */
	descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (descriptor < 0)
		return errno;

	/* Writes every byte. */
	error = file_write_all(descriptor, bytes, length);
	if (error != 0) {
		close(descriptor);
		return error;
	}

	/* Closes the file, which can report a failed write. */
	error = close(descriptor);
	if (error != 0)
		return errno;

	/* Succeeded: the file holds the bytes. */
	return 0;
}

/* Writes every byte to a descriptor, retrying short writes. */
static int
file_write_all(
	int descriptor,
	const unsigned char *bytes,
	size_t length)
{
	ssize_t count;
	size_t offset;
	int error;

	/* Writes until nothing is left. */
	offset = 0;
	while (offset < length) {
		count = write(descriptor, bytes + offset, length - offset);
		if (count < 0) {
			/* A write cut short by a signal is tried again. */
			error = errno;
			if (error == EINTR)
				continue;

			/* Any other failure ends the writing. */
			return error;
		}

		/* Moves past the bytes written. */
		offset += (size_t)count;
	}

	/* Succeeded: every byte was written. */
	return 0;
}
