#!/bin/sh
# ws127-p002 (F-039): spring-loaded folders and the scroll at the content's edges under a drag, on the host
# (files-render, host-render.c; run plan/tools/files/host-build.sh first).  Started in Projects/zedBSD (icons):
#  1. README.md dragged onto the folder docs and held there 800 ms: docs opens under the drag
#     ("DRAG spring"), and the release on its empty part moves README.md into it.
#  2. A quick pass over docs (no rest) opens nothing.
#  3. With 150 more files in the folder, README.md held near the content's bottom edge (y 700 of 12..708) scrolls the content:
#     the first item's place moves up.
#   sh plan/ws127/tests/host-spring.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
out=${1:-build/ws127-spring-host}
mkdir -p "$out"
home=$(pwd)/build/ws071-host/home
project=$home/Projects/zedBSD
status=0
readme=671,130
docs=335,130

# Runs files-render (a fresh home unless KEEP is given) started in Projects/zedBSD; output in $out/NAME.txt.
run() {
	name=$1
	shift
	fresh=--fresh
	[ "${KEEP:-}" = 1 ] && fresh=
	sh plan/tools/files/host-run.sh $fresh "--start=$project" "$@" > "$out/$name.txt" 2>&1
}
expect() {
	if grep -qE "$2" "$out/$1.txt"; then echo "$1: $2 ok"; else echo "$1: $2 MISSING"; status=1; fi
}
refuse() {
	if grep -qE "$2" "$out/$1.txt"; then echo "$1: $2 FOUND"; status=1; else echo "$1: no $2 ok"; fi
}

# 1. Held on docs: it springs open, and the drop on its empty part moves the item there.
run spring press=$readme drag=640,132 drag=340,135 wait=400 wait=450 draw="$out/spring.ppm" drag=700,450 release=700,450 wait=400 wait=400
expect spring "DRAG target kind=folder path=$project/docs$"
expect spring "DRAG spring path=$project/docs$"
expect spring "LOCATION kind=folder path=$project/docs "
expect spring "DRAG drop operation=move items=1 destination=$project/docs$"
if [ -f "$project/docs/README.md" ] && [ ! -e "$project/README.md" ]; then echo "spring: README.md moved into docs ok"; else echo "spring: README.md moved into docs MISSING"; status=1; fi

# 2. A quick pass: no spring.
run pass press=$readme drag=640,132 drag=340,135 wait=200 drag=700,450 wait=900 release=700,450
refuse pass "DRAG spring"

# 3. Many items, the drag held near the bottom edge: the content scrolls.
sh plan/tools/files/host-run.sh --fresh "--start=$project" > /dev/null 2>&1
index=0
while [ $index -lt 150 ]; do
	: > "$project/z-file-$index.txt"
	index=$((index + 1))
done
KEEP=1 run before hits
KEEP=1 run edge press=$readme drag=640,132 drag=640,700 wait=50 wait=50 wait=50 wait=50 wait=50 wait=50 wait=50 wait=50 wait=50 wait=50 hits draw="$out/edge.ppm" key=1
first_before=$(sed -n 's/^hit kind=2 index=0 x=[0-9-]* y=\([0-9-]*\) .*/\1/p' "$out/before.txt" | head -1)
first_after=$(sed -n 's/^hit kind=2 index=0 x=[0-9-]* y=\([0-9-]*\) .*/\1/p' "$out/edge.txt" | head -1)
if [ -n "$first_before" ] && { [ -z "$first_after" ] || [ "$first_after" -lt "$first_before" ]; }; then
	echo "edge: the content scrolled (first item y $first_before -> ${first_after:-gone}) ok"
else
	echo "edge: the content scrolled (first item y ${first_before:-?} -> ${first_after:-?}) MISSING"
	status=1
fi
rm -f "$project"/z-file-*.txt

[ $status = 0 ] && echo "host-spring: PASS" || echo "host-spring: FAIL"
exit $status
