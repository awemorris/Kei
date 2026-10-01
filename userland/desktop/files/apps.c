/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The applications that open files: which ones fit a file, and starting
 * one on it (spec §14).
 *
 * The ways to open a file come from three lists, the first match of the
 * earliest list being the default: the user's
 * $XDG_CONFIG_HOME/keiland/open-with, the system's /etc/keiland/open-with,
 * and the built-in table below.  A list's line is
 *
 *	PATTERNS<TAB>NAME<TAB>COMMAND
 *
 * where PATTERNS are MIME-type globs separated by commas (a star stands for
 * any characters, as in the shell), NAME is what the window shows, and
 * COMMAND is run by the shell with %f standing for the file's path, quoted
 * for the shell (the path is added at the end when there is no %f).  A command that starts
 * with "@terminal " runs the rest in a new terminal window; "@quicklook"
 * shows the file in Quick Look.  Lines starting with '#' are comments.
 *
 * The lists are read each time a file is opened, so an edit takes effect
 * at once, and nothing of them is kept between openings.
 *
 * Always Open With (ws093-p003) writes the user's list: a line for one
 * type, after a "# set by Files" comment, at the top of the list, so that
 * it is the type's default.  Files changes only the lines it wrote that
 * way (one a type); the lines the user wrote stay as they are.  The list
 * is written to a new file beside it and renamed over it, so that a
 * failure part-way leaves the old list whole.
 */

#include "files.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Marks a parameter a function's signature requires but it does not use. */
#define UNUSED_PARAMETER(name)	((void)(name))

/* The system's list, and the user's under the configuration folder. */
#define APPS_SYSTEM_LIST	KEILAND_SYSCONFDIR "/keiland/open-with"
#define APPS_USER_LIST		"keiland/open-with"

/* The terminal a "@terminal" command runs in, and the words that mark the two special commands. */
#define APPS_TERMINAL		KEILAND_BINDIR "/terminal"
#define APPS_TERMINAL_WORD	"@terminal "
#define APPS_QUICKLOOK_WORD	"@quicklook"

/* The longest line of a list, and the longest command once the path is put in. */
#define APPS_LINE		1024
#define APPS_EXPANDED		(FM_OPENER_COMMAND + 4 * FM_PATH_MAX)

/* The comment Files puts before a line it wrote, and the suffix of the new list it writes before the rename. */
#define APPS_MARK		"# set by Files"
#define APPS_NEW_SUFFIX		".new"

/* How many descriptors a started program closes before it runs (all the window may have open). */
#define APPS_DESCRIPTORS	1024

/* The picture types Image Viewer reads (it tells them by their first bytes, and reads no others). */
#define APPS_IMAGE_TYPES	"image/png,image/jpeg,image/gif"

/* The types that read as text for the viewers, beyond text/ itself. */
#define APPS_TEXT_TYPES		"text/" "*,application/json,application/xml,application/x-shellscript,application/javascript"

/*
 * One built-in way to open files: the types it fits, its name, its command,
 * and the program it needs (NULL when it needs none), without which it is
 * not offered.
 */
struct apps_builtin {
	const char *patterns;
	const char *name;
	const char *command;
	const char *needs;
};

/*
 * The built-in ways, after the lists of the user and the system: the
 * system's defaults.  Each application opens the kinds it reads when it is
 * installed (ws093-p002): a PDF in PDF Viewer (ws079-p006), a PNG, JPEG or
 * GIF picture in Image Viewer (which reads no other kind; the others stay
 * with Quick Look), an HTML page in the Browser, and text in Text Editor.
 * Anything the others do not fit is shown by less in a terminal.
 */
