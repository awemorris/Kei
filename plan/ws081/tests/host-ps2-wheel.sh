#!/bin/sh
# ws081-p019 (BUG-156): builds and runs the host test of the PS/2 mouse driver's IntelliMouse protocols
# (host-ps2-wheel.c, which compiles src/drivers/platform/pcat/ps2-8042.c against a model of the 8042 and the mouse).
# Last line: host-ps2-wheel: PASS.
#   sh plan/ws081/tests/host-ps2-wheel.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws081-host
mkdir -p "$out"
cc=${CC:-cc}
"$cc" -std=gnu89 -O1 -g -Wall -Wextra -Werror -I. -Iinclude plan/ws081/tests/host-ps2-wheel.c -o "$out/host-ps2-wheel"
exec "$out/host-ps2-wheel"
