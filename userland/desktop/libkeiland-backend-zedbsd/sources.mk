# The zedBSD side of libkeiland-backend (WS131, plan/ws131/design.md section 3.6): the compositor's
# operating system, linked into the compositor.  This fragment is not a package Makefile (the top-level
# Makefile includes every userland/*/*/Makefile as a package); the compositor's Makefile includes it, and
# libkeiland's Makefile too while libkeiland still forwards its old network and sound calls here (until ws131-p011).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
KL_BACKEND_ZEDBSD_SOURCES := userland/desktop/libkeiland-backend/backend.c \
	userland/desktop/libkeiland-backend-zedbsd/network-zedbsd.c \
	userland/desktop/libkeiland-backend-zedbsd/network-link-zedbsd.c \
	userland/desktop/libkeiland-backend-zedbsd/audio-zedbsd.c \
	userland/desktop/libkeiland-backend-zedbsd/power-zedbsd.c \
	userland/base/net/protocol.c userland/base/net/wifi-conf.c userland/base/net/wifi-store.c
