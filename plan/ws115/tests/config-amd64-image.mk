# ws115-p008: the GLib serial test image (config-amd64-glib.mk) with the
# image, keymap and GL-loader packages and zedBSD's EGL, for running
# plan/ws115/tests/image-probe on the target.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
include plan/ws115/tests/config-amd64-glib.mk
ZEDBSD_USER_PROGRAMS += libpng libjpeg-turbo libtiff gdk-pixbuf graphene libepoxy \
	xkeyboard-config libxkbcommon libegl
