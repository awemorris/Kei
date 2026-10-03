#!/bin/sh
# ws099-p028: the host reproduction of NUL bytes in zdesktop's log (host-log-append.c).  For each mode an old writer
# (">" into the file) prints, the file is truncated by a new ">", and the old writer prints again.  Prints the NUL
# bytes in the file each time: "plain" (no O_APPEND, as zdesktop was) has them, "append" (zdesktop's fix) has none.
# Last line: host-log-append: PASS when plain shows the hole and append does not.
#   sh plan/ws099/tests/host-log-append.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
out=build/ws099-host/log-append
mkdir -p "$out"
cc -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -Werror plan/ws099/tests/host-log-append.c -o "$out/writer" || exit 1
nuls() { size=$(wc -c < "$1"); kept=$(tr -d '\000' < "$1" | wc -c); echo $((size - kept)); }
status=0
for mode in plain append; do
	file=$out/$mode.log
	rm -f "$file"
	"$out/writer" "$file" $mode > "$file" &
	sleep 0.3
	: > "$file"
	echo "ZWL READY new run" >> "$file"
	wait
	count=$(nuls "$file")
	echo "$mode: NUL bytes=$count"
done
plain=$(nuls "$out/plain.log"); append=$(nuls "$out/append.log")
[ "$plain" -gt 0 ] && [ "$append" = 0 ] && echo "host-log-append: PASS" || { echo "host-log-append: FAIL"; status=1; }
exit $status
