/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The storage areas of Web Storage (ws074-p080): the window's host
 * callbacks behind sessionStorage and localStorage.
 *
 * An area belongs to an origin: an http or https page's origin, one for
 * every file: page, and one for the pages without either (about:blank),
 * which is never saved.  The areas live in the process, as the cookie jar
 * does (browser runs its pages on one thread); a session area lasts as
 * long as the process, and a local area is read from its file the first
 * time it is used and written back after each change.  The file is
 * $XDG_DATA_HOME/keiland/browser/local-storage/ORIGIN.ls (with
 * ~/.local/share when XDG_DATA_HOME is not set), ORIGIN the origin's bytes
 * in hexadecimal; a file that cannot be written leaves the area in
 * memory.  The items are kept in the order of their keys' code units,
 * which key(n) reports.  An origin's area holds at most
 * STORAGE_QUOTA_BYTES of keys and values counted as UTF-16.
 */

#include "page/page.h"
#include "net/net.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* The most an origin's area holds: its keys' and values' UTF-16, in bytes (5 MiB, as other browsers). */
#define STORAGE_QUOTA_BYTES	(5UL * 1024UL * 1024UL)

/* The first line of a local area's file. */
#define STORAGE_MAGIC		"KEILAND LOCAL STORAGE 1\n"

/* The directory under the data directory the local areas' files are in. */
#define STORAGE_DIRECTORY	"keiland/browser/local-storage"

/*
 * What a page's scheme means for its storage (storage_scheme_kind).
 */
enum storage_scheme {
	STORAGE_SCHEME_OTHER,	/* about:blank and the like: the empty origin, never saved */
	STORAGE_SCHEME_WEB,	/* http and https: the page's origin */
	STORAGE_SCHEME_FILE	/* file: one origin for every file */
};

/*
 * One item: its key and its value, UTF-16 in memory of its own.
 */
struct storage_item {
	uint16_t *key;
	size_t key_length;
	uint16_t *value;
	size_t value_length;
};

/*
 * One origin's storage area: the origin, which area it is
 * (BIND_STORAGE_*), whether it is saved to a file, its items in the order
 * of their keys, and the bytes they take.
 */
struct storage_area {
	char *origin;
	int area;
	int saved;
	struct wb_vector items;
	size_t bytes;
};

/*
 * The process's storage areas (pointers to struct storage_area), made as
 * pages ask for them and kept until the process ends.  storage_ready is 0
 * until the vector is initialized.
 */
static struct wb_vector storage_areas;

/* Whether storage_areas has been initialized. */
static int storage_ready;

static int storage_length(void *context, int area, size_t *count);
static int storage_key(void *context, int area, size_t index, struct wb_units *key, int *found);
static int storage_get(void *context, int area, const uint16_t *key, size_t key_length, struct wb_units *value, int *found);
static int storage_set(void *context, int area, const uint16_t *key, size_t key_length, const uint16_t *value, size_t value_length);
static int storage_remove(void *context, int area, const uint16_t *key, size_t key_length);
static int storage_clear(void *context, int area);
static int storage_area_of(struct page *page, int area, struct storage_area **found);
static int storage_origin(struct page *page, char **origin, int *saved);
static int storage_scheme_kind(const char *scheme);
static int storage_find(const struct storage_area *area, const uint16_t *key, size_t key_length, size_t *index);
static int storage_compare(const uint16_t *left, size_t left_length, const uint16_t *right, size_t right_length);
static int storage_insert(struct storage_area *area, size_t index, const uint16_t *key, size_t key_length, const uint16_t *value, size_t value_length);
static void storage_remove_at(struct storage_area *area, size_t index);
static uint16_t *storage_copy_units(const uint16_t *units, size_t length);
static int storage_path(const char *origin, struct wb_buffer *path);
static int storage_make_directories(const char *path);
static int storage_load(struct storage_area *area);
static int storage_save(const struct storage_area *area);
static int storage_read_length(const unsigned char *bytes, size_t length, size_t *at, uint32_t *value);
static int storage_append_length(struct wb_buffer *out, uint32_t value);
static int storage_append_units(struct wb_buffer *out, const uint16_t *units, size_t length);

/*
 * The callbacks of the window's host for Web Storage.  The table is
 * constant for the life of the program.
 */
