/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Linux network details, placeholder until ws105-p010.
 * Resolver configuration is readable even without a Wi-Fi service.
 */

#include <keiland.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * Reports no interfaces until the Linux interface backend is installed.
 */
size_t
keiland_network_get_links(
	struct keiland_network_link *links,
	size_t capacity)
{
	/* The placeholder has no interface information. */
	(void)links;
	(void)capacity;

	/* Succeeded: zero interface entries are available. */
	return 0;
}

/*
 * Copies dotted IPv4 resolver addresses from the system configuration.
 */
size_t
keiland_network_get_dns(
	char (*servers)[KEILAND_NETWORK_ADDRESS_MAX],
	size_t capacity)
{
	FILE *file;
	char line[256];
	char word[32];
	char address[64];
	char *read_line;
	struct in_addr ipv4;
	size_t count;
	int fields;
	int differs;
	int parsed;

	/* Avoids opening configuration when the caller has no space. */
	if (servers == NULL || capacity == 0)
		return 0;

	/* An unreadable resolver configuration has no reported servers. */
	file = fopen("/etc/resolv.conf", "r");
	if (file == NULL)
		return 0;

	/* Copies only nameserver lines holding a valid dotted IPv4 address. */
	count = 0;
	while (count < capacity) {
		/* Reads the next resolver entry without retaining the file buffer. */
		read_line = fgets(line, sizeof(line), file);
		if (read_line == NULL)
			break;

		/* Rejects incomplete configuration entries. */
		fields = sscanf(line, "%31s %63s", word, address);
		if (fields != 2)
			continue;

		/* Ignores unrelated resolver directives. */
		differs = strcmp(word, "nameserver");
		if (differs != 0)
			continue;

		/* Rejects IPv6 and malformed IPv4 entries for this public interface. */
		parsed = inet_pton(AF_INET, address, &ipv4);
		if (parsed != 1)
			continue;

		/* Publishes the textual address in the caller's next slot. */
		(void)snprintf(servers[count], KEILAND_NETWORK_ADDRESS_MAX, "%s", address);
		count++;
	}

	/* Releases the resolver file after the bounded scan. */
	(void)fclose(file);

	/* Succeeded: reports how many server addresses were copied. */
	return count;
}

/*
 * Refuses credential changes before the Wi-Fi backend is installed.
 */
int
keiland_network_save_key(
	const char *ssid,
	const char *key)
{
	/* Credential persistence is not yet provided by the placeholder. */
	(void)ssid;
	(void)key;

	/* Reports that this backend cannot save credentials yet. */
	return ENOTSUP;
}

/*
 * Reports an empty credential list before Wi-Fi is installed.
 */
size_t
keiland_network_get_saved(
	char (*ssids)[KEILAND_NETWORK_SSID_MAX],
	size_t capacity)
{
	/* The placeholder has no credential store. */
	(void)ssids;
	(void)capacity;

	/* Succeeded: zero saved networks are available. */
	return 0;
}
