#!/bin/sh
# BUG-027 (ws046-p015): the SSH guest image (plan/tools/guest/config-amd64-ssh.mk) with the file-fault measurement:
# /bin/ffault and /bin/kbench (plan/tools/kbench, built against BUILD's libc), and a copy of the zedBSD libLLVM.so.23.1
# (80 MiB, the ticket's file) at /var/bug027/libLLVM.so.23.1 as plain data.  The library is taken read-only from
# LLVM_LIBRARY (default: main's clang package stage); nothing of the toolchain is built here.
#
#   plan/ws046/tests/bug027/build-bug027-image.sh BUILD
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:?usage: build-bug027-image.sh BUILD}
library=${LLVM_LIBRARY:-/home/awe/zedBSD-claude1/build/packages/clang/stage/usr/lib/libLLVM.so.23.1}
[ -f "$library" ] || { echo "build-bug027-image: no $library"; exit 1; }
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-bug027-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }

# The image first (BUILD's libc is what the programs link against), then the programs and the image again with them.
make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/tools/guest/config-amd64-ssh.mk BUILD="$build" "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
mkdir -p "$build/bug027"
bash plan/tools/kbench/build.sh "$build" "$build/bug027/ffault" ffault
bash plan/tools/kbench/build.sh "$build" "$build/bug027/kbench" kbench
cp "$library" "$build/bug027/libLLVM.so.23.1"
extra="$extra --file /bin/ffault=$build/bug027/ffault --file /bin/kbench=$build/bug027/kbench"
extra="$extra --file /var/bug027/libLLVM.so.23.1=$build/bug027/libLLVM.so.23.1"
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/tools/guest/config-amd64-ssh.mk BUILD="$build" "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