static const struct bind_storage_calls storage_calls = {
	storage_length,
	storage_key,
	storage_get,
	storage_set,
	storage_remove,
	storage_clear
};

/*
 * Reports the window's host callbacks for Web Storage (the page is their
 * context).
 */
const struct bind_storage_calls *
page_storage_calls(void)
{
	/* The table. */
	return &storage_calls;
}

/* Reports how many items an area of the page's origin has. */
static int
storage_length(
	void *context,
	int area,
	size_t *count)
{
	struct storage_area *found;
	int error;

	/* The area. */
	error = storage_area_of(context, area, &found);
	if (error != 0)
		return error;

	/* Succeeded: its count. */
	*count = found->items.count;
	return 0;
}

/* Reports the key at a place of an area, in the order of the keys. */
static int
storage_key(
	void *context,
	int area,
	size_t index,
	struct wb_units *key,
	int *found)
{
	struct storage_area *storage;
	struct storage_item *item;
	int error;

	/* The area. */
	*found = 0;
	error = storage_area_of(context, area, &storage);
	if (error != 0)
		return error;

	/* A place past the last item has no key. */
	if (index >= storage->items.count)
		return 0;

	/* Succeeded: the key. */
	item = wb_vector_at(&storage->items, index);
	error = wb_units_append(key, item->key, item->key_length);
	if (error != 0)
		return error;
	*found = 1;
	return 0;
}

/* Reports a key's value in an area. */
static int
storage_get(
	void *context,
	int area,
	const uint16_t *key,
	size_t key_length,
	struct wb_units *value,
	int *found)
{
	struct storage_area *storage;
	struct storage_item *item;
	size_t index;
	int present;
	int error;

	/* The area. */
	*found = 0;
	error = storage_area_of(context, area, &storage);
	if (error != 0)
		return error;

	/* A key not there has no value. */
	present = storage_find(storage, key, key_length, &index);
	if (!present)
		return 0;

	/* Succeeded: the value. */
	item = wb_vector_at(&storage->items, index);
	error = wb_units_append(value, item->value, item->value_length);
	if (error != 0)
		return error;
	*found = 1;
	return 0;
}

/*
 * Sets a key's value in an area, within the origin's quota (ENOSPC past
 * it), and saves a local area.
 */
static int
storage_set(
	void *context,
	int area,
	const uint16_t *key,
	size_t key_length,
	const uint16_t *value,
	size_t value_length)
{
	struct storage_area *storage;
	struct storage_item *item;
	uint16_t *copy;
	size_t index;
	size_t bytes;
	int present;
	int error;

	/* The area and where the key is, or goes. */
	error = storage_area_of(context, area, &storage);
	if (error != 0)
		return error;
	present = storage_find(storage, key, key_length, &index);

	/* The bytes the area would take with the item. */
	bytes = storage->bytes + (key_length + value_length) * 2U;
	if (present) {
		item = wb_vector_at(&storage->items, index);
		bytes -= (item->key_length + item->value_length) * 2U;
	}

	/* An item that would take the area past its quota is refused. */
	if (bytes > STORAGE_QUOTA_BYTES)
		return ENOSPC;

	/* A new key is inserted in its place. */
	if (!present) {
		error = storage_insert(storage, index, key, key_length, value, value_length);
		if (error != 0)
			return error;
	} else {
		/* A key there takes the new value. */
		copy = storage_copy_units(value, value_length);
		if (copy == NULL)
			return ENOMEM;
		item = wb_vector_at(&storage->items, index);
		free(item->value);
		item->value = copy;
		item->value_length = value_length;
	}

	/* The area's bytes with the item. */
	storage->bytes = bytes;

	/* Succeeded: the item is set, and a local area saved. */
	if (storage->saved)
		(void)storage_save(storage);
	return 0;
}

/* Removes a key from an area, and saves a local area. */
static int
storage_remove(
	void *context,
	int area,
	const uint16_t *key,
	size_t key_length)
{
	struct storage_area *storage;
	size_t index;
	int present;
	int error;

	/* The area. */
	error = storage_area_of(context, area, &storage);
	if (error != 0)
		return error;

	/* A key not there changes nothing. */
	present = storage_find(storage, key, key_length, &index);
	if (!present)
		return 0;

	/* Succeeded: the item goes, and a local area is saved. */
	storage_remove_at(storage, index);
	if (storage->saved)
		(void)storage_save(storage);
	return 0;
}