static const struct apps_builtin apps_builtins[] = {
	{ "application/pdf", "PDF Viewer", KEILAND_BINDIR "/pdfviewer %f", "pdfviewer" },
	{ APPS_IMAGE_TYPES, "Image Viewer", KEILAND_BINDIR "/imageview %f", "imageview" },
	{ "image/" "*", "Quick Look", "@quicklook", NULL },
	{ "text/html", "Browser", KEILAND_BINDIR "/browser %f", "browser" },
	{ APPS_TEXT_TYPES, "Text Editor", KEILAND_BINDIR "/textedit %f", "textedit" },
	{ APPS_TEXT_TYPES, "Terminal (less)", "@terminal less %f", NULL },
	{ APPS_TEXT_TYPES, "Remacs", "@terminal remacs %f", "remacs" },
	{ APPS_TEXT_TYPES, "Terminal (ed)", "@terminal ed %f", "ed" },
	{ "*", "Terminal (less)", "@terminal less %f", NULL }
};

/* The folders a needed program is looked for in. */
static const char *const apps_program_folders[] = {KEILAND_BINDIR, "/bin", "/usr/bin", "/usr/local/bin"};

static int apps_user_list(char *list, size_t size);
static int apps_rewrite(const char *type, const struct fm_opener *opener);
static int apps_copy_list(FILE *old, FILE *new, const char *type);
static int apps_is_mark(const char *line);
static int apps_is_type_line(const char *line, const char *type);
static int apps_make_folders(const char *list);
static void apps_read_list(const char *path, const char *type, struct fm_opener *openers, int capacity, int *count);
static int apps_parse_line(char *line, char **patterns, char **name, char **command);
static int apps_matches(const char *patterns, const char *type);
static void apps_add(struct fm_opener *openers, int capacity, int *count, const char *name, const char *command);
static int apps_program_exists(const char *program);
static int apps_expand(const char *command, const char *path, char *expanded, size_t size);
static int apps_quote(const char *path, char *quoted, size_t size);
static void apps_run(const char *command);
static void apps_exec(char *const arguments[]);

/*
 * Fills the ways a file can be opened, the default first, and reports how
 * many there are (at least one: the built-in viewer fits anything).
 *
 * A file whose mode lets it run is first offered to run in a terminal.
 */
int
fm_apps_for(
	const char *path,
	const struct fm_mime *mime,
	mode_t mode,
	struct fm_opener *openers,
	int capacity)
{
	char list[FM_PATH_MAX];
	size_t index;
	int regular;
	int matched;
	int count;
	int found;
	int error;

	UNUSED_PARAMETER(path);

	/* A program runs in a terminal first. */
	count = 0;
	regular = S_ISREG(mode);
	if (regular != 0 && (mode & 0111) != 0)
		apps_add(openers, capacity, &count, "Run in Terminal", "@terminal %f");

	/* The user's list, when there is a place for it. */
	error = apps_user_list(list, sizeof(list));
	if (error == 0)
		apps_read_list(list, mime->type, openers, capacity, &count);

	/* The system's list. */
	apps_read_list(APPS_SYSTEM_LIST, mime->type, openers, capacity, &count);

	/* The built-in ways that fit, and whose program is there. */
	for (index = 0; index < sizeof(apps_builtins) / sizeof(apps_builtins[0]); index++) {
		matched = apps_matches(apps_builtins[index].patterns, mime->type);
		if (matched == 0)
			continue;

		/* A way that needs a program the system does not have is left out. */
		if (apps_builtins[index].needs != NULL) {
			found = apps_program_exists(apps_builtins[index].needs);
			if (found == 0)
				continue;
		}

		/* It is offered. */
		apps_add(openers, capacity, &count, apps_builtins[index].name, apps_builtins[index].command);
	}

	/* Reports how many ways there are. */
	return count;
}

/*
 * Tells whether an opener is Quick Look, which the window shows itself
 * rather than starting a program.
 */
int
fm_apps_is_quicklook(
	const struct fm_opener *opener)
{
	int match;

	/* The command is the Quick Look word alone. */
	match = strcmp(opener->command, APPS_QUICKLOOK_WORD);
	if (match != 0)
		return 0;

	/* It is Quick Look. */
	return 1;
}

/*
 * Starts an opener's command on a file, apart from the file manager (in a
 * session of its own, so it outlives the window).
 *
 * Returns 0, ENAMETOOLONG when the command with the path is too long, or
 * the errno value of a failed fork.
 */
