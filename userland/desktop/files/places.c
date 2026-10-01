/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The places of the sidebar and the names of places.
 *
 * Favorites are the home dashboard and the usual folders under the home
 * folder; Locations are the recent files, the trash and the computer's
 * root; Tags are the tags' colored dots.  A favorite whose folder does not
 * exist is kept (pale), so the sidebar keeps its shape on a new account.
 */

#include "files.h"
#include "mounts.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/*
 * One of the usual folders under the home folder.
 */
struct places_folder {
	const char *label;
	const char *name;
	unsigned icon;
};

/* The usual folders, in the sidebar's order. */
static const struct places_folder places_folders[] = {
	{ "Desktop", "Desktop", FM_ICON_DESKTOP },
	{ "Documents", "Documents", FM_ICON_DOCUMENTS },
	{ "Downloads", "Downloads", FM_ICON_DOWNLOADS },
	{ "Pictures", "Pictures", FM_ICON_PICTURES },
	{ "Music", "Music", FM_ICON_MUSIC },
	{ "Movies", "Movies", FM_ICON_MOVIES }
};

/* The file systems whose mounts are not shown as places (virtual ones). */
static const char *const places_hidden_types[] = {
	"tmpfs", "devfs", "proc", "procfs", "sysfs", "devpts", "kernfs", "fdesc", "swap", "bind",
	"cgroup", "cgroup2", "efivarfs", "securityfs", "pstore", "bpf", "tracefs", "debugfs", "mqueue",
	"hugetlbfs", "fusectl", "configfs", "autofs", "binfmt_misc", "nsfs", "rpc_pipefs", "overlay", "squashfs"
};

/* The folders whose mounts belong to the system rather than to the user. */
static const char *const places_system_folders[] = {
	"/sys", "/proc", "/dev", "/run", "/boot", "/snap", "/var/lib"
};

static struct fm_place *places_add(struct fm_places *places, unsigned section, unsigned icon, const char *label, unsigned kind, const char *path);
static void places_favorites(struct fm_places *places, const char *home);
static void places_add_folder(struct fm_places *places, const char *path, const char *home);
static void places_mounts(struct fm_places *places);
static void places_file(char *path, size_t size);

/*
 * Fills the sidebar: Favorites (the home dashboard and the user's folders,
 * or the usual ones), Locations (recent files, the trash, the computer and
 * mounted volumes) and the tags.
 */
void
fm_places_init(
	struct fm_places *places,
	const char *home,
	const struct fm_tags *tags)
{
	struct fm_place *place;
	int index;

	/* The sidebar starts empty. */
	memset(places, 0, sizeof(*places));

	/* Favorites: the home dashboard first, then the folders. */
	(void)places_add(places, FM_SECTION_FAVORITES, FM_ICON_HOME, "Home", FM_LOCATION_HOME, home);
	places_favorites(places, home);

	/* Locations: the recent files, the trash, the computer's root and the volumes. */
	(void)places_add(places, FM_SECTION_LOCATIONS, FM_ICON_RECENTS, "Recents", FM_LOCATION_RECENTS, "");
	(void)places_add(places, FM_SECTION_LOCATIONS, FM_ICON_TRASH, "Trash", FM_LOCATION_TRASH, "");
	(void)places_add(places, FM_SECTION_LOCATIONS, FM_ICON_COMPUTER, "Computer", FM_LOCATION_FOLDER, "/");
	places_mounts(places);

	/* Tags, each with its color. */
	for (index = 0; index < tags->count; index++) {
		place = places_add(places, FM_SECTION_TAGS, 0, tags->items[index].name, FM_LOCATION_TAG, tags->items[index].name);
		if (place == NULL)
			break;
		place->color = tags->items[index].color;
	}
}

/*
 * Adds a folder to the Favorites (after the others) and keeps the list;
 * a folder already there is not added again.  Returns 0, EEXIST or an
 * errno value.
 */
