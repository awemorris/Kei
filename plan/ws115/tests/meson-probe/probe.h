/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The interface of the Meson cross probe library.
 */

#ifndef PROBE_H
#define PROBE_H

#include <stdint.h>

uint32_t probe_checksum(const char *text);

#endif