int
fm_apps_launch(
	const struct fm_opener *opener,
	const char *path)
{
	char expanded[APPS_EXPANDED];
	pid_t child;
	int status;
	int error;

	/* The command with the path put in. */
	error = apps_expand(opener->command, path, expanded, sizeof(expanded));
	if (error != 0)
		return error;

	/*
	 * A child that starts a grandchild and leaves at once: the grandchild
	 * runs the command, and, having no parent left, is not the file
	 * manager's to wait for.
	 */
	child = fork();
	if (child < 0)
		return errno;
	if (child == 0) {
		apps_run(expanded);
		_exit(0);
	}

	/* The child leaves at once; it is waited for so it does not linger. */
	(void)waitpid(child, &status, 0);

	/* The log line the tests wait for. */
	fm_log("LAUNCH name=%s command=%s", opener->name, expanded);

	/* Succeeded: the command is on its way. */
	return 0;
}

/*
 * Starts a program with its arguments apart from the file manager, as a
 * launch does, but keeping the standard input, output and error (a new
 * window of this program writes its log where this one does).
 *
 * Returns 0, or the errno value of a failed fork.
 */
int
fm_apps_spawn(
	char *const arguments[])
{
	pid_t child;
	int status;

	/* A child that starts a grandchild and leaves, as for a launch. */
	child = fork();
	if (child < 0)
		return errno;
	if (child == 0) {
		apps_exec(arguments);
		_exit(0);
	}

	/* The child leaves at once; it is waited for so it does not linger. */
	(void)waitpid(child, &status, 0);
	fm_log("SPAWN program=%s", arguments[0]);

	/* Succeeded: the program is on its way. */
	return 0;
}

/*
 * Makes a way the default for one type: the user's list gets the way's line
 * for exactly that type at its top, in place of the one Files wrote before.
 *
 * Returns 0, or an errno value (ENOENT without a home or configuration
 * folder) with the list as it was.
 */
int
fm_apps_set_default(
	const char *type,
	const struct fm_opener *opener)
{
	int error;

	/* The list rewritten with the way's line first. */
	error = apps_rewrite(type, opener);
	fm_log("DEFAULT set type=%s app=%s error=%d", type, opener->name, error);

	/* Reports why the default could not be changed. */
	if (error != 0)
		return error;

	/* Succeeded: the way opens the type from now on. */
	return 0;
}

/*
 * Gives one type back to the system's default: the line Files wrote for it
 * leaves the user's list (the user's own lines stay).
 *
 * Returns 0, or an errno value with the list as it was.
 */
int
fm_apps_clear_default(
	const char *type)
{
	int error;

	/* The list rewritten without Files' line for the type. */
	error = apps_rewrite(type, NULL);
	fm_log("DEFAULT clear type=%s error=%d", type, error);

	/* Reports why the default could not be given back. */
	if (error != 0)
		return error;

	/* Succeeded: the type opens with the system's default again. */
	return 0;
}

/*
 * Tells whether the user's list has a line Files wrote for a type (the
 * type has a default the user chose): 1 when it has, 0 when not.
 */
int
fm_apps_has_default(
	const char *type)
{
	char list[FM_PATH_MAX];
	char line[APPS_LINE];
	char *read;
	FILE *file;
	int after_mark;
	int is_mark;
	int is_type;
	int error;

	/* The user's list; without one there is no choice of the user's. */
	error = apps_user_list(list, sizeof(list));
	if (error != 0)
		return 0;

	/* The list, when there is one. */
	file = fopen(list, "r");
	if (file == NULL)
		return 0;

	/* Looks for Files' comment followed by the type's line. */
	after_mark = 0;
	for (;;) {
		/* The next line, until the list ends. */
		read = fgets(line, sizeof(line), file);
		if (read == NULL)
			break;

		/* A line of the type right after Files' comment is the user's choice. */
		if (after_mark) {
			is_type = apps_is_type_line(line, type);
			if (is_type) {
				fclose(file);
				return 1;
			}
		}

		/* Whether this line is Files' comment, for the next one. */
		is_mark = apps_is_mark(line);
		after_mark = is_mark;
	}

	/* The list is done with. */
	fclose(file);

	/* No line of Files' for the type. */
	return 0;
}