/* Removes every item of an area, and saves a local area. */
static int
storage_clear(
	void *context,
	int area)
{
	struct storage_area *storage;
	int error;

	/* The area. */
	error = storage_area_of(context, area, &storage);
	if (error != 0)
		return error;

	/* An empty area stays as it is. */
	if (storage->items.count == 0)
		return 0;

	/* Each item, from the last. */
	while (storage->items.count > 0)
		storage_remove_at(storage, storage->items.count - 1U);

	/* Succeeded: the area is empty, and a local area saved. */
	if (storage->saved)
		(void)storage_save(storage);
	return 0;
}

/*
 * Finds the area of a kind for the page's origin, made (and a local
 * area read from its file) the first time.
 */
static int
storage_area_of(
	struct page *page,
	int area,
	struct storage_area **found)
{
	struct storage_area **areas;
	struct storage_area *made;
	char *origin;
	size_t index;
	int saved;
	int same;
	int error;

	/* The table, the first time. */
	if (!storage_ready) {
		wb_vector_init(&storage_areas, sizeof(struct storage_area *));
		storage_ready = 1;
	}

	/* The page's origin. */
	error = storage_origin(page, &origin, &saved);
	if (error != 0)
		return error;

	/* An area of the origin made before. */
	areas = storage_areas.items;
	for (index = 0; index < storage_areas.count; index++) {
		if (areas[index]->area != area)
			continue;
		same = strcmp(areas[index]->origin, origin);
		if (same == 0) {
			free(origin);
			*found = areas[index];
			return 0;
		}
	}

	/* A new one, which keeps the origin. */
	made = calloc(1, sizeof(*made));
	if (made == NULL) {
		free(origin);
		return ENOMEM;
	}

	/* Its origin and kind; only a local area of a web or file: page is saved. */
	made->origin = origin;
	made->area = area;
	made->saved = 0;
	if (area == BIND_STORAGE_LOCAL)
		made->saved = saved;
	wb_vector_init(&made->items, sizeof(struct storage_item));

	/* In the table. */
	error = wb_vector_push(&storage_areas, &made);
	if (error != 0) {
		free(made->origin);
		free(made);
		return ENOMEM;
	}

	/* A local area has what its file kept; a file that cannot be read is an empty area. */
	if (made->saved)
		(void)storage_load(made);

	/* Succeeded: the area. */
	*found = made;
	return 0;
}

/*
 * Finds the origin a page's storage belongs to (an allocated string):
 * an http or https page's origin, "file://" for a file: page, and the
 * empty string for any other; saved says whether its local area is kept
 * in a file.
 */
static int
storage_origin(
	struct page *page,
	char **origin,
	int *saved)
{
	struct net_url url;
	struct wb_buffer text;
	int kind;
	int error;

	/* No origin until one is found. */
	*origin = NULL;
	*saved = 0;
	wb_buffer_init(&text);

	/* The page's URL; a page without one is the empty origin. */
	error = page_url(page, &url);
	if (error == ENOMEM)
		return error;
	if (error == 0) {
		/* An http or https page has its origin, a file: page the one of every file. */
		kind = storage_scheme_kind(url.scheme);
		if (kind == STORAGE_SCHEME_WEB) {
			error = net_url_component(&url, NET_URL_ORIGIN, &text);
			*saved = 1;
		} else if (kind == STORAGE_SCHEME_FILE) {
			error = wb_buffer_append_string(&text, "file://");
			*saved = 1;
		}

		/* The URL is done with. */
		net_url_release(&url);
	}

	/* An origin that could not be written. */
	if (error != 0) {
		wb_buffer_release(&text);
		return error;
	}

	/* The origin's own copy. */
	*origin = strdup(wb_buffer_string(&text));
	wb_buffer_release(&text);
	if (*origin == NULL)
		return ENOMEM;

	/* Succeeded: the origin. */
	return 0;
}

/* Tells what a URL's scheme means for its page's storage (a storage_scheme). */
static int
storage_scheme_kind(
	const char *scheme)
{
	int differs;

	/* http and https. */
	differs = strcmp(scheme, "http");
	if (differs == 0)
		return STORAGE_SCHEME_WEB;
	differs = strcmp(scheme, "https");
	if (differs == 0)
		return STORAGE_SCHEME_WEB;

	/* file. */
	differs = strcmp(scheme, "file");
	if (differs == 0)
		return STORAGE_SCHEME_FILE;

	/* Any other. */
	return STORAGE_SCHEME_OTHER;
}

