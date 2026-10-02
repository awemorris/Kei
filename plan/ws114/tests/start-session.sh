#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Run only in the dedicated q580 Debian overlay, after installing the package.
set -eu
mkdir -p /run/p9-gtk
chmod 700 /run/p9-gtk
export XDG_RUNTIME_DIR=/run/p9-gtk WAYLAND_DISPLAY=wayland-q580
export GDK_BACKEND=wayland XDG_SESSION_TYPE=wayland XDG_CURRENT_DESKTOP=Keiland
DBUS_SESSION_BUS_ADDRESS=$(dbus-daemon --session --fork --print-address)
export DBUS_SESSION_BUS_ADDRESS
printf 'export XDG_RUNTIME_DIR=%s WAYLAND_DISPLAY=%s GDK_BACKEND=%s XDG_SESSION_TYPE=%s XDG_CURRENT_DESKTOP=%s\n' "$XDG_RUNTIME_DIR" "$WAYLAND_DISPLAY" "$GDK_BACKEND" "$XDG_SESSION_TYPE" "$XDG_CURRENT_DESKTOP" > /tmp/q580-session.env
printf "export DBUS_SESSION_BUS_ADDRESS='%s'\n" "$DBUS_SESSION_BUS_ADDRESS" >> /tmp/q580-session.env
openvt -c 10 -s -- sh -c 'exec env KEILAND_SEAT=direct /opt/keiland/bin/wayland --socket=/run/p9-gtk/wayland-q580 --session --glass --wallpaper=/opt/keiland/share/keiland/wallpaper.ppm > /tmp/q580-compositor.log 2>&1'