/* Finds the user's list: under $XDG_CONFIG_HOME, or ~/.config; ENOENT when neither is set. */
static int
apps_user_list(
	char *list,
	size_t size)
{
	const char *config;
	const char *home;
	int written;

	/* $XDG_CONFIG_HOME, when it is set. */
	config = getenv("XDG_CONFIG_HOME");
	if (config != NULL && config[0] != '\0') {
		written = snprintf(list, size, "%s/%s", config, APPS_USER_LIST);
		if (written < 0 || (size_t)written >= size)
			return ENAMETOOLONG;

		/* Succeeded: the list under the configuration folder. */
		return 0;
	}

	/* Otherwise ~/.config; without a home there is no user's list. */
	home = getenv("HOME");
	if (home == NULL || home[0] == '\0')
		return ENOENT;

	/* The list under the home's configuration folder. */
	written = snprintf(list, size, "%s/.config/%s", home, APPS_USER_LIST);
	if (written < 0 || (size_t)written >= size)
		return ENAMETOOLONG;

	/* Succeeded: the list under ~/.config. */
	return 0;
}

/*
 * Writes the user's list anew: Files' line for the type first (none when
 * opener is NULL), then every line of the old list but the one Files wrote
 * for the type and its comment.  The new list is renamed over the old one.
 */
static int
apps_rewrite(
	const char *type,
	const struct fm_opener *opener)
{
	char list[FM_PATH_MAX];
	char fresh[FM_PATH_MAX + sizeof(APPS_NEW_SUFFIX)];
	FILE *old;
	FILE *new;
	int written;
	int error;

	/* The user's list, and the new list beside it. */
	error = apps_user_list(list, sizeof(list));
	if (error != 0)
		return error;

	/* The new list's name: the list's with a suffix (the buffer holds both). */
	snprintf(fresh, sizeof(fresh), "%s%s", list, APPS_NEW_SUFFIX);

	/* The folders the list lives in, made when they are not there. */
	error = apps_make_folders(list);
	if (error != 0)
		return error;

	/* The new list. */
	new = fopen(fresh, "w");
	if (new == NULL)
		return errno;

	/* The way's line first, after Files' comment. */
	if (opener != NULL) {
		written = fprintf(new, "%s\n%s\t%s\t%s\n", APPS_MARK, type, opener->name, opener->command);
		if (written < 0) {
			fclose(new);
			(void)unlink(fresh);
			return EIO;
		}
	}

	/* The old list's lines but Files' line for the type, when there is an old list. */
	old = fopen(list, "r");
	if (old != NULL) {
		error = apps_copy_list(old, new, type);
		fclose(old);
		if (error != 0) {
			fclose(new);
			(void)unlink(fresh);
			return error;
		}
	}

	/* The new list, complete on the disk. */
	error = fclose(new);
	if (error != 0) {
		(void)unlink(fresh);
		return EIO;
	}

	/* The new list takes the old one's place at once. */
	error = rename(fresh, list);
	if (error != 0) {
		error = errno;
		(void)unlink(fresh);
		return error;
	}

	/* Succeeded: the list is written. */
	return 0;
}