/*
 * Finds a key in an area's items (in the order of their keys): reports
 * whether it is there, and its place or the place it would go.
 */
static int
storage_find(
	const struct storage_area *area,
	const uint16_t *key,
	size_t key_length,
	size_t *index)
{
	const struct storage_item *items;
	size_t low;
	size_t high;
	size_t middle;
	int order;

	/* A binary search of the keys. */
	items = area->items.items;
	low = 0;
	high = area->items.count;
	while (low < high) {
		middle = low + (high - low) / 2U;
		order = storage_compare(items[middle].key, items[middle].key_length, key, key_length);
		if (order == 0) {
			*index = middle;
			return 1;
		}

		/* The half the key is in. */
		if (order < 0) {
			low = middle + 1U;
		} else {
			high = middle;
		}
	}

	/* Succeeded: it is not there, and would go at low. */
	*index = low;
	return 0;
}

/* Orders two keys by their code units, a shorter one first when it is the other's start. */
static int
storage_compare(
	const uint16_t *left,
	size_t left_length,
	const uint16_t *right,
	size_t right_length)
{
	size_t index;

	/* The first unit that differs. */
	for (index = 0; index < left_length && index < right_length; index++) {
		if (left[index] < right[index])
			return -1;
		if (left[index] > right[index])
			return 1;
	}

	/* The shorter one first. */
	if (left_length < right_length)
		return -1;
	if (left_length > right_length)
		return 1;

	/* Succeeded: they are the same. */
	return 0;
}

/* Inserts an item at a place of an area's items. */
static int
storage_insert(
	struct storage_area *area,
	size_t index,
	const uint16_t *key,
	size_t key_length,
	const uint16_t *value,
	size_t value_length)
{
	struct storage_item item;
	struct storage_item *items;
	int error;

	/* The item's own copies. */
	memset(&item, 0, sizeof(item));
	item.key = storage_copy_units(key, key_length);
	if (item.key == NULL)
		return ENOMEM;
	item.value = storage_copy_units(value, value_length);
	if (item.value == NULL) {
		free(item.key);
		return ENOMEM;
	}

	/* Their lengths. */
	item.key_length = key_length;
	item.value_length = value_length;

	/* A place at the end, then the later items moved up to make room at index. */
	error = wb_vector_push(&area->items, &item);
	if (error != 0) {
		free(item.key);
		free(item.value);
		return ENOMEM;
	}

	/* The item at index, the later ones one place up. */
	items = area->items.items;
	memmove(&items[index + 1U], &items[index], (area->items.count - 1U - index) * sizeof(*items));
	items[index] = item;

	/* Succeeded: the item is in its place. */
	return 0;
}

/* Removes the item at a place of an area's items, and the bytes it took. */
static void
storage_remove_at(
	struct storage_area *area,
	size_t index)
{
	struct storage_item *items;

	/* The item's memory and bytes. */
	items = area->items.items;
	area->bytes -= (items[index].key_length + items[index].value_length) * 2U;
	free(items[index].key);
	free(items[index].value);

	/* The later items move down. */
	memmove(&items[index], &items[index + 1U], (area->items.count - 1U - index) * sizeof(*items));
	area->items.count--;
}

/* Returns an allocated copy of UTF-16 units (one unit when there are none), or NULL without memory. */
static uint16_t *
storage_copy_units(
	const uint16_t *units,
	size_t length)
{
	uint16_t *copy;

	/* Room for them, never nothing. */
	copy = malloc((length + 1U) * sizeof(*copy));
	if (copy == NULL)
		return NULL;

	/* Succeeded: the copy. */
	if (length > 0)
		memcpy(copy, units, length * sizeof(*copy));
	return copy;
}

/* Writes the path of an origin's local area's file. */
static int
storage_path(
	const char *origin,
	struct wb_buffer *path)
{
	const char *data;
	const char *home;
	size_t index;
	int error;

