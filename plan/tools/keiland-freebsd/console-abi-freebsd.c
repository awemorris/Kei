/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Reports installed native console request values for an independent owned-fixture controller. */
#include <sys/consio.h>
#include <sys/kbio.h>
#include <stdio.h>

/*
 * Supplies native ABI constants without copying platform ioctl encodings into Python.
 */
int
main(
	void)
{
	int status;

	/* The controller captures and restores original VT, keyboard, drawing and terminal state. */
	status = printf("{\"get_active\":%lu,\"activate\":%lu,\"get_vt_mode\":%lu,\"set_vt_mode\":%lu,\"vt_size\":%lu,\"get_drawing\":%lu,\"set_drawing\":%lu,\"get_keyboard\":%lu,\"set_keyboard\":%lu}\n", (unsigned long)VT_GETACTIVE, (unsigned long)VT_ACTIVATE, (unsigned long)VT_GETMODE, (unsigned long)VT_SETMODE, (unsigned long)sizeof(struct vt_mode), (unsigned long)KDGETMODE, (unsigned long)KDSETMODE, (unsigned long)KDGKBMODE, (unsigned long)KDSKBMODE);
	if (status < 0)
		return 1;

	/* Succeeded: every request came from the selected native system headers. */
	return 0;
}
