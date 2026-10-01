/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Reads Linux interfaces and resolver data and saves supplicant credentials.
 * Address acquisition remains the system's responsibility.
 */
#include "../wpa/network-wpa.h"
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

static uint64_t link_counter(const char *interface, const char *field);
static int link_wireless(const char *interface);
static int link_ack(struct kwpa_socket *connection, const char *command);

/*
 * Merges address and hardware records into bounded interface snapshots.
 */
size_t
keiland_network_get_links(
	struct keiland_network_link *links,
	size_t capacity)
{
	struct ifaddrs *addresses;
	struct ifaddrs *address;
	struct sockaddr_in *ipv4;
	struct sockaddr_ll *hardware;
	struct ifreq inquiry;
	size_t count;
	size_t index;
	int descriptor;
	int error;
	int same;
	const char *printed;

	/* The caller must provide actual slots before any interface is read. */
	if (links == NULL || capacity == 0)
		return 0;
	error = getifaddrs(&addresses);
	if (error != 0)
		return 0;
	memset(links, 0, capacity * sizeof(*links));
	count = 0;

	/* IPv4 and packet addresses are separate records for the same interface. */
	for (address = addresses; address != NULL; address = address->ifa_next) {
		if (address->ifa_name == NULL)
			continue;
		for (index = 0; index < count; index++) {
			same = strcmp(links[index].name, address->ifa_name);
			if (same == 0)
				break;
		}

		/* New interfaces consume one slot; extra addresses consume no new slots. */
		if (index == count) {
			if (count == capacity)
				continue;
			(void)snprintf(links[index].name, sizeof(links[index].name), "%s", address->ifa_name);
			count++;
		}

		/* Translate flag meaning explicitly rather than storing bit masks. */
		if ((address->ifa_flags & IFF_UP) != 0)
			links[index].up = 1;
		if ((address->ifa_flags & IFF_RUNNING) != 0)
			links[index].running = 1;
		if ((address->ifa_flags & IFF_LOOPBACK) != 0)
			links[index].loopback = 1;
		if (address->ifa_addr == NULL)
			continue;

		/* The public API carries one IPv4 address and netmask per interface. */
		if (address->ifa_addr->sa_family == AF_INET) {
			ipv4 = (struct sockaddr_in *)address->ifa_addr;
			printed = inet_ntop(AF_INET, &ipv4->sin_addr, links[index].address, sizeof(links[index].address));
			if (printed == NULL)
				links[index].address[0] = '\0';
			if (address->ifa_netmask != NULL) {
				ipv4 = (struct sockaddr_in *)address->ifa_netmask;
				printed = inet_ntop(AF_INET, &ipv4->sin_addr, links[index].netmask, sizeof(links[index].netmask));
				if (printed == NULL)
					links[index].netmask[0] = '\0';
			}
		} else if (address->ifa_addr->sa_family == AF_PACKET) {
			hardware = (struct sockaddr_ll *)address->ifa_addr;
			if (hardware->sll_halen >= sizeof(links[index].hardware))
				memcpy(links[index].hardware, hardware->sll_addr, sizeof(links[index].hardware));
		}
	}

	/* Address storage is no longer needed once all records have been copied. */
	freeifaddrs(addresses);
	descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);

	/* Statistics and MTU are independent of whether an IPv4 address exists. */
	for (index = 0; index < count; index++) {
		links[index].received_bytes = link_counter(links[index].name, "rx_bytes");
		links[index].sent_bytes = link_counter(links[index].name, "tx_bytes");
		if (descriptor >= 0) {
			memset(&inquiry, 0, sizeof(inquiry));
			(void)snprintf(inquiry.ifr_name, sizeof(inquiry.ifr_name), "%s", links[index].name);
			error = ioctl(descriptor, SIOCGIFMTU, &inquiry);
			if (error == 0 && inquiry.ifr_mtu > 0)
				links[index].mtu = (unsigned)inquiry.ifr_mtu;
		}
	}

	/* The optional inquiry socket is the only retained kernel resource. */
	if (descriptor >= 0)
		(void)close(descriptor);

	/* Succeeded: every reported entry belongs to the caller's capacity. */
	return count;
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
 * Saves an exact SSID and escaped passphrase without enabling the profile.
 */
