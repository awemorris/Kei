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

/* The bytes kept of the session manager's lines not read whole yet. */
#define KL_BACKEND_SESSION_LINE	256U

/*
 * The compositor's backend.
 *
 * Allocated by kl_backend_open and freed by kl_backend_close.  host and
 * options are copies of what the compositor passed, kept unchanged for the
 * areas that call back.  power_asked is the action the power area has
 * asked of the session manager (0 when none), so that only one is asked.
 *
 * The session's state (ws131-p006): session_request is the request whose
 * answer is awaited (KL_BACKEND_SESSION_NONE when none); session_line holds
 * session_used bytes of a line not read whole; session_gone is set once
 * the manager closed a session's descriptor (it no longer listens);
 * logout_asked and logout_ms say a Log Out was asked and when the first
 * tick after it saw it (0 until then).
 */
struct kl_backend {
	struct kl_backend_host host;
	struct kl_backend_options options;
	unsigned power_asked;
	unsigned session_request;
	char session_line[KL_BACKEND_SESSION_LINE];
	size_t session_used;
	unsigned session_gone;
	unsigned logout_asked;
	uint64_t logout_ms;
};

/*
 * The session's work in kl_backend_tick: each operating system's session
 * reads what its manager sent and keeps its deadlines.
 */
void kl_backend_session_tick(struct kl_backend *backend, uint64_t now_ms);

#endif
