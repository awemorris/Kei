#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Installs a staged desktop into the disposable guest overlay.
set -eu
STAGE=${1:-build/keiland-linux/stage}
TOOLS=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
test -d "$STAGE/opt/keiland"
ARCHIVE=$(mktemp)
trap 'rm -f "$ARCHIVE"' EXIT HUP INT TERM
tar -C "$STAGE/opt" -cf "$ARCHIVE" keiland
sh "$TOOLS/guest.sh" put "$ARCHIVE" /tmp/keiland-install.tar
sh "$TOOLS/guest.sh" ssh 'mkdir -p /opt; rm -rf /opt/keiland; tar -C /opt -xf /tmp/keiland-install.tar; rm /tmp/keiland-install.tar'
if [ -f "$STAGE/usr/share/wayland-sessions/keiland.desktop" ]; then
	sh "$TOOLS/guest.sh" ssh 'mkdir -p /usr/share/wayland-sessions'
	sh "$TOOLS/guest.sh" put "$STAGE/usr/share/wayland-sessions/keiland.desktop" /usr/share/wayland-sessions/keiland.desktop
fi
