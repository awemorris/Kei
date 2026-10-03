#!/bin/sh
# ws095: builds and runs the host tests of the input method's engines (host-engine.c)
# with the engines' sources, on Linux, under ASan and UBSan.  REmacs's SKK-JISYO.X is
# read too when it is at hand (the shared build/sources/remacs, read only).
#   sh plan/ws095/tests/host-engine.sh [OUTPUT]   (default build/ws095/host-engine)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -e
cd "$(dirname "$0")/../../.."
out=${1:-build/ws095/host-engine}
mkdir -p "$(dirname "$out")"
D=userland/desktop/ime
${CC:-clang} -std=c11 -D_GNU_SOURCE -O1 -g -Wall -Wextra -Werror -Wdeclaration-after-statement \
	-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -I$D \
	plan/ws095/tests/host-engine.c $D/output.c $D/engine-direct.c $D/ja-kana.c $D/ja-romaji.c \
	$D/ja-dict.c $D/ja-user.c $D/ja-inflect.c $D/ja-segment.c $D/ja-engine.c $D/ja-keys.c -pthread -o "$out"
x=
for candidate in build/sources/remacs/dict/SKK-JISYO.X /home/awe/zedBSD-rpi4/build/sources/remacs/dict/SKK-JISYO.X; do
	if [ -f "$candidate" ]; then x=$candidate; break; fi
done
"$out" plan/ws095/tests/ja-test.dict $x
