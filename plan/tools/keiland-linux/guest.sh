#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Controls only the isolated Debian guest described in WS105.
set -eu
exec python3 "$(dirname "$0")/guest.py" "$@"