int
fm_places_add_favorite(
	struct fm_places *places,
	const char *path)
{
	char file[FM_PATH_MAX];
	FILE *out;
	int index;
	int match;

	/* Already a favorite. */
	for (index = 0; index < places->count; index++) {
		if (places->items[index].section != FM_SECTION_FAVORITES || places->items[index].location.kind != FM_LOCATION_FOLDER)
			continue;
		match = strcmp(places->items[index].location.path, path);
		if (match == 0)
			return EEXIST;
	}

	/* The list written again with the folder at its end. */
	places_file(file, sizeof(file));
	out = fopen(file, "w");
	if (out == NULL)
		return errno;
	for (index = 0; index < places->count; index++) {
		if (places->items[index].section == FM_SECTION_FAVORITES && places->items[index].location.kind == FM_LOCATION_FOLDER)
			fprintf(out, "%s\n", places->items[index].location.path);
	}

	/* The new folder last. */
	fprintf(out, "%s\n", path);
	fclose(out);

	/* Succeeded: the caller fills the sidebar again. */
	return 0;
}

/*
 * Takes a folder off the Favorites (by its place's index) and keeps the
 * list.  Returns 0, EINVAL for a place that is not a favorite folder, or an
 * errno value.
 */
int
fm_places_remove_favorite(
	struct fm_places *places,
	int removed)
{
	char file[FM_PATH_MAX];
	FILE *out;
	int index;

	/* Only a favorite folder (not Home) goes. */
	if (removed < 0 || removed >= places->count)
		return EINVAL;
	if (places->items[removed].section != FM_SECTION_FAVORITES || places->items[removed].location.kind != FM_LOCATION_FOLDER)
		return EINVAL;

	/* The list written again without it. */
	places_file(file, sizeof(file));
	out = fopen(file, "w");
	if (out == NULL)
		return errno;
	for (index = 0; index < places->count; index++) {
		if (index == removed)
			continue;
		if (places->items[index].section == FM_SECTION_FAVORITES && places->items[index].location.kind == FM_LOCATION_FOLDER)
			fprintf(out, "%s\n", places->items[index].location.path);
	}

	/* The list is written. */
	fclose(out);

	/* Succeeded: the caller fills the sidebar again. */
	return 0;
}

/*
 * Moves a favorite folder (by its place's index) to where another one is
 * (before it when it moves up, after it when it moves down) and keeps the
 * list.  Returns 0, EINVAL for a place that is not a favorite folder, or an
 * errno value.
 */
int
fm_places_move_favorite(
	struct fm_places *places,
	int moved,
	int to)
{
	char file[FM_PATH_MAX];
	FILE *out;
	int index;
	int ends[2];
	int end;

	/* Both are favorite folders (not Home). */
	ends[0] = moved;
	ends[1] = to;
	for (end = 0; end < 2; end++) {
		if (ends[end] < 0 || ends[end] >= places->count)
			return EINVAL;
		if (places->items[ends[end]].section != FM_SECTION_FAVORITES || places->items[ends[end]].location.kind != FM_LOCATION_FOLDER)
			return EINVAL;
	}

	/* The list written again in the new order. */
	places_file(file, sizeof(file));
	out = fopen(file, "w");
	if (out == NULL)
		return errno;
	for (index = 0; index < places->count; index++) {
		if (index == moved)
			continue;
		if (places->items[index].section != FM_SECTION_FAVORITES || places->items[index].location.kind != FM_LOCATION_FOLDER)
			continue;

		/* Moving up: the moved folder comes before the one it was dropped on. */
		if (index == to && to < moved)
			fprintf(out, "%s\n", places->items[moved].location.path);
		fprintf(out, "%s\n", places->items[index].location.path);

		/* Moving down: after it. */
		if (index == to && to > moved)
			fprintf(out, "%s\n", places->items[moved].location.path);
	}

	/* The list is written. */
	fclose(out);

	/* Succeeded: the caller fills the sidebar again. */
	return 0;
}

/*
 * Reports the name a place is shown by: the dashboard is Home, the home
 * folder is Home, the root is Computer, and a folder is its last part.
 */