	/* The data directory: XDG_DATA_HOME, or ~/.local/share. */
	data = getenv("XDG_DATA_HOME");
	if (data != NULL && data[0] != '\0') {
		error = wb_buffer_printf(path, "%s/%s/", data, STORAGE_DIRECTORY);
	} else {
		home = getenv("HOME");
		if (home == NULL || home[0] == '\0')
			return ENOENT;
		error = wb_buffer_printf(path, "%s/.local/share/%s/", home, STORAGE_DIRECTORY);
	}

	/* A path that could not be written. */
	if (error != 0)
		return error;

	/* The origin's bytes in hexadecimal, and the ending. */
	for (index = 0; origin[index] != '\0' && error == 0; index++)
		error = wb_buffer_printf(path, "%02x", (unsigned)(unsigned char)origin[index]);
	if (error == 0)
		error = wb_buffer_append_string(path, ".ls");
	if (error != 0)
		return error;

	/* Succeeded: the path. */
	return 0;
}

/* Makes each directory of a file's path that is not there (mode 0700). */
static int
storage_make_directories(
	const char *path)
{
	char *copy;
	size_t index;
	int made;
	int error;

	/* A copy the separators can be cut in. */
	copy = strdup(path);
	if (copy == NULL)
		return ENOMEM;

	/* Each directory above the file, from the top. */
	error = 0;
	for (index = 1; copy[index] != '\0'; index++) {
		if (copy[index] != '/')
			continue;

		/* The directory up to this separator. */
		copy[index] = '\0';
		made = mkdir(copy, 0700);
		if (made != 0 && errno != EEXIST)
			error = errno;
		copy[index] = '/';
		if (error != 0)
			break;
	}

	/* The copy is done with. */
	free(copy);

	/* A directory that could not be made. */
	if (error != 0)
		return error;

	/* Succeeded: the directories are there. */
	return 0;
}

/*
 * Reads a local area's items from its file (none when it has no file):
 * the magic line, then for each item the lengths of its key and value
 * (32-bit little-endian, in UTF-16 units) and their units (little-endian).
 */
static int
storage_load(
	struct storage_area *area)
{
	struct wb_buffer path;
	struct wb_buffer file;
	struct wb_units key;
	struct wb_units value;
	const unsigned char *bytes;
	size_t magic;
	size_t at;
	size_t index;
	size_t place;
	uint32_t key_length;
	uint32_t value_length;
	uint16_t unit;
	int differs;
	int present;
	int error;

	/* The file's bytes. */
	wb_buffer_init(&path);
	wb_buffer_init(&file);
	error = storage_path(area->origin, &path);
	if (error == 0)
		error = wb_file_read(wb_buffer_string(&path), &file);
	wb_buffer_release(&path);
	if (error != 0) {
		wb_buffer_release(&file);
		return error;
	}

	/* A file that does not start with the magic line is not read. */
	bytes = (const unsigned char *)file.data;
	magic = strlen(STORAGE_MAGIC);
	differs = 1;
	if (file.length >= magic)
		differs = memcmp(bytes, STORAGE_MAGIC, magic);
	if (differs != 0) {
		wb_buffer_release(&file);
		return EINVAL;
	}

	/* Each item, until the bytes end or one is cut short. */
	wb_units_init(&key);
	wb_units_init(&value);
	at = magic;
	error = 0;
	while (at < file.length && error == 0) {
		error = storage_read_length(bytes, file.length, &at, &key_length);
		if (error == 0)
			error = storage_read_length(bytes, file.length, &at, &value_length);
		if (error != 0)
			break;
		if (((size_t)key_length + value_length) * 2U > file.length - at) {
			error = EINVAL;
			break;
		}

		/* The key's units, each little-endian. */
		key.length = 0;
		value.length = 0;
		for (index = 0; index < key_length && error == 0; index++) {
			unit = (uint16_t)(bytes[at] | (bytes[at + 1U] << 8));
			error = wb_units_append(&key, &unit, 1);
			at += 2U;
		}

		/* The value's. */
		for (index = 0; index < value_length && error == 0; index++) {
			unit = (uint16_t)(bytes[at] | (bytes[at + 1U] << 8));
			error = wb_units_append(&value, &unit, 1);
			at += 2U;
		}

		/* An item that could not be read ends the reading. */
		if (error != 0)
			break;

		/* The item in its place (a key given twice keeps the first). */
		present = storage_find(area, key.data, key.length, &place);
		if (present)
			continue;
		error = storage_insert(area, place, key.data, key.length, value.data, value.length);
		if (error == 0)
			area->bytes += (key.length + value.length) * 2U;
	}

