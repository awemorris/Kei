#!/bin/sh
# ws128-p010 (BUG-150): the host test of Terminal's right margin.
#  1. wrap: a character in the last column leaves the wrap pending (xterm's am and xenl).
#  2. emacs: the first frame of `TERM=xterm emacs -nw -Q` on an 80x24 pty, captured on
#     FreeBSD 15.1 (Emacs 31.1, /etc/termcap.db xterm) and on Debian 13 (Emacs 30.1, ncurses
#     xterm) with terminal-p010-capture.py, replayed into the terminal's own screen.c: the menu
#     bar on row 1, the mode line on row 23, the cursor at row 2.
#
#   plan/ws128/tests/terminal-p010.sh [OUTDIR]
#   KEILAND_INCLUDE=DIR   the native build's include directory (default: main's build/keiland-linux
#                         on Linux, build/native-p2 under the source tree on FreeBSD)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
out=${1:-build/ws128-p010}
mkdir -p "$out"
status=0
# The test program, from the terminal's own sources, with the native build's warnings.
if [ "$(uname -s)" = FreeBSD ]; then
	include=${KEILAND_INCLUDE:-build/native-p2/include}
	set -- cc -std=gnu17 -O2 -Wall -Wextra -Werror -idirafter /usr/local/include
else
	include=${KEILAND_INCLUDE:-/home/awe/zedBSD-claude1/build/keiland-linux/include}
	set -- gcc -std=gnu17 -O2 -Wall -Wextra -Werror -Wno-format-truncation -D_GNU_SOURCE
fi
timeout 120 "$@" -DKEILAND_DATADIR='"/opt/keiland/share"' -DKEILAND_BINDIR='"/opt/keiland/bin"' \
	-I. -Iuserland/desktop/keiland -I"$include" \
	plan/ws128/tests/terminal-p010.c userland/desktop/terminal/screen.c \
	userland/desktop/terminal/width.c \
	-o "$out/terminal-p010" || { echo "build: FAIL"; exit 1; }
echo "build: ok"

echo "== wrap"
timeout 30 "$out/terminal-p010" wrap || status=1
# FreeBSD's Emacs 31.1 shows an empty *scratch* with point on row 2; Debian's Emacs 30.1 shows
# the three lines of initial-scratch-message with point on row 5.
echo "== emacs (freebsd)"
timeout 30 "$out/terminal-p010" emacs plan/ws128/tests/terminal-p010-emacs-freebsd.bin 2 "" || status=1
echo "== emacs (linux)"
timeout 30 "$out/terminal-p010" emacs plan/ws128/tests/terminal-p010-emacs-linux.bin 5 ";; This buffer is for text" || status=1

if [ $status -eq 0 ]; then echo "terminal-p010: PASS"; else echo "terminal-p010: FAIL"; fi
exit $status