const char *
fm_location_name(
	const struct fm_location *location,
	const char *home)
{
	const char *slash;
	int match;

	/* Places that are not folders have names of their own. */
	switch (location->kind) {
	case FM_LOCATION_HOME:
		return "Home";
	case FM_LOCATION_RECENTS:
		return "Recents";
	case FM_LOCATION_TRASH:
		return "Trash";
	case FM_LOCATION_TAG:
		return location->path;
	case FM_LOCATION_SEARCH:
		return "Search";
	default:
		break;
	}

	/* The home folder and the root. */
	match = strcmp(location->path, home);
	if (match == 0)
		return "Home";
	match = strcmp(location->path, "/");
	if (match == 0)
		return "Computer";

	/* A folder is named by the last part of its path. */
	slash = strrchr(location->path, '/');
	if (slash == NULL || slash[1] == '\0')
		return location->path;

	/* Reports the part after the last slash. */
	return slash + 1;
}

/*
 * Writes the names of the tags of a mask (the sidebar's tags in order,
 * bit n for the n-th), separated by commas.
 */
void
fm_tags_text(
	struct fm_app *app,
	unsigned tags,
	char *text,
	size_t length)
{
	const struct fm_place *place;
	size_t used;
	int number;
	int index;

	/* Each tag of the sidebar whose bit is set. */
	text[0] = '\0';
	used = 0;
	number = 0;
	for (index = 0; index < app->places.count; index++) {
		place = &app->places.items[index];
		if (place->section != FM_SECTION_TAGS)
			continue;

		/* A tag in the mask is named, after a comma when others came before. */
		if ((tags & (1U << number)) != 0U && used + 1U < length) {
			if (used != 0U)
				used += (size_t)snprintf(text + used, length - used, ", ");
			if (used < length)
				used += (size_t)snprintf(text + used, length - used, "%s", place->label);
		}

		/* The next tag of the sidebar has the next bit. */
		number++;
	}
}

/* Adds a place to a section of the sidebar; NULL when the sidebar is full. */
static struct fm_place *
places_add(
	struct fm_places *places,
	unsigned section,
	unsigned icon,
	const char *label,
	unsigned kind,
	const char *path)
{
	struct fm_place *place;

	/* The sidebar holds a fixed number of places. */
	if (places->count == FM_PLACES)
		return NULL;

	/* The place, after the others. */
	place = &places->items[places->count];
	memset(place, 0, sizeof(*place));
	place->section = section;
	place->icon = icon;
	snprintf(place->label, sizeof(place->label), "%s", label);
	place->location.kind = kind;
	snprintf(place->location.path, sizeof(place->location.path), "%s", path);
	places->count++;

	/* Reports the new place. */
	return place;
}

/* Adds the favorite folders: the user's list, or the usual folders under the home folder. */
static void
places_favorites(
	struct fm_places *places,
	const char *home)
{
	char file[FM_PATH_MAX];
	char line[FM_PATH_MAX];
	char path[FM_PATH_MAX];
	char *newline;
	char *read;
	FILE *in;
	size_t index;

	/* The user's list, a path a line. */
	places_file(file, sizeof(file));
	in = fopen(file, "r");
	if (in != NULL) {
		for (;;) {
			read = fgets(line, sizeof(line), in);
			if (read == NULL)
				break;
			newline = strchr(line, '\n');
			if (newline != NULL)
				*newline = '\0';
			if (line[0] == '/')
				places_add_folder(places, line, home);
		}

		/* The list is read. */
		fclose(in);
		return;
	}

	/* Without one, the usual folders. */
	for (index = 0; index < sizeof(places_folders) / sizeof(places_folders[0]); index++) {
		snprintf(path, sizeof(path), "%s/%s", home, places_folders[index].name);
		places_add_folder(places, path, home);
	}
}

