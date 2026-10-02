# ws115-p006: the GLib serial test image (config-amd64-glib.mk) with the text
# stack -- libpng, FreeType, HarfBuzz, fontconfig -- and the colour emoji
# font, for running plan/ws115/tests/text-probe and fc-list on the target.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
include plan/ws115/tests/config-amd64-glib.mk
ZEDBSD_USER_PROGRAMS += libpng freetype harfbuzz expat fontconfig noto-color-emoji
