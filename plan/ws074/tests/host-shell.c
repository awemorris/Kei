/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The host build's stand-in for the zdesktop window: the host tests use the
 * headless modes only.
 */

#include "shell/shell.h"

#include <stdio.h>

/*
 * Refuses the window mode on the host.
 */
int
shell_run(
	const struct shell_options *options)
{
	(void)options;

	/* Says why nothing opened. */
	fprintf(stderr, "browser: the host build has no window mode\n");

	/* Reports the refusal. */
	return 1;
}