/* Adds a favorite folder: the usual ones keep their icons, others get a folder's; a missing one is pale. */
static void
places_add_folder(
	struct fm_places *places,
	const char *path,
	const char *home)
{
	struct fm_location location;
	struct fm_place *place;
	struct stat status;
	char usual[FM_PATH_MAX];
	unsigned icon;
	size_t index;
	int error;
	int match;

	/* The icon of a usual folder, else a folder's. */
	icon = FM_ICON_FOLDER_LINE;
	for (index = 0; index < sizeof(places_folders) / sizeof(places_folders[0]); index++) {
		snprintf(usual, sizeof(usual), "%s/%s", home, places_folders[index].name);
		match = strcmp(usual, path);
		if (match == 0)
			icon = places_folders[index].icon;
	}

	/* The place, named as the folder is. */
	location.kind = FM_LOCATION_FOLDER;
	snprintf(location.path, sizeof(location.path), "%s", path);
	place = places_add(places, FM_SECTION_FAVORITES, icon, fm_location_name(&location, home), FM_LOCATION_FOLDER, path);
	if (place == NULL)
		return;

	/* A missing folder is shown pale. */
	error = stat(path, &status);
	if (error != 0)
		place->missing = 1;
}

/* Adds the mounted volumes other than the root and the virtual file systems. */
static void
places_mounts(
	struct fm_places *places)
{
	struct fm_mount mount;
	struct fm_location location;
	struct fm_mounts *table;
	const char *label;
	size_t index;
	size_t prefix;
	int hidden;
	int match;
	int error;
	int available;

	/* Acquires the selected OS's real mount enumeration without exposing its native storage. */
	table = NULL;
	error = fm_mounts_open(&table);
	if (error != 0)
		return;

	/* Each real mount retains the existing root, virtual and system-folder filtering. */
	for (;;) {
		available = fm_mounts_next(table, &mount);
		if (available <= 0)
			break;

		/* The computer's root already has its own common Places entry. */
		match = strcmp(mount.path, "/");
		if (match == 0)
			continue;

		/* Virtual filesystems remain absent from the user's mounted-volume sidebar. */
		hidden = 0;
		for (index = 0; index < sizeof(places_hidden_types) / sizeof(places_hidden_types[0]); index++) {
			match = strcmp(mount.type, places_hidden_types[index]);
			if (match == 0)
				hidden = 1;
		}

		/* Preserves the existing system-directory prefix policy for mount locations. */
		for (index = 0; index < sizeof(places_system_folders) / sizeof(places_system_folders[0]); index++) {
			prefix = strlen(places_system_folders[index]);
			match = strncmp(mount.path, places_system_folders[index], prefix);
			if (match == 0)
				hidden = 1;
		}

		/* Neither a virtual filesystem nor a system folder becomes a user volume. */
		if (hidden != 0)
			continue;

		/* The volume's label retains the mount point's existing common location-name policy. */
		location.kind = FM_LOCATION_FOLDER;
		snprintf(location.path, sizeof(location.path), "%s", mount.path);
		label = fm_location_name(&location, "");
		(void)places_add(places, FM_SECTION_LOCATIONS, FM_ICON_VOLUME, label, FM_LOCATION_FOLDER, mount.path);
	}

	/* Releases only this enumeration's stream or native snapshot. */
	fm_mounts_close(table);

	/* Succeeded: common Places policy consumed the selected OS's actual mount records. */
	return;
}

/* Writes the path of the Favorites' list ($XDG_CONFIG_HOME/files/sidebar), making its folder. */
static void
places_file(
	char *path,
	size_t size)
{
	char folder[FM_PATH_MAX - 16];
	const char *config;
	const char *home;

	/* $XDG_CONFIG_HOME, or ~/.config. */
	config = getenv("XDG_CONFIG_HOME");
	home = getenv("HOME");
	if (home == NULL)
		home = "";
	if (config != NULL && config[0] == '/') {
		snprintf(folder, sizeof(folder), "%s/files", config);
	} else {
		snprintf(folder, sizeof(folder), "%s/.config", home);
		(void)mkdir(folder, 0700);
		snprintf(folder, sizeof(folder), "%s/.config/files", home);
	}

	/* The folder, and the list in it. */
	(void)mkdir(folder, 0700);
	snprintf(path, size, "%s/sidebar", folder);
}