/* Copies a list's lines but Files' line for a type and the comment before it; returns 0 or EIO. */
static int
apps_copy_list(
	FILE *old,
	FILE *new,
	const char *type)
{
	char line[APPS_LINE];
	char *read;
	int held;
	int is_mark;
	int is_type;
	int written;

	/* Each line; Files' comment is held until the line after it says whether it goes. */
	held = 0;
	for (;;) {
		/* The next line, until the list ends. */
		read = fgets(line, sizeof(line), old);
		if (read == NULL)
			break;

		/* A held comment goes with the type's line after it, and the line goes too. */
		if (held) {
			held = 0;
			is_type = apps_is_type_line(line, type);
			if (is_type)
				continue;

			/* The comment belongs to another line, and stays. */
			written = fprintf(new, "%s\n", APPS_MARK);
			if (written < 0)
				return EIO;
		}

		/* Files' comment is held for the line after it. */
		is_mark = apps_is_mark(line);
		if (is_mark) {
			held = 1;
			continue;
		}

		/* Any other line stays as it was. */
		written = fputs(line, new);
		if (written < 0)
			return EIO;
	}

	/* A comment at the list's end stays. */
	if (held) {
		written = fprintf(new, "%s\n", APPS_MARK);
		if (written < 0)
			return EIO;
	}

	/* Succeeded: every line that stays is copied. */
	return 0;
}

/* Tells whether a line of a list (with its end) is the comment Files puts before its own lines. */
static int
apps_is_mark(
	const char *line)
{
	size_t length;
	int differs;

	/* The comment, followed by the line's end or nothing. */
	length = strlen(APPS_MARK);
	differs = strncmp(line, APPS_MARK, length);
	if (differs != 0)
		return 0;

	/* Nothing else may follow on the line. */
	if (line[length] == '\0')
		return 1;
	if (line[length] == '\n')
		return 1;
	if (line[length] == '\r')
		return 1;

	/* A longer comment is the user's. */
	return 0;
}

/* Tells whether a line of a list (with its end) is a way for exactly one type: its patterns are that type. */
static int
apps_is_type_line(
	const char *line,
	const char *type)
{
	size_t length;
	int differs;

	/* The patterns are the type, ended by the first tab. */
	length = strlen(type);
	differs = strncmp(line, type, length);
	if (differs != 0)
		return 0;

	/* The type alone, not the start of a longer pattern. */
	if (line[length] != '\t')
		return 0;

	/* It is the type's line. */
	return 1;
}

/* Makes the folders a list lives in (the configuration folder and its keiland folder) when they are not there. */
static int
apps_make_folders(
	const char *list)
{
	char folder[FM_PATH_MAX];
	char *slash;
	int error;

	/* The list's folder, and the configuration folder above it. */
	snprintf(folder, sizeof(folder), "%s", list);
	slash = strrchr(folder, '/');
	if (slash == NULL)
		return 0;
	*slash = '\0';

	/* The configuration folder first (~/.config may not be there yet). */
	slash = strrchr(folder, '/');
	if (slash != NULL && slash != folder) {
		*slash = '\0';
		error = mkdir(folder, 0755);
		if (error != 0 && errno != EEXIST)
			return errno;

		/* The path back to the list's folder. */
		*slash = '/';
	}

	/* Then the keiland folder in it. */
	error = mkdir(folder, 0755);
	if (error != 0 && errno != EEXIST)
		return errno;

	/* Succeeded: the list's folder is there. */
	return 0;
}

/* Adds the openers of a list that fit a type; a list that cannot be read adds none. */
static void
apps_read_list(
	const char *path,
	const char *type,
	struct fm_opener *openers,
	int capacity,
	int *count)
{
	char line[APPS_LINE];
	char *patterns;
	char *name;
	char *command;
	char *read;
	FILE *file;
	int parsed;
	int matched;

	/* The list. */
	file = fopen(path, "r");
	if (file == NULL)
		return;

	/* Each line that names a way to open the type. */
	for (;;) {
		read = fgets(line, sizeof(line), file);
		if (read == NULL)
			break;

		/* A comment, a blank or a malformed line says nothing. */
		parsed = apps_parse_line(line, &patterns, &name, &command);
		if (parsed == 0)
			continue;

		/* A line for other types says nothing either. */
		matched = apps_matches(patterns, type);
		if (matched == 0)
			continue;

		/* The way is offered. */
		apps_add(openers, capacity, count, name, command);
	}

	/* The list is not needed any more. */
	fclose(file);
}