int
keiland_network_save_key(
	const char *ssid,
	const char *key)
{
	struct kwpa_socket connection;
	char reply[KWPA_REPLY_MAX];
	char command[256];
	char encoded[KEILAND_NETWORK_SSID_MAX * 2];
	char quoted[KEILAND_NETWORK_KEY_MAX * 2 + 1];
	char *end;
	const char *digits = "0123456789abcdef";
	size_t ssid_length;
	size_t key_length;
	size_t index;
	size_t copied;
	unsigned id;
	unsigned long number;
	unsigned char byte;
	int error;

	/* Reject out-of-contract data before creating a credential profile. */
	if (ssid == NULL || key == NULL)
		return EINVAL;
	ssid_length = strlen(ssid);
	key_length = strlen(key);
	if (ssid_length == 0 ||
	    ssid_length >= KEILAND_NETWORK_SSID_MAX ||
	    key_length < KEILAND_NETWORK_KEY_MIN ||
	    key_length > KEILAND_NETWORK_KEY_MAX)
		return EINVAL;

	/* Hex SSIDs cannot introduce quoting or control-interface commands. */
	for (index = 0; index < ssid_length; index++) {
		byte = (unsigned char)ssid[index];
		encoded[index * 2] = digits[byte >> 4];
		encoded[index * 2 + 1] = digits[byte & 15];
	}

	/* Printable passphrases escape the two quote-sensitive characters. */
	encoded[ssid_length * 2] = '\0';
	copied = 0;
	for (index = 0; index < key_length; index++) {
		byte = (unsigned char)key[index];
		if (byte < 32 || byte > 126)
			return EINVAL;
		if (byte == '"' || byte == '\\')
			quoted[copied++] = '\\';
		quoted[copied++] = (char)byte;
	}

	/* This operation owns its socket independently of all existing watches. */
	quoted[copied] = '\0';
	error = kwpa_open(&connection, NULL);
	if (error != 0)
		return error;
	error = kwpa_call(&connection, "LIST_NETWORKS", reply, sizeof(reply));
	if (error != 0) {
		kwpa_close(&connection);
		return error;
	}

	/* Existing profiles keep their identity; new profiles start disabled. */
	error = kwpa_profile(reply, ssid, &id);
	if (error == ENOENT) {
		error = kwpa_call(&connection, "ADD_NETWORK", reply, sizeof(reply));
		if (error != 0) {
			kwpa_close(&connection);
			return error;
		}

		/* Validate the returned numeric ID before constructing another command. */
		errno = 0;
		number = strtoul(reply, &end, 10);
		if (errno != 0 ||
		    end == reply ||
		    (*end != '\n' && *end != '\0') ||
		    number > 0x7fffffffUL) {
			kwpa_close(&connection);
			return EPROTO;
		}

		/* The bounded ID is safe to format into subsequent daemon requests. */
		id = (unsigned)number;
		error = 0;
	}

	/* A malformed listing cannot authorize modification of another profile. */
	if (error != 0) {
		kwpa_close(&connection);
		return error;
	}

	/* The profile remains disabled until the explicit JOIN request. */
	(void)snprintf(command, sizeof(command), "SET_NETWORK %u ssid %s", id, encoded);
	error = link_ack(&connection, command);
	if (error != 0) {
		kwpa_close(&connection);
		return error;
	}

	/* Supplicant parses the escaped printable passphrase within quotes. */
	(void)snprintf(command, sizeof(command), "SET_NETWORK %u psk \"%s\"", id, quoted);
	error = link_ack(&connection, command);
	if (error != 0) {
		kwpa_close(&connection);
		return error;
	}

	/* Persistence failure is reported even though the daemon retains its memory. */
	error = link_ack(&connection, "SAVE_CONFIG");
	kwpa_close(&connection);
	if (error != 0)
		return error;

	/* Succeeded: saved credentials exist without a new connection attempt. */
	return 0;
}

/*
 * Copies decoded saved SSIDs from an independent bounded daemon query.
 */
size_t
keiland_network_get_saved(
	char (*ssids)[KEILAND_NETWORK_SSID_MAX],
	size_t capacity)
{
	struct kwpa_socket connection;
	char reply[KWPA_REPLY_MAX];
	char *line;
	char *save;
	char *tab;
	char *end;
	size_t count;
	int error;

	/* A caller with no slots needs no daemon connection. */
	if (ssids == NULL || capacity == 0)
		return 0;
	error = kwpa_open(&connection, NULL);
	if (error != 0)
		return 0;
	error = kwpa_call(&connection, "LIST_NETWORKS", reply, sizeof(reply));
	kwpa_close(&connection);
	if (error != 0)
		return 0;

	/* The heading is not a saved network. */
	count = 0;
	line = strtok_r(reply, "\n", &save);
	if (line == NULL)
		return 0;

	/* Copy complete SSID fields, leaving profile flags inside the daemon. */
	while (count < capacity) {
		line = strtok_r(NULL, "\n", &save);
		if (line == NULL)
			break;
		tab = strchr(line, '\t');
		if (tab == NULL)
			continue;
		tab++;
		end = strchr(tab, '\t');
		if (end == NULL)
			continue;
		*end = '\0';
		kwpa_decode(tab, ssids[count], KEILAND_NETWORK_SSID_MAX);
		if (ssids[count][0] != '\0')
			count++;
	}

	/* Succeeded: the caller holds a bounded credential-name list. */
	return count;
}

