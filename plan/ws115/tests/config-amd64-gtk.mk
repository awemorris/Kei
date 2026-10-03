# ws115-p002: the compositor criteria image (plan/ws099/tests/config-amd64-criteria.mk: the graphical desktop with
# Terminal, Files and the Vulkan/EGL stack) with GTK 4 and every package it needs, for p010's runs of gtk4-demo.
# Build with plan/ws115/tests/build-desktop-image.sh BUILD '' ZEDBSD_CONFIG=plan/ws115/tests/config-amd64-gtk.mk
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
include plan/ws099/tests/config-amd64-criteria.mk
ZEDBSD_USER_PROGRAMS += zlib libffi pcre2 glib libpng freetype harfbuzz expat fontconfig \
	noto-color-emoji pixman fribidi cairo pango libjpeg-turbo libtiff gdk-pixbuf graphene \
	libepoxy xkeyboard-config libxkbcommon gtk4