/* Splits a list's line at its tabs into its patterns, name and command; zero for a line that is not one. */
static int
apps_parse_line(
	char *line,
	char **patterns,
	char **name,
	char **command)
{
	char *tab;
	size_t length;

	/* The line without its end. */
	length = strlen(line);
	while (length > 0U && (line[length - 1U] == '\n' || line[length - 1U] == '\r')) {
		line[length - 1U] = '\0';
		length--;
	}

	/* A blank line and a comment are not ways. */
	if (line[0] == '\0' || line[0] == '#')
		return 0;

	/* The patterns end at the first tab. */
	*patterns = line;
	tab = strchr(line, '\t');
	if (tab == NULL)
		return 0;
	*tab = '\0';

	/* The name ends at the second. */
	*name = tab + 1;
	tab = strchr(*name, '\t');
	if (tab == NULL)
		return 0;
	*tab = '\0';

	/* The command is the rest, and none of the three may be empty. */
	*command = tab + 1;
	if ((*patterns)[0] == '\0' || (*name)[0] == '\0' || (*command)[0] == '\0')
		return 0;

	/* Succeeded: the line is a way to open files. */
	return 1;
}

/* Tells whether a type fits one of the comma-separated globs of a list. */
static int
apps_matches(
	const char *patterns,
	const char *type)
{
	char pattern[128];
	const char *start;
	const char *end;
	size_t length;
	int miss;

	/* Each glob between the commas, spaces around it left out. */
	start = patterns;
	while (*start != '\0') {
		while (*start == ' ' || *start == ',')
			start++;
		if (*start == '\0')
			break;
		end = strchr(start, ',');
		if (end == NULL)
			end = start + strlen(start);

		/* The glob alone, without the spaces after it. */
		length = (size_t)(end - start);
		while (length > 0U && start[length - 1U] == ' ')
			length--;
		if (length >= sizeof(pattern))
			length = sizeof(pattern) - 1U;
		memcpy(pattern, start, length);
		pattern[length] = '\0';

		/* The type fits this glob. */
		miss = fnmatch(pattern, type, 0);
		if (miss == 0)
			return 1;

		/* The next glob. */
		start = end;
	}

	/* No glob fits. */
	return 0;
}

/* Adds a way to open a file unless one of the same name is there already or the list is full. */
static void
apps_add(
	struct fm_opener *openers,
	int capacity,
	int *count,
	const char *name,
	const char *command)
{
	int index;
	int match;

	/* No room. */
	if (*count >= capacity)
		return;

	/* A name offered already (by an earlier list) is not offered twice. */
	for (index = 0; index < *count; index++) {
		match = strcmp(openers[index].name, name);
		if (match == 0)
			return;
	}

	/* The way, at the end. */
	snprintf(openers[*count].name, sizeof(openers[*count].name), "%s", name);
	snprintf(openers[*count].command, sizeof(openers[*count].command), "%s", command);
	(*count)++;
}

/* Tells whether a program of a name is in one of the folders programs are looked for in. */
static int
apps_program_exists(
	const char *program)
{
	char path[FM_PATH_MAX];
	size_t index;
	int error;

	/* Each folder, for a file that may run. */
	for (index = 0; index < sizeof(apps_program_folders) / sizeof(apps_program_folders[0]); index++) {
		snprintf(path, sizeof(path), "%s/%s", apps_program_folders[index], program);
		error = access(path, X_OK);
		if (error == 0)
			return 1;
	}

	/* No folder has it. */
	return 0;
}

/*
 * Writes a command with each %f replaced by the path quoted for the shell
 * (the path added at the end when there is none); returns 0 or
 * ENAMETOOLONG.
 */