/*
 * Changes only the selected Linux radio's administrative up flag.
 */
int
kwpa_radio(
	const char *interface,
	unsigned enabled)
{
	struct ifreq request;
	int descriptor;
	int error;

	/* The kernel enforces CAP_NET_ADMIN rather than pretending user success. */
	descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (descriptor < 0)
		return errno;
	memset(&request, 0, sizeof(request));
	(void)snprintf(request.ifr_name, sizeof(request.ifr_name), "%s", interface);
	error = ioctl(descriptor, SIOCGIFFLAGS, &request);
	if (error != 0) {
		error = errno;
		(void)close(descriptor);
		return error;
	}

	/* Preserve every flag other than the requested administrative state. */
	if (enabled != 0) {
		request.ifr_flags |= IFF_UP;
	} else {
		request.ifr_flags &= ~IFF_UP;
	}

	/* Report the actual permission or device failure to the user request. */
	error = ioctl(descriptor, SIOCSIFFLAGS, &request);
	if (error != 0) {
		error = errno;
		(void)close(descriptor);
		return error;
	}

	/* The inquiry socket owns no persistent radio state. */
	(void)close(descriptor);

	/* Succeeded: the requested administrative flag is installed. */
	return 0;
}

/*
 * Refreshes connection kind from addressed interfaces without inventing DHCP.
 */
void
kwpa_links(
	struct keiland_network_state *state)
{
	struct keiland_network_link links[KEILAND_NETWORK_LINKS_MAX];
	size_t count;
	size_t index;
	int wireless;

	/* A supplicant authentication alone does not imply an acquired address. */
	state->connected = 0;
	state->kind = KEILAND_NETWORK_NONE;
	state->interface[0] = '\0';
	state->wired[0] = '\0';
	count = keiland_network_get_links(links, KEILAND_NETWORK_LINKS_MAX);

	/* Wired reachability remains visible even while a Wi-Fi connection is chosen. */
	for (index = 0; index < count; index++) {
		if (links[index].loopback != 0 ||
		    links[index].up == 0 ||
		    links[index].address[0] == '\0')
			continue;
		wireless = link_wireless(links[index].name);
		if (wireless != 0) {
			if (state->wifi != KEILAND_WIFI_CONNECTED)
				continue;
			state->kind = KEILAND_NETWORK_WIFI;
			state->connected = 1;
			(void)snprintf(state->interface, sizeof(state->interface), "%s", links[index].name);
		} else {
			(void)snprintf(state->wired, sizeof(state->wired), "%s", links[index].name);
			if (state->kind != KEILAND_NETWORK_WIFI) {
				state->kind = KEILAND_NETWORK_WIRED;
				state->connected = 1;
				(void)snprintf(state->interface, sizeof(state->interface), "%s", links[index].name);
			}
		}
	}

	/* Succeeded: address acquisition is reflected independently of the radio. */
	return;
}

/* Reads one optional sysfs statistic as a bounded unsigned counter. */
static uint64_t
link_counter(
	const char *interface,
	const char *field)
{
	char path[256];
	unsigned long long bytes;
	FILE *file;
	int parsed;

	/* Missing statistics are reported as zero rather than inventing traffic. */
	(void)snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/%s", interface, field);
	file = fopen(path, "r");
	if (file == NULL)
		return 0;
	parsed = fscanf(file, "%llu", &bytes);
	(void)fclose(file);
	if (parsed != 1)
		return 0;

	/* Succeeded: the kernel's counter is copied without retaining its file. */
	return (uint64_t)bytes;
}

/* Identifies Wi-Fi by kernel sysfs topology rather than interface spelling. */
static int
link_wireless(
	const char *interface)
{
	char path[256];
	struct stat status;
	int error;

	/* Wireless extensions expose a wireless directory on older drivers. */
	(void)snprintf(path, sizeof(path), "/sys/class/net/%s/wireless", interface);
	error = stat(path, &status);
	if (error == 0)
		return 1;

	/* Modern mac80211 drivers expose their PHY even without wireless extensions. */
	(void)snprintf(path, sizeof(path), "/sys/class/net/%s/phy80211", interface);
	error = stat(path, &status);
	if (error != 0)
		return 0;

	/* Succeeded: the interface belongs to a wireless PHY. */
	return 1;
}

/* Validates an independent credential command's full acknowledgement. */
static int
link_ack(
	struct kwpa_socket *connection,
	const char *command)
{
	char reply[KWPA_REPLY_MAX];
	int error;

	/* Each command consumes its reply before a later command is sent. */
	error = kwpa_call(connection, command, reply, sizeof(reply));
	if (error != 0)
		return error;
	error = kwpa_ok(reply);
	if (error != 0)
		return error;

	/* Succeeded: the daemon accepted this exact credential operation. */
	return 0;
}
