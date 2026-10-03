#!/bin/sh
# ws128-p009: the host test of Terminal's "Treat Ambiguous-Width Characters as Wide".
#  1. table: every code point 0..10FFFF, with the setting off and on, against the Unicode 17.0.0
#     data (EastAsianWidth.txt and UnicodeData.txt from unicode.org, SHA-256 checked by
#     ambiguous-gen.py): off is the terminal's wide/fullwidth rule as before p009; on adds every
#     A character that is not Mn, Me, Cf or Cc.  ambiguous.h is also regenerated and compared.
#  2. check: the grid (widths, cursor, cells written before a change keep theirs) and the
#     settings file (~/.config/keiland/terminal.conf under a scratch HOME).
#  3. speed: the same 45 MB output with the setting off and on (reported; fails above 1.25x).
#
#   plan/ws128/tests/terminal-p009.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
out=${1:-build/ws128-p009}
mkdir -p "$out"
status=0
main_build=${KEILAND_LINUX_INCLUDE:-/home/awe/zedBSD-claude1/build/keiland-linux/include}

# The test program, from the terminal's own sources, with the Linux build's warnings.
timeout 120 gcc -std=gnu17 -O2 -Wall -Wextra -Werror -Wno-format-truncation -D_GNU_SOURCE \
	-DKEILAND_DATADIR='"/opt/keiland/share"' -DKEILAND_BINDIR='"/opt/keiland/bin"' \
	-I. -Iuserland/desktop/keiland -I"$main_build" \
	plan/ws128/tests/terminal-p009.c userland/desktop/terminal/screen.c \
	userland/desktop/terminal/width.c userland/desktop/terminal/settings.c \
	-o "$out/terminal-p009" || { echo "build: FAIL"; exit 1; }
echo "build: ok"

# The Unicode data, fetched once into the output directory and checked.
if [ ! -f "$out/ucd/EastAsianWidth.txt" ]; then
	(cd "$out" && timeout 120 python3 "$OLDPWD/userland/desktop/terminal/ambiguous-gen.py" --fetch) || { echo "fetch: FAIL"; exit 1; }
fi

# ambiguous.h is what the generator makes from that data.
timeout 60 python3 userland/desktop/terminal/ambiguous-gen.py "$out/ucd" > "$out/ambiguous.h" || status=1
if cmp -s "$out/ambiguous.h" userland/desktop/terminal/ambiguous.h; then
	echo "generated table: ok"
else
	echo "generated table: FAIL (ambiguous.h differs from the generator's output)"
	status=1
fi

# 1. Every code point, off and on.
timeout 120 "$out/terminal-p009" table > "$out/table.txt" || status=1
timeout 120 python3 - "$out/ucd" "$out/table.txt" <<'EOF' || status=1
import sys
ucd, table = sys.argv[1], sys.argv[2]
category = {}
first = None
for line in open(ucd + "/UnicodeData.txt", encoding="utf-8"):
    f = line.split(";")
    code = int(f[0], 16)
    if f[1].endswith(", First>"):
        first = code
        continue
    if f[1].endswith(", Last>"):
        for c in range(first, code + 1):
            category[c] = f[2]
        continue
    category[code] = f[2]
ambiguous = set()
for line in open(ucd + "/EastAsianWidth.txt", encoding="utf-8"):
    line = line.split("#", 1)[0].strip()
    if not line:
        continue
    span, width = [p.strip() for p in line.split(";")]
    if width != "A":
        continue
    lo, _, hi = span.partition("..")
    for c in range(int(lo, 16), int(hi or lo, 16) + 1):
        if category.get(c, "Cn") not in ("Mn", "Me", "Cf", "Cc"):
            ambiguous.add(c)
# The terminal's wide and fullwidth rule before p009 (screen_wide, now width_east_asian_wide).
old = [(0x1100, 0x115f), (0x2329, 0x232a), (0x2e80, 0xa4cf), (0xac00, 0xd7a3), (0xf900, 0xfaff),
       (0xfe10, 0xfe6f), (0xff01, 0xff60), (0xffe0, 0xffe6), (0x20000, 0x3fffd)]
wide = set()
for lo, hi in old:
    wide.update(range(lo, hi + 1))
got = {"off": set(), "on": set()}
for line in open(table):
    name, lo, hi = line.split()
    got[name].update(range(int(lo, 16), int(hi, 16) + 1))
ok = True
if got["off"] != wide:
    print("table off: FAIL (%d differ)" % len(got["off"] ^ wide)); ok = False
else:
    print("table off: ok (%d wide, same as before p009)" % len(wide))
want = wide | ambiguous
if got["on"] != want:
    diff = sorted(got["on"] ^ want)
    print("table on: FAIL (%d differ, first %s)" % (len(diff), [hex(c) for c in diff[:8]])); ok = False
else:
    print("table on: ok (%d wide, %d Ambiguous added)" % (len(want), len(want - wide)))
zero = [c for c in got["on"] - wide if category.get(c, "Cn") in ("Mn", "Me", "Cf", "Cc")]
print("on: no Mn/Me/Cf/Cc made wide: %s" % ("ok" if not zero else "FAIL %d" % len(zero)))
if zero:
    ok = False
sys.exit(0 if ok else 1)
EOF

# 2. The grid and the settings file, under a scratch home.
home="$out/home"
rm -rf "$home"
mkdir -p "$home"
HOME="$PWD/$home" timeout 60 "$out/terminal-p009" check || status=1

# 3. The speed of output, off and on.
timeout 300 "$out/terminal-p009" speed | tee "$out/speed.txt"
ratio=$(sed -n 's/.*ratio=\([0-9.]*\).*/\1/p' "$out/speed.txt")
if awk "BEGIN { exit !(${ratio:-9} <= 1.25) }"; then
	echo "speed: ok"
else
	echo "speed: FAIL (on is ${ratio}x off)"
	status=1
fi

if [ $status -eq 0 ]; then echo "terminal-p009: PASS"; else echo "terminal-p009: FAIL"; fi
exit $status
