#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Run only in the P2-owned p008 Debian overlay, after installing the staged desktop.
set -eu
mkdir -p /run/p2-p008
chmod 700 /run/p2-p008
export XDG_RUNTIME_DIR=/run/p2-p008 WAYLAND_DISPLAY=wayland-p008
export GDK_BACKEND=wayland XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=Keiland
DBUS_SESSION_BUS_ADDRESS=$(dbus-daemon --session --fork --print-address)
export DBUS_SESSION_BUS_ADDRESS
printf 'export XDG_RUNTIME_DIR=%s WAYLAND_DISPLAY=%s GDK_BACKEND=%s XDG_SESSION_TYPE=%s XDG_CURRENT_DESKTOP=%s\n' "$XDG_RUNTIME_DIR" "$WAYLAND_DISPLAY" "$GDK_BACKEND" "$XDG_SESSION_TYPE" "$XDG_CURRENT_DESKTOP" > /tmp/p008-session.env
printf "export DBUS_SESSION_BUS_ADDRESS='%s'\n" "$DBUS_SESSION_BUS_ADDRESS" >> /tmp/p008-session.env
openvt -f -c 10 -s -- sh -c 'exec env KEILAND_SEAT=direct /opt/keiland/bin/wayland --socket=/run/p2-p008/wayland-p008 --session --glass --wallpaper=/opt/keiland/share/keiland/wallpaper.ppm > /tmp/p008-compositor.log 2>&1'