	/* The buffers are done with. */
	wb_units_release(&key);
	wb_units_release(&value);
	wb_buffer_release(&file);

	/* A file cut short keeps what was read before the cut. */
	if (error != 0)
		return error;

	/* Succeeded: the items are in the area. */
	return 0;
}

/* Writes a local area's items to its file, through a temporary file renamed over it. */
static int
storage_save(
	const struct storage_area *area)
{
	const struct storage_item *items;
	struct wb_buffer path;
	struct wb_buffer temporary;
	struct wb_buffer out;
	size_t index;
	int renamed;
	int error;

	/* The file's bytes. */
	wb_buffer_init(&out);
	items = area->items.items;
	error = wb_buffer_append_string(&out, STORAGE_MAGIC);
	for (index = 0; index < area->items.count && error == 0; index++) {
		error = storage_append_length(&out, (uint32_t)items[index].key_length);
		if (error == 0)
			error = storage_append_length(&out, (uint32_t)items[index].value_length);
		if (error == 0)
			error = storage_append_units(&out, items[index].key, items[index].key_length);
		if (error == 0)
			error = storage_append_units(&out, items[index].value, items[index].value_length);
	}

	/* The file's path, its directories and the temporary file's path. */
	wb_buffer_init(&path);
	wb_buffer_init(&temporary);
	if (error == 0)
		error = storage_path(area->origin, &path);
	if (error == 0)
		error = storage_make_directories(wb_buffer_string(&path));
	if (error == 0)
		error = wb_buffer_printf(&temporary, "%s.%ld", wb_buffer_string(&path), (long)getpid());

	/* The bytes to the temporary file, which then takes the file's place. */
	if (error == 0)
		error = wb_file_write(wb_buffer_string(&temporary), out.data, out.length);
	if (error == 0) {
		renamed = rename(wb_buffer_string(&temporary), wb_buffer_string(&path));
		if (renamed != 0) {
			error = errno;
			(void)unlink(wb_buffer_string(&temporary));
		}
	}

	/* The buffers are done with. */
	wb_buffer_release(&out);
	wb_buffer_release(&path);
	wb_buffer_release(&temporary);

	/* A file that could not be written. */
	if (error != 0)
		return error;

	/* Succeeded: the file holds the area. */
	return 0;
}

/* Reads a 32-bit little-endian length at a place of a file's bytes, moving past it. */
static int
storage_read_length(
	const unsigned char *bytes,
	size_t length,
	size_t *at,
	uint32_t *value)
{
	/* Four bytes must be left. */
	if (length - *at < 4U)
		return EINVAL;

	/* Succeeded: the length. */
	*value = (uint32_t)bytes[*at] | ((uint32_t)bytes[*at + 1U] << 8) | ((uint32_t)bytes[*at + 2U] << 16) | ((uint32_t)bytes[*at + 3U] << 24);
	*at += 4U;
	return 0;
}

/* Appends a 32-bit little-endian length. */
static int
storage_append_length(
	struct wb_buffer *out,
	uint32_t value)
{
	unsigned char bytes[4];
	int error;

	/* The four bytes, the lowest first. */
	bytes[0] = (unsigned char)(value & 0xffU);
	bytes[1] = (unsigned char)((value >> 8) & 0xffU);
	bytes[2] = (unsigned char)((value >> 16) & 0xffU);
	bytes[3] = (unsigned char)((value >> 24) & 0xffU);
	error = wb_buffer_append(out, bytes, sizeof(bytes));
	if (error != 0)
		return error;

	/* Succeeded: the length is written. */
	return 0;
}

/* Appends UTF-16 units, each little-endian. */
static int
storage_append_units(
	struct wb_buffer *out,
	const uint16_t *units,
	size_t length)
{
	unsigned char bytes[2];
	size_t index;
	int error;

	/* Each unit, its low byte first. */
	for (index = 0; index < length; index++) {
		bytes[0] = (unsigned char)(units[index] & 0xffU);
		bytes[1] = (unsigned char)(units[index] >> 8);
		error = wb_buffer_append(out, bytes, sizeof(bytes));
		if (error != 0)
			return error;
	}

	/* Succeeded: the units are written. */
	return 0;
}
