#!/bin/sh
# ws074: prints the C sources userland/desktop/libbrowser/Makefile (the engine, ws074-p057) and
# userland/desktop/browser/Makefile (main.c and the shell) list, one per line, as paths from the
# repository root (the generated tables, which live under build/, are not included).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
sed -n 's/^[^#]*\$(LIBBROWSER_DIR)\/\([^ \\]*\.c\).*/userland\/desktop\/libbrowser\/\1/p' userland/desktop/libbrowser/Makefile
sed -n 's/^[^#]*\$(KEILAND_BROWSER_DIR)\/\([^ \\]*\.c\).*/userland\/desktop\/browser\/\1/p' userland/desktop/browser/Makefile
