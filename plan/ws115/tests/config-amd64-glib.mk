# ws115-p005: the serial test image of plan/tools/posix (base userland, console
# mirrored on the first serial port) with GLib and what it needs, for running
# plan/ws115/tests/glib-probe on the target.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
include plan/tools/posix/config-amd64-serial.mk
ZEDBSD_USER_PROGRAMS += zlib libffi pcre2 glib