static int
apps_expand(
	const char *command,
	const char *path,
	char *expanded,
	size_t size)
{
	char quoted[4 * FM_PATH_MAX];
	size_t length;
	size_t done;
	int placed;
	int error;

	/* The path, quoted once for all the places it goes. */
	error = apps_quote(path, quoted, sizeof(quoted));
	if (error != 0)
		return error;
	length = strlen(quoted);

	/* The command, character by character, the path where %f is. */
	done = 0;
	placed = 0;
	while (*command != '\0') {
		if (command[0] == '%' && command[1] == 'f') {
			if (done + length >= size)
				return ENAMETOOLONG;
			memcpy(expanded + done, quoted, length);
			done += length;
			command += 2;
			placed = 1;
			continue;
		}

		/* Any other character is kept. */
		if (done + 1U >= size)
			return ENAMETOOLONG;
		expanded[done] = *command;
		done++;
		command++;
	}

	/* A command without %f takes the path at its end. */
	if (placed == 0) {
		if (done + 1U + length >= size)
			return ENAMETOOLONG;
		expanded[done] = ' ';
		memcpy(expanded + done + 1U, quoted, length);
		done += 1U + length;
	}

	/* Succeeded: the command ends there. */
	expanded[done] = '\0';
	return 0;
}

/* Quotes a path for the shell: in single quotes, each single quote written as '\''; returns 0 or ENAMETOOLONG. */
static int
apps_quote(
	const char *path,
	char *quoted,
	size_t size)
{
	size_t done;

	/* The opening quote. */
	if (size < 3U)
		return ENAMETOOLONG;
	quoted[0] = '\'';
	done = 1;

	/* Each character; a single quote closes the quote, is escaped, and opens it again. */
	while (*path != '\0') {
		if (done + 5U >= size)
			return ENAMETOOLONG;
		if (*path == '\'') {
			memcpy(quoted + done, "'\\''", 4);
			done += 4;
		} else {
			quoted[done] = *path;
			done++;
		}

		/* The next character. */
		path++;
	}

	/* The closing quote. */
	quoted[done] = '\'';
	quoted[done + 1U] = '\0';

	/* Succeeded: the path is quoted. */
	return 0;
}

/*
 * Runs a command from the child that fork made: a grandchild in a session
 * of its own runs it, in a terminal or by the shell, with nothing of the
 * window's input or output.
 */
static void
apps_run(
	const char *command)
{
	char terminal_command[APPS_EXPANDED + 16];
	pid_t grandchild;
	int descriptor;
	int is_terminal;

	/* The grandchild; the child leaves as soon as it is made. */
	grandchild = fork();
	if (grandchild != 0)
		return;

	/* A session of its own, and no terminal of the window's. */
	(void)setsid();
	descriptor = open("/dev/null", O_RDWR);
	if (descriptor >= 0) {
		(void)dup2(descriptor, 0);
		(void)dup2(descriptor, 1);
		(void)dup2(descriptor, 2);
		if (descriptor > 2)
			close(descriptor);
	}

	/*
	 * None of the window's other descriptors: the program must not keep
	 * the file manager's connection to the compositor (or its GPU) open
	 * after the window closes.
	 */
	for (descriptor = 3; descriptor < APPS_DESCRIPTORS; descriptor++)
		(void)close(descriptor);

	/* A terminal command: the rest runs in a new terminal window. */
	is_terminal = strncmp(command, APPS_TERMINAL_WORD, strlen(APPS_TERMINAL_WORD));
	if (is_terminal == 0) {
		snprintf(terminal_command, sizeof(terminal_command), "--command=%s", command + strlen(APPS_TERMINAL_WORD));
		execl(APPS_TERMINAL, "terminal", terminal_command, (char *)NULL);
		_exit(127);
	}

	/* Anything else runs by the shell. */
	execl("/bin/sh", "sh", "-c", command, (char *)NULL);
	_exit(127);
}

/* Runs a program from the child that fork made: a grandchild in a session of its own, with only the standard descriptors. */
static void
apps_exec(
	char *const arguments[])
{
	pid_t grandchild;
	int descriptor;

	/* The grandchild; the child leaves as soon as it is made. */
	grandchild = fork();
	if (grandchild != 0)
		return;

	/* A session of its own, and none of the window's descriptors but the standard ones. */
	(void)setsid();
	for (descriptor = 3; descriptor < APPS_DESCRIPTORS; descriptor++)
		(void)close(descriptor);

	/* The program; only a failed exec comes back. */
	execv(arguments[0], arguments);
	_exit(127);
}
