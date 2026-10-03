#!/bin/sh
# ws005-p028 (BUG-157): builds the WLAN common core (src/kern/net/wifi) with WLAN_TESTING against the stubs of
# host-wlan-retire.c and runs it: a failed connection's reason survives the close that follows it.
# Last line: host-wlan-retire: PASS.   With REVERT=1 the core is built from the commit before the fix (git show
# HEAD~1:...) to show the case fails there (the tree is not changed).
#   sh plan/ws005/tests/host-wlan-retire.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws005-host/wlan-retire
rm -rf "$out"
mkdir -p "$out"
cc=${CC:-cc}
flags="-std=gnu89 -O1 -g -w -DWLAN_TESTING -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -I. -Iinclude -Isrc -Isrc/kern/net/wifi"
for file in wlan wlan-wpa2 wlan-wpa2-codec wlan-crypto wlan-frame wlan-l2; do
	source=src/kern/net/wifi/$file.c
	if [ "${REVERT:-}" = 1 ] && [ "$file" = wlan ]; then
		git show "${REVERT_COMMIT:-HEAD~1}:src/kern/net/wifi/wlan.c" > "$out/wlan-old.c"
		source=$out/wlan-old.c
	fi
	"$cc" $flags -c "$source" -o "$out/$file.o"
done
# The test itself is built with -w too: the kernel's uapi headers redefine the host's SIOC* numbers.
"$cc" -std=gnu89 -O1 -g -w -DWLAN_TESTING -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -I. -Iinclude -Isrc \
    -c plan/ws005/tests/host-wlan-retire.c -o "$out/main.o"
"$cc" -o "$out/host-wlan-retire" "$out"/*.o
exec "$out/host-wlan-retire"
