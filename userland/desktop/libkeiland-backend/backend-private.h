/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The backend object's insides, shared by backend.c and the operating
 * systems' areas that keep state in it (WS131 p005).  The compositor never
 * includes this header; it sees struct kl_backend only through
 * keiland-backend.h.
 */

#ifndef KL_BACKEND_PRIVATE_H
#define KL_BACKEND_PRIVATE_H

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

/*
 * The compositor's backend.
 *
 * Allocated by kl_backend_open and freed by kl_backend_close.  host and
 * options are copies of what the compositor passed, kept unchanged for the
 * areas that call back.  power_asked is the action the power area has
 * asked of the session manager (0 when none), so that only one is asked.
 */
struct kl_backend {
	struct kl_backend_host host;
	struct kl_backend_options options;
	unsigned power_asked;
};

#endif
