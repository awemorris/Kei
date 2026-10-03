/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The terminal's own settings, kept from one run to the next in
 * ~/.config/keiland/terminal.conf (ws128-p009).
 *
 * The file is key=value lines; a line starting with # and an empty line
 * are ignored.  Today it has one key, ambiguous-wide (0 or 1: View > Treat
 * Ambiguous-Width Characters as Wide).  A change rewrites the file with
 * that key replaced and every other line kept, into a new file beside it
 * that is flushed to the disk and renamed over the old one, so a terminal
 * starting at the same time never reads half a file.  The windows and tabs
 * of the terminal that changed it follow at once; another terminal reads
 * the file when it starts.
 *
 * The terminal does not share the desktop's preferences file
 * (~/.config/keiland/desktop.conf): that one is the compositor's.
 */

#include "terminal.h"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* The bytes of a path with its NUL. */
#define SETTINGS_PATH_MAX	1024U

/* The largest settings file read or written, in bytes. */
#define SETTINGS_FILE_MAX	16384U

/* The folders under the home, and the file's name in the last one. */
#define SETTINGS_CONFIG		".config"
#define SETTINGS_FOLDER		".config/keiland"
#define SETTINGS_NAME		"terminal.conf"

/* The key of the Ambiguous-width setting. */
#define SETTINGS_AMBIGUOUS_WIDE	"ambiguous-wide"

static int settings_home(char *home, size_t size);
static int settings_path(char *path, size_t size, const char *suffix);
static int settings_read(const char *path, char *text, size_t capacity, size_t *length);
static int settings_line_key(const char *line, size_t length, const char *key);
static int settings_write(const char *path, const char *text, size_t length);

/*
 * Reads the settings from the file into settings; a missing file, or a key
 * missing from it, leaves the default (everything off).
 */
void
terminal_settings_load(
	struct terminal_settings *settings)
{
	char path[SETTINGS_PATH_MAX];
	char text[SETTINGS_FILE_MAX];
	const char *line;
	const char *end;
	size_t length;
	size_t key_length;
	int matches;
	int error;

	/* The defaults: the terminal as it was before the setting existed. */
	memset(settings, 0, sizeof(*settings));

	/* The file's path; without a home there is nothing to read. */
	error = settings_path(path, sizeof(path), "");
	if (error != 0)
		return;

	/* The whole file; a missing or unreadable one keeps the defaults. */
	error = settings_read(path, text, sizeof(text), &length);
	if (error != 0)
		return;

	/* Each line, looking for the keys the terminal knows. */
	key_length = strlen(SETTINGS_AMBIGUOUS_WIDE);
	line = text;
	while (line < text + length) {
		/* The line runs to its newline or to the end of the file. */
		end = memchr(line, '\n', (size_t)(text + length - line));
		if (end == NULL)
			end = text + length;

		/* ambiguous-wide=1 turns the setting on; any other value leaves it off. */
		matches = settings_line_key(line, (size_t)(end - line), SETTINGS_AMBIGUOUS_WIDE);
		if (matches) {
			/* A later line of the key overrides an earlier one. */
			settings->ambiguous_wide = 0;

			/* The value is the one character 1. */
			if ((size_t)(end - line) == key_length + 2U && line[key_length + 1U] == '1')
				settings->ambiguous_wide = 1;
		}

		/* The next line starts after the newline. */
		line = end + 1;
	}
}

/*
 * Writes the settings to the file, keeping the lines of other keys.
 *
 * Returns 0, or an errno value: ENOENT (no home), ENAMETOOLONG, E2BIG (a
 * file too large) or one of making the folder or writing the file.
 */
