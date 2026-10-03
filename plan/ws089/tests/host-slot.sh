#!/bin/sh
# ws089-p012 (C1): builds Settings' network.c with host-slot.c (a pretend libkeiland carrying one request at a time) and
# runs it: requests asked for while another is out wait in one slot and go after its answer.  Last line: host-slot: PASS.
#   sh plan/ws089/tests/host-slot.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws089-host
mkdir -p "$out/include" "$out/obj"
ln -sf "$(pwd)/userland/desktop/keiland/truetype.h" "$out/include/truetype.h"
ln -sf "$(pwd)/userland/desktop/keiland/keiland.h" "$out/include/keiland.h"
cc=${CC:-cc}
flags="-O2 -g -std=gnu89 -Wall -Wextra -Werror -Wno-unused-parameter -D_GNU_SOURCE -I$out/include -Iuserland/desktop/settings -I."
"$cc" $flags -c userland/desktop/settings/network.c -o "$out/obj/slot-network.o"
"$cc" $flags -c plan/ws089/tests/host-slot.c -o "$out/obj/host-slot.o"
"$cc" -o "$out/host-slot" "$out/obj/host-slot.o" "$out/obj/slot-network.o"
exec "$out/host-slot"
