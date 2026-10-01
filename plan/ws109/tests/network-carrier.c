/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Observes actual guest carrier loss and restoration induced through owned QMP.
 */
#include "userland/desktop/libkeiland/wpa/network-wpa.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * Samples native link and connected state across one real carrier transition.
 */
int
main(
	void)
{
	struct keiland_network_link links[KEILAND_NETWORK_LINKS_MAX];
	struct keiland_network_state state;
	struct timespec pause = {0, 100000000};
	size_t count;
	size_t index;
	unsigned samples;
	unsigned down;
	unsigned restored;
	int same;
	int error;

	/* Announces readiness before the controller changes the guest's virtual cable. */
	down = 0;
	restored = 0;
	(void)printf("READY native carrier sampling\n");
	(void)fflush(stdout);

	/* Bounds the fixture to eight seconds while sampling actual kernel metadata. */
	for (samples = 0; samples < 80; samples++) {
		/* Selects the independently addressed native wired state on every sample. */
		memset(&state, 0, sizeof(state));
		kwpa_links(&state);
		count = keiland_network_get_links(links, KEILAND_NETWORK_LINKS_MAX);

		/* Compares the external interface's actual carrier with public connection kind. */
		for (index = 0; index < count; index++) {
			/* Other guest interfaces cannot stand in for the controlled virtual cable. */
			same = strcmp(links[index].name, "vtnet0");
			if (same != 0)
				continue;

			/* An administratively enabled but carrier-down interface must lose connectivity. */
			if (links[index].running == 0) {
				if (links[index].up != 1 || state.connected != 0)
					return 1;
				down++;
			} else if (down != 0) {
				/* Restored carrier must recover the retained addressed connection. */
				if (state.connected != 1 || state.kind != KEILAND_NETWORK_WIRED)
					return 1;
				restored++;
			}
		}

		/* Samples on real time without injecting link records or clock behavior. */
		error = nanosleep(&pause, NULL);
		if (error != 0)
			return 1;
	}

	/* Both loss and recovery must have been observed through the actual native kernel. */
	if (down == 0 || restored == 0)
		return 1;

	/* Succeeded: actual carrier loss withdrew connectivity and restoration recovered it. */
	(void)printf("PASS native carrier down=%u restored=%u\n", down, restored);
	return 0;
}