int
terminal_settings_save(
	const struct terminal_settings *settings)
{
	char home[SETTINGS_PATH_MAX];
	char folder[SETTINGS_PATH_MAX];
	char path[SETTINGS_PATH_MAX];
	char old_text[SETTINGS_FILE_MAX];
	char new_text[SETTINGS_FILE_MAX];
	const char *line;
	const char *end;
	size_t old_length;
	size_t new_length;
	size_t line_length;
	int written;
	int matches;
	int error;

	/* The home, under which the folders and the file are. */
	error = settings_home(home, sizeof(home));
	if (error != 0)
		return error;

	/* The path of ~/.config, which must fit. */
	written = snprintf(folder, sizeof(folder), "%s/%s", home, SETTINGS_CONFIG);
	if (written < 0 || (size_t)written >= sizeof(folder))
		return ENAMETOOLONG;

	/* Makes ~/.config when it is not there. */
	error = mkdir(folder, 0700);
	if (error != 0 && errno != EEXIST)
		return errno;

	/* The path of ~/.config/keiland, which must fit. */
	written = snprintf(folder, sizeof(folder), "%s/%s", home, SETTINGS_FOLDER);
	if (written < 0 || (size_t)written >= sizeof(folder))
		return ENAMETOOLONG;

	/* Makes ~/.config/keiland when it is not there. */
	error = mkdir(folder, 0755);
	if (error != 0 && errno != EEXIST)
		return errno;

	/* The file's path. */
	error = settings_path(path, sizeof(path), "");
	if (error != 0)
		return error;

	/* The file as it is now; a missing one has no other lines to keep. */
	error = settings_read(path, old_text, sizeof(old_text), &old_length);
	if (error == ENOENT)
		old_length = 0U;
	else if (error != 0)
		return error;

	/* Every line but the old ambiguous-wide one, as it was. */
	new_length = 0U;
	line = old_text;
	while (line < old_text + old_length) {
		/* The line runs to its newline or to the end of the file. */
		end = memchr(line, '\n', (size_t)(old_text + old_length - line));
		if (end == NULL)
			end = old_text + old_length;
		line_length = (size_t)(end - line);

		/* Another key's line, a comment or an empty line is kept with its newline. */
		matches = settings_line_key(line, line_length, SETTINGS_AMBIGUOUS_WIDE);
		if (!matches) {
			/* A file that would not fit is refused rather than cut short. */
			if (new_length + line_length + 1U > sizeof(new_text))
				return E2BIG;

			/* The line and its newline, after the lines kept before it. */
			memcpy(new_text + new_length, line, line_length);
			new_length += line_length;
			new_text[new_length] = '\n';
			new_length++;
		}

		/* The next line starts after the newline. */
		line = end + 1;
	}

	/* The setting's line, at the end. */
	written = snprintf(new_text + new_length, sizeof(new_text) - new_length, "%s=%d\n", SETTINGS_AMBIGUOUS_WIDE, settings->ambiguous_wide);
	if (written < 0 || (size_t)written >= sizeof(new_text) - new_length)
		return E2BIG;
	new_length += (size_t)written;

	/* The new file in place of the old one. */
	error = settings_write(path, new_text, new_length);
	if (error != 0)
		return error;

	/* Succeeded: the next terminal starts with these settings. */
	return 0;
}

/* Finds the home: $HOME, else the user's home in the password file. */
static int
settings_home(
	char *home,
	size_t size)
{
	const char *found;
	struct passwd *entry;
	uid_t user;
	int written;

	/* The environment's home first. */
	found = getenv("HOME");

	/* The password file's when the environment has none. */
	if (found == NULL || found[0] == '\0') {
		/* The user this terminal runs as, and that user's entry. */
		user = getuid();
		entry = getpwuid(user);
		if (entry == NULL ||
		    entry->pw_dir == NULL ||
		    entry->pw_dir[0] == '\0')
			return ENOENT;
		found = entry->pw_dir;
	}

	/* A home too long for a path is refused. */
	written = snprintf(home, size, "%s", found);
	if (written < 0 || (size_t)written >= size)
		return ENAMETOOLONG;

	/* Succeeded: the home. */
	return 0;
}

