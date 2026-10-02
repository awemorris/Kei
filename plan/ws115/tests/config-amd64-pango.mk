# ws115-p007: the text test image (config-amd64-text.mk) with pixman, Cairo,
# FriBidi and Pango, for running plan/ws115/tests/pango-probe on the target.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
include plan/ws115/tests/config-amd64-text.mk
ZEDBSD_USER_PROGRAMS += pixman fribidi cairo pango
