#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Starts only the simulated radios in the disposable Linux guest.
set -eu
modprobe mac80211_hwsim radios=2
cat > /tmp/keiland-hostapd.conf <<'AP'
interface=wlan1
driver=nl80211
ssid=keiland-test
hw_mode=g
channel=6
wpa=2
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
wpa_passphrase=keiland-pass
AP
cat > /tmp/keiland-wpa.conf <<'STA'
ctrl_interface=DIR=/run/wpa_supplicant GROUP=netdev
update_config=1
STA
hostapd -B -P /tmp/keiland-hostapd.pid /tmp/keiland-hostapd.conf
wpa_supplicant -B -i wlan0 -c /tmp/keiland-wpa.conf -P /tmp/keiland-wpa.pid
# DHCP is outside Keiland; a fixed address belongs solely to this radio fixture.
ip address add 192.0.2.2/24 dev wlan0
sleep 2
ls -l /run/wpa_supplicant
wpa_cli -i wlan0 status
