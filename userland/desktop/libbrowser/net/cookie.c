/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Cookies (RFC 6265, the first pass): the cookies of the process's
 * responses kept in memory for as long as the process runs, and the
 * Cookie header of a request.
 *
 * A cookie has a name, a value, a domain (the request's host, or its
 * Domain attribute, which must domain-match the host), a path (its Path
 * attribute, or the request path's directory), and flags: host-only
 * (without a Domain) and Secure.  Expires and Max-Age only delete a
 * cookie when they are in the past (every kept cookie lasts the session).
 * A script (document.cookie) neither sees nor sets an HttpOnly cookie.
 */

#include "net/net.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* The most cookies the jar keeps (the oldest is dropped first). */
#define COOKIE_MAX		512U

/*
 * One cookie: malloc'd strings and its flags.
 */
struct cookie {
	char *name;
	char *value;
	char *domain;
	char *path;
	int host_only;
	int secure;
	int http_only;
};

/*
 * The process's cookie jar: the cookies in the order they were set.  It
 * lives as long as the process; browser's pages run on one thread,
 * so nothing else touches it.
 */
static struct cookie cookie_jar[COOKIE_MAX];

/* How many entries of cookie_jar are in use (the first ones). */
static size_t cookie_count;

static int cookie_store(const struct net_url *url, const char *header, size_t length, int from_script);
static int cookie_pairs(const struct net_url *url, int for_script, struct wb_buffer *out);
static char *cookie_copy(const char *text, size_t length);
static void cookie_free(struct cookie *cookie);
static void cookie_remove(size_t index);
static int cookie_same(const struct cookie *a, const struct cookie *b);
static int cookie_domain_matches(const char *host, const char *domain);
static int cookie_path_matches(const char *request_path, const char *path);
static void cookie_default_path(const char *request_path, struct wb_buffer *out);
static int cookie_attribute(const char *text, size_t length, const char *name, const char **value, size_t *value_length);
static void cookie_lower(char *text);

/*
 * Keeps the cookie a Set-Cookie header of a response to a URL sets (or
 * deletes, for one already expired).  A header that sets nothing is
 * ignored.  Returns 0 or ENOMEM.
 */
int
net_cookie_store(
	const struct net_url *url,
	const char *header,
	size_t length)
{
	int error;

	/* A response may set any cookie. */
	error = cookie_store(url, header, length, 0);
	if (error != 0)
		return error;

	/* Succeeded: the jar is up to date. */
	return 0;
}

/*
 * Keeps the cookie a script sets with document.cookie on a document of a
 * URL, in the form of a Set-Cookie header.  A cookie marked HttpOnly, and
 * one that would replace an HttpOnly cookie, is ignored.  Returns 0 or
 * ENOMEM.
 */
int
net_cookie_store_script(
	const struct net_url *url,
	const char *text,
	size_t length)
{
	int error;

	/* A script may not touch the cookies only HTTP sees. */
	error = cookie_store(url, text, length, 1);
	if (error != 0)
		return error;

	/* Succeeded: the jar is up to date. */
	return 0;
}

/*
 * Appends the Cookie header of a request to a URL ("Cookie: a=b; c=d"
 * and CR LF), when any cookie is for it.
 */
int
net_cookie_header(
	const struct net_url *url,
	struct wb_buffer *out)
{
	struct wb_buffer pairs;
	int error;

	/* The name and value pairs of every cookie for the URL. */
	wb_buffer_init(&pairs);
	error = cookie_pairs(url, 0, &pairs);

	/* The header, when there is a cookie. */
	if (error == 0 && pairs.length != 0)
		error = wb_buffer_printf(out, "Cookie: %s\r\n", wb_buffer_string(&pairs));
	wb_buffer_release(&pairs);
	if (error != 0)
		return error;

	/* Succeeded: the header is written (or there was none to write). */
	return 0;
}

/*
 * Appends what document.cookie reads on a document of a URL: the pairs of
 * the cookies for it that are not HttpOnly ("a=b; c=d", or nothing).
 */
int
net_cookie_string(
	const struct net_url *url,
	struct wb_buffer *out)
{
	int error;

	/* The pairs a script may see. */
	error = cookie_pairs(url, 1, out);
	if (error != 0)
		return error;

	/* Succeeded: the pairs are written. */
	return 0;
}

/*
 * Keeps the cookie of a Set-Cookie header (or of document.cookie when
 * from_script is set, which leaves HttpOnly cookies alone).
 */
static int
cookie_store(
	const struct net_url *url,
	const char *header,
	size_t length,
	int from_script)
{
	struct cookie cookie;
	struct wb_buffer path;
	const char *pair_end;
	const char *equals;
	const char *attributes;
	const char *value;
	const char *next;
	size_t value_length;
	size_t name_length;
	size_t index;
	int expired;
	int matches;
	int found;

	/* The name and value before the first semicolon (the name may be empty). */
	memset(&cookie, 0, sizeof(cookie));
	pair_end = memchr(header, ';', length);
	if (pair_end == NULL)
		pair_end = header + length;
	equals = memchr(header, '=', (size_t)(pair_end - header));
	if (equals == NULL)
		return 0;
	name_length = (size_t)(equals - header);
	while (name_length > 0 && header[name_length - 1U] == ' ')
		name_length--;
	value = equals + 1;
	while (value < pair_end && *value == ' ')
		value++;
	value_length = (size_t)(pair_end - value);
	while (value_length > 0 && value[value_length - 1U] == ' ')
		value_length--;
	cookie.name = cookie_copy(header, name_length);
	cookie.value = cookie_copy(value, value_length);

	/* The attributes after it. */
	expired = 0;
	attributes = pair_end;
	while (attributes < header + length) {
		attributes++;
		while (attributes < header + length && *attributes == ' ')
			attributes++;
		next = memchr(attributes, ';', (size_t)(header + length - attributes));
		if (next == NULL)
			next = header + length;

		/* Domain: a leading dot dropped, in lower case. */
		found = cookie_attribute(attributes, (size_t)(next - attributes), "domain", &value, &value_length);
		if (found && value_length > 0) {
			if (*value == '.') {
				value++;
				value_length--;
			}

			/* A later Domain replaces an earlier one. */
			free(cookie.domain);
			cookie.domain = cookie_copy(value, value_length);
		}

		/* Path: only one that starts with a slash. */
		found = cookie_attribute(attributes, (size_t)(next - attributes), "path", &value, &value_length);
		if (found && value_length > 0 && *value == '/') {
			free(cookie.path);
			cookie.path = cookie_copy(value, value_length);
		}

		/* Max-Age of zero or less deletes it (a date in the past is not checked in this pass). */
		found = cookie_attribute(attributes, (size_t)(next - attributes), "max-age", &value, &value_length);
		if (found && value_length > 0 && (*value == '-' || (*value == '0' && value_length == 1U)))
			expired = 1;

		/* The flags. */
		found = cookie_attribute(attributes, (size_t)(next - attributes), "secure", &value, &value_length);
		if (found)
			cookie.secure = 1;
		found = cookie_attribute(attributes, (size_t)(next - attributes), "httponly", &value, &value_length);
		if (found)
			cookie.http_only = 1;
		attributes = next;
	}

	/* No Domain: the request's host alone; a Domain must cover the host. */
	if (cookie.domain != NULL)
		cookie_lower(cookie.domain);
	if (cookie.domain == NULL && url->host != NULL) {
		cookie.domain = cookie_copy(url->host, strlen(url->host));
		cookie.host_only = 1;
	}

	/* A cookie that could not be copied, or whose domain misses the host, is not kept. */
	matches = 0;
	if (cookie.domain != NULL && cookie.name != NULL && cookie.value != NULL && url->host != NULL)
		matches = cookie_domain_matches(url->host, cookie.domain);
	if (!matches) {
		cookie_free(&cookie);
		return 0;
	}

	/* No Path: the directory of the request's path. */
	if (cookie.path == NULL) {
		wb_buffer_init(&path);
		cookie_default_path(url->path, &path);
		cookie.path = cookie_copy(wb_buffer_string(&path), path.length);
		wb_buffer_release(&path);
	}

	/* A path that could not be copied. */
	if (cookie.path == NULL) {
		cookie_free(&cookie);
		return ENOMEM;
	}

	/* A script cannot set a cookie only HTTP may see. */
	if (from_script && cookie.http_only) {
		cookie_free(&cookie);
		return 0;
	}

	/* A cookie of the same name, domain and path is replaced (or only removed when expired). */
	for (index = 0; index < cookie_count; index++) {
		matches = cookie_same(&cookie_jar[index], &cookie);
		if (!matches)
			continue;

		/* A script cannot replace an HttpOnly cookie either. */
		if (from_script && cookie_jar[index].http_only) {
			cookie_free(&cookie);
			return 0;
		}

		/* The old cookie gives way. */
		cookie_remove(index);
		break;
	}

	/* An expired cookie is only removed. */
	if (expired) {
		cookie_free(&cookie);
		return 0;
	}

	/* A full jar drops its oldest cookie. */
	if (cookie_count == COOKIE_MAX)
		cookie_remove(0);

	/* Succeeded: the jar keeps the cookie. */
	cookie_jar[cookie_count] = cookie;
	cookie_count++;
	return 0;
}

/*
 * Appends the name and value pairs of the cookies for a URL, separated by
 * "; " (only the ones a script may see when for_script is set).
 */
static int
cookie_pairs(
	const struct net_url *url,
	int for_script,
	struct wb_buffer *out)
{
	const struct cookie *cookie;
	const char *host;
	size_t index;
	int is_https;
	int written;
	int matches;
	int error;

	/* Each cookie whose domain, path and security fit the URL. */
	host = url->host;
	if (host == NULL)
		return 0;
	is_https = !strcmp(url->scheme, "https");
	written = 0;
	error = 0;
	for (index = 0; error == 0 && index < cookie_count; index++) {
		cookie = &cookie_jar[index];
		if (cookie->host_only) {
			matches = !strcmp(host, cookie->domain);
		} else {
			matches = cookie_domain_matches(host, cookie->domain);
		}

		/* The path and the security must fit too. */
		if (!matches)
			continue;
		matches = cookie_path_matches(url->path, cookie->path);
		if (!matches || (cookie->secure && !is_https))
			continue;

		/* A script does not see an HttpOnly cookie. */
		if (for_script && cookie->http_only)
			continue;

		/* "; " between them. */
		if (written)
			error = wb_buffer_append_string(out, "; ");

		/* The name (when it has one) and the value. */
		written = 1;
		if (error == 0 && cookie->name[0] != '\0')
			error = wb_buffer_printf(out, "%s=", cookie->name);
		if (error == 0)
			error = wb_buffer_append_string(out, cookie->value);
	}

	/* Reports a buffer that could not grow. */
	if (error != 0)
		return error;

	/* Succeeded: the pairs are written (or there was none). */
	return 0;
}

/* Copies a text into a new string. */
static char *
cookie_copy(
	const char *text,
	size_t length)
{
	char *copy;

	/* The bytes and a terminator. */
	copy = malloc(length + 1U);
	if (copy == NULL)
		return NULL;
	memcpy(copy, text, length);
	copy[length] = '\0';

	/* The copy. */
	return copy;
}

/* Frees a cookie's strings. */
static void
cookie_free(
	struct cookie *cookie)
{
	/* The four strings. */
	free(cookie->name);
	free(cookie->value);
	free(cookie->domain);
	free(cookie->path);
	memset(cookie, 0, sizeof(*cookie));
}

/* Removes the jar's cookie at an index, the later ones moving down. */
static void
cookie_remove(
	size_t index)
{
	/* The cookie's strings, then the gap closed. */
	cookie_free(&cookie_jar[index]);
	memmove(&cookie_jar[index], &cookie_jar[index + 1U], (cookie_count - index - 1U) * sizeof(cookie_jar[0]));
	cookie_count--;
}

/* Tells whether two cookies have the same name, domain and path (one replaces the other). */
static int
cookie_same(
	const struct cookie *a,
	const struct cookie *b)
{
	int differs;

	/* The name, the domain, then the path. */
	differs = strcmp(a->name, b->name);
	if (differs != 0)
		return 0;
	differs = strcmp(a->domain, b->domain);
	if (differs != 0)
		return 0;
	differs = strcmp(a->path, b->path);
	if (differs != 0)
		return 0;

	/* The same cookie. */
	return 1;
}

/* Tells whether a host domain-matches a domain: the same, or a subdomain of it (not an IP address). */
static int
cookie_domain_matches(
	const char *host,
	const char *domain)
{
	size_t host_length;
	size_t domain_length;
	int differs;

	/* The same string. */
	differs = strcmp(host, domain);
	if (differs == 0)
		return 1;

	/* A host that ends with a dot and the domain. */
	host_length = strlen(host);
	domain_length = strlen(domain);
	if (host_length <= domain_length || domain_length == 0)
		return 0;
	differs = strcmp(host + host_length - domain_length, domain);
	if (differs != 0)
		return 0;
	if (host[host_length - domain_length - 1U] != '.')
		return 0;

	/* An IP address has no subdomains. */
	if (host[0] == '[' || (host[host_length - 1U] >= '0' && host[host_length - 1U] <= '9'))
		return 0;
	return 1;
}

/* Tells whether a request path path-matches a cookie's path. */
static int
cookie_path_matches(
	const char *request_path,
	const char *path)
{
	size_t length;
	int differs;

	/* The same path. */
	if (request_path[0] == '\0')
		request_path = "/";
	differs = strcmp(request_path, path);
	if (differs == 0)
		return 1;

	/* A prefix that ends in a slash, or is followed by one. */
	length = strlen(path);
	differs = strncmp(request_path, path, length);
	if (differs != 0)
		return 0;
	if (length > 0 && path[length - 1U] == '/')
		return 1;
	if (request_path[length] == '/')
		return 1;
	return 0;
}

/* Writes the default path of a request path: its directory (up to the last slash), or "/". */
static void
cookie_default_path(
	const char *request_path,
	struct wb_buffer *out)
{
	const char *last;

	/* A path not starting with a slash, or with only the first slash, is "/". */
	last = strrchr(request_path, '/');
	if (request_path[0] != '/' || last == request_path) {
		wb_buffer_append_string(out, "/");
		return;
	}

	/* Up to the last slash. */
	wb_buffer_append(out, request_path, (size_t)(last - request_path));
}

/* Finds an attribute (name, and an optional =value) in one attribute's text; nonzero when it is that one. */
static int
cookie_attribute(
	const char *text,
	size_t length,
	const char *name,
	const char **value,
	size_t *value_length)
{
	size_t name_length;
	size_t index;
	size_t end;
	int c;

	/* The name in any case, then the end or =. */
	name_length = strlen(name);
	if (length < name_length)
		return 0;
	for (index = 0; index < name_length; index++) {
		c = (unsigned char)text[index];
		if (c >= 'A' && c <= 'Z')
			c += 0x20;
		if (c != (unsigned char)name[index])
			return 0;
	}

	/* Spaces, then the end or "=". */
	index = name_length;
	while (index < length && text[index] == ' ')
		index++;
	if (index < length && text[index] != '=')
		return 0;

	/* The value after =, without surrounding spaces. */
	*value = text + length;
	*value_length = 0;
	if (index < length) {
		index++;
		while (index < length && text[index] == ' ')
			index++;
		end = length;
		while (end > index && text[end - 1U] == ' ')
			end--;
		*value = text + index;
		*value_length = end - index;
	}

	/* The attribute. */
	return 1;
}

/* Folds a string's ASCII letters to lower case. */
static void
cookie_lower(
	char *text)
{
	/* Each byte. */
	for (; *text != '\0'; text++) {
		if (*text >= 'A' && *text <= 'Z')
			*text = (char)(*text + 0x20);
	}
}