/* Makes the settings file's path, with a suffix (empty, or one for the new file beside it). */
static int
settings_path(
	char *path,
	size_t size,
	const char *suffix)
{
	char home[SETTINGS_PATH_MAX];
	int written;
	int error;

	/* The home the file is under. */
	error = settings_home(home, sizeof(home));
	if (error != 0)
		return error;

	/* The path, which must fit. */
	written = snprintf(path, size, "%s/%s/%s%s", home, SETTINGS_FOLDER, SETTINGS_NAME, suffix);
	if (written < 0 || (size_t)written >= size)
		return ENAMETOOLONG;

	/* Succeeded: the path. */
	return 0;
}

/* Reads a whole small file; returns 0, ENOENT, E2BIG or another errno value. */
static int
settings_read(
	const char *path,
	char *text,
	size_t capacity,
	size_t *length)
{
	ssize_t count;
	size_t done;
	int descriptor;
	int error;

	/* Nothing read yet. */
	*length = 0U;

	/* Opens the file. */
	descriptor = open(path, O_RDONLY | O_CLOEXEC);
	if (descriptor < 0)
		return errno;

	/* Reads to the end; a file that fills the buffer is too large to be the terminal's. */
	done = 0U;
	for (;;) {
		/* The next part of the file, into the rest of the buffer. */
		count = read(descriptor, text + done, capacity - done);
		if (count < 0) {
			error = errno;
			close(descriptor);
			return error;
		}

		/* The end of the file. */
		if (count == 0)
			break;

		/* What was read counts towards the whole. */
		done += (size_t)count;
		if (done == capacity) {
			close(descriptor);
			return E2BIG;
		}
	}

	/* Closes the file, which was only read. */
	close(descriptor);

	/* Succeeded: the file's bytes. */
	*length = done;
	return 0;
}

/* Tells whether a line (without its newline) sets a key: it starts with the key and an equals sign. */
static int
settings_line_key(
	const char *line,
	size_t length,
	const char *key)
{
	size_t key_length;
	int differs;

	/* The line must be long enough for the key and the equals sign. */
	key_length = strlen(key);
	if (length < key_length + 1U)
		return 0;

	/* It starts with the key. */
	differs = memcmp(line, key, key_length);
	if (differs != 0)
		return 0;

	/* And the key ends at the equals sign (not a longer key with the same start). */
	if (line[key_length] != '=')
		return 0;

	/* The line sets the key. */
	return 1;
}

/* Writes a file's new text into a new file beside it, flushes it and renames it over the old one. */
static int
settings_write(
	const char *path,
	const char *text,
	size_t length)
{
	char temporary[SETTINGS_PATH_MAX];
	ssize_t count;
	size_t done;
	int descriptor;
	int error;

	/* The new file's path, beside the old one. */
	error = settings_path(temporary, sizeof(temporary), ".new");
	if (error != 0)
		return error;

	/* The new file, empty. */
	descriptor = open(temporary, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
	if (descriptor < 0)
		return errno;

	/* Every byte of the text. */
	done = 0U;
	while (done < length) {
		/* The rest of the text, as much as the file takes at once. */
		count = write(descriptor, text + done, length - done);
		if (count < 0) {
			error = errno;
			close(descriptor);
			(void)unlink(temporary);
			return error;
		}

		/* What was written counts towards the whole. */
		done += (size_t)count;
	}

	/* On the disk before it replaces the old file. */
	error = fsync(descriptor);
	if (error != 0) {
		error = errno;
		close(descriptor);
		(void)unlink(temporary);
		return error;
	}

	/* Closes it; a failed close is a failed write. */
	error = close(descriptor);
	if (error != 0) {
		error = errno;
		(void)unlink(temporary);
		return error;
	}

	/* The new file takes the old one's name in one step. */
	error = rename(temporary, path);
	if (error != 0) {
		error = errno;
		(void)unlink(temporary);
		return error;
	}

	/* Succeeded: the file has the new text. */
	return 0;
}
